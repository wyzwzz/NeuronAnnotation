//
// Created by wyz on 2021/4/26.
//

#ifndef NEURONANNOTATION_MOUSEKEYEVENT_HPP
#define NEURONANNOTATION_MOUSEKEYEVENT_HPP
#include<array>
class MouseKeyEvent{

};
class QueryPoint{
public:
    uint32_t x,y;
};
namespace seria{
    template<>
    inline auto register_object<QueryPoint>(){
        return std::make_tuple(
                member("x",&QueryPoint::x),
                member("y",&QueryPoint::y)
        );
    }
}
#endif //NEURONANNOTATION_MOUSEKEYEVENT_HPP
