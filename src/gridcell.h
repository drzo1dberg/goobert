#pragma once

#include <QFrame>
#include "mpvwidget.h"

// Constants
namespace GridCellConstants {
    inline constexpr double kPositionEmitInterval = 0.25;  // ~4Hz throttle
}

class GridCell : public QFrame
{
    Q_OBJECT

public:
    explicit GridCell(int row, int col, QWidget *parent = nullptr);
    ~GridCell() = default;

    [[nodiscard]] int row() const noexcept { return m_row; }
    [[nodiscard]] int col() const noexcept { return m_col; }

    void setPlaylist(const QStringList &files);
    void setSelected(bool selected);
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
    void setLoopFile(bool loop);
    void toggleLoopFile();
    [[nodiscard]] bool isLoopFile() const noexcept;

    // Grid-sync aware next/prev (skips if looping)
    void nextIfNotLooping();
    void prevIfNotLooping();

    // Frame stepping
    void frameStep();
    void frameBackStep();

    // Video transforms
    void rotateVideo();
    void zoomIn();
    void zoomOut();

    // Seek
    void seekRelative(double seconds);

    // Screenshot
    void screenshot();

    // Playlist management
    void updatePlaylistPath(const QString &oldPath, const QString &newPath);

    [[nodiscard]] QString currentFile() const;
    [[nodiscard]] double position() const noexcept;
    [[nodiscard]] double duration() const noexcept;
    [[nodiscard]] bool isPaused() const noexcept;

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
    MpvWidget *m_mpv = nullptr;
    QString m_currentFile;
    double m_position = 0.0;
    double m_duration = 0.0;
    bool m_paused = false;
    bool m_looping = false;
    double m_lastEmitPos = -1.0;
};
