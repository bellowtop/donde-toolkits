#include "Poco/ConsoleChannel.h"
#include "Poco/Exception.h"
#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Thread.h"
#include "Poco/Timestamp.h"
#include "concurrent_processor.h"
#include "config.h"
#include "face_pipeline.h"
#include "face_service.h"
#include "openvino/openvino.hpp"

#include <cstdint>
#include <cxxopts.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

using Poco::AutoPtr;
using Poco::ConsoleChannel;
using Poco::format;
using Poco::Logger;
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

        AutoPtr<ConsoleChannel> pChannel(new ConsoleChannel);
        Logger::root().setChannel(pChannel);

        Logger& logger = Logger::get("face-detect-service-logger");
        logger.setLevel("trace");

        FaceServiceImpl service(config, logger);
        service.Start();
        logger.information("create and started FaceDetectService...");

        std::string server_address("0.0.0.0:9595");
        grpc::EnableDefaultHealthCheckService(true);
        ServerBuilder builder;
        builder.SetSyncServerOption(ServerBuilder::SyncServerOption::MIN_POLLERS, 64);
        builder.SetSyncServerOption(ServerBuilder::SyncServerOption::MAX_POLLERS, 64);
        // grpc::ResourceQuota rq;
        // rq.SetMaxThreads(16);
        // builder.SetResourceQuota(rq);

        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service);
        logger.information("register grpc service");

        std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "Server listening on " << server_address << std::endl;
        logger.information("Server listening on %s", server_address);

        server->Wait();

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
