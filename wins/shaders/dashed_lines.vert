#version 330 core
layout(location=0) in vec3 aP0;      // 实例：线段起点（世界坐标）
layout(location=1) in vec3 aP1;      // 实例：线段终点（世界坐标）
layout(location=2) in vec4 aColor;   // 实例：颜色 (rgba)
layout(location=3) in float aWidth;  // 实例：线宽（像素）
layout(location=4) in float aPatId;  // 实例：线型 id（0~4）

// 每个实例渲染 4 个顶点（一个矩形），用 gl_VertexID 构造角顶
// 顶点 0,1 属于 t=0（起点侧），2,3 属于 t=1（终点侧）；side = -1/+1 表示左右偏移
out VS_OUT {
    vec4 color;
    float widthPx;
    float patId;
    float uPix;   // 该片元沿线的“像素距离”坐标（用于虚线/点线）
} vs_out;

uniform mat4 uMVP;
uniform vec2 uViewportPx;  // (width_px, height_px)，注意乘上 DPR

void main() {
    // 转到裁剪空间
    vec4 c0 = uMVP * vec4(aP0, 1.0);
    vec4 c1 = uMVP * vec4(aP1, 1.0);

    // 裁剪→NDC→屏幕像素
    vec2 ndc0 = c0.xy / c0.w;
    vec2 ndc1 = c1.xy / c1.w;
    vec2 px0  = (ndc0 * 0.5 + 0.5) * uViewportPx;
    vec2 px1  = (ndc1 * 0.5 + 0.5) * uViewportPx;

    // 屏幕空间方向/法向（修正版）
    vec2 dir  = px1 - px0;
    float L   = length(dir);
    vec2 tdir = (L < 1e-4) ? vec2(1.0, 0.0) : dir / L;  // ★ 关键：退化时给个默认方向
    vec2 nrm  = vec2(-tdir.y, tdir.x);

    // 本顶点是矩形的哪个角：t ∈ {0,1}，side ∈ {-1,+1}
    int vid = gl_VertexID % 4;
    float t    = (vid >= 2) ? 1.0 : 0.0;
    float side = (vid == 1 || vid == 3) ? +1.0 : -1.0;

    // 矩形中心线的该点（像素坐标）
    vec2 basePx = mix(px0, px1, t);
    // 向左右偏移半宽
    vec2 outPx  = basePx + nrm * side * (aWidth * 0.5);

    // 回到 NDC
    vec2 outNDC = outPx / uViewportPx * 2.0 - 1.0;
    // 深度用 NDC z 做线性插值即可（近似足够）
    float zNDC = mix(c0.z / c0.w, c1.z / c1.w, t);

    gl_Position = vec4(outNDC, zNDC, 1.0);

    vs_out.color   = aColor;
    vs_out.widthPx = aWidth;
    vs_out.patId   = aPatId;
    vs_out.uPix    = t * L;    // 这就是“沿线像素距离”（用于虚线图案）
}
