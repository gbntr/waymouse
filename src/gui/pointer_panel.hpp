#pragma once

#include <QWidget>
#include <memory>

namespace waymouse {

class PointerManager;

class PointerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PointerPanel(PointerManager* manager, QWidget* parent = nullptr);
    ~PointerPanel() override;

public slots:
    void refresh();

private slots:
    void onThemeChanged(int index);
    void onSizeChanged(int value);
    void onResetClicked();

private:
    void setupUi();
    void updatePreview();
    void updateBadge();
    QPixmap loadPixmapFromTheme(const std::string& theme_name, int size) const;

    PointerManager* m_manager;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace waymouse
