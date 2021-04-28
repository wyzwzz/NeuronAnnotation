#include "WebSocketRequestHandler.hpp"
#include <Camera.hpp>
#include <Image.hpp>
#include <TransferFunction.hpp>
#include <VolumeRenderer.hpp>

#include <Poco/Net/NetException.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Util/Application.h>

#include <seria/deserialize.hpp>
#include <iostream>
using Poco::Util::Application;

void WebSocketRequestHandler::handleRequest(
        Poco::Net::HTTPServerRequest &request,
        Poco::Net::HTTPServerResponse &response) {
    using WebSocket = Poco::Net::WebSocket;

    Application &app = Application::instance();
    std::cout<<"loading render backend..."<<std::endl;
    VolumeRenderer block_volume_renderer("BlockVolumeRenderer");
    block_volume_renderer.set_volume("E:/mouse_23389_29581_10296_512_2_lod3/mouse_23389_29581_10296_9p2_lod3.h264");
    TransferFunction default_tf;
    default_tf.points.emplace_back(0);
    default_tf.colors.emplace_back(std::array<double ,4>{0.0,0.1,0.6,0.0});
    default_tf.points.emplace_back(30);
    default_tf.colors.emplace_back(std::array<double ,4>{0.25, 0.5, 1.0, 0.3});
    default_tf.points.emplace_back(64);
    default_tf.colors.emplace_back(std::array<double ,4>{0.75,0.75,0.75,0.6});
    default_tf.points.emplace_back(224);
    default_tf.colors.emplace_back(std::array<double ,4>{1.0,0.5,0.25,0.6});
    default_tf.points.emplace_back(225);
    default_tf.colors.emplace_back(std::array<double ,4>{0.6,0.1,0.0,0.0});
    block_volume_renderer.set_transferfunc(default_tf);
    //todo register camera... etc

    try
    {
        char buffer[4096];
        int flags = 0;
        int len;

        WebSocket ws(request, response);
        auto one_hour = Poco::Timespan(0, 1, 0, 0, 0);
        ws.setReceiveTimeout(one_hour);

        rapidjson::Document document{};

        do{
            len=ws.receiveFrame(buffer,sizeof(buffer),flags);
            try
            {
                document.Parse(buffer,len);
                if(document.HasParseError() || !document.IsObject())
                {
                    throw std::runtime_error("Parse error");
                }
                auto objects=document.GetObject();
                if(document.HasMember("camera"))
                {
                    auto values=objects["camera"].GetObject();
                    Camera camera;
                    seria::deserialize(camera,values);
                    block_volume_renderer.set_camera(camera);
                }
                else if(document.HasMember("click"))
                {

                }
                block_volume_renderer.render_frame();
                auto &image = block_volume_renderer.get_frame();
                auto encoded = Image::encode(image, Image::Format::JPEG);
                ws.sendFrame(encoded.data.data(), encoded.data.size(),WebSocket::FRAME_BINARY);
            }
            catch (std::exception& error)
            {
                ws.sendFrame(error.what(), std::strlen(error.what()),WebSocket::FRAME_TEXT);
            }
        }while(len>0 && (flags & WebSocket::FRAME_OP_BITMASK) !=WebSocket::FRAME_OP_CLOSE);

    }//try
    catch (Poco::Net::WebSocketException &exc)
    {
        app.logger().log(exc);
        switch (exc.code())
        {
            case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
                response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
                // fallthrough
            case WebSocket::WS_ERR_NO_HANDSHAKE:
            case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
            case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
                response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
                response.setContentLength(0);
                response.send();
                break;
        }//switch
    }//catch
}
