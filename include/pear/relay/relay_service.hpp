#ifndef PEAR_RELAY_RELAY_SERVICE_HPP_
#define PEAR_RELAY_RELAY_SERVICE_HPP_

#include <filesystem>
#include <mutex>
#include <string>

#include "relay.grpc.pb.h"

namespace pear::relay {

class RelayService final : public proto::RelayService::Service {
public:
    explicit RelayService(std::filesystem::path storage_root = ".pear-relay");

    grpc::Status Ping(grpc::ServerContext* ctx, const proto::PingRequest* req, proto::PingResponse* resp) override;
    grpc::Status RegisterDevice(grpc::ServerContext* ctx, const proto::RegisterDeviceRequest* req, proto::RegisterDeviceResponse* resp) override;
    grpc::Status UpdateDB(grpc::ServerContext* ctx, const proto::UpdateDBRequest* req, proto::UpdateDBResponse* resp) override;
    grpc::Status PushWAL(grpc::ServerContext* ctx, const proto::PushWALRequest* req, proto::PushWALResponse* resp) override;
    grpc::Status UploadObject(grpc::ServerContext* ctx, const proto::UploadObjectRequest* req, proto::UploadObjectResponse* resp) override;
    grpc::Status GetObjectSize(grpc::ServerContext* ctx, const proto::ObjectInfoRequest* req, proto::ObjectInfoResponse* resp) override;
    grpc::Status DownloadObject(grpc::ServerContext* ctx, const proto::DownloadObjectRequest* req, proto::DownloadObjectResponse* resp) override;
    grpc::Status DownloadObjectRange(grpc::ServerContext* ctx, const proto::DownloadObjectRangeRequest* req, proto::DownloadObjectRangeResponse* resp) override;
    grpc::Status DeleteObject(grpc::ServerContext* ctx, const proto::DeleteObjectRequest* req, proto::DeleteObjectResponse* resp) override;
    grpc::Status ListFiles(grpc::ServerContext* ctx, const proto::ListFilesRequest* req, proto::ListFilesResponse* resp) override;

private:
    std::filesystem::path storage_root_;
    std::mutex mutex_;
};

} // namespace pear::relay

#endif // PEAR_RELAY_RELAY_SERVICE_HPP_
