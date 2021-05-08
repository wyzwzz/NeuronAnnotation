//
// Created by wyz on 2021/4/19.
//

#include"BlockVolumeRenderer.hpp"

#ifdef _WINDOWS
#include <Common/wgl_wrap.hpp>
#define WGL_NV_gpu_affinity
#else
#include<EGL/egl.h>
#include<EGL/eglext.h>
const EGLint egl_config_attribs[] = {EGL_SURFACE_TYPE,
                                     EGL_PBUFFER_BIT,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_DEPTH_SIZE,
                                     8,
                                     EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_BIT,
                                     EGL_NONE};



void EGLCheck(const char *fn) {
    EGLint error = eglGetError();

    if (error != EGL_SUCCESS) {
        throw runtime_error(fn + to_string(error));
    }
}
#endif

#include <cudaGL.h>

#include <Common/transferfunction_impl.h>
#include <Common/help_gl.hpp>
#include <random>

#include "Shaders.hpp"

extern "C"
{
    WIN_API IRenderer* get_renderer(){
        return reinterpret_cast<IRenderer*>(new BlockVolumeRenderer());
    }
}
BlockVolumeRenderer::BlockVolumeRenderer(int w, int h)
:window_width(w),window_height(h),query(false)
{
    if(w>2048 || h>2048 || w<1 || h<1){
        throw std::runtime_error("bad width or height");
    }
    initResourceContext();
    setupSystemInfo();
}

BlockVolumeRenderer::~BlockVolumeRenderer() {

}

void BlockVolumeRenderer::set_volume(const char *path) {
    volume_manager.reset(nullptr);
    volume_manager=std::make_unique<BlockVolumeManager>(path);
    setupVolumeInfo();
    createResource();
}

void BlockVolumeRenderer::set_camera(Camera camera) noexcept {
    this->camera=camera;
    glm::vec3 camera_pos={camera.pos[0],camera.pos[1],camera.pos[2]};
    glm::vec3 view_direction={camera.front[0],camera.front[1],camera.front[2]};
    glm::vec3 up={camera.up[0],camera.up[1],camera.up[2]};
    glm::vec3 right=glm::normalize(glm::cross(view_direction,up));
    auto center_pos=camera_pos+view_direction*(camera.f+camera.n)/2.f;
    this->view_obb=sv::OBB(center_pos,right,up,view_direction,
                           camera.f*tanf(glm::radians(camera.zoom/2))*window_width/window_height,
                           camera.f*tanf(glm::radians(camera.zoom/2)) ,
                           (camera.f-camera.n)/2.f);
//    print_array(camera.pos);
//    print_array(camera.front);
//    print_array(camera.up);
//    print_vec(center_pos);
//    std::cout<<std::endl;
}

void BlockVolumeRenderer::set_transferfunc(TransferFunction tf) noexcept {
    std::map<uint8_t ,std::array<double,4>> color_setting;
    assert(tf.colors.size()==tf.points.size());
    for(int i=0;i<tf.points.size();i++)
        color_setting[tf.points[i]]=tf.colors[i];
    sv::TransferFunc tf_impl(color_setting);

    glGenTextures(1,&transfer_func_tex);
    glBindTexture(GL_TEXTURE_1D,transfer_func_tex);
//    glCreateTextures(GL_TEXTURE_1D,1,&transfer_func_tex);
//    GL_EXPR(glBindTextureUnit(B_TF_TEX_BINDING,transfer_func_tex));
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D,0,GL_RGBA,TF_DIM,0,GL_RGBA,GL_FLOAT,tf_impl.getTransferFunction().data());

    glGenTextures(1,&preInt_tf_tex);
    glBindTexture(GL_TEXTURE_2D,preInt_tf_tex);
//    glBindTextureUnit(B_PTF_TEX_BINDING,preInt_tf_tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,TF_DIM,TF_DIM,0,GL_RGBA,GL_FLOAT, tf_impl.getPreIntTransferFunc().data());

}

void BlockVolumeRenderer::set_mousekeyevent(MouseKeyEvent event) noexcept {

}
void BlockVolumeRenderer::set_querypoint(std::array<uint32_t, 2> screen_pos) noexcept {
    this->query_point=screen_pos;
    query=true;
}


void BlockVolumeRenderer::render_frame() {
    START_CPU_TIMER
    GL_CHECK
    setupRuntimeResource();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    bool intersect=updateCurrentBlocks(view_obb);

    if(intersect){

        updateNewNeedBlocksInCache();

        volume_manager->setupBlockReqInfo(getBlockRequestInfo());

        BlockDesc block;
        while(volume_manager->getBlock(block)){

            copyDeviceToTexture(block.data_ptr,block.block_index);

            volume_manager->updateCUMemPool(block.data_ptr);

            block.release();
        }

        updateMappingTable();

    }
    else{
        std::cout<<"no intersect!"<<std::endl;
    }

    render_volume();

    glFinish();

    query=false;

    GL_CHECK
    END_CPU_TIMER
}

auto BlockVolumeRenderer::get_frame() -> const Image & {
    frame.width=window_width;
    frame.height=window_height;
    frame.channels=3;
    frame.data.resize(window_width*window_height*3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadPixels(0, 0, frame.width, frame.height, GL_RGB, GL_UNSIGNED_BYTE,
                 reinterpret_cast<void *>(frame.data.data()));
    return frame;
}
auto BlockVolumeRenderer::get_querypoint() -> const std::array<float, 8> {

    return std::array<float, 8>{query_point_result[0],
                                query_point_result[1],
                                query_point_result[2],
                                query_point_result[3],
                                query_point_result[4],
                                query_point_result[5],
                                query_point_result[6],
                                query_point_result[7]};
}

void BlockVolumeRenderer::clear_scene() {

}

void BlockVolumeRenderer::initResourceContext() {
    initGL();
    initCUDA();
}

void BlockVolumeRenderer::createResource() {
    createGLResource();
    createCUDAResource();
    createUtilResource();
}

void BlockVolumeRenderer::initGL() {
#ifdef _WINDOWS
    auto ins=GetModuleHandle(NULL);
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    std::string idx=std::to_string(dist(rng));
    HWND window=create_window(ins,("wgl_invisable"+idx).c_str(),window_width,window_height);
    this->window_handle=GetDC(window);
    this->gl_context=create_opengl_context(this->window_handle);
#else
    static const int MAX_DEVICES = 4;
    EGLDeviceEXT egl_devices[MAX_DEVICES];
    EGLint num_devices;

    auto eglQueryDevicesEXT =
            (PFNEGLQUERYDEVICESEXTPROC)eglGetProcAddress("eglQueryDevicesEXT");
    eglQueryDevicesEXT(4, egl_devices, &num_devices);

    auto eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress(
                    "eglGetPlatformDisplayEXT");

    auto m_egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT,
                                                  egl_devices[0], nullptr);
    EGLCheck("eglGetDisplay");

    EGLint major, minor;
    eglInitialize(m_egl_display, &major, &minor);
    EGLCheck("eglInitialize");

    EGLint num_configs;
    EGLConfig egl_config;

    eglChooseConfig(m_egl_display, egl_config_attribs, &egl_config, 1,
                    &num_configs);
    EGLCheck("eglChooseConfig");

    const EGLint pbuffer_attribs[] = {
            EGL_WIDTH, (EGLint)window_width, EGL_HEIGHT, (EGLint)window_height, EGL_NONE,
    };
    EGLSurface egl_surface =
            eglCreatePbufferSurface(m_egl_display, egl_config, pbuffer_attribs);
    EGLCheck("eglCreatePbufferSurface");

    eglBindAPI(EGL_OPENGL_API);
    EGLCheck("eglBindAPI");

    const EGLint context_attri[] = {EGL_CONTEXT_MAJOR_VERSION,
                                    4,
                                    EGL_CONTEXT_MINOR_VERSION,
                                    6,
                                    EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                    EGL_NONE};
    EGLContext egl_context = eglCreateContext(m_egl_display, egl_config,
                                              EGL_NO_CONTEXT, context_attri);
    EGLCheck("eglCreateContext");

    eglMakeCurrent(m_egl_display, egl_surface, egl_surface, egl_context);
    EGLCheck("eglMakeCurrent");

    if (!gladLoadGLLoader((void *(*)(const char *))(&eglGetProcAddress))) {
        throw runtime_error("Failed to load gl");
    }

    std::cout << "OpenGLVolumeRenderer: Detected " << std::to_string(num_devices)
              << " devices, using first one, OpenGL "
                 "version "
              << std::to_string(GLVersion.major) << "."
              << std::to_string(GLVersion.minor) << std::endl;
    glEnable(GL_DEPTH_TEST);

#endif
}

void BlockVolumeRenderer::initCUDA() {
    CUDA_DRIVER_API_CALL(cuInit(0));
    CUdevice cuDevice=0;
    CUDA_DRIVER_API_CALL(cuDeviceGet(&cuDevice, 0));
    CUDA_DRIVER_API_CALL(cuCtxCreate(&cu_context,0,cuDevice));
}

void BlockVolumeRenderer::setupSystemInfo() {

    vol_tex_block_nx=6;
    vol_tex_block_ny=4;
    vol_tex_num=3;
}

void BlockVolumeRenderer::setupVolumeInfo() {
    auto volume_info=this->volume_manager->getVolumeInfo();
    this->block_length=std::get<0>(volume_info);
    this->padding=std::get<1>(volume_info);
    this->block_dim=std::get<2>(volume_info);
    this->no_padding_block_length=this->block_length-2*this->padding;
}

void BlockVolumeRenderer::createGLResource() {
    createGLTexture();
    createGLSampler();
    createVolumeTexManager();
    createMappingTable();
    createScreenQuad();
    createGLShader();


}

void BlockVolumeRenderer::createCUDAResource() {
    createCUgraphics();
}

void BlockVolumeRenderer::createUtilResource() {
    createVirtualBoxes();
    createQueryPoint();
}

void BlockVolumeRenderer::createVolumeTexManager() {
    spdlog::debug("{0}",__FUNCTION__ );
    assert(vol_tex_num == volume_texes.size());
    for(uint32_t i=0;i<vol_tex_block_nx;i++){
        for(uint32_t j=0;j<vol_tex_block_ny;j++){
            for(uint32_t k=0; k < vol_tex_num; k++){
                BlockTableItem item;
                item.pos_index={i,j,k};
                item.block_index={INVALID,INVALID,INVALID};
                item.valid=false;
                item.cached=false;
                volume_tex_manager.push_back(std::move(item));
            }
        }
    }
}

void BlockVolumeRenderer::createMappingTable() {
    spdlog::info("{0}",__FUNCTION__ );
    mapping_table.assign(block_dim[0]*block_dim[1]*block_dim[2]*4,0);
    glGenBuffers(1,&mapping_table_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,mapping_table_ssbo);
    spdlog::info("mapping table size: {0}",mapping_table.size());
//    std::cout<<"mapping table size: "<<mapping_table.size()<<std::endl;
    GL_EXPR(glBufferData(GL_SHADER_STORAGE_BUFFER,mapping_table.size()*sizeof(uint32_t),mapping_table.data(),GL_DYNAMIC_READ));
    //binding point = 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,mapping_table_ssbo);
    GL_CHECK
}

void BlockVolumeRenderer::createScreenQuad() {
    spdlog::debug("{0}",__FUNCTION__ );
    screen_quad_vertices={
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1,&screen_quad_vao);
    glGenBuffers(1,&screen_quad_vbo);
    glBindVertexArray(screen_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER,screen_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(screen_quad_vertices),screen_quad_vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void BlockVolumeRenderer::createGLTexture() {
    spdlog::debug("{0}",__FUNCTION__ );
    assert(block_length && vol_tex_block_nx && vol_tex_block_ny && vol_tex_num);
    assert(volume_texes.size()==0);
    volume_texes.assign(vol_tex_num, 0);

    glCreateTextures(GL_TEXTURE_3D, vol_tex_num, volume_texes.data());
    for(int i=0; i < vol_tex_num; i++){
//        GL_EXPR(glBindTextureUnit(i+2,volume_texes[i]));
        glTextureStorage3D(volume_texes[i],1,GL_R8,vol_tex_block_nx*block_length,
                           vol_tex_block_ny*block_length,
                           block_length);
    }
    GL_CHECK
}

void BlockVolumeRenderer::createGLSampler() {
    spdlog::debug("{0}",__FUNCTION__ );
    GL_EXPR(glCreateSamplers(1,&gl_sampler));
    GL_EXPR(glSamplerParameterf(gl_sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR));
    GL_EXPR(glSamplerParameterf(gl_sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR));
    float color[4]={0.f,0.f,0.f,0.f};
    glSamplerParameterf(gl_sampler,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_BORDER);
    glSamplerParameterf(gl_sampler,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glSamplerParameterf(gl_sampler,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);

    GL_EXPR(glSamplerParameterfv(gl_sampler,GL_TEXTURE_BORDER_COLOR,color));
}
void BlockVolumeRenderer::createGLShader() {
    raycasting_shader=std::make_unique<sv::Shader>("../../src/Render/Shaders/block_raycast_v.glsl",
                                                   "../../src/Render/Shaders/block_raycast_f.glsl");
//    raycasting_shader->setShader(shader::mix_block_raycast_v,shader::mix_block_raycast_f,nullptr);

}

void BlockVolumeRenderer::createCUgraphics() {
    spdlog::info("{0}",__FUNCTION__ );
    assert(vol_tex_num == volume_texes.size() && vol_tex_num != 0);
    cu_resources.resize(volume_texes.size());
    for(int i=0;i<volume_texes.size();i++){
        std::cout<<"cuda register "<<i<<" : "<<volume_texes[i]<<std::endl;
        //max texture memory is 3GB for register cuda image for rtx3090
        CUDA_DRIVER_API_CALL(cuGraphicsGLRegisterImage(&cu_resources[i], volume_texes[i],GL_TEXTURE_3D, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    }
}

void BlockVolumeRenderer::createVirtualBoxes() {
    spdlog::info("{0}",__FUNCTION__ );
    for(uint32_t z=0;z<block_dim[2];z++){
        for(uint32_t y=0;y<block_dim[1];y++){
            for(uint32_t x=0;x<block_dim[0];x++){
                virtual_blocks.emplace_back(glm::vec3(x*no_padding_block_length,y*no_padding_block_length,z*no_padding_block_length),
                                            glm::vec3((x+1)*no_padding_block_length,(y+1)*no_padding_block_length,(z+1)*no_padding_block_length),
                                            std::array<uint32_t,3>{x,y,z});
            }
        }
    }
//    std::cout<<"finish "<<__FUNCTION__ <<std::endl;
}

void BlockVolumeRenderer::setupRuntimeResource() {
    bindGLTextureUnit();
    setupShaderUniform();
}

#define B_TF_TEX_BINDING 0
#define B_PTF_TEX_BINDING 1
#define B_VOL_TEX_0_BINDING 2
#define B_VOL_TEX_1_BINDING 3
#define B_VOL_TEX_2_BINDING 4

void BlockVolumeRenderer::setupShaderUniform() {
//    spdlog::info("{0}",__FUNCTION__ );
    raycasting_shader->use();
    raycasting_shader->setInt("transfer_func",B_TF_TEX_BINDING);
    raycasting_shader->setInt("preInt_transfer_func",B_PTF_TEX_BINDING);
    raycasting_shader->setInt("cache_volume0",B_VOL_TEX_0_BINDING);
    raycasting_shader->setInt("cache_volume1",B_VOL_TEX_1_BINDING);
    raycasting_shader->setInt("cache_volume2",B_VOL_TEX_2_BINDING);
    raycasting_shader->setInt("window_width",window_width);
    raycasting_shader->setInt("window_height",window_height);
    glm::vec3 camera_pos={camera.pos[0],camera.pos[1],camera.pos[2]};
    glm::vec3 view_direction={camera.front[0],camera.front[1],camera.front[2]};
    glm::vec3 up={camera.up[0],camera.up[1],camera.up[2]};
    glm::vec3 right=glm::normalize(glm::cross(view_direction,up));
    raycasting_shader->setVec3("camera_pos",camera_pos);
    raycasting_shader->setVec3("view_pos",camera_pos+view_direction*camera.n);
    raycasting_shader->setVec3("view_direction",view_direction);
    raycasting_shader->setVec3("view_right",right);
    raycasting_shader->setVec3("view_up",up);
    raycasting_shader->setFloat("view_depth",camera.f);
    float space=camera.f*tanf(glm::radians(camera.zoom/2))*2/window_height;
    raycasting_shader->setFloat("view_right_space",space);
    raycasting_shader->setFloat("view_up_space",space);
    raycasting_shader->setFloat("step",0.3f);

    raycasting_shader->setVec4("bg_color",0.f,0.f,0.f,0.f);
    raycasting_shader->setInt("block_length",block_length);
    raycasting_shader->setInt("padding",padding);
    raycasting_shader->setIVec3("block_dim",block_dim[0],block_dim[1],block_dim[2]);
    raycasting_shader->setIVec3("texture_size3",vol_tex_block_nx*block_length,
                                vol_tex_block_ny*block_length,block_length);
    raycasting_shader->setFloat("ka",0.5f);
    raycasting_shader->setFloat("kd",0.8f);
    raycasting_shader->setFloat("shininess",100.0f);
    raycasting_shader->setFloat("ks",1.0f);
    raycasting_shader->setVec3("light_direction",glm::normalize(glm::vec3(-1.0f,-1.0f,-1.0f)));

    if(query){
        glm::ivec2 _query_point={query_point[0],window_height-query_point[1]};
        raycasting_shader->setIVec2("query_point",_query_point);
        raycasting_shader->setBool("query",true);
    }
    else{
        raycasting_shader->setBool("query",false);
    }
}


void BlockVolumeRenderer::bindGLTextureUnit() {
//    spdlog::info("{0}",__FUNCTION__ );
    GL_EXPR(glBindTextureUnit(B_TF_TEX_BINDING,transfer_func_tex));
    GL_EXPR(glBindTextureUnit(B_PTF_TEX_BINDING,preInt_tf_tex));
    GL_EXPR(glBindTextureUnit(B_VOL_TEX_0_BINDING,volume_texes[0]));
    GL_EXPR(glBindTextureUnit(B_VOL_TEX_1_BINDING,volume_texes[1]));
    GL_EXPR(glBindTextureUnit(B_VOL_TEX_2_BINDING,volume_texes[2]));

    GL_EXPR(glBindSampler(B_VOL_TEX_0_BINDING,gl_sampler));
    glBindSampler(B_VOL_TEX_1_BINDING,gl_sampler);
    glBindSampler(B_VOL_TEX_2_BINDING,gl_sampler);
    GL_CHECK
}

void BlockVolumeRenderer::deleteResource() {
    deleteCUDAResource();
    deleteGLResource();
    deleteUtilResource();
}

void BlockVolumeRenderer::deleteGLResource() {
    glDeleteBuffers(1,&mapping_table_ssbo);
    glDeleteSamplers(1,&gl_sampler);
    glDeleteTextures(1,&transfer_func_tex);
    glDeleteTextures(1,&preInt_tf_tex);
    glDeleteVertexArrays(1,&screen_quad_vao);
    glDeleteBuffers(1,&screen_quad_vbo);
    glDeleteTextures(volume_texes.size(),volume_texes.data());
    GL_CHECK
}

void BlockVolumeRenderer::deleteCUDAResource() {
    spdlog::info("{0}",__FUNCTION__ );
    assert(volume_texes.size()==cu_resources.size() && vol_tex_num == volume_texes.size() && vol_tex_num != 0);
    for(int i=0;i<cu_resources.size();i++){
        CUDA_DRIVER_API_CALL(cuGraphicsUnregisterResource(cu_resources[i]));
    }

}

void BlockVolumeRenderer::deleteUtilResource() {

}

bool BlockVolumeRenderer::updateCurrentBlocks(const sv::OBB &view_box) {
    auto aabb=view_box.getAABB();
    std::unordered_set<sv::AABB,Myhash> current_intersect_blocks;
//    print(aabb);
    for(auto& it:virtual_blocks){
        if(aabb.intersect(it)){
            current_intersect_blocks.insert(it);
//            print(it);
        }
    }
//    std::cout<<"current_intersect_blocks size: " <<current_intersect_blocks.size()<<std::endl;
    if(current_intersect_blocks.empty()){
        //no intersect, so no need to draw
        return false;
    }
    assert(new_need_blocks.empty());
    assert(no_need_blocks.empty());
//    std::cout<<"last intersect block num: "<<current_blocks.size()<<std::endl;
    for(auto& it:current_intersect_blocks){
        if(current_blocks.find(it)==current_blocks.cend()){
            new_need_blocks.insert(it);
        }
    }

    for(auto& it:current_blocks){
        if(current_intersect_blocks.find(it)==current_intersect_blocks.cend()){
            no_need_blocks.insert(it);
        }
    }
//    std::cout<<"new size: "<<new_need_blocks.size()<<std::endl;
//    std::cout<<"no size: "<<no_need_blocks.size()<<std::endl;
    current_blocks=std::move(current_intersect_blocks);

    for(auto& it:volume_tex_manager){
        auto t=sv::AABB(it.block_index);
        //not find
        if(current_blocks.find(t)==current_blocks.cend()){
            it.valid=false;
            //update mapping_table
//            print_array(it.block_index);
            if(it.block_index[0]!=INVALID){
                uint32_t flat_idx=it.block_index[2]*block_dim[0]*block_dim[1]+it.block_index[1]*block_dim[0]+it.block_index[0];
                mapping_table[flat_idx*4+3]=0;
            }
        }
        else{
            assert(it.valid==true || (it.valid==false && it.cached==true));
        }
    }
    return true;
}

void BlockVolumeRenderer::updateNewNeedBlocksInCache() {
    for(auto& it:volume_tex_manager){
        //find cached but invalid block in texture
        if(it.cached && !it.valid){
            auto t=sv::AABB(it.block_index);
            if(new_need_blocks.find(t)!=new_need_blocks.cend()){//find!
                it.valid=true;

                uint32_t flat_idx=it.block_index[2]*block_dim[0]*block_dim[1]+it.block_index[1]*block_dim[0]+it.block_index[0];
                mapping_table[flat_idx*4+0]=it.pos_index[0];
                mapping_table[flat_idx*4+1]=it.pos_index[1];
                mapping_table[flat_idx*4+2]=it.pos_index[2];
                mapping_table[flat_idx*4+3]=1;

                new_need_blocks.erase(t);
            }
        }
    }
    updateMappingTable();
}

auto BlockVolumeRenderer::getBlockRequestInfo() -> BlockReqInfo {
    BlockReqInfo request;
    for(auto& it:new_need_blocks){
        request.request_blocks_queue.push_back(it.index);
    }
    for(auto& it:no_need_blocks){
        request.noneed_blocks_queue.push_back(it.index);
    }
//    std::cout<<"new_need_blocks size: "<<new_need_blocks.size()<<std::endl;
    new_need_blocks.clear();
    no_need_blocks.clear();
    return request;
}

void BlockVolumeRenderer::copyDeviceToTexture(CUdeviceptr ptr, std::array<uint32_t, 3> idx) {
    spdlog::info("{0}",__FUNCTION__ );
    //inspect if idx is still in current_blocks

    //getTexturePos() set valid=false cached=false

    std::array<uint32_t,3> tex_pos_index;
    bool cached=getTexturePos(idx,tex_pos_index);


//    print_array(tex_pos_index);

    if(!cached){
        assert(tex_pos_index[2] < vol_tex_num && tex_pos_index[0] < vol_tex_block_nx
               && tex_pos_index[1]<vol_tex_block_ny);

        CUDA_DRIVER_API_CALL(cuGraphicsMapResources(1, &cu_resources[tex_pos_index[2]], 0));

        CUarray cu_array;

        CUDA_DRIVER_API_CALL(cuGraphicsSubResourceGetMappedArray(&cu_array,cu_resources[tex_pos_index[2]],0,0));



        CUDA_MEMCPY3D m = { 0 };
        m.srcMemoryType=CU_MEMORYTYPE_DEVICE;
        m.srcDevice=ptr;


        m.dstMemoryType=CU_MEMORYTYPE_ARRAY;
        m.dstArray=cu_array;
        m.dstXInBytes=tex_pos_index[0]*block_length;
        m.dstY=tex_pos_index[1]*block_length;
        m.dstZ=0;

        m.WidthInBytes=block_length;
        m.Height=block_length;
        m.Depth=block_length;

        CUDA_DRIVER_API_CALL(cuMemcpy3D(&m));

        CUDA_DRIVER_API_CALL(cuGraphicsUnmapResources(1, &cu_resources[tex_pos_index[2]], 0));

//        spdlog::info("finish cuda opengl copy");
    }
    //update volume_tex_manager

    for(auto& it:volume_tex_manager){
        if(it.pos_index==tex_pos_index){
            if(cached){
                assert(it.cached==true && it.block_index==idx);
            }
            it.block_index=idx;
            it.valid=true;
            it.cached=true;
        }
    }

    //update mapping_table
    uint32_t flat_idx=idx[2]*block_dim[0]*block_dim[1]+idx[1]*block_dim[0]+idx[0];
    mapping_table[flat_idx*4+0]=tex_pos_index[0];
    mapping_table[flat_idx*4+1]=tex_pos_index[1];
    mapping_table[flat_idx*4+2]=tex_pos_index[2];
    mapping_table[flat_idx*4+3]=1;
//    spdlog::info("float_idx: {0}",flat_idx);
//    spdlog::info("idx: {0} {1} {2}",idx[0],idx[1],idx[2]);
//    spdlog::info("tex_pox_index:{0} {1} {2}",tex_pos_index[0],tex_pos_index[1],tex_pos_index[2]);
}

bool BlockVolumeRenderer::getTexturePos(const array<uint32_t, 3> & idx, array<uint32_t, 3> &pos) {
    for(auto&it :volume_tex_manager){
        if(it.block_index==idx && it.cached && !it.valid){
            pos=it.pos_index;
            return true;
        }
        else if(it.block_index==idx && !it.cached && !it.valid){
            pos=it.pos_index;
            return false;
        }
    }
    for(auto& it:volume_tex_manager){
        if(!it.valid && !it.cached){
            pos=it.pos_index;
            return false;
        }
    }
    for(auto& it:volume_tex_manager){
        if(!it.valid){
            pos=it.pos_index;
            return false;
        }
    }
    spdlog::critical("not find empty texture pos");
    throw std::runtime_error("not find empty texture pos");
}

void BlockVolumeRenderer::updateMappingTable() {
    GL_EXPR(glNamedBufferSubData(mapping_table_ssbo,0,mapping_table.size()*sizeof(uint32_t),mapping_table.data()));

}

void BlockVolumeRenderer::render_volume() {
    raycasting_shader->use();

    glBindVertexArray(screen_quad_vao);

    glDrawArrays(GL_TRIANGLES,0,6);
}

void BlockVolumeRenderer::createQueryPoint() {

    glGenBuffers(1,&query_point_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER,query_point_ssbo);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,sizeof(float)*8,nullptr,GL_MAP_WRITE_BIT|GL_MAP_PERSISTENT_BIT|GL_MAP_COHERENT_BIT);
    query_point_result=(float*)glMapBuffer(GL_SHADER_STORAGE_BUFFER,GL_WRITE_ONLY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,query_point_ssbo);

    GL_CHECK
}



