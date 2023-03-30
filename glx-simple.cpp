
// https://github.com/gamedevtech/X11OpenGLWindow
// spronovo@DESKTOP-4RE4J3H:~$ export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA
// spronovo@DESKTOP-4RE4J3H:~$ glxinfo -B
// https://github.com/microsoft/wslg/wiki/GPU-selection-in-WSLg



#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>		// clock_gettime, nano_sleep

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

#define WIDTH   512
#define HEIGHT  512

int main() {

    Display *display = XOpenDisplay(NULL);

    int attr_list[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *visual = glXChooseVisual(display, 0, attr_list);
    if (!visual) {
        fprintf(stderr, "Unable to choose visual\n");
        exit(1);
    }
    printf("Visual ID: %li\n", visual->visualid);

    GLXContext gl_context = glXCreateContext(display, visual, 0, True);
    if (!gl_context) {
        fprintf(stderr, "Unable to create GL context\n");
        exit(1);
    }

    Window window = XCreateSimpleWindow(
        display,
        XDefaultRootWindow(display),    // parent
        0, 0,                           // x, y
        WIDTH, HEIGHT,                  // width, height
        0,                              // border width
        0x00000000,                     // border color
        0x00000000                      // background color
    );

    glXMakeCurrent(display, window, gl_context);

    XStoreName(display, window, "OpenGL");
    XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
    XMapWindow(display, window);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // unsigned char data[] = { 255, 0, 0, 255 };
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	uint8_t *fb = (uint8_t *)malloc(4 * WIDTH * HEIGHT);
	if (!fb) {
		fputs("malloc()\n", stderr);
		return -1;
	}
    clock_t now = clock();
    int offx = now / (CLOCKS_PER_SEC / 500) % WIDTH;
    int offy = now / (CLOCKS_PER_SEC / 500) % HEIGHT;
    for (int y = 0; y < HEIGHT; y++) {
        uint16_t r = ((y + offy) % 256);
        for (int x = 0; x < WIDTH; x++) {
            uint16_t b = ((x + offx) % 256);
            int idx = (y * WIDTH + x) * 4;
            fb[idx + 0] = b;
            fb[idx + 1] = 0x00;
            fb[idx + 2] = r;
            fb[idx + 3] = 0xff;
        }
    }    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);

    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        printf("error! %u", err);
    }


    bool quit = false;
    while (!quit) {
        while (XPending(display) > 0) {
            XEvent event = {0};
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                KeySym keysym = XLookupKeysym(&event.xkey, 0);
                if (keysym == XK_Escape) {
                    quit = true;
                }
            }
        }

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glBindTexture(GL_TEXTURE_2D, tex);
        glEnable(GL_TEXTURE_2D);

        clock_t now = clock();
        int offx = now / (CLOCKS_PER_SEC / 500) % WIDTH;
        int offy = now / (CLOCKS_PER_SEC / 500) % HEIGHT;
        for (int y = 0; y < HEIGHT; y++) {
            uint16_t r = ((y + offy) % 256);
            for (int x = 0; x < WIDTH; x++) {
                uint16_t b = ((x + offx) % 256);
                int idx = (y * WIDTH + x) * 4;
                fb[idx + 0] = b;
                fb[idx + 1] = 0x00;
                fb[idx + 2] = r;
                fb[idx + 3] = 0xff;
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);


        // glNormal3f(0.0f,0.0f,-1.0f);
        // glColor3f(0.68, 0.84, 0.0); 

        glBegin(GL_QUADS);
            glTexCoord2f(-1.0f, -1.0f); 
            glVertex2f(-0.5f, -0.5f); 

            glTexCoord2f(1.0f, -1.0f); 
            glVertex2f(0.5f, -0.5f); 

            glTexCoord2f(1.0f, 1.0f); 
            glVertex2f(0.5f, 0.5f); 

            glTexCoord2f(-1.0f, 1.0f); 
            glVertex2f(-0.5f, 0.5f);
        glEnd();

        glXSwapBuffers(display, window);
    }

    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, gl_context);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    // delete(fb);
    exit(0);
}
