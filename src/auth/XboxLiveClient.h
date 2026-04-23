#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

namespace x4::auth {

class XboxLiveClient : public QObject {
    Q_OBJECT
public:
    explicit XboxLiveClient(QNetworkAccessManager* network, QObject* parent = nullptr);

    // Authenticate with Xbox Live using the MS access token
    void authenticate(const QString& msAccessToken);

signals:
    void authenticated(const QString& xblToken, const QString& userHash);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onReply();

private:
    QNetworkAccessManager* network_;
    static constexpr const char* XBLEndpoint = "https://user.auth.xboxlive.com/user/authenticate";
};

} // namespace x4::auth
