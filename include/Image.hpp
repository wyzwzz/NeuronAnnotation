//
// Created by wyz on 2021/3/31.
//

#ifndef NEURONANNOTATION_IMAGE_H
#define NEURONANNOTATION_IMAGE_H
#include <cstdint>
#include <vector>
#include <array>
#include <cassert>
#include <stdexcept>
class Image{
public:
    Image()=default;
    enum class Format : uint8_t { RAW, JPEG };

    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t channels = 0;
    Format format = Format::RAW;
    std::vector<unsigned char> data = {};

    enum class Quality : uint8_t { HIGH = 90, MEDIUM = 70, LOW = 50 };

    auto at(uint32_t row, uint32_t col) const -> uint8_t;

    static Image encode(const uint8_t *data, uint32_t width, uint32_t height,
                        uint8_t channels, Image::Format format,
                        Image::Quality quality = Image::Quality::MEDIUM,
                        bool flip_vertically = true);

    static Image encode(const Image &image, Image::Format format,
                        Image::Quality Quality = Image::Quality::MEDIUM);
};

template<class T>
class Map{
public:
    Map()=default;
    std::array<T,4> at(int row,int col) const{
        if(row>=0 && row <height && col>=0 && col<width){
            assert(channels==4);
            size_t idx=((size_t)row*width+col)*4;
            return {data[idx],data[idx+1],data[idx+2],data[idx+3]};
        }
        else{
            throw std::runtime_error("Map's vector out of range");
        }
    }
    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t channels = 0;
    std::vector<T> data = {};

};
#endif //NEURONANNOTATION_IMAGE_H
