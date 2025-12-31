#pragma once

#include <QToolBar>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>

class ToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit ToolBar(QWidget *parent = nullptr);

    void setRunning(bool running);
    void setVolume(int volume);
    void setMuteActive(bool active);

    // Config getters
    [[nodiscard]] int rows() const;
    [[nodiscard]] int cols() const;
    [[nodiscard]] QString sourceDir() const;
    [[nodiscard]] QString filter() const;
    void setSourceDir(const QString &dir);

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
    void toggleSidePanel();
    void browseClicked();

private:
    void setupUi();

    QPushButton *m_startBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_muteBtn = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_volumeLabel = nullptr;

    // Config widgets
    QSpinBox *m_colsSpin = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    QLineEdit *m_sourceEdit = nullptr;
    QLineEdit *m_filterEdit = nullptr;
};
