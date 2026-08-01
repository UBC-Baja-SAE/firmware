#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
};

void main() {
    vec2 uv = qt_TexCoord0 * 2.0 - 1.0;

    float edgeX = pow(abs(uv.x), 16.0);
    float edgeY = pow(abs(uv.y), 16.0);
    float edge = clamp(edgeX + edgeY, 0.0, 1.0);

    float pulse = 0.7 + 0.3 * sin(time * 4.0);
    vec3 col = vec3(1.0, 0.15, 0.15);
    float alpha = edge * pulse;

    fragColor = vec4(col * alpha, alpha) * qt_Opacity;
}