#include "sidepanel.h"
#include "theme.h"
#include <QVBoxLayout>

SidePanel::SidePanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SidePanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::Spacing::SM, Theme::Spacing::SM, Theme::Spacing::SM, Theme::Spacing::SM);

    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::North);
    m_tabs->setDocumentMode(true);

    // Apply 2026 theme
    m_tabs->setStyleSheet(Theme::tabWidgetStyle());

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
