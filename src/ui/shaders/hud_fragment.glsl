#version 330 core
in vec4 paint;
in vec2 local;
flat in int mode;
uniform vec2 attitude; // pitch and roll, radians; positive nose up / right wing down.
out vec4 color;
void main() {
    if(mode==1) {
        float edge=length(local);
        if(edge>1.0)discard;
        // Aircraft marker stays fixed. Rotate/shift the world relative to it.
        float level=-sin(attitude.y)*local.x+cos(attitude.y)*local.y+attitude.x/0.7853981634;
        float aa=max(fwidth(level),0.002);
        vec3 ground=vec3(0.29,0.15,0.07),sky=vec3(0.07,0.35,0.59);
        vec3 surface=mix(ground,sky,smoothstep(-aa,aa,level));
        surface=mix(surface,vec3(0.88),1.0-smoothstep(0.006,0.006+aa,abs(level)));
        color=vec4(surface,1.0-smoothstep(1.0-fwidth(edge),1.0,edge));
    } else {
        // A restrained procedural surface on dial geometry; no new bitmap assets.
        float grain=0.985+0.015*sin(gl_FragCoord.x*.25)*sin(gl_FragCoord.y*.25);
        color=vec4(paint.rgb*grain,paint.a);
    }
}
