# Working with Claude in Stitchly2D

Stitchly2D can let Claude work on the knitting project that is open in the running window: read the pieces and their
instructions, change the stitch settings, add stitches and patterns to the catalog, and give a piece bands and
patterns. Every change shows up in the window at once, and you can watch it.

The connection uses the Model Context Protocol (MCP). Stitchly2D runs a small MCP server inside the program; Claude
connects to it. Nothing is sent anywhere by Stitchly2D itself: what Claude sees is what its tools return (piece
names, stitch counts, instructions, screenshots), and that goes to whichever Claude client you connected.

The integration is the same one that Seamly2D has in the fork this project grew from (`vmcp` is copied from it), with
a tool set for knitting instead of pattern drafting.

## Safety model

- **Off by default.** Nothing listens until you switch it on in **View > MCP Config and Log**.
- **Local only.** The server listens on `127.0.0.1` and refuses requests that carry a foreign `Host` or `Origin`
  header (protection against web pages talking to it).
- **Token.** Every request needs a random token. It is created once and saved in the Stitchly2D settings (plain
  text, which is acceptable because the server only listens on this computer), so your Claude client stays
  configured. **New token** in the panel replaces it; then configure your client again.
- **File access needs your OK.** Opening a pattern or project, saving a project and exporting ask you in a dialog the
  first time Claude touches a file in a session (the default answer is No; you can allow that file for the rest of the
  session). Declining makes the tool fail and writes nothing.
- **Undoable.** Claude's changes to the stitch settings, the catalog and the bands are steps in the same history as
  yours: **Edit > Undo** (Ctrl+Z) takes them back, and the steps are named "Claude: ...". Claude has `undo` and `redo`
  tools as well. Saving a project or opening a file is not undoable, and opening a pattern clears the history.
- **The catalog file is not touched.** Stitches and patterns that Claude adds are used in this session and are stored
  in the projects you save, but they are only written to your catalog file when you press OK in the Catalog editor.

## Switching it on

1. **View > MCP Config and Log**, tick "Allow Claude to connect".
2. The status bar now shows "Claude: listening on port 47801". While a client keeps sending requests it shows
   "Claude connected (client name)", and after 90 seconds without a request "idle, last request from ... ago".
3. The panel offers the token and ready-made configuration for external clients, the open windows and a log of every
   tool call.

The default port is **47801**, so Stitchly2D and Seamly2D can both offer Claude's connection at the same time
(Seamly2D uses 47800). You can change the port in the panel.

### Several windows

Each window is its own program instance, but Claude is configured once, with one URL and one token. The window that
gets the configured port answers Claude and passes every call on to the window it is meant for; when that window
closes, another one takes the port over. Claude sees `list_patterns` and an optional `pattern` argument (id, file name
or title) on every tool. Without it, the window you used last is taken, so name the project when more than one is open.

## The built-in Claude shell (Linux)

**View > Claude shell** opens a panel with a terminal that runs the Claude Code CLI (`claude`) and connects it to this
window. You type what you want, in your own words:

> Open /home/me/patterns/shirt.sm2d. Knit the cuff in 2:2 rib for the first 3 cm, then stockinette with two-row
> stripes in A and B. Show me the instructions.

The panel needs the `claude` command (Claude Code) to be installed and you to be logged in. It is built only when
QTermWidget (Qt 6) was found while building: on Debian/Ubuntu install `libqtermwidget-dev` before running qmake.
Without it the program builds as before, just without the panel.

## Using another Claude client

Any MCP client that can talk to a local HTTP server with a bearer token can use `http://127.0.0.1:<port>/mcp`. The
panel shows the configuration with the current port and token:

- **Claude Code:** use the `claude mcp add --transport http stitchly2d http://127.0.0.1:47801/mcp --header
  "Authorization: Bearer <token>"` line from the panel, or the `.mcp.json` snippet.
- **Claude Desktop:** configure the same URL and header the way your version supports local servers; depending on the
  version a small local bridge may be needed.

## What Claude can do

Claude learns the tools from the server itself; you do not need to teach it.

| Area | Tools |
|---|---|
| Look | `get_project_info`, `list_pieces` (size, rows, cast-on, widest row, bind-off, bands), `get_instructions`, `get_row_table`, `get_bands`, `get_catalog`, `screenshot` |
| Settings | `set_stitch_settings` (catalog stitch or stitch width and row height in mm, seam allowance, edge stitches, increase and decrease method, from how many stitches an edge is cast on or bound off), `select_piece` |
| Catalog | `set_basic_stitch`, `delete_basic_stitch`, `set_pattern` (a chart of text, top row first), `delete_pattern` |
| Pieces | `set_bands` (bands from the bottom with a basic stitch, a height and patterns laid over them) |
| Files | `open_pattern`, `open_project`, `save_project`, `export_instructions`, `export_row_table` (each asks you first) |
| History | `undo`, `redo` |
| Windows | `list_patterns` |

Things worth knowing:

- The settings, the catalog and the bands are the same ones you edit by hand; what Claude changes you can change back
  in the window.
- A pattern is a chart of text: stitch symbols (`k`, `p`, `C4F`) for a special stitch, yarn names (`A`, `MC`,
  `#c03030`) for a color pattern. The top row is the last row of the repeat; an empty text leaves the base stitch alone.
- `open_project` keeps your catalog entries when they differ from the ones stored in the project, and reports which.
- Opening a pattern starts a new project: the bands of the previous one are dropped. The window warns the user about
  unsaved changes before it is closed or another file is opened from the menu; Claude's `open_pattern` and
  `open_project` mention unsaved changes in the question to the user, because they cannot be undone afterwards.
