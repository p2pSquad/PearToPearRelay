#ifndef PEAR_RELAY_PHOTO_CONTROLLER_HPP_
#define PEAR_RELAY_PHOTO_CONTROLLER_HPP_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <pear/net/db_types.hpp>
#include <pear/relay/relay_transport.hpp>

namespace pear::relay::photo {

struct PhotoItem {
    std::string name;
    std::string full_path;
    std::string preview_path;
    std::filesystem::path local_full_path;
    std::filesystem::path local_preview_path;
    bool full_downloaded = false;
    bool preview_downloaded = false;
};

class PhotoController {
public:
    PhotoController();

    void setTraceCallback(std::function<void(const std::string&)> callback);
    void setTerminalCallback(std::function<void(const std::string&)> callback);
    void connectRoom(const std::string& relay_address, const std::string& room_name, const std::string& password);
    std::vector<PhotoItem> refresh();
    void addPhoto(const std::filesystem::path& source_path);
    void downloadPhoto(const PhotoItem& item);
    void shutdown();

    const std::filesystem::path& workspaceRoot() const;
    const std::string& roomName() const;
    const std::string& relayAddress() const;
    uint64_t deviceId() const;

private:
    void ensureWorkspace();
    void enterWorkspaceAndRun(const std::function<void()>& action);
    void runCommand(const std::string& command, const std::function<void()>& action);
    void uploadStagedObjectsForPaths(const std::vector<std::string>& paths);
    void pullMissingPreviews();
    void trace(const std::string& line);
    void terminal(const std::string& text);
    std::vector<PhotoItem> collectPhotos();
    std::filesystem::path databasePath() const;
    std::string makeClientRef() const;

    std::string relay_address_;
    std::string room_name_;
    std::string repo_id_;
    std::string token_;
    uint64_t device_id_ = 0;

    std::filesystem::path launch_root_;
    std::filesystem::path workspace_root_;
    std::shared_ptr<RelayTransport> transport_;
    std::function<void(const std::string&)> trace_callback_;
    std::function<void(const std::string&)> terminal_callback_;
};

} // namespace pear::relay::photo

#endif // PEAR_RELAY_PHOTO_CONTROLLER_HPP_
