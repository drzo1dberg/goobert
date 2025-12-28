#include "mpvwidget.h"
#include "config.h"
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QMetaObject>
#include <QDebug>
#include <QTimer>
#include <stdexcept>
#include <clocale>
#include <locale>

static void wakeup(void *ctx)
{
    QMetaObject::invokeMethod(static_cast<MpvWidget*>(ctx), "onMpvEvents", Qt::QueuedConnection);
}

MpvWidget::MpvWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setMinimumSize(100, 100);

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
    mpv_set_option_string(m_mpv, "hwdec", "no");
    mpv_set_option_string(m_mpv, "keep-open", "no");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    mpv_set_option_string(m_mpv, "osc", "no");
    mpv_set_option_string(m_mpv, "loop-playlist", "inf");

    // Load settings from config
    Config &cfg = Config::instance();
    mpv_set_option_string(m_mpv, "loop-file", QString::number(cfg.loopCount()).toUtf8().constData());
    mpv_set_option_string(m_mpv, "image-display-duration", QString::number(cfg.imageDisplayDuration()).toUtf8().constData());
    mpv_set_option_string(m_mpv, "volume", QString::number(cfg.defaultVolume()).toUtf8().constData());

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
            QTimer::singleShot(150, this, [this]() {
                double dur = getProperty("duration").toDouble();
                if (dur > 0) {
                    double target = dur * m_skipPercent;
                    command(QVariantList{"seek", QString::number(target), "absolute", "keyframes"});
                    command(QVariantList{"show-text", QString("start@%1%").arg(int(m_skipPercent * 100)), "1500"});
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
    setProperty("loop-file", loop ? "inf" : "no");
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
    QString msg = current ? "loop-file=no" : "loop-file=inf";
    command(QVariantList{"show-text", msg, "1500"});
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
    m_rotation = (m_rotation + 90) % 360;
    setProperty("video-rotate", m_rotation);
    command(QVariantList{"show-text", QString("rotate: %1°").arg(m_rotation), "1500"});
}

void MpvWidget::zoomIn()
{
    double zoom = getProperty("video-zoom").toDouble();
    setProperty("video-zoom", zoom + 0.1);
}

void MpvWidget::zoomOut()
{
    double zoom = getProperty("video-zoom").toDouble();
    setProperty("video-zoom", zoom - 0.1);
}
