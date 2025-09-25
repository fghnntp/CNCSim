 
#include "NCPlotter.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include "QDebug"
#include <QMatrix4x4>
#include <QApplication>
#include <math.h>
#include <QHBoxLayout>
#include <QStyle>
#include <QLabel>


static inline QPointF worldToScreen(
    const QVector3D& p_world,
    const QMatrix4x4& mvp,        // = projection * view * model
    int vpW, int vpH, float dpr)  // 物理像素（注意用 devicePixelRatioF）
{
    QVector4D clip = mvp * QVector4D(p_world, 1.0f);
    if (clip.w() == 0.0f) return QPointF(1e9,1e9);
    QVector3D ndc = clip.toVector3DAffine();         // [-1,1]
    // NDC -> 屏幕（物理像素）
    float x = (ndc.x() * 0.5f + 0.5f) * (vpW * dpr);
    float y = (1.0f - (ndc.y() * 0.5f + 0.5f)) * (vpH * dpr); // y 轴向下
    return QPointF(x, y);
}

static inline float distPtToSegPx(
    const QPointF& p, const QPointF& a, const QPointF& b, float* tOut=nullptr)
{
    // 返回 p 到线段 ab 的最短距离（像素），并可返回 0..1 上的参数 t
    const QPointF ab = b - a;
    const double L2 = ab.x()*ab.x() + ab.y()*ab.y();
    if (L2 <= 1e-9) { if (tOut) *tOut = 0.f; return std::hypot(p.x()-a.x(), p.y()-a.y()); }
    double t = ((p.x()-a.x())*ab.x() + (p.y()-a.y())*ab.y()) / L2;
    t = std::clamp(t, 0.0, 1.0);
    if (tOut) *tOut = float(t);
    const QPointF q = a + t*ab;
    return std::hypot(p.x()-q.x(), p.y()-q.y());
}


// Initialize camera in constructor
NCPlotter::NCPlotter(QWidget* parent) : QOpenGLWidget(parent){

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

    setContextMenuPolicy(Qt::NoContextMenu);
    setFocusPolicy(Qt::StrongFocus);     // 允许获得键盘焦点
}

void NCPlotter::initializeGL() {
        qDebug() << "GL version" << context()->format().majorVersion()
        << context()->format().minorVersion()
        << "profile" << context()->format().profile()
        << "samples" << context()->format().samples();

        initializeOpenGLFunctions();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);

        m_renderer.init();

        createHud();
        positionHud();
}

void NCPlotter::resizeGL(int w, int h) {
    m_renderer.setViewportPx(w, h, devicePixelRatioF());
    glViewport(0, 0, w, h);
}


QMatrix4x4 NCPlotter::getViewMatrix() const {
                       // 本例没有模型矩阵
    QMatrix4x4 view;
    view.lookAt(m_cameraPosition, m_cameraTarget, QVector3D(0, 1, 0));

    // Only apply rotation in unfixed view mode
    if (!m_fixedView) {
        view.rotate(m_cameraRotation);
    }

    return view;
}

QMatrix4x4 NCPlotter::getProjectionMatrix() const {
    QMatrix4x4 projection;
    float aspect = float(width()) / float(height());

    // 计算路径的包围盒大小
    float size = calculatePathBoundingSize();

    // 动态设置近远裁剪平面
    float nearPlane = qMax(0.001f, size * 0.001f);
    float farPlane = qMax(100.0f, size * 10.0f);

    projection.perspective(m_fieldOfView, aspect, nearPlane, farPlane);
    return projection;
}

float NCPlotter::calculatePathBoundingSize() const {
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

void NCPlotter::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 每帧根据当前相机状态重算
    const QMatrix4x4 projection = getProjectionMatrix();
    const QMatrix4x4 view       = getViewMatrix();
    const QMatrix4x4 vp         = projection * view;

    m_renderer.setMVP(vp);

    // 用现代管线画刀路与局部坐标轴（见下面两个函数）
    drawToolPath(vp);
    if (m_showLocalAxes) drawLocalAxes(vp);


    if (m_showMiniAxes) {
        drawMiniAxesModern();
    }

    drawMsgHudPanel();
}

void NCPlotter::mousePressEvent(QMouseEvent *event) {
    m_lastMousePos = event->pos();
    m_lastMousePosF = event->pos();

    if (event->button() == Qt::LeftButton) {
        m_leftMousePressed = true;
    }
    else if (event->button() == Qt::RightButton) {
        m_rightMousePressed = true;
    }
    else if (event->button() == Qt::MiddleButton) {
        m_middleMousePressed = true;
    }
}

void NCPlotter::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_leftMousePressed = false;
        if (m_rotPath) {
            m_rotPath = false;
        }
        else {
            // —— 点选逻辑 ——
            const float dpr = devicePixelRatioF();
            const int vpW = width(), vpH = height();

            // 每帧都在 paintGL 里算 vp；此处重算一次即可
            const QMatrix4x4 vp = getProjectionMatrix() * getViewMatrix();
            const QMatrix4x4 mvp = vp * pathModelMatrix();

            // 鼠标屏幕坐标（物理像素）
            QPointF mousePx = QPointF(event->pos().x()*dpr, event->pos().y()*dpr);

            int bestIdx = -1;
            float bestD = 1e9f;
            for (int i=0; i<toolPath.size(); ++i) {
                QPointF sp = worldToScreen(toolPath[i], mvp, vpW, vpH, dpr);
                float d = std::hypot(mousePx.x()-sp.x(), mousePx.y()-sp.y());
                if (d < bestD) { bestD = d; bestIdx = i; }
            }

            if (bestIdx >= 0 && bestD <= m_pickRadiusPx) {
                if (m_shiftDown) {
                    m_selectedPointShift = bestIdx;
                }
                else {
                    m_selectedPoint = bestIdx;
                }

                if (ncMoves.length() > 0) {
                    emit updateNCLine(ncMoves[path2Move[bestIdx]].lineNum);

                    qDebug() << ncMoves[path2Move[bestIdx]].lineNum;
                }
            }
            else {
                m_selectedPointShift = -1;
            }
            update();
        }
    }
    else if (event->button() == Qt::RightButton) {
        m_rightMousePressed = false;
        if (m_translatingPath) {
            m_translatingPath = false;
        }
        else {
            if (!m_ctxMenu) buildContextMenu();
            m_ctxMenu->exec(event->globalPos());
        }

    }
    else if (event->button() == Qt::MiddleButton) {
        m_middleMousePressed = false;
    }
}

void NCPlotter::mouseMoveEvent(QMouseEvent *event) {
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
            m_rotPath = true;
        }
        else if (m_rightMousePressed) {
            // 1) 浮点位移 + 高 DPI
            QPointF rawDelta = event->pos() - m_lastMousePosF;
            m_lastMousePosF = event->pos();
            const qreal dpr = devicePixelRatioF();
            QPointF deltaPx = rawDelta * dpr;

            // // 2) （可选）低通滤波，抑制抖动（0~1，越小越稳）
            // static QPointF v(0,0);
            // const qreal alpha = 0.30;
            // v = alpha * deltaPx + (1.0 - alpha) * v;
            // deltaPx = v;

            // 3) 稳定的相机基
            QVector3D forward = (m_cameraTarget - m_cameraPosition);
            if (forward.lengthSquared() < 1e-12f) return;
            forward.normalize();

            QVector3D worldUp(0,1,0);
            if (std::abs(QVector3D::dotProduct(forward, worldUp)) > 0.98f) {
                worldUp = QVector3D(0,0,1); // 备用 up，避免 forward≈up 的奇异
            }
            QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
            QVector3D up    = QVector3D::crossProduct(right, forward).normalized();

            // 4) 每像素→世界单位的比例（透视/正交分别处理）
            float unitsPerPixel = 1.0f;
            // if (m_projMode == ProjectionMode::Perspective) {
                float d   = (m_cameraPosition - m_cameraTarget).length(); // 或你的 m_zoomDistance
                float fov = qDegreesToRadians(m_fieldOfView);
                float H   = float(std::max(1, height()));
                // 竖直方向上：屏幕 1 像素对应的世界高度
                unitsPerPixel = 2.f * d * std::tan(fov * 0.5f) / H;
            // } else { // Orthographic
            //     // 假设你用 ortho(-halfW,halfW,-halfH,halfH, ...)
            //     float halfH = m_orthoHalfHeight;              // 你自己维护的正交半高
            //     float H     = float(std::max(1, height()));
            //     unitsPerPixel = (2.f * halfH) / H;
            // }

            // 5) 应用“斜向”平移（dx、dy 同时参与）
            QVector3D translation =
                (right * float(deltaPx.x()) -
                 up    * float(deltaPx.y()))
                * unitsPerPixel;

            m_pathTranslation += translation;
            m_translatingPath = true;
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

void NCPlotter::wheelEvent(QWheelEvent *event) {
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

void NCPlotter::buildContextMenu() {
    if (m_ctxMenu) return;

    m_ctxMenu = new QMenu(this);

    // 勾选项：显示本地轴/迷你轴
    m_actShowLocalAxes = new QAction(tr("显示本地轴"), this);
    m_actShowLocalAxes->setCheckable(true);
    m_actShowLocalAxes->setChecked(m_showLocalAxes);
    connect(m_actShowLocalAxes, &QAction::toggled, this, [this](bool on){
        m_showLocalAxes = on; update();
    });

    m_actShowMiniAxes = new QAction(tr("显示迷你轴"), this);
    m_actShowMiniAxes->setCheckable(true);
    m_actShowMiniAxes->setChecked(m_showMiniAxes);
    connect(m_actShowMiniAxes, &QAction::toggled, this, [this](bool on){
        m_showMiniAxes = on; update();
    });

    // 常用动作
    m_actFitToPath = new QAction(tr("自适应显示(Fit)"), this);
    connect(m_actFitToPath, &QAction::triggered, this, [this]{ fitToPath(); });

    m_actResetView = new QAction(tr("复位视图(Reset)"), this);
    connect(m_actResetView, &QAction::triggered, this, [this]{ resetView(); });

    // 视图子菜单
    m_viewMenu  = new QMenu(tr("标准视图"), this);
    m_viewGroup = new QActionGroup(this);
    m_viewGroup->setExclusive(true);

    auto makeView = [&](const QString& text, auto which){
        QAction* a = new QAction(text, this);
        a->setCheckable(true);
        m_viewGroup->addAction(a);
        connect(a, &QAction::triggered, this, [this, which]{
            applyStandardView(which);
            fitToPath();
        });
        m_viewMenu->addAction(a);
        return a;
    };
    m_actViewIso = makeView(tr("ISO 等轴测"), StandardView::Iso);
    m_actViewG17 = makeView(tr("G17 (XY俯视)"), StandardView::G17);
    m_actViewG18 = makeView(tr("G18 (XZ前视)"), StandardView::G18);
    m_actViewG19 = makeView(tr("G19 (YZ侧视)"), StandardView::G19);
    m_actViewIso->setChecked(true);

    // 组装菜单
    m_ctxMenu->addMenu(m_viewMenu);
    m_ctxMenu->addSeparator();
    m_ctxMenu->addAction(m_actShowLocalAxes);
    m_ctxMenu->addAction(m_actShowMiniAxes);
    m_ctxMenu->addSeparator();
    m_ctxMenu->addAction(m_actFitToPath);
    m_ctxMenu->addAction(m_actResetView);
}

void NCPlotter::createHud()
{
    if (m_hud) return;

    m_hud = new QFrame(this);
    m_hud->setFrameShape(QFrame::NoFrame);

    m_hud->setStyleSheet(
        "QFrame{"
        "  background:rgba(20,20,20,200);"      // 更深一点的背景
        "  border-radius:6px;"
        "}"
        "QToolButton{"
        "  padding:4px 8px;"
        "  color:white;"                        // 白色字体
        "  background:rgba(80,80,80,200);"      // 默认按钮深灰
        "  border:1px solid rgba(150,150,150,100);"
        "  border-radius:4px;"
        "}"
        "QToolButton:hover{"
        "  background:rgba(100,100,100,220);"   // hover 更亮
        "}"
        "QToolButton:pressed{"
        "  background:rgba(60,60,60,220);"      // 按下更暗
        "}"
        "QToolButton:checked{"
        "  background:rgba(30,144,255,220);"    // checked 用亮蓝色
        "  border:1px solid rgba(200,200,200,180);"
        "}"
        );


    auto lay = new QHBoxLayout(m_hud);
    lay->setContentsMargins(6,6,6,6);
    lay->setSpacing(4);

    btnIso   = new QToolButton(m_hud); btnIso->setText("ISO");
    btnG17   = new QToolButton(m_hud); btnG17->setText("G17");
    btnG18 = new QToolButton(m_hud); btnG18->setText("G18");
    btnG19  = new QToolButton(m_hud); btnG19->setText("G19");
    btnFit   = new QToolButton(m_hud); btnFit->setText("FIT");

    btnFirst = new QToolButton(m_hud); btnFirst->setText("First");
    btnLast = new QToolButton(m_hud); btnLast->setText("Last");

    lay->addWidget(btnIso);
    lay->addWidget(btnG17);
    lay->addWidget(btnG18);
    lay->addWidget(btnG19);
    lay->addWidget(btnFit);
    lay->addSpacing(6);
    lay->addWidget(btnFirst);
    lay->addWidget(btnLast);
    lay->addSpacing(6);

    // 连接
    connect(btnIso,   &QToolButton::clicked, this, [this] {
        applyStandardView(StandardView::Iso);
        fitToPath();
    });
    connect(btnG17,   &QToolButton::clicked, this, [this]{
        applyStandardView(StandardView::G17);
        fitToPath();
    });
    connect(btnG18, &QToolButton::clicked, this, [this]{
        applyStandardView(StandardView::G18);
        fitToPath();
    });
    connect(btnG19,  &QToolButton::clicked, this, [this]{
        applyStandardView(StandardView::G19);
        fitToPath();
    });

    connect(btnFit,   &QToolButton::clicked, this, [this]{
        fitToPath();
    });

    connect(btnFirst, &QToolButton::clicked, this, [this]{
        if (toolPath.length() <= 0)
            return;

        m_selectedPoint = 0;
        if (ncMoves.length() > 0) {
            emit updateNCLine(ncMoves[path2Move[m_selectedPoint]].lineNum);

            qDebug() << ncMoves[path2Move[m_selectedPoint]].lineNum;
        }
        update();
    });

    connect(btnLast, &QToolButton::clicked, this, [this]{
        if (toolPath.length() <= 0)
            return;

        m_selectedPoint = toolPath.length() - 1;

        if (ncMoves.length() > 0) {
            emit updateNCLine(ncMoves[path2Move[m_selectedPoint]].lineNum);

            qDebug() << ncMoves[path2Move[m_selectedPoint]].lineNum;
        }

        update();
    });



    // 初次定位
    positionHud();
    m_hud->show();
}

void NCPlotter::positionHud() {
    if (!m_hud) return;
    const int margin = 8;
    m_hud->adjustSize();
    m_hud->move(margin, margin);
}

void NCPlotter::applyStandardView(StandardView v) {
    // 计算中心和适当距离
    QVector3D center = calculatePathCenter();
    float size = qMax(1.0f, calculatePathBoundingSize());
    float dist = size * 2.0f;  // 可调

    if (m_fixedView) {
        // 以“动世界”为准：给路径设置一个绝对旋转
        // 先重置
        m_pathRotation = QQuaternion();
        switch (v) {
        case StandardView::Iso://ISO
            // 朝向 (1,1,1) 的等轴测：把模型旋到这个方向
            m_pathRotation = QQuaternion::fromAxisAndAngle(QVector3D(0,1,0), -45) *
                             QQuaternion::fromAxisAndAngle(QVector3D(1,0,0),  35.2643897f); // atan(sqrt(2))
            break;
        case StandardView::G17:   // 俯视，即 +Z 朝向相机 G17/XY
            // 不需要额外旋转：Z朝上、相机在 +Z
            m_pathRotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), 0);
            break;
        case StandardView::G18: // 前视：+Y 朝向相机 G18/XZ
            m_pathRotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), -90);
            break;
        case StandardView::G19:  // 左视：+X 朝向相机 G19/YZ
            m_pathRotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), -90) *
                QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), -90);
            break;
        }
        // 相机放在合适位置看向中心（固定视角时你用 lookAt，且不应用 cameraRotation）
        m_cameraTarget = center;
        if (v == StandardView::Iso)
            m_cameraPosition = center + QVector3D(0,0,1).normalized() * dist;
        else if (v == StandardView::G17)
            m_cameraPosition = center + QVector3D(0,0,1) * dist;
        else if (v == StandardView::G18)
            m_cameraPosition = center + QVector3D(0,0,1) * dist;
        else if (v == StandardView::G19)
            m_cameraPosition = center + QVector3D(0,0,1) * dist;

        m_zoomDistance = dist;
    } else {
        // 动相机：重置路径旋转，调整相机姿态
        m_pathRotation = QQuaternion();

        m_cameraTarget = center;
        m_cameraRotation = QQuaternion(); // 先清空旋转

        QVector3D pos;
        switch (v) {
        case StandardView::Iso:
            pos = center + QVector3D(1,1,1).normalized() * dist;
            break;
        case StandardView::G17:
            pos = center + QVector3D(0,0,1) * dist;
            break;
        case StandardView::G18:
            pos = center + QVector3D(0,1,0) * dist;
            break;
        case StandardView::G19:
            pos = center + QVector3D(1,0,0) * dist;
            break;
        }
        m_cameraPosition = pos;
        m_zoomDistance = dist;
    }

    update();
}

void NCPlotter::fitToPath() {
    if (toolPath.isEmpty()) return;
    QVector3D center = calculatePathCenter();
    float size = qMax(1.0f, calculatePathBoundingSize());
    m_cameraTarget = center;
    m_zoomDistance = size * 1.8f;

    // 保持当前朝向，仅调整距离与位置
    QVector3D forward = (m_cameraTarget - m_cameraPosition).normalized();
    if (forward.lengthSquared() < 1e-6f)
        forward = QVector3D(0,0,1);  // 兜底
    m_cameraPosition = m_cameraTarget - forward * m_zoomDistance;

    update();
}

void NCPlotter::drawLocalAxes(const QMatrix4x4& vp) {
    if (toolPath.empty()) return;

    const QVector3D c = calculatePathCenter();
    const float size = qMax(1e-3f, calculatePathBoundingSize());
    const float L = qMax(0.01f, size * 0.15f);

    QVector<LineInstance> axes;
    auto add = [&](QVector3D a, QVector3D b, QColor col, float w){
        LineInstance li;
        li.p0=a; li.p1=b;
        li.color= QVector4D(col.redF(),col.greenF(),col.blueF(),1.0f);
        li.widthPx=w; li.patId=0.0f;
        axes.push_back(li);
    };
    add(c, c+QVector3D(L,0,0), Qt::red,   2.0f);
    add(c, c+QVector3D(0,L,0), Qt::green, 2.0f);
    add(c, c+QVector3D(0,0,L), Qt::blue,  2.0f);

    QMatrix4x4 mvp = vp * pathModelMatrix();
    m_renderer.setInstances(axes);
    m_renderer.setClassicPattern(0);
    m_renderer.setMVP(mvp);
    m_renderer.draw();
}

// 添加路径点
void NCPlotter::addPoint(const QVector3D& point) {
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
void NCPlotter::clearPath() {
    toolPath.clear();
    update();
}

// 设置整个路径
void NCPlotter::setPath(const QVector<QVector3D>& path) {
    ncMoves.clear();
    path2Move.clear();
    if (m_fixedView) {
        toolPath = path;
        calculatePathBoundingBox();
        update();
    } else {
        toolPath = path;
        update();
    }
}

// 清空路径
void NCPlotter::clearMoves() {
    ncMoves.clear();
    path2Move.clear();
    update();
}

void NCPlotter::setMoves(const QVector<NC::Move> &moves)
{
    int moveCnt = 0;
    double totalTime = 0.0;
    if (m_fixedView) {
        ncMoves = moves;
        toolPath.clear();
        path2Move.clear();

        for (auto& mv : ncMoves) {
            auto pts = NC::Parser::toPolyline(mv, /*maxSegLen=*/0.5, /*maxChordErr=*/0.02);
            toolPath.append(pts);
            for (auto p: pts)
                path2Move.append(moveCnt);
            moveCnt++;
            double time = NC::timeSecondsForMove(mv);
            totalTime += time;
            mv.time = time;
            mv.totalTime = totalTime;
        }
        calculatePathBoundingBox();
        update();
    } else {
        ncMoves = moves;
        toolPath.clear();
        path2Move.clear();

        for (const auto& mv : moves) {
            auto pts = NC::Parser::toPolyline(mv, /*maxSegLen=*/0.5, /*maxChordErr=*/0.02);
            toolPath.append(pts);
            path2Move.append(moveCnt++);
        }
        update();
    }
}


// Add these methods for better camera control:
void NCPlotter::resetView() {
    m_cameraPosition = QVector3D(0, 0, 3);
    m_cameraTarget = QVector3D(0, 0, 0);
    m_cameraRotation = QQuaternion();
    update();
}

void NCPlotter::drawToolPath(const QMatrix4x4& vp) {
    if (toolPath.empty()) return;

    QMatrix4x4 mvp = vp * pathModelMatrix();
    m_renderer.setMVP(mvp);

    // 线段
    QVector<LineInstance> segs;
    segs.reserve(qMax(0, toolPath.size()-1));
    const int N = qMax(0, toolPath.size() - 1);
    for (int i=0; i+1<toolPath.size(); ++i) {
        LineInstance li;
        li.p0 = toolPath[i];
        li.p1 = toolPath[i+1];
        if (i < path2Move.length()) {
            float t = (N > 1) ? float(i) / float(N - 1) : 1.0f; // 0→1
            li.color = NC::colorFor(ncMoves[path2Move[i]].type, t);
            li.widthPx = NC::widthFor(ncMoves[path2Move[i]].type);
            li.patId   = NC::patIdFor(ncMoves[path2Move[i]].type);
        }
        else {
            li.color   = QVector4D(0.0f,1.0f,0.5f,1.0f);
            li.widthPx = 2.0f;
            li.patId   = 4.0f;
        }

        segs.push_back(li);
    }
    m_renderer.setInstances(segs);
    m_renderer.setClassicPattern(4);
    m_renderer.draw();



    // 如果有选中点，渲染高亮点
    if (m_selectedPoint >= 0 && m_selectedPoint < toolPath.size()) {
        QVector<QVector3D> selectedPoints{ toolPath[m_selectedPoint] };
        m_renderer.setPoints(selectedPoints);
        m_renderer.drawPoints(12.0f, QVector4D(1.0f, 0.8f, 0.2f, 1.0f));
    }

    if (m_selectedPointShift >= 0 && m_selectedPointShift < toolPath.size()) {
        QVector<QVector3D> selectedPoints{ toolPath[m_selectedPointShift] };
        m_renderer.setPoints(selectedPoints);
        m_renderer.drawPoints(12.0f, QVector4D(1.0f, 0.8f, 0.2f, 1.0f));
    }



    if (!toolPath.isEmpty()) {
        QVector<QVector3D> startPoint{ toolPath.first() };
        QVector<QVector3D> endPoint{ toolPath.last() };

        m_renderer.setPoints(startPoint);
        m_renderer.drawPoints(8.0f, QVector4D(0.0f, 0.3f, 1.0f, 1.0f));

        m_renderer.setPoints(endPoint);
        m_renderer.drawPoints(8.0f, QVector4D(1.0f, 0.3f, 0.3f, 1.0f));
    }
}

void NCPlotter::drawMiniAxesModern()
{
    // 1) 构造一个小视口对应的投影视图（左下角 120*dpr 像素）
    const float dpr = devicePixelRatioF();
    const int miniPx = int(120 * dpr);

    // 2) 构造一个正交投影，用于在 [-1,1]^3 的小坐标系里画
    QMatrix4x4 proj; proj.ortho(-1, 1, -1, 1, -1, 1);
    QMatrix4x4 view; view.setToIdentity();
    QMatrix4x4 model;
    model.rotate(m_pathRotation); // 让迷你轴跟随你的路径旋转
    QMatrix4x4 mvp = proj * view * model;

    // 3) 准备三条轴线
    const float L = 0.6f;
    QVector<LineInstance> axes;
    auto add = [&](QVector3D a, QVector3D b, QVector4D col){
        LineInstance li;
        li.p0=a; li.p1=b;
        li.color= col;
        li.widthPx= 2.0f;
        li.patId= 0.0f; // 实线
        axes.push_back(li);
    };
    add({0,0,0}, {L,0,0}, {1,0,0,1});
    add({0,0,0}, {0,L,0}, {0,1,0,1});
    add({0,0,0}, {0,0,L}, {0,0,1,1});

    // 4) 在小视口绘制
    GLint old[4]; glGetIntegerv(GL_VIEWPORT, old);
    glViewport(0, 0, miniPx, miniPx);
    GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthEnabled) glDisable(GL_DEPTH_TEST);

    m_renderer.setMVP(mvp);
    m_renderer.setInstances(axes);
    m_renderer.setClassicPattern(0);
    m_renderer.draw();

    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    glViewport(old[0], old[1], old[2], old[3]);
}


QVector3D NCPlotter::calculatePathCenter() const {
    if (toolPath.empty()) return QVector3D(0, 0, 0);

    QVector3D sum(0, 0, 0);
    for (const auto& pt : toolPath) {
        sum += pt;
    }
    return sum / toolPath.size();
}

void NCPlotter::calculatePathBoundingBox() {
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

void NCPlotter::drawMsgHudPanel()
{
    MsgHudData d;
    bool measure = false;


    if (m_selectedPoint >= 0 && m_selectedPoint < toolPath.size()) {
        d.toolPos = toolPath[m_selectedPoint];
        if (ncMoves.length() > 0)
            d.move = ncMoves[path2Move[m_selectedPoint]];
        else
            d.move = NC::Move();
    }

    if (m_selectedPointShift >= 0 && m_selectedPointShift < toolPath.size()) {
        d.toolPosShift = toolPath[m_selectedPointShift];
        if (ncMoves.length() > 0)
            d.moveShift = ncMoves[path2Move[m_selectedPointShift]];
        else
            d.moveShift = NC::Move();

        measure = true;
    }


    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    // 字体：标题 / 文本（数字用等宽字体更齐）
    QFont fTitle = p.font(); fTitle.setBold(true); fTitle.setPixelSize(13);
    QFont fText  = p.font(); fText.setPixelSize(13);
    QFont fMono("Consolas"); fMono.setPixelSize(13);            // 数字列

    // 行高/内边距
    const int margin   = 8;
    const int cellH    = 20;
    const int sectionGap = 6;

    // 预排版：左列宽、右列宽

    QStringList leftLabels;
    QStringList rightLabels;
    QStringList values;
    // 值列文本（等宽显示）
    auto num = [&](double v, int fixed=6){ return QString::number(v, 'f', fixed); };
    auto mode2str = [](NC::MoveType &type){
        QString str;
        switch (type) {
        case NC::MoveType::Rapid:
            str = "G0";
            break;
        case NC::MoveType::Linear:
            str = "G1";
            break;
        case NC::MoveType::ArcCW:
            str = "G2";
            break;
        case NC::MoveType::ArcCCW:
            str = "G3";
            break;
        default:
            str = "G0";
            break;
        }
        return str;
    };

    leftLabels  = QStringList{"Pos", "X", "Y", "Z", "Status","Mode", "Len", "F", "T", "Time", "TTime"};
    rightLabels = QStringList{"mm", "", "", "", "", "", "", "mm/min", "", "s", ""};

    QTime t(0, 0, 0);
    t = t.addSecs(d.move.totalTime);

    values = QStringList{
        "",
        num(d.toolPos.x()),
        num(d.toolPos.y()),
        num(d.toolPos.z()),
        "",
        mode2str(d.move.type),
        num((d.move.end - d.move.start).length()),
        num(d.move.feed, 3),
        num(d.move.toolNum, 0),
        num(d.move.time, 3),
        t.toString("hh:mm:ss")

    };


    if (measure) {
        leftLabels.append("PL");
        rightLabels.append("mm");

        values.append(num((d.toolPosShift - d.toolPos).length()));
    }


    QFontMetrics fmTitle(fTitle), fmText(fText), fmMono(fMono);
    int leftColW  = 0, rightColW = 0;
    for (auto& s : leftLabels)  leftColW  = std::max(leftColW,  fmText.horizontalAdvance(s));
    for (auto& s : rightLabels) rightColW = std::max(rightColW, fmText.horizontalAdvance(s));


    int valueColW = 0;
    for (auto& s : values) valueColW = std::max(valueColW, fmMono.horizontalAdvance(s));

    // 面板尺寸
    int rows = 11;

    if (measure)
        rows += 1;

    int wPanel = margin + leftColW + 16 + valueColW + 8 + rightColW + margin;
    int hPanel = margin + cellH*rows + sectionGap*2 + margin;

    // 放到右上角
    QRect panelRect(width()-wPanel-10, 10, wPanel, hPanel);

    // 背景与描边
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(240,242,246,220));
    p.drawRoundedRect(panelRect, 6, 6);
    p.setPen(QColor(160,170,185,255));
    p.drawRoundedRect(panelRect.adjusted(0,0,-1,-1), 6, 6);

    // 画内容
    int xL = panelRect.left()+margin;
    int xV = xL + leftColW + 16;
    int xR = xV + valueColW + 8;
    int y  = panelRect.top()+margin;

    auto drawRow = [&](const QString& left, const QString& val, const QString& right,
                       bool header=false)
    {
        if (!leftLabels.contains(left))
            return;
        if (header) {
            p.setFont(fTitle);
            p.setPen(QColor(60,70,85));
            p.drawText(QRect(xL, y, leftColW, cellH), Qt::AlignVCenter|Qt::AlignLeft, left);
            p.setFont(fText);
            p.setPen(QColor(90,100,120));
            p.drawText(QRect(xR, y, rightColW, cellH), Qt::AlignVCenter|Qt::AlignLeft, right);
        } else {
            p.setFont(fText);
            p.setPen(QColor(90,100,120));
            p.drawText(QRect(xL, y, leftColW, cellH), Qt::AlignVCenter|Qt::AlignLeft, left);
            p.setFont(fMono);
            p.setPen(QColor(30,30,30));
            p.drawText(QRect(xV, y, valueColW, cellH), Qt::AlignVCenter|Qt::AlignRight, val);
            p.setFont(fText);
            p.setPen(QColor(120,130,145));
            p.drawText(QRect(xR, y, rightColW, cellH), Qt::AlignVCenter|Qt::AlignLeft, right);
        }
        y += cellH;
    };

    // 第一节：Pos
    drawRow("Pos", values[0], rightLabels[0], /*header=*/true);
    drawRow("X", values[1], rightLabels[1]);
    drawRow("Y", values[2], rightLabels[2]);
    drawRow("Z", values[3], rightLabels[3]);
    y += sectionGap;

    // 第二节：Status
    drawRow("Status", values[4], rightLabels[4], /*header=*/true);
    drawRow("Mode", values[5], rightLabels[5]);
    drawRow("Len", values[6], rightLabels[6]);
    drawRow("F", values[7], rightLabels[7]);
    drawRow("T", values[8], rightLabels[8]);
    drawRow("Time", values[9], rightLabels[9]);
    drawRow("TTime", values[10], rightLabels[10]);

    if (measure) {
        y += sectionGap;
        drawRow("PL", values[11], rightLabels[11]);
    }
}
