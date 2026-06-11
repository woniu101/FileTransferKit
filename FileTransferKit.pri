isEmpty(FILETRANSFERKIT_ROOT) {
    error("Please set FILETRANSFERKIT_ROOT before including FileTransferKit.pri")
}

isEmpty(FILETRANSFERKIT_QSSH_ROOT) {
    FILETRANSFERKIT_QSSH_ROOT = $$clean_path($$FILETRANSFERKIT_ROOT/3rdparty/qsftp)
}

QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

INCLUDEPATH += \
    $$FILETRANSFERKIT_ROOT/src \
    $$FILETRANSFERKIT_QSSH_ROOT/include

unix:LIBS += -L$$FILETRANSFERKIT_ROOT/lib -lFileTransferKit
unix:LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSsh

win32:CONFIG(release, debug|release) {
    LIBS += -L$$FILETRANSFERKIT_ROOT/lib -lFileTransferKit
    LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSsh
} else:win32:CONFIG(debug, debug|release) {
    LIBS += -L$$FILETRANSFERKIT_ROOT/lib -lFileTransferKitd
    LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSshd
}
