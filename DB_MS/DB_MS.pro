TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=c99
LIBS += -lm

SOURCES += main.c \
    lib.c \
    bit.c

HEADERS += \
    lib.h \
    bit.h

