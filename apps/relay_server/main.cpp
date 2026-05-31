#include <pear/relay/relay_service.hpp>

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    const std::string listen_address = argc > 1 ? argv[1] : "0.0.0.0:50051";

    pear::relay::RelayService service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();

    if (!server) {
        std::cerr << "failed to start relay server on " << listen_address << "\n";
        return 1;
    }

    std::cout << "Pear relay gRPC server listening on " << listen_address << "\n";

    server->Wait();

    return 0;
}
