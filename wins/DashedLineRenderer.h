// DashedLineRenderer.h
#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector4D>

struct LineInstance {
    QVector3D p0, p1;
    QVector4D color;  // RGBA
    float widthPx;    // 线宽（像素）
    float patId;      // 0..4
};

class DashedLineRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    void init();
    void setInstances(const QVector<LineInstance>& lines);
    void setViewportPx(int w, int h, float dpr);
    void setMVP(const QMatrix4x4& mvp);
    void setClassicPattern(int patId);   // 配置 uSegPx/uSegCount 等
    void draw();

    void setPoints(const QVector<QVector3D>& pts);
    void drawPoints(float sizePx, const QVector4D& color);

private:
    QOpenGLShaderProgram m_prog;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo{QOpenGLBuffer::VertexBuffer}; // 实例数据
    int m_count = 0;

    // 点的资源（新增）
    QOpenGLShaderProgram m_pointProg;
    QOpenGLVertexArrayObject m_pointVao;
    QOpenGLBuffer m_pointVbo{QOpenGLBuffer::VertexBuffer};
    int m_pointCount = 0;

    QMatrix4x4 m_mvp;
    QVector2D m_viewPx{1,1};
    float m_dpr = 1.0f;

    void ensureProgram();        // 线的 shader
    void ensurePointProgram();   // 点的 shader（新增）

};
