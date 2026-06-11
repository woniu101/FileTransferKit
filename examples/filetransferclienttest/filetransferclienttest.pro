QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app

TARGET = filetransferclienttest

FILETRANSFERKIT_ROOT = $$clean_path($$PWD/../..)
DESTDIR = $$FILETRANSFERKIT_ROOT/bin
include($$FILETRANSFERKIT_ROOT/FileTransferKit.pri)

SOURCES += main.cpp
