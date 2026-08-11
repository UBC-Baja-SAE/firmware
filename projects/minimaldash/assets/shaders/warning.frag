#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
};

void main() {
    vec2 uv = qt_TexCoord0;

    // 1. CRT Scanlines rolling downwards
    float scanline = sin(uv.y * 250.0 - time * 15.0) * 0.2 + 0.8;

    // 2. Sharp Tech Border
    // Find distance from center (0 = center, 1 = edge)
    vec2 d = abs(uv - 0.5) * 2.0;

    // Create a sharp 5px frame right at the bounds
    float frameX = step(0.98, d.x);
    float frameY = step(0.94, d.y); // Adjust this if top/bottom border is too thick
    float frame = clamp(frameX + frameY, 0.0, 1.0);

    // 3. Soft inner glow pulse
    float glow = pow(max(d.x, d.y), 8.0) * (0.6 + 0.4 * sin(time * 6.0));

    // 4. Combine
    vec3 baseCol = vec3(1.0, 0.2, 0.2); // Intense red
    float alpha = clamp(frame + glow, 0.0, 1.0);

    // Apply scanlines to the color
    vec3 finalColor = baseCol * scanline;

    fragColor = vec4(finalColor * alpha, alpha) * qt_Opacity;
}