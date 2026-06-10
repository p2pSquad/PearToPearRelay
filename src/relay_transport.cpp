#include <pear/relay/relay_transport.hpp>

#include <utility>

namespace pear::relay {

RelayTransport::RelayTransport(std::string relay_address, std::string repo_id, std::string token) : relay_address_(std::move(relay_address)), repo_id_(std::move(repo_id)), token_(std::move(token)), client_(relay_address_) {}

const std::string& RelayTransport::relayAddress() const {
    return relay_address_;
}

const std::string& RelayTransport::repoId() const {
    return repo_id_;
}

const std::string& RelayTransport::token() const {
    return token_;
}

std::string RelayTransport::ping(const std::string& message) {
    return client_.ping(message);
}

void RelayTransport::uploadObject(const std::string& object_hash, const std::string& source_path) {
    client_.uploadObject(repo_id_, token_, object_hash, source_path);
}

std::vector<RelayFileRecord> RelayTransport::listFiles() {
    return client_.listFiles(repo_id_, token_);
}

uint64_t RelayTransport::registerDevice(const std::string& master_ref, const std::string& self_ref) {
    (void)master_ref;
    return client_.registerDevice(repo_id_, token_, self_ref);
}

std::vector<pear::net::WalEntryInfo> RelayTransport::updateDB(const std::string& master_ref, uint64_t last_seq_id, uint64_t device_id) {
    (void)master_ref;
    return client_.updateDB(repo_id_, token_, last_seq_id, device_id);
}

bool RelayTransport::pushWAL(const std::string& master_ref, uint64_t device_id, const std::vector<pear::net::WalEntryInfo>& entries, std::vector<uint64_t>& out_assigned_seq_ids) {
    (void)master_ref;
    return client_.pushWAL(repo_id_, token_, device_id, entries, out_assigned_seq_ids);
}

void RelayTransport::downloadFile(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id, const std::string& destination_path) {
    (void)owner_ref;
    client_.downloadObject(repo_id_, token_, object_hash, requester_device_id, destination_path);
}

uint64_t RelayTransport::getObjectSize(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id) {
    (void)owner_ref;
    return client_.getObjectSize(repo_id_, token_, object_hash, requester_device_id);
}

void RelayTransport::downloadFileRange(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id, uint64_t offset, uint64_t size, const std::string& destination_path) {
    (void)owner_ref;
    client_.downloadObjectRange(repo_id_, token_, object_hash, requester_device_id, offset, size, destination_path);
}

bool RelayTransport::deleteObject(const std::string& owner_ref, const std::string& object_hash, uint64_t requester_device_id) {
    (void)owner_ref;
    return client_.deleteObject(repo_id_, token_, object_hash, requester_device_id);
}

} // namespace pear::relay
