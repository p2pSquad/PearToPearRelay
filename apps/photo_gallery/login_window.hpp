#ifndef PEAR_RELAY_LOGIN_WINDOW_HPP_
#define PEAR_RELAY_LOGIN_WINDOW_HPP_

#include <functional>

#include <QLineEdit>
#include <QWidget>

namespace pear::relay::photo {

class LoginWindow final : public QWidget {
public:
    using ConnectCallback = std::function<void(const QString& relay_address, const QString& room_name, const QString& password)>;

    explicit LoginWindow(QWidget* parent = nullptr);

    void setConnectCallback(ConnectCallback callback);

private:
    QLineEdit* relay_edit_ = nullptr;
    QLineEdit* room_edit_ = nullptr;
    QLineEdit* password_edit_ = nullptr;
    ConnectCallback connect_callback_;
};

} // namespace pear::relay::photo

#endif // PEAR_RELAY_LOGIN_WINDOW_HPP_
