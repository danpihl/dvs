# new_qt_application — current state

Single status file for this effort, edited in place. Only add a line here
once it has actually been built and run/verified — "compiles" is not
"done" (see the plan at
`/Users/daniel/.claude/plans/fancy-nibbling-hanrahan.md`). For lessons
learned from the abandoned first attempt (`src/qt_application`), see
`AUDIT_OF_PRIOR_ATTEMPT.md` in this directory — that file is historical
record and is not updated further as this effort proceeds.

## Done and verified

- **Framework-independent core, fresh-copied from `main_application`.**
  Copied `axes/`, `communication/`, `misc/`, `opengl_low_level/`,
  `plot_objects/`, `project_state/`, `serial_interface/`, plus the
  top-level files confirmed to have zero wxWidgets dependency
  (`color_picker`, `constants.h`, `globals`, `gui_element_state.h`,
  `input_data`, `opengl_debug`, `outer_converter.h`, `platform_paths`,
  `plot_data_handler`, `point_selection`, `shader`,
  `user_supplied_properties`, `buffered_writer.h`, `color.h`,
  `filesystem.h`) directly from `src/main_application`, not from
  `src/qt_application`'s (stale) copies. Confirmed via `grep` across every
  file for wx includes/types before copying — none found in the copied
  set.
  - Diffed every copied file against `src/qt_application`'s version of the
    same path: all differences are attributable to expected staleness in
    the old attempt — `lumos/math/math.h` → `lumos/math.h` path drift from
    LumosAlgo's own internal reorg, the `PLATFORM_APPLE_M` serial-port fix
    landing in `main_application` after `qt_application`'s copy was made,
    and the entire HarfBuzz-based `axes/text_rendering_impl/` rewrite not
    existing yet when `qt_application`'s `axes/` copy was made. No
    unexplained drift found.
  - Builds cleanly as a static library (`new_qt_duoplot_core`,
    `src/new_qt_application/CMakeLists.txt`) via
    `make new_qt_duoplot_core -j5` from `src/build/` — only the same
    benign unused-parameter warnings already present in
    `main_application`/`qt_application`. This validates the copy is
    internally consistent, nothing more — no GUI code exists yet, so this
    proves nothing about the actual Qt port.

- **`GuiElement` (Phase 2, faithful port of `main_application/gui_element.{h,cpp}`'s
  `ApplicationGuiElement`).** Plain (non-`QObject`) mixin class, matching
  wx's multiple-inheritance pattern (`class ButtonGuiElement : public
  QPushButton, public GuiElement`, not yet written). Ports the full
  cursor-square-state edit-mode mouse interaction (click-drag resize/move
  from any edge/corner, `Cmd`-hover cursor preview) that the old
  `qt_application` attempt dropped entirely (its `ApplicationGuiElement`
  has no mouse handling at all — see `AUDIT_OF_PRIOR_ATTEMPT.md` addendum
  below). Builds cleanly, linked into `new_qt_duoplot_core`
  (`make new_qt_duoplot_core -j5`).
  - **Geometry formula verified against the wx original exactly**
    (`gui_element.cpp:429-446`, `main_application/gui_element.cpp:429-446`):
    element position/size is not a simple fraction × parent-size — wx
    reserves `minimum_x_pos_` (0 or 70px, set via `setMinXPos` depending on
    tab count) off the *left* edge for the tab-select button strip, and
    normalizes each element's stored fraction against `(parent size − that
    margin)`, offsetting the final pixel position by the same margin. This
    is baked into every existing `.duoplot` project file's stored
    coordinates, so it's reproduced exactly, including the same formula
    being used identically by every element type's initial construction
    and by mouse-driven resize (`adjustPaneSizeOnMouseMoved`).
  - **Confirmed with the user:** the left-side vertical tab-strip layout
    (not a native top `QTabWidget`) is being kept, matching wx's spatial
    layout and matching what the old `qt_application` already built
    (`tab_button_panel_`, fixed 70px) — so this exact x-offset convention
    carries over unchanged.
  - **`minimum_y_pos_` set to 0**, not wx's 30px. wx reserves 30px of
    *content-area* height for its own hand-painted top chrome (inside a
    `wxNO_BORDER` window). With native Qt chrome, the title bar and
    `QMainWindow`'s native menu bar live outside the content area
    entirely (Qt reserves their space automatically), so there is nothing
    inside the content area that needs a matching reservation. This is a
    direct consequence of the earlier native-chrome decision, not a new
    fork — flagged here so it's visible and can be corrected if a real
    y-margin turns out to be needed once `GuiWindow` exists.
  - Qt's Ctrl/Meta modifier swap on macOS is used deliberately:
    `Qt::ControlModifier` (Cmd key) replaces wx's `WXK_COMMAND` (hover cursor
    preview), `Qt::MetaModifier` (physical Ctrl key) replaces wx's
    `WXK_CONTROL` (actual click-drag-resize trigger) — same physical keys,
    not just same-named Qt enumerators. Not yet runtime-verified (needs
    `GuiWindow`/`PlotPane` to exist to test against
    `cpp object_transform all`).

- **`PlotPane` (Phase 2, faithful port of `main_application/plot_pane.{h,cpp}`).**
  `QOpenGLWidget` + `GuiElement` mixin. Builds cleanly, linked into
  `new_qt_duoplot_core`. Reuses the GL rendering pipeline
  (`initializeGL`/`resizeGL`/`paintGL`/`initShaders`/`processActionQueue`/
  `clearPane`) essentially as `qt_application`'s version had it — that part
  was already a reasonable port and the audit's hands-on testing showed
  real rendering working. What's newly faithful here, ported from the wx
  original rather than kept from `qt_application`:
  - Mouse events routed through `GuiElement`'s edit-mode dispatch first
    (`mouseLeftPressed`/`mouseMovedOverItem`/`mouseLeftReleased`,
    `mouseRightPressed`/`mouseRightReleased`), falling through to camera
    rotate/pan/zoom only when not resizing — `qt_application`'s version
    wired Qt's raw mouse events directly to camera control and had no
    edit-mode path at all.
  - Shift-modifier camera-interaction overrides (shift+left = rotate,
    shift+middle = pan, shift+right = zoom) and middle-button pan, matching
    wx's `mouseMiddlePressed`/`mouseMiddleReleased`/
    `mouseRightPressedGuiElementSpecific` — absent from `qt_application`.
  - Keyboard-driven interaction-axis constraints (`1`/`2`/`3` keys limit
    drag to X/Y/Z or a plane) and `l`+drag axes-box scaling
    (`keyPressedElementSpecific`/`keyReleasedElementSpecific`/
    `mouseMovedGuiElementSpecific`) — not present in `qt_application` at
    all. Implemented via a small `held_keys_` set updated from this
    widget's own `keyPressEvent`/`keyReleaseEvent`, since Qt has no direct
    equivalent of wx's global `wxGetKeyState()`; only accurate while this
    widget has focus, which matches actual usage (interacting with a pane
    focuses it first).
  - **Deferred, not dropped:** the wx original's topic/stream-of-strings
    subscription system (`initSubscribedStreams`/`pushStreamData`, rendered
    inline in `PlotPane::render`) is not ported yet — it's a separate
    feature tied to `MainWindow`'s serial/stream plumbing, out of scope for
    getting the windowing/layout system itself working first.
  - Not yet runtime-verified — needs `WindowTab`/`GuiWindow`/`MainWindow`
    to exist before it can actually be instantiated and shown.

- **`EditingSilhouette` (Phase 2, faithful port).** Small overlay widget
  (transparent fill, black outline) shown during edit-mode resize/move.
  Direct translation of the wx original (`QPainter` instead of `wxPaintDC`).
  Builds cleanly.

- **`WindowTab`/`ZOrderQueue` (Phase 2, faithful port of
  `main_application/gui_tab.{h,cpp}`).** Builds cleanly, linked into
  `new_qt_duoplot_core`.
  - **Plot-pane creation is fully ported and is the direct fix for both Bug
    A and Bug B** found in the old attempt (see `AUDIT_OF_PRIOR_ATTEMPT.md`):
    every creation path (menu-triggered default pane, MainWindow's
    unknown-view-name pane, project-file-loaded panes) now ends with the
    same `setMinXPos(...)` + `updateSizeFromParent(...)` pair the wx
    original always uses, so geometry is always computed by the one correct
    fraction × (parent size − margin) formula — never a raw `setGeometry()`
    using a stored fraction as if it were already pixels. The
    unknown-view-name path now matches wx exactly too: only
    handle/title are set, leaving `x/y/width/height` at `PlotPaneSettings`'s
    own sane defaults (`0, 0, 0.4, 0.4`), not an arbitrary hardcoded
    pixel-sized rectangle.
  - Z-order (raise/lower/`ZOrderQueue`), element lookup/rename/delete,
    tab-settings round-trip (`getTabSettings`), and keyboard/mouse-type
    fan-out to child elements are all ported.
  - **Deferred, clearly stubbed, not silently dropped:**
    `createNewButton`/`createNewSlider`/`createNewCheckbox`/
    `createNewTextLabel`/`createNewListBox`/`createNewEditableText`/
    `createDropdownMenu`/`createRadioButtonGroup`/`createScrollingText`
    each just log a warning — no `GuiElement` subclass exists yet for any
    of those types. This is the next chunk of work, needed before
    `c gui basic` can be used to verify anything.
  - One real Qt-specific gotcha hit and fixed: `QWidget::show()`/`hide()`
    and `GuiElement::show()`/`hide()` are both plain public members with
    the same name, so any class that inherits both (like `PlotPane`) has an
    ambiguous lookup — resolved once with an explicit override in
    `PlotPane` rather than qualifying every call site. Worth remembering
    for every future `Foo : public QSomeWidget, public GuiElement` class.

- **`GuiWindow` (Phase 2, faithful port of `main_application/gui_window.{h,cpp}`).**
  Builds cleanly, linked into `new_qt_duoplot_core`. `QMainWindow` (native
  chrome, per the earlier decision).
  - **Deliberately flat layout, matching wx, and deliberately different from
    the old attempt's architecture.** wx's `GuiWindow` has no persistent
    menu bar (only popup/context menus), so its `GetSize()` is the plain
    window content area with nothing else competing for space, and every
    child (tab buttons, every `WindowTab` element) is a direct child of the
    same `wxFrame`, manually positioned in one shared coordinate space. This
    port keeps that: `central_` has *no* `QLayout` — tab-select buttons and
    every element are direct children of `central_`, positioned absolutely.
    The old `qt_application` attempt instead split a `QHBoxLayout` into a
    fixed-width button strip plus a separate `content_area_` sub-widget —
    a genuinely different architecture that computes a different, smaller
    "available size" for elements than wx does, layered on top of the
    already-identified unit-mismatch bugs. Not reused for that reason.
  - **The other half of the Bug A fix**: `resizeEvent` updates *every* tab's
    elements on every resize, not just the currently-visible tab
    (`main_application/gui_window.cpp:498-513`) — a hidden tab must already
    have correct geometry the instant it's switched to. The old attempt's
    `resizeEvent` only updated `tabs_[current_tab_num_]`, leaving hidden
    tabs stale until `switchToTab` lazily fixed them.
  - Left tab-button strip uses plain (non-custom-painted) checkable
    `QPushButton`s, 70px wide / 30px tall each, per the user's decision to
    mimic wx's spatial layout rather than switch to a native top
    `QTabWidget`. Hidden entirely when there's only one tab, matching wx.
  - **Deferred, clearly stubbed, not silently dropped:** the full
    popup-menu system (new-element submenus, edit/delete/raise/lower
    element, edit window/tab name, interaction-mode toggles, print-gui-code)
    and `HelpPane` (out of scope per the native-chrome decision — it was a
    hand-painted overlay). `mouseRightPressed`/`notifyChildrenOnKeyPressed`
    have the same shape as wx's but skip the popup-menu/HelpPane-specific
    branches. `createNewPlotPane()`/`createNewPlotPane(handle_string)` are
    real and not deferred — `MainWindow` needs them directly, independent
    of any menu.
  - Not yet runtime-verified — needs `MainWindow` to exist before an actual
    window can be shown and driven by `system-test`.

- **`MainWindow` + `main.cpp` (Phase 2, faithful port of
  `main_application/main_window.{h,cpp}` and `main_window_receive.cpp`) —
  first runnable, verified end-to-end slice.** Builds a real executable,
  `new_qt_duoplot`. This is the milestone the whole plan has been building
  toward: a fresh, from-scratch windowing/layout implementation that
  actually launches and handles real traffic from `system_test`.
  - **Confirmed visible, per the audit finding**: `MainWindow` is a real,
    shown `QMainWindow` with one plain `QPushButton` per open `GuiWindow`
    (toggles visibility) plus a "New window" button — not the fully hidden
    router the old attempt assumed.
  - The TCP receive thread, the single `QTimer`-driven receive/dispatch
    loop, and the full plot-object `Function` → `convertRawData` switch are
    faithful ports of `main_window_receive.cpp`. Confirmed by reading the
    wx source directly: wx's `refresh_timer_` is never actually started
    (`// TODO: Remove?`, dead code) — only the receive timer runs, and
    panes redraw reactively via `PlotPane::pushQueue` calling `update()`
    when they get new data. So this port correctly uses **one** timer, not
    two, matching actual wx behavior rather than the old attempt's
    two-timer (`onReceiveTimer`/`onRefreshTimer`) design.
  - **A real bug found and fixed by hands-on testing, not by inspection**:
    the app segfaulted intermittently (not every run — classic
    uninitialized-memory symptom) shortly after `GuiWindow` construction.
    Root cause: `new_window_button_` (a raw `QPushButton*` member) was read
    by `layoutWindowButtons()` — called from `bootstrapDefaultProject()` —
    *before* the line that actually constructed it, later in the
    constructor. An uninitialized member left indeterminate, not
    default-constructed to `nullptr`, so the `!= nullptr` guard sometimes
    passed on garbage and dereferenced it. Fixed by initializing it to
    `nullptr` in the member-initializer list *and* moving its construction
    before `bootstrapDefaultProject()` is called — defense in depth against
    the same class of bug recurring as more members get added. Confirmed
    fixed by running the binary 5+ times back to back (previously crashed
    within ~1.5s most runs; now consistently reaches `MainWindow
    initialized` and stays up).
  - **Verified against real traffic**: launched `new_qt_duoplot`, ran
    `system-test cpp basic plot` against it. Result: clean client exit
    (code 0), server created a new window for the referenced view, created
    a `PlotPane`, initialized OpenGL successfully, no crash, no GL
    framebuffer errors (contrast with the old attempt's Bug B, which
    crashed OpenGL entirely in the equivalent scenario). Server log:
    `PlotPane created` → `OpenGL initialized in PlotPane` → `Created plot
    pane` → `Created new window` — the exact sequence expected.
  - The known startup view-coalescing quirk (documented in
    `AUDIT_OF_PRIOR_ATTEMPT.md` — multiple brand-new view names arriving
    faster than the receive timer drains them means only the last one gets
    a window) reproduces here too, exactly as expected, since it's a
    faithful port of the same wx logic — not a regression.
  - **Deferred, clearly stubbed, not silently dropped:** the menu bar, tray
    icon, `CmdlOutputWindow`/`TopicTextOutputWindow`, and serial data
    parsing (`SerialInterface` still starts and is polled, matching wx
    exactly including its hardcoded dev serial-port path preserved
    verbatim, but no frames are consumed yet — the consuming pipeline isn't
    ported). `SaveManager` and `QUERY_FOR_SYNC_OF_GUI_DATA` were deferred at
    the time this paragraph was written but are now implemented — see their
    own sections below/above.
  - **A second real bug found by the user's hands-on testing**: rotating a
    3D pane by dragging appeared completely frozen — the pane only visibly
    updated once the user clicked on a *different* window, at which point
    the accumulated rotation suddenly appeared. Root cause:
    `PlotPane::update()` was overridden to gate the real
    `QOpenGLWidget::update()` behind `!queued_data_.empty()`, faithfully
    matching wx's `PlotPane::update()` (which is specifically the
    new-*data*-arrived repaint path — see `main_application/plot_pane.cpp`).
    But wx's mouse/keyboard interaction handlers
    (`mouseLeftPressedGuiElementSpecific`, `mouseMovedGuiElementSpecific`,
    etc.) call `Refresh()` **directly**, bypassing that gate entirely — they
    don't go through `update()` at all. This port had called the gated
    `update()` from those same handlers instead of the real
    `QOpenGLWidget::update()`, so camera-drag interaction correctly updated
    `axes_interactor_`'s internal state but the repaint was silently
    suppressed (no queued plot data ⇒ gate never opened) until something
    unrelated — like a window focus change — forced Qt to repaint the
    dirty widget anyway, revealing the backlog of accumulated rotation all
    at once. Fixed by calling `QOpenGLWidget::update()` directly in every
    interaction handler (`wheelEvent`, `mouseMiddleReleased`,
    `mouseLeftPressedGuiElementSpecific`, `mouseLeftReleasedGuiElementSpecific`,
    `mouseRightLeftGuiElementSpecific`, `mouseMovedGuiElementSpecific`,
    `keyPressedElementSpecific`, `addPlotData`), leaving only `pushQueue`
    using the gated `update()` (renamed `repaintIfNewData()` — see the
    "Design simplifications" section below). **Confirmed fixed by the user**:
    camera rotate/pan/zoom now updates live while dragging.

  - **A third real bug, found immediately after the second, from the
    user's own diagnostic logging (not guessing)**: even with live repaints
    working, plot data would render once and then vanish permanently —
    "disappears as soon as you click the pane," not recoverable by
    dragging. Added temporary `[diag]` logging to `paintGL`/
    `mouseLeftPressedGuiElementSpecific` and asked the user to reproduce
    with a terminal attached. The log was decisive:
    `plot_datas_.size()` went `0 -> 5` (data added), then
    `pending_clear_ firing this frame` immediately after, then `5 -> 0`
    (wiped) — all *before* the next logged mouse press. So the click
    wasn't the cause; it was coincidental timing. Root cause: wx's
    original `Function::CLEAR` handling (`main_application/plot_pane.cpp`,
    inside `addSettingsData`) calls `clearPane()` **immediately,
    synchronously**, the moment `CLEAR` is popped off the queue. This
    port's `processActionQueue` (inherited from old `qt_application`'s
    code, which has the identical bug) instead set a `pending_clear_` flag
    only applied at the *start of the next* `paintGL()` call. When a
    client sends `clearView()` immediately followed by several `plot()`
    calls in the same batch (exactly what `system-test`'s `testPlot()`
    does for its `p1` view) the 5 new objects got added *first*, then
    wiped by the deferred clear on whatever the next repaint happened to
    be — which, after the second bug fix above made clicks trigger real
    repaints, was often the very next click. Fixed by calling
    `clearPane()` directly inline where `CLEAR` is encountered (matching
    wx exactly) and removing the `pending_clear_` member entirely — wx
    itself keeps a same-named flag but it's dead/vestigial there (its only
    reader is commented-out code), so removing it rather than keeping it
    unused is the more honest port. **Confirmed fixed by the user**: "now
    plot persists."
  - **Not yet verified**: visual correctness in general (needs the user's
    eyes) beyond what's been explicitly checked above,
    `c gui basic`/interactive GUI-element callbacks (blocked on the
    still-stubbed element factories in `WindowTab`), `object_transform`
    edit-mode dragging, `dynamic_plotting`/`updateable_plotting` streaming
    smoothness, and the Bug-A/B project-file-loading fix specifically
    (needs `SaveManager` or a hand-built `.duoplot` file to test against,
    since project-file loading itself is deferred). Also worth revisiting:
    `PlotPane::processActionQueue` only handles a subset of the
    `Function` values wx's `addSettingsData` does (`AXES_2D`/`AXES_3D`,
    `VIEW`, `CLEAR`, plot-data) — `SHOW_LEGEND`, `WAIT_FOR_FLUSH`,
    `SOFT_CLEAR`, `SET_TITLE`, `SET_OBJECT_TRANSFORM`, and others aren't
    wired up yet. Not a correctness bug (unhandled messages are just
    silently no-ops, not misapplied), but a real feature gap worth
    tracking before considering `PlotPane` complete.

## Design simplifications agreed with the user (2026-09-19)

Per direct discussion: fidelity to wx was costing more than it was worth in
two specific places, and the user explicitly approved relaxing it there
(old .duoplot files need a one-time re-save; interaction just needs to work
well, not match wx's exact keys/cursors). Applied across `GuiElement`,
`PlotPane`, `WindowTab`, `GuiWindow`:

- **Layout formula simplified.** Removed `minimum_x_pos_`/`minimum_y_pos_`/
  `setMinXPos`/`element_x_offset_` and the `(window_size − margin)` ratio
  math entirely. `GuiElement::setElementPositionAndSize` is now a plain
  `pos = fraction * parent_size`. The tab-button strip is back to a real
  `QHBoxLayout` in `GuiWindow` (fixed-width `tab_button_panel_` +
  expanding `content_area_`, which `WindowTab`'s elements are parented to
  and sized against) — the exact architecture originally rejected earlier
  in this session specifically because it wasn't wx-coordinate-compatible;
  now that compatibility isn't required, it's the simpler, more idiomatic
  choice and removes a whole category of margin-math bugs. Trade-off,
  accepted: existing `.duoplot` project files' stored coordinates assume
  wx's margin formula and will need one re-save under the new app once
  `SaveManager` exists.
- **Single modifier key.** Replaced the Cmd-vs-physical-Ctrl split
  (`Qt::ControlModifier` for hover-preview, `Qt::MetaModifier` for the
  actual resize-drag trigger) with `Qt::ControlModifier` for both — Qt
  already reports this as the platform's natural primary modifier (Cmd on
  macOS, Ctrl elsewhere), so this is simpler and still idiomatic per
  platform. No change to a plain click/drag with no modifier held.
- **`update()` renamed to `repaintIfNewData()`.** The frozen-interaction bug described above
  (frozen camera interaction) came directly from naming a data-gated
  method the same as `QOpenGLWidget::update()` — an easy trap in Qt
  specifically, since every future line of code reaches for `update()`
  expecting its normal meaning. Renamed so `update()` unqualified always
  means what Qt says it means; only `pushQueue` (which just pushed to the
  queue itself, so the gate is trivially satisfied) calls
  `repaintIfNewData()` explicitly.
- Rebuilt and re-ran the 5x crash-consistency check after all of the above
  — still 5/5 clean starts, no regressions from the redesign.

## Interactive GUI elements

All 8 non-plot-pane `GuiElement` subclasses are ported and wired up —
`WindowTab`'s `createNewButton`/`createNewSlider`/`createNewCheckbox`/
`createNewTextLabel`/`createNewListBox`/`createNewEditableText`/
`createDropdownMenu`/`createRadioButtonGroup` all construct the real thing
now (previously stubbed with warnings). Builds cleanly, linked into
`new_qt_duoplot_core`; 5x crash-consistency check still clean.
Research for this was done via a background research agent that read
`main_application/gui_elements.{h,cpp}` and `gui_element_state.h` in full
and reported back a byte-for-byte wire-format spec plus a list of
pre-existing wx quirks/bugs — referenced throughout as "the research note."

- **`ButtonGuiElement`**: `QPushButton` + `GuiElement`. `publish_to_local_`/
  `publish_to_serial_`/`id_` from `ButtonSettings`, `clicked()` →
  `sendGuiData()` when `publish_to_local_`, `is_pressed_` tracked via
  `mouseLeftPressedGuiElementSpecific`/`mouseLeftReleasedGuiElementSpecific`
  for the 1-byte GUI-callback payload, `setLabel` via
  `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` (Qt's equivalent of
  wx's `CallAfter` — both exist because the label can be set from a non-GUI
  thread). `sendDataToSerialInterface` isn't ported — a `// TODO:
  Implement` stub in wx too, nothing to port.
- **`CheckboxGuiElement`**: `QCheckBox` + `GuiElement`. wx quirk preserved:
  ignores `publish_to_local`/`publish_to_serial` entirely, always calls
  `sendGuiData()` on toggle. `updateElementSettings` is a no-op, matching wx
  (not a gap from this port — wx's is empty too).
- **`TextLabelGuiElement`**: `QLabel` + `GuiElement`. wx bug *not*
  preserved: wx's `fillGuiPayload` reads a `label_` member that's declared
  but never assigned, so real wx always reports an empty label on the wire
  for this type — but no code path ever actually calls `sendGuiData()` for
  a TextLabel (dead code either way), so there's no observable difference
  between preserving and fixing it. Implemented correctly (reads the live
  Qt label text) since fixing dead code costs nothing.
- **`EditableTextGuiElement`**: `QLineEdit` + `GuiElement`. `textChanged` →
  `sendGuiData()` on every keystroke (unconditional, matches wx — no
  `publish_to_local` check exists for this type in wx either);
  `returnPressed` → one-shot `enter_pressed_=true` pulse around a single
  `sendGuiData()` call, matching wx's `editableTextEnterPressedCallback`
  exactly. `enter_pressed_` initialized to `false` (wx leaves it
  uninitialized — that's undefined behavior, not a behavior worth
  preserving). `updateElementSettings` no-op, matching wx.
- **`DropdownMenuGuiElement`**: `QComboBox` (`setEditable(false)`, wx's
  `wxCB_READONLY`) + `GuiElement`. wx quirk preserved:
  `DropdownMenuSettings::initially_selected_item` is never consulted;
  selection always starts at index 0. `updateElementSettings` no-op,
  matching wx.
- **`ListBoxGuiElement`**: `QListWidget` (single-selection) + `GuiElement`.
  Wire payload is structurally identical to DropdownMenu's (same
  `[len][selected][count][(len,bytes)...]` shape) — matches wx, where the
  two classes' `fillGuiPayload` bodies are the same code independently
  duplicated. `updateElementSettings` no-op, matching wx.
- **`RadioButtonGroupGuiElement`**: `QGroupBox` + `QButtonGroup` of
  `QRadioButton`s in a `QVBoxLayout` — wx has no equivalent single widget
  (built on `wxRadioBox`), so this reconstructs the same visual/functional
  shape (vertical stack, exclusive selection, group title) from Qt parts.
  The sub-buttons are plain Qt children the group box lays out itself, not
  separately tracked `GuiElement`s, matching wx (wx's `wxRadioBox`
  sub-buttons aren't separate `ApplicationGuiElement`s either).
  `updateElementSettings` no-op, matching wx.
  - **Found and fixed a genuine pre-existing bug in the shared framework
    code** (`project_state/other_gui_settings.cpp`, copied verbatim from
    `main_application`, not previously modified): `RadioButtonGroupSettings`'s
    default constructor never sets `type`, unlike every sibling `*Settings`
    default constructor — left at `GuiElementType::Unknown`, a group built
    from it is silently skipped by `WindowTab`'s dispatch switch, i.e. the
    feature is completely non-functional from that path, not an observable
    quirk anyone could be relying on. Confirmed narrow in scope: the
    JSON-loading constructor path (project files) sets `type` correctly via
    `ElementSettings::parseSettings`, so this only affects brand-new,
    freshly-constructed groups (exactly the case the dev bootstrap below
    hit). Fixed in this port's copy; worth reporting upstream in
    `main_application` too, since it's a real bug there as well, not
    something this port introduced.
- **`SliderGuiElement`**: `QSlider` + `GuiElement`, with 3 auxiliary
  `QLabel` siblings (`min_text_`/`max_text_`/`value_text_`, parented to the
  containing tab like wx's `wxStaticText`s, not to the slider itself) whose
  positions are recomputed in an overridden `setElementPositionAndSize`
  using wx's exact hand-tuned pixel offsets. `setSize` clamps to
  `min_x_size_`/`max_x_size_`/`min_y_size_`/`max_y_size_`, and one axis is
  pinned to exactly 30px at construction based on `is_horizontal_` (locks
  slider "thickness", leaves "length" resizable) — matches wx exactly. wx
  quirk preserved: the widget is **always constructed with `Qt::Horizontal`**
  regardless of `is_horizontal_`, even though the sizing/positioning code
  fully supports vertical — matches a likely-dormant wx bug (the research
  agent flagged `getStyle()` as computed-but-never-called in wx, always
  passing the horizontal style to the `wxSlider` constructor). Also
  preserved: `getGuiElementState()` hardcodes `is_horizontal=true` in the
  `SliderState` it returns, regardless of actual orientation (a literal
  `true` in wx, not a use of the real member) — currently dead code either
  way since nothing calls `getGuiElementState()` yet
  (`QUERY_FOR_SYNC_OF_GUI_DATA` is deferred). None of the Slider quirks
  were reported as things the user wants changed, so all are preserved and
  flagged rather than silently fixed — see the class comment in
  `gui_elements.h` for exactly where.
- **Wire-protocol note for anyone extending this later**: each element type
  has two independent, hand-written serialization paths — `sendGuiData()`'s
  `getGuiPayloadSize()`/`fillGuiPayload()` (the live per-interaction push,
  the one actually exercised right now) and `getGuiElementState()`'s
  `XxxState::serializeToBuffer()` (a full-state-snapshot path, currently
  unused pending `QUERY_FOR_SYNC_OF_GUI_DATA`). For Slider these two
  diverge in wx (payload omits `is_horizontal`, state includes it); for
  every other type they match. Both paths are ported byte-for-byte matching
  wx for every type.
- **Dev-only test bootstrap added** (`MainWindow::bootstrapDefaultProject`,
  clearly commented as temporary): creates one instance of each of the 8
  element types with the exact handle names `system-test`'s `c gui basic`
  (`basic_c/gui_test.c`) expects — `button0`, `slider0`, `checkbox0`,
  `text_label0`, `listbox0`, `ddm0`, `rbg0`, `text_entry` — since in wx
  these only ever come into existence via the "New Element" popup menu
  (deferred) or a loaded project file (`SaveManager`, deferred), and this
  port had no other way to create one for testing. To be removed once
  either of those exists. Confirmed via logs: all 8 elements construct
  without the "unknown GuiElementType, skipping" warning that surfaced the
  bug above, across 5 repeated launches with no crashes.
- **Two more bugs found and fixed while trying to actually run `c gui basic`**,
  both in pre-existing code outside `new_qt_application` — confirmed via
  hands-on testing (build, run, watch it fail, trace the real cause), not
  guessed:
  1. **`src/interfaces/c/duoplot/internal.h`'s `duoplot_internal_isDuoplotRunning()`**
     checked `ps -ef` output for the literal substring `"duoplotplot"`
     (doubled) — a typo that can never match any real process name, so the
     check always returned false regardless of what was running. Confirmed
     pre-existing and unrelated to this port: reproduced the identical
     failure against the original wx `duoplot` binary. Fixed to check for
     `"duoplot"` (one line). This unblocked the client from starting its
     GUI query thread at all.
  2. **`Function::QUERY_FOR_SYNC_OF_GUI_DATA` was still a deferred no-op**
     in `MainWindow` — but fixing bug #1 meant the client now actually sent
     this query and then blocked waiting for a response that never came
     (`duoplot_internal_waitForSyncForAllGuiElements()` blocks on a
     `recv()`). Implemented `MainWindow::updateClientApplicationAboutGuiState()`,
     a direct port of `main_application/main_window.cpp`'s method of the
     same name: iterates `gui_elements_`, calls each element's (now fully
     implemented) `getGuiElementState()`, and sends the serialized buffer
     over the GUI TCP port. No longer deferred — moved out of the
     "Deferred" list in the class-level comment in `main_window.h`.
  - **Verified working, precisely**: with both fixes, a `script`-wrapped
    run showed the client print `duoplot is running, starting GUI receive
    thread!` → `Waiting for duoplot application to send GUI state...` →
    `GUI state received!` in sequence — confirming the sync request is
    sent, received, and parsed correctly for all 8 bootstrapped elements
    (right length-prefix, right per-type payload layout, matching wx
    byte-for-byte). The process then pegged at 100% CPU indefinitely after
    that point — traced to `testGUIBasic()`'s own `while(true) { scanf(...);
    ... }` interactive input loop spinning because this session has no real
    interactive terminal to type into (confirmed via CPU-time-vs-wall-time:
    climbing in lockstep, a tight spin, not a blocked wait) — not a protocol
    bug. This specific point (past the sync, into the interactive
    click-and-type part of the test) is where hands-on testing takes over
    from here.
- **Confirmed working, hands-on, by the user**: the slider produced correct
  live callbacks in `system-test` — the wire protocol, `sendGuiData`, and
  callback registration/dispatch are genuinely functioning end to end for
  at least one element type.
- **A fourth bug found from that same test**: every element rendered
  "super small," matching the very first layout bug found in `GuiWindow`
  back near the start of Phase 2 (tiny stuck panes) — but this is a new,
  different instance of the same *class* of bug, introduced by the later
  redesign to a real `QHBoxLayout` (`content_area_`/`tab_button_panel_`).
  Root cause: `GuiWindow`'s constructor calls `setGeometry(...)` and then
  immediately (still inside the constructor, before the window has ever
  been shown) reads `content_area_->size()` to lay out each tab's elements
  — but for a widget that's never been shown, Qt may not have actually
  finished recalculating child-layout geometry yet, even after
  `setGeometry()` returns, so this reads a stale, tiny, pre-layout size.
  wx avoids the equivalent problem by calling `Show()` *before* creating
  any tabs/elements, then relying on a later `SetSize()` call to fire a
  real `wxSizeEvent` — `OnSize` — which re-lays-out everything using the
  event's own (guaranteed-correct) size. Fixed with the Qt equivalent:
  added a `showEvent()` override that performs the real element layout
  pass the first time the window is actually shown, since that's the first
  point Qt guarantees `content_area_`'s layout has been resolved — the
  constructor's own post-`setGeometry()` pass is left in place as a
  harmless first attempt, superseded by `showEvent`'s pass whenever it
  runs too early. Rebuilt cleanly, 3x crash check still clean.
  **Confirmed fixed by the user**: elements now show up at proper sizes,
  and all callbacks work — button, slider, checkbox, listbox, dropdown,
  radio group, and editable text are all functionally verified end to end
  (client → server wire protocol → live Qt widget → user interaction →
  server → client callback), not just the slider.
- `ScrollingTextGuiElement` stays deferred (tied to the separate topic/
  stream-of-strings system, already deferred elsewhere).

## SaveManager (2026-09-20)

Ported and wired up fully, replacing the earlier "deferred, bootstraps a
hardcoded default window" state. `project_state/save_manager.h` needed zero
changes — copied verbatim from `main_application` (already framework-
independent, header-only). Everything built on it in `MainWindow` is a
faithful port of `main_application/main_window.cpp`'s equivalent methods:

- **Constructor**: same `configuration_agent_->hasKey("last_opened_file") &&
  lumos::filesystem::exists(...)` check wx uses, then
  `setupWindows(save_manager_->getCurrentProjectSettings())`.
- **`setupWindows(const ProjectSettings&)`** (new): iterates
  `project_settings.getWindows()`, constructs one `GuiWindow` per entry —
  the general form `bootstrapDefaultProject()` was standing in for. Skips
  wx's `plot_pane_subscriptions_`/serial-topic bookkeeping (out of scope —
  serial data parsing is separately deferred).
- **`newProject()`, `saveProject()`, `saveProjectAs(path)`,
  `saveProjectAs()`, `openExistingFile(path)`, `openExistingFile()`,
  `fileModified()`, `setIsFileSavedForAllWindows()`**: all ported 1:1. The
  two no-arg, dialog-driven overloads use `QFileDialog`/`QMessageBox` in
  place of `wxFileDialog`/`wxMessageBox` — same "unsaved changes, proceed?"
  confirmation, same save/open filter string. No menu exists yet to trigger
  these interactively (menu bar is still deferred), but they're implemented
  and ready for it.
- **`Function::OPEN_PROJECT_FILE`** now actually does something: mirrors
  wx's queue-then-drain pattern exactly (`open_project_file_queued_`/
  `queued_project_file_name_` set in `manageReceivedData` on the TCP
  receive thread, consumed in `receiveData()` on the main/GUI thread ahead
  of `new_window_queued_`, same order as wx) — needed since
  `openExistingFile` touches `GuiWindow`s and can't run off the GUI thread.
- **One deliberate, documented deviation** (flagged in `main_window.h`'s
  class comment rather than silently decided): a completely fresh install
  with no `last_opened_file` would leave wx with *zero* windows —
  `bootstrapDefaultProject()`'s demo window (one of each GUI element type)
  is now a fallback used *only* when `setupWindows` produces zero windows,
  to keep `c gui basic` and friends testable without a project file on
  disk. Real project loading/saving is otherwise unaffected.

**Verified, not just compiled**:
- Repurposed the pre-existing shared `~/Library/Preferences/duoplot/
  configuration.json` (already pointing `last_opened_file` at a real
  `project_files/embedded_gui_test.duoplot` from prior wx usage — same
  config file wx's `main_application` reads). Launched `new_qt_duoplot`:
  log showed `ProjectSettings` correctly parsing the real file, one
  `GuiWindow` "Window 0" created with tab "primary_view" containing a real
  `PlotPane` ("p1") plus `Button`/`Slider` elements from the file (not the
  demo bootstrap) — `setupWindows` logged "Created 1 windows". 3x repeated
  launch, no crash.
- Temporarily pointed `last_opened_file` at a nonexistent path: confirmed
  the fallback fires correctly (`setupWindows` logs "Created 0 windows",
  then `bootstrapDefaultProject` logs "Created 1 windows" for the demo
  window) — config restored afterward.
- Ran `system-test cpp basic openProjectFile` against a live
  `new_qt_duoplot`: confirmed the full wire-protocol round trip end to end
  — server received `OPEN_PROJECT_FILE`, drained it on the main thread,
  called `removeAllWindows()` (destroyed the existing `PlotPane`, logged
  `PlotPane destroyed`), then attempted each requested path. The test's
  hardcoded paths (`/Users/danielpi/work/dvs/project_files/...` — a
  different username, not this machine's — already flagged as a known
  cross-machine issue in the original plan) don't resolve here, but the
  resulting `ProjectSettings(file_path)` JSON-parse exceptions are caught
  by the pre-existing, framework-independent `project_settings.cpp:380`
  handler (shared, unmodified code — logs and leaves `windows_` empty), so
  `setupWindows` cleanly logs "Created 0 windows" each time rather than
  crashing. No hang, no crash, across 6 consecutive bad-path attempts —
  confirms `openExistingFile`'s error path is robust, independent of
  whether the specific test fixture paths resolve on this machine.

## Popup-menu system (2026-09-20)

Ported `GuiWindow`'s three right-click context menus, faithfully mirroring
`main_application/gui_window.cpp:134-1286` (structure, item order, and
known quirks), adapted to Qt idioms where wx's event plumbing has no direct
equivalent:

- **Menu construction** (`buildPopupMenus()`, called once from the
  constructor): three `QMenu*` — `popup_menu_window_`/`_element_`/`_tab_`
  — each with a "New element" `QMenu*` submenu, built with the exact same
  item order wx uses per menu. Uses direct per-`QAction` lambda connections
  instead of wx's numeric-ID + `Bind()` indirection — Qt has no need for
  that dance, so the corresponding `create*Callback`/`edit*`/`delete*`/
  `raise*`/`lower*`/`toggle*` methods are private (called only from those
  lambdas), unlike wx's public `wxCommandEvent`-taking slots.
- **Dispatch**: `mouseRightPressed(pos, source, item_name)` (previously a
  stub — the surrounding plumbing, `ClickSource` enum, `last_clicked_item_`,
  `notify_parent_window_right_mouse_pressed_`, was already in place from
  earlier work but never wired to real menus) now actually pops up the
  matching menu. `pos`'s coordinate space depends on `source` (content_area_
  for a GUI element, the GuiWindow itself for the background, the tab-button
  panel for a tab button — matches how each call site already produced it),
  mapped to global via the matching widget's `mapToGlobal`. Added
  right-click wiring for tab buttons (`addTabButton()`'s
  `customContextMenuRequested`), which didn't exist before this.
- **New-element dialog**: `SettingsDialog` (`settings_dialog.h/.cpp`) is a
  direct port of `main_application/settings_window.{h,cpp}` — one
  `QLineEdit` per field (label + initial value) plus OK/Cancel, built with
  `QFormLayout`/`QDialogButtonBox` instead of wx's manual `wxBoxSizer` nesting.
  `getValidNewElementHandleString()` reuses it in a validation loop
  (non-empty, unique handle name) matching wx's exact loop structure, using
  `QMessageBox::warning` in place of `wxMessageDialog`. Edit-window-name and
  edit-tab-name use `QInputDialog::getText` instead (a single text field
  needs no custom dialog — wx's `wxTextEntryDialog` maps directly).
- **Matches wx's own actual scope, not full 8-type coverage**: the "New
  element" submenu lists all 9 wx menu entries (including Plot pane), but
  wx itself only wires up Plot pane/Button/Slider/Checkbox/Text label —
  List box/Editable text/Dropdown menu/Radio button group have empty
  handler functions in wx (`gui_window.cpp:755-794`), so those 4 items are
  present but disabled (`QAction::setEnabled(false)`) here too, matching wx
  exactly rather than "completing" something wx itself left unfinished.
  `WindowTab` already has working `createNewListBox`/`createNewEditableText`/
  `createDropdownMenu`/`createRadioButtonGroup` (built earlier for the
  wire-protocol path), so enabling these later is a one-line change per
  item if ever wanted.
- **`editElementName()` bug caught and fixed before it shipped**: an early
  draft populated `ret_fields` (the new field values to apply) only on the
  "chose a new unique name" branch of the validation loop, not the "kept
  the same name" branch — meaning editing e.g. a Button's label while
  leaving its handle name unchanged would have silently discarded the
  label edit and reverted to the original value. Fixed by populating
  `ret_fields` from the dialog once, right before the shared `break`,
  regardless of which of the two valid paths was taken — matching wx's own
  structure (it fills `ret_fields` once, after the validation loop, not
  conditionally inside it).
- **`CheckboxGuiElement::updateElementSettings` is a no-op** (so renaming a
  checkbox via "Edit element" silently does nothing) — checked against wx
  and confirmed this is a **matching wx quirk**, not a gap introduced by
  the port: `main_application/gui_elements.cpp:267`'s
  `CheckboxGuiElement::updateElementSettings` is equally empty. Left as-is.
- **`deleteTab()`'s reselection edge case is an adapted simplification, not
  a line-for-line port**: wx re-derives which tab becomes visible after a
  delete from its `TabButtons`' own internal selection bookkeeping, which
  wasn't fully traced. The port instead checks *before* deleting whether
  the removed tab's button was the checked/visible one, and falls back to
  `tabs_[0]` if so — behaviorally reasonable and low-risk (this menu item
  is only reachable with ≥2 tabs, since the tab-button strip that hosts the
  right-click target is itself hidden with ≤1 tab), but flagged here as a
  judgment call rather than a verified match.
- **`GuiWindow::createNewPlotPane()` (0-arg) is genuinely dead code in both
  wx and the port** — confirmed via `grep`, zero call sites in either
  codebase. Not connected to any popup-menu item (the menu's "Plot pane"
  entry goes through `createNewPlotPaneCallback()`, the dialog-based path,
  matching wx's `createNewPlotPaneCallbackFunction`). Left untouched.
- **Fixed a latent stale-capture bug while touching this code**: the
  constructor's original tab-button setup captured a tab's *index* by
  value in its `clicked` lambda (`[this, idx]() { tabChanged(tabs_[idx]->
  getName()); }`). That index would go stale for every button to the right
  of one removed by the new `deleteTab()`. Refactored into a shared
  `addTabButton(WindowTab* tab)` helper that captures the `WindowTab*`
  pointer directly instead — stable across insertion/removal. This wasn't
  a live bug before now (tab deletion didn't exist yet), but would have
  been the moment `deleteTab()` shipped, so fixed as part of this change
  rather than left as a trap.
- **`MainWindow::deleteWindow(callback_id)` and `printGuiCallbackCode()`
  added** (previously nonexistent): `deleteWindow` is a straightforward
  port of `main_window.cpp:1067-1119`, using an int `callback_id` passed
  directly from the menu action's lambda closure instead of wx's
  event-ID-based lookup (Qt has no need for the indirection — the closure
  already has the right `GuiWindow`'s `callback_id_` at hand).
  `printGuiCallbackCode` ports the text-generation logic in
  `main_window.cpp:294-379` verbatim (byte-for-byte matching output
  format), but sinks through `push_text_to_cmdl_output_window_`, whose
  default was changed from a no-op lambda to `std::cout <<` — a reasonable
  interim since `CmdlOutputWindow` itself is still deferred; swap this one
  line once that's ported.
- **Verified by the user, bugs found and fixed**:
  - Right-edge element resize was reported not working; retested after an
    unrelated fix and the user confirmed it already works — no code change
    was needed (most likely user error on the first attempt, or it was
    fixed incidentally; not chased further since it's now confirmed good).
  - **Real bug found**: could only ever add one extra tab (2 total) via
    "New tab", no matter how many times it was clicked afterward.
    Diagnosed via temporary `[diag]` logging (removed after the fix) in
    `newTab()`/`mouseRightPressed()`, which proved `tabs_`/
    `tab_button_widgets_` grew correctly on every click (5 tabs after 4
    clicks) — so the bug was pure visibility, not logic. Root cause:
    `GuiWindow::layoutTabButtons()` never called `show()` on the tab
    buttons. A widget only inherits visibility from its parent's
    hidden→visible *transition* at the moment that transition happens —
    `tab_button_panel_` transitions from hidden to visible exactly once
    (when the 2nd tab is added), which cascades to whatever children
    existed at that instant, but every button added afterward (parent
    already visible) needs its own explicit `show()`. Fixed by adding
    `tab_button_widgets_[k]->show();` in the layout loop — the same
    pattern `MainWindow::layoutWindowButtons()` already used correctly for
    its own per-window buttons (a discrepancy that should have been
    caught by cross-referencing that method at write-time). **Confirmed
    fixed by the user**: "I can add arbitrarily many tabs and they keep
    track of their elements."
  - New window, new tab, and new-element creation (button/slider/checkbox/
    text label/plot pane via dialog) are now hands-on confirmed working.
    Edit/delete element, raise/lower, edit window/tab name, delete
    window/tab, the Zoom/Pan/Rotate/Select mode radios, and "Print gui
    code" have not been explicitly exercised yet.

## Preferences dialog (2026-09-20)

**Not a port — genuine new functionality, explicitly approved by the user
after checking wx first.** wx's own "Preferences" (`main_window.cpp:974`)
is a dead stub: `preferences()` just prints `"Preferences!"` to stdout, and
the tray icon's Preferences menu item that would reach it is commented out
(`tray_icon.cpp:173`), so it's never actually reachable in the real app.
There's no `SettingsHandler` class anywhere in `main_application` either —
that name in the original planning doc was aspirational, not a reference
to something that exists. Given there was nothing to mimic, asked the user
how to proceed; they chose to build a real one anyway.

- **`PreferencesDialog`** (`preferences_dialog.h/.cpp`): a small `QDialog`
  (`QFormLayout` + one `QSpinBox` + `QDialogButtonBox`) exposing the one
  `ConfigurationAgent`-backed tunable that actually exists:
  `visualization_period_ms` (wx reads this once at startup, clamped
  [1,100], to set the receive-timer's period — `main_window.cpp:152-159`).
- **`MainWindow::openPreferences()`**: reads the current value via a new
  `getVisualizationPeriodMs()` helper (factored out of the constructor,
  which now calls it too — previously the clamped-read logic only existed
  inline in the constructor), shows the dialog, and on accept both writes
  the new value via `configuration_agent_->writeValue(...)` *and* applies
  it live with `receive_timer_->setInterval(new_period_ms)` — a small,
  deliberate improvement over wx, which never live-updates the timer after
  startup (you'd have to restart the app). Low-risk since this is new
  functionality with no existing behavior to regress.
- **Trigger**: a plain `preferences_button_` `QPushButton` added to
  `MainWindow`'s existing control-panel button stack, right after "New
  window" (`layoutWindowButtons()` extended to position it) — the menu bar
  that would normally host a "Preferences" item is still deferred, so this
  reuses the same native-button vocabulary the control panel already uses.
- **Verified**: builds cleanly, 3x repeated launch with no crash. Not yet
  hands-on tested (dialog opens, spinbox saves/applies correctly) — needs
  the user.

## Not started

Phase 3 backlog remaining: tray icon, `CmdlOutputWindow`/
`TopicTextOutputWindow`, serial data parsing, and the topic/stream-of-strings
subscription system for `PlotPane`. None of this is copied from
`src/qt_application` — per the plan, that code is reference-only and each
piece gets read, compared against the wx original in `main_application`,
and only then adapted or rewritten.
