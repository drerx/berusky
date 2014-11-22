/*
 *        .þÛÛþ þ    þ þÛÛþ.     þ    þ þÛÛÛþ.  þÛÛÛþ .þÛÛþ. þ    þ
 *       .þ   Û Ûþ.  Û Û   þ.    Û    Û Û    þ  Û.    Û.   Û Ûþ.  Û
 *       Û    Û Û Û  Û Û    Û    Û   þ. Û.   Û  Û     Û    Û Û Û  Û
 *     .þþÛÛÛÛþ Û  Û Û þÛÛÛÛþþ.  þþÛÛ.  þþÛÛþ.  þÛ    Û    Û Û  Û Û
 *    .Û      Û Û  .þÛ Û      Û. Û   Û  Û    Û  Û.    þ.   Û Û  .þÛ
 *    þ.      þ þ    þ þ      .þ þ   .þ þ    .þ þÛÛÛþ .þÛÛþ. þ    þ
 *
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>

#include "portability.h"

#ifdef LINUX
#include <sys/wait.h>
#elif WINDOWS
#include <stdio.h>
#include <process.h>
#endif

#include "berusky.h"
#include "berusky_gui.h"
#include "main.h"
#include "editor.h"

ITEM_REPOSITORY *editor_panel::p_repo = NULL;
ITEM_REPOSITORY *editor_panel_slot::p_repo = NULL;

void editor_panel_slot::draw(tpos x, tpos y, bool highlighted, bool selected)
{
  p_grf->fill(x,y,EDITOR_ITEM_SIZE_X,EDITOR_ITEM_SIZE_Y,0);

  assert(p_repo->item_valid(item) && p_repo->variant_valid(item,variant));
  
  #define COLOR_HIGHLIGHTED_R   0
  #define COLOR_HIGHLIGHTED_G   0
  #define COLOR_HIGHLIGHTED_B   200
  
  #define COLOR_SELECTED_R      175
  #define COLOR_SELECTED_G      0
  #define COLOR_SELECTED_B      0
  
  if(highlighted) {
    p_grf->fill(x,y,EDITOR_ITEM_SIZE_X,EDITOR_ITEM_SIZE_Y,
                p_grf->color_map(COLOR_HIGHLIGHTED_R,
                COLOR_HIGHLIGHTED_G, COLOR_HIGHLIGHTED_B));
  }
  if(selected) {
    p_grf->fill(x,y,EDITOR_ITEM_SIZE_X,EDITOR_ITEM_SIZE_Y, 
                p_grf->color_map(COLOR_SELECTED_R,
                COLOR_SELECTED_G, COLOR_SELECTED_B));
  }

  tpos x_shift = EDITOR_ITEM_SIZE_X/2-CELL_SIZE_X/2;
  tpos y_shift = EDITOR_ITEM_SIZE_Y/2-CELL_SIZE_Y/2;

  p_repo->draw(x+x_shift,y+y_shift,item,variant,0);
}

void editor_panel::clear(void)
{
  ffree(p_slots);
  slot_num = 0;

  visible_slot_first = 0;
  visible_slot_highlighted = 0;
  visible_slot_selected = 0;
}

bool editor_panel::slot_return(tpos x, tpos y, int &slot)
{
  tpos sx = start_x;
  tpos sy = start_y;

  int i;
  for(i = 0; i < visible_slot_num; i++) {
    if(!slot_valid(i+visible_slot_first))
      return(FALSE);
    
    tpos tx = sx + dx*i;
    tpos ty = sy + dy*i;
    
    if(INSIDE(tx, x, EDITOR_ITEM_SIZE_X) && 
       INSIDE(ty, y, EDITOR_ITEM_SIZE_Y)) 
    {
      slot = i+visible_slot_first;
      return(TRUE);
    }
  }
  
  return(FALSE);
}

void editor_panel::panel_draw(bool draw)
{
  RECT r = boundary_get();

  p_grf->fill(&r,0);
  int i;
  for(i = 0;
      i < visible_slot_num; 
      i++)
  {
    int slot = i+visible_slot_first;
    if(slot_valid(slot)) {
      RECT r = boundary_get();
      p_slots[slot].draw(r.x + dx*i,
                         r.y + dy*i,
                         slot == visible_slot_highlighted,
                         slot == visible_slot_selected);
    }
  }
  if(draw)
    p_grf->redraw_add(&r);

//  controls_draw(draw);
}

void editor_panel::panel_scroll(int direction, EDITOR_SELECTION *p_sel, bool redraw)
{  
  int slot_first_new;

  // No need to scroll
  if(slot_num <= visible_slot_num)
    return;

  if(direction == ITEMS_START) {
    slot_first_new = 0;
  }
  else if(direction == ITEMS_END) {
    slot_first_new = visible_slot_first - visible_slot_num;
  }
  else {  
    slot_first_new = visible_slot_first + direction;
  }
  
  if(slot_first_new < 0) {
    slot_first_new = 0;
  }
  
  if(slot_first_new == visible_slot_first) {
    // no scroll - don't change it
    return;
  }

  panel_set(slot_first_new, p_sel, TRUE, redraw);
}

RECT editor_panel::boundary_get(void)
{
  RECT r;

  tpos sx = start_x;
  tpos sy = start_y;

  if(dx) {
    r.x = sx;
    r.y = sy;
    r.w = dx*visible_slot_num;
    r.h = EDITOR_ITEM_SIZE_Y;
  } else {
    r.x = sx;
    r.y = sy;
    r.w = EDITOR_ITEM_SIZE_X;
    r.h = dy*visible_slot_num;
  }

  return r;
}

void editor_panel::register_controls_events(INPUT *p_input)
{
  RECT r = boundary_get();

  assert(!p_registered_events);

  #define EDITOR_PANEL_EVENTS 5
  MOUSE_EVENT events[EDITOR_PANEL_EVENTS] = 

  { MOUSE_EVENT(MOUSE_STATE(r,MASK_WHEEL_UP),
    MEVENT_ACTIVATE_ONCE|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS,
    LEVEL_EVENT(ED_LEVEL_IPANEL_SCROLL, panel_handle,-1)),
  
    MOUSE_EVENT(MOUSE_STATE(r,MASK_WHEEL_DOWN),
    MEVENT_ACTIVATE_ONCE|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS,
    LEVEL_EVENT(ED_LEVEL_IPANEL_SCROLL, panel_handle, 1)),
    
    MOUSE_EVENT(MOUSE_STATE(r),
    MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_IN, 
    panel_handle),
  
    MOUSE_EVENT(MOUSE_STATE(r),
    MEVENT_MOUSE_EXTERNAL|MEVENT_ACTIVATE_ONCE|MEVENT_MOUSE_OUT,
    panel_handle),
  
    MOUSE_EVENT(MOUSE_STATE(r,MASK_BUTTON_LEFT),
    MEVENT_MOUSE_EXTERNAL|MEVENT_ACTIVATE_ONCE|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS, 
    panel_handle)
  };

  p_registered_events = p_input->mevent_add(events, EDITOR_PANEL_EVENTS);
}

void editor_panel::unregister_controls_events(INPUT *p_input)
{
  assert(p_registered_events);
  p_input->mevent_remove(p_registered_events, EDITOR_PANEL_EVENTS);
  p_registered_events = NULL;
}

item_handle editor_panel::item_base_get(item_handle item)
{
  if(item >= P_PLAYER_1 && item <= P_PLAYER_5)
    return(P_PLAYER_1);
  else if(item >= P_KEY1 && item <= P_KEY5)
    return(P_KEY1);
  else if(item >= P_DOOR1_H_O && item <= P_DOOR5_V_Z)
    return(P_DOOR1_H_O);  
  else if(item >= P_ID_DOOR1_H_O && item <= P_ID_DOOR5_V_Z)
    return(P_ID_DOOR1_H_O);
  else if(item >= P_DV_H_O && item <= P_DV_V)
    return(P_DV_H_O);
  
  return(item);
}

item_panel::item_panel(int panel_item_num,
                       DIRECTION direction,
                       tpos start_x, tpos start_y, 
                       int handle,
                       VARIANT_PANEL *p_var)
: editor_panel(panel_item_num, direction, start_x, start_y, handle)  
{
  attached_panel_set(p_var);

  slot_num = 14;
  p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);

  int i = 0;

  p_slots[i].item = P_GROUND;
  p_slots[i].variant = 0;
  i++;

  p_slots[i].item = P_PLAYER_1;
  p_slots[i].variant = 0;
  i++;

  p_slots[i].item = P_BOX;
  p_slots[i].variant = 0;
  i++;

  p_slots[i].item = P_BOX_LIGHT;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_TNT;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_WALL;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_EXIT;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_STONE;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_KEY;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_MATTOCK;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_KEY1;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_DOOR1_H_O;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_ID_DOOR1_H_O;
  p_slots[i].variant = 0;
  i++;
  
  p_slots[i].item = P_DV_H_O;
  p_slots[i].variant = 0;
  i++;
  
  assert(i == slot_num);
}

/* Select the select item by variant panel
*/
void variant_panel::configure(int item)
{
  clear();

  // Configure the variant panel according the master (item) panel

  /* Item rendered as default:
  #define P_GROUND                  0
  #define P_EXIT                    9
  #define P_STONE                   10
  #define P_KEY                     11
  #define P_MATTOCK                 12
  */
  if(item == P_GROUND ||
     item == P_EXIT ||
     item == P_STONE ||
     item == P_KEY ||
     item == P_MATTOCK)
  {
    slot_num = p_repo->variants_get(item);
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = item;
      p_slots[i].variant = i;
    }
  
  /* Player are in one line
  */  
  } else if(item >= P_PLAYER_1 && item <= P_PLAYER_5) {
    slot_num = 5;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_PLAYER_1+i;
      p_slots[i].variant = 0;
    }
  
  /* Boxes
  */
  } else if(item == P_BOX) {
    slot_num = p_repo->variants_get(item);
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_BOX;
      p_slots[i].variant = i;      
    }
  
  /* Light boxes
  */
  } else if(item == P_BOX_LIGHT) {
    slot_num = p_repo->variants_get(item);
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_BOX_LIGHT;
      p_slots[i].variant = i;
    }
  
  /* TNT
  */
  } else if(item == P_TNT) {
    slot_num = p_repo->variants_get(item);
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_TNT;
      p_slots[i].variant = i;
    }
  
  /* WALL
  */
  } else if(item == P_WALL) {
    int variants = p_repo->variants_get(item);
    slot_num = variants - 67;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    int slot = 0;
  
    for(int i = 0; i <= 24; i++) {
      p_slots[slot].item = P_WALL;
      p_slots[slot].variant = i;
      slot++;
    }
    for(int i = 52; i <= 65; i++) {
      p_slots[slot].item = P_WALL;
      p_slots[slot].variant = i;
      slot++;
    }
    for(int i = 106; i < variants; i++) {
      p_slots[slot].item = P_WALL;
      p_slots[slot].variant = i;
      slot++;
    }  
    assert(slot == slot_num);
  
  /* Color keys
  */
  } else if(item >= P_KEY1 && item <= P_KEY5) {
    slot_num = 5;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_KEY1+i;
      p_slots[i].variant = 0;
    }
  
  /* key doors
  */
  } else if(item >= P_DOOR1_H_O && item <= P_DOOR5_V_Z) {
    slot_num = 20;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_DOOR1_H_O+i;
      p_slots[i].variant = 0;
    }
  
  /* Color doors
  */
  } else if(item >= P_ID_DOOR1_H_O && item <= P_ID_DOOR5_V_Z) {
    slot_num = 20;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_ID_DOOR1_H_O+i;
      p_slots[i].variant = 0;
    }
  
  /* One way doors
  */
  } else if(item >= P_DV_H_O && item <= P_DV_V) {
    slot_num = 6;
    p_slots = (EDITOR_PANEL_SLOT *)mmalloc(sizeof(EDITOR_PANEL_SLOT)*slot_num);
    for(int i = 0; i < slot_num; i++) {
      p_slots[i].item = P_DV_H_O+i;
      p_slots[i].variant = 0;
    }
  }
}

editor_layer_config::editor_layer_config(void)
{

}

editor_gui::editor_gui(ITEM_REPOSITORY *p_repo_, DIR_LIST *p_dir_):
  gui_base(), 
  p_dir(p_dir_),
  p_repo(p_repo_),
  level(p_repo_),
  console(&input,CONSOLE_X,CONSOLE_Y,CONSOLE_DX,CONSOLE_LINES),
  console_callback(NULL),
  console_callback_id(0),
  p_side_event_first(NULL),
  panel_items(ITEMS_IN_PANEL, VERTICAL, 0, 0, PANEL_HANDLE_ITEMS, &panel_variants),
  panel_variants(ITEMS_IN_PANEL+2, HORIZONTAL, EDITOR_ITEM_SIZE_X, 0, PANEL_HANDLE_VARIANTS)
{
  editor_panel::set_up(p_repo_);
  editor_panel_slot::set_up(p_repo_);
    
  input.mevent_clear();
  input.events_wait(TRUE);

  draw_level = TRUE;
  draw_panels = TRUE;

  // Clear whole screen
  p_grf->clear();
    
  panel_items.register_controls_events(&input);  
  panel_variants.register_controls_events(&input);  

  level_config();

  layer_active_set(ALL_LEVEL_LAYERS, FALSE);
  side_menu_create();
  //layer_status_create();
  level.back_max_set(background_num(p_dir));
}

editor_gui::~editor_gui(void)
{
}

void editor_gui::editor_init(void)
{
}

// Panel interface
void editor_gui::panel_item_select(int panel, tpos x, tpos y)
{
  if(panel == PANEL_HANDLE_ITEMS) {
    panel_items.slot_select(x,y,&selected_editor_item,TRUE,TRUE);
  }
  else {
    panel_variants.slot_select(x,y,&selected_editor_item,TRUE,TRUE);
  }
  
  // Draw the current selection
  selection_draw(TRUE);

  // Set active layer
  layer_active_set(p_repo->item_get_layer(selected_editor_item.item), TRUE);
}

void editor_gui::panel_item_highlight(int panel, tpos x, tpos y)
{
  if(panel == PANEL_HANDLE_ITEMS) {
    panel_items.slot_highlight(x,y,TRUE);
  }
  else {
    panel_variants.slot_highlight(x,y,TRUE);
  }
}

void editor_gui::panel_draw(bool draw)
{
  panel_items.panel_draw(draw);
  panel_variants.panel_draw(draw);
}

void editor_gui::panel_scroll(int panel, int direction)
{
  if(panel == PANEL_HANDLE_ITEMS) {
    panel_items.panel_scroll(direction,&selected_editor_item,TRUE);
  }
  else {
    panel_variants.panel_scroll(direction,&selected_editor_item,TRUE);
  }
  selection_draw(TRUE);
}

void editor_gui::panel_scroll_mouse(int direction)
{
  MOUSE_STATE *p_state = input.mouse_state_get();

  if(p_state->in_rect(panel_items.boundary_get())) {
    panel_scroll(PANEL_HANDLE_ITEMS, direction);
  }
  else if(p_state->in_rect(panel_variants.boundary_get())) {
    panel_scroll(PANEL_HANDLE_VARIANTS, direction);
  }
}

/*
#define EDIT_ITEM_PICT_START_X   (EDIT_ITEM_START_X)
#define EDIT_ITEM_PICT_START_Y   (EDIT_ITEM_START_Y+EDIT_ITEM_DY+10)
*/
void editor_gui::selection_draw(bool draw)
{   
  p_font->select(FONT_DEFAULT);
  p_font->alignment_set(MENU_LEFT);
  p_grf->fill(EDIT_ITEM_START_X,EDIT_ITEM_START_Y,
              EDIT_ITEM_DX,EDIT_ITEM_DY,0);
  p_font->print(NULL,EDIT_ITEM_START_X,EDIT_ITEM_START_Y,_("I:%d V:%d R:%d L:%d"),
                selected_editor_item.item, 
                selected_editor_item.variant, 
                selected_editor_item.rotation, 
                p_repo->item_get_layer(selected_editor_item.item));  
  if(draw)
    p_grf->redraw_add(EDIT_ITEM_START_X,EDIT_ITEM_START_Y,EDIT_ITEM_DX,EDIT_ITEM_DY);

  {
    #define COLOR_BACK_R      100
    #define COLOR_BACK_G      100
    #define COLOR_BACK_B      100
  
    RECT r = {EDIT_ITEM_PICT_START_X,
              EDIT_ITEM_PICT_START_Y,
              EDITOR_ITEM_SIZE_X,
              EDITOR_ITEM_SIZE_Y};
  
    p_grf->fill(&r,p_grf->color_map(COLOR_BACK_R, COLOR_BACK_G, COLOR_BACK_B));
    p_repo->draw(EDIT_ITEM_PICT_START_X+EDITOR_ITEM_SIZE_X/2-CELL_SIZE_X/2,
                 EDIT_ITEM_PICT_START_Y+EDITOR_ITEM_SIZE_Y/2-CELL_SIZE_Y/2,
                 selected_editor_item.item,
                 selected_editor_item.variant,
                 selected_editor_item.rotation);
    if(draw)
      p_grf->redraw_add(&r);
  }
  {
    RECT r = {EDIT_ITEM_PICT_START_X+EDITOR_ITEM_SIZE_X+10,
              EDIT_ITEM_PICT_START_Y,
              EDIT_ITEM_DX-10,
              3*20};
    RECT dr;
  
    p_grf->fill(&r,0);
    p_font->print(&dr,r.x,r.y,_("Item: %s\nVariation: %d\nRotation: %s"),
            p_repo->item_get_name(selected_editor_item.item),
            selected_editor_item.variant,
            p_repo->item_get_rotation(selected_editor_item.rotation));
    r.h = dr.h;
    if(draw)
      p_grf->redraw_add(&r);
  }  
}

/* Draw the status line
*/
void editor_gui::selection_cursor_draw_status(bool draw, char *p_text,...)
{
  char      text[200];
  va_list   arguments;  

  va_start(arguments,p_text);
  vsnprintf(text,200,p_text,arguments);
  va_end(arguments);

  p_font->select(FONT_DEFAULT);
  p_font->alignment_set(MENU_LEFT);
  p_grf->fill(EDIT_COORD_START_X,EDIT_COORD_START_Y,
              EDIT_COORD_DX,EDIT_COORD_DY,0);
  p_font->print(NULL,EDIT_COORD_START_X,EDIT_COORD_START_Y,text);
  if(draw) {
    p_grf->redraw_add(EDIT_COORD_START_X,EDIT_COORD_START_Y,
                      EDIT_COORD_DX,EDIT_COORD_DY);
  }
}

/* draw screen cursor */
void editor_gui::selection_cursor_draw(bool draw)
{
  tpos x, y;
  tpos sx,sy;
  tpos dx,dy;

  if(draw_panels) {
    if(level.selection_get()) {
      level.selection_get(&x,&y);
      if(level.selection_rectangle_get()) {
        level.selection_rectangle_get(&sx,&sy,&dx,&dy);
        selection_cursor_draw_status(draw,_("selection %d,%d - %d,%d"),sx,sy,dx,dy);
      }
      else {
        selection_cursor_draw_status(draw,_("level cursor %d x %d"),x,y);
      }
    }
    else {
      selection_cursor_draw_status(draw,_("No selection"));
    }
  }
}

/* Side menu - general
*/
/*
#define SIDE_MENU_X         (EDITOR_SCREEN_START_X+(DOUBLE_SIZE ? GAME_RESOLUTION_X/2+300 : GAME_RESOLUTION_X)+10)
#define SIDE_MENU_Y         (EDITOR_SCREEN_START_Y+(DOUBLE_SIZE ? 160 : 60))
#define SIDE_MENU_DX        (EDITOR_RESOLUTION_X-SIDE_MENU_X)
#define SIDE_MENU_DY        (EDITOR_RESOLUTION_Y-SIDE_MENU_Y)
#define SIDE_MENU_X_DIFF    0
#define SIDE_MENU_Y_DIFF    35

static char *side_menu[] = 
{ 
  _("help (f1)"),
  _("new level"),
  _("save (f2)"),
  _("save as (ct+f2)"),
  _("load (f3)"),
  _("quit (esc)"),
  _("run level (f9)"),
  _("undo (ctrl+u)"),
  _("redo (ctrl+r)"),
  _("rotate (shft+r)"),
  _("shade floor"),
  _("background (b)"),
};

void editor_gui::side_menu_create(void)
{
  p_font->print(NULL,SIDE_MENU_X, SIDE_MENU_Y, "Editor menu:");

  menu_item_set_pos(SIDE_MENU_X, SIDE_MENU_Y+SIDE_MENU_Y_DIFF);
  menu_item_set_diff(SIDE_MENU_X_DIFF, SIDE_MENU_Y_DIFF);

  menu_item_draw(side_menu[0], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_HELP));
  menu_item_draw(side_menu[1], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_NEW));
  menu_item_draw(side_menu[2], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_SAVE));
  menu_item_draw(side_menu[3], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_SAVE_AS));
  menu_item_draw(side_menu[4], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_LOAD));
  menu_item_draw(side_menu[5], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_QUIT));
  menu_item_draw(side_menu[6], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_RUN));
  menu_item_draw(side_menu[7], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_UNDO));
  menu_item_draw(side_menu[9], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_ROTATE_SELECTION));
  menu_item_draw(side_menu[10],MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_SHADER));
  menu_item_draw(side_menu[11],MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_CHANGE_BACKGROUND));

  //p_grf->redraw_add(SIDE_MENU_X,SIDE_MENU_Y,SIDE_MENU_DX,SIDE_MENU_DY);
}
*/

#define SIDE_MENU_X         (EDITOR_SCREEN_START_X+GAME_RESOLUTION_X/2+200)
#define SIDE_MENU_Y         (650)
#define SIDE_MENU_DX        (EDITOR_RESOLUTION_X-SIDE_MENU_X)
#define SIDE_MENU_DY        (EDITOR_RESOLUTION_Y-SIDE_MENU_Y)
#define SIDE_MENU_X_DIFF    0
#define SIDE_MENU_Y_DIFF    35

static char *side_menu[] = 
{ 
  _("help (f1)"),
  _("run level (f9)"),
  _("undo (ctrl+u)"),
  _("shade floor (ctrl+s)"),
  _("change background (b)"),
};

void editor_gui::side_menu_create(void)
{
  assert(!p_side_event_first);

  menu_item_set_pos(SIDE_MENU_X, SIDE_MENU_Y+SIDE_MENU_Y_DIFF);
  menu_item_set_diff(SIDE_MENU_X_DIFF, SIDE_MENU_Y_DIFF);
  
  LLIST_ITEM *p_last = input.mevents_get()->list_get_last();

  menu_item_draw(side_menu[0], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_HELP));
  menu_item_draw(side_menu[1], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_RUN));
  menu_item_draw(side_menu[2], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_UNDO));
  menu_item_draw(side_menu[3], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_SHADER));
  menu_item_draw(side_menu[4], MENU_LEFT, MENU_SAVE_BACK, LEVEL_EVENT(ED_LEVEL_CHANGE_BACKGROUND));

  p_side_event_first = p_last->list_next();
  side_event_num = llist_count(p_side_event_first);

  //p_grf->redraw_add(SIDE_MENU_X,SIDE_MENU_Y,SIDE_MENU_DX,SIDE_MENU_DY);
}

void editor_gui::side_menu_remove(void)
{
  assert(p_side_event_first);
  input.mevent_remove(reinterpret_cast<MOUSE_EVENT *>(p_side_event_first), side_event_num);
  p_side_event_first = NULL;
  side_event_num = 0;
}

void editor_gui::side_menu_draw(bool draw)
{
  menu_item_set_pos(SIDE_MENU_X, SIDE_MENU_Y+SIDE_MENU_Y_DIFF);
  menu_item_set_diff(SIDE_MENU_X_DIFF, SIDE_MENU_Y_DIFF);

  menu_item_draw(side_menu[0], MENU_LEFT, MENU_SAVE_BACK|MENU_DRAW_ONLY);
  menu_item_draw(side_menu[1], MENU_LEFT, MENU_SAVE_BACK|MENU_DRAW_ONLY);
  menu_item_draw(side_menu[2], MENU_LEFT, MENU_SAVE_BACK|MENU_DRAW_ONLY);
  menu_item_draw(side_menu[3], MENU_LEFT, MENU_SAVE_BACK|MENU_DRAW_ONLY);
  menu_item_draw(side_menu[4], MENU_LEFT, MENU_SAVE_BACK|MENU_DRAW_ONLY);
}

#define SIDE_STATUS_X     (GAME_RESOLUTION_X-400)
#define SIDE_STATUS_Y     (EDITOR_ITEM_SIZE_Y)
#define SIDE_STATUS_DX    320
#define SIDE_STATUS_DY    20

void editor_gui::layer_active_draw(bool draw)
{
  p_font->select(FONT_DEFAULT);
  p_font->alignment_set(MENU_LEFT);
  p_grf->fill(SIDE_STATUS_X, SIDE_STATUS_Y,
              SIDE_STATUS_DX, SIDE_STATUS_DY, 0);

  char *p_act = NULL;

  /* Side menu - layers
  */
  int active_layer = layer_active_get();
  switch(active_layer) {
    case LAYER_FLOOR:
      p_act = _("floor");
      break;
    case LAYER_ITEMS:
      p_act = _("items");
      break;
    case LAYER_PLAYER:
      p_act = _("players");
      break;
    case ALL_LEVEL_LAYERS:
      p_act = _("all");
      break;
    default:
      break;
  }
  if(p_act)
    p_font->print(NULL,SIDE_STATUS_X,SIDE_STATUS_Y,_("edited layer: %s"),p_act);
  if(draw)
    p_grf->redraw_add(SIDE_STATUS_X,SIDE_STATUS_Y,SIDE_STATUS_DX,SIDE_STATUS_DY);
}

/*
#define EDITOR_LAYER_STATUS_X     (ITEM_SIZE_X*2)
#define EDITOR_LAYER_STATUS_Y     (ITEM_SIZE_Y*2)
#define EDITOR_LAYER_STATUS_DX    (LEVEL_RESOLUTION_Y)
#define EDITOR_LAYER_STATUS_DY    (30)
*/
void editor_gui::layer_status_create(void)
{ //TODO
  RECT r = {EDITOR_LAYER_STATUS_X,
            EDITOR_LAYER_STATUS_Y,
            EDITOR_LAYER_STATUS_DX,
            EDITOR_LAYER_STATUS_DY};

  p_grf->fill(&r,0);

  static char *layer_names[ALL_LEVEL_LAYERS] = 
  {   
    _("Grid:"),
    _("Floor:"),
    _("Items:"),
    _("Players:")
  };

  static int layer_handle[] = 
  {
    LAYER_GRID,
    LAYER_FLOOR,
    LAYER_ITEMS,
    LAYER_PLAYER
  };

  #define LAYER_DIFF 174  

  menu_item_set_pos(r.x, r.y);
  menu_item_set_diff(LAYER_DIFF,0);

  int i;
  for(i = 0; i < ALL_LEVEL_LAYERS; i++) {
    menu_item_draw_text(layer_names[i], MENU_LEFT, 0, 
                        LEVEL_EVENT(ED_LEVEL_LAYER,layer_handle[i],LAYER_CHANGE));
    menu_get_last_rect(config.coord+i);
  }

  // p_grf->redraw_add(&r);
}

void editor_gui::layer_status_draw(bool draw)
{
  p_font->select(FONT_DEFAULT);
  p_font->alignment_set(MENU_LEFT);
  
  static char * layers_status[2] = 
  {
    _("off"),
    _("on")
  };

  static int layer_handle[] = 
  {
    LAYER_GRID,
    LAYER_FLOOR,
    LAYER_ITEMS,
    LAYER_PLAYER
  };

  #define LAYER_STATUS_DIFF   LAYER_DIFF
  #define LAYER_STATUS_OFFSET 120

  RECT r;
  int  i;
  for(i = 0; i < ALL_LEVEL_LAYERS; i++) {
    p_grf->fill(config.coord[i].x+config.coord[i].w,
                config.coord[i].y,50,config.coord[i].h,0);
    p_font->print(&r,config.coord[i].x+config.coord[i].w,
                  config.coord[i].y, 
                  layers_status[config.lc.get(layer_handle[i]) ? 1 : 0]);
    r.w += 20;
    if(draw)
      p_grf->redraw_add(&r);
  }  
}

void editor_gui::layer_status_switch(int layer, LAYER_STATE state)
{
  if(state == LAYER_CHANGE) {
    config.lc.negate(layer);
  } else {
    config.lc.set(layer,state);
  }
  level.set_layers(&config.lc);  
  draw();
}

void editor_gui::layer_active_set(int layer, bool draw)
{
  config.lc.set_active(layer);
  layer_active_draw(draw);
}

int editor_gui::layer_active_get(void)
{
  return(config.lc.get_active());
}

int editor_gui::layer_active_get(tpos x, tpos y)
{
  int active_layer;

  if((active_layer = layer_active_get()) == ALL_LEVEL_LAYERS) {
    int i;
    for(i = LEVEL_LAYERS-1; i >= 0; i--) {
      if(level.cell_is_full(x,y,i)) {
        return(i);
      }
    }
    return(LAYER_NONE);
  }
  else {
    return(active_layer);  
  }
}

void editor_gui::draw(bool force)
{
  if(force) {
    p_grf->fill(0,0,EDITOR_RESOLUTION_X,EDITOR_RESOLUTION_Y,0);
    level.level_redraw();
  }

  bool panel_refresh = level.level_draw();
  if(panel_refresh && draw_panels) {
    panel_draw(FALSE);
    selection_draw(FALSE);
    selection_cursor_draw(FALSE);
    console_draw(FALSE);
    side_menu_draw(FALSE);
    layer_active_draw(FALSE);  
    //layer_status_draw(FALSE);
  }

  if(force || panel_refresh) {
    p_grf->redraw_add(0,0,EDITOR_RESOLUTION_X,EDITOR_RESOLUTION_Y);
  }
}

void editor_gui::level_config(void)
{
  // Set coordinates of level
  level.window_offset_set(EDITOR_SCREEN_START_X,EDITOR_SCREEN_START_Y);

  // in rect
  RECT r = {EDITOR_SCREEN_START_X,EDITOR_SCREEN_START_Y,LEVEL_RESOLUTION_X,LEVEL_RESOLUTION_Y};

  // Cursor in level
  input.mevent_add(MOUSE_EVENT(MOUSE_STATE(r),
                   MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_IN,
                   SCREEN_HANDLE_IN));
  input.mevent_add(MOUSE_EVENT(MOUSE_STATE(r),
                   MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_OUT,
                   SCREEN_HANDLE_OUT));

  // Set/Remove items in level
  input.mevent_add(MOUSE_EVENT(MOUSE_STATE(r,MASK_BUTTON_LEFT|MASK_BUTTON_RIGHT),
                   MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS,
                   SCREEN_HANDLE_CLICK));

  // Modifications in level - rotation
  input.mevent_add(MOUSE_EVENT(MOUSE_STATE(r,MASK_WHEEL_UP|MASK_WHEEL_DOWN,K_R),
                   MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS|MEVENT_KEY,
                   SCREEN_HANDLE_ROTATE));

  // Modifications in level - variants
  input.mevent_add(MOUSE_EVENT(MOUSE_STATE(r,MASK_WHEEL_UP|MASK_WHEEL_DOWN,K_V),
                   MEVENT_MOUSE_EXTERNAL|MEVENT_MOUSE_IN|MEVENT_MOUSE_BUTTONS|MEVENT_KEY,
                   SCREEN_HANDLE_VARIATE));                   
}

void editor_gui::level_move(tpos dx, tpos dy)
{
  level.level_move(dx, dy);
}

void editor_gui::event_add(LEVEL_EVENT ev)
{
  queue.add(ev);
  queue.commit();
}

void editor_gui::level_new(bool force)
{
  if(!force) {
    input_start(&editor_gui::level_new_callback,NO_HANDLE,INPUT_BOOLEAN, _("erase all data?"));
  } else {
    undo_store();
    console.print("new level");
    level.level_new();

    level_name[0] = '\0';
#ifdef WINDOWS
    // Windows usualy don't allow to write to current working directory
    // it may be in C:\\Program Files or so...
    strcpy(level_name,p_dir->levels_user_get());
#endif
    strcat(level_name,DEFAULT_LEVEL_NAME);

    level_edited_set();
    level_edited_clear();
  }
}

void editor_gui::level_new_callback(void)
{
  if(input_get_bool()) {
    level_new(TRUE);
  }
}

char *editor_gui::level_name_get(void)
{
  return(level_name);
}

void editor_gui::level_name_set(char *p_name)
{
  strncpy(level_name,p_name,MAX_FILENAME);  
  level_caption_update();
}

void editor_gui::level_load(char *p_file, int force)
{
  static char file[MAX_FILENAME] = "";

  // Level is edited and not saved - ask for action
  if(level_edited_get() && !force) {
    if(p_file)
      strncpy(file,p_file,MAX_FILENAME);
    input_start(&editor_gui::level_load_callback, HANDLE_1, INPUT_BOOLEAN, _("level is modified. erase all data?"));
    return;
  }

  // restore saved filename, if it's saved or ask user for level-name
  if(!p_file) {
    p_file = file;
    if(!p_file[0]) {
      input_start(&editor_gui::level_load_callback, HANDLE_2, INPUT_STRING, _("Level to load:"));
      return;
    }
  }

  // Do we have a valid filename?
  if(p_file[0]) {
    // Save undo
    undo_store();
  
    // Load given level
    const char *p_paths[] = { p_dir->levels_user_get(), p_dir->cwd_get(), NULL };
  
    bool loaded = level.level_load(p_file,p_paths,sizeof(p_paths)/sizeof(p_paths[0]));
    if(loaded) {
      level_name_set(p_file);
      level_edited_clear();
    } else {
      console.print(_("Unable to load level %s"),p_file);
    }
  
    // Clear internal name
    file[0] = '\0';
  }
}

void editor_gui::level_load_callback(void)
{  
  switch(input_get_callback_handle()) {
    case HANDLE_1:      
      if(input_get_bool())
        level_load(NULL, TRUE);
      break;
    case HANDLE_2:
      level_load(input_get_string(), TRUE);
      break;
    default:
      assert(0);
      break;
  }
}

void editor_gui::level_save_as(char *p_file, int force)
{
  static char file[MAX_FILENAME] = "";
  static int  force_saved = BOOL_UNDEFINED;

  if(force == BOOL_UNDEFINED)
    force = force_saved;
  else
    force_saved = force;

  if(!p_file && file[0] == '\0') {
    input_start(&editor_gui::level_save_as_callback, HANDLE_1, INPUT_STRING, _("save to:"));
    return;
  } else if(file[0] != '\0') {
    p_file = file;  
  } else {
    strncpy(file, p_file, MAX_FILENAME);
  }

  if(!force && level.level_exists(p_file)) {
    input_start(&editor_gui::level_save_as_callback, HANDLE_2, INPUT_BOOLEAN, _("file %s exists! overwrite?:"),p_file);
    return;
  }

  // Do we have a corect filename?
  if(p_file[0]) {
    if(level.level_save(p_file)) {
      level_edited_clear();
      level_name_set(p_file);
    }
    else {
      console.print(_("Unable to save level %s"),p_file);
    }
    console.print(_("Saved as %s"),p_file);
  }
  file[0] = '\0';
  force_saved = BOOL_UNDEFINED;
}

void editor_gui::level_save_as_callback(void)
{
  switch(input_get_callback_handle()) {
    case HANDLE_1:
      level_save_as(input_get_string(), BOOL_UNDEFINED);
      break;
    case HANDLE_2:
      if(input_get_bool())
        level_save_as(NULL, TRUE);
      break;    
    default:
      assert(0);
      break;
  }  
}

void editor_gui::help_print_line(tpos x_pos, tpos &y_pos, char *p_key, char *p_name = NULL)
{
  p_font->print(NULL,x_pos,y_pos,p_key);
  if(p_name) {
    p_font->print(NULL,x_pos+EDIT_HELP_KEY_DX,y_pos,p_name);
  }
  y_pos += p_font->height_get();
  y_pos += EDIT_HELP_DY;
}

void editor_gui::help(void)
{
  p_grf->fill(0,0,EDITOR_RESOLUTION_X,EDITOR_RESOLUTION_Y,0);
  
  p_font->select(FONT_DEFAULT);

  tpos x_pos = EDIT_HELP_KEY_X;
  tpos y_pos = EDIT_HELP_START_Y;

  help_print_line(x_pos,y_pos,_("Keyboard control:"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("F1"),_("- Help"));
  help_print_line(x_pos,y_pos,_("ESC"),_("- Quit"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("CTRL+N"),_("- New level"));
  help_print_line(x_pos,y_pos,_("f2"),_("- Save level"));
  help_print_line(x_pos,y_pos,_("CTRL+F2"),_("- Save level as"));
  help_print_line(x_pos,y_pos,_("F3"),_("- Load level"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("F9"),_("- Run level"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("P"),_("- Pick item from cursor"));
  help_print_line(x_pos,y_pos,_("SHIFT+R"),_("- Rotate item"));
  help_print_line(x_pos,y_pos,_("CTRL+U"),_("- Undo"));
  help_print_line(x_pos,y_pos,_("CTRL+S"),_("- Shade level"));
  help_print_line(x_pos,y_pos,_("B"),_("- Background"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("1"),_("- Select floor layer"));
  help_print_line(x_pos,y_pos,_("2"),_("- Select items layer"));
  help_print_line(x_pos,y_pos,_("3"),_("- Select players layer"));
  help_print_line(x_pos,y_pos,_("4"),_("- Select all layer"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("CTRL+1"),_("- on/off background"));
  help_print_line(x_pos,y_pos,_("CTRL+2"),_("- on/off floor layer"));
  help_print_line(x_pos,y_pos,_("CTRL+3"),_("- on/off items layer"));
  help_print_line(x_pos,y_pos,_("CTRL+4"),_("- on/off players layer"));

  x_pos = EDIT_HELP_MOUSE_X;
  y_pos = EDIT_HELP_START_Y;
  help_print_line(x_pos,y_pos,_("Mouse control:"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("first"),_("- insert selected item"));
  help_print_line(x_pos,y_pos,_("third"),_("- clear selected cell"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("R+wheel"),_("- in place rotation"));
  help_print_line(x_pos,y_pos,_("V+wheel"),_("- in place variation"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("F+first"),_("- fill rect with item"));
  help_print_line(x_pos,y_pos,_("D+first"),_("- draw rect with item"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("F+third"),_("- clear solid rect"));
  help_print_line(x_pos,y_pos,_("D+third"),_("- clear empty rect"));

  x_pos = EDIT_HELP_MOUSE_X;
  y_pos += EDIT_HELP_DY*6;
  help_print_line(x_pos,y_pos,_("item panel mouse control:"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("wheel"),_("- scroll by one"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("PgUp"),_("- scroll page up"));
  help_print_line(x_pos,y_pos,_("PgDown"),_("- scroll page down"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("Home"),_("- first item"));
  help_print_line(x_pos,y_pos,_("End"),_("- last item"));

  y_pos += EDIT_HELP_DY*6;
  help_print_line(x_pos,y_pos,_("Screen control:"));
  y_pos += EDIT_HELP_DY;
  help_print_line(x_pos,y_pos,_("F10"),_("- Full screen mode"));  
  help_print_line(x_pos,y_pos,_("Arrows"),_("- Move screen"));

  p_grf->redraw_add(0,0,EDITOR_RESOLUTION_X,EDITOR_RESOLUTION_Y);

  menu_key_input.set((GUI_BASE *)this,(GUI_BASE_FUNC)&editor_gui::console_wait);
  console.input_start(INPUT_WAIT, NULL);  
  
  draw_level = FALSE;
}

void editor_gui::help_quit(void)
{
  draw_level = TRUE;
  draw(TRUE);
}

void editor_gui::level_save(int force)
{
  level_save_as(level_name,force);
}

void editor_gui::level_cursor_set(tpos x, tpos y)
{
  bool state;  

  if((state = level.coord_in_level(x,y))) {
    level.coord_to_cell(&x,&y);
    level.selection_set(x,y);  
  }

  level.selection_set(state);
  selection_cursor_draw(TRUE);
}

void editor_gui::level_item_insert(void)
{
  if(level.selection_get()) {
    undo_store();
  
    tpos layer = p_repo->item_get_layer(selected_editor_item.item);
  
    bool rect = level.selection_rectangle_get();  
    if(rect) {
      bool fill = level.selection_filled_get();
      tpos sx,sy,dx,dy;
    
      level.selection_rectangle_get(&sx,&sy,&dx,&dy);
      level.cell_set(sx, sy, dx, dy, layer, 
                     selected_editor_item.item, 
                     selected_editor_item.variant, 
                     selected_editor_item.rotation, 
                     TRUE, fill);
    
      level.selection_rectangle_set(FALSE);
      console.print(_("Inserted item %d at %dx%d - %dx%d layer %d"),
                    selected_editor_item.item,sx,sy,dx,dy,layer);
    }
    else {
      tpos x, y;
    
      level.selection_get(&x, &y);
      level.cell_set(x, y, layer, 
                     selected_editor_item.item, 
                     selected_editor_item.variant, 
                     selected_editor_item.rotation, TRUE);
    
      console.print(_("Inserted item %d at %dx%d layer %d"),
                    selected_editor_item.item,x,y,layer);
    }
  
    level_edited_set();
  }
}

void editor_gui::level_item_insert_rectangle(bool filled)
{
  if(level.selection_get()) {
    tpos x,y;
    level.selection_get(&x,&y);
    level.selection_rectangle_set(TRUE);
    level.selection_filled_set(filled);
    level.selection_rectangle_start_set(x,y);
  }
}

void editor_gui::level_item_rotate(CHANGE_TYPE type, int rotation)
{
  tpos x, y;

  if(level.selection_get()) {

    level.selection_get(&x,&y);
    int layer = layer_active_get(x,y);

    if(layer != LAYER_NONE) {
      if(level.cell_can_rotate(x,y,layer)) {
        undo_store();
        level_edited_set();
      
        if(type == ITEM_MODIFY) {
          level.cell_modify_rotation(x,y,layer,rotation,TRUE);
          console.print(_("[Modify] Rotating item at %dx%d layer %d"),x,y,layer);
        } 
        else if(type == ITEM_SET) {
          level.cell_set_rotation(x,y,layer,rotation,TRUE);
          console.print(_("[Set] Rotating item at %dx%d layer %d"),x,y,layer);
        }
      } else {
        console.print(_("Can't rotate item at %dx%d layer %d"),x,y,layer);
      }
    }
  }
}

void editor_gui::level_item_variate(CHANGE_TYPE type, int variant)
{  
  tpos x, y;

  if(level.selection_get()) {
  
    level.selection_get(&x,&y);
    int layer = layer_active_get(x,y);
  
    if(layer != LAYER_NONE) {
      undo_store();
      level_edited_set();
      
      if(type == ITEM_MODIFY) {
        level.cell_modify_variation(x,y, layer, variant, TRUE, TRUE);
        console.print(_("[Modify] Variating item at %dx%d layer %d"),x,y,layer);
      }
      else if(type == ITEM_SET) {
        level.cell_set_variation(x,y, layer, variant, TRUE, TRUE);
        console.print(_("[Set] Variating item at %dx%d layer %d"),x,y,layer);
      }
    }
  }
}

void editor_gui::level_item_clear(void)
{
  tpos x, y;

  if(level.selection_get()) {
    undo_store();
    level_edited_set();

    level.selection_get(&x,&y);
    int layer = layer_active_get(x,y);
  
    bool rect = level.selection_rectangle_get();
    if(rect) {
      bool fill = level.selection_filled_get();
      tpos sx,sy,dx,dy;
    
      level.selection_rectangle_get(&sx,&sy,&dx,&dy);
      level.cell_clear(sx, sy, dx, dy, layer, TRUE, fill);
    
      level.selection_rectangle_set(FALSE);
      console.print(_("Erased items from %dx%d - %dx%d layer %d"),sx,sy,dx,dy,layer);
    }
    else {
      if(layer != LAYER_NONE) {      
        level.cell_clear(x,y, layer, TRUE);      
        console.print(_("Erased item at %dx%d layer %d"),x,y,layer);
      }
    }
  }
}

void editor_gui::level_item_clear_rectangle(bool filled)
{
  if(level.selection_get()) {
    tpos x,y;
    level.selection_get(&x,&y);
    level.selection_rectangle_set(TRUE);
    level.selection_filled_set(filled);
    level.selection_rectangle_start_set(x,y);
  }
}

void editor_gui::selection_rotate(int direction)
{
  if(p_repo->item_can_rotate(selected_editor_item.item)) {
    ROTATE_ITEM(selected_editor_item.rotation, direction);
    selection_draw(TRUE);
  } else {
    console.print(_("Can't rotate item %d"),selected_editor_item.item);
  }
}

void editor_gui::selection_pickup(void)
{
/*
  Select item from the cursor
  set panels for that
*/  
  if(level.selection_get()) {    
    tpos x,y;
    level.selection_get(&x,&y);

    layer_active_set(ALL_LEVEL_LAYERS, TRUE);

    int layer = layer_active_get(x,y);
    if(layer == LAYER_NONE)
      return;

    item_handle item = level.cell_get_item(x, y, layer);
    if(item == NO_SELECTION)
      return;

    item_handle base_item = editor_panel::item_base_get(item);
    int variant = level.cell_get_variation(x, y, layer);    

    console.print("Picking up item %d variant %d", item, variant);

    EDITOR_PANEL_SLOT *p_slot;  
    for(int i = 0; (p_slot = panel_items.slot_get(i)); i++) {
      if(p_slot->item == base_item) {
        panel_items.slot_select(i, NULL, TRUE, TRUE);
        panel_items.panel_set(i, NULL, FALSE, TRUE);
      
        for(i = 0; (p_slot = panel_variants.slot_get(i)); i++) {
          if(p_slot->item == item && p_slot->variant == variant) {            
            panel_variants.panel_set(i, NULL, FALSE, TRUE);
            panel_variants.slot_highlight(i, FALSE);
            panel_variants.slot_select(i, &selected_editor_item, TRUE, TRUE);
            selection_draw(TRUE);
            return;
          }
        }
        // Not found?
        assert(0);
        return;
      }
    }
  }
}

// format: [GI_MOUSE_EVENT, handle, x, y, button]
void editor_gui::mouse_handler(LEVEL_EVENT_QUEUE *p_queue, LEVEL_EVENT ev)
{
  int button, direction;

  switch(ev.param_int_get(PARAM_0)) {
    case SCREEN_HANDLE_IN:
      p_queue->add(LEVEL_EVENT(ED_LEVEL_SET_CURSOR,ev.param_get(PARAM_1),ev.param_get(PARAM_2),ET(TRUE)));
      p_queue->commit();
      break;
    case SCREEN_HANDLE_OUT:
      p_queue->add(LEVEL_EVENT(ED_LEVEL_SET_CURSOR,-1,-1,TRUE));
      p_queue->commit();
      break;
    case SCREEN_HANDLE_CLICK:
      p_queue->add(LEVEL_EVENT(ED_LEVEL_SET_CURSOR,ev.param_get(PARAM_1),ev.param_get(PARAM_2)));
      button = ev.param_int_get(PARAM_4);
      if(button == BUTTON_LEFT)
        p_queue->add(LEVEL_EVENT(ED_LEVEL_DRAW_CURSOR_INSERT_ITEM));
      else if(button == BUTTON_MIDDLE)
        ;
      else if(button == BUTTON_RIGHT)
        p_queue->add(LEVEL_EVENT(ED_LEVEL_DRAW_CURSOR_CLEAR_ITEM));
      p_queue->commit();
      break;
    case SCREEN_HANDLE_ROTATE:
      direction = (ev.param_int_get(PARAM_4) == WHEEL_UP) ? -1 : 1;
      p_queue->add(LEVEL_EVENT(ED_LEVEL_SET_CURSOR,ev.param_get(PARAM_1),ev.param_get(PARAM_2)));
      p_queue->add(LEVEL_EVENT(ED_LEVEL_DRAW_CURSOR_ROTATE_ITEM, ITEM_MODIFY, direction));
      p_queue->commit();
      break;
    case SCREEN_HANDLE_VARIATE:
      direction = (ev.param_int_get(PARAM_4) == WHEEL_UP) ? -1 : 1;
      p_queue->add(LEVEL_EVENT(ED_LEVEL_SET_CURSOR,ev.param_get(PARAM_1),ev.param_get(PARAM_2)));
      p_queue->add(LEVEL_EVENT(ED_LEVEL_DRAW_CURSOR_VARIATE_ITEM,ITEM_MODIFY,direction));
      p_queue->commit();
      break;
    case PANEL_HANDLE_ITEMS:
    case PANEL_HANDLE_VARIANTS:
      if(ev.param_int_get(PARAM_3)) {
        panel_item_select(ev.param_int_get(PARAM_0),ev.param_int_get(PARAM_1),ev.param_int_get(PARAM_2));
      } else {
        panel_item_highlight(ev.param_int_get(PARAM_0),ev.param_int_get(PARAM_1),ev.param_int_get(PARAM_2));
      }
      break;
    default:
      break;
  }
}

bool editor_gui::editor_quit(bool force)
{
  if(level_edited_get() && !force) {
    input_start(&editor_gui::editor_quit_callback, NO_HANDLE, INPUT_BOOLEAN, _("level is not saved. exit anyway?"));
    return(FALSE);
  } else {
    return(TRUE);
  }
}

void editor_gui::editor_quit_callback(void)
{
  if(input_get_bool()) {
    queue.add(LEVEL_EVENT(ED_QUIT,TRUE));
  }
}

void editor_gui::editor_fullscreen(void)
{
  draw_panels = !draw_panels;  
  level.level_selection_area_set(draw_panels);

  // Register or undergister events from item panels

  if(draw_panels) {
    panel_items.register_controls_events(&input);
    panel_variants.register_controls_events(&input);
  }
  else {
    panel_items.unregister_controls_events(&input);
    panel_variants.unregister_controls_events(&input);
  }  

  // Side menu
  if(draw_panels) {
    side_menu_create();  
  } else {
    side_menu_remove();
  }

  // Set-up console
  console.output_draw_set(draw_panels);

  draw(TRUE);
}

void editor_gui::editor_run_level(void)
{
  #define TMP_LEVEL "berusky-editor-level.lv3"

  console.print("Run level...");

  char filename[MAX_FILENAME];
  return_path(p_dir->tmp_get(), TMP_LEVEL, filename, MAX_FILENAME);

  if(level.level_save(filename)) {
#ifdef LINUX
    int pid = fork();
    if(!pid) {
      char level_name[MAX_FILENAME];
      return_path(p_dir->tmp_get(), TMP_LEVEL, level_name, MAX_FILENAME);
      bprintf("%s -u %s",p_dir->game_binary_get(),level_name);
      int ret = execlp(p_dir->game_binary_get(),p_dir->game_binary_get(),"-u",level_name,NULL);
      if(ret == -1) {
        bprintf("Error: %s",strerror(errno));
      }      
    } 
    else {
      int status;
      bprintf("Waiting for %d",pid);
      waitpid(pid,&status,0);
      bprintf("Pid %d done",pid);
    }
#elif WINDOWS  
    bprintf("Saved as %s",filename);
    char level_name[MAX_FILENAME];
    return_path(p_dir->tmp_get(), TMP_LEVEL, level_name, MAX_FILENAME);
    bprintf("%s -u %s",p_dir->game_binary_get(),level_name);

    char game_path[PATH_MAX] = "\"";
    strcat(game_path, p_dir->game_binary_get());
    strcat(game_path, "\"");

    char level_path[PATH_MAX] = "\"";
    strcat(level_path, level_name);
    strcat(level_path, "\"");

    int ret = _spawnl( _P_WAIT, p_dir->game_binary_get(),game_path,"-u",level_path,NULL);
    if(ret == -1) {
      bprintf("Error: %s",strerror(errno));
    }
#endif  
  }
}


#define FLOOR_TYPES 5

#define CHANGE_FLOOR(x,y,new_variant)                                         \
{                                                                             \
  int variant = level.cell_get_variation(x,y,LAYER_FLOOR);                    \
  int base = variant / FLOOR_TYPES;                                           \
  level.cell_set_variation(x, y, LAYER_FLOOR, base*FLOOR_TYPES + new_variant, TRUE, TRUE);\
}

#define IS_WALL(x,y)  (                                                           \
                       level.cell_get_item(x,y,LAYER_ITEMS) == P_WALL ||          \
                       level.cell_get_item(x,y,LAYER_ITEMS) == P_EXIT ||          \
                       (level.cell_get_item(x,y,LAYER_ITEMS) >= PP_LEFT_JAMB_O && \
                        level.cell_get_item(x,y,LAYER_ITEMS) <= PP_BOTTOM_JAMB_Z) \
                      )


void editor_gui::editor_shader(tpos sx, tpos sy, tpos dx, tpos dy)
{
  int x,y;

  undo_store();
  console.print(_("Shading..."));

  for(x = sx; x < sx+dx; x++) {
    for(y = sy; y < sy+dy; y++) {    
      if(IN_LEVEL(x-1,y)   && IS_WALL(x-1,y) &&
         IN_LEVEL(x,  y-1) && IS_WALL(x,  y-1)) {
         // ####
         // ##@@         
         CHANGE_FLOOR(x,y,0);
      }
      else if(IN_LEVEL(x,  y-1) && IS_WALL(x,  y-1)) {
         //   ##        
         //   @@         
         CHANGE_FLOOR(x,y,3);
      }
      else if(IN_LEVEL(x-1, y ) && IS_WALL(x-1,y)) {
         //
         // ##@@         
         CHANGE_FLOOR(x,y,2);
      }
      else if(IN_LEVEL(x-1, y-1) && IS_WALL(x-1,y-1)) {
         // ##
         //   @@
         CHANGE_FLOOR(x,y,1);
      }
      else  {
         // 
         //   @@
         CHANGE_FLOOR(x,y,4);
      }
    }  
  }

  level_edited_set();
}

void editor_gui::editor_shader(void)
{
  editor_shader(0, 0, LEVEL_CELLS_X, LEVEL_CELLS_Y);
}

void editor_gui::level_change_backgroud(void)
{
  int max = level.back_max_get();
  int back = level.level_background_get()+1;

  if(back >= max)
    back = 0;
    
  level.level_background_set(back);
  
  console.print("background %d (from %d)",back, max);
}

void editor_gui::test_gui(void)
{
  bprintf("Test-gui!!(%s)",input_get_string());
}

void editor_gui::test(void)
{
  console.print("Test's gift");
}

bool editor_gui::event_handler(void)
{
  /* load events from system from SDL event handler */
  input.events_loop(&queue);

  /* Save all new events here */
  static LEVEL_EVENT_QUEUE tmp_queue;

  do {
    while(!queue.empty()) {
      LEVEL_EVENT ev = queue.get();
        
      switch(ev.action_get()) {

        case EV_TEST:
          test();
          break;

        case ED_HELP:
          // Clear all waiting events
          queue.clear();
          tmp_queue.clear();
        
          // Print help page
          help();        
          break;
        
        case ED_LEVEL_NEW:
          level_new(ev.param_int_get(PARAM_0));
          break;
      
        case ED_LEVEL_LOAD:
          level_load((char *)ev.param_point_get(PARAM_0));
          break;
        case ED_LEVEL_SAVE:
          level_save();
          break;
        case ED_LEVEL_SAVE_AS:
          level_save_as((char *)ev.param_point_get(PARAM_0));
          break;
      
        case ED_LEVEL_SET_CURSOR:
          level_cursor_set(ev.param_int_get(PARAM_0), ev.param_int_get(PARAM_1));
          break;

        case ED_LEVEL_RECTANGLE_SELECTION:
          level_item_insert_rectangle(ev.param_int_get(PARAM_0));
          break;
        
        case ED_LEVEL_DRAW_CURSOR_INSERT_ITEM:
          level_item_insert();
          break;
        
        case ED_LEVEL_DRAW_CURSOR_ROTATE_ITEM:
          level_item_rotate((CHANGE_TYPE)ev.param_int_get(PARAM_0), ev.param_int_get(PARAM_1));
          break;
  
        case ED_LEVEL_DRAW_CURSOR_VARIATE_ITEM:
          level_item_variate((CHANGE_TYPE)ev.param_int_get(PARAM_0), ev.param_int_get(PARAM_1));
          break;
  
        case ED_LEVEL_DRAW_CURSOR_CLEAR_ITEM:
          level_item_clear();
          break;          

        case ED_LEVEL_IPANEL_SCROLL:
          panel_scroll(ev.param_int_get(PARAM_0), ev.param_int_get(PARAM_1));
          break;
        
        case ED_LEVEL_MOUSE_PANEL_SCROLL:
          panel_scroll_mouse(ev.param_int_get(PARAM_0));
          break;
        
        case ED_LEVEL_LAYER:        
          layer_status_switch(ev.param_int_get(PARAM_0),(LAYER_STATE)(ev.param_int_get(PARAM_1)));
          break;
        
        case ED_LEVEL_SELECT_LAYER:
          layer_active_set(ev.param_int_get(PARAM_0), TRUE);
          break;
        
        case ED_LEVEL_CHANGE_BACKGROUND:
          level_change_backgroud();
          break;
        
        case ED_LEVEL_MOVE:
          level_move(ev.param_int_get(PARAM_0), ev.param_int_get(PARAM_1));
          break;
        
        case ED_CURSOR_PICKUP:
          selection_pickup();
          break;
  
        case ED_UNDO:
          undo_restore();
          break;
        case ED_REDO:
          break;
        
        case ED_LEVEL_RUN:
          editor_run_level();
          break;

        case ED_LEVEL_FULLSCREEN:
          editor_fullscreen();
          break;
        
        case ED_LEVEL_SHADER:
          editor_shader();
          break;
        
        case ED_ROTATE_SELECTION:
          selection_rotate();
          break;
                
        case GC_MENU_QUIT:
        case ED_QUIT:
          if(editor_quit(ev.param_int_get(PARAM_0))) {
            return(FALSE);
          }
        
        case GI_MENU_BACK_POP:
          back_pop();
          break;
        case GI_MENU_BACK_PUSH:
          back_push();
          break;
        
        case GI_SPRITE_DRAW:
        case GI_STRING_DRAW:
        case GI_CHECKBOX_SWITCH:
        case GI_KEY_DOWN:
          menu_services(&queue, &tmp_queue, ev);
          break;
        
        case GI_MOUSE_EVENT:
          mouse_handler(&tmp_queue, ev);
          break;          
        
        default:
          if(ev.valid()) {
            tmp_queue.add(ev);
          }
          break;
      }
    }
  
    /* Commit all writes */
    queue.add(&tmp_queue);
    queue.commit();
  
    tmp_queue.clear();
  
  } while(!queue.empty());

  // Draw all changes in level
  if(draw_level) {
    draw();
  }
  level.flip();

  return(TRUE);
}

void editor_gui::keyset_set(EVENT_KEY_SET *p_keyset)
{
  input.keyset_set(p_keyset);
}

void editor_gui::level_caption_update(void)
{
  window_set_title(_("Level %s %s"),level_name, level_edited_get() ? _("(unsaved)") : _("(saved)"));
}

void editor_gui::level_edited_set(void)
{
  if(!level_edited) {
    level_edited = TRUE;
    level_caption_update();
  }
}

bool editor_gui::level_edited_get(void)
{
  return(level_edited);
}

void editor_gui::level_edited_clear(void)
{
  if(level_edited) {
    level_edited = FALSE;
    level_caption_update();
  }
}

// An Undo interface
void editor_gui::undo_store(void)
{
  level.core_push();
}

bool editor_gui::undo_restore(void)
{
  bool ret = level.core_pop();
  if(ret) {
    level_edited_set();
    console.print(_("undo"));
  } else {
    console.print(_("no more undo"));
  }
  return(ret);
}

/* Start input from console
*/
void editor_gui::input_start(EDITOR_CONSOLE_CALLBACK callback, int callback_id, INPUT_TYPE type,
                             char *p_text,...)
{
  console_callback    = callback;
  console_callback_id = callback_id;

  char *p_tmp      = console.input_title_get();
  int   max_lenght = console.input_title_get_max_lenght();

  va_list  arguments;
  va_start(arguments,p_text);
  vsnprintf(p_tmp,max_lenght,p_text,arguments);
  va_end(arguments);

  menu_key_input.set((GUI_BASE *)this,(GUI_BASE_FUNC)&editor_gui::console_input);
  console.input_start(type, NULL);
}

/* An editor interface to console
*/
void editor_gui::console_input(MENU_STATE state, size_ptr data, size_ptr data1)
{ 
  switch(data) {
    case K_ENTER:      
    case K_ESC:
      input_stop(data == K_ENTER);
      break;
    case K_BKSP:
    case K_DEL:
      console.input_clear_last();
      break;
    default:
      {
        char c;
        if(key_to_ascii(data,&c)) {
          console.input_add_char(c);
        }
      }
      break;
  }
}

/* Stop input from console
*/
void editor_gui::input_stop(bool success)
{
  menu_key_input.clear();
  console.input_stop();
  if(console_callback) {
    (this->*console_callback)();
  }
}

/* Wait for a key
*/
void editor_gui::console_wait(MENU_STATE state, size_ptr data, size_ptr data1)
{ 
  help_quit();
  input_stop(TRUE);
}

// ----------------------------------------------------------------------
//  An editor console code
// ----------------------------------------------------------------------

const char * editor_console::boolean_yes = _("y");
const char * editor_console::boolean_no = _("n");

editor_console::editor_console(INPUT *p_input_, tpos sx, tpos sy, tpos dx, int lines_)
{
  istate = INPUT_NONE;
  
  p_input = p_input_;

  lines = lines_;

  height_diff = p_font->height_get();

  x = sx;
  y = sy;
  w = dx;
  h = lines * height_diff;

  ix_title = ix_input = sx;
  iy_title = iy_input = sy + height_diff * lines + CONSOLE_INPUT_LINE_SHIFT;

  output_clear();
  input_clear();

  output_draw = TRUE;
}

void editor_console::input_start(INPUT_TYPE type, char *p_text)
{
  input_clear(FALSE);

  istate = INPUT_READY;
  itype = type;
  if(p_text) {
    strncpy(p_text, input_line_title, CONSOLE_MAX_INPUT_LINE);
  }

  if(itype == INPUT_BOOLEAN) {
    sprintf(input_line_title+strlen(input_line_title)," (%s/%s) ",
            boolean_yes,boolean_no);
  } 
  else if(itype == INPUT_STRING) {
    sprintf(input_line_title+strlen(input_line_title)," ");
  }
  else if(type == INPUT_WAIT) {
  }

  p_input->block(TRUE);
  p_input->key_repeat(TRUE);

  input_redraw();
}

void editor_console::input_stop(bool redraw)
{  
  p_input->block(FALSE);
  p_input->key_repeat(FALSE);

  istate = INPUT_NONE;
  input_redraw();
}

void editor_console::input_clear(bool redraw)
{
  istate = INPUT_NONE;

  input_boolean = CONSOLE_BOOLEAN_NO_INPUT;
  input_string_lenght = 0;
  input_string[0] = '\0';

  if(redraw)
    input_redraw();
}

void editor_console::input_clear_last(bool redraw)
{
  if(istate == INPUT_READY) {
    if(itype == INPUT_BOOLEAN) {
      input_boolean = CONSOLE_BOOLEAN_NO_INPUT;
    } else if(itype == INPUT_STRING) {
      input_string_lenght--;
      if(input_string_lenght < 0)
        input_string_lenght = 0;
      input_string[input_string_lenght] = '\0';
    }
    if(redraw)
      input_redraw();
  }
}

void editor_console::input_add_char(char c, bool redraw)
{
  if(istate == INPUT_READY) {
    switch(itype) {
      case INPUT_BOOLEAN:
        if(c == boolean_yes[0]) {
          input_boolean = TRUE;
        }      
        else if(c == boolean_no[0]) {
          input_boolean = FALSE;
        }
        break;
      case INPUT_STRING:
        if(input_string_lenght+1 < CONSOLE_MAX_INPUT_LINE) {
          input_string[input_string_lenght] = c;
          input_string[input_string_lenght+1] = '\0';
          input_string_lenght++;
        }
        break;
      default:
        break;
    }
    if(redraw)
      input_redraw();
  }
}

void editor_console::input_redraw(void)
{ 
  if(itype == INPUT_WAIT) {
    // nothing here...
  } 
  else if(istate == INPUT_READY) {  
    p_grf->fill(ix_title,iy_title,w,CONSOLE_INPUT_LINE_HEIGHT,COLOR_MAP(0,0,255));

    p_font->select(FONT_DEFAULT);
    p_font->alignment_set(MENU_LEFT);
  
    RECT r;
    p_font->print(&r,ix_title,iy_title,input_line_title);
  
    ix_input = ix_title + r.w;
    iy_input = iy_title;
      
    if(itype == INPUT_BOOLEAN) {
      const char *p_tmp;
    
      if(input_boolean == CONSOLE_BOOLEAN_NO_INPUT)
        p_tmp = "_";
      else if(input_boolean)
        p_tmp = boolean_yes;
      else
        p_tmp = boolean_no;
      
      p_font->print(NULL,ix_input,iy_input,p_tmp);
    } 
    else if(itype == INPUT_STRING) {
      p_font->print(NULL,ix_input,iy_input,"%s_",input_string);
    }  
  }
  else {
    p_grf->fill(ix_title,iy_title,w,CONSOLE_INPUT_LINE_HEIGHT,COLOR_MAP(0,0,0));
  }
  p_grf->redraw_add(ix_title,iy_title,w,CONSOLE_INPUT_LINE_HEIGHT);
}

void editor_console::print(const char *p_text,...)
{
  strncpy(output_lines[0],output_lines[1],CONSOLE_MAX_INPUT_LINE);
  
  va_list   arguments;
  va_start(arguments,p_text);
  vsnprintf(output_lines[1]+2,CONSOLE_MAX_INPUT_LINE-2,p_text,arguments);
  va_end(arguments);  
  output_lines[1][0] = '*';
  output_lines[1][1] = ' ';

  output_redraw(TRUE);
}

void editor_console::output_clear(bool redraw)
{
  int i;
  for(i = 0; i < lines; i++)
    output_lines[i][0] = '\0';
  
  if(redraw)
    output_redraw(TRUE);
}

void editor_console::output_redraw(bool draw)
{
  if(output_draw) {
    p_font->select(FONT_DEFAULT);
    p_font->alignment_set(MENU_LEFT);
  
    p_grf->fill(x,y,w,h,0);
    int i;
    for(i = 0; i < lines; i++)
      p_font->print(NULL,x,y+i*height_diff,output_lines[i]);
    
    if(draw)
      p_grf->redraw_add(x,y,w,h);
  }
}

void editor_console::output_draw_set(bool draw)
{
  output_draw = draw;
}
