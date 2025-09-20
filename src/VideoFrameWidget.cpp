#include "VideoFrameWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMutexLocker>

VideoFrameWidget::VideoFrameWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(160, 90);
    setAutoFillBackground(true);
}

QSize VideoFrameWidget::sizeHint() const {
    return {320, 180};
}

void VideoFrameWidget::setFrame(const QImage &image) {
    {
        QMutexLocker locker(&frameMutex);
        currentFrame = image.copy();
    }
    update();
}

void VideoFrameWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    QImage frameCopy;
    {
        QMutexLocker locker(&frameMutex);
        frameCopy = currentFrame;
    }

    if (!frameCopy.isNull()) {
        const QSize imgSize = frameCopy.size();
        const QSize targetSize = rect().size();
        const Qt::AspectRatioMode mode = Qt::KeepAspectRatio;
        QSize scaled = imgSize.scaled(targetSize, mode);
        QRect target((width() - scaled.width())/2, (height() - scaled.height())/2,
                     scaled.width(), scaled.height());
        p.drawImage(target, frameCopy);
    }
}


