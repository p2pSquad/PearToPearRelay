#ifndef PEAR_RELAY_RELAY_TRANSPORT_HPP_
#define PEAR_RELAY_RELAY_TRANSPORT_HPP_

#include <cstdint>
#include <string>
#include <vector>

#include <pear/net/pear_transport.hpp>

namespace pear::relay {

class RelayTransport final : public pear::net::PearTransport {
public:
    RelayTransport(std::string relay_address, std::string repo_id, std::string token);

    const std::string& relayAddress() const;
    const std::string& repoId() const;
    const std::string& token() const;

    uint64_t registerDevice(const std::string& master_ref, const std::string& self_ref) override;
    std::vector<pear::net::WalEntryInfo> updateDB(const std::string& master_ref, uint64_t last_seq_id, uint64_t device_id) override;
    bool pushWAL(const std::string& master_ref, uint64_t device_id, const std::vector<pear::net::WalEntryInfo>& entries, std::vector<uint64_t>& out_assigned_seq_ids) override;
    void downloadFile(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id, const std::string& destination_path) override;

private:
    std::string relay_address_;
    std::string repo_id_;
    std::string token_;
};

} // namespace pear::relay

#endif // PEAR_RELAY_RELAY_TRANSPORT_HPP_
