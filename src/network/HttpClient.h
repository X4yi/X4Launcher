#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCoreApplication>

namespace x4 {
    class HttpClient : public QObject {
        Q_OBJECT

    public:
        static HttpClient &instance();

        QNetworkReply *get(const QUrl &url);

        QNetworkReply *head(const QUrl &url);

        QNetworkAccessManager *manager() { return nam_; }

    private:
        HttpClient();

        QNetworkAccessManager *nam_ = nullptr;

        Q_DISABLE_COPY(HttpClient)
    };
}