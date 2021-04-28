#include "NeuronAnnotaterApplication.hpp"
#include "RequestHandlerFactory.hpp"
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/NumberParser.h>
#include <Poco/URI.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/IntValidator.h>
#include <iostream>

void NeuronAnnotaterApplication::initialize(Application &self) {}

void NeuronAnnotaterApplication::defineOptions(Poco::Util::OptionSet &options) {
  using Option = Poco::Util::Option;
  using OptionCallback = Poco::Util::OptionCallback<NeuronAnnotaterApplication>;

  options.addOption(
      Option("help", "h", "display argument help information")
          .required(false)
          .repeatable(false)
          .callback(OptionCallback(this, &NeuronAnnotaterApplication::hanldle_option)));

  options.addOption(
      Option("port", "p", "port listening")
          .required(false)
          .argument("port")
          .repeatable(false)
          .validator(new Poco::Util::IntValidator(0, 65536))
          .callback(OptionCallback(this, &NeuronAnnotaterApplication::hanldle_option)));

    ServerApplication::defineOptions(options);
}

void NeuronAnnotaterApplication::hanldle_option(const std::string &name,
                                       const std::string &value) {
  if (name == "help") {
    using HelpFormatter = Poco::Util::HelpFormatter;
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("NeuronAnnotater");
    helpFormatter.format(std::cout);
    stopOptionsProcessing();
    m_show_help = true;
    return;
  }

  if (name == "port") {
    m_port = Poco::NumberParser::parse(value);
    return;
  }
}

int NeuronAnnotaterApplication::main(const std::vector<std::string> &args) {
  if (m_show_help) {
    return Application::EXIT_OK;
  }

  using ServerSocket = Poco::Net::ServerSocket;
  using HTTPServer = Poco::Net::HTTPServer;
  using HTTPServerParams = Poco::Net::HTTPServerParams;

  ServerSocket svs(m_port);//tcp socket
  HTTPServer srv(Poco::makeShared<RequestHandlerFactory>(), svs,
                 Poco::makeAuto<HTTPServerParams>());

  srv.start();

  this->logger().information("server starts at port: " +
                             std::to_string(m_port));

  waitForTerminationRequest();

  srv.stop();

  return Application::EXIT_OK;

}