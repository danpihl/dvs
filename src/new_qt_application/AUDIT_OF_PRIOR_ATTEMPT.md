# Audit of the prior Qt migration attempt (`src/qt_application`)

Historical record only — see `/Users/daniel/.claude/plans/fancy-nibbling-hanrahan.md`
(Phase 0) for why this exists. This is hands-on-verified fact, not a
restatement of `qt_application`'s own (already known to be unreliable)
status docs. `src/qt_application` itself was left untouched.

## Build

- After checking out `third_party/LumosConfig` (`git submodule update --init
  third_party/LumosConfig`) and re-enabling
  `add_subdirectory(qt_application)` in `src/CMakeLists.txt`, `qt_duoplot`
  configures and builds cleanly with `make qt_duoplot -j5` — no errors, only
  the usual unused-parameter warnings already present in the shared
  `main_application`-derived plot-object code. This contradicts
  `MIGRATION_PLAN.md`'s more pessimistic framing; the build itself is not
  the blocker it was made out to be.

## Runtime smoke test (via `src/system_test`)

Built `duoplot`, `qt_duoplot`, and `system_test/system-test` from the same
tree. Ran `system-test cpp basic <name>` for `plot`, `surf`, `scatter`,
`scatter3`, `imShow`, `drawMesh`, `lineCollection`, `stairs`, `stem`,
`screen_space_primitive`, `axis`, `legend`, `plotCollection`, `plot3`,
`fastPlot` against a freshly launched `qt_duoplot`. All 15 scenarios
connected, sent their data, and exited cleanly (exit code 0); the server
process did not crash or hang for any of them, and its log showed no
errors, warnings, or unhandled-exception output for any of these scenarios.

**This is a connectivity/crash smoke test only.** It proves `qt_duoplot`
accepts the full range of plot-object wire-protocol messages without
faulting. It does **not** prove the rendered output is visually correct —
that requires a human looking at the window, which this session cannot do.
Recommend the user runs at least `cpp basic all` by hand against
`qt_duoplot` and eyeballs each pane before trusting any of these render
paths.

## Concrete finding: view/window-creation coalescing on startup

Running `cpp basic plot` **immediately** (~2s) after launching `qt_duoplot`
showed only one of five referenced views (`p_view_0`, `p1`, `p_view_1`,
`p_view_2`, `w1_p_view_0`) actually get a window/pane created — only the
*last* one (`w1_p_view_0`, since it needed a second window) shows a
"Created plot pane" log line; the other four's `setActiveView` calls are
logged but never followed by pane creation. Traced the cause to
`MainWindow::setActiveView`/`receiveData()`
(`src/qt_application/main_window.cpp:475-526`):

```cpp
void MainWindow::setActiveView(const ReceivedData& received_data)
{
    ...
    current_element_name_ = name;
    if (plot_panes_.count(current_element_name_) == 0)
    {
        new_window_queued_ = true;
    }
}
```

`new_window_queued_` is a single boolean and `current_element_name_` is a
single string, both overwritten on every call. If several never-before-seen
view names arrive faster than the receive timer drains the queue, only the
*last* one queued survives — the others are silently dropped (their
`queued_data_[name]` entries are never routed anywhere, since
`plot_panes_.count(name)` stays 0 forever).

**This is not a Qt-introduced bug.** The exact same logic, including the
same single-flag/single-name coalescing, exists verbatim in the wx original
at `src/main_application/main_window_receive.cpp:15-34` and `:339-376`
(`MainWindow::setActiveView` / `MainWindow::receiveData`) — compared
side-by-side, the two are a faithful line-for-line port of each other. So
this qualifies as **correctly mimicked behavior**, not a regression, per
this plan's "mimic, don't redesign" rule.

What's still open: later test runs (after the app had been up longer)
*did* successfully populate `p_view_0`/`p_view_1`/`p_view_2` and
`s_view_0`/`s_view_1`/`s_view_2` panes, apparently via a default project
layout (`primary_view`/`secondary_view` tabs) that finishes loading a few
seconds after launch. So in practice this coalescing bug is a **narrow
startup race**: a client that fires several `setActiveView` calls for
distinct, never-before-seen views within the first couple of seconds after
the server starts (before its default layout finishes loading) can lose
all but the last one. Confirming this exists identically on `duoplot`
(wx) couldn't be done from logs alone — `duoplot`'s stdout doesn't have
per-view creation logging the way `qt_duoplot`'s newer `LUMOS_LOG` calls
do — so this is confirmed by source comparison, not by observed runtime
parity. If the user wants this actually fixed (in both implementations, or
just going forward in the Qt port), that's a design decision worth asking
about explicitly rather than assuming — flagging here rather than silently
changing it.

## Concrete finding: unit mismatch between normalized-fraction and absolute-pixel geometry

User's own hands-on run of `qt_duoplot` against `exp0` (a real project
file, two windows) showed plot panes and GUI elements rendered at a tiny
fixed size, clustered in the window's top-left corner, with the rest of
the window showing nothing but `content_area_`'s plain background color.
A later screenshot showed a *third*, dynamically-created window ("Window
4", spawned in response to a `setActiveView` for a view name not in the
loaded project) with its single pane rendering correctly, filling the
window. That both outcomes exist side by side, from the same binary, is
the key clue: they go through two different code paths in
`window_tab.cpp`, and only one of them is correct.

**Confirmed unit convention** (checked in three places — the two Qt
`updateSizeFromParent` implementations and the wx original): `x`/`y`/
`width`/`height` on `ElementSettings` (`project_settings.h:19-24`) are
always **normalized fractions of the parent's pixel size (0.0-1.0)**, not
absolute pixels:
- wx original, `main_application/gui_element.cpp:439-442`:
  `wxSize new_size(element_settings_->width * px * ratio_x, ...)` —
  fraction × parent pixel size.
- Qt port, `qt_application/plot_pane.cpp:452-461`
  (`PlotPane::updateSizeFromParent`): `new_width =
  element_settings_->width * parent_size.width()` — same convention,
  correctly implemented here.

**Bug A — breaks every project-file-loaded pane (`exp0`'s two windows):**
`WindowTab::createNewPlotPane(const std::shared_ptr<ElementSettings>&)`
(`window_tab.cpp:192-217`), which is what `MainWindow::setupWindows`/
`openExistingFile` calls for each pane defined in a loaded project file,
sets the pane's *initial* geometry like this (`window_tab.cpp:206-211`):

```cpp
plot_pane->setGeometry(
    static_cast<int>(element_settings->x) + element_x_offset_,
    static_cast<int>(element_settings->y),
    static_cast<int>(element_settings->width),
    static_cast<int>(element_settings->height)
);
```

This truncates the normalized fraction directly to pixels instead of
multiplying by the parent size first — a `width` of `0.3` becomes
`static_cast<int>(0.3)` = **0 pixels**. And `MainWindow::setupWindows`
(`main_window.cpp:201-261`) never calls `updateSizeFromParent` afterward
to correct it — it just calls `window->show()` and moves on. So every
project-loaded pane starts at (effectively) zero/near-zero size and stays
that way until something *else* happens to trigger the correct
fraction×parent-size math in `updateSizeFromParent` — i.e. the user
manually resizing the window (`GuiWindow::resizeEvent`,
`gui_window.cpp:519-525`) or switching tabs (`GuiWindow::switchToTab`,
`gui_window.cpp:204`), both of which *do* call it correctly. This predicts
a falsifiable fix: **resizing either `exp0` window by even a pixel should
make its panes snap to the correct size.**

**Confirmed by the user, hands-on:** resizing Window 0 or Window 1 snaps
their panes to the correctly-sized layout immediately. This validates both
halves of the theory — the stored fractions are correct and
`updateSizeFromParent`'s multiply-by-parent-size math is correct; the only
defect is that the initial `createNewPlotPane` call bypasses that math.

**Bug B — makes "Window 4" look right by accident, but leaves it fragile:**
`WindowTab::createNewPlotPane(const std::string& element_handle_string)`
(`window_tab.cpp:179-190`) — the path used by
`MainWindow::newWindowWithoutFileModification` when a client references a
view name that doesn't exist yet — hardcodes *absolute pixel* values into
the same fraction-typed fields: `x = 10.0f; y = 10.0f; width = 600.0f;
height = 400.0f;`. Fed into the same `setGeometry()` call as Bug A, this
happens to produce a plausible 600×400 pane at creation, which is exactly
why "Window 4" renders correctly in the screenshot — but it's a coincidence
that corrupts `element_settings_`'s semantics: if that window is ever
resized or its tab switched, `updateSizeFromParent` will treat `600.0f` as
a fraction and multiply it by the parent's pixel width, producing a pane
roughly 600× too large. **Falsifiable prediction: resizing "Window 4"
should make its pane balloon to a huge, broken size, not stay correct.**

**Confirmed by the user, hands-on — and worse than predicted.** Reproduced
with a fresh `qt_duoplot` (no project loaded) and
`system-test cpp basic plot`, which creates exactly this scenario via its
`w1_p_view_0` view (not part of any loaded project, so it goes through
`newWindowWithoutFileModification`/the buggy `createNewPlotPane(string)`
overload). Resizing that window (or its pane) does not just produce an
oversized-but-visible rectangle — it breaks OpenGL outright:

```
QOpenGLFramebufferObject: Framebuffer incomplete attachment.
QOpenGLFramebufferObject: Framebuffer incomplete, missing attachment.
QOpenGLWidget: Failed to create wrapper texture
```

Once `updateSizeFromParent` reinterprets `600.0f` as a fraction and
multiplies it by the window's actual pixel width (e.g. `600 × ~900px` ≈
540,000px), the requested `QOpenGLWidget` framebuffer size exceeds the
driver's maximum texture/framebuffer dimensions, and FBO allocation fails.
So this isn't just a cosmetic layout glitch — the same unit-mismatch bug
can take down the GL rendering surface for that pane entirely. Reinforces
the lesson above: a single field silently switching between "fraction" and
"pixels" depending on which code path wrote it isn't just an off-by-scale
bug, it's a landmine that surfaces as a completely different, harder-to
diagnose failure (GL framebuffer errors) depending on how it's triggered.

Both are squarely "poor migration" bugs of the kind the user named as the
root cause of the first attempt's failure — a straightforward unit
confusion between two call sites that both write into the same
supposedly-normalized field, not a wxWidgets-vs-Qt fundamental issue, and
not present in the wx original (which only ever computes pixels from
fractions via multiplication, per `gui_element.cpp:439-442` — it has no
equivalent of either buggy `createNewPlotPane` overload). **Lesson for
`new_qt_application`:** pick one convention for element geometry storage
(fractions, matching the wire protocol / project-file format and the wx
original) and make *only* the parent-size-multiplication path
(`updateSizeFromParent`'s logic) ever turn it into pixels — never let a
second call site assign or consume that field as if it were already
pixels. And call the correct conversion once, explicitly, right after
construction and right after project load — not only reactively from
`resizeEvent`/tab-switch — verified on a freshly launched, never-touched
window before considering any pane-creation path "done."

## Concrete finding: the entire edit-mode mouse-interaction system was dropped

Found while writing `new_qt_application/gui_element.h` (Phase 2), by
comparing the wx original's `ApplicationGuiElement`
(`main_application/gui_element.h`/`.cpp`) against the old attempt's
`ApplicationGuiElement` (`qt_application/gui_element.h`). The wx original
implements a full cursor-square-state machine (`CursorSquareState::LEFT/
RIGHT/TOP/BOTTOM/*_CORNER/INSIDE/OUTSIDE`) driving click-drag resize/move
from any edge or corner while the physical Ctrl key is held, plus a
`Cmd`-hover cursor preview, all in `mouseLeftPressed`/`mouseMovedOverItem`/
`adjustPaneSizeOnMouseMoved` (`gui_element.cpp:97-329`). The old attempt's
`ApplicationGuiElement` has **no mouse-event handling at all** — it was
replaced with a much simpler `setBoundingRect`/`getBoundingRect` pair, and
`setMinXPos` is an empty no-op stub (`virtual void setMinXPos(const int
min_x_pos) {}`) rather than actually storing the value anywhere. No
`CursorSquareState`-equivalent machinery, and no evidence anywhere in
`qt_application` of resize-by-dragging-an-edge being implemented. This is
consistent with `MIGRATION_PLAN.md`'s own admission that edit-mode
interaction wasn't finished, but the gap is more specific than "not
finished" — the interface itself was redesigned in a way that has no path
to the original behavior without rewriting it, which is exactly why this
effort is writing `new_qt_application/gui_element.h` as a faithful port
rather than adapting the old interface.

## Not yet audited (needs the user, not more automation)

- Visual correctness of any rendered scenario (2D/3D lines, scatter, surf,
  imShow, legend, mesh, screen-space primitives) — needs eyes on the
  window.
- `c gui basic` and `cpp two_way_comm first_test` — both require typing
  into the running client's stdin *and* clicking generated widgets in the
  live GUI to prove the callback round trip; this session can launch the
  binaries but can't click a button or watch a window, so this is squarely
  a hands-on task for the user.
- `cpp dynamic_plotting all` / `cpp updateable_plotting all` — same
  constraint; also the right test for whether `qt_duoplot`'s update timer
  (whatever replaced wx's ~100 Hz `wxTimer`) is actually running at a
  reasonable cadence, which needs visual judgment of smoothness.
- `cpp object_transform all` — edit-mode resize/move, needs mouse
  interaction.
- Project save/load, serial port integration — not covered by
  `system_test` at all; needs manual exercise either way.
