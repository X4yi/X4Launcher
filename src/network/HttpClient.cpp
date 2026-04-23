#include "HttpClient.h"
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QNetworkProxyFactory>

namespace x4 {

HttpClient& HttpClient::instance() {
    static HttpClient hc;
    return hc;
}

HttpClient::HttpClient() {
    nam_ = new QNetworkAccessManager();
    // Disable proxy resolution which consumes CPU on every request
    QNetworkProxyFactory::setUseSystemConfiguration(false);

    if (QCoreApplication::instance()) {
        nam_->moveToThread(QCoreApplication::instance()->thread());
    }
    // keep previous timeout behavior if available
    nam_->setParent(nullptr);
    // set transfer timeout if supported by Qt build
    #ifdef Q_NETWORKACCESSMANAGER_TRANSFER_TIMEOUT
    nam_->setTransferTimeout(30000);
    #endif
}

QNetworkReply* HttpClient::get(const QUrl& url) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("X4Launcher/0.1"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return nam_->get(req);
}

QNetworkReply* HttpClient::head(const QUrl& url) {
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("X4Launcher/0.1"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return nam_->head(req);
}

}
