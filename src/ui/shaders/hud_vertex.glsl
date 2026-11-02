#version 330 core
// AI-assisted pixel-to-NDC conversion; no external matrix builders.
layout(location=0) in vec2 position;
layout(location=1) in vec4 vertex_color;
layout(location=2) in vec2 local_position;
layout(location=3) in float instrument_mode;
uniform vec2 resolution;
out vec4 paint;
out vec2 local;
flat out int mode;
void main() {
    gl_Position=vec4(2.0*position.x/resolution.x-1.0,1.0-2.0*position.y/resolution.y,0,1);
    paint=vertex_color;
    local=local_position;
    mode=int(instrument_mode);
}
