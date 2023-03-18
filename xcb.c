
// gcc xcb.c -o xcb -l xcb

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

void update();

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_window_t win;
xcb_pixmap_t pixmap_id;
xcb_void_cookie_t pixmap;
xcb_gcontext_t gc;


int main() {

	char *title = "Hello World !";

	// Open the connection to the X server
	conn = xcb_connect(NULL, NULL);

	// Get the first screen
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	printf ("\n");
	printf ("Informations of screen %u:\n", screen->root);
	printf ("  screen width..: %u\n", screen->width_in_pixels);
	printf ("  screen height.: %u\n", screen->height_in_pixels);
	printf ("  root depth....: %u\n", screen->root_depth);
	printf ("  allowed depth.: %u\n", screen->allowed_depths_len);
	printf ("  white pixel...: %u\n", screen->white_pixel);
	printf ("  black pixel...: %u\n", screen->black_pixel);
	printf ("\n");

	// Ask for our window's Id
	win = xcb_generate_id(conn);
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[] = { XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS };

	// Create the window
	xcb_create_window(	conn,							// Connection
						XCB_COPY_FROM_PARENT,			// depth
						win,							// window Id
						screen->root,					// parent window
						0, 0,							// x, y
						300, 300,						// width, height
						10,								// border_width
						XCB_WINDOW_CLASS_INPUT_OUTPUT,	// class
						// screen->root_visual,			// visual
						XCB_COPY_FROM_PARENT,			// visual
						mask, values);					// masks, not used

	// Set the title of the window
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, win,
						XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
						strlen(title), title);

 
	// create graphic context
	uint32_t gc_mask;
	uint32_t gc_values[3];

	gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	gc_values[0] = screen->black_pixel;
	gc_values[1] = screen->white_pixel;
	gc_values[2] = 0;

	gc = xcb_generate_id(conn);
	xcb_create_gc(conn, gc, win, gc_mask, gc_values);


	// create pixmap
	pixmap_id = xcb_generate_id(conn);
	pixmap = xcb_create_pixmap( conn,
						screen->root_depth,     // depth of the screen
						pixmap_id,  			// id of the pixmap
						win,
						300, 300 );  			// pixel height of the window


	// Map the window on the screen
	xcb_map_window(conn, win);
	xcb_flush(conn);
	
	xcb_generic_event_t *event;
	// should use xcb_poll_for_event instead?
	while(event = xcb_wait_for_event(conn)) {
		switch (event->response_type & ~0x80) {
			case XCB_EXPOSE: {
				xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
				printf ("Window %u exposed, redraw at (%u, %u), dimensions (%u, %u)\n",
						expose->window, expose->x, expose->y, expose->width, expose->height );
				update();
				break;
			}
			case XCB_KEY_PRESS: {
				xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
				printf ("Key pressed in window %u \n", kp->event);
				exit(0);
				break;
			}
			default: {
				printf ("Unknown event: %u\n", event->response_type);
				break;
			}
		}

		free(event);
		update();
	}
	return 0;
}


void update() {
	
	xcb_copy_area(conn, pixmap_id, win, gc,
		0, 0, 0, 0, 300, 300);
	
	xcb_flush(conn);
}