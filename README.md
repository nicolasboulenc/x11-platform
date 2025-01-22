# x11-platform


sudo apt install gcc g++ build-essential gdb
sudo apt install mesa-utils libx11-dev libx11-xcb-dev libwayland-dev libgl1 libgl-dev


<!-- for some reason sometimes gcc doesnt seem to work without g++? -->

gcc glx-simple.cpp -lX11 -lGL