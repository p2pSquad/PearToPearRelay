#include "photo_controller.hpp"

#include <pear/cli/commands.hpp>
#include <pear/db/sqlite_database.hpp>
#include <pear/demon/demon.hpp>
#include <pear/fs/workspace.hpp>
#include <pear/net/transport_registry.hpp>

#include "command_utils.hpp"

#include <QImage>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace pear::relay::photo {
namespace {

uint64_t fnv1a(const std::string& text) {
    uint64_t hash = 14695981039346656037ull;

    for (unsigned char ch : text) {
        hash ^= ch;
        hash *= 1099511628211ull;
    }

    return hash;
}

std::string toHex(uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

std::string stableId(const std::string& text) {
    return toHex(fnv1a(text));
}

std::string makeRepoId(const std::string& room_name) {
    return "room_" + stableId(room_name);
}

std::string makeToken(const std::string& room_name, const std::string& password) {
    return "token_" + stableId(room_name + ":" + password);
}

bool isSupportedImage(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".webp";
}

std::string normalizeFileName(std::string name) {
    for (char& ch : name) {
        if (ch == '/' || ch == '\\' || ch == '\t' || ch == '\n' || ch == '\r') {
            ch = '_';
        }
    }

    if (name.empty()) {
        name = "photo.jpg";
    }

    return name;
}

std::string previewPathForName(const std::string& name) {
    return "previews/" + name + ".preview.jpg";
}

std::string fullPathForName(const std::string& name) {
    return "photos/" + name;
}

void copyFile(const std::filesystem::path& from, const std::filesystem::path& to) {
    std::filesystem::create_directories(to.parent_path());
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
}

void makePreview(const std::filesystem::path& source, const std::filesystem::path& destination) {
    QImage image(QString::fromStdString(source.string()));

    if (image.isNull()) {
        throw std::runtime_error("failed to read selected image");
    }

    std::filesystem::create_directories(destination.parent_path());

    QImage preview = image.scaled(180, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (!preview.save(QString::fromStdString(destination.string()), "JPG", 25)) {
        throw std::runtime_error("failed to save preview image");
    }
}

bool isRealFile(const std::filesystem::path& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

class CurrentDirectoryGuard {
public:
    explicit CurrentDirectoryGuard(const std::filesystem::path& path) : old_path_(std::filesystem::current_path()) {
        std::filesystem::current_path(path);
    }

    ~CurrentDirectoryGuard() {
        std::filesystem::current_path(old_path_);
    }

private:
    std::filesystem::path old_path_;
};

std::string trimTerminalLine(std::string line) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || std::isspace(static_cast<unsigned char>(line.back())))) {
        line.pop_back();
    }

    std::size_t first = 0;

    while (first < line.size() && std::isspace(static_cast<unsigned char>(line[first]))) {
        ++first;
    }

    if (first > 0) {
        line.erase(0, first);
    }

    return line;
}

void eraseAll(std::string& text, const std::string& needle) {
    std::size_t pos = 0;

    while ((pos = text.find(needle, pos)) != std::string::npos) {
        text.erase(pos, needle.size());
    }
}

std::string cleanTerminalLine(std::string line) {
    eraseAll(line, "\xF0\x9F\x8D\x90");
    eraseAll(line, "🍐");
    line = trimTerminalLine(line);

    while (!line.empty() && static_cast<unsigned char>(line.front()) < 32) {
        line.erase(line.begin());
    }

    return trimTerminalLine(line);
}

std::string formatTerminalBlock(const std::string& command, const std::string& output) {
    std::ostringstream result;
    result << "$ " << command << '\n';

    bool printed_any_line = false;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        line = cleanTerminalLine(line);

        if (line.empty()) {
            continue;
        }

        result << "pear> " << line << '\n';
        printed_any_line = true;
    }

    if (!printed_any_line) {
        result << "pear> (no output)\n";
    }

    return result.str();
}

} // namespace

PhotoController::PhotoController() : launch_root_(std::filesystem::current_path()) {}

void PhotoController::setTraceCallback(std::function<void(const std::string&)> callback) {
    trace_callback_ = std::move(callback);
}

void PhotoController::setTerminalCallback(std::function<void(const std::string&)> callback) {
    terminal_callback_ = std::move(callback);
}

void PhotoController::connectRoom(const std::string& relay_address, const std::string& room_name, const std::string& password) {
    if (relay_address.empty() || room_name.empty() || password.empty()) {
        throw std::runtime_error("relay address, room and password are required");
    }

    relay_address_ = relay_address;
    room_name_ = room_name;
    repo_id_ = makeRepoId(room_name);
    token_ = makeToken(room_name, password);
    workspace_root_ = launch_root_ / "photo_gallery_data" / repo_id_ / "workspace";

    ensureWorkspace();

    trace("setTransport(RelayTransport)");
    terminal(formatTerminalBlock("setTransport(RelayTransport)", "network transport switched to relay\n"));
    transport_ = std::make_shared<RelayTransport>(relay_address_, repo_id_, token_);
    pear::net::setTransport(transport_);

    pear::storage::Workspace workspace = pear::storage::Workspace::discover(workspace_root_);
    pear::db::SqliteDatabase database(databasePath());

    trace("registerDevice(room client)");
    terminal(formatTerminalBlock("transport.registerDevice relay-room", "registered room device\n"));
    device_id_ = transport_->registerDevice("relay-room", makeClientRef());
    database.setMasterAddress("relay-room");
    database.setDeviceId(device_id_);

    runCommand("run_update", [&]() {
        enterWorkspaceAndRun([]() {
            pear::cli::run_update();
        });
    });

    pullMissingPreviews();
}

std::vector<PhotoItem> PhotoController::refresh() {
    if (!transport_) {
        throw std::runtime_error("not connected to room");
    }

    pear::net::setTransport(transport_);

    runCommand("run_update", [&]() {
        enterWorkspaceAndRun([]() {
            pear::cli::run_update();
        });
    });

    pullMissingPreviews();

    return collectPhotos();
}

void PhotoController::addPhoto(const std::filesystem::path& source_path) {
    if (!transport_) {
        throw std::runtime_error("not connected to room");
    }

    if (!std::filesystem::exists(source_path)) {
        throw std::runtime_error("selected photo does not exist");
    }

    if (!isSupportedImage(source_path)) {
        throw std::runtime_error("selected file is not a supported image");
    }

    pear::net::setTransport(transport_);

    const std::string name = normalizeFileName(source_path.filename().string());
    const std::string full_path = fullPathForName(name);
    const std::string preview_path = previewPathForName(name);

    const std::filesystem::path local_full = workspace_root_ / full_path;
    const std::filesystem::path local_preview = workspace_root_ / preview_path;

    trace("copy full photo -> " + full_path);
    copyFile(source_path, local_full);

    trace("create low-res preview -> " + preview_path);
    makePreview(source_path, local_preview);

    runCommand("run_add --readonly " + full_path, [&]() {
        enterWorkspaceAndRun([&]() {
            pear::cli::run_add({std::filesystem::path(full_path)}, false, true);
        });
    });

    runCommand("run_add --readonly " + preview_path, [&]() {
        enterWorkspaceAndRun([&]() {
            pear::cli::run_add({std::filesystem::path(preview_path)}, false, true);
        });
    });

    uploadStagedObjectsForPaths({full_path, preview_path});

    runCommand("run_push", [&]() {
        enterWorkspaceAndRun([]() {
            pear::cli::run_push();
        });
    });
}

void PhotoController::downloadPhoto(const PhotoItem& item) {
    if (!transport_) {
        throw std::runtime_error("not connected to room");
    }

    pear::net::setTransport(transport_);

    runCommand("run_pull " + item.full_path, [&]() {
        enterWorkspaceAndRun([&]() {
            pear::cli::run_pull({item.full_path}, true);
        });
    });
}

void PhotoController::shutdown() {
    if (workspace_root_.empty()) {
        return;
    }

    try {
        if (pear::demon::is_alive(workspace_root_)) {
            trace("demon::kill(workspace)");
            terminal(formatTerminalBlock("demon::kill workspace", "stopped local demon\n"));
            pear::demon::kill(workspace_root_);
        }
    } catch (...) {
    }
}

const std::filesystem::path& PhotoController::workspaceRoot() const {
    return workspace_root_;
}

const std::string& PhotoController::roomName() const {
    return room_name_;
}

const std::string& PhotoController::relayAddress() const {
    return relay_address_;
}

uint64_t PhotoController::deviceId() const {
    return device_id_;
}

void PhotoController::ensureWorkspace() {
    std::filesystem::create_directories(workspace_root_);

    if (!std::filesystem::exists(workspace_root_ / ".peer")) {
        runCommand("run_init " + workspace_root_.string(), [&]() {
            pear::cli::run_init(workspace_root_);
        });
    }

    std::filesystem::create_directories(workspace_root_ / "photos");
    std::filesystem::create_directories(workspace_root_ / "previews");
}

void PhotoController::enterWorkspaceAndRun(const std::function<void()>& action) {
    CurrentDirectoryGuard guard(workspace_root_);
    action();
}

void PhotoController::runCommand(const std::string& command, const std::function<void()>& action) {
    trace(command);

    std::ostringstream captured;
    auto* old_cout = std::cout.rdbuf(captured.rdbuf());
    auto* old_cerr = std::cerr.rdbuf(captured.rdbuf());

    try {
        action();
    } catch (...) {
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
        terminal(formatTerminalBlock(command, captured.str()));
        throw;
    }

    std::cout.rdbuf(old_cout);
    std::cerr.rdbuf(old_cerr);
    terminal(formatTerminalBlock(command, captured.str()));
}

void PhotoController::uploadStagedObjectsForPaths(const std::vector<std::string>& paths) {
    pear::storage::Workspace workspace = pear::storage::Workspace::discover(workspace_root_);
    pear::db::SqliteDatabase database(databasePath());
    const auto staged_files = database.getStagedFiles();
    const std::set<std::string> wanted(paths.begin(), paths.end());

    for (const auto& staged : staged_files) {
        if (!wanted.contains(staged.path)) {
            continue;
        }

        const std::filesystem::path object_path = workspace.get_obj_dir() / staged.object_hash;
        trace("upload object " + staged.object_hash);
        terminal(formatTerminalBlock("transport.uploadObject " + staged.object_hash, "object uploaded to relay\n"));

        if (std::filesystem::exists(object_path)) {
            transport_->uploadObject(staged.object_hash, object_path.string());
        } else {
            transport_->uploadObject(staged.object_hash, staged.local_path);
        }
    }
}

void PhotoController::pullMissingPreviews() {
    pear::storage::Workspace workspace = pear::storage::Workspace::discover(workspace_root_);
    pear::db::SqliteDatabase database(databasePath());
    const auto files = database.getAllFiles();

    std::vector<std::string> targets;

    for (const auto& file : files) {
        if (file.path.rfind("previews/", 0) != 0) {
            continue;
        }

        const std::filesystem::path preview_path = workspace_root_ / file.path;

        if (!isRealFile(preview_path)) {
            targets.push_back(file.path);
        }
    }

    for (const auto& target : targets) {
        try {
            runCommand("run_pull preview " + target, [&]() {
                enterWorkspaceAndRun([&]() {
                    pear::cli::run_pull({target}, true);
                });
            });
        } catch (...) {
            trace("failed to pull preview " + target);
        }
    }
}

void PhotoController::trace(const std::string& line) {
    if (trace_callback_) {
        trace_callback_(line);
    }
}

void PhotoController::terminal(const std::string& text) {
    if (terminal_callback_) {
        terminal_callback_(text);
    }
}

std::vector<PhotoItem> PhotoController::collectPhotos() {
    pear::db::SqliteDatabase database(databasePath());
    const auto files = database.getAllFiles();

    std::vector<PhotoItem> photos;

    for (const auto& file : files) {
        if (file.path.rfind("photos/", 0) != 0) {
            continue;
        }

        PhotoItem item;
        item.full_path = file.path;
        item.name = file.path.substr(std::string("photos/").size());
        item.preview_path = previewPathForName(item.name);
        item.local_full_path = workspace_root_ / item.full_path;
        item.local_preview_path = workspace_root_ / item.preview_path;
        item.full_downloaded = isRealFile(item.local_full_path);
        item.preview_downloaded = isRealFile(item.local_preview_path);

        photos.push_back(std::move(item));
    }

    std::sort(photos.begin(), photos.end(), [](const PhotoItem& left, const PhotoItem& right) {
        return left.name < right.name;
    });

    return photos;
}

std::filesystem::path PhotoController::databasePath() const {
    pear::storage::Workspace workspace = pear::storage::Workspace::discover(workspace_root_);
    return pear::cli::get_database_path(workspace);
}

std::string PhotoController::makeClientRef() const {
    return "photo-gallery-" + stableId(launch_root_.string() + ":" + room_name_);
}

} // namespace pear::relay::photo
