# Contributing to Goobert

This guide is for developers who want to maintain or extend the Goobert codebase.

## Build & Development

### Quick Start

```bash
# Clone and build
git clone https://github.com/drzo1dberg/goobert.git
cd goobert
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./goobert /path/to/media
```

### Dependencies

| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| CMake | 3.16 | Build system |
| Qt6 | 6.2 | UI framework |
| libmpv | 0.35 | Video playback |
| C++ Compiler | C++20 | GCC 10+ or Clang 11+ |

### CMake Options

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..    # Debug build
cmake -DCMAKE_BUILD_TYPE=Release ..  # Release build
```

## Architecture Overview

### Component Hierarchy

```
MainWindow (QMainWindow)
│
├── ToolBar (QToolBar)
│   ├── Grid spinboxes (cols x rows)
│   ├── Source path + browse button
│   ├── Filter input
│   ├── Start/Stop buttons
│   ├── Playback controls (|< || >| Shuf FS)
│   ├── Volume slider
│   └── Panel toggle button
│
└── QSplitter (horizontal)
    ├── SidePanel (QWidget, collapsible)
    │   └── QTabWidget
    │       ├── MonitorWidget (cell status table)
    │       └── PlaylistWidget (per-cell playlists)
    │
    └── Video Wall (QWidget)
        └── QGridLayout
            └── GridCell[] (QFrame)
                ├── MpvWidget (QOpenGLWidget)
                └── Loop indicator (QLabel overlay)
```

### Key Classes

#### Core

| Class | File | Responsibility |
|-------|------|----------------|
| `MainWindow` | mainwindow.cpp/h | Application shell, grid lifecycle, input dispatch, fullscreen modes |
| `MpvWidget` | mpvwidget.cpp/h | libmpv OpenGL rendering, playback control, property observation |
| `GridCell` | gridcell.cpp/h | Video container, mouse events, loop indicator overlay |

#### UI Components

| Class | File | Responsibility |
|-------|------|----------------|
| `ToolBar` | toolbar.cpp/h | Unified control bar with all configuration and playback controls |
| `SidePanel` | sidepanel.cpp/h | Tabbed container hosting Monitor and Playlist widgets |
| `MonitorWidget` | monitorwidget.cpp/h | Real-time cell status table with context menu |
| `PlaylistWidget` | playlistwidget.cpp/h | Per-cell playlist display and management |
| `PlaylistPicker` | playlistpicker.cpp/h | Modal dialog for quick file search and selection |

#### Services

| Class | File | Responsibility |
|-------|------|----------------|
| `Config` | config.cpp/h | Singleton settings manager using QSettings |
| `KeyMap` | keymap.cpp/h | Centralized keyboard shortcut definitions |
| `FileScanner` | filescanner.cpp/h | Recursive directory scanner with filter support |

#### Theme

| File | Purpose |
|------|---------|
| `theme.h` | Centralized color palette, spacing constants, stylesheet generators |

### Signal Flow

```
User Input
    │
    ▼
MainWindow::keyPressEvent() / eventFilter()
    │
    ▼
KeyMap::getAction() → Action enum
    │
    ▼
MainWindow action handlers (e.g., playPauseAll, seekSelected)
    │
    ▼
GridCell methods (e.g., togglePause, seekRelative)
    │
    ▼
MpvWidget commands → mpv_command() / mpv_set_property()
    │
    ▼
mpv event callbacks → MpvWidget signals
    │
    ▼
GridCell / MonitorWidget / PlaylistWidget updates
```

### Data Flow

```
Startup:
  Config::instance() → Load ~/.config/goobert/goobert.ini
  ToolBar reads defaults from Config

Start Grid:
  FileScanner::scan(path, filter) → QStringList files
  MainWindow::buildGrid() → Create GridCell instances
  GridCell::setPlaylist() → MpvWidget::loadPlaylist()

Playback:
  MpvWidget observes: time-pos, duration, pause, path
  MpvWidget emits: positionChanged, fileChanged, pauseChanged
  GridCell throttles position updates (~4Hz)
  MonitorWidget updates cell status table
```

## Code Conventions

### Naming

- **Classes**: PascalCase (`MainWindow`, `MpvWidget`)
- **Methods**: camelCase (`playPauseAll`, `seekSelected`)
- **Members**: `m_` prefix (`m_cells`, `m_selectedRow`)
- **Constants**: `k` prefix in namespace (`kDefaultWidth`, `kGridSpacing`)
- **Signals**: No prefix, descriptive (`fileChanged`, `selected`)

### File Organization

```cpp
// Header files
#pragma once

#include <QtIncludes>
#include "localincludes.h"

// Forward declarations if needed
class ClassName;

// Constants namespace (if needed)
namespace ClassConstants {
    inline constexpr int kValue = 42;
}

class ClassName : public QBaseClass
{
    Q_OBJECT

public:
    // Constructors, destructor
    // Public methods
    // Getters with [[nodiscard]]

signals:
    // Qt signals

protected:
    // Overridden virtuals

private slots:
    // Private slot handlers

private:
    // Private methods
    // Member variables (m_ prefix)
};
```

### Memory Management

- Use Qt's parent-child ownership model
- Widgets created with `new` should have a parent
- No manual `delete` needed for parented widgets
- Use smart pointers only for non-QObject resources

### Signals & Slots

```cpp
// Prefer new-style connections
connect(sender, &Sender::signal, receiver, &Receiver::slot);

// Lambda for simple one-liners
connect(button, &QPushButton::clicked, this, [this]() {
    doSomething();
});

// Named slot for complex logic
connect(m_mpv, &MpvWidget::fileChanged, this, &GridCell::onFileChanged);
```

## Adding Features

### Adding a Keyboard Shortcut

1. **Add action enum** in `keymap.h`:
```cpp
enum class Action {
    // ...
    MyNewAction,
    // ...
};
```

2. **Add binding** in `keymap.cpp`:
```cpp
m_bindings[{Qt::Key_H, Qt::NoModifier}] = Action::MyNewAction;
m_descriptions[Action::MyNewAction] = "Description for tooltip";
```

3. **Add key to event filter** in `mainwindow.cpp` (if needed):
```cpp
if (key == Qt::Key_H || /* other keys */) {
    keyPressEvent(keyEvent);
    return true;
}
```

4. **Handle action** in `MainWindow::keyPressEvent()`:
```cpp
case KeyMap::Action::MyNewAction:
    handleMyNewAction();
    break;
```

### Adding a Config Setting

1. **Add getter/setter** in `config.h`:
```cpp
[[nodiscard]] int myNewSetting() const;
void setMyNewSetting(int value);
```

2. **Implement** in `config.cpp`:
```cpp
int Config::myNewSetting() const {
    return m_settings.value("section/my_new_setting", 42).toInt();
}

void Config::setMyNewSetting(int value) {
    m_settings.setValue("section/my_new_setting", value);
}
```

### Adding an MpvWidget Feature

1. **Add method** in `mpvwidget.h`:
```cpp
void myFeature();
```

2. **Implement** in `mpvwidget.cpp`:
```cpp
void MpvWidget::myFeature() {
    if (!m_mpv) return;
    // Use command() for mpv commands
    command(QVariantList{"command-name", "arg1", "arg2"});
    // Or setProperty() for properties
    setProperty("property-name", value);
}
```

3. **Forward through GridCell** if needed:
```cpp
void GridCell::myFeature() {
    m_mpv->myFeature();
}
```

### Adding UI Elements

Use `theme.h` for consistent styling:

```cpp
#include "theme.h"

// Use color constants
setStyleSheet(QString("background: %1;").arg(Theme::Colors::Surface));

// Use spacing constants
layout->setContentsMargins(Theme::Spacing::MD, Theme::Spacing::MD, ...);

// Use helper functions
button->setStyleSheet(Theme::buttonStyle());
```

## Testing

### Manual Testing Checklist

- [ ] Start/stop grid with various sizes (1x1, 3x3, 5x5)
- [ ] All keyboard shortcuts work
- [ ] Mouse controls (click, double-click, scroll, middle-click)
- [ ] Fullscreen modes (global and tile)
- [ ] Volume control and mute
- [ ] Loop toggle with visual indicator
- [ ] Playlist picker (Y key) selects correct file
- [ ] Zoom to cursor works correctly
- [ ] Side panel toggle (P button)
- [ ] Config values persist across restarts

### Debug Tips

```cpp
// Enable mpv console output temporarily
mpv_set_option_string(m_mpv, "terminal", "yes");
mpv_set_option_string(m_mpv, "msg-level", "all=v");

// Qt debug output
qDebug() << "Variable:" << variable;

// Check mpv properties
qDebug() << "Current file:" << getProperty("path");
qDebug() << "Position:" << getProperty("time-pos");
```

## Common Pitfalls

### mpv Threading

- mpv callbacks run on mpv's thread, not the Qt main thread
- Use `Glib::Dispatcher` or `QMetaObject::invokeMethod` for thread-safe UI updates
- The wakeup callback should only trigger event processing, not do heavy work

### OpenGL Context

- `MpvWidget::initializeGL()` is called after the GL context is ready
- Don't call mpv render functions before `m_initialized` is true
- Always call `makeCurrent()` before GL operations outside `paintGL()`

### Event Filter

- `MainWindow::eventFilter()` intercepts key events from child widgets
- Make sure new shortcut keys are added to the filter's key list
- Return `true` to consume the event, `false` to let it propagate

### Playlist Order

- After `shuffle`, mpv's internal playlist order differs from `m_currentPlaylist`
- Use `MpvWidget::currentPlaylist()` which queries mpv directly
- Indices from `PlaylistPicker` correspond to mpv's current order

## Performance Considerations

- GridCell throttles position updates to ~4Hz (see `kPositionEmitInterval`)
- Each MpvWidget has its own render context (GPU memory per cell)
- FileScanner is synchronous - large directories may cause UI freeze
- Watchdog timer checks cells every 5 seconds for auto-restart

## Git Workflow

```bash
# Feature branch
git checkout -b feature/my-feature
# ... make changes ...
git add -A
git commit -m "Add my feature"
git push origin feature/my-feature
# Create PR on GitHub

# Bugfix
git checkout -b fix/issue-description
# ... fix bug ...
git commit -m "Fix: description of the fix"
```

### Commit Message Style

```
Short summary (imperative mood, <50 chars)

Optional longer description explaining the what and why.
Wrap at 72 characters.

- Bullet points are okay
- Use present tense ("Add feature" not "Added feature")
```

## Statistics System

### Overview

Goobert includes an SQLite-based statistics tracking system with a web dashboard.

**Database Location:** `~/.config/goobert/goobert.db`

### Database Schema

| Table | Purpose |
|-------|---------|
| `file_stats` | Per-file watch time, play count, last position, duration |
| `watch_sessions` | Individual playback sessions with timestamps |
| `skip_events` | Skip/seek events with from/to positions |
| `loop_events` | Loop toggle events |
| `favorites` | User-marked favorite files |
| `position_samples` | Position sampling for heatmaps |
| `pause_events` | Pause/resume events |
| `volume_events` | Volume change history |
| `zoom_events` | Zoom/pan events |
| `screenshot_events` | Screenshot captures |
| `fullscreen_events` | Fullscreen enter/exit |
| `grid_events` | Grid start/stop sessions |
| `rotation_events` | Video rotation events |

### Adding New Statistics

1. **Add table** in `StatsManager::createTables()`:
```cpp
ok = query.exec(R"(
    CREATE TABLE IF NOT EXISTS my_new_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        file_id INTEGER,
        timestamp INTEGER NOT NULL,
        my_data TEXT,
        FOREIGN KEY (file_id) REFERENCES file_stats(id)
    )
)");
```

2. **Add index** for performance:
```cpp
query.exec("CREATE INDEX IF NOT EXISTS idx_my_events_file ON my_new_events(file_id)");
```

3. **Add logging method** in `StatsManager`:
```cpp
void StatsManager::logMyEvent(const QString &filePath, const QString &data)
{
    if (!m_initialized || filePath.isEmpty()) return;

    qint64 fileId = getOrCreateFileId(filePath, 0, false);
    if (fileId < 0) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO my_new_events (file_id, timestamp, my_data) VALUES (?, ?, ?)");
    query.addBindValue(fileId);
    query.addBindValue(QDateTime::currentMSecsSinceEpoch());
    query.addBindValue(data);
    query.exec();
}
```

4. **Add query method**:
```cpp
QList<MyEvent> StatsManager::getMyEvents(int limit) const
{
    // Implementation...
}
```

5. **Declare in header** (`statsmanager.h`):
```cpp
void logMyEvent(const QString &filePath, const QString &data);
[[nodiscard]] QList<MyEvent> getMyEvents(int limit = 100) const;
```

### Web Dashboard

The dashboard is a Flask application in the `dashboard/` directory.

#### Running

```bash
cd dashboard
pip install flask flask-cors
python3 app.py
# Open http://localhost:5000
```

#### Adding API Endpoints

1. **Add route** in `dashboard/app.py`:
```python
@app.route('/api/my-stats')
def api_my_stats():
    """Get my custom statistics"""
    conn = get_db()
    cur = conn.cursor()

    cur.execute("""
        SELECT my_data, COUNT(*) as cnt
        FROM my_new_events
        GROUP BY my_data
    """)

    result = {}
    for row in cur.fetchall():
        result[row['my_data']] = row['cnt']

    conn.close()
    return jsonify(result)
```

2. **Add UI component** in `dashboard/templates/index.html`:
```html
<div class="card">
    <div class="card-header">
        <span class="card-title">My Stats</span>
    </div>
    <div class="chart-container">
        <canvas id="myStatsChart"></canvas>
    </div>
</div>
```

3. **Add JavaScript loader**:
```javascript
async function loadMyStats() {
    const data = await fetchData('my-stats');
    // Create Chart.js visualization
}
```

4. **Call in refreshAll()**:
```javascript
async function refreshAll() {
    await Promise.all([
        // ... existing loaders
        loadMyStats()
    ]);
}
```

#### Chart.js Colors

Use the predefined color palette:
```javascript
const chartColors = {
    accent: '#00d4ff',
    accentDim: 'rgba(0, 212, 255, 0.3)',
    success: '#00ff88',
    successDim: 'rgba(0, 255, 136, 0.3)',
    grid: 'rgba(255, 255, 255, 0.05)',
    text: '#a0a0b0'
};
```

### Settings Dialog

The `SettingsDialog` has 4 tabs. To add a setting:

1. **Add UI control** in the appropriate tab method
2. **Connect to Config** getter/setter
3. **Apply changes** in the apply/save handlers

## Resources

- [Qt6 Documentation](https://doc.qt.io/qt-6/)
- [Qt6 SQL](https://doc.qt.io/qt-6/qtsql-index.html)
- [libmpv API](https://mpv.io/manual/master/#embedding-into-other-programs-libmpv)
- [mpv Properties Reference](https://mpv.io/manual/master/#properties)
- [mpv Commands Reference](https://mpv.io/manual/master/#list-of-input-commands)
- [Flask Documentation](https://flask.palletsprojects.com/)
- [Chart.js Documentation](https://www.chartjs.org/docs/)
