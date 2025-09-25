#version 330 core
in VS_OUT {
    vec4 color;
    float widthPx;
    float patId;
    float uPix;
} fs_in;
out vec4 FragColor;

// 统一的可编程图案：最多 6 段 on/off，相邻段交替开/关
uniform int  uSegCount;          // 实际段数
uniform float uSegPx[6];         // 每段长度（像素）
uniform float uPeriod;           // 周期 = 所有段之和
uniform float uPhase;            // 相位（像素），可做滚动动画等

// 经典风格的预设（按 id 选择）；也可以全靠 uSegPx/uSegCount 自己配置
bool patternOn(float u) {
    float x = mod(u + uPhase, uPeriod);
    float acc = 0.0;
    for (int i=0; i<uSegCount; ++i) {
        float segEnd = acc + uSegPx[i];
        if (x >= acc && x < segEnd) {
            // 偶数段“画”，奇数段“空”
            return (i % 2 == 0);
        }
        acc = segEnd;
    }
    // 超出（数值边界）默认画
    return true;
}

void main() {
    int pid = int(fs_in.patId + 0.5);

    // Solid / 经典预置
    if (pid == 0) {                         // Solid
        FragColor = fs_in.color; return;
    } else if (pid == 1) {                  // Dash: 8 on, 4 off
        if (!patternOn(fs_in.uPix)) discard;
        FragColor = fs_in.color; return;
    } else if (pid == 2) {                  // Dot: 1 on, 5 off（可调）
        if (!patternOn(fs_in.uPix)) discard;
        FragColor = fs_in.color; return;
    } else if (pid == 3) {                  // Dash-Dot: 8 on,4 off,1 on,4 off
        if (!patternOn(fs_in.uPix)) discard;
        FragColor = fs_in.color; return;
    } else if (pid == 4) {                  // Dash-Dot-Dot: 8,4,1,2,1,4
        if (!patternOn(fs_in.uPix)) discard;
        FragColor = fs_in.color; return;
    } else {
        // 兜底
        FragColor = fs_in.color; return;
    }
}
