#include "RtspStream.h"

#include <QDebug>
#include <QImage>

static QString gstStateToString(GstState state) {
    switch (state) {
        case GST_STATE_VOID_PENDING: return "PENDING";
        case GST_STATE_NULL: return "NULL";
        case GST_STATE_READY: return "READY";
        case GST_STATE_PAUSED: return "PAUSED";
        case GST_STATE_PLAYING: return "PLAYING";
        default: return "UNKNOWN";
    }
}

RtspStream::RtspStream(const QString &id, const QUrl &url, QObject *parent)
    : QObject(parent), id(id), url(url), pipeline(nullptr), appsink(nullptr), rtspsrc(nullptr), queue(nullptr), videoconvert(nullptr), running(false), linked(false) {
}

RtspStream::~RtspStream() {
    stop();
}

void RtspStream::start() {
    if (running) return;

    // Build explicit pipeline: rtspsrc ! depay ! decode ! queue ! videoconvert ! appsink

    pipeline = gst_pipeline_new(nullptr);
    if (!pipeline) {
        emit stateChanged(id, "Failed to create pipeline");
        return;
    }

    rtspsrc = gst_element_factory_make("rtspsrc", nullptr);
    videoconvert = gst_element_factory_make("videoconvert", nullptr);
    queue = gst_element_factory_make("queue", nullptr);
    appsink = gst_element_factory_make("appsink", nullptr);
    if (!rtspsrc || !videoconvert || !queue || !appsink) {
        emit stateChanged(id, "Failed to create elements");
        if (pipeline) gst_object_unref(pipeline);
        pipeline = nullptr;
        return;
    }

    g_object_set(G_OBJECT(rtspsrc),
                 "location", url.toString().toUtf8().constData(),
                 "latency", 0,
                 NULL);
    // Prefer BGRA for easy QImage mapping; if upstream can't provide, uridecodebin will convert if possible
    GstCaps *caps = gst_caps_from_string("video/x-raw,format=BGRA");
    g_object_set(G_OBJECT(appsink),
                 "emit-signals", TRUE,
                 "sync", FALSE,
                 "max-buffers", 5,
                 "drop", TRUE,
                 "caps", caps,
                 NULL);
    if (caps) gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipeline), rtspsrc, queue, videoconvert, appsink, NULL);

    // rtspsrc has dynamic pads per stream. We create depay/decoder based on caps.
    g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, gpointer user_data) {
        RtspStream *self = static_cast<RtspStream*>(user_data);
        if (self->linked) return;

        GstCaps *caps = gst_pad_get_current_caps(new_pad);
        if (!caps) return;
        GstStructure *st = gst_caps_get_structure(caps, 0);
        const gchar *media = gst_structure_get_string(st, "media");
        const gchar *encoding = gst_structure_get_string(st, "encoding-name");
        bool isVideo = media && g_str_equal(media, "video");
        bool isH264 = encoding && g_str_equal(encoding, "H264");
        bool isH265 = encoding && (g_str_equal(encoding, "H265") || g_str_equal(encoding, "HEVC"));
        gst_caps_unref(caps);
        if (!isVideo) return;

        GstElement *depay = nullptr;
        GstElement *parse = nullptr;
        GstElement *dec = nullptr;
        if (isH265) {
            depay = gst_element_factory_make("rtph265depay", nullptr);
            parse = gst_element_factory_make("h265parse", nullptr);
            dec = gst_element_factory_make("avdec_h265", nullptr);
        } else if (isH264) {
            depay = gst_element_factory_make("rtph264depay", nullptr);
            parse = gst_element_factory_make("h264parse", nullptr);
            dec = gst_element_factory_make("avdec_h264", nullptr);
        } else {
            emit self->stateChanged(self->id, "Unsupported RTP encoding");
            return;
        }

        if (!depay || !parse || !dec) {
            if (depay) gst_object_unref(depay);
            if (parse) gst_object_unref(parse);
            if (dec) gst_object_unref(dec);
            emit self->stateChanged(self->id, "Failed to create depay/decoder");
            return;
        }

        gst_bin_add_many(GST_BIN(self->pipeline), depay, parse, dec, NULL);
        if (!gst_element_link_many(depay, parse, dec, self->queue, NULL)) {
            emit self->stateChanged(self->id, "Failed to link depay/parse/dec/queue");
            return;
        }

        GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
        if (sinkpad) {
            if (gst_pad_link(new_pad, sinkpad) == GST_PAD_LINK_OK) {
                self->linked = true;
            }
            gst_object_unref(sinkpad);
        }

        // Ensure new elements follow parent state
        gst_element_sync_state_with_parent(depay);
        gst_element_sync_state_with_parent(parse);
        gst_element_sync_state_with_parent(dec);
    }), this);

    // Static link: queue ! videoconvert ! appsink
    gst_element_link_many(queue, videoconvert, appsink, NULL);

    GstAppSinkCallbacks callbacks = { nullptr, nullptr, &RtspStream::onNewSampleStatic };
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, this, nullptr);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        emit stateChanged(id, "Failed to start pipeline");
        gst_object_unref(pipeline);
        pipeline = nullptr;
        appsink = nullptr;
        return;
    }

    running = true;
    emit stateChanged(id, QString::fromLatin1("%1 -> PLAYING").arg(id));
}

void RtspStream::stop() {
    if (!running) return;
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    pipeline = nullptr;
    appsink = nullptr;
    running = false;
}

GstFlowReturn RtspStream::onNewSampleStatic(GstAppSink *appsink, gpointer user_data) {
    return static_cast<RtspStream*>(user_data)->onNewSample(appsink);
}

static QImage makeImageFromSample(GstSample *sample) {
    GstCaps *caps = gst_sample_get_caps(sample);
    if (!caps) return {};

    GstStructure *s = gst_caps_get_structure(caps, 0);
    int width = 0, height = 0;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) return {};

    GstVideoInfo vinfo;
    if (!gst_video_info_from_caps(&vinfo, caps)) {
        return {};
    }

    GstVideoFrame vframe;
    if (!gst_video_frame_map(&vframe, &vinfo, buffer, GST_MAP_READ)) {
        return {};
    }

    QImage image;
    if (GST_VIDEO_INFO_FORMAT(&vinfo) == GST_VIDEO_FORMAT_BGRA || GST_VIDEO_INFO_FORMAT(&vinfo) == GST_VIDEO_FORMAT_BGRx) {
        QImage::Format qfmt = (GST_VIDEO_INFO_FORMAT(&vinfo) == GST_VIDEO_FORMAT_BGRA) ? QImage::Format_ARGB32 : QImage::Format_RGB32;
        guint8 *data = static_cast<guint8*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 0));
        int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
        image = QImage(data, width, height, stride, qfmt).copy();
    }

    gst_video_frame_unmap(&vframe);
    return image;
}

GstFlowReturn RtspStream::onNewSample(GstAppSink *appsink) {
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) return GST_FLOW_OK;

    QImage image = makeImageFromSample(sample);
    if (!image.isNull()) {
        emit frameReady(id, image);
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}


