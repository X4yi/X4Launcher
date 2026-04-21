#pragma once

#include <QObject>
#include <QTcpServer>

namespace x4::auth {

class LoopbackServer : public QObject {
    Q_OBJECT
public:
    explicit LoopbackServer(QObject* parent = nullptr);
    ~LoopbackServer() override;

    bool listen();
    quint16 port() const;
    void close();

signals:
    void codeReceived(const QString& code, const QString& state);
    void errorReceived(const QString& error, const QString& errorDescription);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void sendResponse(QTcpSocket* socket, bool success);

    QTcpServer* server_;
};

} // namespace x4::auth
