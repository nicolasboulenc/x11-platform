#include <stdlib.h>
#include <stdio.h>

#include <X11/Xutil.h>
#include <X11/Xos.h>

int main (int argc, char **argv) 
{
	Display *dpy;
	Window win;
	Window rootwin;
	int screen;
	XEvent event;
	XWindowAttributes rootwinattribs;
	int *framebuf = 0;
	XImage *ximage;
	XVisualInfo xvisinfo;
	Visual *xvisual;	
	GC gc;
	Pixmap pixmap;
	XSetWindowAttributes attribs= {0};

	int width;
	int height;
	int i;

	dpy = XOpenDisplay(0);
	screen = DefaultScreen(dpy);

	rootwin = RootWindow(dpy, screen);

	XGetWindowAttributes(dpy, rootwin, &rootwinattribs);

	width = rootwinattribs.width;
	height = rootwinattribs.height;

	framebuf = malloc(width*height*4);
	if (!framebuf) {
		perror("malloc");
		exit(1);
	}

	for (i = 0; i < width*height; ++i) {
		*(framebuf+i) = 0xF0F0F0FF;
	}

	attribs.border_pixel = 0;
	attribs.event_mask = KeyPressMask|KeyReleaseMask|ExposureMask;
	XMatchVisualInfo(dpy, screen, 32, DirectColor, &xvisinfo);
	xvisual = xvisinfo.visual;

        /*The 24 for depth, I arrived at that as a guess. Other values, ie 32, result in a BadMatch Error*/
	win = XCreateWindow(dpy, rootwin, 0, 0, width, height, 0, CopyFromParent, InputOutput, xvisual, CWEventMask|CWBorderPixel, &attribs);
	pixmap = XCreatePixmap(dpy, win, width, height, 24);
	ximage = XCreateImage(dpy, xvisual, 24, ZPixmap, 0, (char *)framebuf, width, height, 32, 0);
	if (!ximage) {
		printf("XCreateImage() failed. Exiting...");
		exit(1);
	}

	XPutImage(dpy, pixmap, DefaultGC(dpy, screen), ximage, 0, 0, 0, 0, width, height);
	XCopyArea(dpy, pixmap, win, DefaultGC(dpy, screen), 0, 0, width, height, 0, 0);
	XSelectInput(dpy, win, ExposureMask|KeyPressMask);
	XMapWindow(dpy, win);


    while(!XNextEvent(dpy, &event)) {
    	if (event.type == KeyPress) break;
        if (event.type == Expose) {
            XCopyArea(dpy, pixmap, win, DefaultGC(dpy, screen), 0, 0, width, height, 0, 0);
        }
    };

	return 0;
}

