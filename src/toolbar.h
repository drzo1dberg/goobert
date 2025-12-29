#pragma once

#include <QToolBar>
#include <QSlider>
#include <QLabel>

class ToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit ToolBar(QWidget *parent = nullptr);

    void setRunning(bool running);
    void setVolume(int volume);

signals:
    void startClicked();
    void stopClicked();
    void fullscreenClicked();
    void playPauseClicked();
    void nextClicked();
    void prevClicked();
    void shuffleClicked();
    void muteClicked();
    void volumeChanged(int volume);

private:
    void setupUi();

    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_volumeLabel = nullptr;
};
