#include "FileTransferClient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

static void printUsage(const QString &program)
{
    QTextStream(stderr) << "Usage: " << QFileInfo(program).fileName()
        << " <ftp|sftp> <host> <user> <password> <utf8|gbk> <local-file> <remote-file>"
        << " [port] [--remove]" << endl;
}

static bool parsePort(const QStringList &args, quint16 *port)
{
    *port = 0;
    if (args.count() <= 8 || args.at(8) == QLatin1String("--remove"))
        return true;
    bool ok = false;
    const int value = args.at(8).toInt(&ok);
    if (!ok || value <= 0 || value > 65535)
        return false;
    *port = quint16(value);
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.count() < 8) {
        printUsage(args.first());
        return 1;
    }

    const QString protocolArg = args.at(1).toLower();
    FileTransferClient::Protocol protocol;
    if (protocolArg == QLatin1String("ftp"))
        protocol = FileTransferClient::Ftp;
    else if (protocolArg == QLatin1String("sftp"))
        protocol = FileTransferClient::Sftp;
    else {
        printUsage(args.first());
        return 1;
    }

    const QString encodingArg = args.at(5).toLower();
    FileTransferClient::FileNameEncoding encoding;
    if (encodingArg == QLatin1String("gbk"))
        encoding = FileTransferClient::GbkFileNameEncoding;
    else if (encodingArg == QLatin1String("utf8") || encodingArg == QLatin1String("utf-8"))
        encoding = FileTransferClient::Utf8FileNameEncoding;
    else {
        printUsage(args.first());
        return 1;
    }

    quint16 port = 0;
    if (!parsePort(args, &port)) {
        printUsage(args.first());
        return 1;
    }

    const QString localPath = args.at(6);
    const QString remotePath = args.at(7);
    const QString downloadPath = QDir::temp().absoluteFilePath(
        QFileInfo(localPath).fileName() + QLatin1String(".filetransferclient.download"));
    const bool removeRemoteFile = args.contains(QLatin1String("--remove"));

    FileTransferClient client;
    client.setConnectionInfo(protocol, args.at(2), port, args.at(3), args.at(4));
    client.setFileNameEncoding(encoding);

    QObject::connect(&client, &FileTransferClient::progress, [](qint64 done, qint64 total) {
        QTextStream(stdout) << "Progress: " << done << "/" << total << endl;
    });

    QTextStream(stdout) << "Uploading..." << endl;
    if (!client.uploadFile(localPath, remotePath)) {
        QTextStream(stderr) << "FAILED upload: " << client.errorString() << endl;
        return 1;
    }

    QTextStream(stdout) << "Downloading..." << endl;
    if (!client.downloadFile(remotePath, downloadPath)) {
        QTextStream(stderr) << "FAILED download: " << client.errorString() << endl;
        return 1;
    }

    QFile source(localPath);
    QFile downloaded(downloadPath);
    if (!source.open(QIODevice::ReadOnly) || !downloaded.open(QIODevice::ReadOnly)) {
        QTextStream(stderr) << "FAILED compare: cannot open local comparison file." << endl;
        return 1;
    }
    if (source.readAll() != downloaded.readAll()) {
        QTextStream(stderr) << "FAILED compare: downloaded content differs." << endl;
        return 1;
    }
    source.close();
    downloaded.close();
    QFile::remove(downloadPath);

    if (removeRemoteFile) {
        QTextStream(stdout) << "Removing..." << endl;
        if (!client.removeFile(remotePath)) {
            QTextStream(stderr) << "FAILED remove: " << client.errorString() << endl;
            return 1;
        }
    } else {
        QTextStream(stdout) << "Remote file kept: " << remotePath << endl;
    }

    QTextStream(stdout) << "OK: " << remotePath << endl;
    return 0;
}
