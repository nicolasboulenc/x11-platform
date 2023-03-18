#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <SFML/Graphics.hpp>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main() {
  //Get X11-Display and Root-Window
  Display*          x11_display;
  Window            x11_window;
  XWindowAttributes x11_window_attr;
  x11_display = XOpenDisplay(NULL);
  x11_window =  RootWindow(x11_display, DefaultScreen(x11_display));
  XGetWindowAttributes(x11_display, x11_window, &x11_window_attr);

  //Create a fullscreen SFML-window
  sf::RenderWindow sfml_window(sf::VideoMode(x11_window_attr.width, x11_window_attr.height), 
                                "SFML Window", sf::Style::Fullscreen);
  x11_window = sfml_window.getSystemHandle();
  

  while (sfml_window.isOpen()) {
    //Get SFML events
    sf::Event event;
    while (sfml_window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        sfml_window.close();
      }
    }

    //Color the window (increase r, g and b on each rendering loop)
    static int cl = 0;
    sfml_window.clear(sf::Color(cl,cl,cl));
    cl = (cl+1) & 0xFF;

    //Update the SFML window
    sfml_window.display();
    
    //Wait 10ms (should not be required, but to be sure that it's not related to timing...)
    usleep(10000);

    //Get a screenshot and print a few pixel values
    XImage* x11_image;
    x11_image = XGetImage(x11_display, x11_window, 0, 0, x11_window_attr.width, 
                          x11_window_attr.height, AllPlanes, ZPixmap);
    printf("%d %d %d %d\n", x11_image->data[10], x11_image->data[20],  
                            x11_image->data[30], x11_image->data[40]);
    XDestroyImage(x11_image);
  }

  return 0;
}