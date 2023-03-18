#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>


uint32_t win_width = 480;
uint32_t win_height = 480;
xcb_connection_t *con;
xcb_screen_t *scr;
xcb_drawable_t win;
xcb_gcontext_t gc;
xcb_colormap_t map;

int main() {


	con = xcb_connect(NULL, NULL);
	scr = xcb_setup_roots_iterator(xcb_get_setup(con)).data;


	gc = xcb_generate_id(con);
	uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t gc_values[] = { scr->black_pixel, 1 };
	xcb_create_gc(con, gc, scr->root, gc_mask, gc_values);


	win = xcb_generate_id(con);
	uint32_t window_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t window_values[] = { scr->white_pixel, XCB_EVENT_MASK_EXPOSURE };
	xcb_create_window(con,
			XCB_COPY_FROM_PARENT,
			win,
			scr->root,
			0, 0, win_width, win_height,
			10,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			XCB_COPY_FROM_PARENT,
			window_mask, window_values );


	map = xcb_generate_id(con);
	xcb_create_colormap(con,
			XCB_COLORMAP_ALLOC_NONE,
			map,
			win,
			scr->root_visual);


	xcb_map_window(con, win);
	xcb_flush(con);


	xcb_image_t *image = xcb_image_create_native(con,
							480, 480,
							XCB_IMAGE_FORMAT_Z_PIXMAP,
							scr->root_depth,
							NULL, 0, NULL);


	if (!image) {
		printf("No image\n");
	}
	else {

		for(int i=0; i<image->size; i+=3) {
			image->data[i + 0] = 0xff;
			image->data[i + 1] = 0x00;
			image->data[i + 2] = 0x00;
		}
		xcb_image_put(con, win, gc, image, 0, 0, 0);
		// xcb_copy_area(con, image, win, gc, 0, 0, 0, 0, 480, 480);
		xcb_flush(con);
	}
	sleep(2);
	return 0;
}