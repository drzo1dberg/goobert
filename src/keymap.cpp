#include "keymap.h"

KeyMap& KeyMap::instance()
{
    static KeyMap instance;
    return instance;
}

KeyMap::KeyMap()
{
    setupDefaultBindings();
}

void KeyMap::setupDefaultBindings()
{
    // ===========================================================================
    // DOPPELBELEGUNGEN: Einfach mehrere Tasten auf die gleiche Action mappen!
    // ===========================================================================

    // Pause/Play - DOPPELBELEGUNG: Space und P
    m_bindings[{Qt::Key_Space, Qt::NoModifier}] = Action::PauseAll;
    m_bindings[{Qt::Key_P, Qt::NoModifier}] = Action::PauseAll;

    // Volume - U/I
    m_bindings[{Qt::Key_U, Qt::NoModifier}] = Action::VolumeUp;
    m_bindings[{Qt::Key_I, Qt::NoModifier}] = Action::VolumeDown;

    // Navigation - WASD
    m_bindings[{Qt::Key_W, Qt::NoModifier}] = Action::NavigateUp;
    m_bindings[{Qt::Key_S, Qt::NoModifier}] = Action::NavigateDown;
    m_bindings[{Qt::Key_A, Qt::NoModifier}] = Action::NavigateLeft;
    m_bindings[{Qt::Key_D, Qt::NoModifier}] = Action::NavigateRight;

    // Seek - V und C
    m_bindings[{Qt::Key_V, Qt::NoModifier}] = Action::SeekForward;
    m_bindings[{Qt::Key_C, Qt::NoModifier}] = Action::SeekBackward;

    // Screenshot
    m_bindings[{Qt::Key_T, Qt::NoModifier}] = Action::Screenshot;

    // Frame stepping
    m_bindings[{Qt::Key_B, Qt::NoModifier}] = Action::FrameStepBackward;
    m_bindings[{Qt::Key_N, Qt::NoModifier}] = Action::FrameStepForward;

    // Grid controls (S conflicts with navigation, using Q for shuffle)
    m_bindings[{Qt::Key_E, Qt::NoModifier}] = Action::NextAll;  // E for next all
    m_bindings[{Qt::Key_Q, Qt::NoModifier}] = Action::ShuffleAll;
    m_bindings[{Qt::Key_X, Qt::NoModifier}] = Action::ShuffleThenNextAll;

    // Fullscreen
    m_bindings[{Qt::Key_F11, Qt::NoModifier}] = Action::FullscreenGlobal;
    m_bindings[{Qt::Key_Escape, Qt::NoModifier}] = Action::ExitFullscreen;
    m_bindings[{Qt::Key_F, Qt::NoModifier}] = Action::FullscreenSelected;

    // Selected cell controls
    m_bindings[{Qt::Key_L, Qt::NoModifier}] = Action::ToggleLoop;
    m_bindings[{Qt::Key_Comma, Qt::NoModifier}] = Action::PrevSelected;   // , for prev in playlist
    m_bindings[{Qt::Key_Period, Qt::NoModifier}] = Action::NextSelected;  // . for next in playlist
    m_bindings[{Qt::Key_Z, Qt::NoModifier}] = Action::ZoomIn;
    m_bindings[{Qt::Key_Z, Qt::ShiftModifier}] = Action::ZoomOut;
    m_bindings[{Qt::Key_R, Qt::ControlModifier}] = Action::Rotate;

    // Descriptions
    m_descriptions[Action::PauseAll] = "Pause/Play all cells";
    m_descriptions[Action::NextAll] = "Next video (all cells)";
    m_descriptions[Action::PrevAll] = "Previous video (all cells)";
    m_descriptions[Action::ShuffleAll] = "Shuffle all playlists";
    m_descriptions[Action::ShuffleThenNextAll] = "Shuffle then next";
    m_descriptions[Action::FullscreenGlobal] = "Toggle fullscreen";
    m_descriptions[Action::ExitFullscreen] = "Exit fullscreen";
    m_descriptions[Action::VolumeUp] = "Volume up";
    m_descriptions[Action::VolumeDown] = "Volume down";
    m_descriptions[Action::NavigateUp] = "Navigate selection up";
    m_descriptions[Action::NavigateDown] = "Navigate selection down";
    m_descriptions[Action::NavigateLeft] = "Navigate selection left";
    m_descriptions[Action::NavigateRight] = "Navigate selection right";
    m_descriptions[Action::FullscreenSelected] = "Fullscreen selected cell";
    m_descriptions[Action::SeekForward] = "Seek forward 5s";
    m_descriptions[Action::SeekBackward] = "Seek backward 5s";
    m_descriptions[Action::FrameStepForward] = "Frame step forward";
    m_descriptions[Action::FrameStepBackward] = "Frame step backward";
    m_descriptions[Action::ToggleLoop] = "Toggle loop on selected";
    m_descriptions[Action::NextSelected] = "Next in selected playlist";
    m_descriptions[Action::PrevSelected] = "Prev in selected playlist";
    m_descriptions[Action::ZoomIn] = "Zoom in";
    m_descriptions[Action::ZoomOut] = "Zoom out";
    m_descriptions[Action::Rotate] = "Rotate video";
    m_descriptions[Action::Screenshot] = "Take screenshot";
}

KeyMap::Action KeyMap::getAction(QKeyEvent *event) const
{
    KeyBinding binding;
    binding.key = static_cast<Qt::Key>(event->key());
    binding.modifiers = event->modifiers() & ~Qt::KeypadModifier; // Ignore keypad modifier

    return m_bindings.value(binding, Action::NoAction);
}

QList<QString> KeyMap::getKeysForAction(Action action) const
{
    QList<QString> keys;
    for (auto it = m_bindings.constBegin(); it != m_bindings.constEnd(); ++it) {
        if (it.value() == action) {
            keys.append(getKeyDescription(it.key().key, it.key().modifiers));
        }
    }
    return keys;
}

QString KeyMap::getActionDescription(Action action) const
{
    return m_descriptions.value(action, "Unknown action");
}

QString KeyMap::getKeyDescription(Qt::Key key, Qt::KeyboardModifiers mods) const
{
    QString result;

    // Modifiers
    if (mods & Qt::ControlModifier) result += "Ctrl+";
    if (mods & Qt::ShiftModifier) result += "Shift+";
    if (mods & Qt::AltModifier) result += "Alt+";

    // Special keys
    switch (key) {
    case Qt::Key_Space: return result + "Space";
    case Qt::Key_Up: return result + "↑";
    case Qt::Key_Down: return result + "↓";
    case Qt::Key_Left: return result + "←";
    case Qt::Key_Right: return result + "→";
    case Qt::Key_F11: return result + "F11";
    case Qt::Key_Escape: return result + "Esc";
    default:
        // Regular keys
        result += QKeySequence(key).toString();
        return result;
    }
}

QString KeyMap::generateTooltip() const
{
    QString tooltip = "Keyboard Shortcuts:\n\n";

    // Global actions
    tooltip += "Global:\n";
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::PauseAll).join("/")).arg(getActionDescription(Action::PauseAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::VolumeUp).join("/")).arg(getActionDescription(Action::VolumeUp));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::VolumeDown).join("/")).arg(getActionDescription(Action::VolumeDown));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::NextAll).join("/")).arg(getActionDescription(Action::NextAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ShuffleAll).join("/")).arg(getActionDescription(Action::ShuffleAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ShuffleThenNextAll).join("/")).arg(getActionDescription(Action::ShuffleThenNextAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FullscreenGlobal).join("/")).arg(getActionDescription(Action::FullscreenGlobal));

    // Navigation
    tooltip += "\nNavigation:\n";
    tooltip += QString("  WASD - Navigate grid selection\n");

    // Selected cell
    tooltip += "\nSelected Cell:\n";
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FullscreenSelected).join("/")).arg(getActionDescription(Action::FullscreenSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::SeekForward).join("/")).arg(getActionDescription(Action::SeekForward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::SeekBackward).join("/")).arg(getActionDescription(Action::SeekBackward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FrameStepForward).join("/")).arg(getActionDescription(Action::FrameStepForward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FrameStepBackward).join("/")).arg(getActionDescription(Action::FrameStepBackward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::Screenshot).join("/")).arg(getActionDescription(Action::Screenshot));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ToggleLoop).join("/")).arg(getActionDescription(Action::ToggleLoop));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::NextSelected).join("/")).arg(getActionDescription(Action::NextSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::PrevSelected).join("/")).arg(getActionDescription(Action::PrevSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ZoomIn).join("/")).arg(getActionDescription(Action::ZoomIn));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ZoomOut).join("/")).arg(getActionDescription(Action::ZoomOut));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::Rotate).join("/")).arg(getActionDescription(Action::Rotate));

    // Mouse controls
    tooltip += "\nMouse (on cell):\n";
    tooltip += "  Left Click - Select cell\n";
    tooltip += "  Right Click - Pause cell\n";
    tooltip += "  Middle Click - Toggle loop\n";
    tooltip += "  Forward Button - Next video\n";
    tooltip += "  Double Click - Fullscreen cell\n";
    tooltip += "  Scroll Wheel - Frame step\n";
    tooltip += "  Side Scroll - Seek ±30s";

    return tooltip;
}
