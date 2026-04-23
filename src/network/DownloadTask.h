#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QFile>
#include <QCryptographicHash>

class QNetworkReply;

namespace x4 {
class DownloadTask : public QObject {
    Q_OBJECT
public:
    DownloadTask(const QUrl& url, const QString& destPath,
                 const QString& expectedSha1 = {}, qint64 expectedSize = 0,
                 QObject* parent = nullptr);

    void start();
    void abort();

    const QUrl& url() const { return url_; }
    const QString& destPath() const { return destPath_; }
    bool isRunning() const { return running_; }

signals:
    void progress(qint64 bytesReceived, qint64 bytesTotal);
    void finished(bool success, const QString& errorMsg);

private slots:
    void onReadyRead();
    void onFinished();

private:
    bool verifyHash();

    QUrl url_;
    QString destPath_;
    QString expectedSha1_;
    qint64 expectedSize_ = 0;

    QFile file_;
    QNetworkReply* reply_ = nullptr;
    QCryptographicHash hash_{QCryptographicHash::Sha1};
    qint64 bytesWritten_ = 0;
    qint64 lastProgressRecv_ = 0;
    bool running_ = false;
};

}
