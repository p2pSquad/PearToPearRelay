#ifndef PEAR_RELAY_GALLERY_WINDOW_HPP_
#define PEAR_RELAY_GALLERY_WINDOW_HPP_

#include "photo_controller.hpp"

#include <functional>
#include <vector>

#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QWidget>

namespace pear::relay::photo {

class GalleryWindow final : public QWidget {
public:
    using SimpleCallback = std::function<void()>;
    using PhotoCallback = std::function<void(const PhotoItem& item)>;

    explicit GalleryWindow(QWidget* parent = nullptr);

    void setRoomTitle(const QString& title);
    void setPhotos(const std::vector<PhotoItem>& photos);
    void appendTrace(const QString& line);
    void appendTerminal(const QString& text);
    void setRefreshCallback(SimpleCallback callback);
    void setAddCallback(SimpleCallback callback);
    void setDownloadCallback(PhotoCallback callback);

private:
    void clearGrid();
    QWidget* createPhotoCard(const PhotoItem& item);
    QWidget* createAddCard();

    QLabel* title_label_ = nullptr;
    QScrollArea* scroll_area_ = nullptr;
    QWidget* grid_widget_ = nullptr;
    QGridLayout* grid_layout_ = nullptr;
    QPlainTextEdit* trace_view_ = nullptr;
    QPlainTextEdit* terminal_view_ = nullptr;

    SimpleCallback refresh_callback_;
    SimpleCallback add_callback_;
    PhotoCallback download_callback_;
};

} // namespace pear::relay::photo

#endif // PEAR_RELAY_GALLERY_WINDOW_HPP_
