#ifndef NEW_QT_APPLICATION_EDITING_SILHOUETTE_H_
#define NEW_QT_APPLICATION_EDITING_SILHOUETTE_H_

#include <QPoint>
#include <QSize>
#include <QWidget>

// Faithful port of main_application/editing_silhouette.{h,cpp}: a
// transparent-fill, black-outline rectangle overlay shown while an element
// is being resized/moved via GuiElement's edit-mode (Ctrl+drag).
class EditingSilhouette : public QWidget
{
    Q_OBJECT

public:
    EditingSilhouette() = delete;
    EditingSilhouette(QWidget* parent, const QPoint& pos, const QSize& size);

    void setPosAndSize(const QPoint& pos, const QSize& size);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QSize size_;
    QPoint pos_;
};

#endif  // NEW_QT_APPLICATION_EDITING_SILHOUETTE_H_
