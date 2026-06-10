#include "login_window.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#ifndef PEAR_RELAY_ASSET_DIR
#define PEAR_RELAY_ASSET_DIR "."
#endif

namespace pear::relay::photo {

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Pear Family Photos");
    resize(500, 330);
    setStyleSheet(R"css(
        QWidget {
            background: #141414;
            color: #E1E3E6;
            font-family: Arial;
            font-size: 14px;
        }

        QLabel#Title {
            color: #FFFFFF;
            font-size: 30px;
            font-weight: 800;
        }

        QLabel#FieldLabel {
            color: #AEB7C2;
            font-size: 13px;
            font-weight: 600;
        }

        QLineEdit {
            background: #222222;
            color: #E1E3E6;
            border: 1px solid #333333;
            border-radius: 12px;
            padding: 11px;
            min-height: 24px;
            selection-background-color: #0077FF;
        }

        QLineEdit:focus {
            border: 1px solid #0077FF;
        }

        QPushButton {
            background: #0077FF;
            color: #FFFFFF;
            border: none;
            border-radius: 12px;
            padding: 12px 18px;
            font-weight: 700;
        }

        QPushButton:hover {
            background: #2688EB;
        }

        QPushButton#Secondary {
            background: #2A2D30;
            color: #71AAEB;
        }

        QPushButton#Secondary:hover {
            background: #33373A;
        }
    )css");

    auto* logo = new QLabel;
    logo->setAlignment(Qt::AlignCenter);
    QPixmap logo_pixmap(QString(PEAR_RELAY_ASSET_DIR) + "/logo.png");

    if (!logo_pixmap.isNull()) {
        logo->setPixmap(logo_pixmap.scaled(84, 84, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    auto* title = new QLabel("Pear Family Photos");
    title->setObjectName("Title");
    title->setAlignment(Qt::AlignCenter);

    relay_edit_ = new QLineEdit("127.0.0.1:50051");
    relay_edit_->hide();

    room_edit_ = new QLineEdit("family");
    password_edit_ = new QLineEdit;
    password_edit_->setEchoMode(QLineEdit::Password);
    password_edit_->setPlaceholderText("password");

    auto* room_label = new QLabel("Room");
    room_label->setObjectName("FieldLabel");

    auto* password_label = new QLabel("Password");
    password_label->setObjectName("FieldLabel");

    auto* create_button = new QPushButton("Create room");
    auto* join_button = new QPushButton("Join room");
    join_button->setObjectName("Secondary");

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(12);
    buttons->addWidget(create_button);
    buttons->addWidget(join_button);

    auto* layout = new QVBoxLayout;
    layout->setContentsMargins(42, 24, 42, 30);
    layout->setSpacing(11);
    layout->addWidget(logo);
    layout->addWidget(title);
    layout->addSpacing(10);
    layout->addWidget(room_label);
    layout->addWidget(room_edit_);
    layout->addWidget(password_label);
    layout->addWidget(password_edit_);
    layout->addSpacing(10);
    layout->addLayout(buttons);
    setLayout(layout);

    auto connect_action = [this]() {
        if (connect_callback_) {
            connect_callback_(relay_edit_->text(), room_edit_->text(), password_edit_->text());
        }
    };

    QObject::connect(create_button, &QPushButton::clicked, connect_action);
    QObject::connect(join_button, &QPushButton::clicked, connect_action);
}

void LoginWindow::setConnectCallback(ConnectCallback callback) {
    connect_callback_ = std::move(callback);
}

} // namespace pear::relay::photo
