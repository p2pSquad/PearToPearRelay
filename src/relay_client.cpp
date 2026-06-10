#include <pear/relay/relay_client.hpp>

#include <grpcpp/grpcpp.h>

#include <fstream>
#include <stdexcept>
#include <utility>

namespace pear::relay {
namespace {

void fillAuth(proto::Auth* auth, const std::string& repo_id, const std::string& token) {
    auth->set_repo_id(repo_id);
    auth->set_token(token);
}

void fillProtoWalEntry(proto::WalEntry* proto_entry, const pear::net::WalEntryInfo& entry) {
    proto_entry->set_seq_id(entry.seq_id);
    proto_entry->set_timestamp(entry.timestamp);
    proto_entry->set_op_type(static_cast<proto::WalOpType>(entry.op_type));

    if (entry.op_type == pear::net::WalOpTypeInfo::kFileUpdate) {
        auto* file_update = proto_entry->mutable_file_update();
        file_update->set_path(entry.file.path);
        file_update->set_object_hash(entry.file.object_hash);
        file_update->set_version(entry.file.version);
        file_update->set_owner_device_id(entry.file.owner_device_id);
        file_update->set_read_only(entry.file.read_only);
        return;
    }

    if (entry.op_type == pear::net::WalOpTypeInfo::kFileDelete) {
        auto* file_delete = proto_entry->mutable_file_delete();
        file_delete->set_path(entry.file_delete.path);
        file_delete->set_version(entry.file_delete.version);
        file_delete->set_owner_device_id(entry.file_delete.owner_device_id);
        return;
    }

    if (entry.op_type == pear::net::WalOpTypeInfo::kDeviceUpdate) {
        auto* device_update = proto_entry->mutable_device_update();
        device_update->set_device_id(entry.device.device_id);
        device_update->set_address(entry.device.address);
        return;
    }

    if (entry.op_type == pear::net::WalOpTypeInfo::kObjectOwnerUpdate) {
        auto* object_owner_update = proto_entry->mutable_object_owner_update();
        object_owner_update->set_object_hash(entry.object_owner.object_hash);
        object_owner_update->set_owner_device_id(entry.object_owner.owner_device_id);
        return;
    }

    if (entry.op_type == pear::net::WalOpTypeInfo::kObjectOwnerDelete) {
        auto* object_owner_delete = proto_entry->mutable_object_owner_delete();
        object_owner_delete->set_object_hash(entry.object_owner.object_hash);
        object_owner_delete->set_owner_device_id(entry.object_owner.owner_device_id);
    }
}

pear::net::WalEntryInfo parseProtoWalEntry(const proto::WalEntry& proto_entry) {
    pear::net::WalEntryInfo entry;
    entry.seq_id = proto_entry.seq_id();
    entry.timestamp = proto_entry.timestamp();
    entry.op_type = static_cast<pear::net::WalOpTypeInfo>(proto_entry.op_type());

    if (proto_entry.has_file_update()) {
        entry.file.path = proto_entry.file_update().path();
        entry.file.object_hash = proto_entry.file_update().object_hash();
        entry.file.version = proto_entry.file_update().version();
        entry.file.owner_device_id = proto_entry.file_update().owner_device_id();
        entry.file.read_only = proto_entry.file_update().read_only();
        return entry;
    }

    if (proto_entry.has_file_delete()) {
        entry.file_delete.path = proto_entry.file_delete().path();
        entry.file_delete.version = proto_entry.file_delete().version();
        entry.file_delete.owner_device_id = proto_entry.file_delete().owner_device_id();
        return entry;
    }

    if (proto_entry.has_device_update()) {
        entry.device.device_id = proto_entry.device_update().device_id();
        entry.device.address = proto_entry.device_update().address();
        return entry;
    }

    if (proto_entry.has_object_owner_update()) {
        entry.object_owner.object_hash = proto_entry.object_owner_update().object_hash();
        entry.object_owner.owner_device_id = proto_entry.object_owner_update().owner_device_id();
        return entry;
    }

    if (proto_entry.has_object_owner_delete()) {
        entry.object_owner.object_hash = proto_entry.object_owner_delete().object_hash();
        entry.object_owner.owner_device_id = proto_entry.object_owner_delete().owner_device_id();
    }

    return entry;
}

std::string readWholeFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);

    if (!in) {
        throw std::runtime_error("failed to open file for reading: " + path);
    }

    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeWholeFile(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);

    if (!out) {
        throw std::runtime_error("failed to open file for writing: " + path);
    }

    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

} // namespace

RelayClient::RelayClient(std::string relay_address) : relay_address_(std::move(relay_address)) {
    auto channel = grpc::CreateChannel(relay_address_, grpc::InsecureChannelCredentials());
    stub_ = proto::RelayService::NewStub(channel);
}

std::string RelayClient::ping(const std::string& message) {
    proto::PingRequest req;
    req.set_message(message);

    proto::PingResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->Ping(&ctx, req, &resp);

    if (!status.ok()) {
        throw std::runtime_error("relay ping failed");
    }

    return resp.message();
}

uint64_t RelayClient::registerDevice(const std::string& repo_id, const std::string& token, const std::string& self_ref) {
    proto::RegisterDeviceRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_self_ref(self_ref);

    proto::RegisterDeviceResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->RegisterDevice(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay register failed: " + resp.error_message());
    }

    return resp.assigned_device_id();
}

std::vector<pear::net::WalEntryInfo> RelayClient::updateDB(const std::string& repo_id, const std::string& token, uint64_t last_seq_id, uint64_t device_id) {
    proto::UpdateDBRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_last_seq_id(last_seq_id);
    req.set_device_id(device_id);

    proto::UpdateDBResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->UpdateDB(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay update failed: " + resp.error_message());
    }

    std::vector<pear::net::WalEntryInfo> entries;
    entries.reserve(static_cast<std::size_t>(resp.entries_size()));

    for (int i = 0; i < resp.entries_size(); ++i) {
        entries.push_back(parseProtoWalEntry(resp.entries(i)));
    }

    return entries;
}

bool RelayClient::pushWAL(const std::string& repo_id, const std::string& token, uint64_t device_id, const std::vector<pear::net::WalEntryInfo>& entries, std::vector<uint64_t>& out_assigned_seq_ids) {
    proto::PushWALRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_device_id(device_id);

    for (const auto& entry : entries) {
        fillProtoWalEntry(req.add_entries(), entry);
    }

    proto::PushWALResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->PushWAL(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        return false;
    }

    out_assigned_seq_ids.clear();
    out_assigned_seq_ids.reserve(static_cast<std::size_t>(resp.assigned_seq_ids_size()));

    for (int i = 0; i < resp.assigned_seq_ids_size(); ++i) {
        out_assigned_seq_ids.push_back(resp.assigned_seq_ids(i));
    }

    return true;
}

void RelayClient::uploadObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, const std::string& source_path) {
    proto::UploadObjectRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_object_hash(object_hash);
    req.set_data(readWholeFile(source_path));

    proto::UploadObjectResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->UploadObject(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay upload object failed: " + resp.error_message());
    }
}

uint64_t RelayClient::getObjectSize(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id) {
    proto::ObjectInfoRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_object_hash(object_hash);
    req.set_requester_device_id(requester_device_id);

    proto::ObjectInfoResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->GetObjectSize(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay get object size failed: " + resp.error_message());
    }

    return resp.size();
}

void RelayClient::downloadObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id, const std::string& destination_path) {
    proto::DownloadObjectRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_object_hash(object_hash);
    req.set_requester_device_id(requester_device_id);

    proto::DownloadObjectResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->DownloadObject(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay download object failed: " + resp.error_message());
    }

    writeWholeFile(destination_path, resp.data());
}

void RelayClient::downloadObjectRange(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id, uint64_t offset, uint64_t size, const std::string& destination_path) {
    proto::DownloadObjectRangeRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_object_hash(object_hash);
    req.set_requester_device_id(requester_device_id);
    req.set_offset(offset);
    req.set_size(size);

    proto::DownloadObjectRangeResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->DownloadObjectRange(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay download object range failed: " + resp.error_message());
    }

    writeWholeFile(destination_path, resp.data());
}

bool RelayClient::deleteObject(const std::string& repo_id, const std::string& token, const std::string& object_hash, uint64_t requester_device_id) {
    proto::DeleteObjectRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);
    req.set_object_hash(object_hash);
    req.set_requester_device_id(requester_device_id);

    proto::DeleteObjectResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->DeleteObject(&ctx, req, &resp);

    return status.ok() and resp.success() and !resp.busy();
}

std::vector<RelayFileRecord> RelayClient::listFiles(const std::string& repo_id, const std::string& token) {
    proto::ListFilesRequest req;
    fillAuth(req.mutable_auth(), repo_id, token);

    proto::ListFilesResponse resp;
    grpc::ClientContext ctx;
    grpc::Status status = stub_->ListFiles(&ctx, req, &resp);

    if (!status.ok() or !resp.success()) {
        throw std::runtime_error("relay list files failed: " + resp.error_message());
    }

    std::vector<RelayFileRecord> files;
    files.reserve(static_cast<std::size_t>(resp.files_size()));

    for (int i = 0; i < resp.files_size(); ++i) {
        const auto& proto_file = resp.files(i);
        RelayFileRecord file;
        file.path = proto_file.path();
        file.object_hash = proto_file.object_hash();
        file.version = proto_file.version();
        file.owner_device_id = proto_file.owner_device_id();
        file.read_only = proto_file.read_only();
        file.downloaded = proto_file.downloaded();
        files.push_back(std::move(file));
    }

    return files;
}

} // namespace pear::relay
