#pragma once

#include <QOpenGLWidget>
#include <QSet>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <functional>

class MpvWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit MpvWidget(QWidget *parent = nullptr);
    ~MpvWidget();

    void command(const QVariant &args);
    void setProperty(const QString &name, const QVariant &value);
    QVariant getProperty(const QString &name) const;

    void loadFile(const QString &file);
    void loadPlaylist(const QStringList &files);
    void play();
    void pause();
    void stop();
    void togglePause();
    void next();
    void prev();
    void shuffle();
    void seek(double seconds);
    void setVolume(int volume);
    void toggleMute();
    void mute();
    void unmute();

    QString currentFile() const;
    double position() const;
    double duration() const;
    bool isPaused() const;
    bool isMuted() const;

    // Skipper: seek to percentage on file load
    void setSkipPercent(double percent);
    double skipPercent() const { return m_skipPercent; }
    void setSkipperEnabled(bool enabled);
    bool isSkipperEnabled() const { return m_skipperEnabled; }

    // Loop control
    void setLoopFile(bool loop);
    bool isLoopFile() const;
    void toggleLoopFile();

    // Frame stepping
    void frameStep();
    void frameBackStep();

    // Video transforms
    void rotateVideo();
    void zoomIn();
    void zoomOut();

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

    // Skipper state
    double m_skipPercent = 0.33;
    bool m_skipperEnabled = true;
	QSet<QString> m_seenFiles;

    // Video transform state
    int m_rotation = 0;
};
