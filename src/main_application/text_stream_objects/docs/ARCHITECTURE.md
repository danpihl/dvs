# `src/main_application/text_stream_objects/` — architecture overview

**This module is dead, unused scaffolding — not a working part of the
application.** It's documented here mainly so a future refactor doesn't
waste time trying to understand or wire up code that was never finished
and has since been superseded by a different implementation elsewhere.

## What's actually here

Two files, 18 lines total:

- `text_stream_object_base.h`:
  ```cpp
  struct TextStreamSettings
  {
      bool show_topic_id;
      bool show_timestamp;
  };

  class TextStreamObjectBase
  {
  private:
  public:
      TextStreamObjectBase();
      TextStreamObjectBase() {}
  };
  ```
- `text_stream_object_base.cpp`: **completely empty** (0 bytes).

## Why this doesn't (and can't) work

- `TextStreamObjectBase` declares its default constructor **twice** —
  once with no body (`TextStreamObjectBase();`) and once with an empty
  body (`TextStreamObjectBase() {}`) — which is an illegal redeclaration
  in C++ and would fail to compile if this header were ever included
  anywhere.
- The out-of-line declaration (`TextStreamObjectBase();`) has no matching
  definition — the `.cpp` file that would provide one is empty.
- **Nothing in the codebase includes `text_stream_object_base.h`, and
  nothing references `TextStreamObjectBase` or `TextStreamSettings`**
  (confirmed by searching the whole `main_application` tree). The build
  currently succeeds only because the header is never parsed — the empty
  `.cpp` file compiles trivially on its own, and
  `src/main_application/CMakeLists.txt` does list it as a source file, but
  since it contains nothing, that's a no-op.

In short: this is inert code. It costs nothing to build today, but it also
does nothing, and the header would not even compile if anyone tried to
actually use it.

## What this looks like it was meant to be

The naming and the `TextStreamSettings` fields (`show_topic_id`,
`show_timestamp`) strongly suggest this was meant to parallel
`src/main_application/plot_objects/stream_object_base/` +
`stream_objects/` (see that module's docs) — i.e. a rendered "stream
object" base class for displaying incoming serial-stream text, analogous
to how `StreamObjectBase`/`Plot2DStream`/`ScatterStream`/`StairsStream`
render incoming serial-stream *numeric* data as OpenGL plots.

**That feature was actually built, but a different way, and lives
elsewhere:** `ScrollingTextGuiElement`
(`src/main_application/gui_elements.h`/`.cpp`) is a
**`wxTextCtrl`-derived** GUI element (`class ScrollingTextGuiElement :
public wxTextCtrl, public ApplicationGuiElement`) that receives new text
lines via `pushNewText(TopicId, ...)` and appends them directly to a
native wx text control, using `print_timestamp_`/`print_topic_id_` member
flags that mirror this module's abandoned `TextStreamSettings` fields
almost exactly. Its settings counterpart,
`ScrollingTextSettings`/`SubscribedTextStreamSettings`
(`src/main_application/project_state/`, see that module's docs), is fully
implemented and actively used for save/load of scrolling-text panes.

**GUI-toolkit note for the wxWidgets → Qt migration**: this dead module
itself has no wx dependency (it has no working code at all), but the
*actual* scrolling-text feature it appears to have been an abandoned
attempt at superseding is implemented as a direct `wxTextCtrl` subclass —
that's the file that will need a Qt equivalent (most likely a
`QPlainTextEdit`/`QTextEdit` subclass with the same append-line behavior),
not anything in this directory.

## Recommendation

There is nothing to preserve here for a refactor or migration. Treat
`src/main_application/text_stream_objects/` as a candidate for deletion
(after confirming with the codebase's history/author whether it was
intentionally kept around for a planned future rewrite), and look at
`gui_elements.h`/`.cpp`'s `ScrollingTextGuiElement` for the real,
working implementation of the feature this module's name suggests.
