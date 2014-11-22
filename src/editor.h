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

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include "gui.h"

typedef enum {

  HORIZONTAL,
  VERTICAL

} DIRECTION;

#define NO_SELECTION (-1)

typedef struct editor_selection {

  // current selection
  int item;
  int variant;
  int rotation;

public:

  editor_selection(void)
  {
    item = 0;
    variant = 0;
    rotation = 0;
  }

} EDITOR_SELECTION;

typedef enum {
  
  ITEM_SET,
  ITEM_MODIFY

} CHANGE_TYPE;

#define ITEMS_X_START   0
#define ITEMS_Y_START   0
#define ITEMS_X_DIFF    0
#define ITEMS_Y_DIFF    35
#define ITEMS_DX        (EDITOR_RESOLUTION_X-ITEMS_X_START)
#define ITEMS_DY        (EDITOR_RESOLUTION_Y-ITEMS_Y_START)

//-- finish the pannel
#define ITEMS_IN_PANEL  7
#define ITEMS_START     0xffff
#define ITEMS_END       0xfffe

class editor_panel;
typedef class editor_panel EDITOR_PANEL;

typedef struct editor_panel_slot {  

  static ITEM_REPOSITORY *p_repo;
  
  static void set_up(ITEM_REPOSITORY *p_repo_)
  {    
    p_repo = p_repo_;
  }

public:

  int item;
  int variant;

public:

  void draw(tpos x, tpos y, bool highlighted, bool selected);

} EDITOR_PANEL_SLOT;

typedef class editor_panel {

  // Attached panel with items
  class editor_panel *p_attached_panel;

  // Location and appearance of the panel
  tpos                start_x, dx;
  tpos                start_y, dy;

  int                 panel_handle;

  MOUSE_EVENT        *p_registered_events;
  
protected:
  
  // Content of te panel
  EDITOR_PANEL_SLOT  *p_slots;
  int                 slot_num;

  // Current slot configuration
  // it's from 0 to slot_num
  int                 visible_slot_first;
  int                 visible_slot_num;

  int                 visible_slot_highlighted;
  int                 visible_slot_selected;

public:

  // Remove all items from the panel
  void clear(void);

  // Configure the panel
  virtual void configure(int item) {};

  /* Return index of slot at x,y coordinates
  */
  bool slot_return(tpos x, tpos y, int &slot);

  /* Return index of slot at x,y coordinates 
     or NO_SELECTION
  */
  int slot_return(tpos x, tpos y)
  {
    int slot;
    return(slot_return(x, y, slot) ? slot : NO_SELECTION);
  }

  bool slot_valid(int slot)
  {
    return(slot >= 0 && slot < slot_num);
  }

  EDITOR_PANEL_SLOT * slot_get(int slot)
  {
    return(slot_valid(slot) ? p_slots+slot : NULL);
  }

  /* Panel highlight routines
  */
  void slot_highlight(int slot, bool redraw)
  {
    visible_slot_highlighted = slot;
  
    assert(visible_slot_highlighted >= NO_SELECTION && 
           visible_slot_highlighted < slot_num);
  
    if(redraw)
      panel_draw(redraw);
  }
  void slot_highlight(tpos x, tpos y, bool redraw)
  {
    slot_highlight(slot_return(x,y),redraw);
  }
  void slot_unhighlight(bool redraw)
  {
    slot_highlight(NO_SELECTION,redraw);
  }

  /* Panel selection routines
  */
  void slot_select(int slot, EDITOR_SELECTION *p_sel, bool propagate, bool redraw)
  {
    // Set selected slot
    visible_slot_selected = slot;
  
    assert(visible_slot_selected >= NO_SELECTION && 
           visible_slot_selected < slot_num);
  
    if(redraw) {
      panel_draw(redraw);
    }
  
    // Propagate to editor selection (item/variant)  
    if(propagate) {
      EDITOR_SELECTION selection;
      slot_selection_get(&selection);
      if(p_attached_panel && slot != NO_SELECTION) {
        p_attached_panel->configure(selection.item);
        p_attached_panel->panel_draw(redraw);
      }
      if(p_sel)
        *p_sel = selection;
    }
  }

  void slot_select(tpos x, tpos y, EDITOR_SELECTION *p_sel, bool propagate, bool redraw)
  {
    slot_select(slot_return(x,y),p_sel,propagate,redraw);
  }
  void slot_unselect(bool propagate, bool redraw)
  {
    slot_select(NO_SELECTION,NULL,propagate, redraw);
  }

  void slot_selection_fix(int last_valid_slot, bool redraw)
  {
    if(visible_slot_selected != NO_SELECTION) {
      if(visible_slot_selected >= slot_num)
        visible_slot_selected = slot_num-1;
    }
    if(visible_slot_highlighted != NO_SELECTION) {
      if(visible_slot_highlighted >= slot_num)
        visible_slot_highlighted = slot_num-1;
    }
    if(redraw) {
      panel_draw(redraw);
    }
  }

  void slot_selection_get(EDITOR_SELECTION *p_sel)
  {    
    if(p_sel && visible_slot_selected != NO_SELECTION) {
      p_sel->item = p_slots[visible_slot_selected].item;
      p_sel->variant = p_slots[visible_slot_selected].variant;
    }
  }

protected:

  static ITEM_REPOSITORY *p_repo;

public:
  
  /* Setting up
  */
  static void set_up(ITEM_REPOSITORY *p_repo_)
  {    
    p_repo = p_repo_;
  }

public:

  static int item_base_get(int item);

  void attached_panel_set(class editor_panel *p_panel)
  {
    p_attached_panel = p_panel;
  }

  class editor_panel * attached_panel_get(void)
  {
    return(p_attached_panel);
  }

  void panel_set(int slot_first_new, EDITOR_SELECTION *p_sel, 
                 bool propagate, bool redraw)
  {
    if(slot_num <= visible_slot_num) {
      slot_first_new = 0;
    }
    else if(slot_first_new + visible_slot_num > slot_num) {
      slot_first_new = slot_num - visible_slot_num;
      assert(slot_first_new >= visible_slot_first);
    }

    if(slot_first_new != visible_slot_first) {
      visible_slot_first = slot_first_new;

      if(slot_num < visible_slot_num) {
        slot_selection_fix(slot_num-1, redraw);
      }
      else {
        slot_selection_fix(visible_slot_num-1, redraw);
      }

      if(propagate)
        slot_selection_get(p_sel);
    }
  }

  // Draws controls and panel item(s)
  void panel_draw(bool draw);

  // Scroll the pannel with given direction
  // and preserve selected item
  void panel_scroll(int direction, EDITOR_SELECTION *p_sel, bool redraw);
  
  void register_controls_events(INPUT *p_input);
  void unregister_controls_events(INPUT *p_input);

  // Get pannel size, without control buttons
  RECT boundary_get(void);

  void set_up(int panel_size, DIRECTION direction, 
              tpos sx, tpos sy, int handle)
  {
    switch(direction) {
      case HORIZONTAL:
        dx = EDITOR_ITEM_SIZE_X;
        dy = 0;
        break;
      case VERTICAL:
        dx = 0;
        dy = EDITOR_ITEM_SIZE_Y;
        break;
    }

    visible_slot_num = panel_size;
  
    start_x = sx;
    start_y = sy;
  
    panel_handle = handle;

    visible_slot_highlighted = NO_SELECTION;
    visible_slot_selected = NO_SELECTION;
  }  

  editor_panel(int panel_item_num, DIRECTION direction, 
              tpos start_x, tpos start_y,
              int handle)
  {
    memset(this,0,sizeof(*this));
    set_up(panel_item_num, direction, start_x, start_y, handle);
  }

} EDITOR_PANEL;

/* Derived panel for variants
*/
typedef class variant_panel : public editor_panel {

public:
  
  // Configure variant panel according the selected item
  virtual void configure(int item);

public:

  variant_panel(int panel_size, DIRECTION direction,
                tpos start_x, tpos start_y, int handle)
  : editor_panel(panel_size, direction, start_x, start_y, handle)
  {
  }

} VARIANT_PANEL;

/* Derived panel for items
*/
typedef class item_panel : public editor_panel {

public:
  
  item_panel(int panel_item_num,
             DIRECTION direction,
             tpos start_x, tpos start_y, 
             int handle, 
             VARIANT_PANEL *p_var);

} ITEM_PANEL;


typedef class editor_layer_config {

public:

  LAYER_CONFIG lc;
  RECT         coord[ALL_LEVEL_LAYERS];

public:
  
  editor_layer_config(void);  

} EDITOR_LAYER_CONFIG;

#define CONSOLE_MAX_INPUT_LINE            200
#define CONSOLE_OUTPUT_LINES              2
#define CONSOLE_BOOLEAN_NO_INPUT          (-1)
#define CONSOLE_INPUT_LINE_HEIGHT         22
#define CONSOLE_INPUT_LINE_SHIFT          5

typedef enum {
  
  INPUT_BOOLEAN,
  INPUT_STRING,
  INPUT_WAIT,

} INPUT_TYPE;

typedef enum {

  INPUT_NONE,
  INPUT_READY  

} INPUT_STATE;

typedef class editor_console {

  static const char *boolean_yes;
  static const char *boolean_no;

  INPUT_TYPE   itype;
  INPUT_STATE  istate;

  tpos         x,y;
  tpos         w,h;
  tpos         height_diff;

  int          lines;
  char         output_lines[CONSOLE_OUTPUT_LINES][CONSOLE_MAX_INPUT_LINE];
  bool         output_draw;

  tpos         ix_title;
  tpos         iy_title;
  char         input_line_title[CONSOLE_MAX_INPUT_LINE];

  tpos         ix_input;
  tpos         iy_input;

  int          input_boolean;

  int          input_string_lenght;
  char         input_string[CONSOLE_MAX_INPUT_LINE];

  INPUT       *p_input;

public:

  editor_console(INPUT *p_input_, tpos sx, tpos sy, tpos dx, int lines_);

  // ----------------------------------------------
  // Output interface
  // ----------------------------------------------
  
  void print(const char *p_text,...);
  void output_clear(bool redraw = TRUE);
  void output_redraw(bool draw);
  void output_draw_set(bool draw);


  // ----------------------------------------------
  // Input line interface
  // ----------------------------------------------
  void  input_start(INPUT_TYPE type, char *p_text = NULL);
  void  input_stop(bool redraw = TRUE);
  void  input_clear(bool redraw = TRUE);
  void  input_clear_last(bool redraw = TRUE);
  void  input_add_char(char c, bool redraw = TRUE);
  void  input_redraw(void);    
  
  char * input_string_get(void)
  {
    return(input_string);
  }
  int  input_string_get_max_lenght(void)
  {
    return(CONSOLE_MAX_INPUT_LINE);
  }
  void input_string_update(char *p_str = NULL)
  {
    if(p_str) {
      strncpy(input_string,p_str,CONSOLE_MAX_INPUT_LINE);
    }
    input_string[CONSOLE_MAX_INPUT_LINE-1] = '\0';
    input_string_lenght = strlen(input_string);
  }

  // ----------------------------------------------
  // Input line title interface
  // ----------------------------------------------
  char * input_title_get(void)
  {
    return(input_line_title);
  }
  int  input_title_get_max_lenght(void)
  {
    return(CONSOLE_MAX_INPUT_LINE);
  }
  void input_title_update(char *p_str, bool redraw = TRUE)
  {
    if(p_str) {
      strncpy(input_line_title,p_str,CONSOLE_MAX_INPUT_LINE);
    }
    input_line_title[CONSOLE_MAX_INPUT_LINE-1] = '\0';
  
    if(redraw) {
      input_redraw();
    }
  }
  bool input_bool_get(void)
  {
    return(input_boolean != CONSOLE_BOOLEAN_NO_INPUT && input_boolean);
  }

  editor_console()
  {
    memset(this,0,sizeof(*this));
  }

} EDITOR_CONSOLE;

class editor_gui;

typedef class editor_gui EDITOR_GUI;
typedef void (EDITOR_GUI::*EDITOR_CONSOLE_CALLBACK)(void);

typedef class editor_gui : public gui_base {

  DIR_LIST               *p_dir;

  ITEM_REPOSITORY        *p_repo;
  LEVEL_EDITOR            level;
  bool                    draw_level;   // Draw the level?
  bool                    draw_panels;  // Draw the panels?

  LEVEL_EVENT_QUEUE       queue;

public:
  
  EDITOR_CONSOLE          console;              // console interface
  EDITOR_CONSOLE_CALLBACK console_callback;     // console callback function
  int                     console_callback_id;  // ID of callback

public:

  #define NO_HANDLE       0
  #define HANDLE_1        (NO_HANDLE+1)
  #define HANDLE_2        (NO_HANDLE+2)
  #define HANDLE_3        (NO_HANDLE+3)
  #define HANDLE_4        (NO_HANDLE+4)

  void  input_start(EDITOR_CONSOLE_CALLBACK callback,  int callback_id, INPUT_TYPE type, char *p_text,...);
  void  console_input(MENU_STATE state, size_ptr data, size_ptr data1);  
  void  console_wait(MENU_STATE state, size_ptr data, size_ptr data1);
  void  input_stop(bool success);  

  void  console_draw(bool draw)
  {
    console.output_redraw(draw);
  }

  bool  input_get_bool(void)
  {
    return(console.input_bool_get());
  }

  char * input_get_string(void)
  {
    return(console.input_string_get());
  }

  int    input_get_callback_handle(void)
  {
    return(console_callback_id);
  }

public:
  
  ITEM_PANEL      panel_items;
  VARIANT_PANEL   panel_variants;

public:
  
  // Panel interface
  void panel_item_select(int panel, tpos x, tpos y);
  void panel_item_highlight(int panel, tpos x, tpos y);
  void panel_draw(bool draw);
  void panel_scroll(int panel, int direction);
  void panel_scroll_mouse(int direction);

private:

  // Recent selected item (in editor)
  EDITOR_SELECTION        selected_editor_item;

private:

  void selection_cursor_draw_status(bool draw, char *p_text,...);

public:
  
  // Selection interface
  void selection_draw(bool draw);
  void selection_cursor_draw(bool draw);

  void selection_rotate(int direction = 1);

  // Pick up selected item from cursor
  void selection_pickup(void);

private:

  LLIST_ITEM * p_side_event_first;
  int          side_event_num;

  void side_menu_create(void);
  void side_menu_remove(void);
  void side_menu_draw(bool draw);

private:

  EDITOR_LAYER_CONFIG config;

public:

  // General level interface
  char level_name[MAX_FILENAME];
  bool level_edited;

  char *level_name_get(void);
  void level_name_set(char *p_name);

  void level_caption_update(void);
  
  void level_edited_set(void);
  bool level_edited_get(void);
  void level_edited_clear(void);

  void help(void);
  void help_quit(void);

  void level_new(bool force = FALSE);
  void level_new_callback(void);

  void level_load(char *p_file, int force = FALSE);
  void level_load_callback(void);

  void level_save(int force = TRUE);
  void level_save_as(char *p_file = NULL, int force = FALSE);
  void level_save_as_callback(void);

  void level_move(tpos dx, tpos dy);

  void draw(bool force = FALSE);

public:

  editor_gui(ITEM_REPOSITORY *p_repo_, DIR_LIST *p_dir_);
  ~editor_gui(void);
  
  void editor_init(void);

  bool editor_quit(bool force);
  void editor_quit_callback(void);

  // Test event
  void test(void);
  void test_gui(void);

  // Level screen interface  
  bool level_draw(void);
  void level_config(void);

  // Menu interface
  void menu_draw(void);

  // Level screen events
  void level_cursor_set(tpos x, tpos y);  

  void level_item_insert(void);
  void level_item_insert_rectangle(bool filled);
  
  void level_item_rotate(CHANGE_TYPE type, int rotation);
  void level_item_variate(CHANGE_TYPE type, int variant);
  
  void level_item_clear(void);
  void level_item_clear_rectangle(bool filled);

  void layer_active_draw(bool draw);

  void layer_status_create(void);
  void layer_status_draw(bool draw);
  void layer_status_switch(int layer, LAYER_STATE state);  
  void layer_active_set(int layer, bool draw);
  int  layer_active_get(void);
  int  layer_active_get(tpos x, tpos y);
  

  // Mouse handler
  void mouse_handler(LEVEL_EVENT_QUEUE *p_queue, LEVEL_EVENT ev);

  // general handler
  bool event_handler(void);

  // Add an event to main queue
  void event_add(LEVEL_EVENT ev);
  
  // Set a keyset for editor
  void keyset_set(EVENT_KEY_SET *p_keyset);
  
  // Run level
  void editor_run_level(void);
  
  // Run level in fullscreen
  void editor_fullscreen(void);
  
  // Shade level 
  void editor_shader(void);
  void editor_shader(tpos sx, tpos sy, tpos dx, tpos dy);

  void level_change_backgroud(void);

  void help_print_line(tpos x_pos, tpos &y_pos, char *p_key, char *p_name);

public:  
  // An Undo interface
  void undo_store(void);
  bool undo_restore(void);

} EDITOR_GUI;

#define SCREEN_HANDLE_IN      0
#define SCREEN_HANDLE_OUT     1
#define SCREEN_HANDLE_CLICK   2
#define SCREEN_HANDLE_ROTATE  3
#define SCREEN_HANDLE_VARIATE 4

#define PANEL_HANDLE_ITEMS    11
#define PANEL_HANDLE_VARIANTS 12

#endif // __EDITOR_H__
