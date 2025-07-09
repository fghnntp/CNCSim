#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QProcess>
#include <QDebug>
#include "cnc_config.h"

void setupLinuxCNCEnvironment() {
    QProcess envProcess;

    // Run the script in bash to get the environment
    envProcess.start("bash",
                     QStringList() << "-c" <<
                     ENV_TRIG_CMD);

    if (!envProcess.waitForFinished()) {
        qCritical() << "Failed to source LinuxCNC environment";
        return;
    }

    // Parse the environment variables
    QByteArray output = envProcess.readAllStandardOutput();
    foreach (const QByteArray &line, output.split('\n')) {
        int equalIndex = line.indexOf('=');
        if (equalIndex > 0) {
            QByteArray name = line.left(equalIndex);
            QByteArray value = line.mid(equalIndex + 1);
            qputenv(name.constData(), value);
        }
    }
}


int main(int argc, char *argv[])
{
    // Set up LinuxCNC environment before creating QApplication
    setupLinuxCNCEnvironment();

    // Verify environment
    qDebug() << "PATH:" << qgetenv("PATH");
    qDebug() << "PYTHONPATH:" << qgetenv("PYTHONPATH");
    qDebug() << "LD_LIBRARY_PATH:" << qgetenv("LD_LIBRARY_PATH");

    // enable high DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
