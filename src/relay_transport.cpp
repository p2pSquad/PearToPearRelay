#include <pear/relay/relay_transport.hpp>

#include <stdexcept>
#include <utility>

namespace pear::relay {

RelayTransport::RelayTransport(std::string relay_address, std::string repo_id, std::string token)
    : relay_address_(std::move(relay_address)), repo_id_(std::move(repo_id)), token_(std::move(token)) {}

const std::string& RelayTransport::relayAddress() const {
    return relay_address_;
}

const std::string& RelayTransport::repoId() const {
    return repo_id_;
}

const std::string& RelayTransport::token() const {
    return token_;
}

uint64_t RelayTransport::registerDevice(const std::string& master_ref, const std::string& self_ref) {
    (void)master_ref;
    (void)self_ref;
    throw std::runtime_error("RelayTransport::registerDevice is not implemented yet");
}

std::vector<pear::net::WalEntryInfo> RelayTransport::updateDB(const std::string& master_ref, uint64_t last_seq_id, uint64_t device_id) {
    (void)master_ref;
    (void)last_seq_id;
    (void)device_id;
    throw std::runtime_error("RelayTransport::updateDB is not implemented yet");
}

bool RelayTransport::pushWAL(const std::string& master_ref, uint64_t device_id, const std::vector<pear::net::WalEntryInfo>& entries, std::vector<uint64_t>& out_assigned_seq_ids) {
    (void)master_ref;
    (void)device_id;
    (void)entries;
    (void)out_assigned_seq_ids;
    throw std::runtime_error("RelayTransport::pushWAL is not implemented yet");
}

void RelayTransport::downloadFile(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id, const std::string& destination_path) {
    (void)owner_ref;
    (void)object_hash;
    (void)requester_device_id;
    (void)destination_path;
    throw std::runtime_error("RelayTransport::downloadFile is not implemented yet");
}

} // namespace pear::relay
