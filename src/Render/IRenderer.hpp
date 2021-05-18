//
// Created by wyz on 2021/3/31.
//

#ifndef NEURONANNOTATION_IRENDERER_H
#define NEURONANNOTATION_IRENDERER_H
#include <Camera.hpp>
#include <Image.hpp>
#include <MouseKeyEvent.hpp>
#include <TransferFunction.hpp>
class IRenderer{
public:
    virtual void set_volume(const char* path) = 0;

    virtual void set_camera(Camera camera) noexcept=0;

    virtual void set_transferfunc(TransferFunction tf) noexcept=0;

    virtual void set_mousekeyevent(MouseKeyEvent event) noexcept=0;

    virtual void set_querypoint(std::array<uint32_t,2> screen_pos) noexcept=0;

    virtual void render_frame()=0;

    virtual auto get_frame()->const Image& = 0;

    virtual auto get_pos_frame()->const Map<float>& = 0;

    virtual auto get_querypoint()->const std::array<float,8> =0;

    virtual void clear_scene()=0;

};
#endif //NEURONANNOTATION_IRENDERER_H
