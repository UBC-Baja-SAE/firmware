#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    vec2 resolution; // This will now receive the low-res dimensions
};

// 1. Fast 3D Hash
float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 2. Cheap Value Noise (Replacing expensive texture lookups)
float noise(in vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(p + vec3(0,0,0)), hash(p + vec3(1,0,0)), f.x),
                   mix(hash(p + vec3(0,1,0)), hash(p + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash(p + vec3(0,0,1)), hash(p + vec3(1,0,1)), f.x),
                   mix(hash(p + vec3(0,1,1)), hash(p + vec3(1,1,1)), f.x), f.y), f.z) * 2.0 - 1.0;
}

// 3. Ultra-Fast Density Map (Just 2 octaves instead of 5)
float map(in vec3 p) {
    vec3 q = p - vec3(0.0, 0.1, 1.0) * (time * 0.5);
    float f = 0.500 * noise(q); q = q * 2.02;
    f += 0.250 * noise(q);
    return clamp(-0.5 - p.y + 1.75 * f, 0.0, 1.0);
}

const vec3 sundir = vec3(-0.7071, 0.0, -0.7071);

// 4. Heavily Culled Raymarcher
vec4 raymarch(in vec3 ro, in vec3 rd, in vec3 bgcol, in vec2 fragCoord) {
    vec4 sum = vec4(0.0);

    // Dithering hides the low step count
    float t = 0.05 * fract(sin(dot(fragCoord, vec2(12.9898, 78.233))) * 43758.5453);

    // OPTIMIZATION: Max 25 steps (Down from 140)
    for(int i = 0; i < 25; i++) {
        vec3 pos = ro + t * rd;
        if(pos.y < -3.0 || pos.y > 2.0 || sum.a > 0.99) break;

        float den = map(pos);
        if(den > 0.01) {
            // Cheaper lighting logic
            float dif = clamp((den - map(pos + 0.3 * sundir)) / 0.6, 0.0, 1.0);
            vec3 lin = vec3(1.0, 0.6, 0.3) * dif + vec3(0.91, 0.98, 1.05);
            vec4 col = vec4(mix(vec3(1.0, 0.95, 0.8), vec3(0.25, 0.3, 0.35), den), den);
            col.xyz *= lin;
            col.xyz = mix(col.xyz, bgcol, 1.0 - exp(-0.003 * t * t));
            col.w *= 0.5;
            col.rgb *= col.a;
            sum += col * (1.0 - sum.a);
        }
        // Aggressive distance leaps
        t += max(0.1, 0.12 * t);
    }
    return clamp(sum, 0.0, 1.0);
}

mat3 setCamera(in vec3 ro, in vec3 ta, float cr) {
    vec3 cw = normalize(ta - ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = normalize(cross(cu, cw));
    return mat3(cu, cv, cw);
}

void main() {
    // 5. Flip Y for Qt coordinate system
    vec2 fixed_uv = vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y);
    vec2 fragCoord = fixed_uv * resolution;
    vec2 p = (2.0 * fragCoord - resolution.xy) / resolution.y;

    // Auto-pan camera
    vec2 m = vec2(time * 0.05, 0.5);
    vec3 ro = 4.0 * normalize(vec3(sin(3.0 * m.x), 0.8 * m.y, cos(3.0 * m.x))) - vec3(0.0, 0.1, 0.0);
    vec3 ta = vec3(0.0, -1.0, 0.0);
    mat3 ca = setCamera(ro, ta, 0.07 * cos(0.25 * time));
    vec3 rd = ca * normalize(vec3(p.xy, 1.5));

    float sun = clamp(dot(sundir, rd), 0.0, 1.0);

    // Sky gradient
    vec3 col = vec3(0.6, 0.71, 0.75) - rd.y * 0.2 * vec3(1.0, 0.5, 1.0) + 0.15 * 0.5;
    col += 0.2 * vec3(1.0, 0.6, 0.1) * pow(sun, 8.0);

    // Clouds
    vec4 res = raymarch(ro, rd, col, fragCoord);
    col = col * (1.0 - res.w) + res.xyz;

    // Sun glare
    col += vec3(0.2, 0.08, 0.04) * pow(sun, 3.0);

    fragColor = vec4(col, 1.0) * qt_Opacity;
}