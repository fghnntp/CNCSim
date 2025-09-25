// DashedLineRenderer.cpp
#include "DashedLineRenderer.h"
#include <QFile>
#include <QTextStream>
#include <QOpenGLFunctions>

static QString loadText(const char* path) {
    QFile f(path);
    f.open(QIODevice::ReadOnly|QIODevice::Text);
    return QString::fromUtf8(f.readAll());
}

void DashedLineRenderer::init() {
    initializeOpenGLFunctions();
    glEnable(GL_PROGRAM_POINT_SIZE);   // 让 gl_PointSize 生效
    ensureProgram();
    ensurePointProgram();              // 可提前编译，也可按需
}

void DashedLineRenderer::ensureProgram() {
    if (m_prog.isLinked()) return;
    m_prog.create();
    m_prog.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/dashed_lines.vert");
    m_prog.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/dashed_lines.frag");
    if (!m_prog.link()) qFatal("Shader link failed: %s", qPrintable(m_prog.log()));

    m_vao.create(); m_vao.bind();
    m_vbo.create(); m_vbo.bind();
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    // 顶点属性：我们用 gl_VertexID 生成四个角点，所以这里只有实例属性
    // layout 0: aP0 (vec3)
    // layout 1: aP1 (vec3)
    // layout 2: aColor (vec4)
    // layout 3: aWidth (float)
    // layout 4: aPatId (float)
    const int stride = sizeof(LineInstance);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(LineInstance, p0));
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(LineInstance, p1));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(LineInstance, color));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(LineInstance, widthPx));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(LineInstance, patId));
    glVertexAttribDivisor(4, 1);

    m_vao.release(); m_vbo.release();
}

void DashedLineRenderer::ensurePointProgram()
{
    if (m_pointProg.isLinked())
        return;

    m_pointProg.create();

    const char* vs = R"(#version 330 core
        layout(location=0) in vec3 aPos;
        uniform mat4 uMVP;
        uniform float uSizePx;
        void main(){
            gl_Position = uMVP * vec4(aPos,1.0);
            gl_PointSize = uSizePx;
        }
    )";
    const char* fs = R"(#version 330 core
        out vec4 FragColor;
        uniform vec4 uColor;
        void main(){
            FragColor = uColor;
        }
    )";

    if (!m_pointProg.addShaderFromSourceCode(QOpenGLShader::Vertex, vs)) {
        qFatal("Vertex compile failed: %s", qPrintable(m_pointProg.log()));
        return;
    }
    if (!m_pointProg.addShaderFromSourceCode(QOpenGLShader::Fragment, fs)) {
        qFatal("Fragment compile failed: %s", qPrintable(m_pointProg.log()));
        return;
    }
    if (!m_pointProg.link()) {
        qFatal("Program link failed: %s", qPrintable(m_pointProg.log()));
        return;
    }

    // VAO/VBO
    m_pointVao.create(); m_pointVao.bind();
    m_pointVbo.create(); m_pointVbo.bind();
    m_pointVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    m_pointVbo.release(); m_pointVao.release();
}



void DashedLineRenderer::setInstances(const QVector<LineInstance>& lines) {
    m_count = lines.size();
    if (m_count == 0) return;
    m_vao.bind(); m_vbo.bind();
    m_vbo.allocate(lines.constData(), m_count * sizeof(LineInstance));
    m_vbo.release(); m_vao.release();
}

void DashedLineRenderer::setViewportPx(int w, int h, float dpr) {
    m_viewPx = QVector2D(float(w)*dpr, float(h)*dpr);
    m_dpr = dpr;
}

void DashedLineRenderer::setMVP(const QMatrix4x4& mvp) { m_mvp = mvp; }

void DashedLineRenderer::setClassicPattern(int patId) {
    // 把经典图案填到 uSegPx/uSegCount/uPeriod/uPhase（像素单位）
    // 你也可以暴露一个自定义接口来自行配置
    float seg[6]; int cnt = 0;
    auto setUniforms = [&](float* s, int n){
        m_prog.bind();
        m_prog.setUniformValue("uSegCount", n);
        m_prog.setUniformValue("uPeriod", 0.0f);
        float period = 0.f;
        for (int i=0;i<6;i++) seg[i] = (i<n)? s[i] : 0.f;
        m_prog.setUniformValueArray("uSegPx", seg, 6, 1);
        for (int i=0;i<n;i++) period += s[i];
        m_prog.setUniformValue("uPeriod", period);
        m_prog.setUniformValue("uPhase", 0.0f);
        m_prog.release();
    };

    switch (patId) {
        case 0: { float s[]={1e6}; setUniforms(s,1); } break; // Solid（超长“实段”）
        case 1: { float s[]={8,4}; setUniforms(s,2); } break; // Dash
        case 2: { float s[]={1,5}; setUniforms(s,2); } break; // Dot
        case 3: { float s[]={8,4,1,4}; setUniforms(s,4); } break; // Dash-Dot
        case 4: { float s[]={8,4,1,2,1,4}; setUniforms(s,6); } break; // Dash-Dot-Dot
        default:{ float s[]={8,4}; setUniforms(s,2); } break;
    }
}

void DashedLineRenderer::draw() {
    if (m_count <= 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 在 QSurfaceFormat 开启样本后生效

    m_prog.bind();
    m_prog.setUniformValue("uMVP", m_mvp);
    m_prog.setUniformValue("uViewportPx", m_viewPx);

    m_vao.bind();
    // 每个实例 4 个顶点（一个矩形），共 m_count 个实例
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_count);
    m_vao.release();
    m_prog.release();
}

void DashedLineRenderer::setPoints(const QVector<QVector3D>& pts) {
    m_pointCount = pts.size();
    if (m_pointCount <= 0) return;
    m_pointVao.bind(); m_pointVbo.bind();
    m_pointVbo.allocate(pts.constData(), m_pointCount * int(sizeof(QVector3D)));
    m_pointVbo.release(); m_pointVao.release();
}

void DashedLineRenderer::drawPoints(float sizePx, const QVector4D& color) {
    if (m_pointCount <= 0) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE); // 在 QSurfaceFormat 开启样本后生效
    glEnable(GL_PROGRAM_POINT_SIZE);

    bool binded = false;

   binded =  m_pointProg.bind();
    m_pointProg.setUniformValue("uMVP", m_mvp);
    m_pointProg.setUniformValue("uSizePx", sizePx);
    m_pointProg.setUniformValue("uColor", color);

    m_pointVao.bind();
    glDrawArrays(GL_POINTS, 0, m_pointCount);
    m_pointVao.release();
    m_pointProg.release();
}
