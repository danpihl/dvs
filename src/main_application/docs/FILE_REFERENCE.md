# File-by-file reference — `src/main_application/` (top-level files)

> **Updated for the post-`39a6dc9e` pull** — see `ARCHITECTURE.md`'s
> "LumosAlgo dependency swap" section before reading code snippets in this
> file: every `duoplot::`/`#include "duoplot/..."` reference described
> below now reads `lumos::`/`#include "lumos/..."` in the actual source.

Grouped by role rather than alphabetically, since this is a flat directory
of ~65 files serving very different purposes. See
[`ARCHITECTURE.md`](ARCHITECTURE.md) for the narrative and
[`WX_WIDGET_INVENTORY.md`](WX_WIDGET_INVENTORY.md) for the wx→Qt migration
angle — this document fills in per-file detail those two don't cover.

## Bootstrap

- **`main.cpp`** — `MainApp : wxApp`, `IMPLEMENT_APP(MainApp)`. See
  `ARCHITECTURE.md`.
- **`globals.h`/`.cpp`** — one file-scope `std::atomic<int>
  current_unused_element_idx`, used elsewhere for generating unique
  default element names. Trivial.
- **`events.h`/`.cpp`** — custom wx event tags and the `duoplot_ids::DuoplotIds`
  menu/control ID enum. See `WX_WIDGET_INVENTORY.md`'s note on `NEW_EVENT`
  being declared but never defined.
- **`constants.h`** — a handful of `constexpr int` layout constants for the
  main window and its window-switcher buttons (margins, heights).

## Platform/portability shims

- **`filesystem.h`** — `duoplot::filesystem` namespace alias:
  `std::experimental::filesystem` on Linux, `std::filesystem` on Apple.
  Properly platform-guarded.
- **`platform_paths.h`/`.cpp`** — `getResourcesPathString()`,
  `getConfigDir()`. **Contains the second confirmed unconditional
  macOS-only code path in this codebase** — see `ARCHITECTURE.md`'s rough
  edges. Only `getExecutablePath()` is actually split by
  `#ifdef PLATFORM_LINUX_M`/`PLATFORM_APPLE_M`; `getConfigDirRoot()`
  (hardcoded `/Users/<user>/Library/Preferences`), `getApplicationRootPath()`
  (`CFBundleGetMainBundle`), and the public `getConfigDir()`/
  `getResourcesPathString()` that depend on them are not. Also note
  `getResourcesPathString()` currently just returns the literal
  `"../resources/"` — the "real" bundle-relative implementation
  (`getApplicationRootPath()`-based) is written but not actually wired up
  (dead code, commented out at the call site).
- **`opengl_debug.h`/`.cpp`** — `opengl_debug::begin()`/`end()`/
  `getSamplesPassed()`, a minimal `GL_SAMPLES_PASSED` occlusion-query
  wrapper. Pure OpenGL, no wx. Check current callers before assuming it's
  wired into any active render path — it reads as a standalone debugging
  utility.
- **`color.h`** — `enum class Color_t` (RED/GREEN/BLUE/... — the small
  fixed palette used for colored log-window text; see
  `cmdl_output_window.cpp`/`topic_text_output_window.cpp`'s
  `ColorToWxColour`). Unrelated to `misc/rgb_triplet.h`'s `RGBTriplet`.

## Colors

- **`color_picker.h`/`.cpp`** — `ColorPicker`, the automatic per-series
  color-cycling helper consumed by `plot_objects/`'s `PlotObjectBase`
  (documented there) and independently by `PlotDataHandler`. Backed by a
  fixed 6-color palette (`colors[]` in the `.cpp`). Three independent
  rotating indices (`color_idx_`/`edge_color_idx_`/`face_color_idx_`) —
  note `getNextFaceColor()` actually increments/reads `edge_color_idx_`
  and `getNextEdgeColor()` reads/increments `face_color_idx_` (the two are
  swapped relative to their names); since both cycle through the identical
  palette array this has no visible effect today, but is a landmine if the
  two colors are ever given different palettes.

## Data-shape carriers (protocol ↔ rendering glue)

- **`input_data.h`/`.cpp`** — `InputData`, documented in `ARCHITECTURE.md`.
  Move-only-by-convention carrier (methods are literally named
  `moveAllData()`/`moveAllDataButConvertedData()`, returning `std::tuple`s)
  for shuttling one received message's full context from the TCP receive
  thread's queue to GUI-thread processing.
- **`user_supplied_properties.h`/`.cpp`** — `UserSuppliedProperties`,
  documented in `ARCHITECTURE.md`. Every property field is now a real
  `std::optional<T>` (post-pull rewrite — see `ARCHITECTURE.md`'s "Other
  confirmed changes" section; previously a hand-rolled `OptionalParameter<T>`
  struct). One inconsistency worth flagging: `silhouette` is constructed
  as `std::optional<RGBTripletf> silhouette{kDefaultSilhouette}` — i.e. it
  starts with `has_value() == true` — while every sibling optional
  (`edge_color`, `face_color`, `color_map`, `color`, ...) starts as
  `std::nullopt`. Anything that checks `silhouette.has_value()` to mean
  "the client explicitly set this" will get a false positive for every
  message, regardless of whether a silhouette was actually requested. The
  `.cpp` implements the `CommunicationHeader`-parsing constructor (reads
  whichever properties are present via the header's property lookup table
  — mirrors `PlotObjectAttributes`' own header-reading pattern in
  `plot_objects/`) and `appendProperties()` (merges a second
  `UserSuppliedProperties` in, used for `PROPERTIES_EXTENSION` messages
  arriving after the initial plot call).
- **`buffered_writer.h`** — `BufferedWriter`, a minimal bounds-checked
  writer counterpart to `serial_interface/`'s `BufferedReader` (throws
  `std::runtime_error` on overflow rather than the reader's silent
  unescape-as-you-go approach). Used for building outgoing serial-publish
  payloads (`GuiElementState::serializeToSerialBuffer`,
  `ApplicationGuiElement`'s serial-publish path).
- **`outer_converter.h`** — `applyConverter<O>()`, documented in
  `plot_objects/`'s docs (the runtime-`DataType`-to-template dispatch
  helper). Lives here because it's shared by `plot_objects/` and
  `user_supplied_properties.h`, not because it belongs to this module
  specifically.

## Top-level windows

- **`main_window.h`/`.cpp`** — `MainWindow`. The largest and most central
  file in this module (1123 lines) — see `ARCHITECTURE.md` for its role as
  root orchestrator, its custom-chrome borderless window, and the
  hardcoded-serial-port rough edge. Also owns menu construction
  (`createMainMenuBar()`), save/open/new-project flows (`wxFileDialog`,
  `wxMessageBox`), and window lifecycle (`newWindow`/`deleteWindow`/
  `toggleWindowVisibility`).
- **`main_window_receive.cpp`** — the TCP-message dispatch logic
  documented in depth in `communication/`'s and `plot_objects/`'s docs
  (`convertPlotObjectData()`'s `Function`-keyed switch,
  `addActionToQueue()`, `manageReceivedData()`).
- **`main_window_serial.cpp`** — the serial-frame dispatch logic
  documented in `serial_interface/`'s docs (`TopicId`/`ObjectType`
  parsing via `BufferedReader`, routing to `PlotPane::pushStreamData`/
  `ScrollingTextGuiElement::pushNewText`).
- **`gui_window.h`/`.cpp`** — `GuiWindow`, documented in `ARCHITECTURE.md`.
  The 1286-line `.cpp` is mostly wx menu/dialog boilerplate: one
  `createNew*CallbackFunction` per element type (each opens a
  `SettingsWindow` dialog to collect a handle string, then delegates to the
  matching `WindowTab::createNew*`), plus window-level operations (rename,
  resize handling, screenshot, projection-mode/mouse-interaction-mode
  toggles broadcast to the current tab).

## Tabs, elements, and layout editing

- **`gui_tab.h`/`.cpp`** — `WindowTab` and `ZOrderQueue`, documented in
  `ARCHITECTURE.md`. `ZOrderQueue` is a plain `vector<string>` of handle
  strings (not a wx type) — `raise()`/`lower()` move a handle string to the
  front/back, `getOrderOfElement()` returns its index; used purely to
  decide wx z-order (`Raise()`/`Lower()` calls) when elements overlap. The
  943-line `.cpp` is one `createNew*` factory method per element type
  (mirroring `GuiWindow`'s dispatch) plus the element-management methods
  declared in the header (delete/rename/raise/lower/find-by-handle-string).
- **`gui_element.h`/`.cpp`** — `ApplicationGuiElement`, `CursorSquareState`,
  `Bound2D`. Documented in depth in `ARCHITECTURE.md` (layout-editing
  math and the GUI-feedback wire channel).
- **`gui_element_state.h`** — `GuiElementState` and one subclass per
  widget type (`CheckboxState`, `SliderState`, `ButtonState`,
  `TextLabelState`, `ListBoxState`, `EditableTextState`,
  `DropdownMenuState`, `RadioButtonGroupState`). Pure serialization
  structs — see `ARCHITECTURE.md`'s note on the `SliderState`/
  `SliderInternal` wire-format mismatch. Also defines
  `serializeToSerialBuffer()` per type (only meaningfully implemented for
  a subset — e.g. `SliderState` writes its `value_` as a raw `int32_t`;
  check each subclass before assuming they're all wired up for the
  serial-publish path).
- **`gui_elements.h`/`.cpp`** — every concrete `*GuiElement` class, listed
  in full in `WX_WIDGET_INVENTORY.md`'s native-control-wrapper table. The
  721-line `.cpp` is mostly per-type constructors (creating the wx widget,
  binding its native change event to update internal state and call
  `sendGuiData()`) — structurally repetitive across all eight types once
  you've read one.
- **`editing_silhouette.h`/`.cpp`**, **`help_pane.h`/`.cpp`** — custom-painted
  chrome widgets, see `WX_WIDGET_INVENTORY.md`.

## Custom-painted button chrome

- **`close_button.h`/`.cpp`**, **`custom_button.h`** (header-only),
  **`tab_button.h`/`.cpp`**, **`tab_buttons.h`/`.cpp`**,
  **`window_button.h`/`.cpp`** — see `WX_WIDGET_INVENTORY.md`'s dedicated
  table; `tab_buttons.h`/`.cpp` (`TabButtons`, plural) is the container
  managing a row of `TabButton` instances for one `GuiWindow` (creation,
  selection, layout/reflow on resize, rename, delete) — not itself a wx
  type, just an orchestrator around a `vector<TabButton*>`.

## Log / debug output windows

- **`cmdl_output_window.h`/`.cpp`**, **`topic_text_output_window.h`/`.cpp`**
  — near-duplicate `wxFrame`+`wxTextCtrl` log windows, see
  `ARCHITECTURE.md`'s rough edges and `WX_WIDGET_INVENTORY.md`.
- **`settings_window.h`/`.cpp`** — `SettingsWindow`, a generic `wxDialog`
  that builds a form from a `map<string, pair<description, init_value>>`
  at construction time — used by `GuiWindow`'s `createNew*CallbackFunction`
  methods to prompt for a new element's handle string (and any other
  per-type fields) before creating it.
- **`tray_icon.h`/`.cpp`** — `CustomTaskBarIcon`, `MyMenu` (a `wxMenu`
  subclass whose destructor runs a teardown callback — an RAII-via-wx-object
  pattern for cleaning up dynamically-allocated menu IDs). See
  `ARCHITECTURE.md`'s macOS-no-op rough edge.

## Rendering glue

- **`shader.h`/`.cpp`** — `ShaderCollection` and every shader wrapper
  class. Pure OpenGL, no wx — documented in `ARCHITECTURE.md`. Shared by
  `axes/`, `plot_objects/`, and (nominally, mostly unused) `BackgroundRenderer`.
  Post-`39a6dc9e`: gained a new `screen_space_shader` member (backs the new
  `ScreenSpacePrimitive` plot type — see `plot_objects/`'s docs), while
  `text_shader` (`TextShader`) is now unused dead weight — `axes/`'s
  text-rendering rewrite moved to its own self-contained shader pipeline
  outside `ShaderCollection` entirely (see `axes/`'s docs).
- **`plot_pane.h`/`.cpp`** — `PlotPane`, documented in `ARCHITECTURE.md`
  and `WX_WIDGET_INVENTORY.md`. The 972-line `.cpp` bridges: wx GL canvas
  setup/context management, the `axes/`-documented per-frame render
  lifecycle, `plot_objects/`-documented data ingestion
  (`processActionQueue`/`addPlotData`), and `serial_interface/`-documented
  stream subscription handling (`initSubscribedStreams`, `pushStreamData`)
  — genuinely the single busiest integration point in the whole
  application, touching every other module document written so far.
- **`plot_data_handler.h`/`.cpp`** — `PlotDataHandler`, documented in
  `ARCHITECTURE.md` and extensively in `plot_objects/`'s docs (it's the
  class that actually calls `new Plot2D(...)` etc. on the GUI thread).
- **`point_selection.h`/`.cpp`** — `PointSelection`, documented in
  `ARCHITECTURE.md`. Small holder class: per-pane list of
  `(header, attributes, properties, converted_data)` tuples plus
  `getClosestPoint()`, which linearly scans every held object's
  `ConvertedDataBase::getClosestPoint()` (see `plot_objects/`'s
  `CAPABILITY_MATRIX.md` for which types actually implement that
  meaningfully) and returns the single closest hit across the whole pane.
  `softClear()`/`clear()` implement the same
  persistent-vs-non-persistent-object split documented for
  `PlotDataHandler` in `plot_objects/`'s docs.
- **`background_renderer.h`/`.cpp`** — dead code, see `ARCHITECTURE.md`.
  Not instantiated anywhere; do not treat as a template for new
  `wxGLCanvas`-based widgets without first confirming it even compiles
  cleanly against the current `ShaderCollection`/shader-file layout (its
  shader-loading code is mostly commented out).
- **`graphic_window.h`/`.cpp`** — `ShapedFrame`, a second piece of dead
  scaffolding added in the most recent pull (see `ARCHITECTURE.md`'s
  "Other confirmed changes" section) — a near-verbatim copy of wxWidgets'
  own shaped-window sample, never instantiated, with a hardcoded path to a
  different developer's home directory. Do not build on this without
  cleaning it up first; treat it the same as `background_renderer.h`/`.cpp`.
