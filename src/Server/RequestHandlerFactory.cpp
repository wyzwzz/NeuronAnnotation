#include "RequestHandlerFactory.hpp"
#include "MyHTTPRequestHandler.hpp"
#include "WebSocketRequestHandler.hpp"
#include <iostream>
Poco::Net::HTTPRequestHandler *RequestHandlerFactory::createRequestHandler(
        const Poco::Net::HTTPServerRequest &request) {
    auto &uri = request.getURI();
    if (uri == "/render") {
        std::cout << "create WebSocketRequestHandler" << std::endl;
        return new WebSocketRequestHandler();
    }
    std::cout << "create MyHTTPRequestHandler" << std::endl;
    return new MyHTTPRequestHandler();
}
