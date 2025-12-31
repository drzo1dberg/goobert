#include "toolbar.h"
#include "config.h"
#include "theme.h"
#include <QHBoxLayout>
#include <QFileDialog>

ToolBar::ToolBar(QWidget *parent)
    : QToolBar(parent)
{
    setupUi();
}

void ToolBar::setupUi()
{
    setMovable(false);
    setFloatable(false);

    setStyleSheet(QString(
        "QToolBar { background: %1; border: none; border-bottom: 1px solid %2; padding: 4px 8px; spacing: 4px; }"
        "QLabel { color: %3; font-size: 11px; }"
    ).arg(Theme::Colors::Surface, Theme::Colors::GlassBorder, Theme::Colors::TextMuted));

    QString btn = QString(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 3px; padding: 4px 8px; "
        "color: %3; font-size: 11px; }"
        "QPushButton:hover { background: %4; }"
        "QPushButton:disabled { color: %5; }"
    ).arg(Theme::Colors::SurfaceLight, Theme::Colors::GlassBorder,
          Theme::Colors::TextPrimary, Theme::Colors::SurfaceHover, Theme::Colors::TextMuted);

    QString accentBtn = QString(
        "QPushButton { background: %1; border: none; border-radius: 3px; padding: 4px 10px; "
        "color: %2; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background: #33ddff; }"
        "QPushButton:disabled { background: %3; color: %4; }"
    ).arg(Theme::Colors::AccentPrimary, Theme::Colors::Background,
          Theme::Colors::SurfaceLight, Theme::Colors::TextMuted);

    QString stopBtnStyle = QString(
        "QPushButton { background: %1; border: none; border-radius: 3px; padding: 4px 10px; "
        "color: white; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background: #ff5566; }"
        "QPushButton:disabled { background: %2; color: %3; }"
    ).arg(Theme::Colors::Error, Theme::Colors::SurfaceLight, Theme::Colors::TextMuted);

    QString inputStyle = QString(
        "QLineEdit, QSpinBox { background: %1; border: 1px solid %2; border-radius: 3px; "
        "padding: 3px 6px; color: %3; font-size: 11px; }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 12px; }"
    ).arg(Theme::Colors::SurfaceLight, Theme::Colors::GlassBorder, Theme::Colors::TextPrimary);

    Config &cfg = Config::instance();

    // Grid config
    addWidget(new QLabel("Grid"));
    m_colsSpin = new QSpinBox();
    m_colsSpin->setRange(1, 10);
    m_colsSpin->setValue(cfg.defaultCols());
    m_colsSpin->setFixedWidth(42);
    m_colsSpin->setStyleSheet(inputStyle);
    addWidget(m_colsSpin);

    addWidget(new QLabel("x"));

    m_rowsSpin = new QSpinBox();
    m_rowsSpin->setRange(1, 10);
    m_rowsSpin->setValue(cfg.defaultRows());
    m_rowsSpin->setFixedWidth(42);
    m_rowsSpin->setStyleSheet(inputStyle);
    addWidget(m_rowsSpin);

    addSeparator();

    // Source
    addWidget(new QLabel("Src"));
    m_sourceEdit = new QLineEdit();
    m_sourceEdit->setFixedWidth(200);
    m_sourceEdit->setStyleSheet(inputStyle);
    m_sourceEdit->setText(cfg.defaultMediaPath());
    addWidget(m_sourceEdit);

    auto *browseBtn = new QPushButton("..");
    browseBtn->setFixedWidth(24);
    browseBtn->setStyleSheet(btn);
    connect(browseBtn, &QPushButton::clicked, this, &ToolBar::browseClicked);
    addWidget(browseBtn);

    addSeparator();

    // Filter
    addWidget(new QLabel("Filter"));
    m_filterEdit = new QLineEdit();
    m_filterEdit->setFixedWidth(100);
    m_filterEdit->setPlaceholderText("AND");
    m_filterEdit->setStyleSheet(inputStyle);
    addWidget(m_filterEdit);

    addSeparator();

    // Start/Stop
    m_startBtn = new QPushButton("Start");
    m_startBtn->setStyleSheet(accentBtn);
    connect(m_startBtn, &QPushButton::clicked, this, &ToolBar::startClicked);
    addWidget(m_startBtn);

    m_stopBtn = new QPushButton("Stop");
    m_stopBtn->setStyleSheet(stopBtnStyle);
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked, this, &ToolBar::stopClicked);
    addWidget(m_stopBtn);

    addSeparator();

    // Playback
    auto *prevBtn = new QPushButton("|<");
    prevBtn->setStyleSheet(btn);
    prevBtn->setFixedWidth(28);
    connect(prevBtn, &QPushButton::clicked, this, &ToolBar::prevClicked);
    addWidget(prevBtn);

    auto *playBtn = new QPushButton("||");
    playBtn->setStyleSheet(btn);
    playBtn->setFixedWidth(28);
    connect(playBtn, &QPushButton::clicked, this, &ToolBar::playPauseClicked);
    addWidget(playBtn);

    auto *nextBtn = new QPushButton(">|");
    nextBtn->setStyleSheet(btn);
    nextBtn->setFixedWidth(28);
    connect(nextBtn, &QPushButton::clicked, this, &ToolBar::nextClicked);
    addWidget(nextBtn);

    auto *shuffleBtn = new QPushButton("Shuf");
    shuffleBtn->setStyleSheet(btn);
    connect(shuffleBtn, &QPushButton::clicked, this, &ToolBar::shuffleClicked);
    addWidget(shuffleBtn);

    auto *fsBtn = new QPushButton("FS");
    fsBtn->setStyleSheet(btn);
    fsBtn->setToolTip("Fullscreen [Tab]");
    connect(fsBtn, &QPushButton::clicked, this, &ToolBar::fullscreenClicked);
    addWidget(fsBtn);

    addSeparator();

    // Volume - smaller
    m_muteBtn = new QPushButton("V");
    m_muteBtn->setStyleSheet(btn);
    m_muteBtn->setFixedWidth(24);
    m_muteBtn->setToolTip("Mute [`]");
    connect(m_muteBtn, &QPushButton::clicked, this, &ToolBar::muteClicked);
    addWidget(m_muteBtn);

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setFixedWidth(60);
    m_volumeSlider->setStyleSheet(Theme::sliderStyle());
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_volumeLabel->setText(QString::number(val));
        emit volumeChanged(val);
    });
    addWidget(m_volumeSlider);

    m_volumeLabel = new QLabel("30");
    m_volumeLabel->setFixedWidth(20);
    addWidget(m_volumeLabel);

    addSeparator();

    // Panel toggle
    auto *panelBtn = new QPushButton("P");
    panelBtn->setStyleSheet(btn);
    panelBtn->setFixedWidth(24);
    panelBtn->setToolTip("Toggle panel");
    connect(panelBtn, &QPushButton::clicked, this, &ToolBar::toggleSidePanel);
    addWidget(panelBtn);

    // Settings button
    auto *settingsBtn = new QPushButton("S");
    settingsBtn->setStyleSheet(btn);
    settingsBtn->setFixedWidth(24);
    settingsBtn->setToolTip("Settings");
    connect(settingsBtn, &QPushButton::clicked, this, &ToolBar::settingsClicked);
    addWidget(settingsBtn);

    // Spacer
    auto *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    addWidget(spacer);

    // Hints
    auto *hints = new QLabel("Y:Pick R:Loop G:Pause");
    addWidget(hints);

    m_volumeSlider->setValue(cfg.defaultVolume());
}

void ToolBar::setRunning(bool running)
{
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    m_colsSpin->setEnabled(!running);
    m_rowsSpin->setEnabled(!running);
    m_sourceEdit->setEnabled(!running);
    m_filterEdit->setEnabled(!running);
}

void ToolBar::setVolume(int volume)
{
    m_volumeSlider->setValue(volume);
}

void ToolBar::setMuteActive(bool active)
{
    if (active) {
        m_muteBtn->setText("M");
        m_muteBtn->setStyleSheet(QString(
            "QPushButton { background: %1; border: none; border-radius: 3px; padding: 4px 8px; "
            "color: white; font-weight: 600; font-size: 11px; }"
        ).arg(Theme::Colors::Error));
    } else {
        m_muteBtn->setText("V");
        m_muteBtn->setStyleSheet(QString(
            "QPushButton { background: %1; border: 1px solid %2; border-radius: 3px; padding: 4px 8px; "
            "color: %3; font-size: 11px; }"
            "QPushButton:hover { background: %4; }"
        ).arg(Theme::Colors::SurfaceLight, Theme::Colors::GlassBorder,
              Theme::Colors::TextPrimary, Theme::Colors::SurfaceHover));
    }
}

int ToolBar::rows() const { return m_rowsSpin->value(); }
int ToolBar::cols() const { return m_colsSpin->value(); }
QString ToolBar::sourceDir() const { return m_sourceEdit->text(); }
QString ToolBar::filter() const { return m_filterEdit->text().trimmed(); }
void ToolBar::setSourceDir(const QString &dir) { m_sourceEdit->setText(dir); }
