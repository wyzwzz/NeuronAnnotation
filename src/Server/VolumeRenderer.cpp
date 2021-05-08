//
// Created by wyz on 2021/4/19.
//
#include<VolumeRenderer.hpp>
#include<Render/IRenderer.hpp>
#include<string>
#include<stdexcept>
#include<functional>

#ifdef _WINDOWS
#include <Windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR);
#else
#include <dlfcn.h>
#endif



VolumeRenderer::VolumeRenderer(const char *renderer) {
    renderer_name=renderer;
    std::string postfix;
#ifndef NDEBUG
    postfix = "d";
#endif

#ifndef _WINDOWS
    auto lib_name = std::string("lib") + renderer + postfix + ".so";
    void *lib = dlopen(lib_name.c_str(), RTLD_NOW);
    if (lib == nullptr) {
      throw std::runtime_error(dlerror());
    }
    void *symbol = dlsym(lib, "get_renderer");

    if (symbol == nullptr) {
      throw std::runtime_error("Cannot find symbol `voxer_get_renderer` in " +
                          lib_name);
    }
#else
    auto lib_name =  renderer + postfix + ".dll";
    wchar_t win_lib_name[100];
    swprintf(win_lib_name,100,L"%hs",lib_name.c_str());
    HINSTANCE  lib = LoadLibrary(win_lib_name);
    if (lib == nullptr) {
      throw std::runtime_error("Windows open dll failed");
    }
    MYPROC  symbol =
            (MYPROC)GetProcAddress(lib, "get_renderer");
    if (symbol == nullptr) {
      throw std::runtime_error("Cannot find symbol `get_renderer` in " +
                          lib_name);
    }
#endif
    std::function<IRenderer *()> get_renderer = reinterpret_cast<GetRendererBackend>(symbol);
    auto context=get_renderer();
    impl.reset(context);
}

VolumeRenderer::~VolumeRenderer() {

}

auto VolumeRenderer::get_renderer_name() const noexcept -> const char * {
    return renderer_name.c_str();
}

void VolumeRenderer::set_volume(const char *path) {
    impl->set_volume(path);
}

void VolumeRenderer::set_camera(Camera camera) noexcept {
    impl->set_camera(camera);
}

void VolumeRenderer::set_transferfunc(TransferFunction tf) noexcept {
    impl->set_transferfunc(tf);
}

void VolumeRenderer::set_mousekeyevent(MouseKeyEvent event) noexcept {
    impl->set_mousekeyevent(event);
}

void VolumeRenderer::render_frame() {
    impl->render_frame();
}

auto VolumeRenderer::get_frame() -> const Image & {
    return impl->get_frame();
}

void VolumeRenderer::clear_scene() {
    impl->clear_scene();
}

void VolumeRenderer::set_querypoint(std::array<uint32_t, 2> screen_pos) noexcept {
    impl->set_querypoint(screen_pos);
}

auto VolumeRenderer::get_querypoint() -> const std::array<float, 8> {
    return impl->get_querypoint();
}

