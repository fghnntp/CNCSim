#include "live_plotter.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include "QDebug"
#include <QMatrix4x4>

// Initialize camera in constructor
LivePlotter::LivePlotter(QWidget* parent) : QOpenGLWidget(parent) {

    // Initialize camera to view a 2x2x2 area
    m_cameraPosition = QVector3D(0, 0, 3);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_zoomDistance = 3.0f;
    m_cameraRotation = QQuaternion();
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
    view.rotate(m_cameraRotation);
    return view;
}

QMatrix4x4 LivePlotter::getProjectionMatrix() const {
    QMatrix4x4 projection;
    float aspect = float(width()) / float(height());
    projection.perspective(m_fieldOfView, aspect, 0.1f, 100.0f);
    return projection;
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

    // Draw content
    drawCoordinateAxes();
    drawToolPath();
}

void LivePlotter::mousePressEvent(QMouseEvent *event) {
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton)
        m_leftMousePressed = true;
    else if (event->button() == Qt::RightButton)
        m_rightMousePressed = true;
    else if (event->button() == Qt::MiddleButton)
        m_middleMousePressed = true;
}

void LivePlotter::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        m_leftMousePressed = false;
    else if (event->button() == Qt::RightButton)
        m_rightMousePressed = false;
    else if (event->button() == Qt::MiddleButton)
        m_middleMousePressed = false;
}

void LivePlotter::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    float sensitivity = 0.5f;

    if (m_leftMousePressed) {
        // Rotation
        QQuaternion rotX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -delta.y() * sensitivity);
        QQuaternion rotY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -delta.x() * sensitivity);
        m_cameraRotation = rotX * rotY * m_cameraRotation;
    }
    else if (m_rightMousePressed) {
        // Panning
        float panSpeed = m_zoomDistance * 0.005f;
        QVector3D right =  m_cameraRotation.rotatedVector(QVector3D(1, 0, 0));
        QVector3D up = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));

        m_cameraTarget -= right * delta.x() * panSpeed;
        m_cameraTarget += up * delta.y() * panSpeed;
    }
    else if (m_middleMousePressed) {
        // Free movement (combines rotation and panning)
        QQuaternion rotX = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -delta.y() * sensitivity);
        QQuaternion rotY = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, -delta.x() * sensitivity);
        m_cameraRotation = rotX * rotY * m_cameraRotation;

        float panSpeed = m_zoomDistance * 0.001f;
        QVector3D right =  m_cameraRotation.rotatedVector(QVector3D(1, 0, 0));
        QVector3D up = m_cameraRotation.rotatedVector(QVector3D(0, 1, 0));

        m_cameraTarget -= right * delta.x() * panSpeed;
        m_cameraTarget += up * delta.y() * panSpeed;
    }

    update();
}

void LivePlotter::wheelEvent(QWheelEvent *event) {
    // Zoom in/out
    float zoomFactor = 1.1f;
    if (event->angleDelta().y() > 0) {
        m_zoomDistance /= zoomFactor;
    } else {
        m_zoomDistance *= zoomFactor;
    }
    resetView();
    // Update camera position
    QVector3D forward = m_cameraRotation.rotatedVector(QVector3D(0, 0, -1));
    m_cameraPosition = m_cameraTarget - forward * m_zoomDistance;

    update();
}

void LivePlotter::drawToolPath() {
    if (toolPath.empty()) return;

    // Draw path lines
    glColor3f(0.0f, 1.0f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (const auto& pt : toolPath) {
        glVertex3f(pt.x(), pt.y(), pt.z());
    }
    glEnd();

    // Draw points
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
    toolPath.append(point);

    // Auto-scale view to fit points
    if (toolPath.size() == 1) {
        // First point - reset view
        m_cameraTarget = point;
        m_zoomDistance = 3.0f;
    }

    update();
}

// 清空路径
void LivePlotter::clearPath() {
    toolPath.clear();
    update();
}

// 设置整个路径
void LivePlotter::setPath(const QVector<QVector3D>& path) {
    toolPath = path;
    update();
}

// Add these methods for better camera control:
void LivePlotter::resetView() {
    m_cameraPosition = QVector3D(0, 0, 3);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraRotation = QQuaternion();
    update();
}
