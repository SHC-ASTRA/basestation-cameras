#include "MainWindow.h"
#include "VideoFrameWidget.h"
#include "RtspStream.h"
#include "IpcServer.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStatusBar>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      centralContainer(new QWidget(this)),
      gridLayout(new QGridLayout()),
      gridRows(2),
      gridCols(2) {
    centralContainer->setLayout(gridLayout);
    setCentralWidget(centralContainer);

    statusBar()->showMessage("Ready");

    ipcServer = std::make_unique<IpcServer>(this);
    connect(ipcServer.get(), &IpcServer::receivedCommand,
            this, &MainWindow::handleIpcCommand);
    ipcServer->start();
}

MainWindow::~MainWindow() {
    // Ensure streams are destroyed before widgets
    streams.clear();
}

void MainWindow::addStream(const QString &id, const QUrl &url) {
    if (streams.contains(id)) {
        statusBar()->showMessage(QString::fromLatin1("Stream %1 already exists").arg(id));
        return;
    }

    auto widget = new VideoFrameWidget(centralContainer);
    auto stream = new RtspStream(id, url, this);

    connect(stream, &RtspStream::frameReady,
            this, &MainWindow::handleFrame);
    connect(stream, &RtspStream::stateChanged,
            this, &MainWindow::handleState);

    streams.insert(id, StreamItem{ stream, widget });
    relayout();

    streams[id].stream->start();
}

void MainWindow::removeStream(const QString &id) {
    if (!streams.contains(id)) return;
    auto item = streams.take(id);
    if (item.stream) { item.stream->stop(); item.stream->deleteLater(); }
    delete item.widget;
    relayout();
}

void MainWindow::setGridDimensions(int rows, int cols) {
    if (rows < 1 || cols < 1) return;
    gridRows = rows;
    gridCols = cols;
    relayout();
}

void MainWindow::relayout() {
    // Clear layout
    while (QLayoutItem *child = gridLayout->takeAt(0)) {
        if (child->widget()) child->widget()->setParent(nullptr);
        delete child;
    }

    int idx = 0;
    for (auto it = streams.begin(); it != streams.end(); ++it) {
        int r = idx / gridCols;
        int c = idx % gridCols;
        gridLayout->addWidget(it.value().widget, r, c);
        ++idx;
    }
}

void MainWindow::handleFrame(const QString &id, const QImage &frame) {
    if (!streams.contains(id)) return;
    streams[id].widget->setFrame(frame);
}

void MainWindow::handleState(const QString &id, const QString &state) {
    Q_UNUSED(id);
    statusBar()->showMessage(state, 2000);
}

void MainWindow::handleIpcCommand(const QString &cmd, const QJsonObject &payload) {
    if (cmd == "add_stream") {
        const QString id = payload.value("id").toString();
        const QString urlStr = payload.value("url").toString();
        addStream(id, QUrl(urlStr));
    } else if (cmd == "remove_stream") {
        const QString id = payload.value("id").toString();
        removeStream(id);
    } else if (cmd == "grid") {
        int rows = payload.value("rows").toInt(gridRows);
        int cols = payload.value("cols").toInt(gridCols);
        setGridDimensions(rows, cols);
    }
}


