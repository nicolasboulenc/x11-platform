#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>


uint16_t win_width = 480;
uint16_t win_height = 480;
xcb_connection_t *con;
xcb_screen_t *scr;
xcb_drawable_t win;
xcb_gcontext_t gc;
xcb_image_t *buffer;

void redraw();

int main() {


	con = xcb_connect(NULL, NULL);
	scr = xcb_setup_roots_iterator(xcb_get_setup(con)).data;


	gc = xcb_generate_id(con);
	uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t gc_values[] = { scr->black_pixel, 1 };
	xcb_create_gc(con, gc, scr->root, gc_mask, gc_values);
	// xcb_create_gc(con, gc, scr->root, 0, 0);


	win = xcb_generate_id(con);
	uint32_t window_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t window_values[] = { scr->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS };
	xcb_create_window(con,
			XCB_COPY_FROM_PARENT,
			win,
			scr->root,
			0, 0, win_width, win_height,
			10,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			XCB_COPY_FROM_PARENT,
			window_mask, window_values );


	xcb_map_window(con, win);
	xcb_flush(con);

	xcb_generic_event_t *event;
	while(event = xcb_wait_for_event(con)) {
		switch(event->response_type) {
			case XCB_EXPOSE:
				printf("Expose\n");
				redraw();
				break;
			case XCB_KEY_PRESS:
				printf("Key press\n");
				return 0;
		}
	}

	return 0;
}


void redraw() {

	if(buffer == NULL) {
		buffer = xcb_image_create_native(con,
							win_width, win_height,
							XCB_IMAGE_FORMAT_Z_PIXMAP,
							0,
							NULL, 0, NULL);
		// buffer = xcb_image_get(con, win, 0, 0, win_width, win_height, 0, XCB_IMAGE_FORMAT_Z_PIXMAP);
		for(int i=0; i< 100 * 100 * 4; i+=4) {
			buffer->data[i + 0] = 0x00; // b
			buffer->data[i + 1] = 0x00;	// g
			buffer->data[i + 2] = 0xff;	// r	
			buffer->data[i + 3] = 0xff;
		}
	}
}