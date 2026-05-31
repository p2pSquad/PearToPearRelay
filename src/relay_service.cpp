#include <pear/relay/relay_service.hpp>

namespace pear::relay {

grpc::Status RelayService::Ping(grpc::ServerContext* context, const pear::relay::proto::PingRequest* request, pear::relay::proto::PingResponse* response) {
    (void)context;

    response->set_message("pear-relay-ok: " + request->message());

    return grpc::Status::OK;
}

} // namespace pear::relay
