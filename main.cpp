#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QProcess>
#include <QDebug>
#include "cnc_config.h"

int main(int argc, char *argv[])
{
    // enable high DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
