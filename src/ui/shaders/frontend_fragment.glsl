#version 330 core
// AI-assisted procedural surface; no external bitmap assets.
in vec4 paint;
out vec4 color;
void main() {
    float grain=0.992+0.008*sin(gl_FragCoord.x*.14)*sin(gl_FragCoord.y*.17);
    color=vec4(paint.rgb*grain,paint.a);
}
