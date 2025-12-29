#include "toolbar.h"
#include "config.h"
#include <QAction>
#include <QWidget>
#include <QHBoxLayout>

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setupUi();
}

void ToolBar::setupUi()
{
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(20, 20));
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    setStyleSheet(R"(
        QToolBar { background-color: #1a1a1a; border: none; spacing: 4px; padding: 4px; }
        QToolButton { background-color: #2a2a2a; border: none; padding: 6px 10px; color: #ccc; }
        QToolButton:hover { background-color: #3a3a3a; }
        QToolButton:disabled { color: #555; }
        QToolButton:checked { background-color: #4a4a4a; }
        QSlider::groove:horizontal { background: #333; height: 6px; border-radius: 3px; }
        QSlider::handle:horizontal { background: #888; width: 14px; margin: -4px 0; border-radius: 7px; }
        QSlider::handle:horizontal:hover { background: #aaa; }
        QLabel { color: #888; padding: 0 4px; }
    )");

    // Start/Stop actions
    m_startAction = addAction("Start");
    m_startAction->setToolTip("Start grid playback");
    connect(m_startAction, &QAction::triggered, this, &ToolBar::startClicked);

    m_stopAction = addAction("Stop");
    m_stopAction->setToolTip("Stop grid playback");
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this, &ToolBar::stopClicked);

    addSeparator();

    // Fullscreen
    QAction *fsAction = addAction("Fullscreen");
    fsAction->setToolTip("Toggle fullscreen (F11)");
    connect(fsAction, &QAction::triggered, this, &ToolBar::fullscreenClicked);

    addSeparator();

    // Playback controls
    QAction *prevAction = addAction("Prev");
    prevAction->setToolTip("Previous track");
    connect(prevAction, &QAction::triggered, this, &ToolBar::prevClicked);

    QAction *playPauseAction = addAction("Play");
    playPauseAction->setToolTip("Play/Pause all (Space)");
    connect(playPauseAction, &QAction::triggered, this, &ToolBar::playPauseClicked);

    QAction *nextAction = addAction("Next");
    nextAction->setToolTip("Next track (E)");
    connect(nextAction, &QAction::triggered, this, &ToolBar::nextClicked);

    QAction *shuffleAction = addAction("Shuffle");
    shuffleAction->setToolTip("Shuffle all playlists (Q)");
    connect(shuffleAction, &QAction::triggered, this, &ToolBar::shuffleClicked);

    addSeparator();

    // Mute
    QAction *muteAction = addAction("Mute");
    muteAction->setToolTip("Toggle mute (M)");
    connect(muteAction, &QAction::triggered, this, &ToolBar::muteClicked);

    // Volume slider in a widget
    auto *volWidget = new QWidget();
    auto *volLayout = new QHBoxLayout(volWidget);
    volLayout->setContentsMargins(4, 0, 4, 0);
    volLayout->setSpacing(4);

    m_volumeLabel = new QLabel("Vol");
    volLayout->addWidget(m_volumeLabel);

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setToolTip("Volume (U/I)");
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_volumeLabel->setText(QString("Vol %1%").arg(val));
        emit volumeChanged(val);
    });
    volLayout->addWidget(m_volumeSlider);

    addWidget(volWidget);

    // Set initial volume from config
    Config &cfg = Config::instance();
    m_volumeSlider->setValue(cfg.defaultVolume());

    // Spacer to push everything left
    auto *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addWidget(spacer);
}

void ToolBar::setRunning(bool running)
{
    m_startAction->setEnabled(!running);
    m_stopAction->setEnabled(running);
}

void ToolBar::setVolume(int volume)
{
    m_volumeSlider->setValue(volume);
}
