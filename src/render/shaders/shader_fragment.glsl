#version 330 core
// AI-assisted OBJ/MTL shading: imported UVs, diffuse colors and alpha.
in vec4 position_world;
in vec4 position_model;
in vec4 normal;
in vec2 texcoords;
uniform mat4 view;
uniform vec3 material_diffuse;
uniform float material_opacity;
uniform bool has_diffuse_texture;
uniform sampler2D diffuse_texture;
uniform vec3 camera_position;
uniform int procedural_surface;
uniform float alpha_cutoff;
out vec4 color;

float surfaceDetail(vec3 p)
{
    // Object-local detail stays attached to the model; fade subpixel frequencies.
    float frequency = procedural_surface == 2 ? 4.0 : procedural_surface == 3 ? 22.0 : 10.0;
    vec3 q = p*frequency;
    float filtered = 1.0-smoothstep(0.25,1.0,length(fwidth(q)));
    float grain = sin(q.x*1.7+sin(q.z*1.3))*sin(q.y*2.1+q.z*2.9);
    return 0.98+0.02*grain*filtered;
}

void main()
{
    vec4 texel = has_diffuse_texture ? texture(diffuse_texture,texcoords) : vec4(1.0);
    if(!has_diffuse_texture && procedural_surface != 0)
        texel = vec4(vec3(surfaceDetail(position_model.xyz)),1.0);
    float alpha = texel.a * material_opacity;
    if(alpha < alpha_cutoff) discard;
    vec3 n = normalize(normal.xyz);
    if(!gl_FrontFacing) n = -n;
    vec3 sun = normalize(vec3(-0.4,0.85,-0.3));
    vec3 camera = camera_position;
    vec3 v = normalize(camera-position_world.xyz);
    float diffuse = max(dot(n,sun),0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n,normalize(sun+v)),0.0),32.0) : 0.0;
    vec3 lit = material_diffuse * texel.rgb * (0.35+0.65*diffuse) + 0.04*specular;
    vec3 displayColor = pow(max(lit,vec3(0)),vec3(1.0/2.2));
    float fog = smoothstep(8000.0,28000.0,length(camera-position_world.xyz));
    color = vec4(mix(displayColor,vec3(0.60,0.76,0.90),fog),alpha);
}
