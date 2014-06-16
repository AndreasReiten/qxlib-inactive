LIBS += -lOpenCL
QMAKE_CXXFLAGS += -std=c++0x # C++11 
QT += core gui opengl widgets script
TARGET = qxlib
QMAKE_MAKEFILE = Makefile

OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
UI_DIR = .ui

HEADERS += \
    qxfile/utils/fileformat.h \
    qxfile/utils/filetreeview.h \
    qxfile/qxfilelib.h \
    qximage/utils/imagepreview.h \
    qximage/qximagelib.h \
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
    qxsvo/qxsvolib.h \
    qxlib.h

SOURCES += \
    qxfile/utils/fileformat.cpp \
    qxfile/utils/filetreeview.cpp \
    qxfile/qxfilelib.cpp \
    qximage/utils/imagepreview.cpp \
    qximage/qximagelib.cpp \
    qxmath/qxmathlib.cpp \
    qxopencl/utils/contextcl.cpp \
    qxopencl/utils/devicecl.cpp \
    qxopencl/qxopencllib.cpp \
    qxopengl/utils/openglwindow.cpp \
    qxopengl/utils/sharedcontext.cpp \
    qxopengl/utils/transferfunction.cpp \
    qxopengl/qxopengllib.cpp \
    qxsvo/qxsvolib.cpp
