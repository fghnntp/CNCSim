#ifndef _NC_PLOTTER_H_
#define _NC_PLOTTER_H_

#include <QOpenGLWidget>
#include <QTimer>
#include <QVector3D>
#include <QVector2D>
#include <QMouseEvent>
#include <QQuaternion>

#include <QFrame>
#include <QToolButton>
#include <QButtonGroup>
#include "DashedLineRenderer.h"
#include "NCPaser.h"

#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QContextMenuEvent>

class NCPlotter : public QOpenGLWidget , protected QOpenGLFunctions_3_3_Core{
    Q_OBJECT
public:
    explicit NCPlotter(QWidget* parent = nullptr);
    ~NCPlotter() = default;

    void addPoint(const QVector3D &point);
    void clearPath();
    void setPath(const QVector<QVector3D> &path);
    void clearMoves();
    void setMoves(const QVector<NC::Move> &moves);
    void resetView();

private:
    QVector<NC::Move> ncMoves;
    QVector<int> path2Move;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;


    void keyPressEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Shift) {
            m_shiftDown = true;
            e->accept();
            return;
        }
        if (e->modifiers() & Qt::ShiftModifier) {
            // Shift 组合其它键
        }
        QOpenGLWidget::keyPressEvent(e);
    }

    void keyReleaseEvent(QKeyEvent* e) override {
        if (e->key() == Qt::Key_Shift) {
            m_shiftDown = false;
            e->accept();
            return;
        }
        QOpenGLWidget::keyReleaseEvent(e);
    }

private:
    bool m_shiftDown = false;

private:
    // 右键菜单
    void buildContextMenu();                // 初始化一次
    QMenu*        m_ctxMenu = nullptr;
    QAction*      m_actShowLocalAxes = nullptr;
    QAction*      m_actShowMiniAxes  = nullptr;
    QAction*      m_actFitToPath     = nullptr;
    QAction*      m_actResetView     = nullptr;

    // 视图子菜单（等轴测/G17/G18/G19）
    QMenu*        m_viewMenu = nullptr;
    QActionGroup* m_viewGroup = nullptr;
    QAction*      m_actViewIso = nullptr;
    QAction*      m_actViewG17 = nullptr;
    QAction*      m_actViewG18 = nullptr;
    QAction*      m_actViewG19 = nullptr;

    // 状态开关
    bool m_showLocalAxes = true;            // ← 新增：是否显示本地轴
    bool m_showMiniAxes  = true;            // ← 你的迷你轴（左下角）

private:
    int   m_selectedPoint = -1;     // 当前选中的顶点索引
    int   m_hoverPoint    = -1;     // 可选：悬停高亮
    float m_pickRadiusPx  = 20.0f;   // 命中阈值（像素）

    int   m_selectedPointShift = -1;     // 当前选中的顶点索引

private:
    // HUD
    QFrame* m_hud = nullptr;
    QToolButton *btnIso=nullptr, *btnG17=nullptr, *btnG18=nullptr, *btnG19=nullptr;
    QToolButton *btnFit=nullptr, *btnFirst=nullptr, *btnLast=nullptr; // Lock=固定视角
    void createHud();
    void positionHud(); // 在左上角摆放
    // 视图切换实现
    enum class StandardView { Iso, G17, G18, G19};
    void applyStandardView(StandardView v);

    // 适配/重置
    void fitToPath();   // 让路径充满视口

    void drawLocalAxes(const QMatrix4x4& vp);


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
    void drawToolPath(const QMatrix4x4& vp);
    void drawMiniAxesModern();

    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getProjectionMatrix() const;

private:
    QPointF m_lastMousePosF;               // 浮点位置
    QPointF m_deltaResidual{0,0};          // 累积残差（亚像素）

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
            // QVector3D center = calculatePathCenter();
            // float size = calculatePathBoundingSize();

            // m_cameraTarget = center;
            // m_zoomDistance = size * 2.0f;
            // m_cameraPosition = center + QVector3D(1, 1, 1).normalized() * m_zoomDistance;
            m_cameraRotation = QQuaternion(); // Reset rotation
        } else {
            // Switching FROM fixed view
            // Keep current camera position but reset rotation
            m_cameraRotation = QQuaternion();
        }

        // update();
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
    bool m_rotPath = false;
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

private:
    DashedLineRenderer m_renderer;

    QMatrix4x4 pathModelMatrix() const {
        const QVector3D c = calculatePathCenter();
        QMatrix4x4 M;
        M.translate(m_pathTranslation);
        M.translate(c);
        if (!m_pathRotation.isIdentity()) M.rotate(m_pathRotation);
        M.translate(-c);
        return M;
    }

private:
    // ——— 数据载体（你也可以直接传值） ———
    struct MsgHudData {
        QVector3D toolPos;   // X,Y,Z
        NC::Move move;
        QVector3D toolPosShift;
        NC::Move moveShift;
    };


    void drawMsgHudPanel(void);

signals:
    void updateNCLine(long long line);
};
#endif
