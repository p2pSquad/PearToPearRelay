#include "gallery_window.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

#include <filesystem>

#ifndef PEAR_RELAY_ASSET_DIR
#define PEAR_RELAY_ASSET_DIR "."
#endif

namespace pear::relay::photo {
namespace {

constexpr int kCardWidth = 208;
constexpr int kPreviewWidth = 180;
constexpr int kPreviewHeight = 132;

} // namespace

GalleryWindow::GalleryWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Pear Family Photos");
    resize(1080, 810);
    setStyleSheet(R"css(
        QWidget {
            background: #141414;
            color: #E1E3E6;
            font-family: Arial;
            font-size: 14px;
        }

        QLabel#Title {
            color: #FFFFFF;
            font-size: 22px;
            font-weight: 800;
            background: transparent;
        }

        QLabel#TerminalTitle {
            color: #AEB7C2;
            font-size: 12px;
            font-weight: 700;
            background: transparent;
        }

        QPushButton {
            background: #0077FF;
            color: #FFFFFF;
            border: none;
            border-radius: 10px;
            padding: 10px 14px;
            font-weight: 700;
        }

        QPushButton:hover {
            background: #2688EB;
        }

        QPushButton#LightButton {
            background: #2A2D30;
            color: #71AAEB;
        }

        QPushButton#LightButton:hover {
            background: #33373A;
        }

        QFrame#TopBar {
            background: #19191A;
            border-bottom: 2px solid #0077FF;
        }

        QFrame#CardDownloaded {
            background: #222222;
            border: 2px solid #4BB34B;
            border-radius: 16px;
        }

        QFrame#CardRemote {
            background: #222222;
            border: 2px solid #E64646;
            border-radius: 16px;
        }

        QFrame#AddCard {
            background: #1C1D1F;
            border: 2px dashed #0077FF;
            border-radius: 16px;
        }

        QPlainTextEdit {
            background: #0B0B0C;
            color: #D7E8FF;
            border: 1px solid #333333;
            border-radius: 12px;
            padding: 8px;
            font-family: "DejaVu Sans Mono", "Consolas", Monospace;
            font-size: 12px;
        }
    )css");

    auto* logo = new QLabel;
    logo->setFixedSize(42, 42);
    logo->setAlignment(Qt::AlignCenter);
    QPixmap logo_pixmap(QString(PEAR_RELAY_ASSET_DIR) + "/logo.png");

    if (!logo_pixmap.isNull()) {
        logo->setPixmap(logo_pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    title_label_ = new QLabel("Pear Family Photos");
    title_label_->setObjectName("Title");

    auto* refresh_button = new QPushButton("Refresh");
    refresh_button->setObjectName("LightButton");

    QObject::connect(refresh_button, &QPushButton::clicked, [this]() {
        if (refresh_callback_) {
            refresh_callback_();
        }
    });

    auto* top_bar = new QFrame;
    top_bar->setObjectName("TopBar");

    auto* header = new QHBoxLayout;
    header->setContentsMargins(22, 12, 22, 12);
    header->addWidget(logo);
    header->addWidget(title_label_);
    header->addStretch();
    header->addWidget(refresh_button);
    top_bar->setLayout(header);

    grid_widget_ = new QWidget;
    grid_layout_ = new QGridLayout;
    grid_layout_->setSpacing(16);
    grid_layout_->setContentsMargins(20, 20, 20, 20);
    grid_widget_->setLayout(grid_layout_);

    scroll_area_ = new QScrollArea;
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    scroll_area_->setWidget(grid_widget_);

    trace_view_ = new QPlainTextEdit;
    trace_view_->setReadOnly(true);
    trace_view_->setMinimumHeight(120);
    trace_view_->setMaximumHeight(155);
    trace_view_->appendPlainText("$ short trace");

    terminal_view_ = new QPlainTextEdit;
    terminal_view_->setReadOnly(true);
    terminal_view_->setMinimumHeight(120);
    terminal_view_->setMaximumHeight(155);
    terminal_view_->appendPlainText("$ PearToPear terminal output");

    auto* trace_title = new QLabel("short command trace");
    trace_title->setObjectName("TerminalTitle");

    auto* terminal_title = new QLabel("real CLI output");
    terminal_title->setObjectName("TerminalTitle");

    auto* trace_box = new QVBoxLayout;
    trace_box->setContentsMargins(12, 8, 6, 12);
    trace_box->addWidget(trace_title);
    trace_box->addWidget(trace_view_);

    auto* terminal_box = new QVBoxLayout;
    terminal_box->setContentsMargins(6, 8, 12, 12);
    terminal_box->addWidget(terminal_title);
    terminal_box->addWidget(terminal_view_);

    auto* logs = new QHBoxLayout;
    logs->setSpacing(0);
    logs->addLayout(trace_box, 1);
    logs->addLayout(terminal_box, 2);

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(top_bar);
    layout->addWidget(scroll_area_, 1);
    layout->addLayout(logs);
    setLayout(layout);
}

void GalleryWindow::setRoomTitle(const QString& title) {
    title_label_->setText(title);
}

void GalleryWindow::setPhotos(const std::vector<PhotoItem>& photos) {
    clearGrid();

    int index = 0;

    for (const auto& photo : photos) {
        grid_layout_->addWidget(createPhotoCard(photo), index / 4, index % 4);
        ++index;
    }

    grid_layout_->addWidget(createAddCard(), index / 4, index % 4);
    grid_layout_->setRowStretch(index / 4 + 1, 1);
}

void GalleryWindow::appendTrace(const QString& line) {
    trace_view_->appendPlainText("$ " + line);
    trace_view_->verticalScrollBar()->setValue(trace_view_->verticalScrollBar()->maximum());
}

void GalleryWindow::appendTerminal(const QString& text) {
    terminal_view_->appendPlainText(text.trimmed());
    terminal_view_->appendPlainText("");
    terminal_view_->verticalScrollBar()->setValue(terminal_view_->verticalScrollBar()->maximum());
}

void GalleryWindow::setRefreshCallback(SimpleCallback callback) {
    refresh_callback_ = std::move(callback);
}

void GalleryWindow::setAddCallback(SimpleCallback callback) {
    add_callback_ = std::move(callback);
}

void GalleryWindow::setDownloadCallback(PhotoCallback callback) {
    download_callback_ = std::move(callback);
}

void GalleryWindow::clearGrid() {
    while (QLayoutItem* item = grid_layout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

QWidget* GalleryWindow::createPhotoCard(const PhotoItem& item) {
    auto* card = new QFrame;
    card->setObjectName(item.full_downloaded ? "CardDownloaded" : "CardRemote");
    card->setFixedWidth(kCardWidth);

    auto* preview = new QLabel;
    preview->setFixedSize(kPreviewWidth, kPreviewHeight);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet("background: #111111; border-radius: 12px; color: #818C99;");

    const std::filesystem::path image_path = item.full_downloaded ? item.local_full_path : item.local_preview_path;
    QPixmap pixmap(QString::fromStdString(image_path.string()));

    if (!pixmap.isNull()) {
        preview->setPixmap(pixmap.scaled(kPreviewWidth, kPreviewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        preview->setText("preview\nnot loaded");
    }

    auto* name = new QLabel(QString::fromStdString(item.name));
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setMaximumHeight(42);
    name->setToolTip(QString::fromStdString(item.name));
    name->setStyleSheet("font-weight: 700; color: #E1E3E6; font-size: 12px;");

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(preview, 0, Qt::AlignCenter);
    layout->addWidget(name);

    if (!item.full_downloaded) {
        auto* download = new QPushButton("Download full photo");
        QObject::connect(download, &QPushButton::clicked, [this, item]() {
            if (download_callback_) {
                download_callback_(item);
            }
        });
        layout->addWidget(download);
    }

    card->setLayout(layout);

    return card;
}

QWidget* GalleryWindow::createAddCard() {
    auto* card = new QFrame;
    card->setObjectName("AddCard");
    card->setFixedWidth(kCardWidth);

    auto* plus = new QPushButton("+");
    plus->setFixedSize(kPreviewWidth, kPreviewHeight);
    plus->setStyleSheet("font-size: 54px; font-weight: 800; background: #102A43; color: #71AAEB; border-radius: 12px;");

    auto* text = new QLabel("Add photo");
    text->setAlignment(Qt::AlignCenter);
    text->setStyleSheet("font-weight: 800; color: #E1E3E6;");

    QObject::connect(plus, &QPushButton::clicked, [this]() {
        if (add_callback_) {
            add_callback_();
        }
    });

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    layout->addWidget(plus, 0, Qt::AlignCenter);
    layout->addWidget(text);
    card->setLayout(layout);

    return card;
}

} // namespace pear::relay::photo
