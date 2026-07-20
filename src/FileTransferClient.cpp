#include "FileTransferClient.h"

#include "qftp.h"
#include "qurlinfo.h"
#include "ssh/sftpchannel.h"
#include "ssh/sshconnection.h"

#include <QEventLoop>
#include <QFile>
#include <QPointer>
#include <QTextCodec>
#include <QTimer>

namespace {
const int DefaultTimeoutMs = 300000;
const quint16 DefaultFtpPort = 21;
const quint16 DefaultSftpPort = 22;

enum SftpOperation {
    SftpUploadOperation,
    SftpDownloadOperation,
    SftpRemoveOperation,
    SftpListDirectoryOperation,
    SftpCreateDirectoryOperation,
    SftpRemoveDirectoryOperation,
    SftpRenameOperation
};

QString remotePathJoin(const QString &directory, const QString &name)
{
    if (directory.isEmpty())
        return name;
    if (directory == QLatin1String("/"))
        return directory + name;
    return directory.endsWith(QLatin1Char('/'))
        ? directory + name : directory + QLatin1Char('/') + name;
}

QString remoteParentPath(const QString &path)
{
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    if (slash < 0)
        return QString();
    if (slash == 0)
        return QLatin1String("/");
    return path.left(slash);
}

QString remoteBaseName(const QString &path)
{
    QString normalized = path;
    while (normalized.length() > 1 && normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    return slash < 0 ? normalized : normalized.mid(slash + 1);
}
}

FileTransferClient::FileTransferClient(QObject *parent)
    : QObject(parent),
      m_protocol(Sftp),
      m_port(0),
      m_timeoutMs(DefaultTimeoutMs),
      m_fileNameCodecName("UTF-8"),
      m_ftp(0)
{
}

FileTransferClient::~FileTransferClient()
{
    close();
}

void FileTransferClient::setProtocol(Protocol protocol)
{
    if (m_protocol == protocol)
        return;
    close();
    m_protocol = protocol;
}

FileTransferClient::Protocol FileTransferClient::protocol() const
{
    return m_protocol;
}

void FileTransferClient::setHost(const QString &host)
{
    m_host = host;
}

void FileTransferClient::setPort(quint16 port)
{
    m_port = port;
}

void FileTransferClient::setUserName(const QString &userName)
{
    m_userName = userName;
}

void FileTransferClient::setPassword(const QString &password)
{
    m_password = password;
}

void FileTransferClient::setTimeoutMs(int timeoutMs)
{
    m_timeoutMs = timeoutMs > 0 ? timeoutMs : DefaultTimeoutMs;
}

void FileTransferClient::setFileNameEncoding(FileNameEncoding encoding)
{
    setFileNameCodecName(encoding == GbkFileNameEncoding ? QByteArray("GBK") : QByteArray("UTF-8"));
}

void FileTransferClient::setFileNameCodecName(const QByteArray &codecName)
{
    m_fileNameCodecName = QTextCodec::codecForName(codecName)
        ? codecName : QByteArray("UTF-8");
}

void FileTransferClient::setConnectionInfo(Protocol protocol, const QString &host, quint16 port,
    const QString &userName, const QString &password)
{
    setProtocol(protocol);
    m_host = host;
    m_port = port;
    m_userName = userName;
    m_password = password;
}

bool FileTransferClient::uploadFile(const QString &localPath, const QString &remotePath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp
        ? ftpUploadFile(localPath, remotePath) : sftpUploadFile(localPath, remotePath);
    finishOperation(ok);
    return ok;
}

bool FileTransferClient::downloadFile(const QString &remotePath, const QString &localPath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp
        ? ftpDownloadFile(remotePath, localPath) : sftpDownloadFile(remotePath, localPath);
    finishOperation(ok);
    return ok;
}

bool FileTransferClient::removeFile(const QString &remotePath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp ? ftpRemoveFile(remotePath) : sftpRemoveFile(remotePath);
    finishOperation(ok);
    return ok;
}

QList<FileTransferEntry> FileTransferClient::listDirectory(const QString &remotePath)
{
    m_errorString.clear();
    QList<FileTransferEntry> entries;
    const bool ok = m_protocol == Ftp
        ? ftpListDirectory(remotePath, &entries) : sftpListDirectory(remotePath, &entries);
    finishOperation(ok);
    return ok ? entries : QList<FileTransferEntry>();
}

bool FileTransferClient::createDirectory(const QString &remotePath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp ? ftpCreateDirectory(remotePath) : sftpCreateDirectory(remotePath);
    finishOperation(ok);
    return ok;
}

bool FileTransferClient::removeDirectory(const QString &remotePath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp ? ftpRemoveDirectory(remotePath) : sftpRemoveDirectory(remotePath);
    finishOperation(ok);
    return ok;
}

bool FileTransferClient::rename(const QString &oldRemotePath, const QString &newRemotePath)
{
    m_errorString.clear();
    const bool ok = m_protocol == Ftp
        ? ftpRename(oldRemotePath, newRemotePath) : sftpRename(oldRemotePath, newRemotePath);
    finishOperation(ok);
    return ok;
}

bool FileTransferClient::fileExists(const QString &remotePath)
{
    m_errorString.clear();
    const bool exists = m_protocol == Ftp ? ftpFileExists(remotePath) : sftpFileExists(remotePath);
    finishOperation(exists);
    return exists;
}

void FileTransferClient::close()
{
    destroyFtp(true);
}

QString FileTransferClient::errorString() const
{
    return m_errorString;
}

bool FileTransferClient::ftpUploadFile(const QString &localPath, const QString &remotePath)
{
    QFile localFile(localPath);
    if (!localFile.exists())
        return setError(QString::fromLatin1("Local file does not exist: %1").arg(localPath)), false;
    if (!localFile.open(QIODevice::ReadOnly))
        return setError(QString::fromLatin1("Cannot open local file: %1").arg(localFile.errorString())), false;
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->put(&localFile, encodeFtpString(remotePath), QFtp::Binary);
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP upload failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpDownloadFile(const QString &remotePath, const QString &localPath)
{
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return setError(QString::fromLatin1("Cannot open local file: %1").arg(localFile.errorString())), false;
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->get(encodeFtpString(remotePath), &localFile, QFtp::Binary);
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP download failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpRemoveFile(const QString &remotePath)
{
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->remove(encodeFtpString(remotePath));
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP remove failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpListDirectory(const QString &remotePath, QList<FileTransferEntry> *entries)
{
    if (!ensureFtpConnected()) {
        close();
        return false;
    }

    QMetaObject::Connection listConnection = connect(m_ftp, &QFtp::listInfo,
        this, [&](const QUrlInfo &info) {
            if (info.name() == QLatin1String(".") || info.name() == QLatin1String(".."))
                return;
            FileTransferEntry entry;
            entry.name = decodeFtpString(info.name());
            entry.path = remotePathJoin(remotePath, entry.name);
            entry.isDirectory = info.isDir();
            entry.size = info.size();
            entries->append(entry);
        });
    const int commandId = m_ftp->list(encodeFtpString(remotePath));
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP list directory failed."));
    disconnect(listConnection);
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpCreateDirectory(const QString &remotePath)
{
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->mkdir(encodeFtpString(remotePath));
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP create directory failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpRemoveDirectory(const QString &remotePath)
{
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->rmdir(encodeFtpString(remotePath));
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP remove directory failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpRename(const QString &oldRemotePath, const QString &newRemotePath)
{
    if (!ensureFtpConnected()) {
        close();
        return false;
    }
    const int commandId = m_ftp->rename(encodeFtpString(oldRemotePath), encodeFtpString(newRemotePath));
    const bool ok = waitForFtpCommand(commandId, QString::fromLatin1("FTP rename failed."));
    closeFtpAfterOperation();
    return ok;
}

bool FileTransferClient::ftpFileExists(const QString &remotePath)
{
    const QString name = remoteBaseName(remotePath);
    if (name.isEmpty()) {
        setError(QString::fromLatin1("Remote path is empty."));
        return false;
    }

    QList<FileTransferEntry> entries;
    if (!ftpListDirectory(remoteParentPath(remotePath), &entries))
        return false;
    foreach (const FileTransferEntry &entry, entries) {
        if (entry.name == name)
            return true;
    }
    setError(QString::fromLatin1("Remote path does not exist: %1").arg(remotePath));
    return false;
}

bool FileTransferClient::ensureFtpConnected()
{
    if (!m_ftp) {
        m_ftp = new QFtp(this);
        connect(m_ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SIGNAL(progress(qint64,qint64)));
    }
    if (m_ftp->state() == QFtp::LoggedIn)
        return true;

    m_ftp->abortRightNow();
    m_ftp->connectToHost(m_host, effectivePort());
    const int loginId = m_ftp->login(encodeFtpString(m_userName), encodeFtpString(m_password));
    const bool ok = waitForFtpCommand(loginId, QString::fromLatin1("FTP login failed."));
    if (ok)
        emit connected();
    return ok;
}

bool FileTransferClient::waitForFtpCommand(int commandId, const QString &defaultError)
{
    bool finished = false;
    bool hasError = false;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QMetaObject::Connection finishedConnection = connect(m_ftp, &QFtp::commandFinished,
        this, [&](int id, bool error) {
            if (id != commandId)
                return;
            finished = true;
            hasError = error;
            loop.quit();
        });
    QMetaObject::Connection timeoutConnection = connect(&timer, &QTimer::timeout,
        &loop, [&]() {
            finished = false;
            loop.quit();
        });

    timer.start(m_timeoutMs);
    loop.exec();
    timer.stop();
    disconnect(finishedConnection);
    disconnect(timeoutConnection);

    if (!finished) {
        if (m_ftp)
            m_ftp->abortRightNow();
        setError(QString::fromLatin1("FTP operation timed out."));
        return false;
    }
    if (hasError) {
        const QString ftpError = m_ftp ? m_ftp->errorString() : QString();
        setError(ftpError.isEmpty() ? defaultError : ftpError);
        return false;
    }
    return true;
}

void FileTransferClient::closeFtpAfterOperation()
{
    if (!m_ftp)
        return;

    // Keep an authenticated FTP control connection for consecutive operations.
    // Reconnecting immediately after a QUIT can make the legacy QFtp data socket
    // fail intermittently against FileZilla Server.
    if (m_ftp->state() == QFtp::Unconnected)
        destroyFtp(false);
}

void FileTransferClient::destroyFtp(bool abortTransfer)
{
    if (!m_ftp)
        return;
    if (abortTransfer)
        m_ftp->abortRightNow();
    delete m_ftp;
    m_ftp = 0;
}

bool FileTransferClient::sftpUploadFile(const QString &localPath, const QString &remotePath)
{
    if (!QFile::exists(localPath))
        return setError(QString::fromLatin1("Local file does not exist: %1").arg(localPath)), false;
    return runSftpFileJob(localPath, remotePath, SftpUploadOperation);
}

bool FileTransferClient::sftpDownloadFile(const QString &remotePath, const QString &localPath)
{
    return runSftpFileJob(localPath, remotePath, SftpDownloadOperation);
}

bool FileTransferClient::sftpRemoveFile(const QString &remotePath)
{
    return runSftpFileJob(QString(), remotePath, SftpRemoveOperation);
}

bool FileTransferClient::sftpListDirectory(const QString &remotePath, QList<FileTransferEntry> *entries)
{
    return runSftpOperation(QString(), remotePath, SftpListDirectoryOperation, entries);
}

bool FileTransferClient::sftpCreateDirectory(const QString &remotePath)
{
    return runSftpOperation(QString(), remotePath, SftpCreateDirectoryOperation, 0);
}

bool FileTransferClient::sftpRemoveDirectory(const QString &remotePath)
{
    return runSftpOperation(QString(), remotePath, SftpRemoveDirectoryOperation, 0);
}

bool FileTransferClient::sftpRename(const QString &oldRemotePath, const QString &newRemotePath)
{
    return runSftpOperation(oldRemotePath, newRemotePath, SftpRenameOperation, 0);
}

bool FileTransferClient::sftpFileExists(const QString &remotePath)
{
    const QString name = remoteBaseName(remotePath);
    if (name.isEmpty()) {
        setError(QString::fromLatin1("Remote path is empty."));
        return false;
    }

    QList<FileTransferEntry> entries;
    if (!sftpListDirectory(remoteParentPath(remotePath), &entries))
        return false;
    foreach (const FileTransferEntry &entry, entries) {
        if (entry.name == name)
            return true;
    }
    setError(QString::fromLatin1("Remote path does not exist: %1").arg(remotePath));
    return false;
}

bool FileTransferClient::runSftpFileJob(const QString &localPath, const QString &remotePath,
    int operation)
{
    return runSftpOperation(localPath, remotePath, operation, 0);
}

bool FileTransferClient::runSftpOperation(const QString &firstPath, const QString &secondPath,
    int operation, QList<FileTransferEntry> *entries)
{
    QSsh::SshConnectionParameters params;
    params.host = m_host;
    params.port = effectivePort();
    params.userName = m_userName;
    params.password = m_password;
    params.timeout = m_timeoutMs / 1000;
    if (params.timeout <= 0)
        params.timeout = 1;
    params.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool done = false;
    bool ok = false;
    bool channelInitialized = false;
    QSsh::SftpJobId jobId = QSsh::SftpInvalidJob;

    QPointer<QSsh::SshConnection> connection(new QSsh::SshConnection(params));
    QSharedPointer<QSsh::SftpChannel> channel;

    connect(&timer, &QTimer::timeout, &loop, [&]() {
        setError(QString::fromLatin1("SFTP operation timed out."));
        done = true;
        loop.quit();
    });

    connect(connection.data(), &QSsh::SshConnection::connected, this, [&]() {
        channel = connection->createSftpChannel();
        if (!channel) {
            setError(QString::fromLatin1("Cannot create SFTP channel."));
            done = true;
            loop.quit();
            return;
        }
        channel->setFileNameCodecName(m_fileNameCodecName);
        connect(channel.data(), &QSsh::SftpChannel::initialized, this, [&]() {
            channelInitialized = true;
            emit connected();
            switch (operation) {
            case SftpUploadOperation:
                jobId = channel->uploadFile(firstPath, secondPath, QSsh::SftpOverwriteExisting);
                break;
            case SftpDownloadOperation:
                jobId = channel->downloadFile(secondPath, firstPath, QSsh::SftpOverwriteExisting);
                break;
            case SftpRemoveOperation:
                jobId = channel->removeFile(secondPath);
                break;
            case SftpListDirectoryOperation:
                jobId = channel->listDirectory(secondPath);
                break;
            case SftpCreateDirectoryOperation:
                jobId = channel->createDirectory(secondPath);
                break;
            case SftpRemoveDirectoryOperation:
                jobId = channel->removeDirectory(secondPath);
                break;
            case SftpRenameOperation:
                jobId = channel->renameFileOrDirectory(firstPath, secondPath);
                break;
            }
            if (jobId == QSsh::SftpInvalidJob) {
                setError(QString::fromLatin1("Cannot start SFTP file operation."));
                done = true;
                loop.quit();
            }
        });
        connect(channel.data(), &QSsh::SftpChannel::channelError, this, [&](const QString &error) {
            setError(error);
            done = true;
            loop.quit();
        });
        connect(channel.data(), &QSsh::SftpChannel::fileInfoAvailable, this,
            [&](QSsh::SftpJobId infoJob, const QList<QSsh::SftpFileInfo> &fileInfoList) {
                if (!entries || infoJob != jobId)
                    return;
                foreach (const QSsh::SftpFileInfo &fileInfo, fileInfoList) {
                    if (fileInfo.name == QLatin1String(".") || fileInfo.name == QLatin1String(".."))
                        continue;
                    FileTransferEntry entry;
                    entry.name = fileInfo.name;
                    entry.path = remotePathJoin(secondPath, entry.name);
                    entry.isDirectory = fileInfo.type == QSsh::FileTypeDirectory;
                    entry.size = fileInfo.sizeValid ? qint64(fileInfo.size) : -1;
                    entries->append(entry);
                }
            });
        connect(channel.data(), &QSsh::SftpChannel::finished, this,
            [&](QSsh::SftpJobId finishedJob, const QString &error) {
                if (finishedJob != jobId)
                    return;
                ok = error.isEmpty();
                if (!ok)
                    setError(error);
                done = true;
                loop.quit();
            });
        channel->initialize();
    });
    connect(connection.data(), &QSsh::SshConnection::error, this, [&](QSsh::SshError) {
        setError(connection ? connection->errorString() : QString::fromLatin1("SFTP connection error."));
        done = true;
        loop.quit();
    });

    timer.start(m_timeoutMs);
    connection->connectToHost();
    loop.exec();
    timer.stop();

    if (channelInitialized && channel)
        channel->closeChannel();
    if (connection)
        connection->disconnectFromHost();
    delete connection.data();

    if (!done && m_errorString.isEmpty())
        setError(QString::fromLatin1("SFTP operation did not finish."));
    return ok;
}

QString FileTransferClient::encodeFtpString(const QString &input) const
{
    QTextCodec *codec = QTextCodec::codecForName(m_fileNameCodecName);
    if (!codec)
        codec = QTextCodec::codecForName("UTF-8");
    return QString::fromLatin1(codec->fromUnicode(input));
}

QString FileTransferClient::decodeFtpString(const QString &input) const
{
    const QByteArray bytes = input.toLatin1();
    QTextCodec *utf8Codec = QTextCodec::codecForName("UTF-8");
    if (utf8Codec && QString::fromLatin1(m_fileNameCodecName)
        .compare(QLatin1String("UTF-8"), Qt::CaseInsensitive) != 0) {
        QTextCodec::ConverterState state;
        const QString utf8Value = utf8Codec->toUnicode(bytes.constData(), bytes.size(), &state);
        if (state.invalidChars == 0)
            return utf8Value;
    }

    QTextCodec *codec = QTextCodec::codecForName(m_fileNameCodecName);
    if (!codec)
        codec = utf8Codec;
    return codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
}

void FileTransferClient::setError(const QString &error)
{
    m_errorString = error.isEmpty() ? QString::fromLatin1("Unknown file transfer error.") : error;
}

void FileTransferClient::finishOperation(bool ok)
{
    emit operationFinished(ok, ok ? QString() : m_errorString);
}

quint16 FileTransferClient::effectivePort() const
{
    if (m_port != 0)
        return m_port;
    return m_protocol == Ftp ? DefaultFtpPort : DefaultSftpPort;
}
