#pragma once

#include <QMainWindow>
#include <QGridLayout>
#include <QScrollArea>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QUrl>
#include <memory>

class VideoFrameWidget;
class RtspStream;
class IpcServer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void addStream(const QString &id, const QUrl &url);
    void removeStream(const QString &id);
    void setGridDimensions(int rows, int cols);

private slots:
    void handleFrame(const QString &id, const QImage &frame);
    void handleState(const QString &id, const QString &state);
    void handleIpcCommand(const QString &cmd, const QJsonObject &payload);

private:
    void relayout();

    QWidget *centralContainer;
    QGridLayout *gridLayout;

    int gridRows;
    int gridCols;

    struct StreamItem {
        RtspStream *stream;
        VideoFrameWidget *widget;
    };

    QMap<QString, StreamItem> streams; // key: id

    std::unique_ptr<IpcServer> ipcServer;
};


