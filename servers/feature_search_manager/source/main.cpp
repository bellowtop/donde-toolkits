#include "Poco/ConsoleChannel.h"
#include "Poco/Exception.h"
#include "Poco/Format.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Thread.h"
#include "Poco/Timestamp.h"
#include "concurrent_processor.h"
#include "config.h"
#include "face_pipeline.h"
#include "openvino/openvino.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <cstdint>
#include <cxxopts.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>
#include <iterator>
#include <spdlog/common.h>
#include <string>
#include <unordered_map>
#include <vector>

using Poco::AutoPtr;
using Poco::ConsoleChannel;
using Poco::format;
using Poco::NotificationQueue;
using Poco::Thread;
using Poco::Timestamp;

using namespace std;

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

auto main(int argc, char** argv) -> int {
    cxxopts::Options options(*argv, "A program to welcome the world!");

    std::string language;
    std::string name;

    // clang-format off
  options.add_options()
      ("help", "Show help")
      ("version", "Print the current version number")
      ("config_path", "config filepath", cxxopts::value<std::string>()->default_value("../../contrib/server.json"));
    // clang-format on

    std::cout << "openvino version: " << ov::get_openvino_version() << std::endl;

    auto opts = options.parse(argc, argv);

    if (opts["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (opts["version"].as<bool>()) {
        std::cout << "FaceDetectService, version " << 10001 << std::endl;
        return 0;
    }

    try {
        std::string config_path = opts["config_path"].as<std::string>();
        Config::init(config_path);

        Config config = Config::getInstance();

        auto console_log = spdlog::stdout_color_mt("main");
        console_log->set_level(spdlog::level::trace);
        spdlog::set_default_logger(console_log);



    } catch (Poco::Exception& exc) {
        std::cerr << exc.displayText() << std::endl;
        return 1;

    } catch (const std::exception& exc2) {
        std::cerr << exc2.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "cannot start server";
        return 1;
    }

    cout << "end." << endl;

    return 0;
}
