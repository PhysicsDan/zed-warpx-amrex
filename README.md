# WarpX input files in Zed

Syntax highlighting and a section outline for WarpX input decks (the AMReX
ParmParse `prefix.name = value` format), as a Zed extension.

## Install

1. Put this `zed-warpx` folder where it can stay (for example `~/zed/zed-warpx`).
   Zed loads the extension from this folder, so don't move it afterwards.
2. In Zed, open the command palette (Cmd-Shift-P), run **zed: install dev extension**
   and choose the `zed-warpx` folder.
   The first install takes a minute: Zed downloads its WebAssembly compiler
   (wasi-sdk, about 100 MB, only once) and builds the parser. Git must be
   installed (on macOS it comes with the Xcode Command Line Tools).
3. Open a deck. The language selector in the status bar should say **WarpX**.

Files ending in `.input` or `.inputs`, and files named `inputs`, `inputs_1d`,
`inputs_2d`, `inputs_3d`, `inputs_rz`, `inputs_rcylinder` or `inputs_rsphere`, are
recognised. For other names, add globs to your Zed settings (Cmd-,):

```json
"file_types": {
  "WarpX": ["inputs_*", "*.warpx"]
}
```

or put `# vim: set ft=warpx:` on the first or last line of a deck. You can also
pick WarpX from the language selector in the status bar for the open file.

If highlighting doesn't appear, run **zed: open log** and look for lines
mentioning `warpx`.

## What it does

- **Keys**: built-in prefixes (`warpx.`, `amr.`, `geometry.`, `algo.`,
  `particles.`, ...) in the keyword colour; names you define (species, lasers,
  diagnostics, injection sources) in the type colour; the parameter itself in
  the property colour. `my_constants.n0 = ...` shows `n0` in the constant colour,
  the same colour its uses get in values.
- **Values**: numbers, constants, operators, amrex parser functions (`sqrt`,
  `exp`, `if`, `atan2`, ...), WarpX constants (`clight`, `q_e`, `m_e`, `pi`, ...),
  `true`/`false`, `and`/`or`.
- **Parser-function arguments**: in
  `hist.filter_function(t,x,y,z,ux,uy,uz) = "abs(x*uz) < origin_window"`,
  `t,x,...` are marked as arguments in the key and wherever the value uses them,
  while `origin_window` shows as a constant. A name that is not declared keeps
  the constant colour, which shows typos such as `x` in a `field_function(X,Y,t)`.
- **Names that refer to other blocks**: the values of `particles.species_names`,
  `lasers.names`, `diagnostics.diags_names`, `warpx.reduced_diags_names`,
  `<species>.injection_sources`, `<diag>.species`, ... take the same colour as the
  keys they introduce.
- Multi-line `"..."` expressions, `\` line continuations, `"""..."""`, `FILE =`,
  `UNSET =`, `[table]` headers, `[..]` arrays, `{..}` tables, and
  `#if AMREX_SPACEDIM == 2` / `#endif` lines.
- **Outline** (Cmd-Shift-O, or the outline panel): one entry per section comment
  such as `# --- GEOMETRY --- #`, `#----PARTICLE DIAGNOSTIC----#` or
  `####### PLASMA #######` (a `#`, at least three of `- = # * ~ _ +`, then a title),
  with the parameters defined under it nested inside.
- Comment toggling (Cmd-/), bracket matching and auto-closing.

## Changing colours

Colours come from your theme. To override one, add `theme_overrides` to your
settings, for example to make section titles yellow in One Dark:

```json
"theme_overrides": {
  "One Dark": {
    "syntax": { "title": { "color": "#e5c07b" } }
  }
}
```

Theme overrides apply to every language, so `title` also changes Markdown headings.

| Highlighted                                 | Theme key                  |
| ------------------------------------------- | -------------------------- |
| comments                                    | `comment`                  |
| section header comment / its title          | `comment.doc` / `title`    |
| `#if AMREX_SPACEDIM ...` lines              | `preproc`                  |
| built-in prefixes, `FILE`, `UNSET`          | `keyword`                  |
| your prefixes and the names that refer to them | `type`                  |
| parameter names                             | `property`                 |
| constants and option names in values        | `constant`                 |
| WarpX constants (`clight`, `pi`, ...)       | `constant.builtin`         |
| parser-function arguments                   | `variable.parameter`       |
| functions / amrex parser functions          | `function` / `function.builtin` |
| numbers, quoted strings, operators          | `number`, `string`, `operator` |

The rules themselves are in `languages/warpx/highlights.scm` (for example the
list of built-in prefixes). After editing them, click **Rebuild** on WarpX in
Zed's Extensions page.

## Folder layout

- `extension.toml`: the extension manifest.
- `languages/warpx/`: language settings and the Tree-sitter queries
  (highlights, outline, brackets, ...).
- `tree-sitter-warpx/`: the grammar (`grammar.js`, `src/scanner.c`, generated
  `src/parser.c`) with its tests (`tree-sitter test`).
- `grammars/`: Zed's build folder. It ships with the grammar already checked out
  so the install needs no setup; Zed writes `grammars/warpx.wasm` there.
- `rebuild-grammar.sh`: only needed if you change the grammar itself (see the
  comments in the script).

To remove the extension, open Zed's Extensions page and uninstall WarpX.
