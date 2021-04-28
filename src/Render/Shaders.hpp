#pragma once

namespace shader{

    const char* mix_block_raycast_v=R"(#version 430 core
layout(location=0) in vec2 vertex_pos;
void main()
{
    gl_Position=vec4(vertex_pos,0.0f,1.0f);
}
)";

    const char* mix_block_raycast_f=R"(#version 430 core

out vec4 frag_color;

uniform sampler1D transfer_func;
uniform sampler2D preInt_transfer_func;
uniform sampler3D cache_volume0;
uniform sampler3D cache_volume1;
uniform sampler3D cache_volume2;

layout(std430,binding=0) buffer MappingTable{
    uvec4 page_entry[];
}mapping_table;

uniform float ka;
uniform float kd;
uniform float shininess;
uniform float ks;

uniform int window_width;
uniform int window_height;


uniform float step;
uniform float view_depth;
uniform vec3 view_pos;//near plane center not camera pos
uniform vec3 camera_pos;
uniform vec3 view_direction;//assert normalized
uniform vec3 view_right;//assert normalized
uniform vec3 view_up;//assert normalized
uniform float view_right_space;//measure in raw volume world, best is 1
uniform float view_up_space;
uniform vec3 light_direction;//assert normalized
uniform vec4 bg_color;


uniform int block_length;
uniform int padding;
uniform ivec3 block_dim;
uniform ivec3 texture_size3;//size3 for cache_volume012

bool virtualSample(vec3 samplePos,out vec4 scalar);

vec3 phongShading(vec3 samplePos,vec3 diffuseColor);
vec3 getVirtualOffset(vec3 samplePos);
void main()
{
    int x_pos=int(gl_FragCoord.x)-window_width/2;
    int y_pos=int(gl_FragCoord.y)-window_height/2;
    vec3 ray_start_pos=view_pos+x_pos*view_right*view_right_space+y_pos*view_up*view_up_space;
    vec3 ray_stop_pos=ray_start_pos+view_direction*view_depth;
    int steps=int(view_depth/step);
    vec3 ray_direction;

    ray_direction=normalize(ray_stop_pos-camera_pos);


        frag_color=vec4(ray_direction,0.f);
        return;
    vec4 color=vec4(0.f);
    vec4 last_sample_scalar=vec4(0.f);
    vec4 sample_scalar;
    vec4 sample_color;
    vec3 sample_start_pos;
    vec3 sample_pos;

    sample_start_pos=camera_pos;

    sample_pos=sample_start_pos;
    for(int i=0;i<steps;i++){
        bool continued=virtualSample(sample_pos,sample_scalar);

        if(!continued)
        break;

        if(sample_scalar.r>0.3f){

            //            frag_color=vec4(getVirtualOffset(sample_pos)/(block_length-2*padding),1.f);
            //            return ;

            sample_color=texture(transfer_func,sample_scalar.r);
            sample_color=texture(preInt_transfer_func,vec2(last_sample_scalar.r,sample_scalar.r));
            sample_color.rgb=phongShading(sample_pos,sample_color.rgb);
            sample_color.a*=(steps*steps-i*i)*1.f/(steps*steps);
            color=color+sample_color*vec4(sample_color.aaa,1.f)*(1.f-color.a);

            if(color.a>0.9f)
            break;
            last_sample_scalar=sample_scalar;
        }
        sample_pos=sample_start_pos+i*ray_direction*step;
    }

    if(color.a==0.f) discard;
    color+=bg_color*(1.f-color.a);

    frag_color=color;
    //    frag_color=vec4(view_direction,1.f);

}
//debug use
vec3 getVirtualOffset(vec3 samplePos){
    //1. first get virtual_index in block-dim
    int no_padding_block_length=block_length-2*padding;
    ivec3 virtual_block_idx=ivec3(samplePos/no_padding_block_length);

    //2. get samplePos's offset in the block
    vec3 offset_in_no_padding_block=samplePos-virtual_block_idx*no_padding_block_length;
    return offset_in_no_padding_block;
}
// samplePos is raw world pos, measure in dim(raw_x,raw_y,raw_z)
bool virtualSample(vec3 samplePos,out vec4 scalar)
{
    //1. first get virtual_index in block-dim
    int no_padding_block_length=block_length-2*padding;
    ivec3 virtual_block_idx=ivec3(samplePos/no_padding_block_length);
    if(samplePos.x<0.f || virtual_block_idx.x>=block_dim.x
    || samplePos.y<0.f || virtual_block_idx.y>=block_dim.y
    || samplePos.z<0.f || virtual_block_idx.z>=block_dim.z){
        //shader meagings view-space intersect with volume, should return true to keep raycasting
        scalar=vec4(0.f);
        return true;
    }
    //2. get samplePos's offset in the block
    vec3 offset_in_no_padding_block=samplePos-virtual_block_idx*no_padding_block_length;
    //3. get block-dim in texture
    int flat_virtual_block_idx=virtual_block_idx.z*block_dim.y*block_dim.x
    +virtual_block_idx.y*block_dim.x+virtual_block_idx.x;
    uvec4 physical_block_idx=mapping_table.page_entry[flat_virtual_block_idx];
    //valid block cached in texture
    if(physical_block_idx.w==1){
        //4.get sample_pos in the texture
        vec3 pyhsical_sampel_pos=vec3(physical_block_idx.xy,0)*block_length
        +offset_in_no_padding_block+vec3(padding);
        pyhsical_sampel_pos/=texture_size3;
        uint texture_id=physical_block_idx.z;
        if(texture_id==0){
            scalar=texture(cache_volume0,pyhsical_sampel_pos);
        }
        else if(texture_id==1){
            scalar=texture(cache_volume1,pyhsical_sampel_pos);
        }
        else if(texture_id==2){
            scalar=texture(cache_volume2,pyhsical_sampel_pos);
        }
        else{
            scalar=vec4(0.65f);
            return false;
        }

        return true;
    }
    else{//invalid block, no cached in texture
        scalar=vec4(0.65f);
        return false;
    }

}

vec3 phongShading(vec3 samplePos,vec3 diffuseColor)
{
    vec3 N;
    #define CUBIC
    #ifdef CUBIC
    float value[27];
    float t1[9];
    float t2[3];
    for(int k=-1;k<2;k++){//z
        for(int j=-1;j<2;j++){//y
            for(int i=-1;i<2;i++){//x
                vec4 scalar;
                virtualSample(samplePos+vec3(i,j,k),scalar);
                value[(k+1)*9+(j+1)*3+i+1]=scalar.r;
            }
        }
    }
    int x,y,z;
    //for x-direction
    for(z=0;z<3;z++){
        for(y=0;y<3;y++){
            t1[z*3+y]=(value[18+y*3+z]-value[y*3+z])/2;
        }
    }
    for(z=0;z<3;z++)
    t2[z]=(t1[z*3+0]+4*t1[z*3+1]+t1[z*3+2])/6;
    N.x=(t2[0]+t2[1]*4+t2[2])/6;


    //for y-direction
    for(z=0;z<3;z++){
        for(x=0;x<3;x++){
            t1[z*3+x]=(value[x*9+6+z]-value[x*9+z])/2;
        }
    }
    for(z=0;z<3;z++)
    t2[z]=(t1[z*3+0]+4*t1[z*3+1]+t1[z*3+2])/6;
    N.y=(t2[0]+t2[1]*4+t2[2])/6;

    //for z-direction
    for(y=0;y<3;y++){
        for(x=0;x<3;x++){
            t1[y*3+x]=(value[x*9+y*3+2]-value[x*9+y*3])/2;
        }
    }
    for(y=0;y<3;y++)
    t2[y]=(t1[y*3+0]+4*t1[y*3+1]+t1[y*3+2])/6;
    N.z=(t2[0]+t2[1]*4+t2[2])/6;
    #else
    //    N.x=value[14]-value[12];
    //    N.y=value[16]-value[10];
    //    N.z=value[22]-value[4];
    //    N.x=(virtualSample(volume_data,samplePos+vec3(step,0,0)).r-virtualSample(volume_data,samplePos+vec3(-step,0,0)).r);
    //    N.y=(virtualSample(volume_data,samplePos+vec3(0,step,0)).r-virtualSample(volume_data,samplePos+vec3(0,-step,0)).r);
    //    N.z=(virtualSample(volume_data,samplePos+vec3(0,0,step)).r-virtualSample(volume_data,samplePos+vec3(0,0,-step)).r);
    #endif

    N=-normalize(N);


    vec3 L=-view_direction;
    vec3 R=-view_direction;//-ray_direction;

    if(dot(N,L)<0.f)
    N=-N;

    vec3 ambient=ka*diffuseColor.rgb;
    vec3 specular=ks*pow(max(dot(N,(L+R)/2.0),0.0),shininess)*vec3(1.0f);
    vec3 diffuse=kd*max(dot(N,L),0.0)*diffuseColor.rgb;
    return ambient+specular+diffuse;
})";
}
