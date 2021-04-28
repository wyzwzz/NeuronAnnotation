//
// Created by wyz on 2021/3/31.
//

#ifndef NEURONANNOTATION_TRANSFERFUNCTION_H
#define NEURONANNOTATION_TRANSFERFUNCTION_H
#include <array>
#include <vector>
#include <cstdint>
#include <seria/object.hpp>
class TransferFunction{
public:
    std::vector<uint8_t> points;
    std::vector<std::array<double,4>> colors;
};
namespace seria{
    template<>
    inline auto register_object<TransferFunction>(){
        return std::make_tuple(
            member("points",&TransferFunction::points),
            member("colors",&TransferFunction::colors)
        );
    }
}
#endif //NEURONANNOTATION_TRANSFERFUNCTION_H
