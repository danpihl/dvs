# Architecture improvement backlog

This is a running list of structural issues in `new_qt_duoplot` worth
addressing now that the codebase has a working, committed baseline and the
work is shifting from "faithful port of wx" to "improve on what wx did."
Unlike `CURRENT_STATE.md` (a record of what's been built and verified) and
`AUDIT_OF_PRIOR_ATTEMPT.md` (a frozen historical record), this file is a
forward-looking backlog — add to it as new issues surface, check items off
(with a short note on what changed and why) as they're addressed.

## 1. Window/tab bookkeeping via parallel vectors — DONE (2026-09-20)

`MainWindow` keeps `windows_` (`std::vector<GuiWindow*>`) and
`window_buttons_` (`std::vector<QPushButton*>`) as separate vectors, kept
in sync purely by convention (insert into both at the same index, erase
from both at the same index). `GuiWindow` does the same for
`tabs_`/`tab_button_widgets_`. This already caused two real bugs this
session:

- A tab-button `clicked` lambda captured an *index* into `tabs_` by value;
  the index went stale for every button after one removed by `deleteTab()`.
- `GuiWindow::layoutTabButtons()` never called `show()` on tab buttons — a
  widget only inherits visibility from its parent's hidden→visible
  *transition* at the moment it happens, so buttons added after the panel
  was already visible (every tab from the 3rd onward) silently never
  appeared, even though the backend (`tabs_`) was completely correct.

Both were fixed as one-off patches, but the underlying structure (two
vectors that must be kept in lockstep by hand) is still there and will
keep producing this class of bug. Fix: replace each pair with a single
vector of a small struct pairing the object with its button, so insertion/
removal is naturally atomic and there's no index to go stale.

**Done.** `GuiWindow::tabs_`/`tab_button_widgets_` merged into
`std::vector<TabEntry>` (`TabEntry{WindowTab* tab, QPushButton* button}`),
and `MainWindow::windows_`/`window_buttons_` merged the same way into
`std::vector<WindowEntry>`. Every insertion (`addTabButton`/
`addWindowButton`) now does one `push_back` of a complete entry instead of
two separate `push_back`s that could drift; every removal (`deleteTab`/
`deleteWindow`) does one `erase` instead of two index-matched ones.
Lookups that used to walk an index across two vectors in lockstep
(`editTabName`, `windowNameChanged`, `getSelectedTab`, etc.) now do a
single `std::find_if` over one vector and use `entry.tab`/`entry.button`
directly. Rebuilt cleanly; 5x repeated launch with no crash; a real
2-window/4-tab/multiple-plot-pane project file loaded correctly via
`setupWindows` immediately afterward, exercising both refactored
structures at once under real (not synthetic) data.

## 2. Callback wiring is very wx-shaped, not very Qt-shaped — DONE (2026-09-20)

Every `GuiWindow`/`WindowTab`/`GuiElement` constructor threads 6-9
`std::function` members down 3-4 levels
(`MainWindow → GuiWindow → WindowTab → GuiElement`/`PlotPane`), all wired
up once at construction and never touched again. This exists because
`GuiElement` is a plain (non-`QObject`) mixin — inherited directly from
wx's multiple-inheritance trick for `ApplicationGuiElement`
(`class ButtonGuiElement : public QPushButton, public GuiElement`), which
means `GuiElement` itself can't have real Qt signals/slots.

This is the single biggest thing blocking a more idiomatic design. Real
Qt signals/slots (or a lightweight event-bus with actual `QObject`
identity) would collapse most of the callback-threading. It's also the
riskiest item on this list: `GuiElement` not being a `QObject` is load-
bearing for every concrete element class (multiple inheritance from a
`QWidget` subclass + a `QObject` subclass at once is exactly what Qt's
moc doesn't support), so touching it means touching every element type,
not just the base class.

**Done, via the lower-risk of the two options considered** (explicitly
chosen over a `QObject`-based event bus — see the conversation this was
decided in): a single `GuiCallbacks` struct (`gui_callbacks.h`) bundling
all 10 callback fields (`key_pressed`, `key_released`,
`right_mouse_pressed`, `tab_about_editing`, `element_deleted`,
`get_all_element_names`, `element_name_changed`, `name_changed`,
`about_modification`, `push_text_to_cmdl_output_window`), passed by
`const&` through every constructor instead of 6-9 separate `std::function`
parameters. Each layer stores one `GuiCallbacks callbacks_;` member
instead of 6-9 named ones:

- `MainWindow` constructs the 8 fields it originates (leaving
  `right_mouse_pressed`/`tab_about_editing` empty — it doesn't participate
  in either) and passes `callbacks_` to every `GuiWindow` it creates (4
  call sites: `setupWindows`, `bootstrapDefaultProject`,
  `newWindowWithoutFileModification` ×2).
- `GuiWindow` copies the incoming bundle, overwrites `right_mouse_pressed`
  with a lambda wrapping its own `mouseRightPressed()`, and passes the
  augmented copy to every `WindowTab` it creates.
- `WindowTab` copies *that*, overwrites `tab_about_editing` with a lambda
  tied to its own `editing_silhouette_`, and passes the fully-populated
  copy to every `GuiElement`/`PlotPane` it creates.
- `GuiElement` (and by inheritance every concrete element type — all 8 in
  `gui_elements.h`/`.cpp` plus `PlotPane`) stores the final bundle as-is
  and calls e.g. `callbacks_.about_modification()` where it used to call
  `notify_main_window_about_modification_()`.

Net effect: `GuiElement`'s constructor went from `(settings, 6 callback
params)` to `(settings, callbacks)`; `WindowTab`'s from `(parent, settings,
6 callback params)` to `(parent, settings, callbacks)`; `GuiWindow`'s
similarly collapsed from 8 callback params to 1. No behavior changed —
this is a pure mechanical restructuring of how the same data flows, not a
new mechanism (still plain function calls, not real Qt signals — the
`QObject`/multiple-inheritance constraint on `GuiElement` is unchanged and
still blocks that path if it's ever wanted later).

Rebuilt cleanly layer by layer (`GuiElement`+concrete elements → `WindowTab`
→ `GuiWindow` → `MainWindow`, matching the dependency order, catching each
layer's own compile errors before moving to the next). Final build clean,
5x repeated launch with no crash, and the same real 2-window/4-tab/
multiple-plot-pane project file used to verify item #1 loaded correctly
again afterward, confirming every hop of the re-threaded callback chain
still reaches the right place.

## 3. Data-flow path has one coarse lock doing double duty — DONE (2026-09-20)

`MainWindow`'s TCP receive thread parses and converts plot data inline,
then queues the result into a single `std::map<std::string, std::queue<...>>`
(`queued_data_`) guarded by one mutex (`receive_mtx_`) for *all* elements.
The 10ms `receive_timer_` drains the whole map under that same lock on the
main thread. This matches wx's own design (not a regression), but it means
a slow drain for one element's queue blocks new data arriving for every
other element, and vice versa.

**A more serious, unrelated bug was found while investigating this**:
`receive_mtx_` doesn't only guard `queued_data_` — `setActiveView()` (TCP
thread, called from inside the same locked dispatch) also reads
`plot_panes_`. But `plot_panes_`/`gui_elements_` were being **written from
the main/GUI thread with no lock at all** in several places
(`deleteWindow`, `elementDeleted`, `setupWindows`,
`newWindowWithoutFileModification` ×2, `elementNameChanged`,
`removeAllWindows`, `bootstrapDefaultProject`). A `std::map` concurrently
read (locked) on one thread and written (unlocked) on another is undefined
behavior, independent of anything about `queued_data_`'s coarseness. Fixing
item 3's stated concern in isolation would have meant moving
`receiveData()`'s `plot_panes_`-reading drain loop out from under
`receive_mtx_` too — which would have made this *already-existing* race
easier to hit (a 10ms-timer code path instead of only occasional menu
actions) without actually fixing it. Surfaced to the user before proceeding;
they asked for both to be fixed together rather than scoping down.

**Fix**:
- Every main-thread write site of `plot_panes_`/`gui_elements_` listed
  above now wraps just its own map mutation in `receive_mtx_` (not the
  whole surrounding function — e.g. `setupWindows`'s `GuiWindow`
  construction and `show()` calls stay unlocked; only the loop populating
  the two maps is wrapped). This makes every touch-point — TCP-thread reads
  in `setActiveView`/`handleGuiManipulation`/`updateClientApplicationAboutGuiState`
  (already under `receive_mtx_` via `manageReceivedData`'s existing outer
  lock, untouched) and GUI-thread writes (newly wrapped) — consistent.
- `queued_data_` got its own dedicated `queued_data_mtx_`, separate from
  `receive_mtx_`. `addActionToQueue`'s three push sites and
  `mainWindowFlushMultipleElements`'s push loop now lock
  `queued_data_mtx_` instead of relying on the outer `receive_mtx_` alone
  (nested inside it — safe, consistent lock order, no cycle since
  `receiveData()` never holds both at once).
- `receiveData()` was restructured: the `open_project_file_queued_`/
  `new_window_queued_`/`current_element_name_` flags are read-and-reset
  under a brief `receive_mtx_` lock into local variables, then
  `openExistingFile()`/`newWindowWithoutFileModification()` are called
  **without** holding any lock (each now takes `receive_mtx_` itself,
  briefly, only around its own map mutation — see above). `queued_data_` is
  swapped into a local map under `queued_data_mtx_` (O(1)), then drained
  unlocked; the per-element `PlotPane*` lookup needed for each drained
  queue takes a brief `receive_mtx_` lock of its own, but the actual
  `pushQueue()` call happens outside any lock. Net effect: the TCP thread
  is no longer blocked for the entire duration of window
  construction/project loading/multi-element draining, only for the brief
  individual map accesses that genuinely need exclusion.
- Deliberately left alone: `manageReceivedData`'s outer `receive_mtx_` hold
  for the whole dispatch (narrowing that further would need restructuring
  which branches actually need the lock and is a bigger, separate change
  for more modest gain, since the TCP thread is single and serial anyway —
  there's no other TCP-thread work it could be doing concurrently with
  itself); and the pre-existing, out-of-scope observation that
  `performScreenshot()` calls `GuiWindow::screenshot()` (a Qt widget/paint
  operation) from the TCP thread rather than the GUI thread — a separate
  Qt-thread-affinity question, not touched here.

**Verified**: rebuilt cleanly, 5x repeated launch with no crash. Ran the
full `system-test cpp dynamic_plotting all` suite (dozens of scenarios,
including sustained streaming plot-data updates that exercise
`queued_data_`'s push/drain path continuously) against a live
`new_qt_duoplot` — every scenario up to the point of killing the test (an
interactive one requiring manual input) reported "ran successfully," and
the server process was still alive and responsive throughout, confirming
the new locking survives real, sustained traffic rather than just a quiet
startup.

## 4. `MainWindow` is a God object — DONE (2026-09-20)

Owns `DataReceiver`, `SerialInterface`, `ConfigurationAgent`,
`SaveManager`, all window-lifecycle bookkeeping, all TCP message dispatch/
parsing, *and* acts as the cross-class callback hub for `GuiWindow`'s
popup-menu system (`deleteWindow`, `newWindow`, `printGuiCallbackCode`).
Matches wx's own `MainWindow` exactly, so not a regression, but a good
candidate for splitting once there's appetite for it — e.g. a dedicated
message-dispatcher/router separate from window-lifecycle management, so
each piece is independently testable.

**Scoped and done: the TCP receive/dispatch extraction.** Presented two
options — extract just the message-routing piece, or a full 3-way split
also pulling window/element lifecycle into a `WindowManager`. The user
picked the smaller, lower-risk cut. Window/element lifecycle and project
save/load (`newWindow`/`deleteWindow`/`setupWindows`/`newProject`/
`saveProject`/`openExistingFile`/etc.) all stay in `MainWindow` for now —
only the network I/O + wire-protocol dispatch moved.

New `MessageRouter` (`message_router.h/.cpp`) now owns: `DataReceiver`, the
TCP receive thread, `queued_data_`/`queued_data_mtx_`, and all dispatch
logic (`manageReceivedData`, `addActionToQueue`, `handleGuiManipulation`,
`mainWindowFlushMultipleElements`, `setActiveView`, `sendGuiStateToClient`
— the last one renamed from `updateClientApplicationAboutGuiState`, moved
verbatim). It has no knowledge of `GuiWindow`, `WindowEntry`,
`SaveManager`, or any Qt widget besides `PlotPane` (needed to call
`pushQueue()` directly on drained data). Its own `flags_mtx_` protects
`new_window_queued_`/`current_element_name_`/`open_project_file_queued_`/
`queued_project_file_name_` — all fully moved from `MainWindow`, along with
the `poll()` method (replacing the queued-data half of the old
`receiveData()`; `handleSerialData()` stays in `MainWindow`, since it's
tied to `SerialInterface`, not the wire protocol).

Everything `MessageRouter` needs from `MainWindow`'s remaining
window/element bookkeeping goes through one `MessageRouterCallbacks`
struct (mirroring `GuiCallbacks`'s bundling pattern from item #2):
`find_plot_pane`, `set_element_label`, `get_all_gui_element_states`,
`perform_screenshot` (invoked from `MessageRouter`'s TCP thread — each
implementation in `MainWindow` locks its own `receive_mtx_` internally,
since the two classes now run on different threads with no shared lock
between them), plus `open_project_file`/`create_new_window_for_element`
(invoked from `poll()` on the GUI thread). `MainWindow::receive_mtx_` now
protects only `plot_panes_`/`gui_elements_` — its old dual purpose (also
guarding the flags) moved to `MessageRouter::flags_mtx_` along with the
flags themselves.

`MainWindow`'s constructor builds the callback bundle, constructs
`message_router_`, and calls `message_router_->start()` in place of
spawning the TCP thread itself; the `receive_timer_` callback calls
`message_router_->poll()` in place of the old `receiveData()`.
`~MainWindow()` now deletes `message_router_` (added for symmetry with
deleting `save_manager_`/`configuration_agent_` — the TCP thread inside it
is still never joined, matching the pre-existing, unfixed "no clean
shutdown path" behavior this was extracted from).

**Verified**: clean build on the first attempt (no iteration needed to fix
compile errors post-extraction). 5x repeated launch, no crash. The same
real 2-window/4-tab/multiple-plot-pane project loaded correctly via
`setupWindows` (now reached through the `open_project_file`/
`create_new_window_for_element` callbacks rather than direct calls). Ran
the full `system-test cpp dynamic_plotting all` suite again — same result
as item #3's verification: every scenario up to the interactive one
reported "ran successfully," server alive and responsive throughout,
confirming `MessageRouter`'s TCP thread and dispatch work correctly under
sustained real traffic after being fully relocated to a new class.

**Follow-up, also done: the deferred `WindowManager` split.** The
window/element-lifecycle half deliberately left in `MainWindow` above
(`WindowEntry`, `plot_panes_`/`gui_elements_`, `newWindow`/`deleteWindow`/
`setupWindows`/`removeAllWindows`/`bootstrapDefaultProject`/
`elementDeleted`/`elementNameChanged`/`notifyChildrenOnKeyPressed`/
`notifyChildrenOnKeyReleased`/`toggleWindowVisibility`/
`getAllElementNames`/`printGuiCallbackCode`/`layoutWindowButtons`'s
per-window part) has since been extracted into a new `WindowManager`
(`window_manager.h/.cpp`), on explicit user request as a direct follow-up
to the `MessageRouter` split above. `WindowManager : public QObject` owns
`WindowEntry`/`window_entries_`, `plot_panes_`/`gui_elements_` (with its
own `elements_mtx_`, replacing `MainWindow`'s now-removed `receive_mtx_`),
`current_window_num_`, `window_callback_id_`, a stored `GuiCallbacks`
copy, a `MainWindow*` (passed through to every `new GuiWindow(...)` call
so `GuiWindow`'s popup-menu `static_cast<MainWindow*>(main_window_)` calls
keep working unchanged), and a read-only `SaveManager*` (not owned —
`MainWindow` still owns and constructs it). `window_initialization_in_progress_`
deliberately stayed in `MainWindow`, since only `MainWindow`'s own
`newWindow()`/`newProject()`/`openExistingFile()` toggle it around thin
calls into `WindowManager`.

`MainWindow` now holds `WindowManager* window_manager_` and every public
window/element method (`newWindow`, `deleteWindow`, `elementDeleted`,
`elementNameChanged`, `notifyChildrenOnKeyPressed/Released`,
`toggleWindowVisibility`, `getAllElementNames`, `printGuiCallbackCode`) is
a one-line delegator to it; project methods (`newProject`/`saveProject`/
`saveProjectAs`/`openExistingFile`) call `WindowManager`'s
`resetForNewProject`/`getCurrentProjectSettings`/
`setIsFileSavedForAllWindows`/`setProjectNameForAllWindows`/
`removeAllWindows`/`setupWindows` instead of touching the old maps/vectors
directly. `MessageRouterCallbacks`' `find_plot_pane`/`set_element_label`/
`get_all_gui_element_states`/`perform_screenshot` now delegate straight to
`WindowManager`'s own thread-safe accessors (each locking
`elements_mtx_` internally) instead of `MainWindow` locking `receive_mtx_`
itself — `MainWindow` no longer has a `receive_mtx_` at all.

**Verified**: clean build on the first attempt. 5x repeated launch, no
crash. Real 2-window/4-tab/multiple-plot-pane project loaded correctly.
Running the full `system-test cpp dynamic_plotting all` suite to
*completion* (not just partially, as earlier passes had always been
manually killed before reaching every scenario) surfaced a real,
previously-undetected bug — see the screenshot-crash write-up below. The
`WindowManager` split itself is not implicated: the crash is in
long-pre-existing thread-affinity behavior, unchanged by this extraction
beyond moving where the affected method physically lives.

### Screenshot-from-TCP-thread crash — found and fixed (2026-09-20)

While finally running `system-test cpp dynamic_plotting all` through to
completion (previous verification passes for items #3 and #4 always ended
with a manual kill partway through), the server crashed with
`EXC_BAD_INSTRUCTION`/SIGILL during the `basic::screenshot` scenario.
Parsing the resulting macOS crash report
(`~/Library/Logs/DiagnosticReports/new_qt_duoplot-*.ips`) showed the
crashed thread's backtrace as `MessageRouter::tcpReceiveThreadFunction` →
`manageReceivedData` → the `perform_screenshot` lambda →
`WindowManager::performScreenshot` → `GuiWindow::screenshot()` →
`QWidget::raise()` → AppKit window-ordering internals → crash.

This was already flagged as an out-of-scope, pre-existing observation in
both item #3's and item #4's write-ups above ("`performScreenshot()` calls
`GuiWindow::screenshot()` ... from the TCP thread rather than the GUI
thread"), but had never actually been triggered in a verification run
before, since every prior pass was manually terminated before the test
suite reached the screenshot scenario. It is not a regression from the
`WindowManager` split or anything else done today — the call site's
threading was identical before any of this session's refactors.

**Root cause**: `QWidget::raise()` (and other real Qt/AppKit widget
operations) are not thread-safe off the GUI thread. `SCREENSHOT` messages
were being dispatched and acted on synchronously, inline, from
`MessageRouter`'s TCP thread.

**Fix**: queue the screenshot request the same way `open_project_file_queued_`/
`new_window_queued_` already were. `MessageRouter` gained
`screenshot_queued_`/`queued_screenshot_path_` (protected by the existing
`flags_mtx_`); `manageReceivedData`'s `Function::SCREENSHOT` branch now
just sets these under lock instead of calling
`callbacks_.perform_screenshot(path)` directly; `poll()` (GUI thread, via
`receive_timer_`) reads-and-resets the flag under `flags_mtx_` and calls
`callbacks_.perform_screenshot(path)` unlocked, alongside the existing
`open_project_file`/`create_new_window_for_element` drains.
`MessageRouterCallbacks::perform_screenshot` is now documented as
GUI-thread-only, matching the other two.

**Verified**: rebuilt cleanly. 5x repeated launch, no crash. Re-ran
`system-test cpp dynamic_plotting all` — the `basic::screenshot` scenario
(the exact one that crashed before) now completes successfully, and the
server survived through the entire suite (streaming, object-transform,
and screenshot scenarios all included) rather than crashing partway
through.

## 5. Single-slot "auto-create window for unseen element" flag could drop data — DONE (2026-09-20)

The client's "current element" model (`setCurrentElement`, all subsequent
plot/property commands implicitly target it) is deliberately kept as-is —
that's the client-facing contract. On the application side, though,
`MessageRouter::setActiveView()` (handling `SET_CURRENT_ELEMENT`) had a
subtle bug in how it signals "this element name has never been seen
before, please auto-create a window for it": that signal was a single
`new_window_queued_` bool + `current_element_name_` string, not a
collection. If a client set two or more *never-before-seen* elements as
current within the same ~10ms `poll()` interval (plausible for a script
that creates several new named views back-to-back in a tight loop before
the GUI thread gets a chance to run), only the last one actually got its
window auto-created on the next `poll()` — the earlier name(s)' queued
plot data sat in `queued_data_` keyed by a name with no matching
`PlotPane`, and `poll()`'s drain loop silently discarded it (`find_plot_pane`
returned `nullptr`, no log, data just vanished). This was a genuine bug,
not a wx-inherited one — it was introduced by this session's item #4
`MessageRouter` extraction reusing the pre-existing single-flag pattern
verbatim rather than a limitation of the original design being ported.

**Fix**: replaced `new_window_queued_`/reliance on `current_element_name_`
for this purpose with `pending_new_window_names_`, a
`flags_mtx_`-protected `std::vector<std::string>`. `setActiveView()`
appends to it (deduplicated — `std::find` before `push_back`, so the same
name queued twice before a `poll()` runs doesn't double-create). `poll()`
swaps the whole vector out under lock, then — before touching
`queued_data_` at all — creates a window for each pending name, with a
defensive re-check (`find_plot_pane(name) == nullptr`) immediately before
each creation call, since an earlier name in the same batch (or something
else entirely, like a manual "new window" click) could have already
created a matching `PlotPane` by then. `current_element_name_` itself is
unchanged — it still tracks the single routing target for subsequent
messages, which is correct as-is since the TCP thread processes messages
strictly in order.

Also added a `LUMOS_LOG_WARNING()` in `poll()`'s existing data-drain loop
for the general case of `find_plot_pane` returning `nullptr` for a name
with queued data — this exact bug's failure mode (and any other future
cause of orphaned queued data, e.g. a stale/typo'd element name) was
previously completely silent; now it's at least visible in the logs.

**Verified**: rebuilt cleanly, zero errors. 5x repeated launch, no crash.
Re-ran `system-test cpp dynamic_plotting all` against a live instance —
server alive and responsive throughout. This run happened to reproduce the
exact race the fix targets, for real: the suite's `openProjectFile` test
uses a hardcoded path from another machine
(`/Users/danielpi/work/dvs/project_files/car.duoplot`, a known-stale path —
see `AUDIT_OF_PRIOR_ATTEMPT.md`'s Phase 0 note), and while its `setActiveView`/
`clearView` teardown loop fired off `SET_CURRENT_ELEMENT` for four
never-before-seen names (`p_view_0`, `p_view_1`, `p_view_2`,
`w1_p_view_0`) back-to-back faster than one `poll()` interval, the log
shows all four windows created together in a single batch (`Window 4`
through `Window 7`) — exactly the multi-simultaneous-new-element case this
fix handles; pre-fix, only the last of the four would have gotten a
window.

**The new warning log also surfaced a second, separate, previously-silent
bug**, unrelated to the one just fixed: `s_view_0`/`s_view_1`/`s_view_2`/
`w1_p_view_1`/`p1`/`p_view_0`/`p_view_1`/`p_view_2` (at different points in
the run) had their queued data discarded even though a window *was*
eventually created for some of them. Root cause: `poll()` processes
`should_open_project_file` *before* draining `queued_data_`, and
`openExistingFile()` calls `window_manager_->removeAllWindows()`
unconditionally, even when the file it's about to load turns out to be
invalid (the suite's hardcoded project paths don't resolve on this
machine, so every `openProjectFile` call in the run failed to parse and
fell back to zero windows). Any plot data queued for elements of the *old*
project between the client sending `OPEN_PROJECT_FILE` and the next
`poll()` tick — data that was completely legitimate when it was queued,
since those elements still existed at the time — gets orphaned the moment
`removeAllWindows()` runs, and silently discarded when the drain loop
can't find a `PlotPane` for it anymore. This is not caused by, or a
regression from, today's fix (the affected names' panes existed at
`SET_CURRENT_ELEMENT` time, so `pending_new_window_names_` was never
involved) — it's a pre-existing race that was always losing data this way,
just invisible before this session's warning log existed. Not fixed here;
noted as a new backlog candidate below.

## 6. `openExistingFile()` can orphan in-flight queued data — DONE (2026-09-22)

Found while verifying item #5 above (see its verification note for the
full trace). `MessageRouter::poll()` always processes a queued
`OPEN_PROJECT_FILE` request before draining `queued_data_`, and
`MainWindow::openExistingFile()` called `window_manager_->removeAllWindows()`
unconditionally before attempting to parse the new file — including when
that parse then fails and `setupWindows()` falls back to zero windows.
Any plot data for the *old* project's elements that was queued between the
client issuing `OPEN_PROJECT_FILE` and the next `poll()` tick was
legitimate when queued but got orphaned the instant the old windows were
torn down, and silently dropped by the drain loop's `find_plot_pane` ==
nullptr check (as of item #5, at least logged as a warning, not silent).

**Confirmed this exactly matches wx's own original behavior** —
`main_application/main_window.cpp:984-1007`'s `MainWindow::openExistingFile`
calls `removeAllWindows()` unconditionally too, before
`save_manager_->openExistingFile(file_path)`, with no rollback if the
parse fails (`ProjectSettings`'s file-loading constructor,
`project_settings.cpp:365-382`, byte-identical between `main_application`
and `new_qt_application`, swallows the parse exception and just leaves an
empty project either way). So this wasn't a Qt-port regression — it's a
pre-existing wx wart, invisible there for the same reason it was invisible
here before item #5's warning log existed. Fixing it is a deliberate
improvement beyond wx, in line with this whole document's stated shift
from "faithful port" to "improve on what wx did."

**Fix**: `ProjectSettings` gained an `is_valid_` bool (default `true`; set
`false` in the file-loading constructor's existing `catch` block) and an
`isValid()` accessor — the smallest possible addition, since the
parse-success information was already being computed and discarded right
there. (Originally named `loaded_successfully_`/`loadedSuccessfully()`;
renamed to match `ConfigurationAgent::isValid()`'s naming for the same
"did this object load correctly" concept — see item #8's note.)
`MainWindow::openExistingFile(file_path)` now constructs a
`ProjectSettings` from the target path *first*, before touching
`window_manager_` or `save_manager_` at all; if `isValid()` is false, it
logs a warning and returns immediately
— the current project (windows, panes, any in-flight queued data for
them) is left completely untouched. Only on a successful parse does it
proceed with the original sequence (`removeAllWindows()` →
`configuration_agent_`/`save_manager_` update → `setupWindows()`). This
means the file gets parsed twice on the success path (once for the
up-front check, once inside `save_manager_->openExistingFile()`) — a
deliberate, minor cost to avoid changing `SaveManager`/`ProjectSettings`'s
existing call shape any further than the one new accessor.

**Verified**: rebuilt cleanly, zero errors. 5x repeated launch, no crash.
Re-ran `system-test cpp dynamic_plotting all` — the same `openProjectFile`
test's three hardcoded stale paths (`car.duoplot`, `legend.duoplot`,
`particles.duoplot`) that previously each destroyed the loaded 2-window
project and orphaned in-flight queued data now each log `Failed to open
project file '...' — current project left unchanged.`, with **no**
`PlotPane destroyed` messages following any of the three — confirming the
specific case this fix targets (a bad path no longer touches the current
project at all) actually works, not just logs more.

**Caveat found during this same verification, correctly out of scope for
this fix**: shortly after, the suite's next `openProjectFile` call uses a
path that *does* resolve and parses successfully — a legitimate project
switch, which (correctly) still runs `removeAllWindows()` and does destroy
the old project. Item #5's discard warnings recur right there, for the
same `p_view_0`/`p_view_1`/`p_view_2`/`s_view_0..2`/`w1_p_view_0`/
`w1_p_view_1` names, because the same `setActiveView`/`clearView` teardown
loop had queued data for them moments before the (valid) switch completed.
This is expected, not a bug this item was meant to fix: item #6 only
protects against a *failed* load discarding the current project; when the
client legitimately switches to a different, valid project, dropping any
plot data that was in flight for the *old* project's elements at that
moment is arguably correct (there's no reason to keep updating plots that
belong to a project the client just replaced). Left as-is.

## 7. `ConfigurationAgent` had two latent correctness bugs — DONE (2026-09-23)

Found while assessing `project_state` as a module in isolation. Almost the
entire module (`project_settings.*`, `plot_pane_settings.cpp`,
`element_settings.cpp`, `scrolling_text_settings.cpp`, `helper_functions.h`,
`save_manager.h`, `configuration_agent.*`) is a byte-identical carryover
from `main_application` — confirmed by diffing every file, with the one
exception being a prior session's already-applied fix to
`RadioButtonGroupSettings`'s default constructor
(`other_gui_settings.cpp`). `main_application/project_state/docs/
ARCHITECTURE.md` already catalogs 7 verified, still-unfixed serialization/
equality bugs inherited into this copy unchanged (inverted `operator==`
logic breaking `SaveManager`'s dirty-check, an unreachable JSON branch, an
unconditional `std::optional::value()` that can throw, a case-mismatch
round-trip bug, two `operator==` UB bugs, and a field silently never
serialized) — noted here as a backlog candidate, not fixed in this pass
(several would benefit from checking against real saved `.duoplot` files
before touching the on-disk format).

Two further bugs, specific to `ConfigurationAgent` and not called out in
the wx docs, were fixed directly:

- **`writeValue<T>()` had no try/catch around its `readConfigurationFile()`
  call**, unlike `readValue`/`hasKey`, which both wrap the identical call.
  A corrupted or externally-modified `configuration.json` at the moment of
  a write would throw straight out of `writeValue` uncaught — no call site
  in `MainWindow` wraps `configuration_agent_->writeValue(...)` — crashing
  the app instead of degrading gracefully like every other accessor here.
- **`readValue<T>()`'s "invalid agent" fallback was `T data = 0; return
  data;`.** For `T = std::string` — exactly how it's called for
  `"last_opened_file"` — this implicitly treats `0` as a null `const
  char*` and constructs a `std::string` from it: undefined behavior, not
  an empty string. Only reachable when `ConfigurationAgent` failed to
  initialize at all (e.g. couldn't create its config directory — a
  broken/read-only environment), which is exactly the scenario where a
  graceful empty-value fallback matters most instead of a crash.

**Fix**: `writeValue<T>()`'s `readConfigurationFile()` call is now wrapped
in the same try/catch pattern as `readValue`/`hasKey` (log, set
`is_valid_ = false`, return early). Both of `readValue<T>()`'s "default
value" spots (`T data = 0;` in the invalid-agent branch, and the
originally default-initialized `T data;` before the file-read try/catch)
are now `T data{};` — value-initialization, which is the correct empty
default for every `T` this template is instantiated with (`std::string`
→ `""`, `int`/`bool` → `0`/`false`), and also fixes an adjacent,
previously-unnoticed issue: `T data;` alone leaves a built-in `T`
uninitialized, so a missing/unreadable key could have returned garbage
instead of a deterministic default even on the already-handled path.

**Verified**: rebuilt cleanly, zero errors. 5x repeated launch, no crash.
Confirmed `configuration.json`'s normal read/write path (`last_opened_file`,
`visualization_period_ms`) still round-trips correctly post-fix. The two
bugs themselves only fire on the invalid-agent/corrupted-file paths, which
aren't exercised by normal operation — verified by code inspection rather
than forcing those failure modes (would require deliberately corrupting or
permission-locking the config directory).

## 8. `ConfigurationAgent`'s external interface was generic where it should be typed — DONE (2026-09-23)

Found while assessing the module architecturally, separate from item #7's
internal bugfixes. The public interface was `bool hasKey(key)` +
`template <typename T> T readValue(key)` / `template <typename T> void
writeValue(key, val)` — a generic JSON key/value store. But the actual
usage, all in `main_window.cpp`, is exactly two hardcoded keys with
hardcoded types: `readValue<std::string>("last_opened_file")` and
`readValue<int>("visualization_period_ms")`, always paired with a
`hasKey()` check first. The generic interface bought nothing for this
usage and cost real things:

- No compile-time check that a key string is spelled consistently or read
  back with the same `T` it was written with — every failure mode (typo,
  wrong type) is a silent runtime default, not a compiler error.
- `hasKey(key)` + `readValue<T>(key)` is two full file reads/parses for
  one logical "get if present" — `ConfigurationAgent` had no in-memory
  cache at all; every accessor round-tripped the whole file from disk.
- The one place that actually knew what a *valid* `visualization_period_ms`
  looks like (`1-100`, default `10`) was the call site in
  `MainWindow::getVisualizationPeriodMs()`, not `ConfigurationAgent` —
  duplicated logic sitting one layer away from the data it constrains.
- `AppPreferences` (a plain struct: `last_opened_file`, 
  `open_main_window_on_start`) sat in the header, unused by anything,
  documenting the *shape* of a typed contract that was never actually
  wired up as one.

**Fix**: replaced the generic templates with typed accessors for the two
settings that actually exist — `std::optional<std::string>
getLastOpenedFile() const` / `void setLastOpenedFile(const std::string&)`,
and `int getVisualizationPeriodMs() const` / `void
setVisualizationPeriodMs(int)`, the latter owning its own `1-100`
clamp/`10` default internally now instead of at the call site. Added an
in-memory `cache_` (the parsed JSON), loaded once at construction and
written straight through on every `set*` call — collapses every
`hasKey`+`readValue` pair into one call and one avoided round-trip, with
no change to the "always persisted immediately" behavior callers already
relied on. Removed the now-fully-superseded `hasKey`/`readValue`/
`writeValue` templates and the unused `AppPreferences` struct entirely,
rather than keeping them alongside the new accessors — there was no
remaining caller for the generic path once `main_window.cpp`'s three call
sites were migrated, and no reason to keep an alternate, weaker-typed way
to reach the same two settings. `MainWindow::getVisualizationPeriodMs()`
(the private wrapper that owned the clamp/default logic) was deleted; its
two call sites now call `configuration_agent_->getVisualizationPeriodMs()`
directly.

**Verified**: rebuilt cleanly, zero errors. 5x repeated launch, no crash.
Confirmed the real, persisted `configuration.json`
(`last_opened_file: "../../project_files/exp0.duoplot"`,
`visualization_period_ms: 16`) still round-trips correctly through the new
typed accessors — the log shows the same 2-window project loading at
startup via `getLastOpenedFile()`, and `16` (within the `1-100` clamp)
being applied to `receive_timer_` via `getVisualizationPeriodMs()`, both
exactly matching pre-refactor behavior.

**Follow-up (2026-09-23)**: noticed, while comparing this class's
`isValid()` against item #6's `ProjectSettings::loadedSuccessfully()`,
that both answer the same "did this object load correctly" question with
different names. Renamed `ProjectSettings::loadedSuccessfully()`/
`loaded_successfully_` to `isValid()`/`is_valid_` to match — a pure rename
(one call site in `MainWindow::openExistingFile`), no behavior change.
Rebuilt cleanly, 3x repeated launch, no crash.

## 9. `newWindow()` was missing a `fileModified()` call, and project save/load didn't have its own class — DONE (2026-09-23)

Prompted by a direct question about whether `MainWindow`'s
newProject/saveProject/saveProjectAs/openExistingFile flow was in good
shape, given that some of this logic exists specifically to work around
timing/event quirks in wx that Qt might share. Investigated by diffing
against `main_application/main_window.cpp`'s equivalent methods.

**The core pattern is correctly and faithfully carried over.**
`window_initialization_in_progress_` exists to suppress spurious
`fileModified()`/`about_modification()` signals while windows/elements are
being constructed programmatically (setup, project load, popup-menu
element creation), with an explicit, deliberate `fileModified()` call
fired once construction completes, to mark the *real* semantic change.
This works identically in Qt to wx, since neither framework's widget
construction is asynchronous — there's no actual event-loop/threading
mismatch underlying this pattern, just synchronous C++ in both cases. Most
call sites already had it right: `WindowManager::deleteWindow()`, every
"New element" popup-menu callback in `gui_window.cpp`, and — a case where
the Qt port already **fixes** a real wx bug — `newProject()`'s "No, don't
discard my changes" branch, which in wx's original
(`main_window.cpp:922-934`) returns early on `wxNO` *without* resetting
`window_initialization_in_progress_`, permanently disabling the dirty-flag
for the rest of the process.

**One real gap found**: `MainWindow::newWindow()` was missing the trailing
`fileModified()` call that wx's own `newWindow()` has
(`main_application/main_window.cpp:704-712`):
```cpp
void MainWindow::newWindow()  // wx original
{
    window_initialization_in_progress_ = true;
    newWindowWithoutFileModification();
    window_initialization_in_progress_ = false;
    fileModified();   // <-- was missing in the Qt port
}
```
So clicking "New window" created the window but never marked the project
dirty. Currently zero observable impact — there's no File menu yet, so
`saveProjectAs()`/`openExistingFile()`'s no-arg dialog-driven overloads
(the only things that check `save_manager_->isSaved()`) are unreachable —
but a real latent bug: the moment a File menu is wired up, a user could
click "New window," then open a different project, and silently lose that
window with no "unsaved changes?" prompt.

**Fix**: added the missing `fileModified()` call back (now
`project_controller_->fileModified()`, see below).

**Also done, per explicit request**: extracted `fileModified()` and every
project-lifecycle method (`newProject`, `saveProject`, `saveProjectAs` ×2,
`openExistingFile` ×2) — plus `SaveManager` ownership and
`window_initialization_in_progress_` itself — out of `MainWindow` into a
new `ProjectController` (`project_controller.h/.cpp`), completing the
3-way split `MessageRouter`/`WindowManager` started (item #4). Design
notes:
- `window_initialization_in_progress_` moved here, not into
  `WindowManager`: it's fundamentally about whether a modification counts
  as dirty-worthy (this class's concern), not about window/element
  bookkeeping. `beginInitialization()`/`endInitialization()` are exposed
  publicly so `MainWindow::newWindow()` — which straddles both classes,
  asking `WindowManager` to build the window but `ProjectController` to
  bracket/fire the modification signal — can coordinate them, exactly
  mirroring wx's own `newWindow()` sequencing.
- `ProjectController` owns `SaveManager` outright now (previously
  `MainWindow`); `WindowManager` still takes a `SaveManager*` (unowned) at
  construction, sourced from `project_controller_->getSaveManager()`
  instead of `MainWindow`'s own member — no change to `WindowManager`'s
  code, just where the pointer originates.
- `ConfigurationAgent` stayed on `MainWindow`, not moved to
  `ProjectController`: it also backs `openPreferences()`/
  `getVisualizationPeriodMs()`, which have nothing to do with project
  save/load. `ProjectController` receives it as an unowned pointer
  (needs `setLastOpenedFile()`).
- Construction-order wrinkle: `WindowManager`'s constructor needs
  `SaveManager*` to exist (to read `getCurrentFileName()`/`isSaved()` per
  window), but `ProjectController`'s own methods need `WindowManager*`.
  Neither is needed by the *other's constructor body*, only by methods
  called well after both exist, so this isn't a real circular dependency —
  resolved with a `setWindowManager(WindowManager*)` setter, called once
  in `MainWindow`'s constructor right after `window_manager_` is built
  (same "safe to wire up before assignment, since nothing invokes it yet"
  reasoning already used for `callbacks_`'s lambdas in that constructor).
- `MainWindow::openExistingFile(file_path)` needed to keep calling
  `layoutWindowButtons()` only when the project actually reloaded (not on
  a same-path or failed-parse no-op, matching original behavior exactly) —
  `ProjectController::openExistingFile(file_path)` now returns `bool` for
  this.
- `MainWindow`'s own `newProject`/`saveProject`/`saveProjectAs` ×2/
  `openExistingFile` ×2 remain as public one-line delegators (kept for API
  compatibility with `MessageRouterCallbacks::open_project_file` and any
  future File menu wiring).

**Verified**: rebuilt cleanly, zero errors. 5x repeated launch, no crash.
Confirmed the real, persisted 2-window/4-tab project still loads correctly
at startup through the new `ProjectController`-owned `SaveManager`. Ran
`system-test cpp basic all` against a live instance — 41+ scenarios
(including `basic`, `dynamic_plotting`, into `object_transform`) reported
"ran successfully" with the server alive and responsive throughout,
confirming the constructor/destructor reordering and the
`WindowManager`/`ProjectController` wiring survive real traffic, not just
a quiet startup.

**Post-hoc correction (2026-09-23): item #9's own extraction edit silently
broke all rendering — found and fixed the same day.** The user reported
"new_qt_duoplot runs fine, but nothing appears when running system-test."
Root cause: the `Edit` that removed
`window_initialization_in_progress_ = false;` from the tail of
`MainWindow`'s constructor (since that flag moved into `ProjectController`)
matched and replaced a block that *also* contained the adjacent
`receive_timer_->start(configuration_agent_->getVisualizationPeriodMs());`
call — and the replacement text dropped that line entirely instead of
preserving it. The constructor still built and connected `receive_timer_`
correctly, just never called `.start()` on it. Effect: `QTimer::timeout`
never fired, `MessageRouter::poll()` was never called, so `queued_data_`
never drained into any `PlotPane` — every plot/GUI-element update sent by
a client was received, parsed, and queued correctly (confirmed via
`addActionToQueue`'s own log path) but silently sat forever, un-rendered.
Every other check performed while verifying items #5-#9 (build success,
5x launch-no-crash, `system-test ... all` reporting "ran successfully",
server-side log warnings/exceptions) is blind to this specific failure
mode: a client-protocol send is judged successful the moment the server
acks receipt, not when the GUI actually repaints, and this bug produced
zero warnings/errors anywhere — draining just never happened, silently.
None of today's verification passes ever visually confirmed a plot
actually appeared on screen.

Diagnosed by adding temporary `[diag]` logging at each stage of the
pipeline (`MessageRouter::manageReceivedData`, `addActionToQueue`,
`poll()`, `PlotPane::pushQueue`, `PlotPane::processActionQueue`) and
tracing exactly where the chain went cold: data reached
`queued_data_["p_view_0"]` correctly, but `poll()`'s own diagnostic never
fired even once — confirming it was never being called at all, pointing
straight at the timer. **Fix**: restored the single dropped line,
`receive_timer_->start(configuration_agent_->getVisualizationPeriodMs());`,
immediately before `show()`. All temporary diagnostic logging was removed
afterward.

**Verified visually, not just via logs this time**: launched a clean
instance (confirmed sole holder of the TCP port via `lsof`), ran
`system-test cpp basic plot`, and took an actual screen capture (macOS
Screen Recording permission needed enabling + an app restart to work at
all in this sandboxed environment — a one-time environment setup step,
unrelated to the bug itself) — confirmed real, colored, correctly
auto-scaled plot data rendered in every pane of both open windows, where
before the fix every pane stayed blank with axes stuck at their
un-updated default range. 5x repeated launch afterward, no crash.

**Process lesson**: for any future change touching `MainWindow`'s
constructor, a passing build and "no crash" is not enough — this session's
own `system-test ... all` runs "ran successfully" the entire time this bug
was live, because that phrase only reflects the *client's* send-side
success. Visual (or, absent screen access, an explicit drain-side log
check) confirmation of actual rendering is the only thing that would have
caught this, and should be part of verifying any change to the
receive-timer/`MessageRouter::poll()` wiring specifically.

## Not yet investigated

These came up in conversation as areas worth an opinion on, but haven't
been looked at closely enough yet to have concrete findings: overall
"new data" arrival handling beyond the locking concern in #3, and whether
`WindowTab`'s element-update propagation (`updateAllElements`,
per-element `show()`/`hide()` on tab switch) has similar structural issues
to #1's parallel-vector problem.
