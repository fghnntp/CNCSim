#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QProcess>
#include <QDebug>
#include "cnc_config.h"
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(3, 3);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(4);               // 开启 MSAA（4x），显卡不支持时驱动会降级
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
