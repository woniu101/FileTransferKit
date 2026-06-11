isEmpty(FILETRANSFERKIT_ROOT) {
    error("Please set FILETRANSFERKIT_ROOT before including FileTransferKitSources.pri")
}

isEmpty(FILETRANSFERKIT_QSSH_ROOT) {
    FILETRANSFERKIT_QSSH_ROOT = $$clean_path($$FILETRANSFERKIT_ROOT/3rdparty/qsftp)
}

QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

INCLUDEPATH += \
    $$FILETRANSFERKIT_ROOT/src \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp \
    $$FILETRANSFERKIT_QSSH_ROOT/include

SOURCES += \
    $$FILETRANSFERKIT_ROOT/src/FileTransferClient.cpp \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp/qftp.cpp \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp/qurlinfo.cpp

HEADERS += \
    $$FILETRANSFERKIT_ROOT/src/FileTransferClient.h \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp/qftp.h \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp/qurlinfo.h \
    $$FILETRANSFERKIT_ROOT/3rdparty/qftp/keepalive.h

unix:LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSsh

win32:CONFIG(release, debug|release): LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSsh
else:win32:CONFIG(debug, debug|release): LIBS += -L$$FILETRANSFERKIT_QSSH_ROOT/lib -lQSshd
