#ifndef PEAR_RELAY_RELAY_CLIENT_HPP_
#define PEAR_RELAY_RELAY_CLIENT_HPP_

#include <memory>
#include <string>

#include <relay.grpc.pb.h>

namespace pear::relay {

class RelayClient {
public:
    explicit RelayClient(std::string relay_address);

    std::string ping(const std::string& message);

private:
    std::string relay_address_;
    std::unique_ptr<pear::relay::proto::RelayService::Stub> stub_;
};

} // namespace pear::relay

#endif // PEAR_RELAY_RELAY_CLIENT_HPP_
