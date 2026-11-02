#version 330 core
// AI-assisted world-anchored billboard, with a readable size in logical pixels.
layout(location=0) in vec2 position;
uniform vec4 clip_anchor;
uniform vec2 size_ndc;
out vec2 local;
void main() {
    gl_Position=clip_anchor;
    gl_Position.xy+=position*size_ndc*clip_anchor.w;
    local=position*vec2(0.6,1.0);
}
