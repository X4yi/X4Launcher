#pragma once

#include <QObject>
#include <QUrl>
#include <QVector>
#include <QQueue>
#include <QMutex>

namespace x4 {
    class DownloadTask;

    class DownloadManager : public QObject {
        Q_OBJECT

    public:
        struct Entry {
            QUrl url;
            QString destPath;
            QString sha1;
            qint64 size = 0;
        };

        explicit DownloadManager(QObject *parent = nullptr);

        void setParallelism(int n) { maxParallel_ = qBound(1, n, 20); }

        void enqueue(const QUrl &url, const QString &destPath,
                     const QString &sha1 = {}, qint64 size = 0);

        void enqueue(const QVector<Entry> &entries);

        void startAll();

        void abortAll();

        int totalCount() const { return totalCount_ > 0 ? totalCount_ : queue_.size(); }
        int completedCount() const { return completedCount_; }

        signals:
    

        void overallProgress(int completed, int total);

        void taskProgress(const QString &file, qint64 received, qint64 total);

        void allFinished(bool success, const QStringList &errors);

    private:
        void startNext();

        void onTaskFinished(bool success, const QString &error);

        QQueue<Entry> queue_;
        QVector<DownloadTask *> active_;
        QMutex mutex_;
        QStringList errors_;
        int maxParallel_ = 6;
        int totalCount_ = 0;
        int completedCount_ = 0;
    };
}