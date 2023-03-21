
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_util.h>


xcb_connection_t *connection;
xcb_screen_t *screen;
xcb_window_t win;
xcb_gcontext_t gc;
xcb_pixmap_t backing_pixmap;
xcb_visualtype_t *visual;

struct pixel_buffer {
    int w;
    int h;
    int bytes_per_row;
    void *pixels;
} buffer;



xcb_window_t create_window() {

	uint32_t mask;
	uint32_t values[2];

	xcb_window_t window;
	xcb_void_cookie_t cookie;

	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = screen->white_pixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;

	window = xcb_generate_id(connection);
	cookie = xcb_create_window(connection,
						XCB_COPY_FROM_PARENT, window, screen->root,
						0, 0, 300, 300,
						0,
						XCB_WINDOW_CLASS_INPUT_OUTPUT,
						*visual,
						mask, values);

	xcb_map_window(connection, window);

	return window;
}


void handle_event(xcb_connection_t *c, xcb_generic_event_t *ev)
{
    switch (ev->response_type & ~0x80) {
        case 0: {
            xcb_generic_error_t *err = (xcb_generic_error_t*)ev;
            xcb_errors_context_t *err_ctx;
            xcb_errors_context_new(c, &err_ctx);
            const char *major, *minor, *extension, *error;
            major = xcb_errors_get_name_for_major_code(err_ctx, err->major_code);
            minor = xcb_errors_get_name_for_minor_code(err_ctx, err->major_code, err->minor_code);
            error = xcb_errors_get_name_for_error(err_ctx, err->error_code, &extension);
            printf("XCB Error: %s:%s, %s:%s, resource %u sequence %u\n",
                error, extension ? extension : "no_extension",
                major, minor ? minor : "no_minor",
                (unsigned int)err->resource_id,
                (unsigned int)err->sequence);
            xcb_errors_context_free(err_ctx);
            break;
        }
        case XCB_BUTTON_PRESS:{
            break;
        }
        case XCB_EXPOSE: {
            xcb_expose_event_t *eev = (xcb_expose_event_t *)ev;
            xcb_copy_area(c, backing_pixmap, win, gc,
                        eev->x, eev->y,
                        eev->x, eev->y,
                        eev->width, eev->height);
            xcb_flush(c);
            break;
        } 
    }
}


xcb_visualtype_t *visual_by_id(xcb_screen_t *screen) {

    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    int depth;
    xcb_visuatype_t *visual = NULL;

    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);

        depth = depth_iter.data->depth;

        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (screen->root_visual == visual_iter.data->visual_id) {
                visual = visual_iter.data;
                goto found;
            }
        }
    }

    printf("Failed to match visual id\n");
    return NULL;
found:
    if (visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR &&
        visual->_class != XCB_VISUAL_CLASS_DIRECT_COLOR) {
        printf("Unsupported visual\n");
        return NULL;
    }

    printf("depth %i, R mask %x G mask %x B mask %x\n",
           depth, visual->red_mask, visual->green_mask, visual->blue_mask);

    return visual;
}


int bpp_by_depth(xcb_connection_t *c, int depth) {

    const xcb_setup_t *setup = xcb_get_setup(c);
    xcb_format_iterator_t fmt_iter = xcb_setup_pixmap_formats_iterator(setup);

    for (; fmt_iter.rem; xcb_format_next(&fmt_iter)) {
        if (fmt_iter.data->depth == depth)
            return fmt_iter.data->bits_per_pixel;
    }

    return 0;
}


void init_gc(xcb_connection_t *c) {

    uint32_t values[] = {screen->black_pixel, screen->white_pixel};
    gc = xcb_generate_id(c);
    xcb_create_gc(c, gc, backing_pixmap, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, values);
}


void init_backing_pixmap(xcb_connection_t *c) {

    pixmap = xcb_generate_id(c);
    xcb_create_pixmap(c, screen->root_depth, backing_pixmap, win, w, h);

    init_gc(c);
}


/*
 * Note that without copying the data to a temporary buffer we can send updates
 * only as a horizontal stripes.
 */
void update_backing_pixmap(xcb_connection_t *c, int x, int y, int w, int h) {

    /* Send image data to X server */
    xcb_put_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, backing_pixmap,
                  gc, w, h, 0, y, 0,
                  screen->root_depth, buffer.bytes_per_row * h,
                  buffer.pixels +
                  buffer.bytes_per_row * y);

    /* Copy updated data to window */
    xcb_copy_area(c, backing_pixmap, win, gc, x, y, x, y, w, h);
    xcb_flush(c);
}


int main() {
	
	connection = xcb_connect(NULL, NULL); // Callers need to use xcb_connection_has_error() to check for failure.
	screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
	
    visual = visual_by_id(&screen);

	win = create_window();

    xcb_generic_event_t *event;
	while(event = xcb_wait_for_event(connection)) {
        handle_event(connection, event);
    }

	puts("bye!");
	return EXIT_SUCCESS;
}