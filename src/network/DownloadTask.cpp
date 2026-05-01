#include "DownloadTask.h"
#include "HttpClient.h"
#include "../core/Logger.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>
#include <QDir>

namespace x4 {
    DownloadTask::DownloadTask(const QUrl &url, const QString &destPath,
                               const QString &expectedSha1, qint64 expectedSize,
                               QObject *parent)
        : QObject(parent)
          , url_(url)
          , destPath_(destPath)
          , expectedSha1_(expectedSha1)
          , expectedSize_(expectedSize) {
    }

    void DownloadTask::start() {
        if (running_) return;

        if (!expectedSha1_.isEmpty() && QFileInfo::exists(destPath_)) {
            QFile existing(destPath_);
            if (existing.open(QIODevice::ReadOnly)) {
                QCryptographicHash check(QCryptographicHash::Sha1);
                check.addData(&existing);
                if (check.result().toHex() == expectedSha1_.toLatin1()) {
                    emit finished(true, {});
                    return;
                }
            }
        }

        QDir().mkpath(QFileInfo(destPath_).absolutePath());

        file_.setFileName(destPath_ + QStringLiteral(".part"));
        qint64 resumeOffset = 0;

        if (file_.exists()) {
            resumeOffset = file_.size();
            if (!file_.open(QIODevice::Append)) {
                emit finished(false, QStringLiteral("Cannot open file for writing: %1").arg(file_.errorString()));
                return;
            }
        } else {
            if (!file_.open(QIODevice::WriteOnly)) {
                emit finished(false, QStringLiteral("Cannot open file for writing: %1").arg(file_.errorString()));
                return;
            }
        }

        hash_.reset();
        bytesWritten_ = 0;
        running_ = true;

        QNetworkRequest req(url_);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("X4Launcher/0.1"));
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::AlwaysNetwork); // Save RAM by not caching downloads

        if (resumeOffset > 0) {
            req.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(resumeOffset).toLatin1());

            QFile existing(destPath_ + QStringLiteral(".part"));
            if (existing.open(QIODevice::ReadOnly)) {
                hash_.addData(&existing);
                bytesWritten_ = existing.size();
            }
        }

        reply_ = HttpClient::instance().manager()->get(req);
        connect(reply_, &QNetworkReply::readyRead, this, &DownloadTask::onReadyRead, Qt::QueuedConnection);
        connect(reply_, &QNetworkReply::finished, this, &DownloadTask::onFinished, Qt::QueuedConnection);

        connect(reply_, &QNetworkReply::downloadProgress, this,
                [this](qint64 recv, qint64 total) {
                    // Throttle progress to save CPU during rapid downloads
                    if (recv - lastProgressRecv_ > 1024 * 512 || recv == total) {
                        lastProgressRecv_ = recv;
                        emit progress(bytesWritten_ + recv, total > 0 ? bytesWritten_ + total : -1);
                    }
                }, Qt::QueuedConnection);
    }

    void DownloadTask::abort() {
        if (reply_) {
            reply_->abort();
        }
    }

    void DownloadTask::onReadyRead() {
        if (!reply_) return;

        const QByteArray data = reply_->readAll();
        file_.write(data);
        hash_.addData(data);
        bytesWritten_ += data.size();
    }

    void DownloadTask::onFinished() {
        running_ = false;
        file_.close();

        if (!reply_) {
            emit finished(false, QStringLiteral("No reply"));
            return;
        }

        const auto error = reply_->error();
        const QString errorStr = reply_->errorString();
        reply_->deleteLater();
        reply_ = nullptr;

        if (error != QNetworkReply::NoError) {
            QFile::remove(destPath_ + QStringLiteral(".part"));
            emit finished(false, QStringLiteral("Network error: %1").arg(errorStr));
            return;
        }

        if (!expectedSha1_.isEmpty()) {
            const QString actual = QString::fromLatin1(hash_.result().toHex());
            if (actual != expectedSha1_) {
                QFile::remove(destPath_ + QStringLiteral(".part"));
                emit finished(false, QStringLiteral("Hash mismatch: expected %1, got %2").arg(expectedSha1_, actual));
                return;
            }
        }

        QString partPath = destPath_ + QStringLiteral(".part");
        QFile::remove(destPath_);
        if (!QFile::rename(partPath, destPath_)) {
            if (QFileInfo::exists(destPath_)) {
                QFile::remove(partPath);
                if (!expectedSha1_.isEmpty()) {
                    QFile check(destPath_);
                    if (check.open(QIODevice::ReadOnly)) {
                        QCryptographicHash h(QCryptographicHash::Sha1);
                        h.addData(&check);
                        if (h.result().toHex() == expectedSha1_.toLatin1()) {
                            emit finished(true, {});
                            return;
                        }
                    }
                } else {
                    emit finished(true, {});
                    return;
                }
            }
            emit finished(false, QStringLiteral("Failed to move download to final path: %1 -> %2")
                          .arg(partPath, destPath_));
            return;
        }

        emit finished(true, {});
    }

    bool DownloadTask::verifyHash() {
        return expectedSha1_.isEmpty() ||
               QString::fromLatin1(hash_.result().toHex()) == expectedSha1_;
    }
}