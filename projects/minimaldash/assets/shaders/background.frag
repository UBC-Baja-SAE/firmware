#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
    vec2 resolution;
};

#define TAU 6.28318530718
#define HOLES
#define BUMP_MAP
#define DIST_TYPE 0

vec2 gSc = vec2(1.0)/5.0;
vec2 cntr;
float cir;
int polyID;
int pID;

// WORKAROUND: Swapped mat4x2 for mat4 to fix OpenGL ES 2.0 translation crashes on Raspberry Pi
mat4 vID = mat4(vec4(-.5, -.5, 0, 0), vec4(-.5, .5, 0, 0), vec4(.5, .5, 0, 0), vec4(.5, -.5, 0, 0));
mat4 eID = mat4(vec4(-.5, 0, 0, 0), vec4(0, .5, 0, 0), vec4(.5, 0, 0, 0), vec4(0, -.5, 0, 0));
vec2 vP[16];

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(141.13, 289.97))) * 43758.5453);
}

float distLineS(vec2 p, vec2 a, vec2 b) {
    vec2 dir = normalize(b - a);
    return dot(p - a, vec2(-dir.y, dir.x));
}

float lineIntersect(vec2 p1, vec2 d1, vec2 p2, vec2 p3) {
    vec2 d2 = p3 - p2;
    float crossD1D2 = d1.x*d2.y - d1.y*d2.x;
    if (abs(crossD1D2) < 1e-6) return 0.0;
    return ((p2.x - p1.x)*d2.y - (p2.y - p1.y)*d2.x) / crossD1D2;
}

float sdPoly(vec2 p, vec2 v[16], int n) {
    float d = dot(p-v[0],p-v[0]);
    float s = 1.0;
    for( int i=0, j=n-1; i<16; j=i, i++ ) {
        if(i>=n) break;
        vec2 e = v[j] - v[i];
        vec2 w = p - v[i];
        vec2 b = w - e*clamp( dot(w,e)/dot(e,e), 0.0, 1.0 );
        d = min( d, dot(b,b) );
        bvec3 cond = bvec3( p.y>=v[i].y, p.y<v[j].y, e.x*w.y>e.y*w.x );
        if( all(cond) || all(not(cond)) ) s*=-1.0;
    }
    return s*sqrt(d);
}

float sBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

mat4 getEdges(vec2 ip){
    const float rF = .95;
    vec2 eR = vec2(0, .5*rF);
    mat4 eM;

    for(int i = 0; i<4; i++){
        vec2 edID = ip + eID[i].xy;
        float rndI = mod(dot(edID, vec2(41, 53)), 4.)/4.;
        float rndD = hash21(edID + .06)<.5? -1. : 1.;
        rndI = sin(TAU*rndI*rndD + time*fract(rndD*77.77 + .5))*.5 + .5;
        eM[i] = vec4(eID[i].xy*gSc - rndI*rndD*gSc*rF*eR, 0.0, 0.0);
        eR = eR.yx;
    }
    return eM;
}

vec4 distField(vec2 p){
    vec2 ip = floor(p/gSc);
    p -= (ip + .5)*gSc;
    vec2 svIP = ip;
    mat4 eM = getEdges(ip);

    vec2 minE = min(vec2(eM[1].x, eM[0].y), vec2(eM[3].x, eM[2].y));
    vec2 maxE = max(vec2(eM[1].x, eM[0].y), vec2(eM[3].x, eM[2].y));
    mat4 p4 = mat4(vec4(minE,0,0), vec4(minE.x, maxE.y,0,0), vec4(maxE,0,0), vec4(maxE.x, minE.y,0,0));

    vec2 rDim = (vec2(maxE.x - minE.x, maxE.y - minE.y));
    vec2 rP = mix(minE, maxE, .5);
    vec2 ap = abs(p - rP) - rDim/2.;
    float cPoly = max(ap.x, ap.y);

    float d;

    if(cPoly<0.){
        d = cPoly;
        polyID = 4;
        pID = 4;
        vP[0] = p4[0].xy, vP[1] = p4[1].xy, vP[2] = p4[2].xy, vP[3] = p4[3].xy;
        cntr = rP;
    } else {
        d = -cPoly;
        vec4 ln;
        for(int i = 0; i<4; i++){
            ln[i] = distLineS(p, eM[i].xy, eM[i].xy - eID[i].xy);
        }
        ln = max(ln, -ln.wxyz);
        for(int i = 0; i<4; i++){
            if(ln[i]<0.){
                polyID = i;
                break;
            }
        }

        int i = polyID;
        float dir = (i==0 || i==2)? 1. : -1.;

        vec2 ro = eM[i].xy;
        vec2 rd = -normalize(eID[i].xy);
        float t = lineIntersect(ro, rd, eM[(i + 3)%4].xy, eM[(i + 3)%4].xy - eID[(i + 3)%4].xy*8.);
        vec2 p0 = ro + rd*t;

        mat4 eMD = getEdges(ip + vID[i].xy*2.);
        int k = (i + 1)%4;
        ro = eMD[k].xy;
        rd = -normalize(eID[k].xy);
        t = lineIntersect(ro, rd, eMD[(k + 1)%4].xy, eMD[(k + 1)%4].xy - eID[(k + 1)%4].xy*8.);
        vec2 p1 = ro + rd*t + vID[i].xy*2.*gSc;
        cntr = mix(p0, p1, .5);

        d = max(d, ln[i]);
        vec2 q = p - p1;
        vec2 ln2 = q*sign(vID[i].xy);
        d = max(d, max(ln2.x, ln2.y));

        vec2 minI = min(vec2(eMD[1].x, eMD[0].y), vec2(eMD[3].x, eMD[2].y));
        vec2 maxI = max(vec2(eMD[1].x, eMD[0].y), vec2(eMD[3].x, eMD[2].y));
        mat4 p4D = mat4(vec4(minI,0,0), vec4(minI.x, maxI.y,0,0), vec4(maxI,0,0), vec4(maxI.x, minI.y,0,0));

        rDim = (vec2(maxI.x - minI.x, maxI.y - minI.y));
        vec2 rQ = mix(minI, maxI, .5);
        q = p - vID[i].xy*2.*gSc;
        vec2 aq = abs(q - rQ) - (rDim)/2.;
        float rect = max(aq.x, aq.y);
        d = max(d, -rect);

        #if DIST_TYPE == 0
        vP[0] = p0;
        vP[1] = vec2(p0.x, p1.y);
        vP[2] = p1;
        vP[3] = vec2(p1.x, p0.y);
        if(i%2==1){
           vec2 tmp = vP[1]; vP[1] = vP[3]; vP[3] = tmp;
        }
        #endif

        mat4 cP = mat4(vec4(vP[0],0,0), vec4(vP[1],0,0), vec4(vP[2],0,0), vec4(vP[3],0,0));
        int vIndex = 0;
        int hit = 0;

        #if DIST_TYPE == 0
        eM *= dir;
        if(eM[1].x<eM[3].x && -eM[0].y<-eM[2].y){
            vP[vIndex++] = p4[(i + 1)%4].xy;
            vP[vIndex++] = p4[(i + 0)%4].xy;
            vP[vIndex++] = p4[(i + 3)%4].xy;
            hit = 1;
        }
        if(hit==0) vP[vIndex++] = cP[0].xy;
        #endif

        mat4 eMI = getEdges(svIP + eID[(i + 3)%4].xy*2.);
        minI = min(vec2(eMI[1].x, eMI[0].y), vec2(eMI[3].x, eMI[2].y));
        maxI = max(vec2(eMI[1].x, eMI[0].y), vec2(eMI[3].x, eMI[2].y));
        mat4 p4I = mat4(vec4(minI,0,0), vec4(minI.x, maxI.y,0,0), vec4(maxI,0,0), vec4(maxI.x, minI.y,0,0));

        rDim = (vec2(maxI.x - minI.x, maxI.y - minI.y));
        rQ = mix(minI, maxI, .5);
        q = p - eID[(i + 3)%4].xy*gSc*2.;
        q = abs(q - rQ) - (rDim)/2.;
        rect = max(q.x, q.y);
        d = max(d, -rect);

        #if DIST_TYPE == 0
        hit = 0;
        eMI *= dir;
        if(-eMI[1].x<-eMI[3].x && eMI[0].y<eMI[2].y){
            vP[vIndex++] = p4I[(i + 2)%4].xy + eID[(i + 3)%4].xy*2.*gSc;
            vP[vIndex++] = p4I[(i + 1)%4].xy + eID[(i + 3)%4].xy*2.*gSc;
            vP[vIndex++] = p4I[(i + 0)%4].xy + eID[(i + 3)%4].xy*2.*gSc;
            hit = 1;
        }
        if(hit==0) vP[vIndex++] = cP[1].xy;
        #endif

        #if DIST_TYPE == 0
        hit = 0;
        eMD *= dir;
        if(eMD[1].x<eMD[3].x && -eMD[0].y<-eMD[2].y){
            vP[vIndex++] = p4D[(i + 3)%4].xy + vID[i].xy*2.*gSc;
            vP[vIndex++] = p4D[(i + 2)%4].xy + vID[i].xy*2.*gSc;
            vP[vIndex++] = p4D[(i + 1)%4].xy + vID[i].xy*2.*gSc;
            hit = 1;
        }
        if(hit==0) vP[vIndex++] = cP[2].xy;
        #endif

        eMI = getEdges(svIP + eID[i].xy*2.);
        minI = min(vec2(eMI[1].x, eMI[0].y), vec2(eMI[3].x, eMI[2].y));
        maxI = max(vec2(eMI[1].x, eMI[0].y), vec2(eMI[3].x, eMI[2].y));
        p4I = mat4(vec4(minI,0,0), vec4(minI.x, maxI.y,0,0), vec4(maxI,0,0), vec4(maxI.x, minI.y,0,0));

        rDim = (vec2(maxI.x - minI.x, maxI.y - minI.y));
        rQ = mix(minI, maxI, .5);
        q = p - eID[i].xy*gSc*2.;
        q = abs(q - rQ) - (rDim)/2.;
        rect = max(q.x, q.y);
        d = max(d, -rect);

        #if DIST_TYPE == 0
        hit = 0;
        eMI *= dir;
        if(-eMI[1].x<-eMI[3].x && eMI[0].y<eMI[2].y){
            vP[vIndex++] = p4I[(i + 0)%4].xy + eID[i].xy*2.*gSc;
            vP[vIndex++] = p4I[(i + 3)%4].xy + eID[i].xy*2.*gSc;
            vP[vIndex++] = p4I[(i + 2)%4].xy + eID[i].xy*2.*gSc;
            hit = 1;
        }
        if(hit==0) vP[vIndex++] = cP[3].xy;
        #endif

        pID = vIndex;
        ip += vID[i].xy;
    }

    #if DIST_TYPE == 0
    d = sdPoly(p, vP, pID);
    #endif

    cir = length(p - cntr);

    #ifdef HOLES
    if(hash21(ip + .23)<.4 && gSc.x>1./5. - .001){
        d = abs(d + .09*gSc.x) - .09*gSc.x;
    }
    #endif

    float lN = 80.;
    float pat = abs(fract(d*lN + .5) - .5)/lN;
    d = mix(d*1.055, d*.9, smoothstep(0., .02, pat));

    return vec4(d, ip, polyID);
}

float grid(vec2 p){
    vec2 ip = floor(p/gSc);
    p -= (ip + .5)*gSc;
    return sBox(p, gSc/2.);
}

void main() {
    vec2 fragCoord = qt_TexCoord0 * resolution;
    vec2 uv = (fragCoord - resolution.xy*.5)/resolution.y;
    vec2 p = uv - vec2(0, time/12.);

    #ifdef HOLES
    gSc /= 1.5;
    vec4 d4B = distField(p + .5 - vec2(time/12., 0));
    gSc *= 1.5;

    float dB = d4B.x;
    vec2 idB = d4B.yz;

    float rndB = hash21(idB + .1);
    vec3 rColB = .5 + .45*cos(TAU*rndB/3.5 + vec3(0, 1, 2)*1.5 - .3);
    float grB = dot(rColB, vec3(.299, .587, .114));
    vec3 pColB = polyID==4? vec3(grB*.5 + .5)*vec3(.97, 1, 1.03) : rColB.zyx*1.2;
    #endif

    #ifdef BUMP_MAP
    vec2 ld = normalize(vec2(-2.5, -1));
    vec4 d4Hi = distField(p - ld*.003);
    #endif

    vec4 d4 = distField(p);
    float d = d4.x;
    vec2 id = d4.yz;

    float sf = 1./resolution.y;
    float shF = resolution.y/450.;
    float ew = .006;

    vec3 col = vec3(.25);
    #ifdef HOLES
    col = mix(col, vec3(0), 1. - smoothstep(0., sf*shF*16., dB));
    col = mix(col, vec3(0), 1. - smoothstep(0., sf, dB));
    col = mix(col, pColB, 1. - smoothstep(0., sf, dB + ew*.8));
    #endif

    float rnd = hash21(id + .1);
    vec3 rCol = .5 + .45*cos(TAU*rnd/3.5 + vec3(0, 1, 2)*1.5 - .3);
    float gr = dot(rCol, vec3(.299, .587, .114));
    vec3 pCol = polyID<4? vec3(gr*.5 + .5)*vec3(.97, 1, 1.03) : rCol*1.2;

    #ifdef BUMP_MAP
    float b = max(.5 + (d4Hi.x - d)/.003, 0.);
    float b2 = max(.5 + (max(d4Hi.x, -.0125) - max(d, -.0125))/.003, 0.);
    pCol *= .5 + b*b*.5 + b2*b2*.5;
    #else
    pCol *= 1.1;
    #endif

    col = mix(col, col*.4, 1. - smoothstep(0., sf*shF*24., d));
    col = mix(col, vec3(0), 1. - smoothstep(0., sf, d - ew/2.));
    col = mix(col, pCol, 1. - smoothstep(0., sf, d + ew));

    uv = qt_TexCoord0;
    col *= pow(16.*uv.x*uv.y*(1. - uv.x)*(1. - uv.y) , 1./16.);

    fragColor = vec4(sqrt(max(col, 0.)), 1.0) * qt_Opacity;
}