#version 330 core
// AI-assisted light lens mesh and physical-size camera-facing halo.
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
uniform mat3 view_rotation;
uniform mat4 projection;
uniform mat3 model_linear;
uniform mat3 model_normal;
uniform vec3 center_relative;
uniform float light_radius;
uniform int light_pass;
out vec3 local_position;
out vec3 normal_world;
out vec3 position_relative;

void main() {
    local_position=position;
    normal_world=model_normal*normal;
    position_relative=center_relative+model_linear*(position*light_radius);
    vec3 position_view=view_rotation*position_relative;
    if(light_pass==1) {
        // A view-space quad faces the camera and retains the anchor's depth.
        // Ordinary depth testing still occludes it behind wings and buildings.
        position_view=view_rotation*center_relative+vec3(position.xy*light_radius,0.0);
    }
    gl_Position=projection*vec4(position_view,1.0);
}
