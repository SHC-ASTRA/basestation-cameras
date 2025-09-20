#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>

class IpcServer : public QObject {
    Q_OBJECT
public:
    explicit IpcServer(QObject *parent = nullptr);
    ~IpcServer() override;

    void start(const QString &socketName = QStringLiteral("/tmp/basestation-cameras-ipc"));

signals:
    void receivedCommand(const QString &cmd, const QJsonObject &payload);

private slots:
    void handleNewConnection();
    void handleReadyRead();

private:
    void processBuffer();

    QLocalServer server;
    QByteArray buffer;
};


