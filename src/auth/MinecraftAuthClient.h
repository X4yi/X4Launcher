#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QDateTime>

namespace x4::auth {

struct MinecraftProfile {
    QString id;
    QString name;
    QString accessToken;
    QDateTime obtainedAt;
    int expiresIn = 0;
};

class MinecraftAuthClient : public QObject {
    Q_OBJECT
public:
    explicit MinecraftAuthClient(QNetworkAccessManager* network, QObject* parent = nullptr);

    // Login with Xbox token and user hash
    void loginWithXbox(const QString& xstsToken, const QString& userHash);

signals:
    void authenticated(const MinecraftProfile& profile);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onLoginReply();

private:
    void fetchProfile(const QString& mcAccessToken, int expiresIn);

    QNetworkAccessManager* network_;
    static constexpr const char* LoginEndpoint = "https://api.minecraftservices.com/authentication/login_with_xbox";
    static constexpr const char* ProfileEndpoint = "https://api.minecraftservices.com/minecraft/profile";
};

} // namespace x4::auth
