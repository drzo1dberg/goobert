#pragma once

#include <QFrame>
#include "mpvwidget.h"

class GridCell : public QFrame
{
    Q_OBJECT

public:
    explicit GridCell(int row, int col, QWidget *parent = nullptr);
    ~GridCell();

    int row() const { return m_row; }
    int col() const { return m_col; }

    void setPlaylist(const QStringList &files);
    void play();
    void stop();
    void pause();
    void togglePause();
    void next();
    void prev();
    void shuffle();
    void setVolume(int volume);
    void toggleMute();
    void mute();
    void unmute();

    // Loop control
    void toggleLoopFile();
    bool isLoopFile() const;

    // Grid-sync aware next (skips if looping)
    void nextIfNotLooping();

    // Frame stepping
    void frameStep();
    void frameBackStep();

    // Video transforms
    void rotateVideo();
    void zoomIn();
    void zoomOut();

    // Seek
    void seekRelative(double seconds);

    QString currentFile() const;
    double position() const;
    double duration() const;
    bool isPaused() const;

signals:
    void selected(int row, int col);
    void doubleClicked(int row, int col);
    void fileChanged(int row, int col, const QString &path, double pos, double dur, bool paused);
    void loopChanged(int row, int col, bool looping);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void onFileChanged(const QString &path);
    void onPositionChanged(double pos);

private:
    int m_row;
    int m_col;
    MpvWidget *m_mpv;
    QString m_currentFile;
    double m_position = 0;
    double m_duration = 0;
    bool m_paused = false;
    bool m_looping = false;
    double m_lastEmitPos = -1.0;  // Per-instance throttle state
};
