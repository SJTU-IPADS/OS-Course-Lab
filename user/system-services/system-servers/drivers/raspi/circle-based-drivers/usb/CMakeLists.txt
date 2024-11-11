link_libraries(pipe ${circle_home}/lib/usb/libusb.a ${circle_home}/lib/input/libinput.a)
add_circle_lib(usb_circle main.cpp)