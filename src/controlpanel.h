#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTableWidget>

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(const QString &sourceDir, QWidget *parent = nullptr);

    QString sourceDir() const;
    int rows() const;
    int cols() const;

    void setRunning(bool running);
    void setSelectedPath(const QString &path);
    void log(const QString &message);
    void updateCellStatus(int row, int col, const QString &path, double pos, double dur, bool paused);

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
    void gridSizeChanged(int rows, int cols);
    void fileRenamed(const QString &oldPath, const QString &newPath);

private slots:
    void onTableContextMenu(const QPoint &pos);

private:
    void setupUi();
    QString formatTime(double seconds) const;
    void renameFile(int row, int col, const QString &currentPath);

    QLineEdit *m_sourceEdit;
    QSpinBox *m_colsSpin;
    QSpinBox *m_rowsSpin;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_fullscreenBtn;
    QSlider *m_volumeSlider;
    QTableWidget *m_monitor;
    QLabel *m_statusLabel;
    QLabel *m_pathLabel;
};
