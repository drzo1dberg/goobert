#include "gridcell.h"
#include <QVBoxLayout>
#include <QMouseEvent>

GridCell::GridCell(int row, int col, QWidget *parent)
    : QFrame(parent)
    , m_row(row)
    , m_col(col)
{
    setFrameStyle(QFrame::Box);
    setStyleSheet("GridCell { background-color: #0a0a0a; border: 1px solid #202020; }");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_mpv = new MpvWidget(this);
    layout->addWidget(m_mpv);

    connect(m_mpv, &MpvWidget::fileChanged, this, &GridCell::onFileChanged);
    connect(m_mpv, &MpvWidget::positionChanged, this, &GridCell::onPositionChanged);
    connect(m_mpv, &MpvWidget::durationChanged, this, [this](double dur) {
        m_duration = dur;
    });
    connect(m_mpv, &MpvWidget::pauseChanged, this, [this](bool paused) {
        m_paused = paused;
    });
    connect(m_mpv, &MpvWidget::loopChanged, this, [this](bool looping) {
        m_looping = looping;
        emit loopChanged(m_row, m_col, looping);
    });
}

GridCell::~GridCell()
{
}

void GridCell::setPlaylist(const QStringList &files)
{
    m_mpv->loadPlaylist(files);
}

void GridCell::play()
{
    m_mpv->play();
}

void GridCell::stop()
{
    m_mpv->stop();
}

void GridCell::pause()
{
    m_mpv->pause();
}

void GridCell::togglePause()
{
    m_mpv->togglePause();
}

void GridCell::next()
{
    m_mpv->next();
}

void GridCell::prev()
{
    m_mpv->prev();
}

void GridCell::shuffle()
{
    m_mpv->shuffle();
}

void GridCell::setVolume(int volume)
{
    m_mpv->setVolume(volume);
}

void GridCell::toggleMute()
{
    m_mpv->toggleMute();
}

void GridCell::mute()
{
    m_mpv->mute();
}

void GridCell::unmute()
{
    m_mpv->unmute();
}

void GridCell::toggleLoopFile()
{
    m_mpv->toggleLoopFile();
}

bool GridCell::isLoopFile() const
{
    return m_looping;
}

void GridCell::nextIfNotLooping()
{
    // Grid-sync: skip next if this cell is looping
    if (m_looping) {
        return;
    }
    m_mpv->next();
}

void GridCell::frameStep()
{
    m_mpv->frameStep();
}

void GridCell::frameBackStep()
{
    m_mpv->frameBackStep();
}

void GridCell::rotateVideo()
{
    m_mpv->rotateVideo();
}

void GridCell::zoomIn()
{
    m_mpv->zoomIn();
}

void GridCell::zoomOut()
{
    m_mpv->zoomOut();
}

void GridCell::seekRelative(double seconds)
{
    m_mpv->seek(seconds);
}

QString GridCell::currentFile() const
{
    return m_currentFile;
}

double GridCell::position() const
{
    return m_position;
}

double GridCell::duration() const
{
    return m_duration;
}

bool GridCell::isPaused() const
{
    return m_paused;
}

void GridCell::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit selected(m_row, m_col);
    } else if (event->button() == Qt::RightButton) {
        // Right click: toggle pause on this cell
        togglePause();
        event->accept();
        return;
    } else if (event->button() == Qt::ForwardButton) {
        // Mouse 4 (forward/thumb button): next on this cell only
        next();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void GridCell::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(m_row, m_col);
    }
    QFrame::mouseDoubleClickEvent(event);
}

void GridCell::onFileChanged(const QString &path)
{
    m_currentFile = path;
    emit fileChanged(m_row, m_col, path, m_position, m_duration, m_paused);
}

void GridCell::onPositionChanged(double pos)
{
    m_position = pos;

    // Throttle auf ~4Hz, sonst zu viele UI-Updates
    static constexpr double kMinIntervalSec = 0.25;

    if (m_lastEmitPos < 0 || (m_position - m_lastEmitPos) >= kMinIntervalSec || (m_lastEmitPos - m_position) >= kMinIntervalSec) {
        m_lastEmitPos = m_position;
        if (!m_currentFile.isEmpty()) {
            emit fileChanged(m_row, m_col, m_currentFile, m_position, m_duration, m_paused);
        }
    }
}
