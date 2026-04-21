#include "MetaAPI.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include "../network/HttpClient.h"
namespace x4 {

MetaAPI::MetaAPI(QObject* parent) : QObject(parent) {
    rootMgr_ = HttpClient::instance().manager();
}

void MetaAPI::getPackageIndex(const QString& uid) {
    QUrl url(QString("https://meta.prismlauncher.org/v1/%1/index.json").arg(uid));
    auto* reply = rootMgr_->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, uid]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit indexFailed(uid, reply->errorString());
        } else {
            emit indexFinished(uid, reply->readAll());
        }
    });
}

void MetaAPI::getPackageVersion(const QString& uid, const QString& version) {
    QUrl url(QString("https://meta.prismlauncher.org/v1/%1/%2.json").arg(uid, version));
    auto* reply = rootMgr_->get(QNetworkRequest(url));
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, uid, version]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit versionFailed(uid, version, reply->errorString());
        } else {
            emit versionFinished(uid, version, reply->readAll());
        }
    });
}

}
