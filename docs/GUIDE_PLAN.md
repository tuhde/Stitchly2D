# Fullscreen rolling-instructions "knit guide" for Raspberry Pi with row counter

## Context
Goal: a program that shows an open dialog for a pattern, then shows its instructions as rolling, fullscreen text; later it follows a knitting machine's row counter and shows the instruction for the current row. Target: Raspberry Pi with touch monitor, fullscreen, later autologin + autostart; needs GPIO and USB access for the row counter.

## Recommendation: a second program in this repo (not a new repo, not a view mode)
- **Not a new repo:** it needs `stitchcore` (grid, shaping, `writeInstructions`, `.stitchly` project format) and the pattern loader (`src/app/stitchly2d/core/pattern_loader.*`), which are still changing. A separate repo means duplicating or vendoring them.
- **Not a view mode of the editor:** different runtime (appliance on a Pi, touch only, no editing/catalog/MCP/undo, must be stable and start fast, owns hardware access). The editor is a big Widgets app with Seamly libs, multimedia and network modules. Hardware access and kiosk lifecycle belong in a small separate process.
- **Chosen:** new app `src/app/guider/` (program name: **guider**) added to `src/app/app.pro` SUBDIRS; same repo, same qmake build, built natively on the Pi (Raspberry Pi OS is Debian, so the README's Qt 6 + Xerces-C requirements apply).

## Structure
1. **Share code.** Move `pattern_loader.*` (and what it needs from `core/`) into a static lib, e.g. `src/libs/stitchload/` (done in phase 1: also holds Application2D, HeadlessWindow, VPattern), used by editor and guide. `stitchcore` stays GUI-free. Guide links only the libs it needs; drop multimedia/network/printsupport from its `QT +=`.
2. **Input:** touch-friendly open dialog (large, non-native `QFileDialog` or own list of recent files) for `.stitchly` (preferred, carries bands/patterns/method) and `.sm2d`. Also a CLI path argument so autostart can open the last file. Remember last file and row (QSettings).
   - Optional later: the editor exports a precomputed guide file (JSON of pieces + steps) so the Pi needs no Seamly parser at all. Not needed for v1.
3. **Model:** extend `stitchcore/instructions.*` to also return structured steps `{firstRow, lastRow, text}` (keep `writeInstructions` as a wrapper so CLI/tests stay valid). Guide maps current row → step.
4. **UI (Qt Widgets, touch):** fullscreen/frameless, large high-contrast fonts, current step centered, previous dimmed above, next below, row + piece header. Touch targets 48+ px: next/prev row, jump to row, piece switch, back to open dialog; kinetic scrolling (`QScroller`) on the list. No hover-only controls; hide cursor. Keys (F11/Esc) for development on desktop.
5. **Row counter abstraction:** `RowCounterSource` interface (`rowChanged(int)`, `connected`, `error`). Implementations:
   - manual (touch buttons) — v1,
   - **GPIO** via libgpiod (works on Pi 4 and 5; sysfs GPIO is deprecated) with debounce, direction, reset; reed switch/optical sensor on a machine's carriage is the typical source,
   - **USB** via QtSerialPort (CDC/Arduino-type counter) — HID/libusb only if the device needs it.
   Row counter runs in a worker thread/object; UI stays responsive. Row source choice and options in a small settings screen.
6. **Autologin/autostart (last phase, docs + files only):** raspi-config autologin, systemd user service or compositor autostart (labwc/wayfire) launching the guide, restart on crash, `QT_QPA_PLATFORM` (wayland/eglfs) note, user in `gpio`/`dialout` groups. Ship an example service file and a short doc in `docs/`.
7. **i18n:** reuse `translations.pri` setup with a new context.

## Passes per row (decided)
A row can need several carriage passes. The counter counts passes; the guide decides when a row is done:
- tubular double-bed machine: every yarn of the row on the front and then on the rear bed (plain row: 2 passes);
- flat on a double-bed machine (new method `double_bed_flat`, for large pieces): one pass per yarn of the row (plain row: 1);
- all other methods: one pass; the colours of the row are worked within it.
The yarns of a row come from the colour patterns (base yarn first, then the pattern's legend order). `stitchcore/passes.*` computes them per row; `KnitPiece::rowPasses` carries them to the guide. The top bar shows the row as `[red,green]  Row 16 / 200` or `[front/rear]  Round 13 / 50` with the pass the machine is at marked. "Pulses per pass" in the settings is the sensor's pulses for one pass (usually 1). Hand moves restart the row at its first pass and forget unfinished pulses.

## Phases
1. (done) Extract loader lib; editor and tests still build/pass.
2. (done) Guide skeleton: open dialog → piece → fullscreen rolling view, manual row control, runs on desktop.
3. (done) Structured steps + tests (`src/test/stitchcore`), offscreen UI test like `src/test/mainwindow`.
4. (in progress: interface, simulated and one-pin GPIO done; settings screen done; USB open) Row counter interface + GPIO and USB sources (needs hardware details).
5. Pi packaging: build notes, autologin/autostart, kiosk behavior.

## Decisions
- Program name: **guider**.
- Target: Raspberry Pi 400 (Pi 4 class, libgpiod fine), X11 (`QT_QPA_PLATFORM=xcb`; autostart via the X11 desktop session / systemd user service), Full HD for now.
- Layout must be screen independent: no fixed pixel sizes; derive font and touch-target sizes from the screen's logical DPI / height (e.g. a base unit = height/40), use layouts, test at several sizes (offscreen test at 800x480, 1920x1080, portrait).

## Open questions (ask before implementing phase 4)
- Row counter hardware: GPIO sensor (which pin, pulse per row, one or two directions) and/or USB device (what protocol)? (Pi 400 exposes the 40-pin header.)

## Verification
- `cd build && qmake6 ../Stitchly2D.pro && make -j8` builds both apps; `src/test` still pass.
- Desktop: run guide on `src/test/data/patterns/male_shirt.sm2d`; open dialog works, rolling view follows row buttons, steps equal `stitchly2d --cli` output.
- Pi: native build, run fullscreen on touch screen, touch navigation works, GPIO/USB source updates the row (simulated pulses first), service restarts after kill and starts after reboot.
