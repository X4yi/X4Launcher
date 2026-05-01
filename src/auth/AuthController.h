#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <memory>

#include "AuthAccount.h"
#include "OAuthSession.h"

namespace x4::auth {
    class MicrosoftOAuthClient;
    class XboxLiveClient;
    class XSTSClient;
    class MinecraftAuthClient;
    class LoopbackServer;
    class TokenStore;
    struct MicrosoftTokenResponse;
    struct MinecraftProfile;

    class AuthController : public QObject {
        Q_OBJECT

    public:
        explicit AuthController(QObject *parent = nullptr);

        ~AuthController() override;

        void login();

        void loginWithRefreshToken(const QString &refreshToken);

        void logout(const QString &accountId);

        // Secure token storage proxy
        bool saveRefreshToken(const QString &accountId, const QString &refreshToken);

        QString loadRefreshToken(const QString &accountId);

        bool deleteRefreshToken(const QString &accountId);

        signals:
    

        void loginStatusChanged(const QString &status);

        void loginSuccess(const AuthAccount &account);

        void loginFailed(const QString &errorMsg);

    private
        slots:
    

        void onCodeReceived(const QString &code, const QString &state);

        void onLoopbackError(const QString &error, const QString &errorDescription);

        void onMsTokenReceived(const MicrosoftTokenResponse &token);

        void onXblAuthenticated(const QString &xblToken, const QString &userHash);

        void onXstsAuthenticated(const QString &xstsToken, const QString &userHash);

        void onMinecraftAuthenticated(const MinecraftProfile &profile);

        void onErrorOccurred(const QString &errorMsg);

    private:
        void cleanupSession();

        QNetworkAccessManager *network_;
        MicrosoftOAuthClient *msClient_;
        XboxLiveClient *xblClient_;
        XSTSClient *xstsClient_;
        MinecraftAuthClient *mcClient_;

        std::unique_ptr<TokenStore> tokenStore_;
        std::unique_ptr<OAuthSession> currentSession_;
        LoopbackServer *server_;

        // Temporarily stored tokens during flow
        QString currentRefreshToken_;

        static constexpr const char *ClientId = "c36a9fb6-4f2a-41ff-90bd-ae7cc92031eb";
    };
} // namespace x4::auth