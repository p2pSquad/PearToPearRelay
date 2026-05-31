#include <pear/relay/relay_client.hpp>

#include <grpcpp/grpcpp.h>

#include <stdexcept>
#include <utility>

namespace pear::relay {

RelayClient::RelayClient(std::string relay_address) : relay_address_(std::move(relay_address)) {
    stub_ = pear::relay::proto::RelayService::NewStub(grpc::CreateChannel(relay_address_, grpc::InsecureChannelCredentials()));
}

std::string RelayClient::ping(const std::string& message) {
    pear::relay::proto::PingRequest request;
    pear::relay::proto::PingResponse response;

    request.set_message(message);

    grpc::ClientContext context;
    const grpc::Status status = stub_->Ping(&context, request, &response);

    if (!status.ok()) {
        throw std::runtime_error("relay ping failed: " + status.error_message());
    }

    return response.message();
}

} // namespace pear::relay
