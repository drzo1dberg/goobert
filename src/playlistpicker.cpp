#include "playlistpicker.h"
#include "theme.h"
#include <QKeyEvent>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>

PlaylistPicker::PlaylistPicker(const QStringList &playlist, QWidget *parent)
    : QDialog(parent)
    , m_fullPlaylist(playlist)
{
    setWindowTitle("Select a playlist entry");
    setMinimumSize(800, 500);
    resize(1000, 700);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // Apply 2026 theme with glassmorphism
    setStyleSheet(Theme::dialogStyle() + Theme::inputStyle() + Theme::listWidgetStyle());

    // Main container with glass effect
    auto *container = new QWidget(this);
    container->setObjectName("PickerContainer");
    container->setStyleSheet(QString(
        "QWidget#PickerContainer {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "}"
    ).arg(Theme::Colors::GlassBg, Theme::Colors::GlassBorder, QString::number(Theme::Radius::XL)));

    // Add shadow to container
    Theme::addShadow(container, 30, 8);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(Theme::Spacing::LG, Theme::Spacing::LG, Theme::Spacing::LG, Theme::Spacing::LG);
    outerLayout->addWidget(container);

    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(Theme::Spacing::XL, Theme::Spacing::XL, Theme::Spacing::XL, Theme::Spacing::XL);
    layout->setSpacing(Theme::Spacing::MD);

    // Header with title and close button
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("Select File", this);
    titleLabel->setStyleSheet(QString(
        "font-size: 16px; font-weight: 600; color: %1;"
    ).arg(Theme::Colors::TextPrimary));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    auto *closeBtn = new QPushButton("x", this);
    closeBtn->setFixedSize(24, 24);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QString(
        "QPushButton { background: %1; border: none; border-radius: 12px; color: %2; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: %3; color: white; }"
    ).arg(Theme::Colors::SurfaceHover, Theme::Colors::TextSecondary, Theme::Colors::Error));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn);

    layout->addLayout(headerLayout);

    // Search input
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Type to search...");
    layout->addWidget(m_searchEdit);

    // Count label
    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Theme::Colors::TextMuted));
    layout->addWidget(m_countLabel);

    // List widget - no word wrap, elide long text
    m_listWidget = new QListWidget(this);
    m_listWidget->setWordWrap(false);
    m_listWidget->setTextElideMode(Qt::ElideMiddle);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_listWidget, 1);

    // Keyboard hints at bottom
    auto *hintsLabel = new QLabel("↑↓ Navigate  •  Enter Select  •  Esc Close", this);
    hintsLabel->setAlignment(Qt::AlignCenter);
    hintsLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px; padding: 8px; background: %2; border-radius: %3px;"
    ).arg(Theme::Colors::TextMuted, Theme::Colors::SurfaceLight, QString::number(Theme::Radius::SM)));
    layout->addWidget(hintsLabel);

    // Build display names
    m_displayNames.reserve(m_fullPlaylist.size());
    for (const QString &path : m_fullPlaylist) {
        m_displayNames.append(QFileInfo(path).fileName());
    }

    // Initial list
    updateList();

    // Connections
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PlaylistPicker::onSearchTextChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &PlaylistPicker::onItemDoubleClicked);
    connect(m_listWidget, &QListWidget::itemActivated, this, &PlaylistPicker::onItemActivated);

    // Focus search on open
    m_searchEdit->setFocus();
}

void PlaylistPicker::onSearchTextChanged(const QString &text)
{
    m_searchText = text.toLower();
    updateList();
}

void PlaylistPicker::updateList()
{
    m_listWidget->clear();

    int matchCount = 0;
    for (int i = 0; i < m_displayNames.size(); ++i) {
        const QString &name = m_displayNames[i];

        // Filter by search text (case insensitive)
        if (m_searchText.isEmpty() || name.toLower().contains(m_searchText)) {
            auto *item = new QListWidgetItem(name);
            item->setData(Qt::UserRole, i);  // Store original index
            m_listWidget->addItem(item);
            ++matchCount;
        }
    }

    // Update count label
    if (m_searchText.isEmpty()) {
        m_countLabel->setText(QString("%1 files").arg(m_fullPlaylist.size()));
    } else {
        m_countLabel->setText(QString("%1 of %2 matches").arg(matchCount).arg(m_fullPlaylist.size()));
    }

    // Select first item if available
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void PlaylistPicker::selectItem(QListWidgetItem *item)
{
    if (!item) return;

    int originalIndex = item->data(Qt::UserRole).toInt();
    if (originalIndex >= 0 && originalIndex < m_fullPlaylist.size()) {
        m_selectedFile = m_fullPlaylist[originalIndex];
        m_selectedIndex = originalIndex;
        accept();
    }
}

void PlaylistPicker::onItemDoubleClicked(QListWidgetItem *item)
{
    selectItem(item);
}

void PlaylistPicker::onItemActivated(QListWidgetItem *item)
{
    selectItem(item);
}

void PlaylistPicker::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        reject();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        selectItem(m_listWidget->currentItem());
        break;
    case Qt::Key_Up:
        if (m_listWidget->currentRow() > 0) {
            m_listWidget->setCurrentRow(m_listWidget->currentRow() - 1);
        }
        break;
    case Qt::Key_Down:
        if (m_listWidget->currentRow() < m_listWidget->count() - 1) {
            m_listWidget->setCurrentRow(m_listWidget->currentRow() + 1);
        }
        break;
    default:
        QDialog::keyPressEvent(event);
    }
}
