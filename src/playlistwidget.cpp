#include "playlistwidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QUrl>

// ============================================================================
// PlaylistTree - Custom QTreeWidget with drag & drop
// ============================================================================

PlaylistTree::PlaylistTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
}

QStringList PlaylistTree::mimeTypes() const
{
    return {"text/uri-list", "application/x-playlist-items"};
}

QMimeData* PlaylistTree::mimeData(const QList<QTreeWidgetItem*> &items) const
{
    auto *mimeData = new QMimeData();
    QList<QUrl> urls;
    QStringList paths;

    for (QTreeWidgetItem *item : items) {
        // Only allow dragging file items (not cell headers)
        if (item->parent()) {
            QString path = item->data(0, Qt::UserRole).toString();
            if (!path.isEmpty()) {
                urls.append(QUrl::fromLocalFile(path));
                paths.append(path);
            }
        }
    }

    mimeData->setUrls(urls);
    mimeData->setData("application/x-playlist-items", paths.join("\n").toUtf8());
    return mimeData;
}

void PlaylistTree::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasFormat("application/x-playlist-items")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void PlaylistTree::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->position().toPoint());

    // Allow drop on cell headers or between file items
    if (item) {
        // If hovering over a file item, target its parent cell
        QTreeWidgetItem *targetCell = item->parent() ? item->parent() : item;
        if (!targetCell->parent()) {  // It's a cell header
            event->acceptProposedAction();
            return;
        }
    }
    event->acceptProposedAction();
}

void PlaylistTree::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem *item = itemAt(event->position().toPoint());
    if (!item) {
        event->ignore();
        return;
    }

    // Find target cell (parent if dropping on file, item itself if cell header)
    QTreeWidgetItem *targetCell = item->parent() ? item->parent() : item;

    // Only accept drops on cell headers
    if (targetCell->parent()) {
        event->ignore();
        return;
    }

    const QMimeData *mimeData = event->mimeData();
    QStringList paths;

    if (mimeData->hasFormat("application/x-playlist-items")) {
        // Internal move
        QString data = QString::fromUtf8(mimeData->data("application/x-playlist-items"));
        paths = data.split("\n", Qt::SkipEmptyParts);
    } else if (mimeData->hasUrls()) {
        // External file drop
        for (const QUrl &url : mimeData->urls()) {
            if (url.isLocalFile()) {
                paths.append(url.toLocalFile());
            }
        }
    }

    if (!paths.isEmpty()) {
        emit filesDropped(targetCell, paths);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

bool PlaylistTree::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    Q_UNUSED(index);
    Q_UNUSED(action);

    if (!parent || parent->parent()) {
        // Only accept drops on cell headers (top-level items)
        return false;
    }

    QStringList paths;
    if (data->hasFormat("application/x-playlist-items")) {
        QString strData = QString::fromUtf8(data->data("application/x-playlist-items"));
        paths = strData.split("\n", Qt::SkipEmptyParts);
    }

    if (!paths.isEmpty()) {
        emit filesDropped(parent, paths);
        return true;
    }
    return false;
}

// ============================================================================
// PlaylistWidget
// ============================================================================

PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void PlaylistWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new PlaylistTree();
    m_tree->setHeaderLabels({"Playlist"});
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->header()->setVisible(false);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);

    m_tree->setStyleSheet(R"(
        QTreeWidget {
            background-color: #1e1e1e;
            color: #ccc;
            border: none;
        }
        QTreeWidget::item { padding: 4px; }
        QTreeWidget::item:selected { background-color: #3a5a8a; }
        QTreeWidget::item:alternate { background-color: #222; }
        QTreeWidget::item:hover { background-color: #2a2a2a; }
    )");

    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
        if (!item || !item->parent()) return;

        QTreeWidgetItem *cellItem = item->parent();
        int row = cellItem->data(0, Qt::UserRole).toInt();
        int col = cellItem->data(0, Qt::UserRole + 1).toInt();
        QString file = item->data(0, Qt::UserRole).toString();

        emit fileSelected(row, col, file);
    });

    connect(m_tree, &PlaylistTree::filesDropped, this, &PlaylistWidget::onFilesDropped);
    connect(m_tree, &PlaylistTree::fileMovedWithinCell, this, &PlaylistWidget::onFileMovedWithinCell);
    connect(m_tree, &QWidget::customContextMenuRequested, this, &PlaylistWidget::onContextMenu);

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
    item->setFlags(item->flags() | Qt::ItemIsDropEnabled);

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
        fileItem->setData(0, Qt::UserRole, file);
        fileItem->setToolTip(0, file);
        fileItem->setFlags(fileItem->flags() | Qt::ItemIsDragEnabled);
    }

    cellItem->setText(0, QString("Cell [%1,%2] (%3)")
        .arg(row).arg(col).arg(files.size()));
}

void PlaylistWidget::updateCurrentFile(int row, int col, const QString &file)
{
    QPair<int,int> key{row, col};
    if (!m_cellItems.contains(key)) return;

    QTreeWidgetItem *cellItem = m_cellItems[key];

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

void PlaylistWidget::removeFile(int row, int col, const QString &file)
{
    QPair<int,int> key{row, col};
    if (!m_cellItems.contains(key)) return;

    QTreeWidgetItem *cellItem = m_cellItems[key];

    for (int i = 0; i < cellItem->childCount(); ++i) {
        QTreeWidgetItem *child = cellItem->child(i);
        if (child->data(0, Qt::UserRole).toString() == file) {
            delete cellItem->takeChild(i);
            break;
        }
    }

    // Update count in header
    cellItem->setText(0, QString("Cell [%1,%2] (%3)")
        .arg(row).arg(col).arg(cellItem->childCount()));
}

QStringList PlaylistWidget::getPlaylist(int row, int col) const
{
    QPair<int,int> key{row, col};
    if (!m_cellItems.contains(key)) return {};

    return getPlaylistFromItem(m_cellItems[key]);
}

QStringList PlaylistWidget::getPlaylistFromItem(QTreeWidgetItem *cellItem) const
{
    QStringList files;
    for (int i = 0; i < cellItem->childCount(); ++i) {
        files.append(cellItem->child(i)->data(0, Qt::UserRole).toString());
    }
    return files;
}

void PlaylistWidget::onFilesDropped(QTreeWidgetItem *targetCell, const QStringList &files)
{
    if (!targetCell) return;

    int row = targetCell->data(0, Qt::UserRole).toInt();
    int col = targetCell->data(0, Qt::UserRole + 1).toInt();

    // Add files to the cell
    for (const QString &file : files) {
        QFileInfo fi(file);
        auto *fileItem = new QTreeWidgetItem(targetCell);
        fileItem->setText(0, fi.fileName());
        fileItem->setData(0, Qt::UserRole, file);
        fileItem->setToolTip(0, file);
        fileItem->setFlags(fileItem->flags() | Qt::ItemIsDragEnabled);
    }

    // Update header count
    targetCell->setText(0, QString("Cell [%1,%2] (%3)")
        .arg(row).arg(col).arg(targetCell->childCount()));

    // Emit reordered signal with new playlist
    emit playlistReordered(row, col, getPlaylistFromItem(targetCell));
}

void PlaylistWidget::onFileMovedWithinCell(QTreeWidgetItem *cellItem)
{
    if (!cellItem) return;

    int row = cellItem->data(0, Qt::UserRole).toInt();
    int col = cellItem->data(0, Qt::UserRole + 1).toInt();

    emit playlistReordered(row, col, getPlaylistFromItem(cellItem));
}

void PlaylistWidget::onContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (!item || !item->parent()) return;  // Only for file items

    QTreeWidgetItem *cellItem = item->parent();
    int row = cellItem->data(0, Qt::UserRole).toInt();
    int col = cellItem->data(0, Qt::UserRole + 1).toInt();
    QString file = item->data(0, Qt::UserRole).toString();

    QMenu menu(this);
    QAction *playAction = menu.addAction("Play this file");
    QAction *removeAction = menu.addAction("Remove from playlist");
    menu.addSeparator();
    QAction *moveUpAction = menu.addAction("Move up");
    QAction *moveDownAction = menu.addAction("Move down");

    // Disable move actions at boundaries
    int idx = cellItem->indexOfChild(item);
    moveUpAction->setEnabled(idx > 0);
    moveDownAction->setEnabled(idx < cellItem->childCount() - 1);

    QAction *selected = menu.exec(m_tree->viewport()->mapToGlobal(pos));

    if (selected == playAction) {
        emit fileSelected(row, col, file);
    } else if (selected == removeAction) {
        delete cellItem->takeChild(idx);
        cellItem->setText(0, QString("Cell [%1,%2] (%3)")
            .arg(row).arg(col).arg(cellItem->childCount()));
        emit playlistReordered(row, col, getPlaylistFromItem(cellItem));
        emit fileRemovedFromPlaylist(row, col, file);
    } else if (selected == moveUpAction && idx > 0) {
        QTreeWidgetItem *taken = cellItem->takeChild(idx);
        cellItem->insertChild(idx - 1, taken);
        m_tree->setCurrentItem(taken);
        emit playlistReordered(row, col, getPlaylistFromItem(cellItem));
    } else if (selected == moveDownAction && idx < cellItem->childCount()) {
        QTreeWidgetItem *taken = cellItem->takeChild(idx);
        cellItem->insertChild(idx + 1, taken);
        m_tree->setCurrentItem(taken);
        emit playlistReordered(row, col, getPlaylistFromItem(cellItem));
    }
}
