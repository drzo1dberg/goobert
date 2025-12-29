#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QMap>
#include <QPair>

// Custom tree widget with drag & drop support
class PlaylistTree : public QTreeWidget
{
    Q_OBJECT

public:
    explicit PlaylistTree(QWidget *parent = nullptr);

signals:
    void filesDropped(QTreeWidgetItem *targetCell, const QStringList &files);
    void fileMovedWithinCell(QTreeWidgetItem *cellItem);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QList<QTreeWidgetItem*> &items) const override;
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) override;
};

class PlaylistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);

    void setCellPlaylist(int row, int col, const QStringList &files);
    void updateCurrentFile(int row, int col, const QString &file);
    void clear();
    void removeFile(int row, int col, const QString &file);
    [[nodiscard]] QStringList getPlaylist(int row, int col) const;

signals:
    void fileSelected(int row, int col, const QString &file);
    void playlistReordered(int row, int col, const QStringList &files);
    void fileRemovedFromPlaylist(int row, int col, const QString &file);

private slots:
    void onFilesDropped(QTreeWidgetItem *targetCell, const QStringList &files);
    void onFileMovedWithinCell(QTreeWidgetItem *cellItem);
    void onContextMenu(const QPoint &pos);

private:
    void setupUi();
    QTreeWidgetItem* findOrCreateCellItem(int row, int col);
    QStringList getPlaylistFromItem(QTreeWidgetItem *cellItem) const;

    PlaylistTree *m_tree = nullptr;
    QMap<QPair<int,int>, QTreeWidgetItem*> m_cellItems;
};
