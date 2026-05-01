#include "LoopbackServer.h"
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>

namespace x4::auth {
    LoopbackServer::LoopbackServer(QObject *parent)
        : QObject(parent), server_(new QTcpServer(this)) {
        connect(server_, &QTcpServer::newConnection, this, &LoopbackServer::onNewConnection);
    }

    LoopbackServer::~LoopbackServer() {
        close();
    }

    bool LoopbackServer::listen() {
        // Bind to 127.0.0.1 on any available port
        return server_->listen(QHostAddress::LocalHost, 0);
    }

    quint16 LoopbackServer::port() const {
        return server_->serverPort();
    }

    void LoopbackServer::close() {
        if (server_->isListening()) {
            server_->close();
        }
    }

    void LoopbackServer::onNewConnection() {
        QTcpSocket *socket = server_->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &LoopbackServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }

    void LoopbackServer::onReadyRead() {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
        if (!socket) return;

        if (!socket->canReadLine()) return;

        QByteArray requestLine = socket->readLine();
        QString requestStr = QString::fromUtf8(requestLine);

        // Read the rest of the headers to clear the socket buffer
        while (socket->canReadLine()) {
            QByteArray line = socket->readLine();
            if (line == "\r\n" || line == "\n") break;
        }

        // Expected format: GET /?code=...&state=... HTTP/1.1
        QRegularExpression re(QStringLiteral("^GET\\s+(.*?)\\s+HTTP"));
        QRegularExpressionMatch match = re.match(requestStr);

        if (match.hasMatch()) {
            QString path = match.captured(1);
            QUrl url(path);
            QUrlQuery query(url.query());

            if (query.hasQueryItem(QStringLiteral("code"))) {
                QString code = query.queryItemValue(QStringLiteral("code"));
                QString state = query.queryItemValue(QStringLiteral("state"));

                sendResponse(socket, true);
                emit codeReceived(code, state);

                // Auto close after receiving code
                close();
                return;
            } else if (query.hasQueryItem(QStringLiteral("error"))) {
                QString error = query.queryItemValue(QStringLiteral("error"));
                QString errorDescription = query.queryItemValue(QStringLiteral("error_description"));

                sendResponse(socket, false);
                emit errorReceived(error, errorDescription);

                close();
                return;
            }
        }

        // If it doesn't match or doesn't have our params, just drop it.
        socket->disconnectFromHost();
    }

    void LoopbackServer::sendResponse(QTcpSocket *socket, bool success) {
        QString html = QStringLiteral(
            "<html>"
            "<head><title>Authentication %1</title>"
            "<style>"
            "body { font-family: sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #1e1e2e; color: #cdd6f4; }"
            ".box { background: #313244; padding: 2rem; border-radius: 8px; text-align: center; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }"
            "h1 { color: %2; }"
            "</style>"
            "</head>"
            "<body>"
            "<div class=\"box\">"
            "<h1>%3</h1>"
            "<p>You can now safely close this tab and return to the launcher.</p>"
            "</div>"
            "</body>"
            "</html>"
        ).arg(
            success ? "Successful" : "Failed",
            success ? "#a6e3a1" : "#f38ba8",
            success ? "Authentication Successful" : "Authentication Failed"
        );

        QString response = QStringLiteral(
                               "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                           ) + html;

        socket->write(response.toUtf8());
        socket->flush();
        socket->disconnectFromHost();
    }
} // namespace x4::auth