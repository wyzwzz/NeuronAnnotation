//
// Created by wyz on 2021/4/19.
//

#ifndef NEURONANNOTATION_BLOCKVOLUMERENDERER_H
#define NEURONANNOTATION_BLOCKVOLUMERENDERER_H
#include "IRenderer.hpp"

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <Common/utils.hpp>
#include <unordered_set>
#include <Common/boundingbox.hpp>
#include <Common/help_cuda.hpp>

#include "ShaderProgram.hpp"
#include <Render/Data/BlockVolumeManager.hpp>

struct Myhash{
    std::size_t operator()(const sv::AABB& aabb) const {
        return (aabb.index[0]<<16)+(aabb.index[1]<<8)+aabb.index[2];
    }
};


class BlockVolumeRenderer final: public IRenderer{
public:
    BlockVolumeRenderer(int w=1200,int h=900);
    BlockVolumeRenderer(const BlockVolumeRenderer&)=delete;
    BlockVolumeRenderer(BlockVolumeRenderer&&)=delete;
    ~BlockVolumeRenderer();
public:
    void set_volume(const char* path) override;

    void set_camera(Camera camera) noexcept override;

    void set_transferfunc(TransferFunction tf) noexcept override;

    void set_mousekeyevent(MouseKeyEvent event) noexcept override;

    void render_frame() override;

    auto get_frame()->const Image& override;

    void clear_scene() override;

private:

    struct BlockTableItem{
        std::array<uint32_t,3> block_index;
        bool valid;
        std::array<uint32_t,3> pos_index;
        bool cached;
    };

    void initResourceContext();
    void initGL();
    void initCUDA();

    void setupSystemInfo();

    void setupVolumeInfo();

    void createResource();
    void createGLResource();
    void createCUDAResource();
    void createUtilResource();

    //createGLResource
    void createVolumeTexManager();
    void createScreenQuad();
    void createGLTexture();
    void createGLSampler();
    void createGLShader();

    //createCUDAResource
    void createCUgraphics();


    //createUtilResource
    void createVirtualBoxes();
    void createMappingTable();

    //setupRuntimeResource
    void setupRuntimeResource();
    void setupShaderUniform();
    void bindGLTextureUnit();


    //deleteResource
    void deleteResource();
    void deleteGLResource();
    void deleteCUDAResource();
    void deleteUtilResource();

private:
    void copyDeviceToTexture(CUdeviceptr,std::array<uint32_t,3>);
    bool updateCurrentBlocks(const sv::OBB& view_box);
    void updateNewNeedBlocksInCache();
    void updateMappingTable();
    bool getTexturePos(const std::array<uint32_t,3>&,std::array<uint32_t,3>&);
    void render_volume();
    auto getBlockRequestInfo()->BlockReqInfo;

public:
    void print_args();
private:
    uint32_t block_length,padding,no_padding_block_length;
    std::array<uint32_t,3> block_dim;

    uint32_t vol_tex_block_nx,vol_tex_block_ny;
    uint32_t vol_tex_num;//equal to volume texture num
    std::list<BlockTableItem> volume_tex_manager;
    std::vector<GLuint> volume_texes;


    std::vector<sv::AABB> virtual_blocks;

    GLuint gl_sampler;

    GLuint mapping_table_ssbo;
    std::vector<uint32_t> mapping_table;

    std::unordered_set<sv::AABB,Myhash> current_blocks;
    std::unordered_set<sv::AABB,Myhash> new_need_blocks,no_need_blocks;

    std::vector<CUgraphicsResource> cu_resources;
    CUcontext cu_context;

    GLuint transfer_func_tex;
    GLuint preInt_tf_tex;

    GLuint screen_quad_vao;
    GLuint screen_quad_vbo;
    std::array<GLfloat,24> screen_quad_vertices;//6 vertices for x y and u v

    std::unique_ptr<sv::Shader> raycasting_shader;

#ifdef _WINDOWS
    HDC window_handle;
    HGLRC gl_context;
#elif LINUX

#endif


    uint32_t window_width,window_height;

    sv::OBB view_obb;

    Camera camera;

    std::unique_ptr<BlockVolumeManager> volume_manager;

    Image frame;
};

extern "C"{
WIN_API   IRenderer* get_renderer();
}
#endif //NEURONANNOTATION_BLOCKVOLUMERENDERER_H
