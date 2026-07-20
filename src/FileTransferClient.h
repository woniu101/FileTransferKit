#ifndef FILETRANSFERCLIENT_H
#define FILETRANSFERCLIENT_H

#include <QObject>
#include <QList>
#include <QString>
#include <QByteArray>

class QFtp;

struct FileTransferEntry
{
    FileTransferEntry()
        : isDirectory(false),
          size(-1)
    {
    }

    QString name;
    QString path;
    bool isDirectory;
    qint64 size;
};

class FileTransferClient : public QObject
{
    Q_OBJECT
public:
    enum Protocol { Ftp, Sftp };
    enum FileNameEncoding { Utf8FileNameEncoding, GbkFileNameEncoding };

    explicit FileTransferClient(QObject *parent = 0);
    ~FileTransferClient();

    void setProtocol(Protocol protocol);
    Protocol protocol() const;

    void setHost(const QString &host);
    void setPort(quint16 port);
    void setUserName(const QString &userName);
    void setPassword(const QString &password);
    void setTimeoutMs(int timeoutMs);
    void setFileNameEncoding(FileNameEncoding encoding);
    void setFileNameCodecName(const QByteArray &codecName);

    void setConnectionInfo(Protocol protocol, const QString &host, quint16 port,
        const QString &userName, const QString &password);

    bool uploadFile(const QString &localPath, const QString &remotePath);
    bool downloadFile(const QString &remotePath, const QString &localPath);
    bool removeFile(const QString &remotePath);
    QList<FileTransferEntry> listDirectory(const QString &remotePath);
    bool createDirectory(const QString &remotePath);
    bool removeDirectory(const QString &remotePath);
    bool rename(const QString &oldRemotePath, const QString &newRemotePath);
    bool fileExists(const QString &remotePath);
    void close();

    QString errorString() const;

signals:
    void progress(qint64 done, qint64 total);
    void connected();
    void operationFinished(bool ok, const QString &error);

private:
    bool ftpUploadFile(const QString &localPath, const QString &remotePath);
    bool ftpDownloadFile(const QString &remotePath, const QString &localPath);
    bool ftpRemoveFile(const QString &remotePath);
    bool ftpListDirectory(const QString &remotePath, QList<FileTransferEntry> *entries);
    bool ftpCreateDirectory(const QString &remotePath);
    bool ftpRemoveDirectory(const QString &remotePath);
    bool ftpRename(const QString &oldRemotePath, const QString &newRemotePath);
    bool ftpFileExists(const QString &remotePath);
    bool ensureFtpConnected();
    bool waitForFtpCommand(int commandId, const QString &defaultError);
    void closeFtpAfterOperation();
    void destroyFtp(bool abortTransfer);

    bool sftpUploadFile(const QString &localPath, const QString &remotePath);
    bool sftpDownloadFile(const QString &remotePath, const QString &localPath);
    bool sftpRemoveFile(const QString &remotePath);
    bool sftpListDirectory(const QString &remotePath, QList<FileTransferEntry> *entries);
    bool sftpCreateDirectory(const QString &remotePath);
    bool sftpRemoveDirectory(const QString &remotePath);
    bool sftpRename(const QString &oldRemotePath, const QString &newRemotePath);
    bool sftpFileExists(const QString &remotePath);
    bool runSftpFileJob(const QString &localPath, const QString &remotePath, int operation);
    bool runSftpOperation(const QString &firstPath, const QString &secondPath, int operation,
        QList<FileTransferEntry> *entries);

    QString encodeFtpString(const QString &input) const;
    QString decodeFtpString(const QString &input) const;
    void setError(const QString &error);
    void finishOperation(bool ok);
    quint16 effectivePort() const;

    Protocol m_protocol;
    QString m_host;
    quint16 m_port;
    QString m_userName;
    QString m_password;
    int m_timeoutMs;
    QByteArray m_fileNameCodecName;
    QString m_errorString;
    QFtp *m_ftp;
};

#endif // FILETRANSFERCLIENT_H
