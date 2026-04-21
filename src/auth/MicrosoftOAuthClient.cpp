#include "MicrosoftOAuthClient.h"

#include <QNetworkRequest>
#include <QDebug>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>

namespace x4::auth {

MicrosoftOAuthClient::MicrosoftOAuthClient(QNetworkAccessManager* network, QObject* parent)
    : QObject(parent), network_(network) {
}

QUrl MicrosoftOAuthClient::buildAuthorizeUrl(const QString& clientId, const QString& redirectUri, const QString& state, const QString& pkceChallenge, const QString& pkceMethod) const {
    QUrl url(AuthorizeEndpoint);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("client_id"), clientId);
    query.addQueryItem(QStringLiteral("response_type"), QStringLiteral("code"));
    query.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    query.addQueryItem(QStringLiteral("scope"), Scopes);
    query.addQueryItem(QStringLiteral("state"), state);
    query.addQueryItem(QStringLiteral("code_challenge"), pkceChallenge);
    query.addQueryItem(QStringLiteral("code_challenge_method"), pkceMethod);
    query.addQueryItem(QStringLiteral("prompt"), QStringLiteral("select_account"));
    url.setQuery(query);
    return url;
}

void MicrosoftOAuthClient::exchangeCodeForToken(const QString& clientId, const QString& redirectUri, const QString& code, const QString& pkceVerifier) {
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), clientId);
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("authorization_code"));
    body.addQueryItem(QStringLiteral("code"), code);
    body.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    body.addQueryItem(QStringLiteral("code_verifier"), pkceVerifier);
    body.addQueryItem(QStringLiteral("scope"), Scopes);

    sendTokenRequest(body.toString(QUrl::FullyEncoded).toUtf8());
}

void MicrosoftOAuthClient::refreshToken(const QString& clientId, const QString& redirectUri, const QString& refreshToken) {
    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), clientId);
    body.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("refresh_token"));
    body.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
    body.addQueryItem(QStringLiteral("redirect_uri"), redirectUri);
    body.addQueryItem(QStringLiteral("scope"), Scopes);

    sendTokenRequest(body.toString(QUrl::FullyEncoded).toUtf8());
}

void MicrosoftOAuthClient::sendTokenRequest(const QByteArray& body) {
    QNetworkRequest request(QUrl(QStringLiteral("%1").arg(TokenEndpoint)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = network_->post(request, body);
    connect(reply, &QNetworkReply::finished, this, &MicrosoftOAuthClient::onTokenReply);
}

void MicrosoftOAuthClient::onTokenReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    // Always read the body — even on HTTP errors, Microsoft returns a JSON body with the actual error
    QByteArray data = reply->readAll();
    
    qDebug() << "[Auth] Token endpoint HTTP status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "[Auth] Token endpoint response body:" << data;

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        emit errorOccurred(QStringLiteral("Failed to parse Microsoft token response: %1 (raw: %2)")
            .arg(jsonError.errorString(), QString::fromUtf8(data.left(500))));
        return;
    }

    QJsonObject obj = doc.object();
    
    if (obj.contains(QStringLiteral("error"))) {
        QString error = obj.value(QStringLiteral("error")).toString();
        QString desc = obj.value(QStringLiteral("error_description")).toString();
        qDebug() << "[Auth] Token error:" << error << desc;
        emit errorOccurred(QStringLiteral("Microsoft OAuth Error: %1 - %2").arg(error, desc));
        return;
    }

    if (reply->error() != QNetworkReply::NoError && !obj.contains(QStringLiteral("access_token"))) {
        emit errorOccurred(QStringLiteral("Microsoft OAuth network error: %1").arg(reply->errorString()));
        return;
    }

    MicrosoftTokenResponse token;
    token.accessToken = obj.value(QStringLiteral("access_token")).toString();
    token.refreshToken = obj.value(QStringLiteral("refresh_token")).toString();
    token.expiresIn = obj.value(QStringLiteral("expires_in")).toInt();
    token.obtainedAt = QDateTime::currentDateTimeUtc();

    if (token.accessToken.isEmpty()) {
        emit errorOccurred(QStringLiteral("Access token is missing from Microsoft response"));
        return;
    }

    qDebug() << "[Auth] Token exchange successful, expires in" << token.expiresIn << "seconds";
    emit tokenReceived(token);
}

} // namespace x4::auth
