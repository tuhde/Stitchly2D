#include "stitch_tools.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QSaveFile>
#include <QSet>

#include "../mainwindow.h"
#include "../../../libs/stitchcore/catalog.h"
#include "../../../libs/stitchcore/project.h"
#include "../../../libs/vmcp/mcptoolregistry.h"

using namespace stitchcore;

namespace
{
// ---- JSON schema helpers --------------------------------------------------------------------------------------------
QJsonObject typed(const char *type, const QString &description)
{
    return QJsonObject{{QStringLiteral("type"), QLatin1String(type)}, {QStringLiteral("description"), description}};
}
QJsonObject str(const QString &d) { return typed("string", d); }
QJsonObject num(const QString &d) { return typed("number", d); }
QJsonObject integer(const QString &d) { return typed("integer", d); }
QJsonObject flag(const QString &d) { return typed("boolean", d); }
QJsonObject oneOf(const QStringList &values, const QString &d)
{
    QJsonObject o = str(d);
    o.insert(QStringLiteral("enum"), QJsonArray::fromStringList(values));
    return o;
}
QJsonObject object(const QJsonObject &properties, const QStringList &required = QStringList())
{
    QJsonObject o{{QStringLiteral("type"), QStringLiteral("object")}, {QStringLiteral("properties"), properties}};
    if (!required.isEmpty())
    {
        o.insert(QStringLiteral("required"), QJsonArray::fromStringList(required));
    }
    return o;
}

// ---- arguments ------------------------------------------------------------------------------------------------------
QString text(const QJsonObject &a, const char *key)
{
    return a.value(QLatin1String(key)).toString().trimmed();
}
bool has(const QJsonObject &a, const char *key)
{
    return a.contains(QLatin1String(key)) && !a.value(QLatin1String(key)).isNull();
}

// ---- file access: asks the user ---------------------------------------------------------------------------------------
QSet<QString> &allowedFiles()
{
    static QSet<QString> allowed;
    return allowed;
}

bool confirmFileAccess(QWidget *parent, const QString &title, const QString &message, const QString &key)
{
    if (allowedFiles().contains(key))
    {
        return true;
    }
    // Test hook: automated tests cannot click a dialog. Never set this in normal use.
    if (qEnvironmentVariableIsSet("STITCHLY_MCP_ASSUME_YES"))
    {
        return true;
    }
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(title);
    box.setText(message);
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    auto *remember = new QCheckBox(QObject::tr("Don't ask again for this file in this session"), &box);
    box.setCheckBox(remember);
    if (box.exec() != QMessageBox::Yes)
    {
        return false;
    }
    if (remember->isChecked())
    {
        allowedFiles().insert(key);
    }
    return true;
}

QString absolute(const QString &path)
{
    return QFileInfo(path).absoluteFilePath();
}

// ---- conversions ----------------------------------------------------------------------------------------------------
QString increaseText(IncreaseMethod m)
{
    return m == IncreaseMethod::YarnOver ? QStringLiteral("yarnover")
         : m == IncreaseMethod::KFB      ? QStringLiteral("kfb")
                                         : QStringLiteral("m1");
}
QString decreaseText(DecreaseMethod m)
{
    return m == DecreaseMethod::K2tog ? QStringLiteral("k2tog") : QStringLiteral("k2tog_ssk");
}

QJsonObject settingsJson(const ProjectSettings &s)
{
    return QJsonObject{{QStringLiteral("stitchType"), s.stitchType.isEmpty() ? QStringLiteral("custom") : s.stitchType},
                       {QStringLiteral("stitchWidthMm"), s.size.stitchWidthMm},
                       {QStringLiteral("rowHeightMm"), s.size.rowHeightMm},
                       {QStringLiteral("stitchesPer10cm"), s.size.stitchesPer10cm()},
                       {QStringLiteral("rowsPer10cm"), s.size.rowsPer10cm()},
                       {QStringLiteral("includeSeamAllowance"), s.includeSeamAllowance},
                       {QStringLiteral("edgeStitches"), s.shaping.edgeStitches},
                       {QStringLiteral("bulkFrom"), s.shaping.bulkFrom},
                       {QStringLiteral("shapeOn"), s.shaping.shapeOn == ShapingRows::EveryRow ? QStringLiteral("every") : QStringLiteral("outside")},
                       {QStringLiteral("increase"), increaseText(s.shaping.increase)},
                       {QStringLiteral("decrease"), decreaseText(s.shaping.decrease)}};
}

QJsonObject bandJson(const BandSpec &b)
{
    QJsonArray overlays;
    for (const Overlay &o : b.overlays)
    {
        overlays.append(QJsonObject{{QStringLiteral("pattern"), o.pattern},
                                    {QStringLiteral("firstRow"), o.firstRow},
                                    {QStringLiteral("rowCount"), o.rowCount},
                                    {QStringLiteral("firstStitch"), o.firstStitch},
                                    {QStringLiteral("stitchCount"), o.stitchCount},
                                    {QStringLiteral("shiftX"), o.offsetX},
                                    {QStringLiteral("shiftY"), o.offsetY}});
    }
    return QJsonObject{{QStringLiteral("stitchType"), b.stitchType},
                       {QStringLiteral("heightMm"), b.heightMm},
                       {QStringLiteral("patterns"), overlays}};
}

QJsonObject patternJson(const PatternDef &p)
{
    QJsonArray rows;
    for (const QStringList &row : p.toChart()) rows.append(QJsonArray::fromStringList(row));
    return QJsonObject{{QStringLiteral("name"), p.name},
                       {QStringLiteral("kind"), p.kind == PatternDef::Color ? QStringLiteral("color")
                                                                              : QStringLiteral("special")},
                       {QStringLiteral("width"), p.width},
                       {QStringLiteral("height"), p.height},
                       {QStringLiteral("rows"), rows}};
}

QJsonObject catalogJson(const StitchCatalog &c)
{
    QJsonArray basics;
    for (const StitchType &t : c.basics().types())
    {
        basics.append(QJsonObject{{QStringLiteral("name"), t.name},
                                  {QStringLiteral("stitchWidthMm"), t.size.stitchWidthMm},
                                  {QStringLiteral("rowHeightMm"), t.size.rowHeightMm}});
    }
    QJsonArray patterns;
    for (const PatternDef &p : c.patterns()) patterns.append(patternJson(p));
    return QJsonObject{{QStringLiteral("basicStitches"), basics}, {QStringLiteral("patterns"), patterns}};
}

QJsonArray stringArray(const QStringList &list) { return QJsonArray::fromStringList(list); }

// ---- the piece an argument names; default is the selected one ---------------------------------------------------------
bool pieceArgument(MainWindow *w, const QJsonObject &a, QString &piece, McpToolResult &failure)
{
    piece = text(a, "piece");
    if (piece.isEmpty())
    {
        piece = w->currentPiece();
    }
    if (piece.isEmpty())
    {
        failure = McpToolResult::error(QStringLiteral("No pattern is open. Use open_pattern or open_project first."));
        return false;
    }
    if (!w->pieceNames().contains(piece))
    {
        failure = McpToolResult::error(QStringLiteral("No piece named \"%1\". The pieces are: %2.")
                                           .arg(piece, w->pieceNames().join(QStringLiteral(", "))));
        return false;
    }
    return true;
}

McpToolResult writeTextFile(const QString &path, const QString &content)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(content.toUtf8()) < 0 || !f.commit())
    {
        return McpToolResult::error(QStringLiteral("Could not write %1: %2").arg(path, f.errorString()));
    }
    return McpToolResult::ok(QStringLiteral("Wrote %1").arg(path));
}

/** While a tool runs, the undo steps it makes are labelled as Claude's. */
struct UndoLabel
{
    explicit UndoLabel(MainWindow *window) : m_window(window) { m_window->setUndoPrefix(QStringLiteral("Claude: ")); }
    ~UndoLabel() { m_window->setUndoPrefix(QString()); }
    MainWindow *m_window;
};

McpTool tool(MainWindow *window, const QString &name, const QString &description, const QJsonObject &schema,
             bool readOnly, McpToolHandler handler)
{
    McpTool t;
    t.name = name;
    t.description = description;
    t.inputSchema = schema;
    t.readOnly = readOnly;
    t.handler = [window, handler](const QJsonObject &arguments) {
        UndoLabel label(window);
        return handler(arguments);
    };
    return t;
}
}

void registerStitchTools(McpToolRegistry &registry, MainWindow *w)
{
    const QJsonObject noArguments = object(QJsonObject());
    const QJsonObject pieceOnly = object({{QStringLiteral("piece"), str(QStringLiteral("Piece name; default: the selected piece."))}});

    // ---- Look --------------------------------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("get_project_info"),
        QStringLiteral("State of the open project: pattern file, project file, the pieces, the selected piece, the stitch "
                       "settings and what the catalog contains. Call this first."),
        noArguments, true, [w](const QJsonObject &) {
            QJsonObject info = w->mcpProjectInfo();
            info.insert(QStringLiteral("pieces"), stringArray(w->pieceNames()));
            info.insert(QStringLiteral("settings"), settingsJson(w->settings()));
            QStringList basics, patterns;
            for (const StitchType &t : w->catalog().basics().types()) basics << t.name;
            for (const PatternDef &p : w->catalog().patterns()) patterns << p.name;
            info.insert(QStringLiteral("basicStitches"), stringArray(basics));
            info.insert(QStringLiteral("patterns"), stringArray(patterns));
            return McpToolResult::ok(QStringLiteral("Project info:"), info);
        }));

    registry.registerTool(tool(w,
        QStringLiteral("list_pieces"),
        QStringLiteral("All pieces with their size in mm and, with the current settings, rows, cast-on, widest row, "
                       "bind-off and bands."),
        noArguments, true, [w](const QJsonObject &) {
            QJsonArray pieces;
            for (const QString &name : w->pieceNames()) pieces.append(w->pieceSummary(name));
            return McpToolResult::ok(QStringLiteral("%1 piece(s):").arg(pieces.size()),
                                     QJsonObject{{QStringLiteral("pieces"), pieces}});
        }));

    registry.registerTool(tool(w,
        QStringLiteral("get_instructions"),
        QStringLiteral("The written knitting instructions of a piece (cast on, rows, shaping, bind off), as shown in the "
                       "Instructions panel."),
        pieceOnly, true, [w](const QJsonObject &a) {
            QString piece;
            McpToolResult failure;
            if (!pieceArgument(w, a, piece, failure)) return failure;
            const QStringList lines = w->instructionsFor(piece);
            return McpToolResult::ok(lines.join(QLatin1Char('\n')),
                                     QJsonObject{{QStringLiteral("piece"), piece},
                                                 {QStringLiteral("summary"), w->pieceSummary(piece)}});
        }));

    registry.registerTool(tool(w,
        QStringLiteral("get_row_table"),
        QStringLiteral("Row table of a piece as CSV: row, band, stitch type, first stitch, stitch count and the change "
                       "at the left and right edge."),
        pieceOnly, true, [w](const QJsonObject &a) {
            QString piece;
            McpToolResult failure;
            if (!pieceArgument(w, a, piece, failure)) return failure;
            return McpToolResult::ok(w->rowTableFor(piece));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("screenshot"),
        QStringLiteral("Picture of the Stitchly2D window: the selected piece with its stitch grid, the settings and the "
                       "instructions. Look at it to check the result."),
        noArguments, true, [w](const QJsonObject &) {
            McpToolResult r = McpToolResult::ok(QStringLiteral("Screenshot of the Stitchly2D window"));
            r.imagePng = w->screenshotPng();
            return r;
        }));

    registry.registerTool(tool(w,
        QStringLiteral("select_piece"),
        QStringLiteral("Show a piece in the window (the user sees it on the canvas)."),
        object({{QStringLiteral("piece"), str(QStringLiteral("Piece name."))}}, {QStringLiteral("piece")}), false,
        [w](const QJsonObject &a) {
            const QString piece = text(a, "piece");
            if (!w->selectPiece(piece))
            {
                return McpToolResult::error(QStringLiteral("No piece named \"%1\". The pieces are: %2.")
                                                .arg(piece, w->pieceNames().join(QStringLiteral(", "))));
            }
            return McpToolResult::ok(QStringLiteral("Selected %1").arg(piece), w->pieceSummary(piece));
        }));

    // ---- Stitch settings ---------------------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("set_stitch_settings"),
        QStringLiteral("Change the stitch settings shared by all pieces. All fields are optional. `stitchType` takes a "
                       "basic stitch of the catalog (its width and height are used) or \"custom\"; "
                       "`stitchWidthMm` and `rowHeightMm` set the size of one stitch and of one row in mm (this makes the "
                       "stitch custom if it differs from the catalog). Bands that use \"Default stitch\" follow these "
                       "settings."),
        object({{QStringLiteral("stitchType"), str(QStringLiteral("Catalog stitch name, or \"custom\"."))},
                {QStringLiteral("stitchWidthMm"), num(QStringLiteral("Width of one stitch in mm."))},
                {QStringLiteral("rowHeightMm"), num(QStringLiteral("Height of one row in mm."))},
                {QStringLiteral("includeSeamAllowance"), flag(QStringLiteral("Knit the outline with seam allowance."))},
                {QStringLiteral("edgeStitches"), integer(QStringLiteral("Plain stitches kept at each edge before shaping."))},
                {QStringLiteral("bulkFrom"), integer(QStringLiteral("A change of at least this many stitches at one edge is cast on or bound off instead of increased or decreased in one row; 0 = never."))},
                {QStringLiteral("shapeOn"), oneOf({QStringLiteral("outside"), QStringLiteral("every")},
                                                  QStringLiteral("Rows on which the edges are shaped: outside rows only (row 1 is an outside row; a change needed on an inside row is worked on the outside row before it) or every row."))},
                {QStringLiteral("increase"), oneOf({QStringLiteral("m1"), QStringLiteral("yarnover"), QStringLiteral("kfb")},
                                                   QStringLiteral("How increases are worked."))},
                {QStringLiteral("decrease"), oneOf({QStringLiteral("k2tog_ssk"), QStringLiteral("k2tog")},
                                                   QStringLiteral("How decreases are worked."))}}),
        false, [w](const QJsonObject &a) {
            ProjectSettings s = w->settings();
            if (has(a, "stitchType"))
            {
                const QString name = text(a, "stitchType");
                if (name.compare(QStringLiteral("custom"), Qt::CaseInsensitive) == 0)
                {
                    s.stitchType.clear();
                }
                else if (w->catalog().basics().contains(name))
                {
                    s.stitchType = name;
                    s.size = w->catalog().basics().find(name).size;
                }
                else
                {
                    QStringList names;
                    for (const StitchType &t : w->catalog().basics().types()) names << t.name;
                    return McpToolResult::error(QStringLiteral("Unknown basic stitch \"%1\". The catalog has: %2.")
                                                    .arg(name, names.join(QStringLiteral(", "))));
                }
            }
            if (has(a, "stitchWidthMm")) s.size.stitchWidthMm = a.value(QStringLiteral("stitchWidthMm")).toDouble();
            if (has(a, "rowHeightMm")) s.size.rowHeightMm = a.value(QStringLiteral("rowHeightMm")).toDouble();
            if (!s.size.isValid() || s.size.stitchWidthMm > 100 || s.size.rowHeightMm > 100)
            {
                return McpToolResult::error(QStringLiteral("stitchWidthMm and rowHeightMm must be between 0 and 100."));
            }
            if (has(a, "includeSeamAllowance")) s.includeSeamAllowance = a.value(QStringLiteral("includeSeamAllowance")).toBool();
            if (has(a, "edgeStitches"))
            {
                const int n = a.value(QStringLiteral("edgeStitches")).toInt(-1);
                if (n < 0 || n > 10) return McpToolResult::error(QStringLiteral("edgeStitches must be between 0 and 10."));
                s.shaping.edgeStitches = n;
            }
            if (has(a, "shapeOn"))
            {
                const QString v = text(a, "shapeOn");
                if (v == QLatin1String("outside")) s.shaping.shapeOn = ShapingRows::OutsideRows;
                else if (v == QLatin1String("every")) s.shaping.shapeOn = ShapingRows::EveryRow;
                else return McpToolResult::error(QStringLiteral("shapeOn must be outside or every."));
            }
            if (has(a, "bulkFrom"))
            {
                const int n = a.value(QStringLiteral("bulkFrom")).toInt(-1);
                if (n < 0 || n > 50) return McpToolResult::error(QStringLiteral("bulkFrom must be between 0 and 50."));
                s.shaping.bulkFrom = n;
            }
            if (has(a, "increase"))
            {
                const QString v = text(a, "increase");
                if (v == QLatin1String("m1")) s.shaping.increase = IncreaseMethod::M1;
                else if (v == QLatin1String("yarnover")) s.shaping.increase = IncreaseMethod::YarnOver;
                else if (v == QLatin1String("kfb")) s.shaping.increase = IncreaseMethod::KFB;
                else return McpToolResult::error(QStringLiteral("increase must be m1, yarnover or kfb."));
            }
            if (has(a, "decrease"))
            {
                const QString v = text(a, "decrease");
                if (v == QLatin1String("k2tog_ssk")) s.shaping.decrease = DecreaseMethod::K2tog_Ssk;
                else if (v == QLatin1String("k2tog")) s.shaping.decrease = DecreaseMethod::K2tog;
                else return McpToolResult::error(QStringLiteral("decrease must be k2tog_ssk or k2tog."));
            }
            w->setSettings(s, QObject::tr("Change stitch settings"));
            return McpToolResult::ok(QStringLiteral("Stitch settings changed:"), settingsJson(w->settings()));
        }));

    // ---- Catalog -----------------------------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("get_catalog"),
        QStringLiteral("The catalog: basic stitches (name, stitch width, row height in mm) and special / color patterns. "
                       "A pattern is a chart of text: `rows` lists the rows top first, the last row is the first row "
                       "knitted; an empty string leaves the base stitch alone."),
        noArguments, true, [w](const QJsonObject &) {
            return McpToolResult::ok(QStringLiteral("Catalog:"), catalogJson(w->catalog()));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("set_basic_stitch"),
        QStringLiteral("Add a basic stitch to the catalog or change an existing one. Changes apply to this session and are "
                       "stored in saved projects; the user keeps them in the catalog file by pressing OK in the Catalog "
                       "editor."),
        object({{QStringLiteral("name"), str(QStringLiteral("Name, e.g. \"Seed stitch rib\"."))},
                {QStringLiteral("stitchWidthMm"), num(QStringLiteral("Width of one stitch in mm."))},
                {QStringLiteral("rowHeightMm"), num(QStringLiteral("Height of one row in mm."))}},
               {QStringLiteral("name"), QStringLiteral("stitchWidthMm"), QStringLiteral("rowHeightMm")}),
        false, [w](const QJsonObject &a) {
            StitchType t;
            t.name = text(a, "name");
            t.size.stitchWidthMm = a.value(QStringLiteral("stitchWidthMm")).toDouble();
            t.size.rowHeightMm = a.value(QStringLiteral("rowHeightMm")).toDouble();
            if (t.name.isEmpty() || t.name == QLatin1String(kDefaultStitchName))
            {
                return McpToolResult::error(QStringLiteral("Give the stitch a name (not \"%1\").").arg(QLatin1String(kDefaultStitchName)));
            }
            if (!t.size.isValid() || t.size.stitchWidthMm > 100 || t.size.rowHeightMm > 100)
            {
                return McpToolResult::error(QStringLiteral("stitchWidthMm and rowHeightMm must be between 0 and 100."));
            }
            StitchCatalog c = w->catalog();
            const bool existed = c.basics().contains(t.name);
            c.basics().set(t);
            w->setCatalog(c, QObject::tr("%1 basic stitch %2").arg(existed ? QObject::tr("Change") : QObject::tr("Add"), t.name));
            return McpToolResult::ok(QStringLiteral("%1 basic stitch %2").arg(existed ? QStringLiteral("Changed") : QStringLiteral("Added"), t.name));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("delete_basic_stitch"),
        QStringLiteral("Remove a basic stitch from the catalog. Bands that used it fall back to the default stitch."),
        object({{QStringLiteral("name"), str(QStringLiteral("Name of the stitch."))}}, {QStringLiteral("name")}), false,
        [w](const QJsonObject &a) {
            StitchCatalog c = w->catalog();
            const QString name = text(a, "name");
            if (!c.basics().remove(name))
            {
                return McpToolResult::error(QStringLiteral("No basic stitch named \"%1\".").arg(name));
            }
            w->setCatalog(c, QObject::tr("Delete basic stitch %1").arg(name));
            return McpToolResult::ok(QStringLiteral("Removed %1").arg(name));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("set_pattern"),
        QStringLiteral("Add a special-stitch or color pattern to the catalog, or replace one with the same name. The "
                       "pattern is the chart of one repeat: `rows` is a list of rows, top row first (the last row is the "
                       "first row knitted), each row a list of texts of equal length. Special patterns use stitch "
                       "symbols (\"k\", \"p\", \"C4F\"), color patterns yarn names (\"A\", \"MC\", \"#c03030\"); \"\" "
                       "leaves the base stitch alone. Lay it over a band with set_bands."),
        object({{QStringLiteral("name"), str(QStringLiteral("Pattern name."))},
                {QStringLiteral("kind"), oneOf({QStringLiteral("special"), QStringLiteral("color")},
                                               QStringLiteral("Stitch symbols or yarn colors."))},
                {QStringLiteral("rows"),
                 QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                             {QStringLiteral("description"), QStringLiteral("Rows of the chart, top row first.")},
                             {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                                                                   {QStringLiteral("items"), QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}}}}},
               {QStringLiteral("name"), QStringLiteral("kind"), QStringLiteral("rows")}),
        false, [w](const QJsonObject &a) {
            const QString kindText = text(a, "kind");
            if (kindText != QLatin1String("special") && kindText != QLatin1String("color"))
            {
                return McpToolResult::error(QStringLiteral("kind must be \"special\" or \"color\"."));
            }
            QVector<QStringList> rows;
            for (const QJsonValue &rv : a.value(QStringLiteral("rows")).toArray())
            {
                QStringList row;
                for (const QJsonValue &cell : rv.toArray())
                {
                    row << cell.toString();
                }
                rows.append(row);
            }
            PatternDef p;
            QString error;
            if (!PatternDef::fromChart(text(a, "name"), kindText == QLatin1String("color") ? PatternDef::Color : PatternDef::Special,
                                       rows, p, &error))
            {
                return McpToolResult::error(error);
            }
            if (p.width > 200 || p.height > 200)
            {
                return McpToolResult::error(QStringLiteral("A repeat can be at most 200 stitches wide and 200 rows high."));
            }
            StitchCatalog c = w->catalog();
            const bool existed = c.contains(p.name);
            c.set(p);
            w->setCatalog(c, QObject::tr("%1 pattern %2").arg(existed ? QObject::tr("Change") : QObject::tr("Add"), p.name));
            return McpToolResult::ok(QStringLiteral("%1 pattern %2 (%3 x %4)")
                                         .arg(existed ? QStringLiteral("Changed") : QStringLiteral("Added"), p.name)
                                         .arg(p.width).arg(p.height),
                                     patternJson(p));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("delete_pattern"),
        QStringLiteral("Remove a pattern from the catalog. It is taken off every band that used it."),
        object({{QStringLiteral("name"), str(QStringLiteral("Pattern name."))}}, {QStringLiteral("name")}), false,
        [w](const QJsonObject &a) {
            StitchCatalog c = w->catalog();
            const QString name = text(a, "name");
            if (!c.remove(name))
            {
                return McpToolResult::error(QStringLiteral("No pattern named \"%1\".").arg(name));
            }
            w->setCatalog(c, QObject::tr("Delete pattern %1").arg(name));
            return McpToolResult::ok(QStringLiteral("Removed %1").arg(name));
        }));

    // ---- Bands and patterns ------------------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("get_bands"),
        QStringLiteral("The bands of a piece, from the bottom up, with the patterns laid over them."),
        pieceOnly, true, [w](const QJsonObject &a) {
            QString piece;
            McpToolResult failure;
            if (!pieceArgument(w, a, piece, failure)) return failure;
            QJsonArray bands;
            for (const BandSpec &b : w->bands(piece)) bands.append(bandJson(b));
            return McpToolResult::ok(QStringLiteral("Bands of %1:").arg(piece),
                                     QJsonObject{{QStringLiteral("piece"), piece}, {QStringLiteral("bands"), bands}});
        }));

    const QJsonObject overlaySchema = object(
        {{QStringLiteral("pattern"), str(QStringLiteral("Name of a pattern of the catalog."))},
         {QStringLiteral("firstRow"), integer(QStringLiteral("First row of the band the pattern covers, counted from 0."))},
         {QStringLiteral("rowCount"), integer(QStringLiteral("Rows covered; 0 = up to the end of the band."))},
         {QStringLiteral("firstStitch"), integer(QStringLiteral("First stitch covered, counted from the first stitch of each row, from 0."))},
         {QStringLiteral("stitchCount"), integer(QStringLiteral("Stitches covered; 0 = up to the end of the row."))},
         {QStringLiteral("shiftX"), integer(QStringLiteral("Shifts the repeat sideways."))},
         {QStringLiteral("shiftY"), integer(QStringLiteral("Shifts the repeat up."))}},
        {QStringLiteral("pattern")});
    const QJsonObject bandSchema = object(
        {{QStringLiteral("stitchType"), str(QStringLiteral("Basic stitch of the catalog, or \"Default stitch\" for the one in the settings."))},
         {QStringLiteral("heightMm"), num(QStringLiteral("Height of the band in mm. Not needed for the last band, which reaches the top."))},
         {QStringLiteral("patterns"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                                                  {QStringLiteral("description"), QStringLiteral("Patterns laid over this band.")},
                                                  {QStringLiteral("items"), overlaySchema}}}},
        {QStringLiteral("stitchType")});
    registry.registerTool(tool(w,
        QStringLiteral("set_bands"),
        QStringLiteral("Replace the bands of a piece. A piece is knitted from the bottom up in bands; each band has a basic "
                       "stitch, a height in mm (the last band reaches the top) and optionally special-stitch or color "
                       "patterns laid over it. At a change of stitch the stitch count changes evenly across the row. "
                       "Use get_instructions or screenshot to check the result."),
        object({{QStringLiteral("piece"), str(QStringLiteral("Piece name; default: the selected piece."))},
                {QStringLiteral("bands"), QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                                                      {QStringLiteral("description"), QStringLiteral("Bands from the bottom up.")},
                                                      {QStringLiteral("items"), bandSchema}}}},
               {QStringLiteral("bands")}),
        false, [w](const QJsonObject &a) {
            QString piece;
            McpToolResult failure;
            if (!pieceArgument(w, a, piece, failure)) return failure;
            QVector<BandSpec> bands;
            for (const QJsonValue &bv : a.value(QStringLiteral("bands")).toArray())
            {
                const QJsonObject bj = bv.toObject();
                BandSpec b;
                b.stitchType = text(bj, "stitchType");
                b.heightMm = bj.value(QStringLiteral("heightMm")).toDouble(0);
                for (const QJsonValue &ov : bj.value(QStringLiteral("patterns")).toArray())
                {
                    const QJsonObject oj = ov.toObject();
                    Overlay o;
                    o.pattern = text(oj, "pattern");
                    o.firstRow = oj.value(QStringLiteral("firstRow")).toInt(0);
                    o.rowCount = oj.value(QStringLiteral("rowCount")).toInt(0);
                    o.firstStitch = oj.value(QStringLiteral("firstStitch")).toInt(0);
                    o.stitchCount = oj.value(QStringLiteral("stitchCount")).toInt(0);
                    o.offsetX = oj.value(QStringLiteral("shiftX")).toInt(0);
                    o.offsetY = oj.value(QStringLiteral("shiftY")).toInt(0);
                    b.overlays.append(o);
                }
                bands.append(b);
            }
            QString error;
            if (!w->setBands(piece, bands, &error))
            {
                return McpToolResult::error(error);
            }
            w->selectPiece(piece);
            return McpToolResult::ok(QStringLiteral("Bands of %1 set:").arg(piece), w->pieceSummary(piece));
        }));

    // ---- Undo ---------------------------------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("undo"),
        QStringLiteral("Undo the last change to the stitch settings, the catalog or the bands (also the user's own "
                       "changes). Returns what was undone. Steps made by Claude are listed as \"Claude: ...\"."),
        noArguments, false, [w](const QJsonObject &) {
            const QString what = w->undo();
            if (what.isEmpty())
            {
                return McpToolResult::error(QStringLiteral("There is nothing to undo."));
            }
            return McpToolResult::ok(QStringLiteral("Undone: %1").arg(what),
                                     QJsonObject{{QStringLiteral("undone"), what},
                                                 {QStringLiteral("nextUndo"), stringArray(w->undoTexts().mid(0, 5))}});
        }));
    registry.registerTool(tool(w,
        QStringLiteral("redo"),
        QStringLiteral("Redo what was undone last. Returns what was redone."),
        noArguments, false, [w](const QJsonObject &) {
            const QString what = w->redo();
            if (what.isEmpty())
            {
                return McpToolResult::error(QStringLiteral("There is nothing to redo."));
            }
            return McpToolResult::ok(QStringLiteral("Redone: %1").arg(what));
        }));

    // ---- Files (the user is asked) -----------------------------------------------------------------------------------
    registry.registerTool(tool(w,
        QStringLiteral("open_pattern"),
        QStringLiteral("Open a Seamly2D pattern (.sm2d) with its measurements. This starts a new project: bands and "
                       "patterns of the previous one are dropped. The user is asked to allow reading the file."),
        object({{QStringLiteral("path"), str(QStringLiteral("Path of the .sm2d file."))},
                {QStringLiteral("measurements"), str(QStringLiteral("Optional measurement file (.smis) to use instead of the one named in the pattern."))}},
               {QStringLiteral("path")}),
        false, [w](const QJsonObject &a) {
            const QString path = absolute(text(a, "path"));
            const QString measurements = text(a, "measurements").isEmpty() ? QString() : absolute(text(a, "measurements"));
            if (text(a, "path").isEmpty() || !QFileInfo::exists(path))
            {
                return McpToolResult::error(QStringLiteral("File not found: %1").arg(path));
            }
            if (!confirmFileAccess(w, QObject::tr("Claude wants to open a pattern"),
                                   QObject::tr("Allow Claude to open this pattern?%1\n\n%2")
                                       .arg(w->isModified() ? QObject::tr("\n\nThe current project has unsaved changes, which will be lost.") : QString(), path), path))
            {
                return McpToolResult::error(QStringLiteral("The user did not allow opening %1").arg(path));
            }
            QString error;
            if (!w->openPattern(path, &error, measurements))
            {
                return McpToolResult::error(error);
            }
            return McpToolResult::ok(QStringLiteral("Opened %1 with %2 piece(s): %3")
                                         .arg(path).arg(w->pieceNames().size()).arg(w->pieceNames().join(QStringLiteral(", "))),
                                     w->mcpProjectInfo());
        }));

    registry.registerTool(tool(w,
        QStringLiteral("open_project"),
        QStringLiteral("Open a Stitchly2D project (.stitchly): its pattern, settings, bands and patterns. If catalog "
                       "entries of the project differ from the user's catalog, the user's entries are kept and the names "
                       "are reported. The user is asked to allow reading the file."),
        object({{QStringLiteral("path"), str(QStringLiteral("Path of the .stitchly file."))}}, {QStringLiteral("path")}),
        false, [w](const QJsonObject &a) {
            const QString path = absolute(text(a, "path"));
            if (text(a, "path").isEmpty() || !QFileInfo::exists(path))
            {
                return McpToolResult::error(QStringLiteral("File not found: %1").arg(path));
            }
            if (!confirmFileAccess(w, QObject::tr("Claude wants to open a project"),
                                   QObject::tr("Allow Claude to open this project?%1\n\n%2")
                                       .arg(w->isModified() ? QObject::tr("\n\nThe current project has unsaved changes, which will be lost.") : QString(), path), path))
            {
                return McpToolResult::error(QStringLiteral("The user did not allow opening %1").arg(path));
            }
            QString error;
            Project project;
            if (!loadProject(path, project, &error))
            {
                return McpToolResult::error(error);
            }
            const CatalogDiff diff = diffCatalog(w->catalog(), project.catalog);
            if (!w->openProject(path, &error, MainWindow::CatalogConflict::KeepMine))
            {
                return McpToolResult::error(error);
            }
            QJsonObject info = w->mcpProjectInfo();
            info.insert(QStringLiteral("catalogEntriesKeptFromUser"), stringArray(diff.differing));
            return McpToolResult::ok(QStringLiteral("Opened project %1").arg(path), info);
        }));

    registry.registerTool(tool(w,
        QStringLiteral("save_project"),
        QStringLiteral("Save the project (settings, bands, patterns and the catalog entries they use) as a .stitchly file. "
                       "The user is asked to allow writing the file."),
        object({{QStringLiteral("path"), str(QStringLiteral("Where to save, ending in .stitchly."))}}, {QStringLiteral("path")}),
        false, [w](const QJsonObject &a) {
            QString path = absolute(text(a, "path"));
            if (text(a, "path").isEmpty())
            {
                return McpToolResult::error(QStringLiteral("Give a path."));
            }
            if (QFileInfo(path).suffix().isEmpty())
            {
                path += QStringLiteral(".stitchly");
            }
            if (!confirmFileAccess(w, QObject::tr("Claude wants to save the project"),
                                   QObject::tr("Allow Claude to write this file?\n\n%1").arg(path), path))
            {
                return McpToolResult::error(QStringLiteral("The user did not allow writing %1").arg(path));
            }
            QString error;
            if (!w->saveProject(path, &error))
            {
                return McpToolResult::error(error);
            }
            return McpToolResult::ok(QStringLiteral("Saved %1").arg(path));
        }));

    registry.registerTool(tool(w,
        QStringLiteral("export_instructions"),
        QStringLiteral("Write the written instructions of all pieces to a text file. The user is asked to allow writing "
                       "the file."),
        object({{QStringLiteral("path"), str(QStringLiteral("Where to write, e.g. /home/me/knit.txt."))}}, {QStringLiteral("path")}),
        false, [w](const QJsonObject &a) {
            const QString path = absolute(text(a, "path"));
            if (text(a, "path").isEmpty() || w->pieceNames().isEmpty())
            {
                return McpToolResult::error(QStringLiteral("Give a path, and open a pattern first."));
            }
            if (!confirmFileAccess(w, QObject::tr("Claude wants to write the instructions"),
                                   QObject::tr("Allow Claude to write this file?\n\n%1").arg(path), path))
            {
                return McpToolResult::error(QStringLiteral("The user did not allow writing %1").arg(path));
            }
            return writeTextFile(path, w->allInstructionsText());
        }));

    registry.registerTool(tool(w,
        QStringLiteral("export_row_table"),
        QStringLiteral("Write the row table of a piece to a CSV file. The user is asked to allow writing the file."),
        object({{QStringLiteral("path"), str(QStringLiteral("Where to write, e.g. /home/me/rows.csv."))},
                {QStringLiteral("piece"), str(QStringLiteral("Piece name; default: the selected piece."))}},
               {QStringLiteral("path")}),
        false, [w](const QJsonObject &a) {
            QString piece;
            McpToolResult failure;
            if (!pieceArgument(w, a, piece, failure)) return failure;
            const QString path = absolute(text(a, "path"));
            if (text(a, "path").isEmpty())
            {
                return McpToolResult::error(QStringLiteral("Give a path."));
            }
            if (!confirmFileAccess(w, QObject::tr("Claude wants to write the row table"),
                                   QObject::tr("Allow Claude to write this file?\n\n%1").arg(path), path))
            {
                return McpToolResult::error(QStringLiteral("The user did not allow writing %1").arg(path));
            }
            return writeTextFile(path, w->rowTableFor(piece));
        }));
}
