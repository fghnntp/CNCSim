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

private:
    // 固定视角相关
    bool m_fixedView = true;  // 是否使用固定视角
    QVector3D m_fixedViewDirection = QVector3D(0, 0, -1); // 固定视角方向
    float m_pathRotationAngle = 0.0f; // 路径旋转角度
    QVector3D m_pathRotationAxis = QVector3D(0, 1, 0); // 路径旋转轴(Y轴)

public:
    // 设置固定视角模式
    void setFixedView(bool fixed) {
        if (m_fixedView == fixed) return;

        m_fixedView = fixed;

        if (m_fixedView) {
            // Switching TO fixed view
            QVector3D center = calculatePathCenter();
            float size = calculatePathBoundingSize();

            m_cameraTarget = center;
            m_zoomDistance = size * 2.0f;
            m_cameraPosition = center + QVector3D(1, 1, 1).normalized() * m_zoomDistance;
            m_cameraRotation = QQuaternion(); // Reset rotation
        } else {
            // Switching FROM fixed view
            // Keep current camera position but reset rotation
            m_cameraRotation = QQuaternion();
        }

        update();
    }

    // 设置路径旋转角度
    void setPathRotation(float angle, const QVector3D& axis = QVector3D(0, 1, 0)) {
        m_pathRotationAngle = angle;
        m_pathRotationAxis = axis.normalized();
        update();
    }

    // 获取当前路径旋转角度
    float pathRotationAngle() const { return m_pathRotationAngle; }

    // 获取当前路径旋转轴
    QVector3D pathRotationAxis() const { return m_pathRotationAxis; }

    void calculatePathBoundingBox();
private:
    // 旋转控制相关
    QQuaternion m_pathRotation;    // 使用四元数表示路径的当前旋转
    bool m_rotatingPath = false;   // 是否正在旋转路径
public:
    // 设置路径旋转(使用四元数)
    void setPathRotation(const QQuaternion& rotation) {
        m_pathRotation = rotation.normalized();
        update();
    }

    // 获取当前路径旋转
    QQuaternion pathRotation() const {
        return m_pathRotation;
    }

    // 绕特定轴旋转一定角度
    void rotatePath(float angle, const QVector3D& axis) {
        QQuaternion rot = QQuaternion::fromAxisAndAngle(axis.normalized(), angle);
        m_pathRotation = (rot * m_pathRotation).normalized();
        update();
    }
    float calculatePathBoundingSize() const;
    QVector3D calculatePathCenter() const;
private:
    // 平移控制相关
    bool m_translatingPath = false;    // 是否正在平移路径
    QVector3D m_pathTranslation;       // 当前路径平移量
public:
    // 设置路径平移量
    void setPathTranslation(const QVector3D& translation) {
        m_pathTranslation = translation;
        update();
    }

    // 获取当前路径平移量
    QVector3D pathTranslation() const {
        return m_pathTranslation;
    }

    // 重置平移
    void resetPathTranslation() {
        m_pathTranslation = QVector3D(0,0,0);
        update();
    }
//Something can do
//1.Give some defaut viewscese distance dirction for fix and unfixed view
//2.Give some diffent point view look, which means give different render ways
};
#endif
