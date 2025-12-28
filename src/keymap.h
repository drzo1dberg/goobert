#pragma once

#include <QKeyEvent>
#include <QMap>
#include <QSet>
#include <QString>
#include <functional>

// Central keymap for all keyboard shortcuts
class KeyMap
{
public:
    enum Action {
        // Global actions
        PAUSE_ALL,
        NEXT_ALL,
        PREV_ALL,
        SHUFFLE_ALL,
        SHUFFLE_THEN_NEXT_ALL,
        FULLSCREEN_GLOBAL,
        EXIT_FULLSCREEN,
        VOLUME_UP,
        VOLUME_DOWN,

        // Navigation
        NAVIGATE_UP,
        NAVIGATE_DOWN,
        NAVIGATE_LEFT,
        NAVIGATE_RIGHT,

        // Selected cell actions
        FULLSCREEN_SELECTED,
        SEEK_FORWARD,
        SEEK_BACKWARD,
        FRAME_STEP_FORWARD,
        FRAME_STEP_BACKWARD,
        TOGGLE_LOOP,
        ZOOM_IN,
        ZOOM_OUT,
        ROTATE,
        SCREENSHOT,

        // No action
        NO_ACTION
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
    void setupDefaultBindings();

    QMap<KeyBinding, Action> m_bindings;
    QMap<Action, QString> m_descriptions;
};

// Hash function for KeyBinding to use in QSet/QMap
inline uint qHash(const KeyMap::KeyBinding &key, uint seed = 0) {
    return qHash(key.key, seed) ^ qHash(int(key.modifiers), seed);
}
