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
    QTextStream(stderr) << "       " << QFileInfo(program).fileName()
        << " manage <ftp|sftp> <host> <user> <password> <utf8|gbk> <local-file> <remote-directory>"
        << " [port]" << endl;
    QTextStream(stderr) << "       " << QFileInfo(program).fileName()
        << " cleanup <ftp|sftp> <host> <user> <password> <utf8|gbk> <remote-directory> [port]" << endl;
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

static bool parsePortAt(const QStringList &args, int index, quint16 *port)
{
    *port = 0;
    if (args.count() <= index)
        return true;
    bool ok = false;
    const int value = args.at(index).toInt(&ok);
    if (!ok || value <= 0 || value > 65535)
        return false;
    *port = quint16(value);
    return true;
}

static bool parseProtocol(const QString &argument, FileTransferClient::Protocol *protocol)
{
    const QString value = argument.toLower();
    if (value == QLatin1String("ftp")) {
        *protocol = FileTransferClient::Ftp;
        return true;
    }
    if (value == QLatin1String("sftp")) {
        *protocol = FileTransferClient::Sftp;
        return true;
    }
    return false;
}

static bool parseEncoding(const QString &argument, FileTransferClient::FileNameEncoding *encoding)
{
    const QString value = argument.toLower();
    if (value == QLatin1String("gbk")) {
        *encoding = FileTransferClient::GbkFileNameEncoding;
        return true;
    }
    if (value == QLatin1String("utf8") || value == QLatin1String("utf-8")) {
        *encoding = FileTransferClient::Utf8FileNameEncoding;
        return true;
    }
    return false;
}

static QString remotePathJoin(const QString &directory, const QString &name)
{
    if (directory.isEmpty())
        return name;
    if (directory == QLatin1String("/"))
        return directory + name;
    return directory.endsWith(QLatin1Char('/'))
        ? directory + name : directory + QLatin1Char('/') + name;
}

static int fail(const QString &step, const FileTransferClient &client)
{
    QTextStream(stderr) << "FAILED " << step << ": " << client.errorString() << endl;
    return 1;
}

static int runManagementTest(const QStringList &args)
{
    if (args.count() < 9) {
        printUsage(args.first());
        return 1;
    }

    FileTransferClient::Protocol protocol;
    FileTransferClient::FileNameEncoding encoding;
    quint16 port = 0;
    if (!parseProtocol(args.at(2), &protocol) || !parseEncoding(args.at(6), &encoding)
        || !parsePortAt(args, 9, &port)) {
        printUsage(args.first());
        return 1;
    }

    const QString localPath = args.at(7);
    const QString remoteDirectory = args.at(8);
    const QString originalName = QString::fromUtf8(
        "\xE6\xB5\x8B\xE8\xAF\x95\xE6\x96\x87\xE4\xBB\xB6.txt");
    const QString renamedName = QString::fromUtf8(
        "\xE6\xB5\x8B\xE8\xAF\x95\xE6\x96\x87\xE4\xBB\xB6_"
        "\xE5\xB7\xB2\xE9\x87\x8D\xE5\x91\xBD\xE5\x90\x8D.txt");
    const QString originalPath = remotePathJoin(remoteDirectory, originalName);
    const QString renamedPath = remotePathJoin(remoteDirectory, renamedName);

    FileTransferClient client;
    client.setConnectionInfo(protocol, args.at(3), port, args.at(4), args.at(5));
    client.setFileNameEncoding(encoding);

    QTextStream(stdout) << "Creating directory..." << endl;
    if (!client.createDirectory(remoteDirectory))
        return fail(QString::fromLatin1("create directory"), client);

    QTextStream(stdout) << "Listing empty directory..." << endl;
    const QList<FileTransferEntry> emptyEntries = client.listDirectory(remoteDirectory);
    if (!client.errorString().isEmpty())
        return fail(QString::fromLatin1("list empty directory"), client);
    if (!emptyEntries.isEmpty()) {
        QTextStream(stderr) << "FAILED list empty directory: directory is not empty." << endl;
        return 1;
    }

    QTextStream(stdout) << "Uploading test file..." << endl;
    if (!client.uploadFile(localPath, originalPath))
        return fail(QString::fromLatin1("upload test file"), client);

    QTextStream(stdout) << "Listing directory..." << endl;
    const QList<FileTransferEntry> entries = client.listDirectory(remoteDirectory);
    if (!client.errorString().isEmpty())
        return fail(QString::fromLatin1("list directory"), client);
    bool foundOriginal = false;
    foreach (const FileTransferEntry &entry, entries) {
        QTextStream(stdout) << "Entry: " << entry.name << " size=" << entry.size
            << " directory=" << entry.isDirectory << endl;
        if (entry.name == originalName)
            foundOriginal = true;
    }
    if (!foundOriginal) {
        QTextStream(stderr) << "FAILED list directory: uploaded file was not returned." << endl;
        return 1;
    }

    QTextStream(stdout) << "Checking file exists..." << endl;
    if (!client.fileExists(originalPath))
        return fail(QString::fromLatin1("file exists"), client);

    QTextStream(stdout) << "Renaming file..." << endl;
    if (!client.rename(originalPath, renamedPath))
        return fail(QString::fromLatin1("rename file"), client);

    QTextStream(stdout) << "Checking renamed file exists..." << endl;
    if (!client.fileExists(renamedPath))
        return fail(QString::fromLatin1("renamed file exists"), client);

    QTextStream(stdout) << "Removing file..." << endl;
    if (!client.removeFile(renamedPath))
        return fail(QString::fromLatin1("remove file"), client);

    QTextStream(stdout) << "Removing directory..." << endl;
    if (!client.removeDirectory(remoteDirectory))
        return fail(QString::fromLatin1("remove directory"), client);

    QTextStream(stdout) << "OK: management operations completed." << endl;
    return 0;
}

static int runCleanup(const QStringList &args)
{
    if (args.count() < 8) {
        printUsage(args.first());
        return 1;
    }

    FileTransferClient::Protocol protocol;
    FileTransferClient::FileNameEncoding encoding;
    quint16 port = 0;
    if (!parseProtocol(args.at(2), &protocol) || !parseEncoding(args.at(6), &encoding)
        || !parsePortAt(args, 8, &port)) {
        printUsage(args.first());
        return 1;
    }

    const QString remoteDirectory = args.at(7);
    FileTransferClient client;
    client.setConnectionInfo(protocol, args.at(3), port, args.at(4), args.at(5));
    client.setFileNameEncoding(encoding);

    const QList<FileTransferEntry> entries = client.listDirectory(remoteDirectory);
    if (!client.errorString().isEmpty()) {
        const QString listError = client.errorString();
        if (client.removeDirectory(remoteDirectory)) {
            QTextStream(stdout) << "OK: removed empty " << remoteDirectory << endl;
            return 0;
        }
        QTextStream(stderr) << "FAILED list cleanup directory: " << listError << endl;
        return 1;
    }
    foreach (const FileTransferEntry &entry, entries) {
        if (entry.isDirectory) {
            QTextStream(stderr) << "FAILED cleanup: nested directory is not supported: "
                << entry.path << endl;
            return 1;
        }
        if (!client.removeFile(entry.path))
            return fail(QString::fromLatin1("remove cleanup file"), client);
    }
    if (!client.removeDirectory(remoteDirectory))
        return fail(QString::fromLatin1("remove cleanup directory"), client);

    QTextStream(stdout) << "OK: removed " << remoteDirectory << endl;
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.count() > 1) {
        const QString mode = args.at(1).toLower();
        if (mode == QLatin1String("manage"))
            return runManagementTest(args);
        if (mode == QLatin1String("cleanup"))
            return runCleanup(args);
    }
    if (args.count() < 8) {
        printUsage(args.first());
        return 1;
    }

    FileTransferClient::Protocol protocol;
    if (!parseProtocol(args.at(1), &protocol)) {
        printUsage(args.first());
        return 1;
    }

    FileTransferClient::FileNameEncoding encoding;
    if (!parseEncoding(args.at(5), &encoding)) {
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
