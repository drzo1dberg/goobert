#include "gridcell.h"
#include "config.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <cmath>

GridCell::GridCell(int row, int col, QWidget *parent)
    : QFrame(parent)
    , m_row(row)
    , m_col(col)
{
    setFrameStyle(QFrame::Box);
    setObjectName("GridCell");
    setStyleSheet(QString("QFrame#GridCell { background-color: %1; border: 1px solid %2; border-radius: %3px; }")
        .arg(Theme::Colors::Background, Theme::Colors::Border, QString::number(Theme::Radius::SM)));
    setMouseTracking(true);  // Enable mouse tracking for hover events

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_mpv = new MpvWidget(this);
    layout->addWidget(m_mpv);

    // Loop indicator overlay (top-right corner)
    m_loopIndicator = new QLabel("LOOP", this);
    m_loopIndicator->setStyleSheet(QString(
        "background: %1; color: %2; padding: 2px 6px; border-radius: 3px; font-size: 10px; font-weight: bold;"
    ).arg(Theme::Colors::AccentPrimary, Theme::Colors::Background));
    m_loopIndicator->setFixedSize(42, 18);
    m_loopIndicator->setAlignment(Qt::AlignCenter);
    m_loopIndicator->hide();

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
        updateLoopIndicator();
        emit loopChanged(m_row, m_col, looping);
    });
}


void GridCell::setPlaylist(const QStringList &files)
{
    m_mpv->loadPlaylist(files);
}

void GridCell::loadFile(const QString &file)
{
    m_mpv->loadFile(file);
}

void GridCell::setSelected(bool selected)
{
    if (selected) {
        setStyleSheet(QString("QFrame#GridCell { background-color: %1; border: 2px solid %2; border-radius: %3px; }")
            .arg(Theme::Colors::Background, Theme::Colors::AccentPrimary, QString::number(Theme::Radius::SM)));
    } else {
        setStyleSheet(QString("QFrame#GridCell { background-color: %1; border: 1px solid %2; border-radius: %3px; }")
            .arg(Theme::Colors::Background, Theme::Colors::Border, QString::number(Theme::Radius::SM)));
    }
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

void GridCell::playIndex(int index)
{
    m_mpv->playIndex(index);
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

void GridCell::setLoopFile(bool loop)
{
    m_mpv->setLoopFile(loop);
}

void GridCell::toggleLoopFile()
{
    m_mpv->toggleLoopFile();
}

bool GridCell::isLoopFile() const noexcept
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

void GridCell::prevIfNotLooping()
{
    // Grid-sync: skip prev if this cell is looping
    if (m_looping) {
        return;
    }
    m_mpv->prev();
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

void GridCell::zoomAt(double delta, double normalizedX, double normalizedY)
{
    m_mpv->zoomAt(delta, normalizedX, normalizedY);
}

void GridCell::resetZoom()
{
    m_mpv->resetZoom();
}

void GridCell::seekRelative(double seconds)
{
    m_mpv->seek(seconds);
}

void GridCell::screenshot()
{
    m_mpv->screenshot();
}

void GridCell::setOscEnabled(bool enabled)
{
    m_mpv->setOscEnabled(enabled);
}

void GridCell::setOsdLevel(int level)
{
    m_mpv->setOsdLevel(level);
}

void GridCell::updatePlaylistPath(const QString &oldPath, const QString &newPath)
{
    m_mpv->updatePlaylistPath(oldPath, newPath);

    // Update current file reference if it matches
    if (m_currentFile == oldPath) {
        m_currentFile = newPath;
    }
}

QString GridCell::currentFile() const
{
    return m_currentFile;
}

QStringList GridCell::currentPlaylist() const
{
    return m_mpv->currentPlaylist();
}

double GridCell::position() const noexcept
{
    return m_position;
}

double GridCell::duration() const noexcept
{
    return m_duration;
}

bool GridCell::isPaused() const noexcept
{
    return m_paused;
}

void GridCell::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit selected(m_row, m_col);
    } else if (event->button() == Qt::RightButton) {
        // Right click: toggle pause on this cell (no select)
        togglePause();
        event->accept();
        return;
    } else if (event->button() == Qt::MiddleButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            // Shift+Middle click: reset zoom
            resetZoom();
        } else {
            // Middle click: toggle loop on this cell (no select)
            toggleLoopFile();
        }
        event->accept();
        return;
    } else if (event->button() == Qt::ForwardButton) {
        // Mouse 4: next (or prev with Shift)
        if (event->modifiers() & Qt::ShiftModifier) {
            prev();
        } else {
            next();
        }
        event->accept();
        return;
    } else if (event->button() == Qt::BackButton) {
        // Mouse 5 (back button): Shift+Mouse5 = mute
        if (event->modifiers() & Qt::ShiftModifier) {
            toggleMute();
            event->accept();
            return;
        }
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

void GridCell::wheelEvent(QWheelEvent *event)
{
    Config &cfg = Config::instance();
    int seekAmount = cfg.seekAmountSeconds();

    int hdelta = event->angleDelta().x();
    int vdelta = event->angleDelta().y();

    // Horizontal scroll (side scroll): seek by configured amount
    if (hdelta != 0) {
        if (hdelta < 0) {
            seekRelative(seekAmount);  // Scroll left -> forward
        } else {
            seekRelative(-seekAmount);  // Scroll right -> backward
        }
        event->accept();
    }
    // Vertical scroll: zoom towards mouse position
    else if (vdelta != 0) {
        // Get mouse position relative to the video widget
        QPointF globalPos = event->globalPosition();
        QPointF localPos = m_mpv->mapFromGlobal(globalPos.toPoint());

        // Normalize to 0.0 - 1.0 range
        double normalizedX = localPos.x() / m_mpv->width();
        double normalizedY = localPos.y() / m_mpv->height();

        // Clamp to valid range
        normalizedX = std::clamp(normalizedX, 0.0, 1.0);
        normalizedY = std::clamp(normalizedY, 0.0, 1.0);

        // Zoom step based on scroll direction
        double zoomDelta = (vdelta > 0) ? 0.15 : -0.15;  // Up = zoom in, down = zoom out
        zoomAt(zoomDelta, normalizedX, normalizedY);
        event->accept();
    }
}

void GridCell::onFileChanged(const QString &path)
{
    m_currentFile = path;
    emit fileChanged(m_row, m_col, path, m_position, m_duration, m_paused);
}

void GridCell::onPositionChanged(double pos)
{
    m_position = pos;

    // Throttle to ~4Hz to avoid excessive UI updates
    using namespace GridCellConstants;
    if (m_lastEmitPos < 0 || std::abs(m_position - m_lastEmitPos) >= kPositionEmitInterval) {
        m_lastEmitPos = m_position;
        if (!m_currentFile.isEmpty()) {
            emit fileChanged(m_row, m_col, m_currentFile, m_position, m_duration, m_paused);
        }
    }
}

void GridCell::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    // Reposition loop indicator on resize
    if (m_looping) {
        updateLoopIndicator();
    }
}

void GridCell::updateLoopIndicator()
{
    if (m_looping) {
        // Position in top-right corner
        m_loopIndicator->move(width() - m_loopIndicator->width() - 8, 8);
        m_loopIndicator->show();
        m_loopIndicator->raise();
    } else {
        m_loopIndicator->hide();
    }
}
