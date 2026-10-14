# AGENTS.md

Guidelines for AI agents (and humans) contributing to sngrep. sngrep is a C
ncurses tool; most review comments on generated PRs are about consistency with
the existing code, so match the surrounding files before anything else. When a
better pattern conflicts with the existing one, follow the existing one and
mention the improvement in the PR description; changes of style are done for
every file at once in a separate change.

## Philosophy

AI-assisted contributions are welcome, but the final code is maintained by
humans. Working code is not enough: a maintainer must be able to read it,
understand it and change it without the tool that generated it.

- Write plain, readable C: simple control flow, descriptive names and
  short functions, reusing the helpers already in the codebase.
- Follow the existing patterns in the code: new code looks like it was
  written by the same people who wrote the files around it.
- The contributor understands every line of the PR and can explain it during
  review.
- Keep PRs small enough to review; a large feature is a series of PRs.

## Building and testing

Reproduce the CI matrix in `.github/workflows/build.yml` before opening a PR:

- autotools with gcc (`-g -O0 -Wall -Werror -Wno-unused-but-set-variable`) and
  clang, each with the default, OpenSSL and GnuTLS configure options, plus
  `make check`.
- CMake with all options enabled.

New source files go in both `src/Makefile.am` and `CMakeLists.txt`. New tests
are `tests/test_NNN.c`, registered in `tests/Makefile.am`.

## Code style

### Files

Every `.c` and `.h` starts with the GPL license banner followed by a Doxygen
block, copied from a sibling file:

```c
/**
 * @file ui_stats.h
 * @author Ivan Alonso [aka Kaian] <kaian@irontec.com>
 *
 * @brief Functions to manage ui window for capture stats display
 */
```

Header guards are `SNGREP_<NAME>_H`, closed with `#endif /* SNGREP_<NAME>_H */`.

### Headers

- `#define` constants, structs, enums and typedefs of a module live in its
  `.h` file, not at the top of the `.c`.
- Every non-static function is declared in the header with a Doxygen block
  describing it, its `@param`s and its `@return`. The `.c` definition has no
  repeated block. Small `static` helpers in core modules (`rtp.c`) are fine;
  UI panels follow the rule in the UI section.
- Return type goes on its own line, in declarations and definitions:

```c
/**
 * @brief Creates a new stats panel
 *
 * @param ui UI structure pointer
 */
void
stats_create(ui_t *ui);
```

### Formatting

- 4 spaces indentation.
- Function and struct/enum opening braces on their own line; control
  statements (`if`, `while`, `switch`) keep the brace on the same line.
- Use `//` comments inside function bodies. `/** */` blocks are for file and
  function headers, or for a long explanation.

### Module boundaries

- `main.c` parses options and calls into modules. Logic, locking and stats
  gathering belong in the module, exposed through a function that returns a
  struct; `main.c` only prints the result.
- New command line options get a short version too, when a free letter exists.
- New settings are added to `src/setting.h` / `src/setting.c`, documented in
  `config/sngreprc` and `doc/sngrep.8`. If the setting appears in the settings
  screen, grow the dialog height so one empty line stays at the bottom.

## UI panels (`src/curses/`)

Use an existing panel (`ui_stats.c`, `ui_call_raw.c`) as the template:

- Every function in `ui_<name>.c` is prefixed with the panel name
  (`media_dashboard_draw`, `call_raw_handle_key`).
- Panel functions, structs and typedefs are declared in `ui_<name>.h`, even if
  they are only used in that file. The `ui_t` definition goes at the top of
  the `.c`, right after the includes.
- Keyboard input goes through the panel's `handle_key`; mouse clicks through
  its `handle_mouse` callback in `ui_t`.
- Reuse the sngrep look: the color pairs in `theme.h` for title bars, column
  headers and the status bar, and the footer key hints drawn with
  `ui_draw_bindings()`.
- Lists keep a stable order between refreshes, so a row can be followed.
- New features take free keys. Existing keybindings stay as they are unless
  the maintainer agrees to change them.

## Commits

- Subject is `<area>: <summary>`, lowercase and imperative, where area is a
  module or directory: `capture:`, `sip:`, `filter:`, `ui:`, `eep:`, `doc:`,
  `ci:`. Run `git log --oneline` for examples.
- Body wrapped at 72 columns explaining why the change is needed.
- One logical change per commit, and every commit builds. Unrelated fixes
  found along the way go in their own commits.
- Before asking for the final review, squash review fixes into the commits
  they belong to, leaving the minimum number of commits.

## Pull requests

- Description with a short **Summary** of what changes and why, and a
  **Validation** section listing the builds, `make check` results and manual
  tests actually run (and what could not be tested).
- Link the issue it resolves (`Closes #NNN`).
- For large features, split into stacked PRs and state which PR each one
  depends on.
