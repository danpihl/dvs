# File-by-file reference — `src/main_application/project_state/`

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the overall class hierarchy,
JSON conventions, and a list of concrete bugs found in this module — this
document is a per-file pointer, not a repeat of that detail.

### `project_settings.h`
The header for nearly everything: `ElementSettings` (base), `StreamType`
enum, `SubscribedTextStreamSettings`, `ScrollingTextSettings`,
`SubscribedStreamSettings`, `PlotPaneSettings`, `GuiElementSettings` (+
`GuiElementId` alias), `ButtonSettings`, `CheckboxSettings`,
`EditableTextSettings`, `DropdownMenuSettings`, `ListBoxSettings`,
`RadioButtonSettings`, `RadioButtonGroupSettings`, `TextLabelSettings`,
`SliderSettings`, `StaticBoxSettings`, `TabSettings`, `WindowSettings`,
`ProjectSettings`. Also declares `extern RGBTriplet<float>
kMainWindowBackgroundColor` (defined in `project_settings.cpp`) and the free
function `areDerivedElementEqual` (needed because comparing two
`shared_ptr<ElementSettings>` needs a `dynamic_pointer_cast` to the actual
concrete type before `operator==` does anything meaningful — see its
`switch` over `GuiElementType` in `project_settings.cpp`).

### `project_settings.cpp`
Implements `TabSettings`, `WindowSettings`, `ProjectSettings`, and
`areDerivedElementEqual`. `TabSettings`'s JSON constructor is the "element
factory" — a `Function`-style `if/else if` over `parseGuiElementType(j)`
that picks which concrete `ElementSettings` subclass to
`make_shared`-construct for each entry in a tab's `"elements"` array; throws
on an unrecognized type string. Contains two of the bugs documented in
`ARCHITECTURE.md`: `TabSettings::toJson()`'s copy-pasted color-default
comparisons, and `TabSettings::operator==`'s inverted equality check.
`ProjectSettings(file_path)` (the file-loading constructor) wraps the whole
parse in a `try/catch` that only logs to `std::cerr` on failure, leaving
`windows_` empty rather than propagating the error — a `.duoplot` file that
fails to parse silently produces an empty project rather than a visible
error dialog or thrown exception (check the caller, `MainWindow`, for
whether it does any post-hoc validation of the result).

### `element_settings.cpp`
`ElementSettings`'s three-constructor/`parseSettings`/`toJson`/`operator==`
implementation — see `ARCHITECTURE.md`'s "JSON shape conventions" for the
clamping behavior on `x`/`y`/`width`/`height`. `kZOrderDefault = -1` is
defined here as a file-local `constexpr`.

### `plot_pane_settings.cpp`
Implements `PlotPaneSettings` and `SubscribedStreamSettings`. Defines all
the `PlotPaneSettings` field defaults as named `constexpr` constants at file
scope (`kElementBackgroundColorDefault = hexToRgbTripletf(0x8EE6DA)`, etc.)
— these are the values `defaultInitializeAllSettings()` resets to and that
`toJson()` compares against to decide whether to emit a field.
`SubscribedStreamSettings`'s parse/serialize pair contains three of the bugs
listed in `ARCHITECTURE.md`: the unreachable `scatter_style` branch in
`toJson()`, the unconditional `color.value()` dereference, and the
`"long_dashed"`/`"LONG_DASHED"` casing mismatch. `stream_type`,
`line_style`, and `scatter_style` are each round-tripped through
hand-written string tables independently maintained in the constructor
(string → enum) and `toJson()` (enum → string) — check both directions
whenever adding a new enum value to any of `StreamType`, `LineStyle`, or
`ScatterStyle`.

### `scrolling_text_settings.cpp`
Implements `ScrollingTextSettings` and `SubscribedTextStreamSettings` (the
latter is the scrolling-text-pane equivalent of `SubscribedStreamSettings`
— much simpler, just a `TopicId` and an optional text color). No known bugs
found in this file specifically.

### `other_gui_settings.cpp`
Implements every remaining `GuiElementSettings` subclass:
`GuiElementSettings` itself, `ButtonSettings`, `CheckboxSettings`,
`EditableTextSettings`, `DropdownMenuSettings`, `ListBoxSettings`,
`RadioButtonSettings`, `RadioButtonGroupSettings`, `TextLabelSettings`,
`SliderSettings`, `StaticBoxSettings`. Notable details:
- `GuiElementSettings::id` (the numeric ID used when
  `publish_to_serial` is true — presumably for routing values to
  `serial_interface`, outside this module) is only read from JSON when
  `publish_to_serial` is `true`; if a file has `publish_to_serial: false`
  but still includes an `"id"` key, that value is silently ignored rather
  than validated or preserved.
- `DropdownMenuSettings`'s constructor validates that
  `initially_selected_item` actually appears in `elements`, resetting it to
  `""` if not (defensive parsing — one of the few places in this module
  that does this kind of cross-field validation).
- `RadioButtonGroupSettings::operator==` and `StaticBoxSettings::operator==`
  are the two additional bugs documented in `ARCHITECTURE.md` (missing
  bounds check; comparing against the wrong container's `end()`).
- `StaticBoxSettings::toJson()` never writes `label` — see `ARCHITECTURE.md`.
- `SliderSettings`'s vertical/horizontal style support is present in the
  types (`is_horizontal`) but its (de)serialization is fully commented out
  — see `ARCHITECTURE.md`.
- `StaticBoxSettings` is the only `ElementSettings` subclass besides
  `TabSettings` that nests other `ElementSettings` (a static box can
  contain buttons, checkboxes, sliders, other static boxes, etc. — but
  notably **not** `PlotPaneSettings`, unlike `TabSettings`'s element
  factory, which does allow a plot pane at the top level of a tab). Its
  element-type dispatch (`if/else if` over `parseGuiElementType`) is a
  near-duplicate of `TabSettings`'s JSON constructor, independently
  maintained — adding a new nestable `GuiElementType` means updating both
  switch-like chains.

### `configuration_agent.h` / `configuration_agent.cpp`
`ConfigurationAgent` and `AppPreferences` — see `ARCHITECTURE.md`'s
dedicated section. `configuration_agent.h`'s templated `readValue<T>`/
`writeValue<T>` are the only templates in this module; both re-read the
whole file from disk on every call (`readValue` even does so once just to
check `is_valid_` state implicitly via the constructor's own read).
`createEmptyConfigurationFile()` writes literally `"{\n}\n"` — a valid but
minimal empty JSON object. Depends on `filesystem.h` (a `duoplot::filesystem`
namespace alias, presumably `std::filesystem` or a portability shim — not
part of this module, check its own definition if you need to know which)
and `platform_paths.h`'s `getConfigDir()` for the OS-specific config
directory location.

### `helper_functions.h`
Free functions shared by nearly every file above — see `ARCHITECTURE.md`'s
"JSON shape conventions" section for `getOptionalValue`/`assignIfNotDefault`/
`colorToJsonObj`/`jsonObjToColor`. Also: `hexToRgbTripletf` (a `constexpr`
0xRRGGBB-to-`RGBTriplet<float>` converter — the "correct", non-truncating
sibling of `misc/rgb_triplet.h`'s `RGBTriplet` hex constructor, which
hardcodes `float` math regardless of `T`; this one is explicitly typed for
`float` and used to define every project-state color default),
`parseGuiElementType`/`guiElementTypeToString` (the string ↔
`duoplot::GuiElementType` tables used by every element factory in this
module — the canonical list of valid `"type"` strings in a `.duoplot`
file), and `throwIfMissing` (declared but check current callers — not
observed in use in this module's other files at the time of writing; may be
a leftover from an earlier, less-defensive parsing style now superseded by
`getOptionalValue`/`.contains()` checks).

### `save_manager.h`
`SaveManager` — see `ARCHITECTURE.md`'s dedicated section for its
dirty-tracking reliability caveat. Entirely header-only/inline; no
corresponding `.cpp` file. `getCurrentFileName()` extracts the filename
from `file_path_` via manual `find("/")` scanning rather than a path
library call — will not produce a sensible result for a Windows-style
`\`-separated path, though the rest of this codebase already assumes POSIX
paths elsewhere (see `data_receiver.h`'s direct POSIX socket includes, etc.)
so this is consistent with the codebase's current platform assumptions
rather than a new one.
