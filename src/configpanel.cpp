#include "configpanel.h"
#include "config.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

ConfigPanel::ConfigPanel(const QString &sourceDir, QWidget *parent)
    : QWidget(parent)
{
    setupUi();

    Config &cfg = Config::instance();
    QString initialDir = sourceDir.isEmpty() ? cfg.defaultMediaPath() : sourceDir;
    m_sourceEdit->setText(initialDir);
}

void ConfigPanel::setupUi()
{
    setStyleSheet(Theme::inputStyle() + QString(
        "QWidget { background: %1; }"
        "QLabel { color: %2; font-size: 11px; }"
    ).arg(Theme::Colors::Surface, Theme::Colors::TextMuted));

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(16);

    Config &cfg = Config::instance();

    // Grid
    auto *gridLabel = new QLabel("Grid");
    mainLayout->addWidget(gridLabel);

    m_colsSpin = new QSpinBox();
    m_colsSpin->setRange(1, 10);
    m_colsSpin->setValue(cfg.defaultCols());
    m_colsSpin->setFixedWidth(50);
    mainLayout->addWidget(m_colsSpin);

    auto *xLabel = new QLabel("x");
    mainLayout->addWidget(xLabel);

    m_rowsSpin = new QSpinBox();
    m_rowsSpin->setRange(1, 10);
    m_rowsSpin->setValue(cfg.defaultRows());
    m_rowsSpin->setFixedWidth(50);
    mainLayout->addWidget(m_rowsSpin);

    mainLayout->addSpacing(16);

    // Source
    auto *srcLabel = new QLabel("Source");
    mainLayout->addWidget(srcLabel);

    m_sourceEdit = new QLineEdit();
    m_sourceEdit->setMinimumWidth(250);
    mainLayout->addWidget(m_sourceEdit, 1);

    auto *browseBtn = new QPushButton("...");
    browseBtn->setFixedWidth(30);
    browseBtn->setStyleSheet(QString(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 3px; color: %3; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Theme::Colors::SurfaceLight, Theme::Colors::GlassBorder,
          Theme::Colors::TextPrimary, Theme::Colors::SurfaceHover));
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Media Directory", m_sourceEdit->text());
        if (!dir.isEmpty()) {
            m_sourceEdit->setText(dir);
        }
    });
    mainLayout->addWidget(browseBtn);

    mainLayout->addSpacing(16);

    // Filter
    auto *filterLabel = new QLabel("Filter");
    mainLayout->addWidget(filterLabel);

    m_filterEdit = new QLineEdit();
    m_filterEdit->setPlaceholderText("terms (AND)");
    m_filterEdit->setFixedWidth(150);
    mainLayout->addWidget(m_filterEdit);

    // Collapse button
    m_collapseBtn = new QPushButton("-");
    m_collapseBtn->setFixedSize(20, 20);
    m_collapseBtn->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; color: %1; font-weight: bold; }"
        "QPushButton:hover { color: %2; }"
    ).arg(Theme::Colors::TextMuted, Theme::Colors::TextPrimary));
    connect(m_collapseBtn, &QPushButton::clicked, this, &ConfigPanel::toggleCollapse);
    mainLayout->addWidget(m_collapseBtn);

    // Connections
    connect(m_colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int cols) {
        emit gridSizeChanged(m_rowsSpin->value(), cols);
    });
    connect(m_rowsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int rows) {
        emit gridSizeChanged(rows, m_colsSpin->value());
    });

    // Content widget for collapse (wrap the whole content)
    m_contentWidget = this; // Simplified - just hide the whole panel
}

QString ConfigPanel::sourceDir() const
{
    return m_sourceEdit->text();
}

QString ConfigPanel::filter() const
{
    return m_filterEdit->text().trimmed();
}

int ConfigPanel::rows() const
{
    return m_rowsSpin->value();
}

int ConfigPanel::cols() const
{
    return m_colsSpin->value();
}

void ConfigPanel::setEnabled(bool enabled)
{
    m_sourceEdit->setEnabled(enabled);
    m_filterEdit->setEnabled(enabled);
    m_rowsSpin->setEnabled(enabled);
    m_colsSpin->setEnabled(enabled);
}

void ConfigPanel::setCollapsed(bool collapsed)
{
    m_collapsed = collapsed;
    setVisible(!collapsed);
}

void ConfigPanel::toggleCollapse()
{
    setCollapsed(!m_collapsed);
}
