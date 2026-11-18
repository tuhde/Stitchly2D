# Stitchly2D

Stitchly2D turns pattern files from [Seamly2D](https://github.com/FashionFreedom/Seamly2D) into something you can
knit. You open a pattern (`.sm2d`) with its measurements (`.smis`), enter the size of one stitch, and get a stitch chart and
written instructions for every piece. Crochet and embroidery are planned as further techniques.

It is a sibling of Seamly2D, not a replacement: Seamly2D stays the drafting tool, Stitchly2D prepares the result for
yarn. It shares Seamly2D's code and framework, so it looks and behaves like it.

> **Status: early development.** The knitting engine, the pattern loader and a first main window work: open a
> pattern, pick a basic stitch or enter the stitch width and row height, split a piece into bands with their own
> stitch, lay special stitch and color patterns over them, and see the stitch grid and written instructions per
> piece. The catalog of stitches and patterns can be edited and exchanged as `.stpat` files; bands, patterns and settings are saved in project files (`.stitchly`), which also carry the catalog entries they use. Edits can be undone (Edit > Undo), and the window warns before unsaved changes would be lost. Instructions cast on or bind off large changes at an edge (adjustable) and write regular shaping as "every 4th row, 6 times". Edges are shaped on outside rows only (row 1 is an outside row) unless you choose every row; the "Edge stitches" setting keeps that many stitches between the edge and the increase or decrease. A piece can be knitted flat, in the round by hand, on a circular knitting machine or on a double-bed machine (one default for all pieces, and a choice per piece); the instructions then speak of rounds, needles and beds.
> See [docs/PLAN.md](docs/PLAN.md).

## How it will work

1. Draft the pattern in Seamly2D and save it (`.sm2d`, plus the `.smis` measurements it refers to).
2. Open it in Stitchly2D. Each pattern piece appears in the pieces list.
3. Pick basic stitch types from the catalog (stockinette, 1:1 rib, patent, ...). Each has the width of one stitch and the height of one row, both in mm, because these depend on the stitch type. Gauges per 10 cm or per inch are calculated from them, and you can enter either form. Split a piece into bands along its height (for example a rib hem, then stockinette) and give each band a basic type. Lay special stitch patterns and color patterns from the catalog over a band, and choose the shaping method. You can extend the catalog with your own types and patterns, and exchange special and color patterns as `.stpat` files ([format](docs/PATTERN_FILE_FORMAT.md)).
4. Stitchly2D lays the piece outline over a stitch grid, with the grainline as the knitting direction. Every row gets
   a stitch count, and the changes between rows become increases and decreases.
5. Read the result on the canvas, as written instructions, or export it as TXT, SVG, PDF, PNG or a CSV row table.

| Technique | State |
|---|---|
| Knitting | In progress, first target |
| Crochet | Planned |
| Embroidery | Planned |

## Building

Stitchly2D builds with qmake and **Qt 6** (Seamly2D no longer builds with Qt 5).

Requirements:

- a C++ compiler, `make` and Qt 6 (`qmake6`) with the widgets, xml, svg, printsupport, network and multimedia modules
- Xerces-C development files (`libxerces-c-dev` on Debian/Ubuntu)

Build the libraries (the application will join them as it is added) in a separate directory. The path must not
contain spaces:

```sh
mkdir build && cd build
qmake6 ../Stitchly2D.pro
make -j$(nproc)
```

## Trying it

```sh
build/src/app/stitchly2d/bin/stitchly2d pattern.sm2d          # main window (a .stitchly project works too)
build/src/app/stitchly2d/bin/stitchly2d --cli --stitch-width 5 --row-height 3.5 pattern.sm2d   # text only
```

`--screenshot file.png` opens the window without a display, saves a picture and quits. `--cli` also accepts a `.stitchly` project: it then uses the project's stitch settings, bands and patterns (`-w`, `-r` and `--seam-allowance` override them, `--piece name` limits the output to one piece). It loads the pattern and its measurements the way Seamly2D does, and prints the number of rows, the cast-on and
bind-off stitches and the written instructions for every piece. `--csv` prints the row table instead,
`--seam-allowance` includes the seam allowance, `--measurements file.smis` replaces the measurement file named in the
pattern. Pieces are turned so their grainline is vertical. `--cli` needs no display. `src/test/smoke_test.sh
<path to stitchly2d>` runs it on the bundled sample patterns.

## Languages

The program is written in English; German is the first translation. View > Language saves the choice (used from the
next start), the default is the system language, and `--lang de_DE` sets it for one run (also with `--cli`, so the
written instructions come out in German). The `.qm` files are built from `share/translations/stitchly2d_<locale>.ts`
and embedded in the program.

To add a language, for example French:

1. add `fr_FR` to `LANGUAGES` in `src/app/stitchly2d/translations.pri` and `share/translations/translations.pro`,
2. run `make lupdate` in the build directory (collects the strings into `stitchly2d_fr_FR.ts`),
3. translate the file in Qt Linguist and rebuild.

`tst_stitchcore` checks that every `.ts` file has no unfinished or empty entry and keeps the `%1` placeholders. Text in
code that is not a `QObject` uses `QCoreApplication::translate` (contexts `Instructions`, `StitchCore`, `Cli`, `main`).
The names of the catalog's stitches and patterns, the CSV header and the tool descriptions for Claude stay English.

## Source layout

| Path | Content |
|---|---|
| `src/libs/` | Shared libraries copied from Seamly2D (`ifc`, `vpatterndb`, `vgeometry`, `vlayout`, `vmisc`, `vwidgets`, `vformat`, `vtools`, `qmuparser`, `vmcp`, ...). Kept unmodified where possible so changes can be compared with upstream. |
| `src/libs/stitchcore/` | Knitting logic without any GUI: stitch size, stitch grid, shaping (planned). |
| `src/app/stitchly2d/` | The program: main window, pattern loader (running Seamly2D's parser without a window) and the `--cli` mode. The main window. |
| `docs/PLAN.md` | Design and roadmap. |
| `NOTICE` | Which Seamly2D commit the libraries come from. |

## AI integration

Like our Seamly2D fork, Stitchly2D contains an optional Model Context Protocol server, so that Claude can open a
pattern, set the stitch size, add stitches and patterns to the catalog, give pieces bands and patterns, and read the
instructions in the running window. It is off by default, listens on `127.0.0.1` only (port 47801), requires a token
and asks before touching files. See [docs/AI_INTEGRATION.md](docs/AI_INTEGRATION.md).

## License

GPL v3, the same as Seamly2D. See [LICENSE](LICENSE). Third-party parts keep their own licenses (for example
`qmuparser`, BSD). Stitchly2D contains code from Seamly2D, Copyright (C) 2017-2025 the Seamly2D project; see
[NOTICE](NOTICE).
