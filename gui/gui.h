/* Copyright (c) 2017-2019 Dmitry Stepanov a.k.a mr.DIMAS
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

/* Inspired by WPF */

/* Set to non-zero to enable visual debugging of GUI */
#define DE_GUI_ENABLE_GUI_DEBUGGING 0

typedef struct de_gui_node_t de_gui_node_t;
typedef struct de_gui_draw_list_t de_gui_draw_list_t;

typedef struct de_gui_thickness_t {
	float left;
	float top;
	float right;
	float bottom;
} de_gui_thickness_t;

/**
 * @brief Initializes thickness with same values. Designed for use with
 * designated initializers.
 */
de_gui_thickness_t de_gui_thickness_uniform(float value);

/**
* @brief Vertical alignment.
*/
typedef enum de_gui_vertical_alignment_t {
	DE_GUI_VERTICAL_ALIGNMENT_STRETCH, /**< Top alignment with vertical stretch */
	DE_GUI_VERTICAL_ALIGNMENT_TOP,    /**< Top alignment */
	DE_GUI_VERTICAL_ALIGNMENT_CENTER, /**< Center alignment */
	DE_GUI_VERTICAL_ALIGNMENT_BOTTOM, /**< Bottom alignment */
} de_gui_vertical_alignment_t;

/**
* @brief Horizontal alignment.
*/
typedef enum de_gui_horizontal_alignment_t {
	DE_GUI_HORIZONTAL_ALIGNMENT_STRETCH, /**< Left alignment with horizontal stretch */
	DE_GUI_HORIZONTAL_ALIGNMENT_LEFT,   /**< Left alignment */
	DE_GUI_HORIZONTAL_ALIGNMENT_CENTER, /**< Center alignment */
	DE_GUI_HORIZONTAL_ALIGNMENT_RIGHT,  /**< Right alignment */
} de_gui_horizontal_alignment_t;

typedef void(*de_gui_callback_func_t)(de_gui_node_t*, void*);

typedef struct de_gui_callback_t {
	void* arg;
	de_gui_callback_func_t func;
} de_gui_callback_t;

#if DE_DISABLE_ASSERTS
#  define DE_ASSERT_GUI_NODE_TYPE(node, expected_type) 
#else
#  define DE_ASSERT_GUI_NODE_TYPE(node, expected_type) \
	if(node->type != expected_type) de_fatal_error("Node must be " #expected_type " type!")
#endif

#include "gui/draw.h"
#include "gui/button.h"
#include "gui/border.h"
#include "gui/canvas.h"
#include "gui/grid.h"
#include "gui/scroll_bar.h"
#include "gui/scroll_content_presenter.h"
#include "gui/scroll_viewer.h"
#include "gui/text.h"
#include "gui/text_box.h"
#include "gui/window.h"
#include "gui/slide_selector.h"
#include "gui/image.h"
#include "gui/check_box.h"

/**
 * @brief Possible types of built-in UI nodes.
 *
 * Custom types should use DE_GUI_NODE_USER_CONTROL as origin point.
 */
typedef enum de_gui_node_type_t {
	DE_GUI_NODE_BASE, /**< Default node type */
	DE_GUI_NODE_TEXT,
	DE_GUI_NODE_BORDER,
	DE_GUI_NODE_WINDOW,
	DE_GUI_NODE_BUTTON,
	DE_GUI_NODE_SCROLL_BAR,
	DE_GUI_NODE_SCROLL_VIEWER,
	DE_GUI_NODE_TEXT_BOX,
	DE_GUI_NODE_IMAGE,
	DE_GUI_NODE_GRID,                    /**< Automatically arranges children by rows and columns */
	DE_GUI_NODE_CANVAS,                  /**< Allows user to directly set position and size of a node */
	DE_GUI_NODE_SCROLL_CONTENT_PRESENTER, /**< Allows user to scroll content */
	DE_GUI_NODE_SLIDE_SELECTOR,
	DE_GUI_NODE_CHECK_BOX,
	/**
	 * TODO. Special isolated node which will be copied and inserted into visual tree. Very useful to
	 * create your own look for standard controls. Template node will be never rendered or interacted
	 * with user.
	 */
	DE_GUI_NODE_TEMPLATE,
	DE_GUI_NODE_USER_CONTROL /* Use this as start index for type for your custom controls */
} de_gui_node_type_t;

/**
 * @brief
 */
typedef enum de_gui_routed_event_type_t {
	DE_GUI_ROUTED_EVENT_MOUSE_DOWN,
	DE_GUI_ROUTED_EVENT_MOUSE_UP,
	DE_GUI_ROUTED_EVENT_MOUSE_ENTER,
	DE_GUI_ROUTED_EVENT_MOUSE_LEAVE,
	DE_GUI_ROUTED_EVENT_MOUSE_MOVE,
	DE_GUI_ROUTED_EVENT_LOST_FOCUS,
	DE_GUI_ROUTED_EVENT_GOT_FOCUS,
	DE_GUI_ROUTED_EVENT_KEY_DOWN,
	DE_GUI_ROUTED_EVENT_KEY_UP,
	DE_GUI_ROUTED_EVENT_MOUSE_WHEEL,
	DE_GUI_ROUTED_EVENT_TEXT
} de_gui_routed_event_type_t;

/**
 * @brief Routed event. Typed union.
 * 
 * It is very important to understand what routed events are. Lets look at this example:
 * user clicks on a button in a window. In this case mouse down event will be generated
 * by UI input handler, then this event will be passed to event router which will 
 * consecutively pass event at first to button and then to window. This hierarchy can
 * be any depth you want, the key point is that the event will go from clicked node 
 * to its parent and parent of parent, and etc. This is so called bubbling event routing -
 * event will behave like an air bubble in water. This routing can be interrupted by
 * "handled" flag, if it is raised then event won't float up - routing will be stopped.
 * So in example with a button in a window, button will "handle" mouse down event and 
 * window will not receive it. If button wouldn't handle event then it would be more like
 * click-through behavior which is unwanted in case of button. 
 */
typedef struct de_gui_routed_event_args_t {
	de_gui_routed_event_type_t type;
	bool handled; /* if set to true, further routing will be stopped */
	de_gui_node_t* source; /* node which initiated this event */
	union {
		struct {
			de_vec2_t pos;
		} mouse_move;
		struct {
			de_vec2_t pos;
			enum de_mouse_button button;
		} mouse_down;
		struct {
			de_vec2_t pos;
			enum de_mouse_button button;
		} mouse_up;
		struct {
			uint32_t unicode;
		} text;
		struct {
			enum de_key key;
			int alt : 1;
			int control : 1;
			int shift : 1;
			int system : 1;
		} key;
		struct {
			int delta;
			de_vec2_t pos;
		} wheel;
	} s;
} de_gui_routed_event_args_t;

/**
 * @brief
 */
typedef enum de_gui_node_visibility_t {
	DE_GUI_NODE_VISIBILITY_VISIBLE,    /**< Node will be visible */
	DE_GUI_NODE_VISIBILITY_HIDDEN,    /**< Node will be invisible, but its bounds will participate in layout */
	DE_GUI_NODE_VISIBILITY_COLLAPSED, /**< Node will be invisible and no space will be reserved for it in layout */
} de_gui_node_visibility_t;

#define DE_MEMBER_SIZE(type, member) sizeof(((type *)0)->member)


#define DE_GUI_NODE_DESIRED_POSITION_PROPERTY "DesiredPosition"
#define DE_GUI_NODE_ACTUAL_POSITION_PROPERTY "ActualPosition"
#define DE_GUI_NODE_SCREEN_POSITION_PROPERTY "ScreenPosition"

#define DE_GUI_BUTTON_HOVERED_COLOR_PROPERTY "HoveredColor"
#define DE_GUI_BUTTON_PRESSED_COLOR_PROPERTY "PressedColor"
#define DE_GUI_BUTTON_NORMAL_COLOR_PROPERTY "NormalColor"

/** Event handlers definitions */
typedef void(*de_mouse_down_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_mouse_up_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_mouse_move_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_mouse_enter_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_mouse_leave_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_lost_focus_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_got_focus_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_text_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_key_down_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_key_up_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);
typedef void(*de_mouse_wheel_event_t)(de_gui_node_t*, de_gui_routed_event_args_t*);

#define DE_DECLARE_PROPERTY_SETTER(type__, field__, passed_name__, field_name__, value__, data_size__, object__) \
	if (strcmp(field_name__, passed_name__) == 0) { \
		if (data_size == DE_MEMBER_SIZE(type__, field__)) { \
			memcpy(&object__->field__, value__, data_size__); \
		} \
	}

#define DE_DECLARE_PROPERTY_GETTER(type__, field__, passed_name__, field_name__, value__, data_size__, object__) \
	if (strcmp(field_name__, passed_name__) == 0) { \
		if (data_size == DE_MEMBER_SIZE(type__, field__)) { \
			memcpy(value__, &object__->field__, data_size__); \
      return true; \
		} \
	}


/**
 * @brief UI node descriptor. Works well with C99 (or C++20) designated
 * initializers.
 *
 * Example:
 *		de_gui_node_t* text_box = de_gui_node_create(gui, DE_GUI_NODE_TEXT_BOX);
 *		de_gui_node_apply_descriptor(text_box, &(de_gui_node_descriptor_t) {
 *		    .row = 1, 
 *		    .column = 1, 
 *		    .parent = grid,
 *			.margin = (de_gui_thickness_t) {
 *			    .left = 2, 
 *			    .top = 2, 
 *			    .right = 2, 
 *			    .bottom = 2 
 *			}
 *		});
 * Every unset value will have correct default value.
 *
 * TODO: Expand this struct to be able to work with more widgets.
 */
typedef struct de_gui_node_descriptor_t {
	size_t row;
	size_t column;
	de_gui_vertical_alignment_t vertical_alignment;
	de_gui_horizontal_alignment_t horizontal_alignment;
	de_gui_node_visibility_t visibility;
	de_color_t color;
	float width;
	float height;
	float x; /**< Desired local x of node */
	float y; /**< Desired local y of node */
	de_gui_thickness_t margin;
	de_gui_node_t* parent;
	void* user_data;
	union {
		de_gui_text_descriptor_t text_block;
		de_gui_scroll_bar_descriptor_t scroll_bar;
		de_gui_grid_descriptor_t grid;
		de_gui_button_descriptor_t button;
		de_gui_image_descriptor_t image;
		de_gui_border_descriptor_t border;
		de_gui_check_box_descriptor_t check_box;
	} s;
} de_gui_node_descriptor_t;

typedef struct de_gui_node_dispatch_table_t {
	void(*init)(de_gui_node_t* n);
	void(*deinit)(de_gui_node_t* n);
	de_vec2_t(*measure_override)(de_gui_node_t* n, const de_vec2_t* available_size);
	de_vec2_t(*arrange_override)(de_gui_node_t* n, const de_vec2_t* final_size);
	void(*update)(de_gui_node_t* n);
	void(*render)(de_gui_draw_list_t* dl, de_gui_node_t* n, uint8_t nesting);
	bool(*get_property)(de_gui_node_t* n, const char* name, void* value, size_t data_size);
	bool(*set_property)(de_gui_node_t* n, const char* name, const void* value, size_t data_size);
	bool(*parse_property)(de_gui_node_t* n, const char* name, const char* value);
	void(*apply_descriptor)(de_gui_node_t* n, const de_gui_node_descriptor_t* desc);
	void(*apply_template)(de_gui_node_t* node, const de_gui_node_t* ctemplate);
} de_gui_node_dispatch_table_t;

/**
 * @brief GUI node. Tagged union (https://en.wikipedia.org/wiki/Tagged_union)
 */
struct de_gui_node_t {
	de_gui_node_type_t type; /**< Actual type of the node */
	de_gui_node_dispatch_table_t* dispatch_table; /**< Table of pointers to type-related functions (vtable) */
	de_vec2_t desired_local_position; /**< Desired position relative to parent node */	
	float width; /**< Explicit width for node or automatic if NaN (means value is undefined). Default is NaN */
	float height; /**< Explicit height for node or automatic if NaN (means value is undefined). Default is NaN */
	de_vec2_t screen_position; /**< Screen position of the node */
	de_vec2_t desired_size; /**< Desired size of the node after Measure pass. */
	de_vec2_t actual_local_position; /**< Actual node local position after Arrange pass. */
	de_vec2_t actual_size; /**< Actual size of the node after Arrange pass. */
	de_vec2_t min_size; /**< Minimum width and height */
	de_vec2_t max_size; /**< Maximum width and height */
	de_color_t color; /**< Overlay color of the node */
	size_t row; /**< Index of row to which this node belongs */
	size_t column; /**< Index of column to which this node belongs */
	de_gui_vertical_alignment_t vertical_alignment; /**< Vertical alignment */
	de_gui_horizontal_alignment_t horizontal_alignment; /**< Horizontal alignment */
	de_gui_thickness_t margin; /**< Margin (four sides) */
	de_gui_node_visibility_t visibility; /**< Current visibility state */
	float opacity; /**< Opacity. Inherited property - children will have same opacity */
	uint16_t tab_index; /**< Index for keyboard navigation */
	struct de_gui_node_t* parent; /**< Pointer to parent node */
	DE_ARRAY_DECLARE(struct de_gui_node_t*, children);  /**< Array of children nodes */
	DE_ARRAY_DECLARE(int, geometry); /**< Array of indices to draw command in draw list */
	bool is_hit_test_visible; /**< Will this control participate in hit-test? */
	void* user_data; /**< Pointer to any data. Useful to pass data to callbacks */
	/* Event handlers */
	de_mouse_down_event_t mouse_down;
	de_mouse_up_event_t mouse_up;
	de_mouse_move_event_t mouse_move;
	de_mouse_enter_event_t mouse_enter;
	de_mouse_leave_event_t mouse_leave;
	de_lost_focus_event_t lost_focus;
	de_got_focus_event_t got_focus;
	de_text_event_t text_entered;
	de_key_down_event_t key_down;
	de_key_up_event_t key_up;
	de_mouse_wheel_event_t mouse_wheel;
	bool is_focused;
	bool is_mouse_over;
	bool is_measure_valid;
	bool is_arrange_valid;	
	DE_LINKED_LIST_ITEM(struct de_gui_node_t);
	de_gui_t* gui;
	/* Specialization (type-specific data) */
	union {
		de_gui_border_t border;
		de_gui_text_t text;
		de_gui_button_t button;
		de_gui_grid_t grid;
		de_gui_scroll_bar_t scroll_bar;
		de_gui_canvas_t canvas;
		de_gui_scroll_viewer_t scroll_viewer;
		de_gui_scroll_content_presenter_t scroll_content_presenter;
		de_gui_window_t window;
		de_gui_text_box_t text_box;
		de_gui_slide_selector_t slide_selector;
		de_gui_image_t image;
		de_gui_check_box_t check_box;
		void* user_control;
	} s;
};

/**
 * @brief
 */
struct de_gui_t {
	DE_LINKED_LIST_DECLARE(de_gui_node_t, nodes);
	de_core_t* core;
	de_gui_draw_list_t draw_list;
	size_t text_buffer_size;
	uint32_t* text_buffer; /**< Temporary buffer to convert utf8 -> utf32. do NOT store pointer to this memory */
	de_font_t* default_font;
	de_gui_node_t* captured_node; /**< Pointer to node that captured mouse input. */
	de_gui_node_t* keyboard_focus;
	de_gui_node_t* picked_node;
	de_gui_node_t* prev_picked_node;
	de_vec2_t prev_mouse_pos;
	size_t tab_width; /* count of spaces per tab */
};

/**
 * @brief Initializes GUI. For internal use only!
 */
de_gui_t* de_gui_init(de_core_t* core);

/**
 * @brief Shutdowns GUI. For internal use only!
 */
void de_gui_shutdown(de_gui_t* gui);

/**
 * @brief Calculates screen positions of node. Acts recursively to child nodes.
 * @param node
 */
void de_gui_node_update_transform(de_gui_node_t* node);

/**
 * @brief Creates UI widget of specified type.
 */
de_gui_node_t* de_gui_node_create(de_gui_t* gui, de_gui_node_type_t type);

/**
 * @brief Creates UI widget of specified type using specified descriptor.
 */
de_gui_node_t* de_gui_node_create_with_desc(de_gui_t* gui, de_gui_node_type_t type, const de_gui_node_descriptor_t* desc);

/**
 * @brief Sets desired local position of a node. Actual position can be different and depends on layout.
 */
void de_gui_node_set_desired_local_position(de_gui_node_t* node, float x, float y);

/**
 * @brief Sets desired size of a node. Actual size can be different. It depends on alignment settings and type of layout.
 */
void de_gui_node_set_size(de_gui_node_t* node, float w, float h);

/**
 * @brief Experimental. Do not use.
 */
bool de_gui_node_set_property(de_gui_node_t* n, const char* name, const void* value, size_t data_size);

/**
 * @brief Experimental. Do not use.
 */
bool de_gui_node_parse_property(de_gui_node_t* n, const char* name, const char* value);

/**
 * @brief Experimental. Do not use.
 */
bool de_gui_node_get_property(de_gui_node_t* n, const char* name, void* value, size_t data_size);

/**
 * @brief Enables or disables hit test for specified UI node. Nodes with disabled hit test
 * will not receive any mouse events.
 */
void de_gui_node_set_hit_test_visible(de_gui_node_t* n, bool visibility);

/**
 * @brief Allows to apply multiple properties of a UI node without need to write a ton
 * of boilerplate code.
 *
 * For example see description of \ref de_gui_node_descriptor_t struct.
 */
void de_gui_node_apply_descriptor(de_gui_node_t* n, const de_gui_node_descriptor_t* desc);

/**
 * @brief Sets overlay color of a node.
 */
void de_gui_node_set_color(de_gui_node_t* node, const de_color_t* color);

/**
 * @brief Sets overlay color of a node.
 */
void de_gui_node_set_color_rgba(de_gui_node_t* node, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * @brief Attaches specified node to a specified parent, so parent's transform will
 * affect children transforms.
 */
bool de_gui_node_attach(de_gui_node_t* node, de_gui_node_t* parent);

/**
 * @brief Detaches node from current parent making a node root node.
 */
void de_gui_node_detach(de_gui_node_t* node);

/**
 * @brief Destroys UI node with all its children.
 */
void de_gui_node_free(de_gui_node_t* node);

/**
 * @brief Sets row number specified node belongs to. Value will be used only if node 
 * placed into grid control.
 */
void de_gui_node_set_row(de_gui_node_t* node, size_t row);

/**
 * @brief Sets column number specified node belongs to. Value will be used only if node 
 * placed into grid control.
 */
void de_gui_node_set_column(de_gui_node_t* node, size_t col);

/**
 * @brief Sets uniform margin for a specified node.
 */
void de_gui_node_set_margin_uniform(de_gui_node_t* node, float margin);

/**
 * @brief Sets margin for a specified node with separate value for each side.
 */
void de_gui_node_set_margin(de_gui_node_t* node, float left, float top, float right, float bottom);

/**
 * @brief Recursively searches parent of specified type.
 */
de_gui_node_t* de_gui_node_find_parent_of_type(de_gui_node_t* node, de_gui_node_type_t type);

/**
 * @brief Performs non-recursive search for a child of specified type.
 */
de_gui_node_t* de_gui_node_find_direct_child_of_type(de_gui_node_t* node, de_gui_node_type_t type);

/**
 * @brief Performs recursive search for a child of specified type.
 */
de_gui_node_t* de_gui_node_find_child_of_type(de_gui_node_t* node, de_gui_node_type_t type);

/**
 * @brief Capturing mouse input by specified node. The node will always receive mouse events even
 * mouse is outside of node bounds. While there is any captured node, none of other nodes won't 
 * receive any mouse event. This method is very handy for draggable controls like sliders, once
 * mouse is captured it must be released when no capture is needed. If this function called if 
 * there is captured node, it at first release capture from current captured node and then captures
 * mouse by specified node.
 */
void de_gui_node_capture_mouse(de_gui_node_t* node);

/**
 * @brief Releases mouse capture.
 */
void de_gui_node_release_mouse_capture(de_gui_node_t* node);

/**
 * @brief Sets mouse down event handler.
 */
void de_gui_node_set_mouse_down(de_gui_node_t* node, de_mouse_down_event_t evt);

/**
 * @brief Sets mouse up event handler.
 */
void de_gui_node_set_mouse_up(de_gui_node_t* node, de_mouse_up_event_t evt);

/**
 * @brief Sets mouse move event handler.
 */
void de_gui_node_set_mouse_move(de_gui_node_t* node, de_mouse_move_event_t evt);

/**
 * @brief Sets visibility of specified node. If a node has keyboard focus, it will 
 * be dropped and lost focus event will be raised.
 */
void de_gui_node_set_visibility(de_gui_node_t* node, de_gui_node_visibility_t vis);

/**
 * @brief Sets vertical alignment for specified node.
 */
void de_gui_node_set_vertical_alignment(de_gui_node_t* node, de_gui_vertical_alignment_t va);

/**
 * @brief Sets horizontal alignment for specified node.
 */
void de_gui_node_set_horizontal_alignment(de_gui_node_t* node, de_gui_horizontal_alignment_t ha);

/**
 * @brief Internal. Draws every UI node into draw list.
 */
de_gui_draw_list_t* de_gui_render(de_gui_t* gui);

/**
 * @brief Performs layouting, animation, transform calculations, etc.
 */
void de_gui_update(de_gui_t* gui);

/**
 * @brief Main UI event processing function, you must call it if you need UI
 * to respond to user input.
 */
bool de_gui_process_event(de_gui_t* gui, const de_event_t* evt);

/**
 * @brief Performs hit test starting from root nodes, returns top-most picked node.
 */
de_gui_node_t* de_gui_hit_test(de_gui_t* gui, float x, float y);

/**
 * @brief Sets user data for a node, which can be obtained when needed.
 *
 * Main purpose of this method is to get user data in event handlers.
 */
void de_gui_node_set_user_data(de_gui_node_t* node, void* user_data);

de_vec2_t de_gui_node_default_measure_override(de_gui_node_t* n, const de_vec2_t* available_size);

void de_gui_node_measure(de_gui_node_t* node, const de_vec2_t* available_size);

de_vec2_t de_gui_node_default_arrange_override(de_gui_node_t* n, const de_vec2_t* final_size);

void de_gui_node_arrange(de_gui_node_t* node, const de_rectf_t* final_rect);

/**
 * @brief Drops focus from current focused ui node, raises LOST_FOCUS event for focused
 * node.
 */
void de_gui_drop_focus(de_gui_t* gui);

/**
 * TODO
 */
void de_gui_node_apply_template(de_gui_node_t* node, const de_gui_node_t* ctemplate);

void de_gui_node_set_min_width(de_gui_node_t* node, float min_width);

void de_gui_node_set_min_height(de_gui_node_t* node, float min_height);

void de_gui_node_set_max_width(de_gui_node_t* node, float max_width);

void de_gui_node_set_max_height(de_gui_node_t* node, float max_height);

float de_gui_node_get_min_width(de_gui_node_t* node);

float de_gui_node_get_min_height(de_gui_node_t* node);

float de_gui_node_get_max_width(de_gui_node_t* node);

float de_gui_node_get_max_height(de_gui_node_t* node);