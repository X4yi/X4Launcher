#include "XSTSClient.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace x4::auth {
    XSTSClient::XSTSClient(QNetworkAccessManager *network, QObject *parent)
        : QObject(parent), network_(network) {
    }

    void XSTSClient::authenticate(const QString &xblToken) {
        QJsonObject properties;
        properties[QStringLiteral("SandboxId")] = QStringLiteral("RETAIL");

        QJsonArray userTokens;
        userTokens.append(xblToken);
        properties[QStringLiteral("UserTokens")] = userTokens;

        QJsonObject body;
        body[QStringLiteral("Properties")] = properties;
        body[QStringLiteral("RelyingParty")] = QStringLiteral("%1").arg(RelyingParty);
        body[QStringLiteral("TokenType")] = QStringLiteral("JWT");

        QJsonDocument doc(body);

        QNetworkRequest request(QUrl(QStringLiteral("%1").arg(XSTSEndpoint)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json");

        QNetworkReply *reply = network_->post(request, doc.toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, &XSTSClient::onReply);
    }

    void XSTSClient::onReply() {
        QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
        if (!reply) return;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            // Special handling for XSTS errors (like child accounts)
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("XErr")) {
                    long long xerr = obj.value("XErr").toVariant().toLongLong();
                    if (xerr == 2148916233) {
                        emit errorOccurred(QStringLiteral("The account doesn't have an Xbox account."));
                        return;
                    } else if (xerr == 2148916238) {
                        emit errorOccurred(
                            QStringLiteral(
                                "The account is a child account and must be added to a Family by an adult."));
                        return;
                    }
                }
            }

            emit errorOccurred(QStringLiteral("XSTS Error: %1").arg(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError) {
            emit errorOccurred(QStringLiteral("Failed to parse XSTS response: %1").arg(error.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        QString token = obj.value(QStringLiteral("Token")).toString();

        QJsonObject displayClaims = obj.value(QStringLiteral("DisplayClaims")).toObject();
        QJsonArray xui = displayClaims.value(QStringLiteral("xui")).toArray();

        if (token.isEmpty() || xui.isEmpty()) {
            emit errorOccurred(QStringLiteral("Invalid XSTS token response format"));
            return;
        }

        QString userHash = xui.first().toObject().value(QStringLiteral("uhs")).toString();

        qDebug() << "[Auth] XSTS token prefix:" << token.left(40) << "... userHash:" << userHash;

        emit authenticated(token, userHash);
    }
} // namespace x4::auth