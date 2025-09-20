#include <QApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"

#include <gst/gst.h>

int main(int argc, char *argv[]) {
    // Initialize GStreamer before Qt starts event loop
    gst_init(&argc, &argv);

    QApplication app(argc, argv);
    QApplication::setApplicationName("Basestation Cameras");
    QApplication::setOrganizationName("astra");

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}


