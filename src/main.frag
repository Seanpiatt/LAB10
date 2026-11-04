#version 120 

// (way more precision than a float can actually store...)
#define LOG_2 0.6931471805599453094172321214581765680755001

// start using these antialiasing values once you start (c)
#define AA 2
const int TOTAL_SAMPLES = AA * AA;

uniform bool firstFrame;

uniform int MAX_ITERS;
uniform vec2 windowSizePixels;
uniform int TRUE_MAX_ITER;

//current
uniform vec2 c;
uniform vec2 camCenterWorld;
uniform float camWorldUnitsPerPixel;

//pevious
uniform vec2 prevCamCenterWorld;
uniform float prevCamWorldUnitsPerPixel;
uniform vec2 prevC;

uniform sampler2D prevFrame;

uniform int TRUE_MAX_INTEGERS;

// reprojection (1d) may require extra uniforms here...

// Use this to get a color from your smoothed Julia set escape times.
vec4 colorTime(float t) {
    return vec4(0.5 + 0.5 * cos(  0.1 * (t) + vec3(0.f, 0.35, 0.7)), 1.f);
}

struct EscapeTime {
    float iters;
    bool isInf;
};

// Escape time of iterating z^2 + c
EscapeTime escapeTimeSmooth(vec2 z) {
    for(float i = 1; i<= MAX_ITERS;++i){
        float xtemp = z.x * z.x  - z.y * z.y + c.x;
        float y = 2.0 * z.x * z.y + c.y;
        float x = xtemp;
        z = vec2(x,y);
        if(z.x*z.x + z.y*z.y > 16.0){
            float smooth = i + 1.0 - log(log(length(z)))/LOG_2;
            return EscapeTime(smooth,false);
        }
    }
    return EscapeTime(0.0,true);
}

vec2 windowToWorld(vec2 windowCoords) {
    return camCenterWorld + (windowCoords - windowSizePixels * 0.5) * camWorldUnitsPerPixel;
}

// For reprojection
vec2 worldToOldWindow(vec2 worldCoords) {
    return (worldCoords - prevCamCenterWorld)/prevCamWorldUnitsPerPixel + windowSizePixels * 0.5;
}

// For SSAA
// Given (dx, dy) in [0, AA)^2, suppose the unit square [0, 1)^2 was cut up into AA equal
// partitions along each dimension. delta((dx, dy)) returns the center of the sub-square
// which is in the dy'th row and dx'th column (both 0-indexed).
vec2 delta(vec2 dxy) {
    return (dxy + 1/AA) / float(AA);
}


void main() {
    // TODO
    vec4 colorSum = vec4(0.0);
    vec4 colorTemp = vec4(0.0);
    float alpha = 1.0;
    for (int dy = 0; dy < AA; ++dy) {
        for (int dx = 0; dx < AA; ++dx) {
            vec2 windowPos = floor(gl_FragCoord.xy) + delta(vec2(dx, dy));
            vec2 world = windowToWorld(windowPos);
            EscapeTime et = escapeTimeSmooth(world);
            if(firstFrame || c!= prevC){
                float alpha = 1.0;
            }
            else{
                float alpha = clamp(et.iters / float(TRUE_MAX_ITER), 0.0, 1.0);
            }       
            if(et.isInf == false){
                colorTemp = colorTime(et.iters);
                colorTemp.w = alpha;
            }
            else{
                colorTemp = vec4(0.5,0.5,0.5,1.0);
            }
            vec2 oldUV = worldToOldWindow(world) / windowSizePixels;
            if(oldUV.x >= 0 && oldUV.x <=1 && oldUV.y >= 0 && oldUV.y <= 1){
                vec4 colorTemp = mix(texture2D(prevFrame, oldUV), colorTemp, alpha);
            }
            colorSum += colorTemp;
        }
    }
    gl_FragColor = colorSum / float(TOTAL_SAMPLES);
}

