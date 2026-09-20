#include "editing_silhouette.h"

#include <QPainter>

EditingSilhouette::EditingSilhouette(QWidget* parent, const QPoint& pos, const QSize& size)
    : QWidget(parent), size_(size), pos_(pos)
{
    move(pos_);
    resize(size_);
    hide();
}

void EditingSilhouette::setPosAndSize(const QPoint& pos, const QSize& size)
{
    pos_ = pos;
    size_ = size;

    resize(size_);
    move(pos_);

    update();
}

void EditingSilhouette::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setPen(QPen(QColor(0, 0, 0)));
    painter.setBrush(QBrush(QColor(0, 0, 0, 0)));
    painter.drawRect(0, 0, size_.width() - 1, size_.height() - 1);
}
