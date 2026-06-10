#include "gallery_window.hpp"
#include "login_window.hpp"
#include "photo_controller.hpp"

#include <memory>
#include <vector>

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

using pear::relay::photo::GalleryWindow;
using pear::relay::photo::LoginWindow;
using pear::relay::photo::PhotoController;
using pear::relay::photo::PhotoItem;

namespace {

void showError(QWidget* parent, const std::exception& error) {
    QMessageBox::critical(parent, "Pear Family Photos", error.what());
}

} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    LoginWindow login;
    std::unique_ptr<PhotoController> controller;
    std::unique_ptr<GalleryWindow> gallery;
    std::vector<QString> pending_trace;
    std::vector<QString> pending_terminal;

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        if (controller) {
            controller->shutdown();
        }
    });

    auto append_trace = [&](const std::string& line) {
        QString text = QString::fromStdString(line);

        if (gallery) {
            gallery->appendTrace(text);
        } else {
            pending_trace.push_back(text);
        }
    };

    auto append_terminal = [&](const std::string& text) {
        QString line = QString::fromStdString(text);

        if (gallery) {
            gallery->appendTerminal(line);
        } else {
            pending_terminal.push_back(line);
        }
    };

    auto refresh_gallery = [&]() {
        try {
            const std::vector<PhotoItem> photos = controller->refresh();
            gallery->setPhotos(photos);
        } catch (const std::exception& error) {
            showError(gallery.get(), error);
        }
    };

    login.setConnectCallback([&](const QString& relay_address, const QString& room_name, const QString& password) {
        try {
            controller = std::make_unique<PhotoController>();
            controller->setTraceCallback(append_trace);
            controller->setTerminalCallback(append_terminal);
            controller->connectRoom(relay_address.toStdString(), room_name.toStdString(), password.toStdString());

            gallery = std::make_unique<GalleryWindow>();
            gallery->setRoomTitle("Pear Family Photos · " + room_name);

            for (const auto& line : pending_trace) {
                gallery->appendTrace(line);
            }

            for (const auto& line : pending_terminal) {
                gallery->appendTerminal(line);
            }

            pending_trace.clear();
            pending_terminal.clear();

            gallery->setRefreshCallback(refresh_gallery);

            gallery->setAddCallback([&]() {
                try {
                    const QString path = QFileDialog::getOpenFileName(gallery.get(), "Choose photo", QString(), "Images (*.png *.jpg *.jpeg *.bmp *.webp)");

                    if (path.isEmpty()) {
                        return;
                    }

                    controller->addPhoto(path.toStdString());
                    refresh_gallery();
                } catch (const std::exception& error) {
                    showError(gallery.get(), error);
                }
            });

            gallery->setDownloadCallback([&](const PhotoItem& item) {
                try {
                    controller->downloadPhoto(item);
                    refresh_gallery();
                } catch (const std::exception& error) {
                    showError(gallery.get(), error);
                }
            });

            refresh_gallery();
            gallery->show();
            login.hide();
        } catch (const std::exception& error) {
            showError(&login, error);
        }
    });

    login.show();

    return app.exec();
}
