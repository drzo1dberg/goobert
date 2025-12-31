#pragma once

#include <QOpenGLWidget>
#include <QSet>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <functional>

// Constants
namespace MpvConstants {
    inline constexpr int kMinWidgetSize = 100;
    inline constexpr int kSkipperDelayMs = 150;
    inline constexpr int kScreenshotDelayMs = 100;
    inline constexpr int kOsdDurationMs = 1500;
    inline constexpr double kDefaultSkipPercent = 0.33;
    inline constexpr double kZoomStep = 0.1;
    inline constexpr int kRotationStep = 90;
}

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget() override;

    void command(const QVariant &args);
    void setProperty(const QString &name, const QVariant &value);
    [[nodiscard]] QVariant getProperty(const QString &name) const;

    void loadFile(const QString &file);
    void loadPlaylist(const QStringList &files);
    void updatePlaylistPath(const QString &oldPath, const QString &newPath);
    void play();
    void pause();
    void stop();
    void togglePause();
    void next();
    void prev();
    void shuffle();
    void playIndex(int index);
    void seek(double seconds);
    void setVolume(int volume);
    void toggleMute();
    void mute();
    void unmute();

    [[nodiscard]] QString currentFile() const;
    [[nodiscard]] QStringList currentPlaylist() const;
    [[nodiscard]] double position() const;
    [[nodiscard]] double duration() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isMuted() const;

    // Skipper: seek to percentage on file load
    void setSkipPercent(double percent);
    [[nodiscard]] double skipPercent() const noexcept { return m_skipPercent; }
    void setSkipperEnabled(bool enabled);
    [[nodiscard]] bool isSkipperEnabled() const noexcept { return m_skipperEnabled; }

    // Loop control
    void setLoopFile(bool loop);
    [[nodiscard]] bool isLoopFile() const;
    void toggleLoopFile();

    // Frame stepping
    void frameStep();
    void frameBackStep();

    // Video transforms
    void rotateVideo();
    void zoomIn();
    void zoomOut();
    void zoomAt(double delta, double normalizedX, double normalizedY);
    void resetZoom();

    // Screenshot
    void screenshot();

    // OSD/OSC control (for fullscreen mode)
    void setOscEnabled(bool enabled);
    void setOsdLevel(int level);

signals:
    void fileChanged(const QString &path);
    void positionChanged(double pos);
    void durationChanged(double dur);
    void pauseChanged(bool paused);
    void fileLoaded(const QString &path);
    void loopChanged(bool looping);

protected:
    void initializeGL() override;
    void paintGL() override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void onMpvEvents();
    void maybeUpdate();

private:
    void createMpv();
    void destroyMpv();
    void handleMpvEvent(mpv_event *event);
    void processPendingCommands();

    static void onUpdate(void *ctx);
    static void *getGlProcAddress(void *ctx, const char *name);

    mpv_handle *m_mpv = nullptr;
    mpv_render_context *m_mpvGl = nullptr;
    bool m_initialized = false;
    QList<QVariantList> m_pendingCommands;
    QStringList m_pendingPlaylist;
    QStringList m_currentPlaylist;

    // Skipper state
    double m_skipPercent = MpvConstants::kDefaultSkipPercent;
    bool m_skipperEnabled = true;
    QSet<QString> m_seenFiles;

    // Video transform state
    int m_rotation = 0;

    // Original loop count from config (for restoring after inf toggle)
    int m_originalLoopCount = 5;

    // OSC state
    bool m_oscEnabled = false;
};
