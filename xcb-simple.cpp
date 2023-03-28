#include <stdlib.h>		// malloc, exit
#include <stdint.h>		// uint16_t...
#include <stdio.h>		// printf, fputs...
#include <time.h>		// clock_gettime, nano_sleep

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_util.h>


typedef struct {
	xcb_connection_t *con;
	xcb_screen_t *scr;
	xcb_window_t win;
	xcb_visualid_t vis;
	xcb_gcontext_t gc;
	uint8_t *fb;
	xcb_pixmap_t pixmap;
	uint16_t width;
	uint16_t height;
	timespec timer;
} t_app;

t_app _app = { .width = 1000, .height = 1000 };
t_app *app = &_app;

uint16_t fps = 60;
long int frame_time_ns = (1000 * 1000000) / fps;


void app_init(t_app *app);
void app_close(t_app *app);
void app_update(t_app *app, long int time_elapsed_ns);


int main() {
	
	app_init(app);
	clock_gettime(CLOCK_REALTIME, &(app->timer));

	while(1) {

		while(1) {
			xcb_generic_event_t *event = xcb_poll_for_event(app->con);
			if(event == NULL) break;

			switch (event->response_type & ~0x80) {
				case XCB_KEY_PRESS:{
					xcb_disconnect(app->con);
					exit(0);
					break;
				}
				case XCB_EXPOSE: {
					app_update(app, 0);
					xcb_expose_event_t *eev = (xcb_expose_event_t *)event;
					xcb_copy_area(app->con, app->pixmap, app->win, app->gc,
								eev->x, eev->y,
								eev->x, eev->y,
								eev->width, eev->height);
					xcb_flush(app->con);
					break;
				} 
			}	
		}

		app_update(app, 0);

		timespec tmp;
		clock_gettime(CLOCK_REALTIME, &tmp);
		long int time_elapsed_ns = tmp.tv_nsec - app->timer.tv_nsec;
		// to fix: how is time_elapsed getting negative???
		while(time_elapsed_ns > 0 && time_elapsed_ns < frame_time_ns) {
			timespec sleep_time = { .tv_sec = 0, .tv_nsec = (frame_time_ns - time_elapsed_ns) };
			int ret = clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, NULL);
			clock_gettime(CLOCK_REALTIME, &tmp);
			time_elapsed_ns = (tmp.tv_nsec - app->timer.tv_nsec);
		}

		char buf[32];
		int n = sprintf(buf, "%5.2f", (float)time_elapsed_ns/(float)1000000.0);

		// XDrawString(app->dis, app->pixmap, app->gc, 10, 20, buf, 6);
		// XCopyArea(app->dis, app->pixmap, app->win, app->gc, 0, 0, app->width, app->height, 0, 0);

		xcb_copy_area(app->con, app->pixmap, app->win, app->gc, 0, 0, 0, 0, app->width, app->height);
		xcb_flush(app->con);

		// printf("%li\n", time_elapsed_ns);
		clock_gettime(CLOCK_REALTIME, &(app->timer));
	}

	puts("bye!");
	return EXIT_SUCCESS;
}


void app_init(t_app *app) {
	
	// Callers need to use xcb_connection_has_error() to check for failure.
	app->con = xcb_connect(NULL, NULL);
	app->scr = xcb_setup_roots_iterator(xcb_get_setup(app->con)).data;

	// visual
	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(app->scr);
	int depth;
	xcb_visualtype_t *visual = NULL;

	while(depth_iter.rem > 0) {
		xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
		depth = depth_iter.data->depth;
		while(visual_iter.rem > 0) {
			if (app->scr->root_visual == visual_iter.data->visual_id) {
				visual = visual_iter.data;
			}
			if(visual != NULL) break;
			xcb_visualtype_next(&visual_iter);
		}
		if(visual != NULL) break;
		xcb_depth_next(&depth_iter);
	}

	if(visual == NULL) {
		printf("Failed to match visual id\n");
		return;
	}

	if (visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR &&
		visual->_class != XCB_VISUAL_CLASS_DIRECT_COLOR) {
		printf("Unsupported visual\n");
		return;
	}

	printf("depth %i, R mask %x G mask %x B mask %x\n",
		   depth, visual->red_mask, visual->green_mask, visual->blue_mask);

	app->vis = visual->visual_id;

	// window
	uint32_t mask;
	uint32_t win_values[2];

	xcb_window_t window;
	xcb_void_cookie_t cookie;

	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	win_values[0] = app->scr->black_pixel;
	win_values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;

	app->win = xcb_generate_id(app->con);
	cookie = xcb_create_window(app->con,
						depth, app->win, app->scr->root,
						0, 0, app->width, app->height,
						0,
						XCB_WINDOW_CLASS_INPUT_OUTPUT,
						app->vis,
						mask, win_values);

	// pixmap
	app->pixmap = xcb_generate_id(app->con);
	xcb_create_pixmap(app->con, app->scr->root_depth, app->pixmap, app->win, app->width, app->height);

	// gc
	app->gc = xcb_generate_id(app->con);
	uint32_t gc_values[] = {app->scr->black_pixel, app->scr->white_pixel};
	xcb_create_gc(app->con, app->gc, app->pixmap, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, gc_values);

	// image
	// Technically malloc() is incorrect since it may be freed later by Xlib
	// via XFree(). Unfortunately XCreateImage is impossible to use correctly,
	// so just make do with malloc() since, in practice, it usually works.
	app->fb = (uint8_t *)malloc(4 * app->width * app->height);
	if (!app->fb) {
		fputs("malloc()\n", stderr);
		return;
	}

	xcb_map_window(app->con, app->win);
	xcb_flush(app->con);
}


void app_update(t_app *app, long int time_elapsed_ns) {

	clock_t now = clock();
	int offx = now / (CLOCKS_PER_SEC / 500) % app->width;
	int offy = now / (CLOCKS_PER_SEC / 500) % app->height;
	for (int y = 0; y < app->height; y++) {
		uint16_t r = ((offy - y) % 256);
		for (int x = 0; x < app->width; x++) {
			uint16_t b = ((offx - x) % 256);
			int idx = (y * app->width + x) * 4;
			app->fb[idx + 0] = b;
			app->fb[idx + 1] = 0x00;
			app->fb[idx + 2] = r;
			app->fb[idx + 3] = 0xff;
		}
	}

	xcb_put_image(app->con, XCB_IMAGE_FORMAT_Z_PIXMAP, app->pixmap,
				  app->gc, app->width, app->height, 0, 0, 0,
				  app->scr->root_depth, 4 * app->width * app->height,
				  app->fb);
}


/*
 * Note that without copying the data to a temporary buffer we can send updates
 * only as a horizontal stripes.
 */
// void update_backing_pixmap(xcb_connection_t *c, int x, int y, int w, int h) {

// 	/* Send image data to X server */
// 	xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, app->pixmap,
// 				  app->gc, app->width, app->height, 0, y, 0,
// 				  app->scr->root_depth, buffer.bytes_per_row * h,
// 				  buffer.pixels +
// 				  buffer.bytes_per_row * y);

// 	/* Copy updated data to window */
// 	xcb_copy_area(c, app->pixmap, app->win, app->gc, x, y, x, y, w, h);
// 	xcb_flush(c);
// }

