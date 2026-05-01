#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

namespace x4::auth {
    class XSTSClient : public QObject {
        Q_OBJECT

    public:
        explicit XSTSClient(QNetworkAccessManager *network, QObject *parent = nullptr);

        // Authenticate with XSTS using the XBL token
        void authenticate(const QString &xblToken);

        signals:
    

        void authenticated(const QString &xstsToken, const QString &userHash);

        void errorOccurred(const QString &errorMsg);

    private
        slots:
    

        void onReply();

    private:
        QNetworkAccessManager *network_;
        static constexpr const char *XSTSEndpoint = "https://xsts.auth.xboxlive.com/xsts/authorize";
        static constexpr const char *RelyingParty = "rp://api.minecraftservices.com/";
    };
} // namespace x4::auth