#version 330 core
// AI-assisted procedural exclamation mark; no image asset or external font.
in vec2 local;
out vec4 color;
void main() {
    float halfWidth=mix(0.15,0.23,clamp((local.y+0.15)/1.05,0.0,1.0));
    float stem=max(abs(local.x)-halfWidth,max(-0.15-local.y,local.y-0.90));
    float dot=length(local-vec2(0.0,-0.68))-0.17;
    float distance=min(stem,dot);
    float aa=max(fwidth(distance),0.001);
    float fill=1.0-smoothstep(-aa,aa,distance);
    float outline=1.0-smoothstep(0.045-aa,0.045+aa,distance);
    vec3 paint=mix(vec3(0.035,0.025,0.01),vec3(1.0,0.80,0.06),fill);
    color=vec4(paint,outline);
}
