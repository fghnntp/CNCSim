#include "live_plotter.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include "QDebug"
#include <QMatrix4x4>
#include <QApplication>
#include <math.h>

// Initialize camera in constructor
LivePlotter::LivePlotter(QWidget* parent) : QOpenGLWidget(parent) {

    if (m_fixedView) {
        // 固定视角设置
        m_cameraPosition = QVector3D(0, 5, 5); // 固定相机位置
        m_cameraTarget = QVector3D(0, 0, 0);   // 固定看向原点
        m_zoomDistance = 5.0f;
        m_fieldOfView = 45.0f;
    }
    else {
        // Initialize camera to view a 2x2x2 area
        m_cameraPosition = QVector3D(0, 0, 3);
        m_cameraTarget = QVector3D(0, 0, 0);
        m_zoomDistance = 3.0f;
        m_cameraRotation = QQuaternion();
    }
}

void LivePlotter::initializeGL() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
}

void LivePlotter::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}


QMatrix4x4 LivePlotter::getViewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(m_cameraPosition, m_cameraTarget, QVector3D(0, 1, 0));

    // Only apply rotation in unfixed view mode
    if (!m_fixedView) {
        view.rotate(m_cameraRotation);
    }

    return view;
}

QMatrix4x4 LivePlotter::getProjectionMatrix() const {
    QMatrix4x4 projection;
    float aspect = float(width()) / float(height());

    // 计算路径的包围盒大小
    float size = calculatePathBoundingSize();

    // 动态设置近远裁剪平面
    float nearPlane = qMax(0.1f, size * 0.1f);
    float farPlane = qMax(100.0f, size * 10.0f);

    projection.perspective(m_fieldOfView, aspect, nearPlane, farPlane);
    return projection;
}

float LivePlotter::calculatePathBoundingSize() const {
    if (toolPath.empty()) return 1.0f;

    QVector3D min = toolPath.first();
    QVector3D max = toolPath.first();

    for (const auto& pt : toolPath) {
        min.setX(qMin(min.x(), pt.x()));
        min.setY(qMin(min.y(), pt.y()));
        min.setZ(qMin(min.z(), pt.z()));
        max.setX(qMax(max.x(), pt.x()));
        max.setY(qMax(max.y(), pt.y()));
        max.setZ(qMax(max.z(), pt.z()));
    }

    return (max - min).length();
}

void LivePlotter::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up matrices
    QMatrix4x4 projection = getProjectionMatrix();
    QMatrix4x4 view = getViewMatrix();

    // For modern OpenGL, we'd use shaders here
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.constData());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(view.constData());

    // 绘制顺序很重要
    drawCoordinateAxes();   // 然后绘制坐标轴和刻度
    drawToolPath();         // 绘制工具路径
}

void LivePlotter::mousePressEvent(QMouseEvent *event) {
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton)
        m_leftMousePressed = true;
    else if (event->button() == Qt::RightButton)
        m_rightMousePressed = true;
    else if (event->button() == Qt::MiddleButton)
        m_middleMousePressed = true;

    // Use Ctrl to toggle view mode
    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        setFixedView(!m_fixedView);
    }
}

void LivePlotter::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        m_leftMousePressed = false;
    else if (event->button() == Qt::RightButton)
        m_rightMousePressed = false;
    else if (event->button() == Qt::MiddleButton)
        m_middleMousePressed = false;

//    m_fixedView = true;
}

void LivePlotter::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    const float rotationSensitivity = 0.5f;
    const float translationSensitivity = 0.005f; // Separate sensitivity for translation

    if (m_fixedView) {
        if (m_leftMousePressed) {
             // Rotation logic (unchanged)
            QVector3D rotAxis = QVector3D(delta.y(), delta.x(), 0.0f).normalized();
            float rotAngle = sqrt(delta.x()*delta.x() + delta.y()*delta.y()) * rotationSensitivity;
            QQuaternion newRot = QQuaternion::fromAxisAndAngle(rotAxis, rotAngle);
            m_pathRotation = (newRot * m_pathRotation).normalized();
        }
        else if (m_rightMousePressed) {
            // 限制方向的平移逻辑
            float panSpeed = m_zoomDistance * translationSensitivity;

            // 获取相机的右方向和上方向
            QVector3D forward = (m_cameraTarget - m_cameraPosition).normalized();
            QVector3D right = QVector3D::crossProduct(QVector3D(0,1,0), forward).normalized();
            QVector3D up = QVector3D::crossProduct(forward, right).normalized();

            // 计算主要移动方向（取绝对值较大的分量）
            bool xDominant = abs(delta.x()) > abs(delta.y());

            // 根据主导方向应用平移
            QVector3D translation(0, 0, 0);
            if (xDominant) {
                translation = -right * delta.x() * panSpeed; // 仅水平移动
            } else {
                translation = up * delta.y() * panSpeed;     // 仅垂直移动
            }

            m_pathTranslation += translation;
        }

        update();
    }
    else {
        if (m_leftMousePressed) {
            // Rotation
            QQuaternion rotX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -delta.y() * rotationSensitivity);
            QQuaternion rotY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -delta.x() * rotationSensitivity);
            m_cameraRotation = rotX * rotY * m_cameraRotation;
        }
        else if (m_rightMousePressed) {
            // Panning
            float panSpeed = m_zoomDistance * 0.001f;
            QVector3D right =  m_cameraRotation.rotatedVector(QVector3D(1, 0, 0));
            QVector3D up = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));

            m_cameraTarget -= right * delta.x() * panSpeed;
            m_cameraTarget += up * delta.y() * panSpeed;
        }
        else if (m_middleMousePressed) {
            //Free movement (combines rotation and panning)
            QQuaternion rotX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -delta.y() * rotationSensitivity);
            QQuaternion rotY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -delta.x() * rotationSensitivity);
            m_cameraRotation = rotX * rotY * m_cameraRotation;

            float panSpeed = m_zoomDistance * 0.001f;
            QVector3D right =  m_cameraRotation.rotatedVector(QVector3D(1, 0, 0));
            QVector3D up = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));

            m_cameraTarget -= right * delta.x() * panSpeed;
            m_cameraTarget += up * delta.y() * panSpeed;
        }

        update();
    }
}

void LivePlotter::wheelEvent(QWheelEvent *event) {
    // Zoom speed adjustment
    const float zoomSpeed = 0.001f;
    float zoomFactor = 1.0f + zoomSpeed * abs(event->angleDelta().y());

    // Determine zoom direction (inverted for natural scrolling)
    if (event->angleDelta().y() > 0) {
        // Zoom in
        m_zoomDistance /= zoomFactor;
    } else {
        // Zoom out
        m_zoomDistance *= zoomFactor;
    }

    // Clamp zoom distance to reasonable limits
    m_zoomDistance = qBound(0.001f, m_zoomDistance, 500.0f);

    // Calculate proper forward vector (camera to target)
    QVector3D forward = (m_cameraTarget - m_cameraPosition).normalized();

    // Update camera position along the view direction
    m_cameraPosition = m_cameraTarget - forward * m_zoomDistance;

    update();
}

void LivePlotter::drawCoordinateAxes() {
    glLineWidth(2.0f);

    glBegin(GL_LINES);
    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);

    // Y axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);

    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();

    // Draw axis labels at the end of each axis
    renderText3D(1.1f, 0.0f, 0.0f, "X", Qt::red);
    renderText3D(0.0f, 1.1f, 0.0f, "Y", Qt::green);
    renderText3D(0.0f, 0.0f, 1.1f, "Z", Qt::blue);
}

void LivePlotter::renderText3D(float x, float y, float z, const QString &text, const QColor &color) {
    // Save current OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Get current matrices
    QMatrix4x4 projection;
    glGetFloatv(GL_PROJECTION_MATRIX, projection.data());
    QMatrix4x4 modelview;
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview.data());

    // Calculate text position in screen coordinates
    QVector3D pos = QVector3D(x, y, z);
    QVector3D screenPos = projection * modelview * pos;

    // Convert to normalized device coordinates
    screenPos.setX(screenPos.x() / screenPos.z());
    screenPos.setY(screenPos.y() / screenPos.z());

    // Convert to window coordinates
    QPoint windowPos;
    windowPos.setX((screenPos.x() + 1.0f) * width() / 2.0f);
    windowPos.setY((1.0f - screenPos.y()) * height() / 2.0f);

    // Set up 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width(), height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Render text with QPainter
    QPainter painter(this);
    painter.setPen(color);
    painter.setFont(QFont("Arial", 12, QFont::Bold));
    painter.drawText(windowPos, text);
    painter.end();

    // Restore OpenGL state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

// 添加路径点
void LivePlotter::addPoint(const QVector3D& point) {
    if (m_fixedView) {
        toolPath.append(point);
        calculatePathBoundingBox();
        update();
    } else {
        toolPath.append(point);

        // Auto-scale view to fit points
        if (toolPath.size() == 1) {
            // First point - reset view
            m_cameraTarget = point;
            m_zoomDistance = 3.0f;
        }
        update();
    }

}

// 清空路径
void LivePlotter::clearPath() {
    toolPath.clear();
    update();
}

// 设置整个路径
void LivePlotter::setPath(const QVector<QVector3D>& path) {
    if (m_fixedView) {
        toolPath = path;
        calculatePathBoundingBox();
        update();
    } else {
        toolPath = path;
        update();
    }
}

// Add these methods for better camera control:
void LivePlotter::resetView() {
    m_cameraPosition = QVector3D(0, 0, 3);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraRotation = QQuaternion();
    update();
}

void LivePlotter::drawToolPath() {
    if (toolPath.empty()) return;

    // Save current matrix state
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Apply transformations in correct order:
    // 1. First apply translation
    glTranslatef(m_pathTranslation.x(), m_pathTranslation.y(), m_pathTranslation.z());

    // 2. Then apply rotation (using quaternion)
    if (!m_pathRotation.isIdentity()) {
        QMatrix4x4 rotMatrix;
        rotMatrix.rotate(m_pathRotation);
        glMultMatrixf(rotMatrix.constData());
    }

    // Draw path lines
    glColor3f(0.0f, 1.0f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (const auto& pt : toolPath) {
        glVertex3f(pt.x(), pt.y(), pt.z());
    }
    glEnd();

    // Draw path points
    glColor3f(1.0f, 0.5f, 0.0f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (const auto& pt : toolPath) {
        glVertex3f(pt.x(), pt.y(), pt.z());
    }
    glEnd();

    // Highlight last point
    if (!toolPath.empty()) {
        const auto& last = toolPath.last();
        glColor3f(1.0f, 0.0f, 0.0f);
        glPointSize(10.0f);
        glBegin(GL_POINTS);
        glVertex3f(last.x(), last.y(), last.z());
        glEnd();
    }

    // Restore matrix state
    glPopMatrix();
}


QVector3D LivePlotter::calculatePathCenter() const {
    if (toolPath.empty()) return QVector3D(0, 0, 0);

    QVector3D sum(0, 0, 0);
    for (const auto& pt : toolPath) {
        sum += pt;
    }
    return sum / toolPath.size();
}

void LivePlotter::calculatePathBoundingBox() {
    if (toolPath.empty()) return;

    QVector3D center = calculatePathCenter();
    float size = calculatePathBoundingSize();

    // 调整相机位置
    m_cameraTarget = center;
    m_zoomDistance = size * 2.0f; // 增加距离系数

    if (m_fixedView) {
        m_cameraPosition = center + QVector3D(0, 1, 1).normalized() * m_zoomDistance;
    } else {
        m_cameraPosition = center + QVector3D(0, 0, 1) * m_zoomDistance;
    }
}

