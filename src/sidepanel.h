#pragma once

#include <QWidget>
#include <QTabWidget>
#include "monitorwidget.h"
#include "playlistwidget.h"

class SidePanel : public QWidget
{
    Q_OBJECT

public:
    explicit SidePanel(QWidget *parent = nullptr);

    MonitorWidget* monitor() const { return m_monitor; }
    PlaylistWidget* playlist() const { return m_playlist; }

    void showMonitor();
    void showPlaylist();

signals:
    void cellSelected(int row, int col);
    void fileRenamed(const QString &oldPath, const QString &newPath);
    void customSourceRequested(int row, int col, const QStringList &paths);
    void fileSelected(int row, int col, const QString &file);

private:
    void setupUi();

    QTabWidget *m_tabs = nullptr;
    MonitorWidget *m_monitor = nullptr;
    PlaylistWidget *m_playlist = nullptr;
};
