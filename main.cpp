#include "mainwindow.h"

#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    // 启用高DPI缩放
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    // 获取主屏幕并计算缩放因子
    QScreen *screen = QApplication::primaryScreen();
    qreal scaleFactor = qMin(screen->size().width() / 1920.0,
                             screen->size().height() / 1080.0);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);  // 启用高DPI缩放[3,5](@ref)
    MainWindow w;
    w.show();
    return a.exec();
}
