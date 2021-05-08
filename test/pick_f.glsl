#version 430 core
in vec3 world_pos;
flat in int vectex_idx;
out vec4 frag_color;
void main() {
    if(vectex_idx==1)
    frag_color=vec4(1.f,0.f,0.f,1.f);
    else if(vectex_idx==2)
    frag_color=vec4(0.f,1.f,0.f,1.f);
    else if(vectex_idx==3)
    frag_color=vec4(0.f,0.f,1.f,1.f);
    else if(vectex_idx==4)
    frag_color=vec4(1.f,1.f,0.f,1.f);
}
