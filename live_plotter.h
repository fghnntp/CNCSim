#ifndef _LIVE_PLOTTER_H_
#define _LIVE_PLOTTER_H_

#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <vector>

class LivePlotter : public QOpenGLWidget {
    Q_OBJECT
public:
    LivePlotter(QWidget* parent = nullptr);
    ~LivePlotter() = default;

    // Call this to update the plotter with simulation data
    void addPoint(const QVector3D& point);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private slots:
    void updatePlot();

private:
    std::vector<QVector3D> toolPath;
    QTimer timer;
};
#endif
