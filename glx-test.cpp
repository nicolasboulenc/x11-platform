#include <GL/glx.h>
#include <GL/gl.h>
#include <string.h>
#include <unistd.h>
 
static int AttributeList[] = { GLX_RGBA, None };
 
static Bool WaitForNotify(Display *d, XEvent *e, char *arg) {
   return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
}
 
void setup_glx12(Display *dpy) {
   XVisualInfo *vi;
   Colormap cmap;
   XSetWindowAttributes swa;
   Window win;
   GLXContext cx;
   XEvent event;
 
   /* Get an appropriate visual */
   vi = glXChooseVisual(dpy, DefaultScreen(dpy), AttributeList);
 
   /* Create a GLX context */
   cx = glXCreateContext(dpy, vi, 0, GL_FALSE);
 
   /* Create a colormap */
   cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),vi->visual, AllocNone);
 
   /* Create a window */
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask = StructureNotifyMask;
   win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 100, 100, 0, vi->depth, InputOutput,
                       vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);
   XMapWindow(dpy, win);
   XIfEvent(dpy, &event, WaitForNotify, (char*)win);
 
   /* Connect the context to the window */
   glXMakeCurrent(dpy, win, cx);
}
 
void setup_glx13(Display *dpy) {
   GLXFBConfig *fbc;
   XVisualInfo *vi;
   Colormap cmap;
   XSetWindowAttributes swa;
   Window win;
   GLXContext cx;
   GLXWindow gwin;
   XEvent event;
   int nelements;
 
   /* Find a FBConfig that uses RGBA.  Note that no attribute list is */
   /* needed since GLX_RGBA_BIT is a default attribute.               */
   fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), 0, &nelements);
   vi = glXGetVisualFromFBConfig(dpy, fbc[0]);
 
   /* Create a GLX context using the first FBConfig in the list. */
   cx = glXCreateNewContext(dpy, fbc[0], GLX_RGBA_TYPE, 0, GL_FALSE);
 
   /* Create a colormap */
   cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),vi->visual, AllocNone);
 
   /* Create a window */
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask = StructureNotifyMask;
   win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0, 100, 100, 0, vi->depth, InputOutput,
                       vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);
   XMapWindow(dpy, win);
   XIfEvent(dpy, &event, WaitForNotify, (char*)win);
 
   /* Create a GLX window using the same FBConfig that we used for the */
   /* the GLX context.                                                 */
   gwin = glXCreateWindow(dpy, fbc[0], win, 0);
 
   /* Connect the context to the window for read and write */
   glXMakeContextCurrent(dpy, gwin, gwin, cx);
}
 
int main(int argc, char **argv) {
   Display *dpy;
   GLXContext cx;
   XEvent event;
   int major, minor;
   char *string_data;
 
   /* get a connection */
   dpy = XOpenDisplay(0);
   if(dpy == NULL) {

   }
 
   /*  */
   if (glXQueryVersion(dpy, &major, &minor)) {
     if (major == 1) {
    //    if (minor < 3) setup_glx12(dpy);
    //    else {
    //      string_data = glXQueryServerString(dpy, DefaultScreen(dpy), GLX_VERSION);
    //      if (strchr(string_data,"1.3")) setup_glx13(dpy);
    //      else setup_glx12(dpy);
    //    }
       setup_glx13(dpy);
       /* clear the buffer */
       glClearColor(1,1,0,1);
       glClear(GL_COLOR_BUFFER_BIT);
       glFlush();
 
       /* wait a while */
       sleep(10);
     }
   }
}