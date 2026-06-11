TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += qssh filetransferkitlib filetransferclienttest

qssh.file = 3rdparty/qsftp/src/src.pro

filetransferkitlib.file = src/FileTransferKitLib.pro
filetransferkitlib.depends = qssh

filetransferclienttest.file = examples/filetransferclienttest/filetransferclienttest.pro
filetransferclienttest.depends = filetransferkitlib
