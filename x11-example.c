// 1) gcc x11-example.c -o x11-example -l X11


#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <stdio.h>
#include <stdlib.h>

Display *dis;
int screen;
Window win;
GC gc;
XImage *buffer;


void init_x();
void close_x();
void redraw();


int main() {

	XEvent event;
	KeySym key;
	char buffer[255];

	init_x();

	while(1) {
		// Note: only events we set the mask for are detected!
		XNextEvent(dis, &event);

		if (event.type == Expose && event.xexpose.count == 0) {
			// the window was exposed redraw it!
			redraw();
		}

		if (event.type == KeyPress && XLookupString(&event.xkey, buffer, 255, &key, 0) == 1) {
			// Use the XLookupString routine to convert the event KeyPress data into regular buffer. Weird but necessary...
			printf("You pressed %c key\n", buffer[0]);
			if (buffer[0]=='q') {
				close_x();
			}
		}

		if (event.type==ButtonPress) {
			int x = event.xbutton.x;
			int y = event.xbutton.y;
			printf("You clicked (%u, %u)\n", x, y);
		}
	}
}


void init_x() {
	unsigned long black;
	unsigned long white;

	dis = XOpenDisplay(NULL);
	screen = DefaultScreen(dis);
	black = BlackPixel(dis, screen),
	white = WhitePixel(dis, screen);
	win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, 300, 300, 5, black, white);
	XSetStandardProperties(dis, win, "Hello world!", NULL, None, NULL, 0, NULL);
	XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);
	gc = XCreateGC(dis, win, 0, 0);
	XSetBackground(dis, gc, white);
	XSetForeground(dis, gc, black);
	XClearWindow(dis, win);
	XMapRaised(dis, win);

};


void close_x() {
	XFreeGC(dis, gc);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);
	exit(1);
};


void redraw() {
	XClearWindow(dis, win);

	if(buffer == NULL) {
		buffer = XGetImage(dis, win, 0, 0, 100, 100, AllPlanes, ZPixmap);
		for(int i=0; i< 100 * 100 * 4; i+=4) {
			buffer->data[i + 0] = 0x00; // b
			buffer->data[i + 1] = 0x00;	// g
			buffer->data[i + 2] = 0xff;	// r	
			buffer->data[i + 3] = 0xff;
		}
	}

	printf("0x%06lx\n", buffer->red_mask);
	printf("0x%06lx\n", buffer->green_mask);
	printf("0x%06lx\n", buffer->blue_mask);
	printf("%u\n", buffer->bits_per_pixel);
	XPutImage(dis, win, gc, buffer, 0, 0, 0, 0, 100, 100);
};
