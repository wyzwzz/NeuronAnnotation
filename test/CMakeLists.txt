
add_executable(test_simple_server test_simple_server.cpp)
target_link_libraries(test_simple_server PRIVATE
        Poco::Net
        Poco::Util
        )

add_executable(test_thread_pool test_thread_pool.cpp)
target_include_directories(test_thread_pool
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/Common)
target_link_libraries(test_thread_pool
        PRIVATE
        glm)
target_compile_features(test_thread_pool
        PRIVATE
        cxx_std_17)

include(${PROJECT_SOURCE_DIR}/third_party/glfw.cmake)
add_executable(test_pick test_pick.cpp
        ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c)
target_link_libraries(test_pick
        PRIVATE
        glfw
        glm)
target_include_directories(test_pick
        PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/glad/include)