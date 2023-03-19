
// https://gist.github.com/skeeto/4ea7801438a01483cf5bdfa92d9f33f7

// cc demo.c -lX11
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <X11/Xutil.h>

static void draw(uint32_t *fb, int w, int h)
{
    clock_t now = clock();
    int offx = now/(CLOCKS_PER_SEC/587) % w;
    int offy = now/(CLOCKS_PER_SEC/593) % h;
    for (int y = 0; y < h; y++) {
        int r = ((y+offy) % 256) << 16;
        for (int x = 0; x < w; x++) {
            int b = ((x+offx) % 256) << 0;
            fb[y*w+x] = r | b;
        }
    }
}

int main(void)
{
    int width = 1000, height = 700;

    Display *dpy = XOpenDisplay(0);
    if (!dpy) {
        fputs("XOpenDisplay()\n", stderr);
        return 1;
    }
    Window root = XDefaultRootWindow(dpy);
    XSync(dpy, True);

    int nxvisuals;
    XVisualInfo vinfo = {.screen = DefaultScreen(dpy)};
    XGetVisualInfo(dpy, VisualScreenMask, &vinfo, &nxvisuals);
    if (!XMatchVisualInfo(dpy, XDefaultScreen(dpy), 24, TrueColor, &vinfo)) {
        fputs("XMatchVisualInfo()\n", stderr);
        return 1;
    }

    XSetWindowAttributes attrs = {
        .colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone)
    };
    Window win = XCreateWindow(
        dpy, root, 100, 100, width, height, 0, vinfo.depth, InputOutput,
        vinfo.visual, CWBackPixel|CWColormap|CWBorderPixel, &attrs
    );
    XSelectInput(dpy, win, ExposureMask|KeyPressMask);
    XMapWindow(dpy, win);

    // Technically malloc() is incorrect since it may be freed later by Xlib
    // via XFree(). Unfortunately XCreateImage is impossible to use correctly,
    // so just make do with malloc() since, in practice, it usually works.
    uint32_t *fb = (uint32_t *)malloc(4*width*height);
    if (!fb) {
        fputs("malloc()\n", stderr);
        return 1;
    }
    XImage *ximage = XCreateImage(
        dpy, vinfo.visual, vinfo.depth, ZPixmap, 0, (char *)fb, width, height, 8, width*4
    );
    if (!ximage) {
        fputs("XCreateImage()\n", stderr);
        return 1;
    }

    XGCValues gcv = {0};
    GC gc = XCreateGC(dpy, root, GCGraphicsExposures, &gcv);

    // for (XEvent event; !XNextEvent(dpy, &event);) {
    //     switch (event.type) {
    //     case KeyPress:
    //     case Expose:
    //         draw(fb, width, height);
    //         XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, width, height);
    //         break;
    //     }
    // }

    while(1) {
      XEvent event;
      Bool s = XCheckWindowEvent(dpy, win, StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask, &event);
      if(s) {
        // process event
        printf("event");
      }
      draw(fb, width, height);
      XPutImage(dpy, win, gc, ximage, 0, 0, 0, 0, width, height);
    }

}