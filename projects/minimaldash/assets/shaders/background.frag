#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

// WARNING: Struct order must exactly match Qt's std140 padding rules
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    vec2 resolution;
    float uiDepth;
    float cloudDensity; // Driven by RPM
    float timeOfDay;    // Driven by Speed
    float panOffset;    // Driven by Steering
};

layout(binding = 1) uniform sampler2D uiSource;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(in vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float n = mix(mix(mix(hash(p + vec3(0,0,0)), hash(p + vec3(1,0,0)), f.x),
                      mix(hash(p + vec3(0,1,0)), hash(p + vec3(1,1,0)), f.x), f.y),
                  mix(mix(hash(p + vec3(0,0,1)), hash(p + vec3(1,0,1)), f.x),
                      mix(hash(p + vec3(0,1,1)), hash(p + vec3(1,1,1)), f.x), f.y), f.z);
    return n * 2.0 - 1.0;
}

float dither(ivec2 px) {
    return fract(sin(dot(vec2(px), vec2(12.9898, 78.233))) * 43758.5453);
}

// Maps 0.0 (few clouds) to 1.0 (heavy overcast) based on RPM
float getCloudOffset() {
    return mix(-1.5, 0.8, cloudDensity);
}

float map5( in vec3 p ) {
    vec3 q = p - vec3(0.0,0.1,1.0)*time;
    float f; float a = 0.5;
    f  = a*noise( q ); q = q*2.02; a = a*0.5;
    f += a*noise( q ); q = q*2.03; a = a*0.5;
    f += a*noise( q ); q = q*2.01; a = a*0.5;
    f += a*noise( q ); q = q*2.02; a = a*0.5;
    f += a*noise( q );
    return clamp( getCloudOffset() - p.y + 1.75*f, 0.0, 1.0 );
}
float map4( in vec3 p ) {
    vec3 q = p - vec3(0.0,0.1,1.0)*time;
    float f; float a = 0.5;
    f  = a*noise( q ); q = q*2.02; a = a*0.5;
    f += a*noise( q ); q = q*2.03; a = a*0.5;
    f += a*noise( q ); q = q*2.01; a = a*0.5;
    f += a*noise( q );
    return clamp( getCloudOffset() - p.y + 1.75*f, 0.0, 1.0 );
}
float map3( in vec3 p ) {
    vec3 q = p - vec3(0.0,0.1,1.0)*time;
    float f; float a = 0.5;
    f  = a*noise( q ); q = q*2.02; a = a*0.5;
    f += a*noise( q ); q = q*2.03; a = a*0.5;
    f += a*noise( q );
    return clamp( getCloudOffset() - p.y + 1.75*f, 0.0, 1.0 );
}
float map2( in vec3 p ) {
    vec3 q = p - vec3(0.0,0.1,1.0)*time;
    float f; float a = 0.5;
    f  = a*noise( q ); q = q*2.02; a = a*0.5;
    f += a*noise( q );
    return clamp( getCloudOffset() - p.y + 1.75*f, 0.0, 1.0 );
}

const vec3 sundir = vec3(-0.7071,0.0,-0.7071);

vec4 raymarch( in vec3 ro, in vec3 rd, in vec3 bgcol, in ivec2 px, in vec2 uv ) {
    vec4 sum = vec4(0.0);
    float t = 0.05 * dither(px);
    bool uiBlended = false;

    if (t >= uiDepth) {
        vec4 uiCol = texture(uiSource, uv);
        sum += uiCol * (1.0 - sum.a);
        uiBlended = true;
    }

    #define MARCH(STEPS,MAPLOD) for(int i=0; i<STEPS; i++) { \
        vec3 pos = ro + t*rd; \
        if( pos.y<-3.0 || pos.y>2.0 || sum.a>0.99 ) break; \
        if (!uiBlended && t >= uiDepth) { \
            vec4 uiCol = texture(uiSource, uv); \
            sum += uiCol * (1.0 - sum.a); \
            uiBlended = true; \
        } \
        float den = MAPLOD( pos ); \
        if( den>0.01 ) { \
            float dif = clamp((den - MAPLOD(pos+0.3*sundir))/0.6, 0.0, 1.0 ); \
            vec3 sunLightCol = mix(vec3(1.0, 0.6, 0.3), vec3(1.0, 0.9, 0.8), timeOfDay); \
            vec3 lin = sunLightCol * dif + vec3(0.91, 0.98, 1.05); \
            vec4 col = vec4( mix( vec3(1.0,0.95,0.8), vec3(0.25,0.3,0.35), den ), den ); \
            col.xyz *= lin; \
            col.xyz = mix( col.xyz, bgcol, 1.0-exp(-0.003*t*t) ); \
            col.w *= 0.4; \
            col.rgb *= col.a; \
            sum += col*(1.0-sum.a); \
        } \
        t += max(0.06,0.05*t); \
    }

    MARCH(30,map5);
    MARCH(30,map4);
    MARCH(20,map3);
    MARCH(20,map2);

    if (!uiBlended) {
        vec4 uiCol = texture(uiSource, uv);
        sum += uiCol * (1.0 - sum.a);
    }

    return clamp( sum, 0.0, 1.0 );
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr ) {
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

void main() {
    vec2 fixed_uv = vec2(qt_TexCoord0.x, 1.0 - qt_TexCoord0.y);
    vec2 fragCoord = fixed_uv * resolution;
    vec2 p = (2.0 * fragCoord - resolution.xy) / resolution.y;

    // Steering dynamically offsets the panning angle
    vec2 m = vec2(time * 0.05 + (panOffset * 0.8), 0.5);

    vec3 ro = 4.0*normalize(vec3(sin(3.0*m.x), 0.8*m.y, cos(3.0*m.x))) - vec3(0.0,0.1,0.0);
    vec3 ta = vec3(0.0, -1.0, 0.0);
    mat3 ca = setCamera( ro, ta, 0.07*cos(0.25*time) );
    vec3 rd = ca * normalize( vec3(p.xy,1.5));

    float sun = clamp( dot(sundir,rd), 0.0, 1.0 );

    // Interpolate Sky Color based on Speedometer (0 = Sunset, 1 = Midday Blue)
    vec3 skySunset = vec3(0.6, 0.71, 0.75);
    vec3 skyMidday = vec3(0.15, 0.4, 0.8);
    vec3 baseSky = mix(skySunset, skyMidday, timeOfDay);

    // Interpolate Sun Glare Color
    vec3 sunSunset = vec3(1.0, 0.6, 0.1);
    vec3 sunMidday = vec3(1.0, 0.9, 0.7);
    vec3 sunColor = mix(sunSunset, sunMidday, timeOfDay);

    vec3 col = baseSky - rd.y*0.2*vec3(1.0,0.5,1.0) + 0.15*0.5;
    col += 0.2 * sunColor * pow( sun, 8.0 );

    ivec2 px = ivec2(fragCoord);
    vec4 res = raymarch( ro, rd, col, px, qt_TexCoord0 );

    col = col*(1.0-res.w) + res.xyz;
    col += vec3(0.2,0.08,0.04)*pow( sun, 3.0 );

    fragColor = vec4(col, 1.0) * qt_Opacity;
}