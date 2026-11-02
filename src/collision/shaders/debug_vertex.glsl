#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 paint;
uniform mat4 view;
uniform mat4 projection;
out vec3 color;
void main() {
    gl_Position=projection*vec4(mat3(view)*position,1);
    color=paint;
}
