// 1) gcc x11-example.c -o x11-example -l X11

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>	// clock_gettime, nano_sleep

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// using namespace std;

typedef struct {

	Display *dis;
	Window win;
	GC gc;
	XImage *ximage;
	uint16_t width;
	uint16_t height;
	timespec timer;
} t_app;

t_app app = { .width = 1000, .height = 1000 };



void app_init(t_app *app);
void app_close(t_app *app);
void loop(t_app *app);
void draw(t_app *app);


int main() {

	XEvent event;
	KeySym key;
	char buffer[255];

	app_init(&app);
	clock_gettime(CLOCK_REALTIME, &(app.timer));

	while(1) {
		// Note: only events we set the mask for are detected!
		// XNextEvent(dis, &event);

		XEvent event;
		Bool s = True;
		while(s) {
			s = XCheckWindowEvent(app.dis, app.win, StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask, &event);
			printf("event %i\n", s);
			if (event.type == Expose && event.xexpose.count == 0) {
				// the window was exposed redraw it!
				// draw(&app);
			}

			if (event.type == KeyPress && XLookupString(&event.xkey, buffer, 255, &key, 0) == 1) {
				// Use the XLookupString routine to convert the event KeyPress data into regular buffer. Weird but necessary...
				// printf("You pressed %c key\n", buffer[0]);
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
		long int time_elapsed = (tmp.tv_nsec - app.timer.tv_nsec);
		while(time_elapsed < 33000000) {
			timespec sleep_time = { .tv_sec = 0, .tv_nsec = (33000000 - time_elapsed) };
			// printf("%li\n", sleep_time.tv_nsec);
			timespec remain_time;
			int ret = clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, &remain_time);
			// int ret = nanosleep(&sleep_time, &remain_time);
			if(ret == -1) {
				printf("interrupted %li\n",remain_time.tv_nsec/(long int)1000000);
			}
			clock_gettime(CLOCK_REALTIME, &tmp);
			time_elapsed = (tmp.tv_nsec - app.timer.tv_nsec);
		}

		draw(&app);
		// printf("%li\n",(tmp.tv_nsec - app.timer.tv_nsec)/(long int)1);
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
	uint32_t *fb = (uint32_t *)malloc(4 * app->width * app->height);
	if (!fb) {
		fputs("malloc()\n", stderr);
		return;
	}

	app->ximage = XCreateImage(app->dis, vinfo.visual, vinfo.depth, ZPixmap, 0, (char *)fb, app->width, app->height, 8, app->width * 4);
	if (!app->ximage) {
		fputs("XCreateImage()\n", stderr);
		return;
	}

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


void draw(t_app *app) {
	// printf("draw\n");

	// XClearWindow(app->dis, app->win);

	timespec tmp;
	clock_gettime(CLOCK_REALTIME, &tmp);
	uint8_t r = tmp.tv_nsec % (long int)255;

	if(app->ximage != NULL) {
		for(int i=0; i < app->width * app->height * 4; i+=4) {
			app->ximage->data[i + 0] = 0xff; 	// b
			app->ximage->data[i + 1] = 0x00;	// g
			app->ximage->data[i + 2] = r;	// r	
			app->ximage->data[i + 3] = 0xff;
		}
		// printf("0x%06lx\n", buffer->red_mask);
		// printf("0x%06lx\n", buffer->green_mask);
		// printf("0x%06lx\n", buffer->blue_mask);
		// printf("%u\n", buffer->bits_per_pixel);
	}

	XPutImage(app->dis, app->win, app->gc, app->ximage, 0, 0, 0, 0, app->width, app->height);
};
