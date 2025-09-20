#include "IpcServer.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>

IpcServer::IpcServer(QObject *parent)
    : QObject(parent) {
}

IpcServer::~IpcServer() {
}

void IpcServer::start(const QString &socketName) {
    // Ensure previous stale socket is removed
    QFile::remove(socketName);
    QLocalServer::removeServer(socketName);
    connect(&server, &QLocalServer::newConnection, this, &IpcServer::handleNewConnection);
    server.listen(socketName);
}

void IpcServer::handleNewConnection() {
    while (QLocalSocket *sock = server.nextPendingConnection()) {
        connect(sock, &QLocalSocket::readyRead, this, &IpcServer::handleReadyRead);
        connect(sock, &QLocalSocket::disconnected, sock, &QObject::deleteLater);
    }
}

void IpcServer::handleReadyRead() {
    QLocalSocket *sock = qobject_cast<QLocalSocket*>(sender());
    if (!sock) return;
    buffer.append(sock->readAll());
    processBuffer();
}

void IpcServer::processBuffer() {
    // Simple newline-delimited JSON protocol
    while (true) {
        int nl = buffer.indexOf('\n');
        if (nl < 0) break;
        QByteArray line = buffer.left(nl);
        buffer.remove(0, nl + 1);
        if (line.trimmed().isEmpty()) continue;
        QJsonParseError err{};
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }
        QJsonObject obj = doc.object();
        QString cmd = obj.value("cmd").toString();
        QJsonObject payload = obj.value("payload").toObject();
        if (!cmd.isEmpty()) emit receivedCommand(cmd, payload);
    }
}


