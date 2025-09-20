#pragma once

#include <QObject>
#include <QImage>
#include <QUrl>
#include <QString>
#include <QThread>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

class RtspStream : public QObject {
    Q_OBJECT
public:
    RtspStream(const QString &id, const QUrl &url, QObject *parent = nullptr);
    ~RtspStream() override;

    void start();
    void stop();

signals:
    void frameReady(const QString &id, const QImage &frame);
    void stateChanged(const QString &id, const QString &state);

private:
        static void onRtspPadAddedStatic(GstElement *src, GstPad *new_pad, gpointer user_data);
        void onRtspPadAdded(GstElement *src, GstPad *new_pad);
    static GstFlowReturn onNewSampleStatic(GstAppSink *appsink, gpointer user_data);
    GstFlowReturn onNewSample(GstAppSink *appsink);

    QString id;
    QUrl url;

    GstElement *pipeline;
    GstElement *appsink;
    GstElement *rtspsrc;
    GstElement *queue;
    GstElement *videoconvert;
    bool running;
        bool linked;
};


