#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

class ConfigPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigPanel(const QString &sourceDir, QWidget *parent = nullptr);

    [[nodiscard]] QString sourceDir() const;
    [[nodiscard]] QString filter() const;
    [[nodiscard]] int rows() const;
    [[nodiscard]] int cols() const;

    void setEnabled(bool enabled);
    void setCollapsed(bool collapsed);
    [[nodiscard]] bool isCollapsed() const { return m_collapsed; }

signals:
    void gridSizeChanged(int rows, int cols);

private:
    void setupUi();
    void toggleCollapse();

    QWidget *m_contentWidget = nullptr;
    QPushButton *m_collapseBtn = nullptr;
    QLineEdit *m_sourceEdit = nullptr;
    QLineEdit *m_filterEdit = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    QSpinBox *m_colsSpin = nullptr;
    bool m_collapsed = false;
};
