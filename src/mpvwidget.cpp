#include "mpvwidget.h"
#include "config.h"
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QMetaObject>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <stdexcept>
#include <clocale>

static void wakeup(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvWidget*>(ctx), "onMpvEvents", Qt::QueuedConnection);
}

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    using namespace MpvConstants;
    setMinimumSize(kMinWidgetSize, kMinWidgetSize);

    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

MpvWidget::~MpvWidget()
{
    makeCurrent();
    destroyMpv();
    doneCurrent();
}

void MpvWidget::createMpv()
{
    // MPV requires C locale for numeric parsing
    std::setlocale(LC_NUMERIC, "C");

    m_mpv = mpv_create();
    if (!m_mpv)
        throw std::runtime_error("Failed to create mpv context");

    // Set options before initialization
    mpv_set_option_string(m_mpv, "terminal", "no");
    mpv_set_option_string(m_mpv, "msg-level", "all=no");
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "keep-open", "no");

    // High-quality playback settings
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");      // GPU decoding when available
    mpv_set_option_string(m_mpv, "profile", "gpu-hq");       // High-quality GPU rendering
    mpv_set_option_string(m_mpv, "scale", "ewa_lanczos");    // Best upscaling algorithm
    mpv_set_option_string(m_mpv, "cscale", "ewa_lanczos");   // Chroma upscaling
    mpv_set_option_string(m_mpv, "dscale", "mitchell");      // Downscaling
    mpv_set_option_string(m_mpv, "video-sync", "display-resample");  // Smooth playback
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "osc", "yes");  // Load OSC script
    mpv_set_option_string(m_mpv, "osd-bar", "yes");
    mpv_set_option_string(m_mpv, "script-opts", "osc-visibility=never");  // Hidden by default
    mpv_set_option_string(m_mpv, "loop-playlist", "inf");

    // Load settings from config
    Config &cfg = Config::instance();
    m_originalLoopCount = cfg.loopCount();  // Store for restoring after inf toggle
    mpv_set_option_string(m_mpv, "loop-file", QString::number(m_originalLoopCount).toUtf8().constData());
    mpv_set_option_string(m_mpv, "image-display-duration", QString::number(cfg.imageDisplayDuration()).toUtf8().constData());
    mpv_set_option_string(m_mpv, "volume", QString::number(cfg.defaultVolume()).toUtf8().constData());
    mpv_set_option_string(m_mpv, "screenshot-directory", cfg.screenshotPath().toUtf8().constData());
    mpv_set_option_string(m_mpv, "screenshot-template", "%f-%P");
    mpv_set_option_string(m_mpv, "screenshot-format", "png");

    if (mpv_initialize(m_mpv) < 0)
        throw std::runtime_error("Failed to initialize mpv");

    // Request property updates
    mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "path", MPV_FORMAT_STRING);

    mpv_set_wakeup_callback(m_mpv, wakeup, this);
}

void MpvWidget::destroyMpv()
{
    if (m_mpvGl) {
        mpv_render_context_free(m_mpvGl);
        m_mpvGl = nullptr;
    }
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
    }
}

void MpvWidget::initializeGL()
{
    qDebug() << "MpvWidget::initializeGL()";
    createMpv();

    mpv_opengl_init_params gl_init_params{
        .get_proc_address = getGlProcAddress,
    };

    mpv_render_param params[]{
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    int err = mpv_render_context_create(&m_mpvGl, m_mpv, params);
    if (err < 0) {
        qDebug() << "mpv_render_context_create failed:" << mpv_error_string(err);
        throw std::runtime_error("Failed to create mpv render context");
    }

    mpv_render_context_set_update_callback(m_mpvGl, onUpdate, this);
    m_initialized = true;
    qDebug() << "MpvWidget initialized successfully";

    // Process any pending commands
    processPendingCommands();
}

void MpvWidget::processPendingCommands()
{
    if (!m_pendingPlaylist.isEmpty()) {
        qDebug() << "Processing pending playlist with" << m_pendingPlaylist.size() << "files";
        QStringList playlist = m_pendingPlaylist;
        m_pendingPlaylist.clear();
        loadPlaylist(playlist);
        play(); // Start playback
    }
    m_pendingCommands.clear();
}

void MpvWidget::paintGL()
{
    if (!m_mpvGl)
        return;

    auto *f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0, 0, width(), height());
    f->glClearColor(0.f, 0.f, 0.f, 1.f);
    f->glClear(GL_COLOR_BUFFER_BIT);

    mpv_opengl_fbo fbo{
        .fbo = static_cast<int>(defaultFramebufferObject()),
        .w = width(),
        .h = height(),
    };

    int flip_y = 1;
    mpv_render_param params[]{
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(m_mpvGl, params);
}

void MpvWidget::onUpdate(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvWidget*>(ctx), "maybeUpdate", Qt::QueuedConnection);
}

void MpvWidget::maybeUpdate()
{
    if (m_mpvGl) {
        if (mpv_render_context_update(m_mpvGl) & MPV_RENDER_UPDATE_FRAME)
            update();
    }
}

void *MpvWidget::getGlProcAddress(void *ctx, const char *name)
{
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx)
        return nullptr;
    return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
}

void MpvWidget::onMpvEvents()
{
    while (m_mpv) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;
        handleMpvEvent(event);
    }
}

void MpvWidget::handleMpvEvent(mpv_event *event)
{
    switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
        mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);
        if (strcmp(prop->name, "time-pos") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
            emit positionChanged(*static_cast<double*>(prop->data));
        } else if (strcmp(prop->name, "duration") == 0 && prop->format == MPV_FORMAT_DOUBLE) {
            emit durationChanged(*static_cast<double*>(prop->data));
        } else if (strcmp(prop->name, "pause") == 0 && prop->format == MPV_FORMAT_FLAG) {
            emit pauseChanged(*static_cast<int*>(prop->data));
        } else if (strcmp(prop->name, "path") == 0 && prop->format == MPV_FORMAT_STRING) {
            emit fileChanged(QString::fromUtf8(*static_cast<char**>(prop->data)));
        }
        break;
    }
    case MPV_EVENT_LOG_MESSAGE: {
        mpv_event_log_message *msg = static_cast<mpv_event_log_message*>(event->data);
        qDebug() << "[mpv]" << msg->prefix << msg->level << msg->text;
        break;
    }
    case MPV_EVENT_FILE_LOADED: {
        qDebug() << "MPV: File loaded";
        QString path = getProperty("path").toString();
        emit fileLoaded(path);

        // Skipper: seek to percentage if enabled and file not seen before
        // Use delay (like original Lua script) to let decoder stabilize
        if (m_skipperEnabled && !path.isEmpty() && !m_seenFiles.contains(path)) {
            m_seenFiles.insert(path);
            QTimer::singleShot(MpvConstants::kSkipperDelayMs, this, [this]() {
                if (double dur = getProperty("duration").toDouble(); dur > 0) {
                    double target = dur * m_skipPercent;
                    command(QVariantList{"seek", QString::number(target), "absolute", "keyframes"});
                    command(QVariantList{"show-text", QString("start@%1%").arg(int(m_skipPercent * 100)), QString::number(MpvConstants::kOsdDurationMs)});
                }
            });
        }
        break;
    }
    case MPV_EVENT_END_FILE:
        qDebug() << "MPV: End file";
        break;
    default:
        break;
    }
}

void MpvWidget::command(const QVariant &args)
{
    if (!m_mpv) {
        qDebug() << "MpvWidget::command: mpv not initialized!";
        return;
    }

    QVariantList list = args.toList();
    QVector<QByteArray> byteArrays;
    byteArrays.reserve(list.size());

    // First pass: convert all args to QByteArray
    for (const QVariant &arg : list) {
        byteArrays.append(arg.toString().toUtf8());
    }

    // Second pass: build pointer array (byteArrays won't reallocate now)
    QVector<const char*> cargs;
    cargs.reserve(byteArrays.size() + 1);
    for (const QByteArray &ba : byteArrays) {
        cargs.append(ba.constData());
    }
    cargs.append(nullptr);

    int err = mpv_command(m_mpv, cargs.data());
    if (err < 0) {
        qDebug() << "mpv_command error:" << mpv_error_string(err) << "for:" << list;
    }
}

void MpvWidget::setProperty(const QString &name, const QVariant &value)
{
    if (!m_mpv) return;

    QByteArray nameUtf8 = name.toUtf8();

    switch (value.typeId()) {
    case QMetaType::Bool: {
        int v = value.toBool();
        mpv_set_property(m_mpv, nameUtf8.constData(), MPV_FORMAT_FLAG, &v);
        break;
    }
    case QMetaType::Int:
    case QMetaType::LongLong: {
        int64_t v = value.toLongLong();
        mpv_set_property(m_mpv, nameUtf8.constData(), MPV_FORMAT_INT64, &v);
        break;
    }
    case QMetaType::Double: {
        double v = value.toDouble();
        mpv_set_property(m_mpv, nameUtf8.constData(), MPV_FORMAT_DOUBLE, &v);
        break;
    }
    default: {
        QByteArray valueUtf8 = value.toString().toUtf8();
        mpv_set_property_string(m_mpv, nameUtf8.constData(), valueUtf8.constData());
        break;
    }
    }
}

QVariant MpvWidget::getProperty(const QString &name) const
{
    if (!m_mpv) return QVariant();

    QByteArray nameUtf8 = name.toUtf8();
    char *result = mpv_get_property_string(m_mpv, nameUtf8.constData());
    if (result) {
        QVariant ret = QString::fromUtf8(result);
        mpv_free(result);
        return ret;
    }
    return QVariant();
}

void MpvWidget::loadFile(const QString &file)
{
    qDebug() << "MpvWidget::loadFile:" << file;
    command(QVariantList{"loadfile", file});
}

void MpvWidget::loadPlaylist(const QStringList &files)
{
    if (files.isEmpty()) return;

    if (!m_initialized) {
        qDebug() << "Queueing playlist with" << files.size() << "files";
        m_pendingPlaylist = files;
        return;
    }

    qDebug() << "Loading playlist with" << files.size() << "files";
    m_currentPlaylist = files;  // Store for rename support
    loadFile(files.first());
    for (int i = 1; i < files.size(); ++i) {
        command(QVariantList{"loadfile", files[i], "append"});
    }
}

void MpvWidget::updatePlaylistPath(const QString &oldPath, const QString &newPath)
{
    // Update in current playlist
    for (int i = 0; i < m_currentPlaylist.size(); ++i) {
        if (m_currentPlaylist[i] == oldPath) {
            m_currentPlaylist[i] = newPath;
            qDebug() << "Updated playlist entry:" << oldPath << "->" << newPath;
        }
    }

    // Update pending playlist if not yet initialized
    for (int i = 0; i < m_pendingPlaylist.size(); ++i) {
        if (m_pendingPlaylist[i] == oldPath) {
            m_pendingPlaylist[i] = newPath;
        }
    }

    // Note: mpv's internal playlist already has file handles open,
    // so we don't need to update it. The rename will work for future
    // navigation since we store our own playlist copy.
}

void MpvWidget::play()
{
    if (!m_initialized) {
        m_pendingCommands.append({"set_property", "pause", "no"});
        return;
    }
    setProperty("pause", false);
}

void MpvWidget::pause()
{
    setProperty("pause", true);
}

void MpvWidget::stop()
{
    command(QVariantList{"stop"});
}

void MpvWidget::togglePause()
{
    command(QVariantList{"cycle", "pause"});
}

void MpvWidget::next()
{
    command(QVariantList{"playlist-next", "force"});
}

void MpvWidget::prev()
{
    command(QVariantList{"playlist-prev"});
}

void MpvWidget::shuffle()
{
    command(QVariantList{"playlist-shuffle"});
}

void MpvWidget::playIndex(int index)
{
    command(QVariantList{"playlist-play-index", index});
}

void MpvWidget::seek(double seconds)
{
    command(QVariantList{"seek", seconds, "relative"});
}

void MpvWidget::setVolume(int volume)
{
    setProperty("volume", volume);
}

void MpvWidget::toggleMute()
{
    command(QVariantList{"cycle", "mute"});
}

void MpvWidget::mute()
{
    setProperty("mute", true);
}

void MpvWidget::unmute()
{
    setProperty("mute", false);
}

QString MpvWidget::currentFile() const
{
    return getProperty("path").toString();
}

QStringList MpvWidget::currentPlaylist() const
{
    QStringList result;
    if (!m_mpv) return result;

    // Get playlist count from mpv
    int64_t count = 0;
    if (mpv_get_property(m_mpv, "playlist-count", MPV_FORMAT_INT64, &count) < 0) {
        return m_currentPlaylist;  // Fallback to stored playlist
    }

    // Get each playlist entry's filename
    for (int64_t i = 0; i < count; ++i) {
        QString propName = QString("playlist/%1/filename").arg(i);
        QByteArray propNameUtf8 = propName.toUtf8();
        char *filename = nullptr;
        if (mpv_get_property(m_mpv, propNameUtf8.constData(), MPV_FORMAT_STRING, &filename) >= 0 && filename) {
            result.append(QString::fromUtf8(filename));
            mpv_free(filename);
        }
    }

    return result.isEmpty() ? m_currentPlaylist : result;
}

double MpvWidget::position() const
{
    return getProperty("time-pos").toDouble();
}

double MpvWidget::duration() const
{
    return getProperty("duration").toDouble();
}

bool MpvWidget::isPaused() const
{
    return getProperty("pause").toBool();
}

bool MpvWidget::isMuted() const
{
    return getProperty("mute").toBool();
}

// Skipper methods
void MpvWidget::setSkipPercent(double percent)
{
    m_skipPercent = qBound(0.0, percent, 1.0);
}

void MpvWidget::setSkipperEnabled(bool enabled)
{
    m_skipperEnabled = enabled;
}

// Loop control
void MpvWidget::setLoopFile(bool loop)
{
    // When toggling off, restore original preset instead of "no"
    QString value = loop ? "inf" : QString::number(m_originalLoopCount);
    setProperty("loop-file", value);
    emit loopChanged(loop);
}

bool MpvWidget::isLoopFile() const
{
    return getProperty("loop-file").toString() == "inf";
}

void MpvWidget::toggleLoopFile()
{
    bool current = isLoopFile();
    setLoopFile(!current);
    QString msg = current ? QString("loop-file=%1").arg(m_originalLoopCount) : "loop-file=inf";
    command(QVariantList{"show-text", msg, QString::number(MpvConstants::kOsdDurationMs)});
}

// Frame stepping
void MpvWidget::frameStep()
{
    command(QVariantList{"frame-step"});
}

void MpvWidget::frameBackStep()
{
    command(QVariantList{"frame-back-step"});
}

// Video transforms
void MpvWidget::rotateVideo()
{
    using namespace MpvConstants;
    m_rotation = (m_rotation + kRotationStep) % 360;
    setProperty("video-rotate", m_rotation);
    command(QVariantList{"show-text", QString("rotate: %1°").arg(m_rotation), QString::number(kOsdDurationMs)});
}

void MpvWidget::zoomIn()
{
    double zoom = getProperty("video-zoom").toDouble();
    setProperty("video-zoom", zoom + MpvConstants::kZoomStep);
}

void MpvWidget::zoomOut()
{
    double zoom = getProperty("video-zoom").toDouble();
    setProperty("video-zoom", zoom - MpvConstants::kZoomStep);
}

void MpvWidget::zoomAt(double delta, double normalizedX, double normalizedY)
{
    // Zoom towards mouse position
    // normalizedX/Y: 0.0 = left/top, 1.0 = right/bottom, 0.5 = center
    double currentZoom = getProperty("video-zoom").toDouble();
    double currentPanX = getProperty("video-pan-x").toDouble();
    double currentPanY = getProperty("video-pan-y").toDouble();

    // Convert normalized coords to centered coords (-0.5 to 0.5)
    double cx = normalizedX - 0.5;
    double cy = normalizedY - 0.5;

    // Calculate zoom factor (video-zoom is log2 scale)
    double newZoom = currentZoom + delta;

    // Limit zoom range
    if (newZoom < -1.0 || newZoom > 3.0) return;

    double oldScale = std::pow(2.0, currentZoom);
    double newScale = std::pow(2.0, newZoom);

    // Pan adjustment: to keep the point under cursor fixed,
    // we need to compensate for the scale change.
    // Pan moves opposite to cursor offset to bring that region into view.
    double scaleRatio = newScale / oldScale;
    double newPanX = currentPanX * scaleRatio - cx * (scaleRatio - 1.0);
    double newPanY = currentPanY * scaleRatio - cy * (scaleRatio - 1.0);

    setProperty("video-zoom", newZoom);
    setProperty("video-pan-x", newPanX);
    setProperty("video-pan-y", newPanY);
}

void MpvWidget::resetZoom()
{
    setProperty("video-zoom", 0.0);
    setProperty("video-pan-x", 0.0);
    setProperty("video-pan-y", 0.0);
}

// OSC/OSD control for fullscreen mode
void MpvWidget::setOscEnabled(bool enabled)
{
    if (!m_mpv) return;

    m_oscEnabled = enabled;

    // Change OSC visibility via script-message
    QString visibility = enabled ? "auto" : "never";
    command(QVariantList{"script-message", "osc-visibility", visibility});

    // Enable input bindings for OSC interaction in fullscreen
    setProperty("input-default-bindings", enabled ? "yes" : "no");
    setProperty("input-vo-keyboard", enabled ? "yes" : "no");

    // Enable mouse tracking for OSC
    setMouseTracking(enabled);
}

void MpvWidget::setOsdLevel(int level)
{
    if (!m_mpv) return;
    setProperty("osd-level", level);
}


// Screenshot
void MpvWidget::screenshot()
{
    // Get screenshot directory from config
    Config &cfg = Config::instance();
    QString screenshotDir = cfg.screenshotPath();

    // Use MPV's screenshot command (it will auto-number)
    command(QVariantList{"screenshot"});

    // Wait a bit for the file to be written, then find the newest screenshot
    QTimer::singleShot(MpvConstants::kScreenshotDelayMs, this, [screenshotDir]() {
        QDir dir(screenshotDir);
        if (!dir.exists()) return;

        // Get all PNG files sorted by modification time
        QFileInfoList files = dir.entryInfoList(QStringList() << "*.png" << "*.jpg",
                                                QDir::Files,
                                                QDir::Time);
        if (!files.isEmpty()) {
            QString newestFile = files.first().absoluteFilePath();

            // Copy to clipboard
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(newestFile);

            // Show message with full path
            // We can't use command() here since we're in a lambda, so we'll just copy to clipboard
            qDebug() << "Screenshot saved and copied to clipboard:" << newestFile;
        }
    });
}

// Mouse event forwarding for OSC interaction
void MpvWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_oscEnabled && m_mpv) {
        // Forward mouse position to mpv for OSC hover
        command(QVariantList{"mouse", event->pos().x(), event->pos().y()});
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void MpvWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_oscEnabled && m_mpv) {
        // Right-click toggles pause (like standalone mpv)
        if (event->button() == Qt::RightButton) {
            togglePause();
            event->accept();
            return;
        }

        // Forward other mouse buttons to OSC
        int btn = 0;
        if (event->button() == Qt::LeftButton) btn = 0;
        else if (event->button() == Qt::MiddleButton) btn = 1;

        command(QVariantList{"mouse", event->pos().x(), event->pos().y(), btn, "single"});
    }
    QOpenGLWidget::mousePressEvent(event);
}

void MpvWidget::mouseReleaseEvent(QMouseEvent *event)
{
    // mpv OSC needs the release event too
    if (m_oscEnabled && m_mpv) {
        // Send mouse position update (release is implicit)
        command(QVariantList{"mouse", event->pos().x(), event->pos().y()});
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void MpvWidget::leaveEvent(QEvent *event)
{
    if (m_oscEnabled && m_mpv) {
        // Signal mouse left the widget (OSC should hide)
        command(QVariantList{"mouse", -1, -1});
    }
    QOpenGLWidget::leaveEvent(event);
}
