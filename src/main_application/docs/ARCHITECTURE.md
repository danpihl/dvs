# `src/main_application/` (top-level files) — architecture overview

> **Updated after a `git pull` (main @ `39a6dc9e`, previously documented at
> `23e65b7a`).** See "The `duoplot` → LumosAlgo dependency swap" below for
> the single biggest change in this pull, and "Changes since the last pass"
> at the end of each section for what else moved. Nothing in this update
> required re-deriving the architecture from scratch — the containment
> hierarchy, custom-chrome window design, and two wx-usage patterns
> documented below are all still accurate.

This covers the ~65 `.h`/`.cpp` files directly under `src/main_application/`
(not its documented subdirectories — `axes/`, `communication/`, `misc/`,
`opengl_low_level/`, `plot_objects/`, `project_state/`, `serial_interface/`,
`text_stream_objects/`). These files are the **application shell itself**:
the wx-based window/tab/element hierarchy, app bootstrap, and the glue
connecting the TCP/serial data pipelines to on-screen rendering. This is by
far the largest and most wxWidgets-dependent part of the codebase — see
[`WX_WIDGET_INVENTORY.md`](WX_WIDGET_INVENTORY.md) for a dedicated
per-class migration worksheet.

## Containment hierarchy

```
MainApp (main.cpp, : wxApp)
  └── MainWindow (: wxFrame, wxNO_BORDER — the whole app's control root)
        ├── DataReceiver, SerialInterface     (documented modules; owned here)
        ├── SaveManager, ConfigurationAgent   (documented in project_state/)
        ├── CmdlOutputWindow, TopicTextOutputWindow  (log/debug wxFrames)
        ├── CustomTaskBarIcon                 (system tray / menu bar)
        └── vector<GuiWindow*> windows_
              └── GuiWindow (: wxFrame — one per user-created top-level window)
                    ├── TabButtons / vector<TabButton*>
                    └── vector<WindowTab*> tabs_
                          └── WindowTab (plain class, not a wx type itself)
                                ├── vector<PlotPane*>                    (: wxGLCanvas + ApplicationGuiElement)
                                ├── vector<ApplicationGuiElement*>       (Button/Slider/Checkbox/... GuiElement)
                                ├── vector<ScrollingTextGuiElement*>
                                ├── ZOrderQueue                          (element stacking order)
                                └── EditingSilhouette                    (: wxPanel — layout-edit drag preview)
```

`MainWindow` is the true root of everything — it owns the data-receiving
infrastructure (documented separately) *and* the window list, and is what
every `GuiWindow`/`WindowTab`/`ApplicationGuiElement` calls back into via
`std::function` callbacks passed down through constructors (there is no
global/singleton access pattern here — every level is handed exactly the
callbacks it needs).

## Application bootstrap (`main.cpp`)

`MainApp : public wxApp`, bootstrapped via the `IMPLEMENT_APP(MainApp)`
macro — **this macro is the single most foundational wxWidgets dependency
in the entire codebase**: it generates the platform's actual `main()`/
`WinMain()` entry point. `MainApp::OnInit()` parses debug args
(`debug_value_args::parseArgs`, from `src/common/`, outside this module),
registers PNG/ICO image handlers, and constructs one `MainWindow` (stored in
a bare global `MainWindow* main_window`, not a smart pointer).
`MainApp::OnExit()` calls `main_window->destroy()`. A Qt port replaces this
whole file with a `QApplication` + `main()` and drops `IMPLEMENT_APP`
entirely — it has no Qt equivalent macro to swap in for.

## `MainWindow` is a custom-chrome, borderless window

`MainWindow`'s `wxFrame` base is constructed with `wxNO_BORDER` — **the
application draws its own window chrome** rather than using the OS title
bar:
- `close_button_`/`minimize_button_` are `CloseButton` instances (a
  hand-painted `wxPanel`, see below) with lambda-supplied icon-drawing
  functions (an X and a underscore, drawn with raw `wxPaintDC::DrawLine`
  calls) and lambda callbacks (`destroy()` / `Iconize()`).
- Dragging the window is implemented manually: `mouseLeftPressed` only
  arms a drag if the click lands within `kMainWindowTopMargin` (the
  hand-drawn "title bar" strip), and `mouseMoved` repositions the whole
  `wxFrame` by the mouse delta while the button is held
  (`main_window.cpp`'s `mouseLeftPressed`/`mouseMoved`/`mouseLeftReleased`).
- Because there's no native per-window title bar, the app instead calls
  `wxMenuBar::MacSetCommonMenuBar(menu_bar_)` on macOS to install one shared
  system menu bar at the top of the screen.

**A Qt port of this window needs `Qt::FramelessWindowHint` (or
`Qt::Window | Qt::CustomizeWindowHint`) plus the same manual
press/move/release drag logic** — Qt doesn't do this for you either, but
the pattern translates directly (`mousePressEvent`/`mouseMoveEvent` on the
window, checking `event->pos().y() < margin`).

**Platform-specific startup difference found**: `MainWindow`'s constructor
only calls `Show()` when `PLATFORM_APPLE_M` is *not* defined
(`#ifndef PLATFORM_APPLE_M ... Show(); #endif`) — on macOS builds the main
window is never explicitly shown at startup by this line; whatever makes it
visible on Mac (if anything currently does) is a separate mechanism, likely
the tray icon. Worth confirming this is intentional before treating it as
just "how it's always worked."

## The receive loop and its ~100 Hz heartbeat

`MainWindow` spawns the TCP receive background thread
(`tcp_receive_thread_`, documented in `communication/`'s docs) and starts
`receive_timer_` — a `wxTimer` whose period is `visualization_period_ms`,
read from `ConfigurationAgent` (key `"visualization_period_ms"`, clamped to
`[1, 100]`, **defaulting to 10 ms — i.e. a 100 Hz GUI-thread tick**) —
`OnReceiveTimer` is the single recurring entry point that:
1. Drains queued TCP-derived `InputData` (per target element name) into the
   matching `PlotPane`/GUI element — see `communication/`'s and
   `plot_objects/`'s docs for the thread-boundary details this crosses.
2. Drains serial-derived `objects::BaseObject` values
   (`handleSerialData()`, reading `SerialInterface::extractRawDataFrames()`
   — see `serial_interface/`'s docs) and routes them by `TopicId` to
   subscribed `PlotPane`s (`pushStreamData`) or `ScrollingTextGuiElement`s
   (`pushNewText`).

`refresh_timer_` is declared and a member of `MainWindow`, but its
`Bind(wxEVT_TIMER, ...)` call is commented out (`// TODO: Remove?`) — it is
never started and never fires; treat it as dead weight, not a second timer
with real effect.

## `ApplicationGuiElement` — the shared "placeable thing on a tab" interface

Every widget a user can drop onto a tab (`PlotPane` and every
`*GuiElement` in `gui_elements.h`) derives from `ApplicationGuiElement`
(`gui_element.h/.cpp`) *in addition to* its native wx base class (multiple
inheritance — `ApplicationGuiElement` has no wx base of its own, so there's
no diamond). It provides two things every placeable element needs:

1. **Layout editing.** Holding the Command/Ctrl key and clicking near an
   element's edge (`getCursorSquareState` — an 8-way hit-test against the
   element's bounds plus a margin) enters resize mode; dragging then calls
   `adjustPaneSizeOnMouseMoved()`, which computes a new position/size,
   clamps it to the parent's bounds and a 10px minimum, and stores the
   result back into `ElementSettings` as **parent-relative normalized
   `[0,1]` coordinates** (`element_settings_->x/y/width/height` — matching
   exactly the clamping documented in `project_state/`'s `ElementSettings`).
   `setElementPositionAndSize()` is the inverse: normalized settings →
   actual `wxPoint`/`wxSize` against the current parent size, called
   whenever the parent is resized (`updateSizeFromParent`).
2. **The GUI-feedback wire channel.** `sendGuiData()` serializes the
   element's `ElementSettings::type`, handle string, and a per-type payload
   (`getGuiPayloadSize()`/`fillGuiPayload()`, both pure virtual) and calls
   `sendThroughTcpInterface(..., kGuiTcpPortNum)` — **this is the exact
   server-side counterpart of the GUI callback channel documented in
   `src/interfaces/cpp/duoplot/docs/PROTOCOL.md`** (port 9758, where the
   *client* library binds and listens). `GuiElementState`
   (`gui_element_state.h`, one subclass per widget type — `SliderState`,
   `ButtonState`, `CheckboxState`, etc.) is the payload-serialization
   counterpart of the interface library's `*Internal` classes
   (`gui_internal.h`) — **see the cross-module protocol mismatch noted in
   "Known rough edges" below.**

## Two distinct wxWidgets usage patterns

This codebase uses wx in two structurally different ways — recognizing
which pattern a given class follows determines how much work its Qt port
is:

1. **Thin wrap of a native wx control** (`gui_elements.h`): `ButtonGuiElement
   : public wxButton, public ApplicationGuiElement`, and likewise
   `SliderGuiElement : wxSlider`, `CheckboxGuiElement : wxCheckBox`,
   `TextLabelGuiElement : wxStaticText`, `ListBoxGuiElement : wxListBox`,
   `EditableTextGuiElement : wxTextCtrl`, `DropdownMenuGuiElement :
   wxComboBox`, `RadioButtonGroupGuiElement : wxRadioBox`,
   `ScrollingTextGuiElement : wxTextCtrl`. Each just delegates
   `getPosition/getSize/setPosition/setSize/hide/show` straight to the
   native widget's own methods. **These map close to 1:1 onto Qt widgets**
   (`QPushButton`, `QSlider`, `QCheckBox`, `QLabel`, `QListWidget`,
   `QLineEdit`/`QTextEdit`, `QComboBox`, a `QGroupBox`+`QRadioButton` set,
   `QPlainTextEdit`) — see the inventory doc for the full table.
2. **Fully hand-painted custom chrome** (`TabButton`, `WindowButton`,
   `CloseButton`, `CustomButton`, `HelpPane`, `EditingSilhouette`): each is
   a bare `wxPanel` subclass that binds `wxEVT_PAINT` to its own
   `OnPaint()`, draws everything itself with a `wxPaintDC` (rectangles,
   rounded rectangles, lines, text), and drives hover/press color
   animations with one or more `wxTimer`s ticking a color-interpolation
   loop (`OnEnteredTimer`/`OnExitedTimer`/`OnClickedTimer` patterns,
   near-identical across `TabButton`/`WindowButton`/`CustomButton`).
   **These have no stock Qt equivalent to swap in** — each needs a
   `QWidget` subclass overriding `paintEvent()` with `QPainter`, plus
   `QTimer`-driven animation reimplemented from scratch. This is the
   costlier half of the wx→Qt UI migration, concentrated in a handful of
   files but repeated with near-identical logic in each one — worth
   factoring into one reusable custom-painted-button base class during the
   port instead of porting each copy independently.

## Plot rendering integration (ties together already-documented modules)

`PlotPane` (`plot_pane.h/.cpp`) is a `wxGLCanvas` + `ApplicationGuiElement`
that owns one `AxesInteractor`/`AxesRenderer` pair (see `axes/`'s docs) and
one `PlotDataHandler*` (`plot_data_handler.h/.cpp`) — the class that owns
the `vector<PlotObjectBase*>` for that pane (see `plot_objects/`'s docs) and
a `ColorPicker`. `PlotPane` also owns a `PointSelection` instance
(`point_selection.h/.cpp` — a thin holder of per-pane plot data plus
`getClosestPoint`, the CPU-side point-picking dispatcher that calls into
whichever `ConvertedDataBase::getClosestPoint` override a given plot object
provides — see `plot_objects/`'s `CAPABILITY_MATRIX.md`). `PlotPane` also
directly owns serial-stream subscriptions (`subscribed_streams_: map<TopicId,
StreamObjectBase*>`, `new_objects_`), independent of the TCP-driven
`PlotDataHandler` path — the two data sources (TCP client protocol, serial
device stream) are rendered side by side in the same pane but through
entirely separate object hierarchies (see `plot_objects/`'s docs on the
stream-object hierarchy).

`InputData` (`input_data.h/.cpp`) is the small carrier struct that bundles
a `ReceivedData` + `PlotObjectAttributes` + `UserSuppliedProperties` +
optional `ConvertedDataBase` together as they move from the TCP receive
thread's queue into `PlotPane`'s processing on the GUI thread.

`UserSuppliedProperties` (`user_supplied_properties.h/.cpp`) is the parsed,
resolved property bag built once per received message
(`templateToPropertyType<T>()` maps a C++ property struct type to the wire
`PropertyType` enum, mirroring — but independently implemented from — the
interface library's own property-type tables). Every field is an
`OptionalParameter<T>` (`{bool has_default_value; T data;}`) so
`PlotObjectBase::assignProperties`/`updateProperties` (see `plot_objects/`'s
docs) can distinguish "client explicitly set this" from "using the
built-in default."

`shader.h/.cpp` (pure OpenGL, no wx) defines `ShaderCollection` and every
concrete shader wrapper (`Plot2DShader`, `Plot3DShader`, `DrawMeshShader`,
`ImShowShader`, `ScatterShader`, `TextShader`) — shared by `axes/`,
`plot_objects/`, and (nominally) `BackgroundRenderer`.

## Known rough edges

- **A hardcoded, developer-specific serial port path ships in
  `MainWindow`'s constructor**: `serial_interface_{"/dev/tty.usbmodem142102",
  115200}` — a literal macOS USB-serial device path for one specific
  microcontroller, baked directly into the app's startup code rather than
  read from configuration. On any machine without that exact device
  enumerated at that exact path, `SerialInterface`/`SerialPort` silently
  fail to open (per `serial_interface/`'s docs — `isValid()` becomes
  `false`, `start()` logs an error and does nothing further), so this
  doesn't crash, but it means the serial-streaming feature is effectively
  non-functional out of the box on any machine other than the original
  developer's. Combined with `serial_interface/`'s own macOS-only-headers
  issue (see that module's docs), the whole serial pipeline looks like it
  was wired up for one local development setup and never generalized.
- **`platform_paths.cpp`'s `getConfigDirRoot()`/`getApplicationRootPath()`
  are macOS-only but not guarded by `#ifdef PLATFORM_APPLE_M`** (only
  `getExecutablePath()` is platform-guarded in that file) — `getConfigDirRoot()`
  hardcodes `"/Users/" + getUsername() + "/Library/Preferences"`, and
  `getApplicationRootPath()` calls `CFBundleGetMainBundle()` (Core
  Foundation, only declared when `PLATFORM_APPLE_M`'s `#include
  <CoreFoundation/CoreFoundation.h>` is active). **This is the same class of
  bug found in `serial_interface/serial_port.h` — unconditional macOS-only
  code with no platform guard** — and it directly backs `ConfigurationAgent`
  (see `project_state/`'s docs), so a Linux build of this file would very
  likely fail to compile. This is now the *second* confirmed instance of
  this exact pattern; worth a project-wide grep for `CFBundle`/`CoreFoundation`/
  `mach/` includes outside `#ifdef PLATFORM_APPLE_M` blocks before assuming
  any given file is portable.
- **`BackgroundRenderer` (`background_renderer.h/.cpp`) is dead code** —
  confirmed by search, it is never instantiated anywhere. Its shader
  initialization is mostly commented out (only `window_background_shader`
  is actually loaded), `setTabContent()`'s body is empty, and its
  `render()` prints a stray debug string ("Rendier", a typo) to stdout on
  every call. Looks like an abandoned attempt at a themed window
  background, safe to treat as a deletion candidate pending confirmation.
- **`cmdl_output_window.h/.cpp` and `topic_text_output_window.h/.cpp` are
  near-identical duplicate files** — same `ColorToWxColour` free function
  (defined twice, once at file scope and once inside an anonymous
  namespace — the only difference between the two copies), same `wxFrame`+
  `wxTextCtrl` structure, same constructor logic. Likely one was
  copy-pasted from the other for a second log window; a shared base class
  would remove the duplication.
- **`CustomTaskBarIcon` is a complete no-op on macOS** (`tray_icon.h`'s
  `#ifdef PLATFORM_APPLE_M` branch defines a class with every method body
  empty, including `SetIcon` which just returns `true` unconditionally) —
  the system tray icon, its popup menu, and every menu callback
  (file new/open/save/save-as, preferences, per-window toggle) **silently
  do nothing on macOS builds**. This looks like an intentional stand-in
  (macOS menu-bar icon integration is more involved than
  `wxTaskBarIcon` handles well) rather than a bug, but it means macOS users
  lose an entire menu surface that Linux/Windows users have — worth
  confirming what (if anything) replaces this functionality on Mac before
  assuming it's covered elsewhere.
- **Slider `is_horizontal` wire-format mismatch between client and
  server**: `SliderState::serializeToBuffer` (this module,
  `gui_element_state.h`) writes 4 `int32_t` fields *plus* a trailing
  `uint8_t is_horizontal` byte (17 bytes total) over the GUI callback
  channel, but the client-side interface library's `SliderInternal::updateState`
  (`src/interfaces/cpp/duoplot/gui_internal.h`, documented in that
  module's docs) only ever reads the first 4 `int32_t` fields (16 bytes) —
  it has no code path that reads a 5th field. This lines up exactly with
  `project_state/`'s finding that `SliderSettings`' vertical/horizontal
  support is fully commented out on the settings-persistence side too:
  vertical sliders were seemingly begun on the server side (both here and
  in project settings) but never completed end-to-end, and the client
  library was never updated to match.
- **`tray_icon.cpp` uses the older static `wxBEGIN_EVENT_TABLE` macro
  instead of `Bind()`** — every other event-handling file in this module
  uses `Bind(wxEVT_*, ...)` (the modern wx idiom); `tray_icon.cpp` is the
  one file still using the older table-based style. Not a bug, but an
  inconsistency worth normalizing if this code is touched again before a
  Qt port (both styles map to Qt signal/slot `connect()` calls regardless).

## The `duoplot` → LumosAlgo dependency swap (the headline change in this pull)

**`main_application` no longer builds against `src/interfaces/cpp/duoplot/`
at all.** Its `CMakeLists.txt` dropped
`include_directories(${REPO_DIR}/src/interfaces/cpp)` and added
`include_directories(${REPO_DIR}/third_party/LumosAlgo/src)` instead — a
**git submodule** (`third_party/LumosAlgo`, `git@github.com:LumosRobotics/LumosAlgo.git`,
not checked out by default; populate it with
`git submodule update --init third_party/LumosAlgo` if you need to read its
source, as this update did). Every file that used to
`#include "duoplot/..."` and write `duoplot::`/`DUOPLOT_LOG_*`/
`DUOPLOT_ASSERT` now includes `"lumos/..."` and writes `lumos::`/
`LUMOS_LOG_*`/`LUMOS_ASSERT` instead — this touched the large majority of
files in `main_application` (and its `axes/`, `communication/`, `misc/`,
`opengl_low_level/`, `plot_objects/`, `project_state/` subdirectories,
already reflected in their own doc updates).

**As of the commit this was checked against, it is a mechanical rename, not
a protocol redesign**: `third_party/LumosAlgo/src/lumos/plotting/` is a
near-byte-for-byte copy of `src/interfaces/cpp/duoplot/` with `duoplot`→
`lumos` and `duoplot::internal`→`lumos::internal` substituted throughout
(verified by diffing `enumerations.h` between the two — identical enum
values, same port numbers, same magic number, same struct layouts). So
everything in `src/interfaces/cpp/duoplot/docs/PROTOCOL.md` (wire format,
enums, framing) still accurately describes what `main_application` speaks
today.

**The risk this creates going forward**: there are now **two independently
tracked copies** of the same client/server protocol library —
`src/interfaces/cpp/duoplot/` (still used by `src/test/CMakeLists.txt` and
presumably the C++ demo clients) and `third_party/LumosAlgo/src/lumos/plotting/`
(what `main_application` now actually builds against). Nothing enforces
these two stay wire-compatible; a future change to either one's struct
layouts or enum values without the same change in the other would silently
break client/server compatibility with no compile-time signal (the two are
different C++ types in different namespaces — there is no shared header, no
version check). Anyone editing the wire protocol should check whether the
matching edit needs making in both places, and this repo would benefit from
either fully retiring `src/interfaces/cpp/duoplot/` in favor of LumosAlgo,
or documenting which one is now the actual source of truth.

The math library moved the same way: `duoplot::math`/`duoplot::Vec3d`/
`duoplot::Matrix` etc. (from `src/interfaces/cpp/duoplot/math/`) are now
`lumos::`-namespaced types from `third_party/LumosAlgo/src/lumos/math/`
(`#include "lumos/math.h"`). One associated rename to know about:
**`MatrixFixed<T, Rows, Cols>` is now `FixedSizeMatrix<T, Rows, Cols>`** —
if you see `MatrixFixed` in an *older* doc snippet (or in
`src/interfaces/cpp/duoplot/`, which still uses the old name), that's the
pre-swap name; `main_application` code now says `FixedSizeMatrix`.

## Other confirmed changes since the last pass

- **Two previously-documented portability bugs are now fixed.** `serial_port.h`/`.cpp`'s
  `<mach/clock.h>`/`<mach/mach.h>` includes are now correctly wrapped in
  `#ifdef PLATFORM_APPLE_M` (see `serial_interface/`'s docs for the
  before/after). `platform_paths.cpp`'s `getConfigDirRoot()` is now
  properly split per-platform (`PLATFORM_APPLE_M`: the same
  `/Users/<user>/Library/Preferences` as before; new
  `PLATFORM_LINUX_M` branch: `$XDG_CONFIG_HOME` or `~/.config`), and
  `getApplicationRootPath()`'s `CFBundleGetMainBundle()` call is now
  correctly guarded by `#ifdef PLATFORM_APPLE_M`/`#endif` — this file
  should now actually compile on Linux, which it likely did not before.
- **A new plot type, `ScreenSpacePrimitive`, was added** — see
  `plot_objects/`'s docs (new `Function::SCREEN_SPACE_PRIMITIVE`, renders
  arbitrary 2D triangles directly in normalized screen space, bypassing the
  axes/camera transform entirely — apparently intended for UI-space
  overlays rather than data plotting).
- **A new, apparently-unfinished dead file pair was added**:
  `graphic_window.h`/`.cpp`, containing `ShapedFrame` — a near-verbatim copy
  of wxWidgets' own "shaped frame" sample code, demonstrating an
  irregularly-shaped (star-bitmap-masked), borderless, always-on-top,
  no-taskbar `wxFrame` via `wxRegion`/`SetShape()`. **It is compiled (added
  to `CMakeLists.txt`) but never instantiated anywhere** — confirmed by
  search. It also hardcodes an absolute path to a *different* developer's
  home directory (`"/Users/danielpi/work/dvs/src/main_application/star.png"`
  — note "danielpi", not the current user), so even if something did
  instantiate it, loading the bitmap would fail on any machine but that
  one's. Treat this exactly like `BackgroundRenderer`
  (documented earlier as dead scaffolding) — experimental, unwired, not a
  template to build on without cleanup. `star.png` (the bitmap it would
  have loaded) was added alongside it at `src/main_application/star.png`.
- **`UserSuppliedProperties` (`user_supplied_properties.h`) was rewritten to
  use real `std::optional<T>` fields** instead of the old hand-rolled
  `OptionalParameter<T>{bool has_default_value; T data;}` struct — same
  intent (distinguish "client explicitly set this" from "using the
  built-in default"), now spelled `has_value()`/`.value()`/`.value_or(default)`
  instead of `!has_default_value`/`.data`. `PlotObjectBase::assignProperties`
  was also renamed to `initializeProperties` (same role: called once from
  the constructor). **This rewrite introduced a real regression** — see
  `plot_objects/`'s docs' rough-edges section: the line-style
  initialization block was commented out of the new `initializeProperties`,
  so `has_line_style_`/`line_style_` are now left **uninitialized** for
  every freshly-constructed plot object.
- `MainWindow::setCurrentElement` was renamed to `setActiveView` (same
  signature and behavior — cosmetic).
- The now-empty `src/main_application/old/` directory (legacy dead code
  from before this doc set existed, never itself documented) was deleted
  entirely in this pull.

See [`WX_WIDGET_INVENTORY.md`](WX_WIDGET_INVENTORY.md) for the full
wx-class-by-class migration worksheet, and
[`FILE_REFERENCE.md`](FILE_REFERENCE.md) for a per-file breakdown of all 65
files.
