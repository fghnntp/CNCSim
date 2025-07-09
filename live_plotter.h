#ifndef _LIVE_PLOTTER_H_
#define _LIVE_PLOTTER_H_

#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <QVector2D>
#include <vector>
#include <QMouseEvent>
#include <QQuaternion>

class LivePlotter : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LivePlotter(QWidget* parent = nullptr);
    ~LivePlotter() = default;

    void addPoint(const QVector3D &point);
    void clearPath();
    void setPath(const QVector<QVector3D> &path);
    void resetView();
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // Camera control
    QVector3D m_cameraPosition;
    QVector3D m_cameraTarget;
    QQuaternion m_cameraRotation;
    float m_zoomDistance;
    float m_fieldOfView = 45.0f;

    // Mouse control
    QPoint m_lastMousePos;
    bool m_leftMousePressed = false;
    bool m_rightMousePressed = false;
    bool m_middleMousePressed = false;

    // Data
    QVector<QVector3D> toolPath;

    // Rendering
    void drawToolPath();
    void drawCoordinateAxes();
    void renderText3D(float x, float y, float z, const QString &text, const QColor &color);
    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getProjectionMatrix() const;
};
#endif
