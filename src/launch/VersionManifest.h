#pragma once

#include <QObject>
#include <QVector>
#include <QUrl>

namespace x4 {

struct VersionEntry {
    QString id;
    QString type;
    QUrl url;
    QString sha1;
    QString releaseTime;
};

class VersionManifest : public QObject {
    Q_OBJECT
public:
    explicit VersionManifest(QObject* parent = nullptr);

    void fetch();

    const QVector<VersionEntry>& versions() const { return versions_; }

    QString latestRelease() const { return latestRelease_; }
    QString latestSnapshot() const { return latestSnapshot_; }

    const VersionEntry* find(const QString& id) const;

    bool isLoaded() const { return loaded_; }

signals:
    void ready();
    void fetchError(const QString& error);

private:
    void loadFromCache();
    void saveToCache(const QByteArray& data);

    QVector<VersionEntry> versions_;
    QString latestRelease_;
    QString latestSnapshot_;
    bool loaded_ = false;
};

}
