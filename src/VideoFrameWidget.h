#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>

class VideoFrameWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoFrameWidget(QWidget *parent = nullptr);
    QSize sizeHint() const override;

public slots:
    void setFrame(const QImage &image);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage currentFrame;
    QMutex frameMutex;
};


