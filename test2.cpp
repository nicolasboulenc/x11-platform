// gcc -I/usr/X11/include -L/usr/X11/lib -lX11 xlib_image.c
// bsd: gcc -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 xlib_image.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

void draw(char *rgb_out, int w, int h)
{
        for(int i=0; i<w*h*4; i++)
                *rgb_out++ = rand();
}

XImage *create_ximage(Display *display, Visual *visual, int width, int height)
{
        char *image32 = (char *)malloc(width * height * 4);
        draw(image32, width, height);
        return XCreateImage(display, visual, 24, ZPixmap, 0, image32, width, height, 32, 0);
}

int x11_err_handler(Display *pd, XErrorEvent *pxev)
{
        char msg[4096] = { 0 };
        XGetErrorText(pd, pxev -> error_code, msg, sizeof(msg));
        printf("%s\n", msg);
        return 0;
}

int main(int argc, char **argv)
{
        Display* display = XOpenDisplay(NULL);
         Window window = XCreateSimpleWindow(
        display, XDefaultRootWindow(display),
        100, 100, 200, 200, 4, 0, 0);

        XMapWindow(display, window);
        XSelectInput(display, window, KeyPressMask | ButtonPressMask | ExposureMask);

        XWindowAttributes wa;
        XGetWindowAttributes(display, window, &wa);
        int width = wa.width, height = wa.height;
        printf("res: %dx%d\n", width, height);

        GC gc = XCreateGC(display, window, 0, NULL);
        Visual *visual = DefaultVisual(display, 0);


        XSetErrorHandler(x11_err_handler);

        XEvent event;
        while (1)
        {
            XNextEvent(display, &event);

            XImage *ximage = create_ximage(display, visual, width, height);
            XPutImage(display, window, gc, ximage, 0, 0, 0, 0, width, height);
            XDestroyImage(ximage);

            XFlush(display);
        }

        return 0;
}