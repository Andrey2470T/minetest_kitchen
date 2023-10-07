TEMPLATE = app
CONFIG += console c++17 x11 opengl
CONFIG -= app_bundle
CONFIG -= qt

TARGET = minetest_kitchen

SOURCES += \
        src/functions.cpp \
        src/globals.cpp \
        src/main.cpp \
        src/point_light.cpp \
        src/screen_quad.cpp

LIBS += -L/home/andrey/irrlicht-1.9/lib/Linux/ -lIrrlicht \
    -L/usr/X11R6/lib$(LIBSELECT) -lXxf86vm -lXext

INCLUDEPATH += /home/andrey/irrlicht-1.9/include/

DISTFILES += \
    shaders/blend.frag \
    shaders/blur.vert \
    shaders/blur_h.frag \
    shaders/blur_v.frag \
    shaders/depth_sort.frag \
    shaders/depth_write.frag \
    shaders/lighting.frag \
    shaders/lighting.vert \
    shaders/rays.frag \
    shaders/rays.vert \
    shaders/shadow_write.frag \
    shaders/shadow_write.vert \
    shadow_write.frag

HEADERS += \
    src/constant_setter.h \
    src/functions.h \
    src/globals.h \
    src/point_light.h \
    src/screen_quad.h
