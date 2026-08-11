#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    vec2 resolution;
};

// 1. Ultra-fast 2D Randomizer
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453123);
}

// 2. Smooth 2D Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    // Hermite curve for smooth interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),
               mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);
}

// 3. Fractional Brownian Motion (Stacks 4 layers of noise)
float fbm(vec2 p) {
    float f = 0.0;
    float amp = 0.5;
    // GLES 2.0 safe fixed-loop
    for(int i = 0; i < 4; i++) {
        f += amp * noise(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return f;
}

void main() {
    vec2 uv = qt_TexCoord0;

    // Fix aspect ratio so clouds aren't squished on the ultrawide screen
    vec2 p = uv * vec2(resolution.x / resolution.y, 1.0);

    // Scale the clouds and add horizontal wind based on time
    vec2 cloudUV = p * 3.0 + vec2(time * 0.1, 0.0);

    // Generate the base cloud pattern
    float q = fbm(cloudUV);

    // DOMAIN WARPING: We feed the first noise pattern into a second noise pattern.
    // This stretches and swirls the clouds, making them look wispy and realistic instead of blobby.
    float w = fbm(cloudUV + vec2(q, q) * 2.0 - vec2(time * 0.05, 0.0));

    // Map the noise to a cloud density (0.0 = clear sky, 1.0 = thick cloud)
    float cloudDensity = smoothstep(0.2, 0.8, w);

    // --- Colors ---
    // Sky gradient: Light blue at the horizon (bottom), deep blue at the top
    vec3 skyColor = mix(vec3(0.5, 0.7, 0.85), vec3(0.1, 0.25, 0.55), uv.y);

    // Cloud color: Pure white
    vec3 cloudColor = vec3(0.95, 0.95, 1.0);

    // Blend sky and clouds based on density
    vec3 finalColor = mix(skyColor, cloudColor, cloudDensity);

    // Add a fake sunset/sun glow in the center background
    float sunGlow = max(0.0, 1.0 - distance(uv, vec2(0.5, 0.5)) * 1.5);
    finalColor += vec3(0.3, 0.15, 0.05) * sunGlow * (1.0 - cloudDensity);

    fragColor = vec4(finalColor, 1.0) * qt_Opacity;
}