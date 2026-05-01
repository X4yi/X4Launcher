#include "DownloadManager.h"
#include "DownloadTask.h"
#include "../core/Logger.h"
#include "../core/SettingsManager.h"

namespace x4 {
    DownloadManager::DownloadManager(QObject *parent)
        : QObject(parent) {
        maxParallel_ = SettingsManager::instance().downloadParallelism();

        connect(&SettingsManager::instance(), &SettingsManager::settingChanged, this,
                [this](const QString &key) {
                    if (key == QStringLiteral("network/parallelism")) {
                        maxParallel_ = SettingsManager::instance().downloadParallelism();
                    }
                });
    }

    void DownloadManager::enqueue(const QUrl &url, const QString &destPath,
                                  const QString &sha1, qint64 size) {
        QMutexLocker locker(&mutex_);
        bool duplicate = false;
        for (const auto &e: queue_)
            if (e.destPath == destPath) {
                duplicate = true;
                break;
            }
        if (!duplicate) {
            for (const auto *task: active_)
                if (task->destPath() == destPath) {
                    duplicate = true;
                    break;
                }
        }
        if (!duplicate) {
            queue_.enqueue({url, destPath, sha1, size});
        }
    }

    void DownloadManager::enqueue(const QVector<Entry> &entries) {
        QMutexLocker locker(&mutex_);
        for (const auto &e: entries)
            queue_.enqueue(e);
    }

    void DownloadManager::startAll() {
        {
            QMutexLocker locker(&mutex_);
            totalCount_ = queue_.size();
            completedCount_ = 0;
            errors_.clear();
            if (queue_.isEmpty()) {
                emit allFinished(true, {});
                return;
            }
        }
        while (true) {
            {
                QMutexLocker locker(&mutex_);
                if (queue_.isEmpty() || active_.size() >= maxParallel_) break;
            }
            startNext();
        }
    }

    void DownloadManager::abortAll() {
        QVector<DownloadTask *> copy;
        {
            QMutexLocker locker(&mutex_);
            copy = active_;
        }
        for (auto *task: copy) {
            if (task) task->abort();
        }
    }

    void DownloadManager::startNext() {
        Entry entry;
        {
            QMutexLocker locker(&mutex_);
            if (queue_.isEmpty()) return;
            entry = queue_.dequeue();
        }

        auto *task = new DownloadTask(entry.url, entry.destPath, entry.sha1, entry.size, this);
        {
            QMutexLocker locker(&mutex_);
            active_.append(task);
        }

        connect(task, &DownloadTask::progress, this,
                [this, path = entry.destPath](qint64 recv, qint64 total) {
                    emit taskProgress(path, recv, total);
                });

        connect(task, &DownloadTask::finished, this, &DownloadManager::onTaskFinished);

        task->start();
    }

    void DownloadManager::onTaskFinished(bool success, const QString &error) {
        auto *task = qobject_cast<DownloadTask *>(sender());
        {
            QMutexLocker locker(&mutex_);
            if (task) {
                active_.removeOne(task);
                task->deleteLater();
            }

            ++completedCount_;

            if (!success && !error.isEmpty()) {
                errors_.append(error);
                X4_WARN("Download", error);
            }
        }

        emit overallProgress(completedCount_, totalCount_);

        bool hasMore = false;
        bool allDone = false;
        {
            QMutexLocker locker(&mutex_);
            hasMore = !queue_.isEmpty();
            allDone = queue_.isEmpty() && active_.isEmpty();
        }

        if (hasMore) {
            startNext();
        } else if (allDone) {
            QMutexLocker locker(&mutex_);
            emit allFinished(errors_.isEmpty(), errors_);
        }
    }
}