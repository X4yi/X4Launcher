#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QJsonObject>
#include <QDateTime>

namespace x4::auth {
    struct MicrosoftTokenResponse {
        QString accessToken;
        QString refreshToken;
        int expiresIn = 0;
        QDateTime obtainedAt;
    };

    class MicrosoftOAuthClient : public QObject {
        Q_OBJECT

    public:
        explicit MicrosoftOAuthClient(QNetworkAccessManager *network, QObject *parent = nullptr);

        // Returns the URL to open in the system browser
        QUrl buildAuthorizeUrl(const QString &clientId, const QString &redirectUri, const QString &state,
                               const QString &pkceChallenge, const QString &pkceMethod) const;

        // Exchanges the code for tokens
        void exchangeCodeForToken(const QString &clientId, const QString &redirectUri, const QString &code,
                                  const QString &pkceVerifier);

        // Refreshes the tokens using the refresh token
        void refreshToken(const QString &clientId, const QString &redirectUri, const QString &refreshToken);

        signals:
    

        void tokenReceived(const MicrosoftTokenResponse &token);

        void errorOccurred(const QString &errorMsg);

    private
        slots:
    

        void onTokenReply();

    private:
        void sendTokenRequest(const QByteArray &body);

        QNetworkAccessManager *network_;
        static constexpr const char *AuthorizeEndpoint =
                "https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize";
        static constexpr const char *TokenEndpoint = "https://login.microsoftonline.com/consumers/oauth2/v2.0/token";
        static constexpr const char *Scopes = "XboxLive.signin XboxLive.offline_access";
    };
} // namespace x4::auth