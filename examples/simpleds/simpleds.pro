# compile with 32bit compiler
# place the resulting file "simpleds.ds" into \Windows\twain_32
# do not forget to have all dependencies present in the PATH

QT += quick multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = simpleds
TARGET_EXT = .ds
TEMPLATE = lib

CONFIG += c++11
DEFINES += TWPP_IS_DS
INCLUDEPATH += $$PWD/../../

DEF_FILE = exports.def

SOURCES += simpleds.cpp \
    camerasever.cpp \
    imageprovider.cpp
HEADERS += simpleds.hpp \
    twglue.hpp \
    camerasever.h \
    imageprovider.h

DISTFILES += \
    exports.def

#RESOURCES += \
#    images.qrc
RESOURCES += \
    qml.qrc

LIBS += -lUser32
