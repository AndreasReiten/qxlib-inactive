LIBS += -lOpenCL
QMAKE_CXXFLAGS += -std=c++0x # C++11 
QT += core gui opengl widgets
TARGET = qxlib
TEMPLATE = lib
CONFIG += staticlib

HEADERS += \
    qxfile/utils/fileformat.h \
    qxfile/utils/filetreeview.h \
    qxfile/qxfilelib.h \
    qxmath/utils/ccmatrix.h \
    qxmath/utils/colormatrix.h \
    qxmath/utils/matrix.h \
    qxmath/utils/rotationmatrix.h \
    qxmath/utils/ubmatrix.h \
    qxmath/qxmathlib.h \
    qxopencl/utils/contextcl.h \
    qxopencl/utils/devicecl.h \
    qxopencl/qxopencllib.h \
    qxopengl/utils/openglwindow.h \
    qxopengl/utils/sharedcontext.h \
    qxopengl/utils/transferfunction.h \
    qxopengl/qxopengllib.h \
    qximage/utils/imagepreview.h \
    qximage/qximagelib.h \
    qxsvo/qxsvolib.h \
    qxlib.h \
    qxsvo/utils/bricknode.h \
    qxsvo/utils/searchnode.h \
    qxsvo/utils/sparsevoxelocttree.h

SOURCES += \
    qxfile/utils/fileformat.cpp \
    qxfile/utils/filetreeview.cpp \
    qxfile/qxfilelib.cpp \
    qxmath/qxmathlib.cpp \
    qxopencl/utils/contextcl.cpp \
    qxopencl/utils/devicecl.cpp \
    qxopencl/qxopencllib.cpp \
    qxopengl/utils/openglwindow.cpp \
    qxopengl/utils/sharedcontext.cpp \
    qxopengl/utils/transferfunction.cpp \
    qxopengl/qxopengllib.cpp \
    qximage/utils/imagepreview.cpp \
    qximage/qximagelib.cpp \
    qxsvo/qxsvolib.cpp \
    qxsvo/utils/bricknode.cpp \
    qxsvo/utils/searchnode.cpp \
    qxsvo/utils/sparsevoxelocttree.cpp

OTHER_FILES += \
    qxopengl/glsl/std_2d_col.f.glsl \
    qxopengl/glsl/std_2d_col.v.glsl \
    qxopengl/glsl/std_2d_tex.f.glsl \
    qxopengl/glsl/std_2d_tex.v.glsl \
    qxopengl/glsl/std_3d_col.f.glsl \
    qxopengl/glsl/std_3d_col.v.glsl \
    qxopengl/glsl/std_blend.f.glsl \
    qxopengl/glsl/std_blend.v.glsl \
    qxopengl/glsl/unitcell.f.glsl \
    qxopengl/glsl/unitcell.v.glsl
