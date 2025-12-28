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
    m_bindings[{Qt::Key_Space, Qt::NoModifier}] = PAUSE_ALL;
    m_bindings[{Qt::Key_P, Qt::NoModifier}] = PAUSE_ALL;

    // Volume - U/I
    m_bindings[{Qt::Key_U, Qt::NoModifier}] = VOLUME_UP;
    m_bindings[{Qt::Key_I, Qt::NoModifier}] = VOLUME_DOWN;

    // Navigation - WASD
    m_bindings[{Qt::Key_W, Qt::NoModifier}] = NAVIGATE_UP;
    m_bindings[{Qt::Key_S, Qt::NoModifier}] = NAVIGATE_DOWN;
    m_bindings[{Qt::Key_A, Qt::NoModifier}] = NAVIGATE_LEFT;
    m_bindings[{Qt::Key_D, Qt::NoModifier}] = NAVIGATE_RIGHT;

    // Seek - V und C
    m_bindings[{Qt::Key_V, Qt::NoModifier}] = SEEK_FORWARD;
    m_bindings[{Qt::Key_C, Qt::NoModifier}] = SEEK_BACKWARD;

    // Screenshot
    m_bindings[{Qt::Key_T, Qt::NoModifier}] = SCREENSHOT;

    // Frame stepping
    m_bindings[{Qt::Key_B, Qt::NoModifier}] = FRAME_STEP_BACKWARD;
    m_bindings[{Qt::Key_N, Qt::NoModifier}] = FRAME_STEP_FORWARD;

    // Grid controls (S conflicts with navigation, using Q for shuffle)
    m_bindings[{Qt::Key_N, Qt::ShiftModifier}] = NEXT_ALL;  // Shift+N for next all
    m_bindings[{Qt::Key_Q, Qt::NoModifier}] = SHUFFLE_ALL;
    m_bindings[{Qt::Key_X, Qt::NoModifier}] = SHUFFLE_THEN_NEXT_ALL;

    // Fullscreen
    m_bindings[{Qt::Key_F11, Qt::NoModifier}] = FULLSCREEN_GLOBAL;
    m_bindings[{Qt::Key_Escape, Qt::NoModifier}] = EXIT_FULLSCREEN;
    m_bindings[{Qt::Key_F, Qt::NoModifier}] = FULLSCREEN_SELECTED;

    // Selected cell controls
    m_bindings[{Qt::Key_L, Qt::NoModifier}] = TOGGLE_LOOP;
    m_bindings[{Qt::Key_Z, Qt::NoModifier}] = ZOOM_IN;
    m_bindings[{Qt::Key_Z, Qt::ShiftModifier}] = ZOOM_OUT;
    m_bindings[{Qt::Key_R, Qt::ControlModifier}] = ROTATE;

    // Descriptions
    m_descriptions[PAUSE_ALL] = "Pause/Play all cells";
    m_descriptions[NEXT_ALL] = "Next video (all cells)";
    m_descriptions[PREV_ALL] = "Previous video (all cells)";
    m_descriptions[SHUFFLE_ALL] = "Shuffle all playlists";
    m_descriptions[SHUFFLE_THEN_NEXT_ALL] = "Shuffle then next";
    m_descriptions[FULLSCREEN_GLOBAL] = "Toggle fullscreen";
    m_descriptions[EXIT_FULLSCREEN] = "Exit fullscreen";
    m_descriptions[VOLUME_UP] = "Volume up";
    m_descriptions[VOLUME_DOWN] = "Volume down";
    m_descriptions[NAVIGATE_UP] = "Navigate selection up";
    m_descriptions[NAVIGATE_DOWN] = "Navigate selection down";
    m_descriptions[NAVIGATE_LEFT] = "Navigate selection left";
    m_descriptions[NAVIGATE_RIGHT] = "Navigate selection right";
    m_descriptions[FULLSCREEN_SELECTED] = "Fullscreen selected cell";
    m_descriptions[SEEK_FORWARD] = "Seek forward 5s";
    m_descriptions[SEEK_BACKWARD] = "Seek backward 5s";
    m_descriptions[FRAME_STEP_FORWARD] = "Frame step forward";
    m_descriptions[FRAME_STEP_BACKWARD] = "Frame step backward";
    m_descriptions[TOGGLE_LOOP] = "Toggle loop on selected";
    m_descriptions[ZOOM_IN] = "Zoom in";
    m_descriptions[ZOOM_OUT] = "Zoom out";
    m_descriptions[ROTATE] = "Rotate video";
    m_descriptions[SCREENSHOT] = "Take screenshot";
}

KeyMap::Action KeyMap::getAction(QKeyEvent *event) const
{
    KeyBinding binding;
    binding.key = static_cast<Qt::Key>(event->key());
    binding.modifiers = event->modifiers() & ~Qt::KeypadModifier; // Ignore keypad modifier

    return m_bindings.value(binding, NO_ACTION);
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
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(PAUSE_ALL).join("/")).arg(getActionDescription(PAUSE_ALL));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(VOLUME_UP).join("/")).arg(getActionDescription(VOLUME_UP));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(VOLUME_DOWN).join("/")).arg(getActionDescription(VOLUME_DOWN));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(NEXT_ALL).join("/")).arg(getActionDescription(NEXT_ALL));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(SHUFFLE_ALL).join("/")).arg(getActionDescription(SHUFFLE_ALL));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(SHUFFLE_THEN_NEXT_ALL).join("/")).arg(getActionDescription(SHUFFLE_THEN_NEXT_ALL));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(FULLSCREEN_GLOBAL).join("/")).arg(getActionDescription(FULLSCREEN_GLOBAL));

    // Navigation
    tooltip += "\nNavigation:\n";
    tooltip += QString("  WASD - Navigate grid selection\n");

    // Selected cell
    tooltip += "\nSelected Cell:\n";
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(FULLSCREEN_SELECTED).join("/")).arg(getActionDescription(FULLSCREEN_SELECTED));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(SEEK_FORWARD).join("/")).arg(getActionDescription(SEEK_FORWARD));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(SEEK_BACKWARD).join("/")).arg(getActionDescription(SEEK_BACKWARD));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(FRAME_STEP_FORWARD).join("/")).arg(getActionDescription(FRAME_STEP_FORWARD));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(FRAME_STEP_BACKWARD).join("/")).arg(getActionDescription(FRAME_STEP_BACKWARD));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(SCREENSHOT).join("/")).arg(getActionDescription(SCREENSHOT));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(TOGGLE_LOOP).join("/")).arg(getActionDescription(TOGGLE_LOOP));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(ZOOM_IN).join("/")).arg(getActionDescription(ZOOM_IN));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(ZOOM_OUT).join("/")).arg(getActionDescription(ZOOM_OUT));
    tooltip += QString("  %1 - %2\n").arg(getKeysForAction(ROTATE).join("/")).arg(getActionDescription(ROTATE));

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
