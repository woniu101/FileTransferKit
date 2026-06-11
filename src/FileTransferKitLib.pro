TEMPLATE = lib
CONFIG += staticlib c++11

TARGET = FileTransferKit

FILETRANSFERKIT_ROOT = $$clean_path($$PWD/..)
DESTDIR = $$FILETRANSFERKIT_ROOT/lib
include($$FILETRANSFERKIT_ROOT/FileTransferKitSources.pri)
