#version 330 core
// AI-assisted 2D frontend geometry, positioned in logical window pixels.
layout(location=0) in vec2 position;
layout(location=1) in vec4 vertex_color;
uniform vec2 resolution;
out vec4 paint;
void main() {
    gl_Position=vec4(2.0*position.x/resolution.x-1.0,1.0-2.0*position.y/resolution.y,0,1);
    paint=vertex_color;
}
