//
// Created by wyz on 2021/3/31.
//

#ifndef NEURONANNOTATION_VOLUMERENDERER_H
#define NEURONANNOTATION_VOLUMERENDERER_H

#include <memory>
#include <string>
#include <unordered_map>
#include "Camera.hpp"
#include "Image.hpp"
#include "TransferFunction.hpp"
#include "MouseKeyEvent.hpp"
class IRenderer;
using GetRendererBackend=IRenderer* (*)();
class VolumeRenderer{
public:
    explicit VolumeRenderer(const char* renderer_name);
    ~VolumeRenderer();
    VolumeRenderer(const VolumeRenderer&)=delete;
    VolumeRenderer(VolumeRenderer&&)=delete;
    VolumeRenderer* operator=(const VolumeRenderer&)=delete;

    auto get_renderer_name() const noexcept-> const char*;

    void set_volume(const char* path);

    void set_camera(Camera camera) noexcept;

    void set_transferfunc(TransferFunction tf) noexcept;

    void set_querypoint(std::array<uint32_t,2> screen_pos) noexcept;

    void set_mousekeyevent(MouseKeyEvent event) noexcept;

    void render_frame();

    auto get_frame()->const Image&;

    auto get_pos_frame()->const Map<float>&;

    auto get_querypoint()->const std::array<float,8> ;

    void clear_scene();

private:
    std::string renderer_name;
    std::unique_ptr<IRenderer> impl;
};


#endif //NEURONANNOTATION_VOLUMERENDERER_H
