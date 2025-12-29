#include "configpanel.h"
#include "config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <utility>

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
    setStyleSheet(R"(
        QWidget { background-color: #1a1a1a; color: #ccc; }
        QPushButton { background-color: #2a2a2a; border: none; padding: 4px 8px; }
        QPushButton:hover { background-color: #3a3a3a; }
        QLineEdit, QSpinBox {
            background-color: #2a2a2a;
            border: 1px solid #333;
            padding: 4px;
            color: #ccc;
        }
        QLabel { color: #888; }
    )");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(4);

    // Header with collapse button
    auto *headerLayout = new QHBoxLayout();
    auto *headerLabel = new QLabel("Configuration");
    headerLabel->setStyleSheet("font-weight: bold; color: #aaa;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    m_collapseBtn = new QPushButton("-");
    m_collapseBtn->setFixedSize(20, 20);
    m_collapseBtn->setToolTip("Collapse/Expand");
    connect(m_collapseBtn, &QPushButton::clicked, this, &ConfigPanel::toggleCollapse);
    headerLayout->addWidget(m_collapseBtn);

    mainLayout->addLayout(headerLayout);

    // Content widget (collapsible)
    m_contentWidget = new QWidget();
    auto *contentLayout = new QHBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 4, 0, 0);
    contentLayout->setSpacing(12);

    Config &cfg = Config::instance();

    // Grid size
    auto *gridLayout = new QHBoxLayout();
    gridLayout->addWidget(new QLabel("Grid:"));
    m_colsSpin = new QSpinBox();
    m_colsSpin->setRange(1, 10);
    m_colsSpin->setValue(cfg.defaultCols());
    m_colsSpin->setFixedWidth(50);
    gridLayout->addWidget(m_colsSpin);
    gridLayout->addWidget(new QLabel("x"));
    m_rowsSpin = new QSpinBox();
    m_rowsSpin->setRange(1, 10);
    m_rowsSpin->setValue(cfg.defaultRows());
    m_rowsSpin->setFixedWidth(50);
    gridLayout->addWidget(m_rowsSpin);
    contentLayout->addLayout(gridLayout);

    // Source
    auto *sourceLayout = new QHBoxLayout();
    sourceLayout->addWidget(new QLabel("Source:"));
    m_sourceEdit = new QLineEdit();
    m_sourceEdit->setMinimumWidth(200);
    sourceLayout->addWidget(m_sourceEdit);

    auto *browseBtn = new QPushButton("...");
    browseBtn->setFixedWidth(30);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Media Directory", m_sourceEdit->text());
        if (!dir.isEmpty()) {
            m_sourceEdit->setText(dir);
        }
    });
    sourceLayout->addWidget(browseBtn);
    contentLayout->addLayout(sourceLayout);

    // Filter
    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Filter:"));
    m_filterEdit = new QLineEdit();
    m_filterEdit->setPlaceholderText("space-separated AND filter");
    m_filterEdit->setMinimumWidth(150);
    filterLayout->addWidget(m_filterEdit);
    contentLayout->addLayout(filterLayout);

    contentLayout->addStretch();

    mainLayout->addWidget(m_contentWidget);

    // Connect grid size changes
    connect(m_colsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int cols) {
        emit gridSizeChanged(m_rowsSpin->value(), cols);
    });
    connect(m_rowsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int rows) {
        emit gridSizeChanged(rows, m_colsSpin->value());
    });
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
    m_contentWidget->setVisible(!collapsed);
    m_collapseBtn->setText(collapsed ? "+" : "-");
}

void ConfigPanel::toggleCollapse()
{
    setCollapsed(!m_collapsed);
}
