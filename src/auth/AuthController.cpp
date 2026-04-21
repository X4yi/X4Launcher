#include "AuthController.h"

#include "MicrosoftOAuthClient.h"
#include "XboxLiveClient.h"
#include "XSTSClient.h"
#include "MinecraftAuthClient.h"
#include "LoopbackServer.h"
#include "TokenStore.h"
#include "../network/HttpClient.h"

#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

namespace x4::auth {

AuthController::AuthController(QObject* parent)
    : QObject(parent),
      network_(HttpClient::instance().manager()),
      msClient_(new MicrosoftOAuthClient(network_, this)),
      xblClient_(new XboxLiveClient(network_, this)),
      xstsClient_(new XSTSClient(network_, this)),
      mcClient_(new MinecraftAuthClient(network_, this)),
      tokenStore_(createTokenStore()),
      server_(nullptr) {

    connect(msClient_, &MicrosoftOAuthClient::tokenReceived, this, &AuthController::onMsTokenReceived);
    connect(msClient_, &MicrosoftOAuthClient::errorOccurred, this, &AuthController::onErrorOccurred);

    connect(xblClient_, &XboxLiveClient::authenticated, this, &AuthController::onXblAuthenticated);
    connect(xblClient_, &XboxLiveClient::errorOccurred, this, &AuthController::onErrorOccurred);

    connect(xstsClient_, &XSTSClient::authenticated, this, &AuthController::onXstsAuthenticated);
    connect(xstsClient_, &XSTSClient::errorOccurred, this, &AuthController::onErrorOccurred);

    connect(mcClient_, &MinecraftAuthClient::authenticated, this, &AuthController::onMinecraftAuthenticated);
    connect(mcClient_, &MinecraftAuthClient::errorOccurred, this, &AuthController::onErrorOccurred);
}

AuthController::~AuthController() {
    cleanupSession();
}

void AuthController::login() {
    cleanupSession();
    
    currentSession_ = std::make_unique<OAuthSession>();
    server_ = new LoopbackServer(this);
    
    connect(server_, &LoopbackServer::codeReceived, this, &AuthController::onCodeReceived);
    connect(server_, &LoopbackServer::errorReceived, this, &AuthController::onLoopbackError);

    if (!server_->listen()) {
        emit loginFailed(QStringLiteral("Failed to bind to local port for authentication."));
        cleanupSession();
        return;
    }

    QString redirectUri = QStringLiteral("http://127.0.0.1:%1").arg(server_->port());
    QUrl url = msClient_->buildAuthorizeUrl(ClientId, redirectUri, currentSession_->state(), currentSession_->pkceChallenge(), currentSession_->pkceChallengeMethod());

    qDebug() << "[Auth] Loopback server listening on port" << server_->port();
    qDebug() << "[Auth] Redirect URI:" << redirectUri;
    qDebug() << "[Auth] PKCE verifier length:" << currentSession_->pkceVerifier().length();
    qDebug() << "[Auth] Opening browser for login...";

    emit loginStatusChanged(QStringLiteral("Waiting for browser login..."));
    QDesktopServices::openUrl(url);
}

void AuthController::loginWithRefreshToken(const QString& refreshToken) {
    if (refreshToken.isEmpty()) {
        emit loginFailed(QStringLiteral("No refresh token available."));
        return;
    }
    
    emit loginStatusChanged(QStringLiteral("Refreshing session..."));
    msClient_->refreshToken(ClientId, QStringLiteral("http://localhost"), refreshToken);
}

void AuthController::logout(const QString& accountId) {
    deleteRefreshToken(accountId);
}

bool AuthController::saveRefreshToken(const QString& accountId, const QString& refreshToken) {
    if (tokenStore_) {
        return tokenStore_->saveToken(accountId, refreshToken);
    }
    return false;
}

QString AuthController::loadRefreshToken(const QString& accountId) {
    if (tokenStore_) {
        return tokenStore_->loadToken(accountId);
    }
    return QString();
}

bool AuthController::deleteRefreshToken(const QString& accountId) {
    if (tokenStore_) {
        return tokenStore_->deleteToken(accountId);
    }
    return false;
}

void AuthController::onCodeReceived(const QString& code, const QString& state) {
    qDebug() << "[Auth] Code received, length:" << code.length() << "state:" << state;
    
    if (!currentSession_ || state != currentSession_->state()) {
        qDebug() << "[Auth] State mismatch! Expected:" << (currentSession_ ? currentSession_->state() : "(no session)");
        emit loginFailed(QStringLiteral("State mismatch during authentication."));
        cleanupSession();
        return;
    }

    emit loginStatusChanged(QStringLiteral("Exchanging authorization code..."));
    
    QString redirectUri = QStringLiteral("http://127.0.0.1:%1").arg(server_->port());
    QString verifier = currentSession_->pkceVerifier();
    
    qDebug() << "[Auth] Token exchange redirect_uri:" << redirectUri;
    qDebug() << "[Auth] Token exchange verifier length:" << verifier.length();
    
    msClient_->exchangeCodeForToken(ClientId, redirectUri, code, verifier);
    
    // We don't need the server or session anymore
    cleanupSession();
}

void AuthController::onLoopbackError(const QString& error, const QString& errorDescription) {
    emit loginFailed(QStringLiteral("Authentication cancelled or failed: %1 - %2").arg(error, errorDescription));
    cleanupSession();
}

void AuthController::onMsTokenReceived(const MicrosoftTokenResponse& token) {
    qDebug() << "[Auth] MS token received, starting Xbox Live auth...";
    currentRefreshToken_ = token.refreshToken;
    emit loginStatusChanged(QStringLiteral("Authenticating with Xbox Live..."));
    xblClient_->authenticate(token.accessToken);
}

void AuthController::onXblAuthenticated(const QString& xblToken, const QString& userHash) {
    qDebug() << "[Auth] Xbox Live authenticated, userHash:" << userHash;
    Q_UNUSED(userHash);
    emit loginStatusChanged(QStringLiteral("Obtaining XSTS Token..."));
    xstsClient_->authenticate(xblToken);
}

void AuthController::onXstsAuthenticated(const QString& xstsToken, const QString& userHash) {
    qDebug() << "[Auth] XSTS authenticated, logging into Minecraft...";
    emit loginStatusChanged(QStringLiteral("Logging into Minecraft..."));
    mcClient_->loginWithXbox(xstsToken, userHash);
}

void AuthController::onMinecraftAuthenticated(const MinecraftProfile& profile) {
    qDebug() << "[Auth] Minecraft authenticated! Player:" << profile.name << "UUID:" << profile.id;
    emit loginStatusChanged(QStringLiteral("Login successful!"));
    
    AuthAccount account;
    account.id = profile.id;
    account.uuid = profile.id;
    account.alias = profile.name;
    account.username = profile.name;
    account.minecraftUsername = profile.name;
    account.accessToken = profile.accessToken;
    account.expiresAt = profile.obtainedAt.addSecs(profile.expiresIn);
    account.isPremium = true;
    
    // Securely save the refresh token
    if (!currentRefreshToken_.isEmpty()) {
        saveRefreshToken(account.id, currentRefreshToken_);
    }
    
    emit loginSuccess(account);
}

void AuthController::onErrorOccurred(const QString& errorMsg) {
    qDebug() << "[Auth] ERROR:" << errorMsg;
    emit loginFailed(errorMsg);
    cleanupSession();
}

void AuthController::cleanupSession() {
    if (server_) {
        server_->close();
        server_->deleteLater();
        server_ = nullptr;
    }
    currentSession_.reset();
}

} // namespace x4::auth
