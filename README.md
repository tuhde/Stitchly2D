# Stitchly2D

Stitchly2D turns pattern files from [Seamly2D](https://github.com/FashionFreedom/Seamly2D) into something you can
knit. You open a pattern (`.sm2d`) with its measurements (`.smis`), enter your gauge, and get a stitch chart and
written instructions for every piece. Crochet and embroidery are planned as further techniques.

It is a sibling of Seamly2D, not a replacement: Seamly2D stays the drafting tool, Stitchly2D prepares the result for
yarn. It shares Seamly2D's code and framework, so it looks and behaves like it.

> **Status: early development.** The project skeleton and the vendored libraries build. The knitting engine and the
> application window are the next steps. See [docs/PLAN.md](docs/PLAN.md).

## How it will work

1. Draft the pattern in Seamly2D and save it (`.sm2d`, plus the `.smis` measurements it refers to).
2. Open it in Stitchly2D. Each pattern piece appears in the pieces list.
3. Set the gauge (stitches and rows per 10 cm or per inch) and the shaping method for a piece or the whole project.
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

## Source layout

| Path | Content |
|---|---|
| `src/libs/` | Shared libraries copied from Seamly2D (`ifc`, `vpatterndb`, `vgeometry`, `vlayout`, `vmisc`, `vwidgets`, `vformat`, `vtools`, `qmuparser`, ...). Kept unmodified where possible so changes can be compared with upstream. |
| `src/libs/stitchcore/` | Knitting logic without any GUI: gauge, stitch grid, shaping (planned). |
| `src/app/stitchly2d/` | The application (planned). |
| `docs/PLAN.md` | Design and roadmap. |
| `NOTICE` | Which Seamly2D commit the libraries come from. |

## AI integration (planned)

Like our Seamly2D fork, Stitchly2D will contain an optional Model Context Protocol server so that Claude can load a
pattern, set the gauge, read the stitch grid and export instructions in the running window. It will be off by default,
listen on `127.0.0.1` only, require a token and ask before touching files.

## License

GPL v3, the same as Seamly2D. See [LICENSE](LICENSE). Third-party parts keep their own licenses (for example
`qmuparser`, BSD). Stitchly2D contains code from Seamly2D, Copyright (C) 2017-2025 the Seamly2D project; see
[NOTICE](NOTICE).
