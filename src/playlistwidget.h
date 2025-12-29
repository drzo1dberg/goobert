#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QMap>
#include <QPair>

class PlaylistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);

    void setCellPlaylist(int row, int col, const QStringList &files);
    void updateCurrentFile(int row, int col, const QString &file);
    void clear();

signals:
    void fileSelected(int row, int col, const QString &file);
    void playlistReordered(int row, int col, const QStringList &files);

private:
    void setupUi();
    QTreeWidgetItem* findOrCreateCellItem(int row, int col);

    QTreeWidget *m_tree = nullptr;
    QMap<QPair<int,int>, QTreeWidgetItem*> m_cellItems;
};
