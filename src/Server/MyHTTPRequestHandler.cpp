#include "MyHTTPRequestHandler.hpp"

void MyHTTPRequestHandler::handleRequest(
    Poco::Net::HTTPServerRequest &request,
    Poco::Net::HTTPServerResponse &response) {
  response.setContentType("text/plain");
  std::ostream &ostr = response.send();
  ostr << "TODO... not implement";
}
