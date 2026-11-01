#version 330 core
// AI-assisted OBJ/MTL shading: imported UVs, diffuse colors and alpha.
in vec4 position_world;
in vec4 normal;
in vec2 texcoords;
uniform mat4 view;
uniform vec3 material_diffuse;
uniform float material_opacity;
uniform bool has_diffuse_texture;
uniform sampler2D diffuse_texture;
out vec4 color;

void main()
{
    vec4 texel = has_diffuse_texture ? texture(diffuse_texture,texcoords) : vec4(1.0);
    float alpha = texel.a * material_opacity;
    if(alpha < 0.2) discard;
    vec3 n = normalize(normal.xyz);
    if(!gl_FrontFacing) n = -n;
    vec3 sun = normalize(vec3(-0.4,0.85,-0.3));
    vec3 camera = vec3(inverse(view)*vec4(0,0,0,1));
    vec3 v = normalize(camera-position_world.xyz);
    float diffuse = max(dot(n,sun),0.0);
    float specular = diffuse > 0.0 ? pow(max(dot(n,normalize(sun+v)),0.0),32.0) : 0.0;
    vec3 lit = material_diffuse * texel.rgb * (0.35+0.65*diffuse) + 0.04*specular;
    vec3 displayColor = pow(max(lit,vec3(0)),vec3(1.0/2.2));
    float fog = smoothstep(8000.0,28000.0,length(camera-position_world.xyz));
    color = vec4(mix(displayColor,vec3(0.60,0.76,0.90),fog),alpha);
}
