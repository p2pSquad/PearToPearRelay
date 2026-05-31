#ifndef PEAR_RELAY_RELAY_SERVICE_HPP_
#define PEAR_RELAY_RELAY_SERVICE_HPP_

#include <grpcpp/grpcpp.h>
#include <relay.grpc.pb.h>

namespace pear::relay {

class RelayService final : public pear::relay::proto::RelayService::Service {
public:
    grpc::Status Ping(grpc::ServerContext* context, const pear::relay::proto::PingRequest* request, pear::relay::proto::PingResponse* response) override;
};

} // namespace pear::relay

#endif // PEAR_RELAY_RELAY_SERVICE_HPP_
