# Yarn inventory module for Stitchly2D

## Context
Yarn is only a free-text label today (`PatternDef::legend`, `RowPass::yarn`). The user wants a yarn inventory
that is saved, linked to a pattern and its measurements (like Seamly2D), and usable when knitting in the guider.
Decisions (from the user): manage yarns in **both** the editor and a standalone program; **estimate yarn need and
suggest gauge** from the yarn; the guider **asks for yarn only if the project does not set it**; yarns track
**stock quantity, deduct used yarn, dye lot/notes, color swatch/photo, and gauges per basic stitch type**.

## Design

### 1. Data layer: new files in `src/libs/stitchcore/` (GUI-free, linked everywhere)
- `yarn.h/.cpp`: `struct Yarn { id (UUID), brand, name, fiber, weightClass, colorName, colorHex, imagePath, dyeLot, notes, metersPerSkein, gramsPerSkein, skeinsInStock, partialMetersLeft; QVector<YarnGauge> gauges; }`
  and `YarnGauge { stitchTypeName, StitchSize size, double yarnPerStitchMm (0 = use default estimate) }`.
  Gauge is converted with `StitchSize::fromPer10cm / fromPerInch` (`stitchsize.h`), no stored gauge is introduced.
- `YarnInventory { QVector<Yarn> yarns; load/save/toJson/fromJson }` modelled on `StitchCatalog` (`catalog.cpp:282-355`):
  JSON with `format` id + integer `version`, refuses newer versions, whole-file validation, `QSaveFile`, translated errors via
  `QCoreApplication::translate("Yarn", ...)`. Location: `userYarnPath()` = `AppConfigLocation/yarns.json`
  next to `userCatalogPath()` (`knit_job.cpp:16`). Must verify editor and guider resolve the same dir.
- `yarn_estimate.h/.cpp`: `estimateYarnMeters(const KnitJob/StitchGrid&, slot -> yarn)`: counts stitches per grid band
  (use `StitchGrid::build` output, `GridBand.size`) per color slot; length = stitches × `yarnPerStitchMm`
  (default ≈ 4 × stitch width, user-calibratable per gauge) plus 10% margin; also `suggestStitchSize(yarn, stitchType)`
  and `stockCheck(yarn, neededMeters)`.
- `Project` (`project.h`): optional key `yarns`: `QMap<slot, YarnRef{ id, snapshot name/colorHex }>` (slots = legend names
  `MC`, `CC1`, … of color patterns, else `main`). Optional key with defaults, so `kProjectFileVersion` stays 1 (as `bulkFrom`).
  Snapshot lets the guider work if the inventory file is missing. Update `operator==` helpers, `saveProject`/`loadProject`
  (`project.cpp:115/174`).
- Also stitched into MCP: `mcp/stitch_tools.cpp` gets list/add/update yarn tools (small, after the core works).

### 2. Shared widgets: new lib `src/libs/yarnui/` (QtWidgets)
- `YarnInventoryWidget` (table + edit form with gauge table per stitch type, color picker, image, stock fields, import/export `.styarn`
  later). `YarnPicker` (combo/list) reused by the editor project panel. Add to `libs.pro`, `LIBNAMES` in `stitchly2d.pro`,
  `guider.pro`, standalone `yarnbox.pro`, and tests; add `$$files(...)` for it in `share/translations/translations.pro`.

### 3. Editor (`src/app/stitchly2d/`)
- "Yarns…" dialog (like `catalog_dialog.*`) hosting `YarnInventoryWidget`.
- Project panel: per-slot yarn pickers; shows estimated meters vs stock (green/red) for the current measurements, and a
  "Use yarn gauge" button that fills `StitchSize` via `suggestStitchSize`. Changes go through `StateCommand`/`recordChange`
  (`mainwindow.cpp` State, `saveProject` 1252, `openProject` 1286, `settings()` 1739).

### 4. Standalone program `src/app/yarnbox/`
- Third executable in `src/app/app.pro`; small `main.cpp` with `Application2D`, `--lang`, hosting `YarnInventoryWidget`
  on the same `yarns.json`. Optional desktop entry in `share/`.

### 5. Guider (`src/app/guider/`)
- In `GuideWindow::openFile` (`guide_window.cpp:124`) after `buildKnitJob`: if the project has no yarn for a required slot,
  show a new touch dialog `yarn_dialog.*` (modelled on `OpenDialog`/`SettingsDialog`, `guideUnit` scaling, add to `guider.pri`).
  Choice remembered per file in `QSettings` (`yarn/<base64 path>` pattern, as `rows/...`). Skipped when project sets yarns.
- Top bar shows yarn name/color swatch (and dye lot) for the current pass (`RowPass::yarn`).
- When the last row of a piece is passed, offer "Deduct ≈ X m from stock?" (confirm only; once per file/piece flag in QSettings),
  writing back `yarns.json` through `YarnInventory::save`.
- Guider with no inventory file: dialog offers "Continue without yarn".

### 6. Docs, translations, naming
- `docs/YARN_PLAN.md` (design + file format, like `PATTERN_FILE_FORMAT.md`), README source-layout table, `docs/PLAN.md` status.
- Every new string gets a German translation (`make lupdate`, `stitchly2d_de_DE.ts`); `translationsComplete` fails otherwise.
- Use "outside/inside" wording only, never RS/WS (memory).
- Commit after each finished phase; push only on request.

## Phases (status: all phases done)
1. `stitchcore`: Yarn model, inventory JSON, estimate, project `yarns` key + tests in `tst_stitchcore`.
2. `yarnui` lib + editor dialog + project-panel pickers/estimate/gauge suggestion (+ `tst_mainwindow`).
3. Guider yarn dialog, remembered choice, yarn display, stock deduction (+ `tst_guider`).
4. `yarnbox` standalone program.
5. MCP tools, docs, German translations, README.

## Verification
- `cd build && qmake6 .. && make`; run `tst_stitchcore`, `tst_mainwindow`, `tst_guider` with `QT_QPA_PLATFORM=offscreen`
  and **check exit codes** (memory tip).
- Tests: inventory JSON round-trip, newer-version refusal, project round-trip with/without `yarns` (old files still load),
  estimate on a sample from `src/test/data/` (`cuff.stitchly`), guider shows the picker only when yarns are unset, and
  deduction updates stock.
- Manual: `--screenshot` of guider yarn dialog at several sizes; run editor, add yarn, link, see estimate; run `yarnbox` on
  the same file.
