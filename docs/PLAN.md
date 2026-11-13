# Stitchly2D – plan

Status 2026-11-13: skeleton done. Libs vendored from Seamly2D 476a5364d (see `NOTICE`) and building with `qmake6`. `vtools` is included because `ifc` and `vwidgets` depend on it; `tools` is left out. `stitchcore` and the app come next.

## Context
Stitchly2D should take pattern files from Seamly2D (`.sm2d` + `.smis`) and prepare them for knitting. Crochet and embroidery follow later. The user wants the sources and framework close to Seamly2D so the look and feel match. The repo is empty (no commits). Seamly2D is at `/var/projects/Seamly2D`: GPLv3, qmake, Qt 6 widgets, static libs under `src/libs/*`, apps under `src/app/{seamly2d,seamlyme}`. Its branch `feature/knitting-grid` (commit 5fdd2e99c, by the same user) already adds a per-piece grid to Seamly2D. That is a good reference for grid anchoring and clipping to the seam line.

Decisions made: **sibling app with Seamly2D's shared libs copied in (vendored)**, and the **first deliverable is a desktop GUI app**.

## Approach

### 1. Repo skeleton (mirrors Seamly2D)
- `Stitchly2D.pro` (subdirs `src`), `common.pri` (copied and trimmed from Seamly2D: c++14, PCH, `FindBuildRevision`), GPLv3 `LICENSE`, and a `NOTICE` recording the Seamly2D commit the libs were copied from.
- `src/libs/`: copy these from Seamly2D unchanged, so upstream diffs stay easy: `qmuparser`, `ifc`, `vgeometry`, `vpatterndb`, `vlayout`, `vmisc`, `vformat`, `vwidgets`, `vpropertyexplorer`. Leave out `vtools`, `vmcp`, `fervor`, `vdxf`, `vobj` and `vtest` for now.
- `src/libs/stitchcore/` (new static lib, no GUI): the knitting logic, described below.
- `src/app/stitchly2d/` (new app): `main.cpp`, `ApplicationStitchly` (derived from `VAbstractApplication`), `mainwindow.{h,cpp,ui}`, `version.h`, `stitchly2d.pro/.pri`, `share/resources`, and translations. Model it on `src/app/seamlyme` (the smaller sibling app).
- Look and feel: reuse the vmisc theme and icon resources (`icon/icons/theme.qrc`), `VCommonSettings`/`VSettings` for preferences and themes, the `VMainGraphicsScene`/`VMainGraphicsView` canvas and the same dock/toolbar layout (pieces list, properties dock, canvas). Add a Stitchly-specific icon set and name.

### 2. Pattern import (reuse, not rewrite)
- Open `.sm2d` through `ifc` `VDomDocument`/`VAbstractPattern` and `vpatterndb` `VContainer`. `VPattern` lives in the app (`src/app/seamly2d/xml/vpattern.*`), so copy a headless, trimmed version into `stitchcore` (parse draft objects and `<pieces>`, no tools or undo).
- Load measurements from the `.smis` referenced by the pattern. Seamly2D already handles the converters for old file versions.
- Build `VPiece` → `VLayoutPiece` (the same path layout export uses) to get the closed seam line and, optionally, the seam allowance polygon and grainline.

### 3. Knitting engine (`stitchcore`, unit-tested)
- `Gauge`: stitches and rows per 10 cm (or per inch), set per piece or for the whole project. Optional stitch aspect ratio.
- `PieceOutline`: the piece polygon in mm, rotated so the grainline is vertical (grainline = knitting direction).
- `StitchGrid`: for each row, take the horizontal span at the row's mid-height, scan across it and produce a stitch count and offset. Keep the result as a row list `{row, firstStitch, count}`.
- `ShapingPlanner`: compare consecutive rows and emit increases and decreases, with a configurable method (`k2tog`, `ssk`, `M1`) and edge-stitch handling. Cast-on count comes from the first row and bind-off from the last.
- Output models: a stitch chart (grid) and written instructions per piece, as text.
- Make the technique pluggable (`ITechnique`: `KnittingTechnique` now; `CrochetTechnique` and `EmbroideryTechnique` later), so the pipeline `outline → grid → instructions` is shared.

### 4. GUI (first version)
- File menu: open pattern (`.sm2d`) and the linked measurements. Recent files, like Seamly2D.
- Pieces dock: list of pieces with checkboxes. Properties dock (`vpropertyexplorer`): gauge, technique, shaping method, seam allowance on or off, edge stitches.
- Canvas: piece outline with the stitch grid overlay and the row/stitch count, modelled on `VGridItem` from the `feature/knitting-grid` branch.
- Instructions tab: written pattern per piece (text, copyable).
- Export: instructions as TXT, chart as SVG, PDF, PNG, and a CSV of the row table. Reuse `SvgGenerator` and Qt's PDF engine.

### 5. Later
Crochet (chart symbols, rounds), embroidery (outline to stitch paths, PES/DST export), a CLI mode (reuse `commandoptions` from vmisc), and optional `.stitchly` project files that store gauge settings next to the pattern.

### 6. AI integration (same as in our Seamly2D fork)
Stitchly2D gets the same Claude integration we added to the fork (branch `feature/ai-integration`, docs in `AI_INTEGRATION.md` and `AI_INTEGRATION_PLAN.md` there). Claude can then load a pattern, set gauge and shaping options, inspect the stitch grid and export instructions in the running window, with undo.
- Vendor `src/libs/vmcp` (`mcpserver`, `mcprouter`, `mcptoolregistry`, `mcpinstances`) from the fork and add the "MCP Config and Log" panel and the optional Claude shell (QTermWidget, Linux only) to the main window, as in Seamly2D.
- Same safety model: off by default, listens on 127.0.0.1 only, bearer token, Host/Origin checks, confirmation dialogs for file access, every action on the undo stack ("Claude: ...").
- Same multi-window routing, using a different default port from Seamly2D (47800) so both apps can run side by side.
- Stitchly tools to register: `get_project_info`, `list_pieces`, `set_gauge`, `set_technique`, `set_shaping`, `get_stitch_grid`, `get_instructions`, `screenshot`, `open_pattern`, `export_chart` / `export_instructions`. Pattern-drawing tools are not needed, since Seamly2D stays the drafting tool.
- Keep `stitchcore` free of GUI code so the tools and the GUI call the same functions, and cover the tools with tests as in Seamly2D.
- Do this after the first working GUI (step 4). Keep the code in `vmcp` close to the fork so fixes can move between the two repos.

## Critical files (Seamly2D, to copy or reference)
- `common.pri`, `Seamly2D.pro`, `src/src.pro`, `src/libs/libs.pro`
- `src/app/seamlyme/*` (app skeleton), `src/libs/vmisc/vabstractapplication.*`, `vcommonsettings.*`, `def.cpp`
- `src/libs/vlayout/vlayoutpiece.*`, `src/libs/vpatterndb/vpiece.*`, `src/app/seamly2d/xml/vpattern.*`
- `src/libs/vwidgets/vmaingraphicsscene.*`, `vmaingraphicsview.*`

## Verification
- `qmake && make -j` builds on this machine (Qt 6.8 via `qmake6`; Seamly2D no longer builds with Qt 5). Check that `xerces-c` dev headers are installed.
- `stitchcore` unit tests (QtTest) cover: rectangle piece → constant stitch count; trapezoid → expected increase and decrease rows; gauge scaling; grainline rotation.
- Open `src/app/share/samples/patterns/*.sm2d` from Seamly2D (for example the trousers sample), choose a gauge, and check that the chart and written instructions are consistent and export correctly.
- Compare the look and feel side by side with Seamly2D (theme, icons, docks).
