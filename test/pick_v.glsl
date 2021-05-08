#version 430 core
layout(location=0) in vec4 vertex_pos;
uniform mat4 MVPMatrix;
out vec3 world_pos;
flat out int vectex_idx;
void main() {
    gl_Position=MVPMatrix*(vec4(vertex_pos.xyz,1.0));
    world_pos=vertex_pos.xyz;
    vectex_idx=int(vertex_pos.w);
}
