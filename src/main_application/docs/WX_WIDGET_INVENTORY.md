# wxWidgets usage inventory — migration worksheet

A per-class map of every wxWidgets dependency in
`src/main_application/`'s top-level files, organized by pattern, with a
likely Qt target for each. This is meant as a working checklist for the
planned wxWidgets → Qt migration, not just a description of current
behavior — see [`ARCHITECTURE.md`](ARCHITECTURE.md) for the narrative
version of the two big patterns summarized here.

Every other already-documented module (`axes/`, `communication/`, `misc/`,
`opengl_low_level/`, `plot_objects/`, `project_state/`, `serial_interface/`,
`text_stream_objects/`) has **zero** wxWidgets dependency — confirmed by
search in each. All of it is concentrated in the files covered by this
document (plus `PlotPane` in `plot_pane.h/.cpp`, which bridges into the
wx-free `axes/`/`plot_objects/` world via a `wxGLCanvas` base).

## Application bootstrap

| Class / file | wx base | Qt target | Notes |
|---|---|---|---|
| `MainApp` (`main.cpp`) | `wxApp`, via `IMPLEMENT_APP` | `QApplication` + `main()` | No macro equivalent needed in Qt — `IMPLEMENT_APP` disappears entirely. |
| `MainWindow` (`main_window.h/.cpp`) | `wxFrame` (`wxNO_BORDER`) | `QWidget`/`QMainWindow` with `Qt::FramelessWindowHint` | See `ARCHITECTURE.md`'s custom-chrome section — manual drag logic must be ported too, Qt gives no free equivalent. |
| `GuiWindow` (`gui_window.h/.cpp`) | `wxFrame` | `QMainWindow` or plain `QWidget` | One per user-created top-level window; owns menus (see below). |

## Top-level chrome / dialogs (native wx container types)

| Class | wx base | Qt target | Notes |
|---|---|---|---|
| `CmdlOutputWindow` | `wxFrame` + `wxTextCtrl` (`wxTE_MULTILINE \| wxTE_READONLY`) | `QWidget`/`QMainWindow` + `QPlainTextEdit` (read-only) | Near-duplicate of `TopicTextOutputWindow` — see `ARCHITECTURE.md`'s rough edges; consider merging before porting rather than porting both. |
| `TopicTextOutputWindow` | same as above | same as above | " |
| `SettingsWindow` | `wxDialog` | `QDialog` | Builds a form dynamically from a `map<string, pair<string,string>>` of field name → (description, value) pairs, using `wxBoxSizer` layout — port to `QFormLayout`. |
| `CustomTaskBarIcon` (non-Apple) | `wxTaskBarIcon` | `QSystemTrayIcon` + `QMenu` | Uses the older `wxBEGIN_EVENT_TABLE` macro (only file in this module still using that style) rather than `Bind()`. |
| `CustomTaskBarIcon` (`PLATFORM_APPLE_M`) | none — plain class, every method a no-op | n/a | See `ARCHITECTURE.md` — the tray/menu-bar feature is inert on macOS today; decide the intended Qt behavior explicitly rather than porting the no-op as-is. |

## Menus

| Usage | wx type | Qt target |
|---|---|---|
| `MainWindow::createMainMenuBar()`, `GuiWindow`'s `new_element_menu_*`/`popup_menu_*` | `wxMenuBar`, `wxMenu`, `wxMenuItem` | `QMenuBar`, `QMenu`, `QAction` |
| `wxMenuBar::MacSetCommonMenuBar(...)` | macOS-specific wx API, used because there's no per-window native title bar (`wxNO_BORDER`) | N/A — Qt's `QMenuBar` set on a top-level widget already integrates with the native macOS menu bar without an equivalent special call. |
| File open/save, confirmation dialogs | `wxFileDialog`, `wxMessageBox` (`main_window.cpp`) | `QFileDialog`, `QMessageBox` | Straightforward 1:1. |

## Custom-drawn "chrome" widgets (no stock Qt equivalent — full custom paint required)

Every one of these is a bare `wxPanel` subclass: binds `wxEVT_PAINT` to its
own handler, draws with a `wxPaintDC`, and (except `EditingSilhouette`)
drives hover/press animations with one or more `wxTimer`s doing incremental
color interpolation. See `ARCHITECTURE.md` for why this is the costlier
half of the UI migration.

| Class | File | Draws | Timers | Qt target |
|---|---|---|---|---|
| `CloseButton` | `close_button.h/.cpp` | Filled rect + caller-supplied icon-draw lambda (used for both the app's close *and* minimize buttons, with different lambdas) | none | `QWidget` + `paintEvent` (`QPainter`), or a `QPushButton` with a custom `QStyle`/stylesheet if the visual can be achieved via QSS instead of manual drawing |
| `CustomButton` | `custom_button.h` (fully inline, no `.cpp`) | Shadow + rounded rect + optional expanding "ripple" circle on click | `click_timer_`, `entered_timer_`, `exited_timer_` — three independent `wxTimer`s each stepping one visual effect | `QWidget` + `paintEvent`, `QTimer`s (or `QPropertyAnimation` for the color/ripple easing, which Qt does more natively than wx here) |
| `TabButton` | `tab_button.h/.cpp` | Selected/unselected/pressed/hover color states via `ColorPair` (base→brighter-color interpolation) | `entered_timer_` | Same pattern as above; `ColorPair`'s interpolation logic ports directly (pure math, no wx) |
| `WindowButton` | `window_button.h/.cpp` | Same idea as `TabButton`, for the window-switcher buttons in the main window | `entered_timer_` | " |
| `HelpPane` | `help_pane.h/.cpp` | A semi-transparent rounded-rect overlay with hardcoded keybinding help text (`h`/`r`/`t`/`z`/`1`/`2`/`3`) | none currently wired (commented out) | `QWidget` + `paintEvent`; note several mouse-hover color-cycle handlers are present but their bodies are commented out — decide whether to revive that behavior or drop it during the port |
| `EditingSilhouette` | `editing_silhouette.h/.cpp` | A simple black-outline, transparent-fill rectangle showing where a dragged element will land | none | `QWidget` + `paintEvent`; this one is simple enough it could alternatively become a semi-transparent always-on-top sibling widget in Qt rather than custom-painted |

`ColorPair` (declared in `tab_button.h`, used by `TabButton`/`WindowButton`)
is pure math (`wxColour` in, `wxColour` out via linear interpolation) —
trivially portable to `QColor`.

## Native-wx-control wrappers (`gui_elements.h/.cpp`)

Each of these uses multiple inheritance: `class X : public wxSomething,
public ApplicationGuiElement` — `ApplicationGuiElement` (`gui_element.h/.cpp`,
documented in `ARCHITECTURE.md`) is a pure interface with no wx base, so
there's no diamond problem. Each delegates `getPosition`/`getSize`/
`setPosition`/`setSize`/`hide`/`show` straight to the wrapped widget's own
methods — the cleanest, most directly-portable pattern in this codebase.

| Class | wx base | Qt target |
|---|---|---|
| `ButtonGuiElement` | `wxButton` | `QPushButton` |
| `SliderGuiElement` | `wxSlider` | `QSlider` (plus its own `wxStaticText` value/min/max labels — `QLabel`s) |
| `CheckboxGuiElement` | `wxCheckBox` | `QCheckBox` |
| `TextLabelGuiElement` | `wxStaticText` | `QLabel` |
| `ListBoxGuiElement` | `wxListBox` | `QListWidget` |
| `EditableTextGuiElement` | `wxTextCtrl` | `QLineEdit` (single-line) or `QPlainTextEdit` depending on current style flags — check construction site |
| `DropdownMenuGuiElement` | `wxComboBox` | `QComboBox` |
| `RadioButtonGroupGuiElement` | `wxRadioBox` | A `QGroupBox` containing a `QButtonGroup` of `QRadioButton`s (`wxRadioBox` is a single all-in-one control; Qt has no direct one-widget equivalent) |
| `ScrollingTextGuiElement` | `wxTextCtrl` (`wxTE_MULTILINE`, likely `wxTE_READONLY` — the actively-used scrolling-log-text feature; see `text_stream_objects/`'s docs for the abandoned alternative implementation this superseded) | `QPlainTextEdit` |

Every one of these also implements `ApplicationGuiElement::getGuiElementState()`,
returning the matching `gui_element_state.h` subclass — this is protocol
logic (documented in `ARCHITECTURE.md`), not a wx concern, and needs no
change for the Qt port beyond whatever the new widget's "current value"
accessor is called.

## OpenGL canvas integration

| Class | wx base | Qt target | Notes |
|---|---|---|---|
| `PlotPane` | `wxGLCanvas` + `ApplicationGuiElement` | `QOpenGLWidget` | The actual rendering code it drives (`axes/`, `plot_objects/`, `shader.h`) has zero wx dependency — only the canvas/context creation and paint-event wiring in `PlotPane` itself needs porting. `wxGLContext`/`wxGLAttributes` setup is platform-conditional today (`#ifdef PLATFORM_APPLE_M` / `PLATFORM_LINUX_M` branches choosing GL profile/version) — `QSurfaceFormat`/`QOpenGLContext` normalize most of this across platforms already, likely *simplifying* this specific piece of the port rather than complicating it. |
| `BackgroundRenderer` | `wxGLCanvas` | n/a | Dead code (see `ARCHITECTURE.md`) — do not port, just delete pending confirmation. |
| `ShapedFrame` (`graphic_window.h/.cpp`) | `wxFrame` (`wxFRAME_SHAPED \| wxSIMPLE_BORDER \| wxFRAME_NO_TASKBAR \| wxSTAY_ON_TOP`) + `wxRegion`/`SetShape()` | `QWidget` with `setMask(QBitmap)` (Qt's equivalent shaped-window mechanism) | Second confirmed dead-code addition (see `ARCHITECTURE.md`) — a copy of wxWidgets' own "shaped" sample, never instantiated, hardcodes another developer's home directory path. Do not port as-is; if irregular window shapes are ever actually wanted, treat this only as a reference for *how wx did it*, not as code to carry forward. |

## Timers

Every `wxTimer` in this module maps directly to a `QTimer`:
`MainWindow::receive_timer_` (the ~100 Hz receive/render tick — see
`ARCHITECTURE.md`), `MainWindow::refresh_timer_` (dead, never started),
and each custom chrome widget's hover/press animation timer(s) listed in
the table above.

## Event handling idiom

Nearly everything uses the modern `Bind(wxEVT_X, &Class::Handler, this)`
idiom, which maps to Qt's `connect(sender, &Sender::signal, this,
&Receiver::slot)`. The one exception is `tray_icon.cpp`'s
`wxBEGIN_EVENT_TABLE`/`wxDECLARE_EVENT_TABLE` (the older static
event-table macro style) — functionally equivalent, just a different wx-era
idiom; both need the same kind of Qt `connect()` call.

Custom application-wide events declared with `wxDECLARE_EVENT`/
`wxDEFINE_EVENT` (`events.h/.cpp`: `EDIT_EVENT`, `NO_ELEMENT_SELECTED`,
`CHILD_WINDOW_IN_FOCUS_EVENT`, `NEW_EVENT` — note `NEW_EVENT` is declared
but never `wxDEFINE_EVENT`'d, so it would fail to link if ever actually
posted/bound; check current usage before relying on it) — port these to
Qt signals on whatever object logically owns them, or to custom `QEvent`
subclasses if they need to travel through `QCoreApplication::postEvent`
rather than a direct signal/slot connection.

`duoplot_ids::DuoplotIds` (also `events.h`) is a `wxID_HIGHEST`-based enum
of menu/control IDs — Qt's `QAction`-based menu model doesn't need
integer IDs at all (connect each `QAction`'s `triggered` signal directly),
so this enum likely disappears rather than needing a direct port.

## Miscellaneous wx-adjacent utility

- `RGBTripletfToWxColour` (declared in `tab_button.h`, used throughout) —
  the one conversion function bridging this codebase's own `RGBTripletf`
  (see `misc/`'s docs) to `wxColour`. Its Qt equivalent is a trivial
  `RGBTripletf` → `QColor` function, needed wherever this is called.
- `wxSetCursor(wxCursor(wxCURSOR_*))` calls throughout `gui_element.cpp`
  and the custom chrome widgets — Qt equivalent is
  `setCursor(Qt::SizeHorCursor)` etc. (called on the widget rather than
  globally, which is arguably a cleaner model already).
