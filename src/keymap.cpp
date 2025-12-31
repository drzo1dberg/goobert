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
    // ONE-HANDED LAYOUT (left hand on QWERTY)
    // All keys accessible with left hand - no F-keys, no arrow keys
    //
    // Layout:
    //   ` 1 2 3 4 5
    //     Q W E R T
    //     A S D F G
    //     Z X C V B
    //   Tab Space
    // ===========================================================================

    // === Global Controls ===
    m_bindings[{Qt::Key_Space, Qt::NoModifier}] = Action::PauseAll;
    m_bindings[{Qt::Key_Tab, Qt::NoModifier}] = Action::FullscreenGlobal;
    m_bindings[{Qt::Key_Escape, Qt::NoModifier}] = Action::ExitFullscreen;

    // === Number Row (1-5) ===
    m_bindings[{Qt::Key_1, Qt::NoModifier}] = Action::VolumeDown;
    m_bindings[{Qt::Key_2, Qt::NoModifier}] = Action::VolumeUp;
    m_bindings[{Qt::Key_3, Qt::NoModifier}] = Action::PrevSelected;
    m_bindings[{Qt::Key_4, Qt::NoModifier}] = Action::NextSelected;
    m_bindings[{Qt::Key_Y, Qt::NoModifier}] = Action::ShowPlaylistPicker;
    m_bindings[{Qt::Key_QuoteLeft, Qt::NoModifier}] = Action::ToggleMute;  // ` backtick
    m_bindings[{Qt::Key_6, Qt::NoModifier}] = Action::PanicReset;  // Panic button - reset session

    // === Top Row (QWERT) ===
    m_bindings[{Qt::Key_Q, Qt::NoModifier}] = Action::ShuffleAll;
    m_bindings[{Qt::Key_W, Qt::NoModifier}] = Action::NavigateUp;
    m_bindings[{Qt::Key_E, Qt::NoModifier}] = Action::NextAll;
    m_bindings[{Qt::Key_R, Qt::NoModifier}] = Action::ToggleLoop;
    m_bindings[{Qt::Key_R, Qt::ShiftModifier}] = Action::Rotate;
    m_bindings[{Qt::Key_T, Qt::NoModifier}] = Action::Screenshot;

    // === Home Row (ASDFG) ===
    m_bindings[{Qt::Key_A, Qt::NoModifier}] = Action::NavigateLeft;
    m_bindings[{Qt::Key_S, Qt::NoModifier}] = Action::NavigateDown;
    m_bindings[{Qt::Key_D, Qt::NoModifier}] = Action::NavigateRight;
    m_bindings[{Qt::Key_F, Qt::NoModifier}] = Action::FullscreenSelected;
    m_bindings[{Qt::Key_G, Qt::NoModifier}] = Action::TogglePauseSelected;

    // === Bottom Row (ZXCVB) ===
    m_bindings[{Qt::Key_Z, Qt::NoModifier}] = Action::ZoomIn;
    m_bindings[{Qt::Key_Z, Qt::ShiftModifier}] = Action::ZoomOut;
    m_bindings[{Qt::Key_X, Qt::NoModifier}] = Action::ShuffleThenNextAll;
    m_bindings[{Qt::Key_C, Qt::NoModifier}] = Action::SeekBackward;
    m_bindings[{Qt::Key_V, Qt::NoModifier}] = Action::SeekForward;
    m_bindings[{Qt::Key_C, Qt::ShiftModifier}] = Action::SeekBackwardLong;
    m_bindings[{Qt::Key_V, Qt::ShiftModifier}] = Action::SeekForwardLong;
    m_bindings[{Qt::Key_B, Qt::NoModifier}] = Action::FrameStepForward;
    m_bindings[{Qt::Key_B, Qt::ShiftModifier}] = Action::FrameStepBackward;

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
    m_descriptions[Action::ToggleMute] = "Toggle mute";
    m_descriptions[Action::NavigateUp] = "Navigate selection up";
    m_descriptions[Action::NavigateDown] = "Navigate selection down";
    m_descriptions[Action::NavigateLeft] = "Navigate selection left";
    m_descriptions[Action::NavigateRight] = "Navigate selection right";
    m_descriptions[Action::FullscreenSelected] = "Fullscreen selected cell";
    m_descriptions[Action::SeekForward] = "Seek forward 5s";
    m_descriptions[Action::SeekBackward] = "Seek backward 5s";
    m_descriptions[Action::SeekForwardLong] = "Seek forward 2min";
    m_descriptions[Action::SeekBackwardLong] = "Seek backward 2min";
    m_descriptions[Action::FrameStepForward] = "Frame step forward";
    m_descriptions[Action::FrameStepBackward] = "Frame step backward";
    m_descriptions[Action::ToggleLoop] = "Toggle loop on selected";
    m_descriptions[Action::TogglePauseSelected] = "Pause/Play selected cell";
    m_descriptions[Action::ShowPlaylistPicker] = "Open playlist picker";
    m_descriptions[Action::NextSelected] = "Next in selected playlist";
    m_descriptions[Action::PrevSelected] = "Prev in selected playlist";
    m_descriptions[Action::ZoomIn] = "Zoom in";
    m_descriptions[Action::ZoomOut] = "Zoom out";
    m_descriptions[Action::Rotate] = "Rotate video";
    m_descriptions[Action::Screenshot] = "Take screenshot";
    m_descriptions[Action::PanicReset] = "Panic! Stop & reset session";
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
    QString tooltip = "Keyboard Shortcuts (Left Hand):\n\n";

    // Global actions
    tooltip += "Global:\n";
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::PauseAll).join("/")).arg(getActionDescription(Action::PauseAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::VolumeDown).join("/")).arg(getActionDescription(Action::VolumeDown));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::VolumeUp).join("/")).arg(getActionDescription(Action::VolumeUp));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ToggleMute).join("/")).arg(getActionDescription(Action::ToggleMute));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::NextAll).join("/")).arg(getActionDescription(Action::NextAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ShuffleAll).join("/")).arg(getActionDescription(Action::ShuffleAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ShuffleThenNextAll).join("/")).arg(getActionDescription(Action::ShuffleThenNextAll));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FullscreenGlobal).join("/")).arg(getActionDescription(Action::FullscreenGlobal));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::PanicReset).join("/")).arg(getActionDescription(Action::PanicReset));

    // Navigation
    tooltip += "\nNavigation:\n";
    tooltip += QString("  WASD - Navigate grid selection\n");

    // Selected cell
    tooltip += "\nSelected Cell:\n";
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FullscreenSelected).join("/")).arg(getActionDescription(Action::FullscreenSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::TogglePauseSelected).join("/")).arg(getActionDescription(Action::TogglePauseSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::SeekBackward).join("/")).arg(getActionDescription(Action::SeekBackward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::SeekForward).join("/")).arg(getActionDescription(Action::SeekForward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FrameStepForward).join("/")).arg(getActionDescription(Action::FrameStepForward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::FrameStepBackward).join("/")).arg(getActionDescription(Action::FrameStepBackward));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ToggleLoop).join("/")).arg(getActionDescription(Action::ToggleLoop));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::PrevSelected).join("/")).arg(getActionDescription(Action::PrevSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::NextSelected).join("/")).arg(getActionDescription(Action::NextSelected));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ShowPlaylistPicker).join("/")).arg(getActionDescription(Action::ShowPlaylistPicker));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ZoomIn).join("/")).arg(getActionDescription(Action::ZoomIn));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::ZoomOut).join("/")).arg(getActionDescription(Action::ZoomOut));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::Rotate).join("/")).arg(getActionDescription(Action::Rotate));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(Action::Screenshot).join("/")).arg(getActionDescription(Action::Screenshot));

    // Mouse controls
    tooltip += "\nMouse (on cell):\n";
    tooltip += "  Left Click - Select cell\n";
    tooltip += "  Right Click - Pause cell (= G)\n";
    tooltip += "  Middle Click - Toggle loop (= R)\n";
    tooltip += "  Forward Button - Next video (= 4)\n";
    tooltip += "  Double Click - Fullscreen cell (= F)\n";
    tooltip += "  Scroll Wheel - Frame step (= B/Shift+B)\n";
    tooltip += "  Side Scroll - Seek (= C/V)";

    return tooltip;
}
