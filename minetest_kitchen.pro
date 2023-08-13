TEMPLATE = app
CONFIG += console c++17 x11 opengl
CONFIG -= app_bundle
CONFIG -= qt

TARGET = minetest_kitchen

SOURCES += \
        functions.cpp \
        globals.cpp \
        main.cpp \
        point_light.cpp \
        screen_quad.cpp

LIBS += -L/home/andrey/irrlicht-1.9/lib/Linux/ -lIrrlicht \
    -L/usr/X11R6/lib$(LIBSELECT) -lXxf86vm -lXext

INCLUDEPATH += /home/andrey/irrlicht-1.9/include/

DISTFILES += \
    blend.frag \
    blur.vert \
    blur_h.frag \
    blur_v.frag \
    depth_sort.frag \
    depth_write.frag \
    lighting.frag \
    lighting.vert \
    rays.frag \
    rays.vert \
    shadow_write.frag

HEADERS += \
    functions.h \
    globals.h \
    point_light.h \
    screen_quad.h
