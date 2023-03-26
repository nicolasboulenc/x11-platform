
#include <stdlib.h>		// malloc, exit
#include <stdint.h>		// uint16_t...
#include <stdio.h>		// printf, fputs...
#include <time.h>		// clock_gettime, nano_sleep

#include <X11/Xlib.h>
#include <X11/Xutil.h>


typedef struct {
	Display *dis;
	Window win;
	GC gc;
	XImage *ximage;
	Pixmap *pixmap;
	uint16_t width;
	uint16_t height;
	timespec timer;
} t_app;

t_app app = { .width = 1000, .height = 1000 };

uint16_t fps = 60;
long int frame_time_ns = (1000 * 1000000) / fps;


void app_init(t_app *app);
void app_close(t_app *app);
void app_draw(t_app *app);


int main() {

	XEvent event;
	KeySym key;
	char buffer[255];

	app_init(&app);
	clock_gettime(CLOCK_REALTIME, &(app.timer));

	while(1) {

		XEvent event;
		Bool s = True;
		while(s) {
			s = XCheckWindowEvent(app.dis, app.win, StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask, &event);

			if (event.type == Expose && event.xexpose.count == 0) {
				app_draw(&app);
			}

			if (event.type == KeyPress && XLookupString(&event.xkey, buffer, 255, &key, 0) == 1) {
				if (buffer[0]=='q') {
					app_close(&app);
				}
			}

			if (event.type==ButtonPress) {
				int x = event.xbutton.x;
				int y = event.xbutton.y;
				printf("You clicked (%u, %u)\n", x, y);
			}
		}

		timespec tmp;
		clock_gettime(CLOCK_REALTIME, &tmp);
		long int time_elapsed_ns = tmp.tv_nsec - app.timer.tv_nsec;
		// to fix: how is time_elapsed getting negative???
		while(time_elapsed_ns > 0 && time_elapsed_ns < frame_time_ns) {
			timespec sleep_time = { .tv_sec = 0, .tv_nsec = (frame_time_ns - time_elapsed_ns) };
			int ret = clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, NULL);
			clock_gettime(CLOCK_REALTIME, &tmp);
			time_elapsed_ns = (tmp.tv_nsec - app.timer.tv_nsec);
		}

		app_draw(&app);
		// printf("%li\n", time_elapsed_ns);
		clock_gettime(CLOCK_REALTIME, &(app.timer));
	}
}


void app_init(t_app *app) {

	app->dis = XOpenDisplay(NULL);
	if (!app->dis) {
		fputs("XOpenDisplay()\n", stderr);
		return;
	}
	Window root = XDefaultRootWindow(app->dis);
	XSync(app->dis, True);

	int nxvisuals;
	XVisualInfo vinfo = { .screen = DefaultScreen(app->dis) };
	XGetVisualInfo(app->dis, VisualScreenMask, &vinfo, &nxvisuals);
	if (!XMatchVisualInfo(app->dis, XDefaultScreen(app->dis), 24, TrueColor, &vinfo)) {
		fputs("XMatchVisualInfo()\n", stderr);
		return;
	}

	XSetWindowAttributes attrs = { .colormap = XCreateColormap(app->dis, root, vinfo.visual, AllocNone) };
	app->win = XCreateWindow(
		app->dis, root, 100, 100, app->width, app->height, 0, vinfo.depth, InputOutput,
		vinfo.visual, CWBackPixel | CWColormap | CWBorderPixel, &attrs
	);

	XSelectInput(app->dis, app->win, ExposureMask | KeyPressMask);
	XMapWindow(app->dis, app->win);

	// Technically malloc() is incorrect since it may be freed later by Xlib
	// via XFree(). Unfortunately XCreateImage is impossible to use correctly,
	// so just make do with malloc() since, in practice, it usually works.
	uint8_t *fb = (uint8_t *)malloc(4 * app->width * app->height);
	if (!fb) {
		fputs("malloc()\n", stderr);
		return;
	}

	app->ximage = XCreateImage(app->dis, vinfo.visual, vinfo.depth, ZPixmap, 0, (char *)fb, app->width, app->height, 8, app->width * 4);
	if (!app->ximage) {
		fputs("XCreateImage()\n", stderr);
		return;
	}


	app->pixmap = XCreatePixmap(app->dis, app->win, app->width, app->height);

	XGCValues gcv = {0};
	app->gc = XCreateGC(app->dis, root, GCGraphicsExposures, &gcv);
	if (!app->gc) {
		fputs("XCreateGC()\n", stderr);
		return;
	}
};


void app_close(t_app *app) {
	XFreeGC(app->dis, app->gc);
	XDestroyWindow(app->dis, app->win);
	XCloseDisplay(app->dis);
	exit(0);
};


void app_draw(t_app *app) {

	clock_t now = clock();
	int offx = now / (CLOCKS_PER_SEC / 500) % app->width;
	int offy = now / (CLOCKS_PER_SEC / 500) % app->height;
	for (int y = 0; y < app->height; y++) {
		uint16_t r = ((y + offy) % 256);
		for (int x = 0; x < app->width; x++) {
			uint16_t b = ((x + offx) % 256);
			int idx = (y * app->width + x) * 4;
			app->ximage->data[idx + 0] = b;
			app->ximage->data[idx + 1] = 0x00;
			app->ximage->data[idx + 2] = r;
			app->ximage->data[idx + 3] = 0xff;
		}
	}
	XCopyArea(app->dis, app->ximage, app->pixmap, app->gc, 0, 0, app->width, app->height,  0, 0);
	// XPutImage(app->dis, app->win, app->gc, app->ximage, 0, 0, 0, 0, app->width, app->height);
};
