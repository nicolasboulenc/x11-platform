#/bin/bash

gcc glx-simple.cpp -lX11 -lGL -fpermissive -o glx-simple
gcc xcb.cpp -lX11 -lxcb -lX11-xcb -lGL -fpermissive -o xcb