#pragma once

#include <QKeyEvent>
#include <QMap>
#include <QSet>
#include <QString>

// Central keymap for all keyboard shortcuts
class KeyMap
{
public:
    enum class Action {
        // Global actions
        PauseAll,
        NextAll,
        PrevAll,
        ShuffleAll,
        ShuffleThenNextAll,
        FullscreenGlobal,
        ExitFullscreen,
        VolumeUp,
        VolumeDown,

        // Navigation
        NavigateUp,
        NavigateDown,
        NavigateLeft,
        NavigateRight,

        // Selected cell actions
        FullscreenSelected,
        NextSelected,
        PrevSelected,
        SeekForward,
        SeekBackward,
        FrameStepForward,
        FrameStepBackward,
        ToggleLoop,
        ZoomIn,
        ZoomOut,
        Rotate,
        Screenshot,

        // No action
        NoAction
    };

    static KeyMap& instance();

    // Get action for a key
    Action getAction(QKeyEvent *event) const;

    // Get all keys for an action
    QList<QString> getKeysForAction(Action action) const;

    // Get human-readable description
    QString getActionDescription(Action action) const;
    QString getKeyDescription(Qt::Key key, Qt::KeyboardModifiers mods = Qt::NoModifier) const;

    // Generate tooltip with all shortcuts
    QString generateTooltip() const;

    // KeyBinding struct needs to be public for qHash
    struct KeyBinding {
        Qt::Key key;
        Qt::KeyboardModifiers modifiers;

        bool operator==(const KeyBinding &other) const {
            return key == other.key && modifiers == other.modifiers;
        }

        bool operator<(const KeyBinding &other) const {
            if (key != other.key) return key < other.key;
            return modifiers < other.modifiers;
        }
    };

private:
    KeyMap();

    // Non-copyable (singleton)
    KeyMap(const KeyMap&) = delete;
    KeyMap& operator=(const KeyMap&) = delete;

    void setupDefaultBindings();

    QMap<KeyBinding, Action> m_bindings;
    QMap<Action, QString> m_descriptions;
};

// Hash function for KeyBinding to use in QSet/QMap
inline uint qHash(const KeyMap::KeyBinding &key, uint seed = 0) {
    return qHash(key.key, seed) ^ qHash(int(key.modifiers), seed);
}
