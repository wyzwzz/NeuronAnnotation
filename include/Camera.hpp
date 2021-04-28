//
// Created by wyz on 2021/3/31.
//

#ifndef NEURONANNOTATION_CAMERA_H
#define NEURONANNOTATION_CAMERA_H
#include <array>
#include <cstdint>
#include <seria/object.hpp>
class Camera{
public:
    std::array<float,3> pos;
    std::array<float,3> front;//normalized direction
    std::array<float,3> up;//normalized direction
    float zoom;
    float n,f;
};

namespace seria{
    template<>
    inline auto register_object<Camera>(){
        return std::make_tuple(
                member("pos",&Camera::pos),
                member("front",&Camera::front),
                member("up",&Camera::up),
                member("zoom",&Camera::zoom),
                member("n",&Camera::n),
                member("f",&Camera::f)
                );
    }
}

#endif //NEURONANNOTATION_CAMERA_H
