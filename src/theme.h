#pragma once

#include <QString>
#include <QColor>
#include <QFont>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

namespace Theme {

// === 2026 Design System ===
// Glassmorphism + Vibrant Accents + Smooth Animations

// Color Palette
namespace Colors {
    // Base colors (deep dark with slight blue tint)
    inline constexpr auto Background = "#0a0a0f";
    inline constexpr auto Surface = "#12121a";
    inline constexpr auto SurfaceLight = "#1a1a24";
    inline constexpr auto SurfaceHover = "#22222e";
    inline constexpr auto SurfaceActive = "#2a2a38";

    // Glass effect colors (semi-transparent)
    inline constexpr auto GlassBg = "rgba(18, 18, 26, 0.85)";
    inline constexpr auto GlassBorder = "rgba(255, 255, 255, 0.08)";
    inline constexpr auto GlassHighlight = "rgba(255, 255, 255, 0.04)";

    // Text colors
    inline constexpr auto TextPrimary = "#e8e8ec";
    inline constexpr auto TextSecondary = "#8888a0";
    inline constexpr auto TextMuted = "#55556a";

    // Accent gradient (Cyan to Magenta)
    inline constexpr auto AccentPrimary = "#00d4ff";
    inline constexpr auto AccentSecondary = "#b400ff";
    inline constexpr auto AccentGlow = "rgba(0, 212, 255, 0.3)";

    // Status colors
    inline constexpr auto Success = "#00ff88";
    inline constexpr auto Warning = "#ffaa00";
    inline constexpr auto Error = "#ff4466";

    // Selection
    inline constexpr auto Selection = "rgba(0, 212, 255, 0.2)";
    inline constexpr auto SelectionBorder = "rgba(0, 212, 255, 0.6)";

    // Borders
    inline constexpr auto Border = "#252530";
    inline constexpr auto BorderLight = "#353545";
}

// Spacing & Sizing
namespace Spacing {
    inline constexpr int XS = 4;
    inline constexpr int SM = 8;
    inline constexpr int MD = 12;
    inline constexpr int LG = 16;
    inline constexpr int XL = 24;
    inline constexpr int XXL = 32;
}

// Border Radius
namespace Radius {
    inline constexpr int SM = 6;
    inline constexpr int MD = 10;
    inline constexpr int LG = 14;
    inline constexpr int XL = 20;
    inline constexpr int Round = 9999;
}

// Animation Durations (ms)
namespace Animation {
    inline constexpr int Fast = 150;
    inline constexpr int Normal = 250;
    inline constexpr int Slow = 400;
}

// Stylesheet Generators
inline QString mainWindowStyle() {
    return QString(R"(
        QMainWindow {
            background-color: %1;
        }
        QToolTip {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 8px 12px;
            font-size: 12px;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: %6;
            border-radius: 4px;
            min-height: 40px;
        }
        QScrollBar::handle:vertical:hover {
            background: %7;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            background: transparent;
            height: 8px;
        }
        QScrollBar::handle:horizontal {
            background: %6;
            border-radius: 4px;
            min-width: 40px;
        }
    )").arg(Colors::Background, Colors::Surface, Colors::TextPrimary,
            Colors::GlassBorder, QString::number(Radius::SM),
            Colors::SurfaceActive, Colors::TextSecondary);
}

inline QString toolBarStyle() {
    return QString(R"(
        QToolBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 %1, stop:1 %2);
            border: none;
            border-bottom: 1px solid %3;
            spacing: %4px;
            padding: %5px %6px;
        }
        QToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: %7px;
            padding: 8px 16px;
            color: %8;
            font-weight: 500;
            font-size: 13px;
        }
        QToolButton:hover {
            background: %9;
            border-color: %10;
        }
        QToolButton:pressed {
            background: %11;
        }
        QToolButton:disabled {
            color: %12;
        }
    )").arg(Colors::Surface, Colors::SurfaceLight, Colors::GlassBorder,
            QString::number(Spacing::SM), QString::number(Spacing::SM), QString::number(Spacing::MD),
            QString::number(Radius::MD), Colors::TextPrimary,
            Colors::SurfaceHover, Colors::GlassBorder, Colors::SurfaceActive, Colors::TextMuted);
}

inline QString sliderStyle() {
    return QString(R"(
        QSlider::groove:horizontal {
            background: %1;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 %2, stop:1 %3);
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background: %2;
            width: 18px;
            height: 18px;
            margin: -7px 0;
            border-radius: 9px;
        }
        QSlider::sub-page:horizontal {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %2, stop:1 %3);
            border-radius: 2px;
        }
    )").arg(Colors::SurfaceActive, Colors::AccentPrimary, Colors::AccentSecondary);
}

inline QString panelStyle() {
    return QString(R"(
        QWidget#Panel {
            background: %1;
            border: 1px solid %2;
            border-radius: %3px;
        }
    )").arg(Colors::GlassBg, Colors::GlassBorder, QString::number(Radius::LG));
}

inline QString tabWidgetStyle() {
    return QString(R"(
        QTabWidget::pane {
            background: %1;
            border: 1px solid %2;
            border-radius: %3px;
            margin-top: -1px;
        }
        QTabBar::tab {
            background: transparent;
            color: %4;
            padding: 10px 20px;
            margin-right: 4px;
            border: none;
            border-bottom: 2px solid transparent;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            color: %5;
            border-bottom: 2px solid %5;
        }
        QTabBar::tab:hover:!selected {
            color: %6;
            background: %7;
            border-radius: %8px %8px 0 0;
        }
    )").arg(Colors::Surface, Colors::GlassBorder, QString::number(Radius::MD),
            Colors::TextSecondary, Colors::AccentPrimary, Colors::TextPrimary,
            Colors::SurfaceHover, QString::number(Radius::SM));
}

inline QString tableStyle() {
    return QString(R"(
        QTableWidget {
            background-color: %1;
            alternate-background-color: %2;
            gridline-color: %3;
            color: %4;
            border: none;
            border-radius: %5px;
            selection-background-color: %6;
        }
        QTableWidget::item {
            padding: 8px;
            border: none;
        }
        QTableWidget::item:selected {
            background: %6;
            color: %7;
        }
        QHeaderView::section {
            background: %8;
            color: %9;
            padding: 10px;
            border: none;
            border-bottom: 1px solid %3;
            font-weight: 600;
        }
    )").arg(Colors::Surface, Colors::SurfaceLight, Colors::GlassBorder,
            Colors::TextPrimary, QString::number(Radius::MD), Colors::Selection,
            Colors::TextPrimary, Colors::SurfaceLight, Colors::TextSecondary);
}

inline QString inputStyle() {
    return QString(R"(
        QLineEdit {
            background: %1;
            border: 1px solid %2;
            border-radius: %3px;
            padding: 10px 14px;
            color: %4;
            font-size: 13px;
            selection-background-color: %5;
        }
        QLineEdit:focus {
            border-color: %6;
            background: %7;
        }
        QLineEdit:hover {
            border-color: %8;
        }
        QSpinBox {
            background: %1;
            border: 1px solid %2;
            border-radius: %3px;
            padding: 4px 8px;
            color: %4;
            font-size: 13px;
            selection-background-color: %5;
        }
        QSpinBox:focus {
            border-color: %6;
            background: %7;
        }
        QSpinBox:hover {
            border-color: %8;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background: transparent;
            border: none;
            width: 16px;
            subcontrol-origin: border;
        }
        QSpinBox::up-button {
            subcontrol-position: top right;
        }
        QSpinBox::down-button {
            subcontrol-position: bottom right;
        }
        QSpinBox::up-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-bottom: 5px solid %9;
            width: 0; height: 0;
        }
        QSpinBox::down-arrow {
            image: none;
            border-left: 4px solid transparent;
            border-right: 4px solid transparent;
            border-top: 5px solid %9;
            width: 0; height: 0;
        }
        QSpinBox::up-arrow:hover, QSpinBox::down-arrow:hover {
            border-bottom-color: %6;
            border-top-color: %6;
        }
    )").arg(Colors::SurfaceLight, Colors::GlassBorder, QString::number(Radius::SM),
            Colors::TextPrimary, Colors::Selection, Colors::AccentPrimary,
            Colors::Surface, Colors::TextMuted, Colors::TextSecondary);
}

inline QString buttonStyle() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 %1, stop:1 %2);
            border: none;
            border-radius: %3px;
            padding: 10px 20px;
            color: %4;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 %5, stop:1 %6);
        }
        QPushButton:pressed {
            background: %2;
        }
        QPushButton:disabled {
            background: %7;
            color: %8;
        }
    )").arg(Colors::SurfaceHover, Colors::SurfaceActive, QString::number(Radius::MD),
            Colors::TextPrimary, Colors::SurfaceActive, Colors::Surface,
            Colors::Surface, Colors::TextMuted);
}

inline QString accentButtonStyle() {
    return QString(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %1, stop:1 %2);
            border: none;
            border-radius: %3px;
            padding: 10px 24px;
            color: %4;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 %5, stop:1 %6);
        }
    )").arg(Colors::AccentPrimary, Colors::AccentSecondary, QString::number(Radius::MD),
            Colors::Background, "#33e0ff", "#cc33ff");
}

inline QString listWidgetStyle() {
    return QString(R"(
        QListWidget {
            background: %1;
            border: 1px solid %2;
            border-radius: %3px;
            color: %4;
            outline: none;
        }
        QListWidget::item {
            padding: 10px 14px;
            border: none;
            border-radius: %5px;
            margin: 2px 4px;
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 rgba(0, 212, 255, 0.3), stop:1 rgba(180, 0, 255, 0.2));
            color: %6;
            border-left: 3px solid %7;
        }
        QListWidget::item:hover:!selected {
            background: %8;
        }
    )").arg(Colors::Surface, Colors::GlassBorder, QString::number(Radius::MD),
            Colors::TextPrimary, QString::number(Radius::SM), Colors::TextPrimary,
            Colors::AccentPrimary, Colors::SurfaceHover);
}

inline QString dialogStyle() {
    return QString(R"(
        QDialog {
            background: %1;
            color: %2;
            border-radius: %3px;
        }
        QLabel {
            color: %4;
        }
    )").arg(Colors::Background, Colors::TextPrimary, QString::number(Radius::XL), Colors::TextSecondary);
}

// Helper: Add glow shadow effect
inline void addGlowEffect(QWidget *widget, const QColor &color = QColor(0, 212, 255, 80)) {
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(20);
    shadow->setColor(color);
    shadow->setOffset(0, 0);
    widget->setGraphicsEffect(shadow);
}

// Helper: Add subtle shadow
inline void addShadow(QWidget *widget, int blur = 15, int offsetY = 4) {
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blur);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, offsetY);
    widget->setGraphicsEffect(shadow);
}

// Helper: Fade in animation
inline QPropertyAnimation* fadeIn(QWidget *widget, int duration = Animation::Normal) {
    auto *anim = new QPropertyAnimation(widget, "windowOpacity");
    anim->setDuration(duration);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    return anim;
}

} // namespace Theme
