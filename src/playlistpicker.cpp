#include "playlistpicker.h"
#include <QKeyEvent>
#include <QFileInfo>

PlaylistPicker::PlaylistPicker(const QStringList &playlist, QWidget *parent)
    : QDialog(parent)
    , m_fullPlaylist(playlist)
{
    setWindowTitle("Select a playlist entry");
    setMinimumSize(500, 400);
    resize(600, 500);

    // Dark theme styling
    setStyleSheet(R"(
        QDialog {
            background-color: #1a1a1a;
            color: #ddd;
        }
        QLineEdit {
            background-color: #2a2a2a;
            color: #fff;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 8px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #4a9eff;
        }
        QListWidget {
            background-color: #2a2a2a;
            color: #ddd;
            border: 1px solid #444;
            border-radius: 4px;
            font-size: 13px;
        }
        QListWidget::item {
            padding: 6px;
        }
        QListWidget::item:selected {
            background-color: #4a9eff;
            color: #fff;
        }
        QListWidget::item:hover {
            background-color: #3a3a3a;
        }
        QLabel {
            color: #888;
            font-size: 12px;
        }
    )");

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Search input
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Type to search...");
    layout->addWidget(m_searchEdit);

    // Count label
    m_countLabel = new QLabel(this);
    layout->addWidget(m_countLabel);

    // List widget
    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget, 1);

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
        m_countLabel->setText(QString("%1 / %2 matches").arg(matchCount).arg(m_fullPlaylist.size()));
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
