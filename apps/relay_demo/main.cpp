#include <pear/relay/relay_transport.hpp>

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::string relay_address = argc > 1 ? argv[1] : "127.0.0.1:50051";
    const std::string repo_id = argc > 2 ? argv[2] : "demo-repo";
    const std::string token = argc > 3 ? argv[3] : "demo-token";

    try {
        pear::relay::RelayTransport transport(relay_address, repo_id, token);

        std::cout << "RelayTransport created\n";
        std::cout << "relay: " << transport.relayAddress() << "\n";
        std::cout << "repo: " << transport.repoId() << "\n";
        std::cout << transport.ping("hello") << "\n";
    } catch (const std::exception& exception) {
        std::cerr << "relay demo failed: " << exception.what() << "\n";
        return 1;
    }

    return 0;
}
