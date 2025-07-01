#include "live_plotter.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include "QDebug"

LivePlotter::LivePlotter(QWidget* parent) : QOpenGLWidget(parent) {
    connect(&timer, &QTimer::timeout, this, &LivePlotter::updatePlot);
    timer.start(20); // 20 ms (50 Hz refresh)
}

void LivePlotter::addPoint(const QVector3D& point) {
    toolPath.push_back(point);
}

void LivePlotter::initializeGL() {
    glClearColor(0, 0, 0, 1);
}

void LivePlotter::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void LivePlotter::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Simple 2D line plot in X-Y
    glColor3f(0, 1, 0);
    glBegin(GL_LINE_STRIP);
    for (const auto& pt : toolPath) {
        glVertex2f(pt.x(), pt.y());
    }
    glEnd();

    // Draw current position as a red dot
    if (!toolPath.empty()) {
        const auto& last = toolPath.back();
        glColor3f(1, 0, 0);
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2f(last.x(), last.y());
        glEnd();
    }
}

void LivePlotter::updatePlot() {
    update();
}
