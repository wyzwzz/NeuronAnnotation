
add_executable(test_simple_server test_simple_server.cpp)
target_link_libraries(test_simple_server PRIVATE
        Poco::Net
        Poco::Util
        )