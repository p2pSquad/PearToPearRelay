#include <pear/relay/relay_service.hpp>

#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pear::relay {
namespace {

constexpr uint64_t kFirstDeviceId = 1;

struct DeviceRecord {
    uint64_t device_id = 0;
    std::string address;
};

bool isSafeName(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        const bool ok = (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z') or (ch >= '0' and ch <= '9') or ch == '_' or ch == '-' or ch == '.';

        if (!ok) {
            return false;
        }
    }

    return true;
}

std::filesystem::path roomPath(const std::filesystem::path& root, const std::string& repo_id) {
    if (!isSafeName(repo_id)) {
        throw std::runtime_error("bad repo id");
    }

    return root / "rooms" / repo_id;
}

std::filesystem::path tokenPath(const std::filesystem::path& room_path) {
    return room_path / "token.txt";
}

std::filesystem::path devicesPath(const std::filesystem::path& room_path) {
    return room_path / "devices.tsv";
}

std::filesystem::path walPath(const std::filesystem::path& room_path) {
    return room_path / "wal.bin";
}

std::filesystem::path objectsPath(const std::filesystem::path& room_path) {
    return room_path / "objects";
}

std::filesystem::path objectPath(const std::filesystem::path& room_path, const std::string& object_hash) {
    if (!isSafeName(object_hash)) {
        throw std::runtime_error("bad object hash");
    }

    return objectsPath(room_path) / object_hash;
}

void ensureRoomAuthorized(const std::filesystem::path& root, const proto::Auth& auth) {
    if (!isSafeName(auth.repo_id()) or auth.token().empty()) {
        throw std::runtime_error("bad room credentials");
    }

    const auto path = roomPath(root, auth.repo_id());
    std::filesystem::create_directories(objectsPath(path));

    const auto token_file = tokenPath(path);

    if (!std::filesystem::exists(token_file)) {
        std::ofstream out(token_file, std::ios::binary);
        out << auth.token();
        return;
    }

    std::ifstream in(token_file, std::ios::binary);
    std::string stored_token;
    std::getline(in, stored_token);

    if (stored_token != auth.token()) {
        throw std::runtime_error("wrong room password");
    }
}

std::vector<DeviceRecord> readDevices(const std::filesystem::path& room_path) {
    std::vector<DeviceRecord> devices;
    std::ifstream in(devicesPath(room_path));

    uint64_t device_id = 0;
    std::string address;

    while (in >> device_id >> address) {
        devices.push_back(DeviceRecord{device_id, address});
    }

    return devices;
}

void writeDevices(const std::filesystem::path& room_path, const std::vector<DeviceRecord>& devices) {
    std::ofstream out(devicesPath(room_path), std::ios::trunc);

    if (!out) {
        throw std::runtime_error("failed to write devices");
    }

    for (const auto& device : devices) {
        out << device.device_id << '\t' << device.address << '\n';
    }
}

void writeWalEntry(std::ofstream& out, const proto::WalEntry& entry) {
    std::string data;

    if (!entry.SerializeToString(&data)) {
        throw std::runtime_error("failed to serialize wal entry");
    }

    const uint64_t size = data.size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::vector<proto::WalEntry> readWal(const std::filesystem::path& room_path) {
    std::vector<proto::WalEntry> entries;
    std::ifstream in(walPath(room_path), std::ios::binary);

    while (in) {
        uint64_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));

        if (!in) {
            break;
        }

        std::string data(size, '\0');
        in.read(data.data(), static_cast<std::streamsize>(data.size()));

        if (!in) {
            throw std::runtime_error("broken wal file");
        }

        proto::WalEntry entry;

        if (!entry.ParseFromString(data)) {
            throw std::runtime_error("failed to parse wal entry");
        }

        entries.push_back(std::move(entry));
    }

    return entries;
}

void appendWal(const std::filesystem::path& room_path, const proto::WalEntry& entry) {
    std::ofstream out(walPath(room_path), std::ios::binary | std::ios::app);

    if (!out) {
        throw std::runtime_error("failed to open wal");
    }

    writeWalEntry(out, entry);
}

uint64_t lastSeqId(const std::vector<proto::WalEntry>& entries) {
    uint64_t last_seq_id = 0;

    for (const auto& entry : entries) {
        last_seq_id = std::max(last_seq_id, entry.seq_id());
    }

    return last_seq_id;
}

void fillError(bool* success, std::string* error_message, const std::exception& error) {
    *success = false;
    *error_message = error.what();
}

} // namespace

RelayService::RelayService(std::filesystem::path storage_root) : storage_root_(std::move(storage_root)) {
    std::filesystem::create_directories(storage_root_);
}

grpc::Status RelayService::Ping(grpc::ServerContext* /*ctx*/, const proto::PingRequest* req, proto::PingResponse* resp) {
    resp->set_message("relay pong: " + req->message());
    return grpc::Status::OK;
}

grpc::Status RelayService::RegisterDevice(grpc::ServerContext* /*ctx*/, const proto::RegisterDeviceRequest* req, proto::RegisterDeviceResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = roomPath(storage_root_, req->auth().repo_id());

        auto devices = readDevices(path);

        for (const auto& device : devices) {
            if (device.address == req->self_ref()) {
                resp->set_success(true);
                resp->set_assigned_device_id(device.device_id);

                for (const auto& entry : readWal(path)) {
                    *resp->add_full_wal() = entry;
                }

                return grpc::Status::OK;
            }
        }

        uint64_t assigned_device_id = kFirstDeviceId;

        for (const auto& device : devices) {
            assigned_device_id = std::max(assigned_device_id, device.device_id + 1);
        }

        devices.push_back(DeviceRecord{assigned_device_id, req->self_ref()});
        writeDevices(path, devices);

        auto wal_entries = readWal(path);
        proto::WalEntry device_entry;
        device_entry.set_seq_id(lastSeqId(wal_entries) + 1);
        device_entry.set_timestamp(0);
        device_entry.set_op_type(proto::WalOpType::DEVICE_UPDATE);
        auto* device_update = device_entry.mutable_device_update();
        device_update->set_device_id(assigned_device_id);
        device_update->set_address(req->self_ref());
        appendWal(path, device_entry);

        resp->set_success(true);
        resp->set_assigned_device_id(assigned_device_id);

        for (const auto& entry : readWal(path)) {
            *resp->add_full_wal() = entry;
        }
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::UpdateDB(grpc::ServerContext* /*ctx*/, const proto::UpdateDBRequest* req, proto::UpdateDBResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = roomPath(storage_root_, req->auth().repo_id());

        for (const auto& entry : readWal(path)) {
            if (entry.seq_id() > req->last_seq_id()) {
                *resp->add_entries() = entry;
            }
        }

        resp->set_success(true);
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::PushWAL(grpc::ServerContext* /*ctx*/, const proto::PushWALRequest* req, proto::PushWALResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = roomPath(storage_root_, req->auth().repo_id());
        auto entries = readWal(path);
        uint64_t next_seq_id = lastSeqId(entries) + 1;

        for (const auto& proto_entry : req->entries()) {
            proto::WalEntry stored_entry = proto_entry;
            stored_entry.set_seq_id(next_seq_id);

            appendWal(path, stored_entry);
            resp->add_assigned_seq_ids(next_seq_id);
            ++next_seq_id;
        }

        resp->set_success(true);
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::UploadObject(grpc::ServerContext* /*ctx*/, const proto::UploadObjectRequest* req, proto::UploadObjectResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = roomPath(storage_root_, req->auth().repo_id());
        std::ofstream out(objectPath(path, req->object_hash()), std::ios::binary | std::ios::trunc);

        if (!out) {
            throw std::runtime_error("failed to open object for writing");
        }

        out.write(req->data().data(), static_cast<std::streamsize>(req->data().size()));
        resp->set_success(true);
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::GetObjectSize(grpc::ServerContext* /*ctx*/, const proto::ObjectInfoRequest* req, proto::ObjectInfoResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = objectPath(roomPath(storage_root_, req->auth().repo_id()), req->object_hash());

        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("object not found");
        }

        resp->set_success(true);
        resp->set_size(std::filesystem::file_size(path));
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::DownloadObject(grpc::ServerContext* /*ctx*/, const proto::DownloadObjectRequest* req, proto::DownloadObjectResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = objectPath(roomPath(storage_root_, req->auth().repo_id()), req->object_hash());
        std::ifstream in(path, std::ios::binary);

        if (!in) {
            throw std::runtime_error("object not found");
        }

        std::string data{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
        resp->set_success(true);
        resp->set_data(std::move(data));
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::DownloadObjectRange(grpc::ServerContext* /*ctx*/, const proto::DownloadObjectRangeRequest* req, proto::DownloadObjectRangeResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = objectPath(roomPath(storage_root_, req->auth().repo_id()), req->object_hash());
        std::ifstream in(path, std::ios::binary);

        if (!in) {
            throw std::runtime_error("object not found");
        }

        const uint64_t file_size = std::filesystem::file_size(path);

        if (req->offset() > file_size) {
            throw std::runtime_error("range offset is outside object");
        }

        const uint64_t readable_size = std::min<uint64_t>(req->size(), file_size - req->offset());
        std::string data(readable_size, '\0');

        in.seekg(static_cast<std::streamoff>(req->offset()));
        in.read(data.data(), static_cast<std::streamsize>(data.size()));

        resp->set_success(true);
        resp->set_data(std::move(data));
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::DeleteObject(grpc::ServerContext* /*ctx*/, const proto::DeleteObjectRequest* req, proto::DeleteObjectResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = objectPath(roomPath(storage_root_, req->auth().repo_id()), req->object_hash());

        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }

        resp->set_success(true);
        resp->set_busy(false);
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

grpc::Status RelayService::ListFiles(grpc::ServerContext* /*ctx*/, const proto::ListFilesRequest* req, proto::ListFilesResponse* resp) {
    std::unique_lock lock(mutex_);

    try {
        ensureRoomAuthorized(storage_root_, req->auth());
        const auto path = roomPath(storage_root_, req->auth().repo_id());
        std::map<std::string, proto::FileRecord> files;

        for (const auto& entry : readWal(path)) {
            if (entry.has_file_update()) {
                auto& file = files[entry.file_update().path()];
                file.set_path(entry.file_update().path());
                file.set_object_hash(entry.file_update().object_hash());
                file.set_version(entry.file_update().version());
                file.set_owner_device_id(entry.file_update().owner_device_id());
                file.set_read_only(entry.file_update().read_only());
                file.set_downloaded(std::filesystem::exists(objectPath(path, entry.file_update().object_hash())));
            } else if (entry.has_file_delete()) {
                files.erase(entry.file_delete().path());
            }
        }

        for (const auto& [_, file] : files) {
            *resp->add_files() = file;
        }

        resp->set_success(true);
    } catch (const std::exception& error) {
        resp->set_success(false);
        resp->set_error_message(error.what());
    }

    return grpc::Status::OK;
}

} // namespace pear::relay
