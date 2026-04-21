#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;

namespace x4 {

class MetaAPI : public QObject {
    Q_OBJECT
public:
    explicit MetaAPI(QObject* parent = nullptr);

    void getPackageIndex(const QString& uid);

    void getPackageVersion(const QString& uid, const QString& version);

signals:
    void indexFinished(const QString& uid, const QByteArray& data);
    void indexFailed(const QString& uid, const QString& error);

    void versionFinished(const QString& uid, const QString& version, const QByteArray& data);
    void versionFailed(const QString& uid, const QString& version, const QString& error);

private:
    QNetworkAccessManager* rootMgr_;
};

}
