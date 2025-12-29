#include "playlistwidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileInfo>

PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PlaylistWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabels({"Playlist"});
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setVisible(false);

    m_tree->setStyleSheet(R"(
        QTreeWidget {
            background-color: #1e1e1e;
            color: #ccc;
            border: none;
        }
        QTreeWidget::item { padding: 4px; }
        QTreeWidget::item:selected { background-color: #3a5a8a; }
        QTreeWidget::item:alternate { background-color: #222; }
        QTreeWidget::branch:has-children:!has-siblings:closed,
        QTreeWidget::branch:closed:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-closed.png);
        }
        QTreeWidget::branch:open:has-children:!has-siblings,
        QTreeWidget::branch:open:has-children:has-siblings {
            border-image: none;
            image: url(:/icons/branch-open.png);
        }
    )");

    // TODO Phase 2: Enable drag & drop
    // m_tree->setDragEnabled(true);
    // m_tree->setAcceptDrops(true);
    // m_tree->setDragDropMode(QAbstractItemView::InternalMove);

    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (!item || !item->parent()) return;  // Only handle file items, not cell items

        QTreeWidgetItem *cellItem = item->parent();
        int row = cellItem->data(0, Qt::UserRole).toInt();
        int col = cellItem->data(0, Qt::UserRole + 1).toInt();
        QString file = item->data(0, Qt::UserRole).toString();

        emit fileSelected(row, col, file);
    });

    layout->addWidget(m_tree);
}

QTreeWidgetItem* PlaylistWidget::findOrCreateCellItem(int row, int col)
{
    QPair<int,int> key{row, col};

    if (m_cellItems.contains(key)) {
        return m_cellItems[key];
    }

    auto *item = new QTreeWidgetItem(m_tree);
    item->setText(0, QString("Cell [%1,%2]").arg(row).arg(col));
    item->setData(0, Qt::UserRole, row);
    item->setData(0, Qt::UserRole + 1, col);
    item->setExpanded(false);

    // Style the cell header
    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);

    m_cellItems[key] = item;
    return item;
}

void PlaylistWidget::setCellPlaylist(int row, int col, const QStringList &files)
{
    QTreeWidgetItem *cellItem = findOrCreateCellItem(row, col);

    // Clear existing children
    while (cellItem->childCount() > 0) {
        delete cellItem->takeChild(0);
    }

    // Add files
    for (const QString &file : files) {
        QFileInfo fi(file);
        auto *fileItem = new QTreeWidgetItem(cellItem);
        fileItem->setText(0, fi.fileName());
        fileItem->setData(0, Qt::UserRole, file);  // Store full path
        fileItem->setToolTip(0, file);
    }

    // Update cell item text with count
    cellItem->setText(0, QString("Cell [%1,%2] (%3 files)")
        .arg(row).arg(col).arg(files.size()));
}

void PlaylistWidget::updateCurrentFile(int row, int col, const QString &file)
{
    QPair<int,int> key{row, col};
    if (!m_cellItems.contains(key)) return;

    QTreeWidgetItem *cellItem = m_cellItems[key];

    // Reset all items styling, highlight current
    for (int i = 0; i < cellItem->childCount(); ++i) {
        QTreeWidgetItem *child = cellItem->child(i);
        QString itemPath = child->data(0, Qt::UserRole).toString();

        QFont font = child->font(0);
        if (itemPath == file) {
            font.setBold(true);
            child->setForeground(0, QColor("#6a9fd4"));
        } else {
            font.setBold(false);
            child->setForeground(0, QColor("#ccc"));
        }
        child->setFont(0, font);
    }
}

void PlaylistWidget::clear()
{
    m_tree->clear();
    m_cellItems.clear();
}
