# `src/main_application/project_state/` — architecture overview

> **Note (post-`39a6dc9e` pull):** `main_application` now depends on the
> external `third_party/LumosAlgo` submodule instead of
> `src/interfaces/cpp/duoplot/` — every `duoplot::`/`DUOPLOT_ASSERT`/
> `#include "duoplot/..."` reference below now reads `lumos::`/
> `LUMOS_ASSERT`/`#include "lumos/..."` in the actual source (e.g.
> `duoplot::GuiElementType` → `lumos::GuiElementType`). See
> `src/main_application/docs/ARCHITECTURE.md`'s dedicated section for what
> this means. **Every bug listed below was re-checked against the current
> source and is still present** — this pull touched nearly every file in
> this module but only for the rename; none of the equality/serialization
> bugs were fixed.

This module is the persistence layer for **project/layout configuration**:
window positions, tabs, and every GUI element placed on them (plot panes,
buttons, sliders, checkboxes, text labels, dropdowns, list boxes, radio
groups, static boxes, scrolling-text panes) — the things a user lays out in
the GUI and expects to still be there after saving and reopening a
`.duoplot` project file. It also contains a small, unrelated second concern:
`ConfigurationAgent`, an application-level (not project-level) preferences
store. 10 files, ~2540 lines.

**GUI-toolkit note (relevant to the planned wxWidgets → Qt migration): no
wxWidgets usage anywhere in this module** (confirmed by search). Everything
here is plain C++ structs plus `nlohmann::json` (de)serialization — no
rendering, no widget creation, no wx types. The only external consumer found
is `main_window.cpp` (`MainWindow` reads a `ProjectSettings` on startup/
open-project and constructs the actual wx widgets from it; it presumably
also walks the live wx widget tree to build a `ProjectSettings` before
saving — that construction/extraction logic itself lives in `MainWindow`,
not here). This module's data model should port to Qt largely unchanged;
only its consumer needs rewriting.

## Two independent concerns, sharing a directory

1. **Project/layout settings** (`project_settings.h`, and most of the
   `.cpp` files) — the bulk of the module. A tree of plain-data structs
   mirroring the `.duoplot` JSON file format: `ProjectSettings` →
   `WindowSettings` → `TabSettings` → `ElementSettings` (and its
   subclasses). `SaveManager` wraps this tree with dirty-tracking and
   file I/O.
2. **Application preferences** (`configuration_agent.h/.cpp`) —
   `ConfigurationAgent`, a generic typed key/value store backed by a single
   JSON file at a fixed OS-specific config path (`getConfigDir()`, from
   `platform_paths.h`, outside this module), used for things like "last
   opened file" (see `AppPreferences`) that aren't part of any one project.
   Unrelated to `ProjectSettings`/`SaveManager` other than sharing the
   `nlohmann::json` library and living in the same directory.

## The settings class hierarchy

```
ElementSettings                          (x, y, width, height, handle_string, z_order, type)
├── PlotPaneSettings                     (colors, grid/box/numbers/letters on-off, projection mode,
│                                          pane_radius, subscribed_streams — serial-stream overlays)
├── ScrollingTextSettings                (title, print_timestamp/topic_id, subscribed text streams)
├── StaticBoxSettings                    (label + nested vector<shared_ptr<ElementSettings>> — a container)
└── GuiElementSettings                   (+ publish_to_local/publish_to_serial, numeric id)
    ├── ButtonSettings                   (label)
    ├── CheckboxSettings                 (label)
    ├── EditableTextSettings             (init_value)
    ├── DropdownMenuSettings             (elements list, initially_selected_item)
    ├── ListBoxSettings                  (elements list)
    ├── RadioButtonGroupSettings         (label + vector<RadioButtonSettings>)
    ├── TextLabelSettings                (label)
    └── SliderSettings                   (min/max/init/step, is_horizontal)
```

`RadioButtonSettings` (a single radio button's label) is a small
non-`ElementSettings` helper type only used inside
`RadioButtonGroupSettings`.

Containment above `ElementSettings`:
```
ProjectSettings → vector<WindowSettings> → vector<TabSettings> → vector<shared_ptr<ElementSettings>>
```
Every level has a matching `toJson()`/JSON-constructor pair and an
`operator==`/`operator!=` — this module's core repeating pattern is
"one struct, one JSON shape, one equality check" repeated ~15 times for
every element type the GUI supports.

`duoplot::GuiElementType` (the enum tag used to pick which concrete subclass
to instantiate when parsing) is defined in the client-interface library
(`src/interfaces/cpp/duoplot/enumerations.h`), not here — `type` field
values are the same enum shared with the wire protocol's GUI-element
messages (see the interfaces docs).

## JSON shape conventions

- Every `ElementSettings` writes `handle_string`, `x`, `y`, `width`,
  `height`, `type` (as a string via `guiElementTypeToString`), and
  optionally `z_order` (only if non-default, via `assignIfNotDefault`).
  `x`/`y` are clamped to `[0, 0.99]` and `width`/`height` to
  `[0.01, 1.0]` on parse (`ElementSettings::parseSettings`) — these are
  pane-relative normalized coordinates, not pixels.
- Subclass-specific fields nest under an `"element_specific_settings"`
  object — every subclass constructor checks `j.contains(
  "element_specific_settings")` and bails early (leaving defaults) if
  absent, so old/minimal project files remain loadable.
- Colors serialize as `{"r": ..., "g": ..., "b": ...}` objects
  (`colorToJsonObj`/`jsonObjToColor` in `helper_functions.h`), and are
  omitted from output entirely when equal to the type's documented default
  (`RGBTriplet` has `operator==`, so `!=` comparisons against a `constexpr`
  default work directly).
- `helper_functions.h`'s `getOptionalValue<T>(j, key, default)` and
  `assignIfNotDefault<T>(j, key, val, default)` are the two generic helpers
  behind nearly every field's read/write — check these first if a
  save/load round-trip issue affects a field's *presence*, as opposed to
  its value.
- Enum-like string fields (line style, scatter style, projection mode,
  stream type) are hand-written string tables rather than using
  `nlohmann::json`'s enum macros — each one is a bespoke `if/else if`
  chain independently maintained in both the parse direction and the
  `toJson` direction. **These two directions have drifted apart in at
  least one place** — see "Known bugs" below.

## `SaveManager`

A thin stateful wrapper (header-only, `save_manager.h`) around one
`ProjectSettings` plus a file path and two booleans (`is_saved_`,
`save_path_is_set_`). `save()`/`saveToNewFile()` both compare the incoming
`ProjectSettings` against the currently-held one via `operator!=` before
writing — **this dirty-check is currently unreliable**, since
`ProjectSettings::operator==` transitively depends on `TabSettings::
operator==`, which has an inverted-logic bug (see "Known bugs"). In
practice this likely means `save()`'s "skip write if nothing changed"
optimization rarely if ever takes effect — a functional but wasteful
degradation (extra disk writes), not a correctness/data-loss risk, since
`saveToNewFile()` and the initial `save()` call still write correctly
either way.

## `ConfigurationAgent`

Independent of everything else in this file. On construction, ensures a
config directory and a `configuration.json` file exist at a fixed
OS-specific path (creating an empty `{}` file if missing or unreadable —
corruption self-heals by silently resetting to empty, logging a warning).
`readValue<T>(key)`/`writeValue<T>(key, val)` are generic accessors that
re-read/rewrite the *entire* file on every single call (no in-memory
caching, no batching) — fine for its actual use (occasional preference
reads/writes), but not something to reuse for frequent or bulk key/value
access without adding caching first. `AppPreferences` (a plain struct: last
opened file path, whether to open the main window on start) documents the
*intended* shape of what's stored but isn't actually used as a typed
read/write unit anywhere in this file — callers presumably call
`readValue`/`writeValue` per individual key (check `main_window.cpp` for
actual key names in use).

## Known bugs (found while reading — verified against the source, not just suspected)

These are concrete, verifiable defects, not style opinions — worth fixing
opportunistically or at least being aware of before relying on the affected
behavior:

- **`TabSettings::toJson()` compares every button-color field against
  `background_color` instead of its own field** (`plot_pane_settings.cpp`
  is not the file — this is in `project_settings.cpp`):
  ```cpp
  if (background_color != kButtonNormalColorDefault) { j["button_normal_color"] = ...; }
  if (background_color != kButtonClickedColorDefault) { j["button_clicked_color"] = ...; }
  if (background_color != kButtonSelectedColorDefault) { j["button_selected_color"] = ...; }
  if (background_color != kButtonTextColorDefault) { j["button_text_color"] = ...; }
  ```
  Each should compare `button_normal_color`/`button_clicked_color`/
  `button_selected_color`/`button_text_color` against its *own* default.
  As written, whether a button color gets serialized depends on whether
  `background_color` happens to differ from a *different* field's default
  — a copy-paste bug. Practical effect: custom button colors can be
  silently dropped from saved files (if `background_color` happens to
  equal one of the button-color defaults) or written unnecessarily.

- **`TabSettings::operator==` has inverted equality logic**:
  ```cpp
  if (areDerivedElementEqual(other_element, elements[k]))
  {
      return false;   // <-- returns false when the elements ARE equal
  }
  ```
  This means two tabs whose elements genuinely match will (for any tab with
  at least one element) still compare as **not equal**. This propagates
  upward through `WindowSettings::operator==` and `ProjectSettings::
  operator==`, which is what `SaveManager::save()`'s dirty-check relies on
  — see the `SaveManager` section above for the practical consequence.

- **`SubscribedStreamSettings::toJson()`'s `scatter_style` branch is
  unreachable**:
  ```cpp
  if (line_style == SOLID) {...}
  else if (line_style == DASHED) {...}
  else if (line_style == SHORT_DASHED) {...}
  else if (line_style == LONG_DASHED) {...}
  else if (scatter_style == SQUARE) {...}   // never reached: line_style always
  else if (scatter_style == CIRCLE) {...}   // matches one of the four branches
  ...
  ```
  Because `line_style` is always one of its four enum values, control never
  falls through to the `scatter_style` checks — **`scatter_style` is never
  written to JSON**, regardless of its actual value, for any
  `SubscribedStreamSettings` (used by scatter/scatter3d serial-stream
  overlays on a `PlotPaneSettings`).

- **`SubscribedStreamSettings::toJson()` also unconditionally dereferences
  an unset `std::optional`**: `color` is declared
  `std::optional<duoplot::properties::Color> color{std::nullopt}` and
  nothing guarantees it's set before `toJson()` runs (the parse constructor
  only sets it `if (j.contains("color"))`), yet `toJson()` does
  `color.value()` unconditionally — serializing a `SubscribedStreamSettings`
  that was never given an explicit color throws `std::bad_optional_access`.

- **`"long_dashed"` vs. `"LONG_DASHED"` round-trip mismatch**: the parser
  (`SubscribedStreamSettings`'s constructor, in `plot_pane_settings.cpp`)
  accepts lowercase `"long_dashed"`, but `toJson()` writes uppercase
  `"LONG_DASHED"`. Saving a `LONG_DASHED`-styled stream and reloading the
  file throws `std::runtime_error("Invalid option for \"line_style\": \"LONG_DASHED\"")`.

- **`RadioButtonGroupSettings::operator==` has no bounds check**:
  ```cpp
  for (size_t i = 0; i < radio_buttons.size(); ++i)
      all_equal = all_equal && radio_buttons[i] == other.radio_buttons[i];
  ```
  If `other.radio_buttons` has fewer elements than `radio_buttons`, this
  indexes past the end of `other.radio_buttons` (undefined behavior) rather
  than safely comparing sizes first.

- **`StaticBoxSettings::operator==` compares an iterator against the wrong
  container's `end()`**:
  ```cpp
  const auto q = std::find_if(other.elements.begin(), other.elements.end(), ...);
  all_equal = all_equal && q != elements.end();   // should be other.elements.end()
  ```
  Comparing an iterator obtained from `other.elements` against `elements.end()`
  (a different container's `end()`) is undefined behavior per the C++
  standard, not merely "probably still correct by luck."

- **`StaticBoxSettings::toJson()` never writes `label`** — the constructor
  reads `label` from `element_specific_settings.label`, but `toJson()`
  only ever writes `elements`, never `label`. A `StaticBoxSettings`'s label
  is silently lost on every save/reload round-trip.

- **`SliderSettings`'s vertical-orientation support is fully commented
  out** (`is_horizontal` is hardcoded `true` in both the JSON constructor
  and the default constructor; the `"style"` field's read logic and the
  corresponding `toJson()` write are both present but entirely inside block
  comments, with a `// TODO: Only support horizontal sliders for now`) —
  the `is_horizontal` member and its accompanying `operator==` check exist,
  but nothing can ever set it to `false` through normal (de)serialization.

None of these bugs affect this module's data *layout* or its JSON schema in
a way that would block a straightforward Qt-side reimplementation — they're
worth fixing (or at least being aware of, since equality/serialization bugs
are easy to accidentally re-introduce if this code is ported line-by-line)
rather than architectural blockers.

See [`FILE_REFERENCE.md`](FILE_REFERENCE.md) for a per-file breakdown.
