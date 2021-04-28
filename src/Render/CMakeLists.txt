

find_package(CUDA REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

list(
        APPEND
        BlockVolumeRenderer_SRCS
        BlockVolumeRenderer.cpp
        Image.cpp
        Data/BlockVolumeManager.cpp
        ${PROJECT_SOURCE_DIR}/third_party/glad_wgl/src/gl.c
        ${PROJECT_SOURCE_DIR}/third_party/glad_wgl/src/wgl.c
)

add_library(BlockVolumeRenderer SHARED ${BlockVolumeRenderer_SRCS})

target_include_directories(
        BlockVolumeRenderer
        PRIVATE
        ${CUDA_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/third_party/glad_wgl/include
        ${PROJECT_SOURCE_DIR}/third_party
)

target_link_libraries(
        BlockVolumeRenderer
        PRIVATE
        seria::seria
        Threads::Threads
        spdlog::spdlog
        glm::glm
        voxel_uncompress
        ${OPENGL_LIBRARIES}
        ${CUDA_CUDA_LIBRARY}
        ${CMAKE_DL_LIBS}
)
target_compile_features(BlockVolumeRenderer PRIVATE cxx_std_14)
include(${PROJECT_SOURCE_DIR}/cmake/utils.cmake)
#embed_shaders(
#        CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Shaders.hpp.in
#        TARGET ${CMAKE_CURRENT_SOURCE_DIR}/Shaders.hpp
#        DIR ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
#)