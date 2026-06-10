#ifndef PEAR_RELAY_RELAY_CLIENT_HPP_
#define PEAR_RELAY_RELAY_CLIENT_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <pear/net/db_types.hpp>

#include "relay.grpc.pb.h"

namespace pear::relay {

struct RelayFileRecord {
    std::string path;
    std::string object_hash;
    uint64_t version = 0;
    uint64_t owner_device_id = 0;
    bool read_only = false;
    bool downloaded = false;
};

class RelayClient {
public:
    explicit RelayClient(std::string relay_address);

    std::string ping(const std::string& message);

    uint64_t registerDevice(const std::string& repo_id, const std::string& token, const std::string& self_ref);
    std::vector<pear::net::WalEntryInfo> updateDB(const std::string& repo_id, const std::string& token, uint64_t last_seq_id, uint64_t device_id);
    bool pushWAL(const std::string& repo_id, const std::string& token, uint64_t device_id, const std::vector<pear::net::WalEntryInfo>& entries, std::vector<uint64_t>& out_assigned_seq_ids);

    void uploadObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, const std::string& source_path);
    uint64_t getObjectSize(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id);
    void downloadObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id, const std::string& destination_path);
    void downloadObjectRange(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id, uint64_t offset, uint64_t size, const std::string& destination_path);
    bool deleteObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id);
    std::vector<RelayFileRecord> listFiles(const std::string& repo_id, const std::string& token);

private:
    std::string relay_address_;
    std::unique_ptr<proto::RelayService::Stub> stub_;
};

} // namespace pear::relay

#endif // PEAR_RELAY_RELAY_CLIENT_HPP_
