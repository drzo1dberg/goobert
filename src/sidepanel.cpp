#include "sidepanel.h"
#include <QVBoxLayout>

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SidePanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::North);
    m_tabs->setDocumentMode(true);

    m_tabs->setStyleSheet(R"(
        QTabWidget::pane {
            border: none;
            background-color: #1e1e1e;
        }
        QTabBar::tab {
            background-color: #2a2a2a;
            color: #888;
            padding: 8px 16px;
            border: none;
            border-bottom: 2px solid transparent;
        }
        QTabBar::tab:selected {
            color: #ccc;
            background-color: #1e1e1e;
            border-bottom: 2px solid #4a9eff;
        }
        QTabBar::tab:hover:!selected {
            background-color: #333;
        }
    )");

    // Monitor tab
    m_monitor = new MonitorWidget();
    m_tabs->addTab(m_monitor, "Monitor");

    // Playlist tab
    m_playlist = new PlaylistWidget();
    m_tabs->addTab(m_playlist, "Playlist");

    layout->addWidget(m_tabs);

    // Forward signals
    connect(m_monitor, &MonitorWidget::cellSelected, this, &SidePanel::cellSelected);
    connect(m_monitor, &MonitorWidget::fileRenamed, this, &SidePanel::fileRenamed);
    connect(m_monitor, &MonitorWidget::customSourceRequested, this, &SidePanel::customSourceRequested);
    connect(m_playlist, &PlaylistWidget::fileSelected, this, &SidePanel::fileSelected);
}

void SidePanel::showMonitor()
{
    m_tabs->setCurrentWidget(m_monitor);
}

void SidePanel::showPlaylist()
{
    m_tabs->setCurrentWidget(m_playlist);
}
