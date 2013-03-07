TEMPLATE = app
TARGET = gui
VPATH += ../protocol/
INCLUDEPATH += $$VPATH
QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Input
HEADERS += main.h simulator.h protocol.h
SOURCES += main.cpp simulator.cpp
