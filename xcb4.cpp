// gcc -I/usr/X11/include -L/usr/X11/lib -lX11 xlib_image.c
// bsd: gcc -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 xlib_image.c

#include <xcb/xcb.h>

struct win {
	xcb_screen_t *scr;
	xcb_window_t win;
	xcb_gcontext_t gc;
	xcb_pixmap_t pixmap;
	// xcb_shm_seg_t shmseg;

	int new_w;
	int new_h;

	int resized:1;
	int fullscreen:1;
};

gp_backend *gp_xcb_init(const char *display, int x, int y, int w, int h,
                        const char *caption)
{
	gp_backend *backend;
	struct win *win;
	size_t size = sizeof(gp_backend) + sizeof(struct win);

	backend = malloc(size);
	if (!backend)
		return NULL;

	memset(backend, 0, size);

	if (!x_connect(display))
		goto err;

	win = GP_BACKEND_PRIV(backend);

	if (create_window(backend, win, x, y, w, h, caption))
		goto err;

	backend->name = "XCB";
	backend->flip = x_flip;
	backend->update_rect = x_update_rect;
	backend->exit = x_exit;
	backend->poll = x_poll;
	backend->wait = x_wait;
	backend->set_attr = x_set_attr;
	backend->resize_ack = x_resize_ack;
	backend->fd = xcb_get_file_descriptor(x_con.c);

	return backend;
err:
	free(backend);
	return NULL;
}

static int create_window(struct gp_backend *self, struct win *win,
                         int x, int y, int w, int h,
                         const char *caption)
{
	xcb_connection_t *c = x_con.c;

	win->scr = xcb_setup_roots_iterator(xcb_get_setup(c)).data;

	/* Create window */
	win->win = xcb_generate_id(c);

	uint32_t mask_val[] = {win->scr->black_pixel,
	                       XCB_EVENT_MASK_EXPOSURE |
	                       XCB_EVENT_MASK_KEY_PRESS |
			       XCB_EVENT_MASK_KEY_RELEASE |
	                       XCB_EVENT_MASK_BUTTON_PRESS |
			       XCB_EVENT_MASK_BUTTON_RELEASE |
			       XCB_EVENT_MASK_POINTER_MOTION |
			       XCB_EVENT_MASK_STRUCTURE_NOTIFY |
	                       XCB_EVENT_MASK_FOCUS_CHANGE};

	xcb_create_window(c,
	                  XCB_COPY_FROM_PARENT,
			  win->win,
	                  win->scr->root,
	                  x, y, w, h,
			  0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  win->scr->root_visual,
	                  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
			  &mask_val);

	if (x_con.delete_window_supported) {
		xcb_change_property(c, XCB_PROP_MODE_REPLACE, win->win,
				    x_con.wm_protocols, 4, 32, 1, &x_con.wm_delete_window);
	}

	set_title(c, win->win, caption);

	//TODO: XCB key handling!!!
	gp_ev_queue_init(&self->event_queue, w, h, 0, GP_EVENT_QUEUE_LOAD_KEYMAP);

	/* Get pixel format */
	int depth;
	xcb_visualtype_t *visual = visual_by_id(win, &depth);

	if (!visual)
		return 1;

	if (visual->_class != XCB_VISUAL_CLASS_TRUE_COLOR &&
	    visual->_class != XCB_VISUAL_CLASS_DIRECT_COLOR) {
		GP_WARN("Unsupported visual->_class %i", visual->_class);
		return 1;
	};
	gp_pixel pixel_type = gp_pixel_rgb_match(visual->red_mask, visual->green_mask, visual->blue_mask, 0, bpp_by_depth(c, depth));

	if (pixel_type == GP_PIXEL_UNKNOWN) {
		GP_DEBUG(1, "Unknown pixel type");
		return 1;
	}

	create_backing_pixmap(self, c, pixel_type, w, h);

	/* Create GC */
	uint32_t values[] = {win->scr->black_pixel, win->scr->white_pixel};
	win->gc = xcb_generate_id(x_con.c);
	xcb_create_gc(x_con.c, win->gc, win->win, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, values);

	/* Finally show the window */
	xcb_map_window(c, win->win);
	xcb_flush(c);

	return 0;
}
