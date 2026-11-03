#version 330 core
// AI-assisted procedural glass lens and soft additive glow; no image textures.
in vec3 local_position;
in vec3 normal_world;
in vec3 position_relative;
uniform vec3 light_color;
uniform float light_intensity;
uniform int light_pass;
out vec4 color;

void main() {
    if(light_pass==1) {
        float radius2=dot(local_position.xy,local_position.xy);
        if(radius2>=1.0)discard;
        float falloff=exp(-4.0*radius2)*(1.0-smoothstep(0.55,1.0,radius2));
        color=vec4(light_color*1.35,0.48*light_intensity*falloff);
        return;
    }
    vec3 n=normalize(normal_world);
    vec3 sun=normalize(vec3(-0.4,0.85,-0.3));
    vec3 v=normalize(-position_relative);
    float diffuse=max(dot(n,sun),0.0);
    // FONTE (Phong reflection model): KhronosGroup/glTF,
    // extensions/1.0/Khronos/KHR_materials_common/README.md#phong
    vec3 reflected=reflect(-sun,n);
    float specular=diffuse>0.0?pow(max(dot(reflected,v),0.0),9.0):0.0;
    // Filter the fine lens ribs when the physical lens becomes subpixel.
    float ribCoordinate=local_position.y*32.0;
    float ribFilter=1.0-smoothstep(0.35,1.3,fwidth(ribCoordinate));
    float lensTexture=0.92+0.08*sin(ribCoordinate)*ribFilter;
    vec3 lens=mix(vec3(0.035),light_color,0.55)*lensTexture;
    vec3 lighting=lens*(0.35+0.65*diffuse)+vec3(0.16*specular);
    vec3 emission=light_color*light_intensity*(1.25+0.15*lensTexture);
    color=vec4(pow(max(lighting+emission,vec3(0.0)),vec3(1.0/2.2)),1.0);
}
