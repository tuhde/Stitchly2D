# Yarn file format (`yarns.json`)

The yarn inventory is one UTF-8 JSON file. The editor, the guider and `yarnbox` all read and write the same file,
`yarns.json` in the program's config folder (on Linux `~/.config/Stitchly2D/Stitchly2D/`, next to `catalog.json`).
`yarnbox` also opens any other file of this format given on the command line.

```json
{
  "format": "stitchly-yarns",
  "version": 1,
  "yarns": [
    {
      "id": "6f1c0a52-3d0e-4b7e-9d52-2c3f6a0c8e11",
      "brand": "Woolly",
      "name": "Merino DK",
      "fiber": "merino 100%",
      "weightClass": "DK",
      "colorName": "red",
      "colorHex": "#b22222",
      "imagePath": "",
      "dyeLot": "A123",
      "notes": "Bought at the market",
      "metersPerSkein": 225,
      "gramsPerSkein": 100,
      "skeinsInStock": 3,
      "partialMeters": 80,
      "gauges": [
        {"stitchType": "Stockinette", "stitchWidthMm": 3.5, "rowHeightMm": 2.5, "yarnPerStitchMm": 0}
      ]
    }
  ]
}
```

## Fields

| Field | Meaning |
|---|---|
| `id` | Unique, never shown. Projects refer to a yarn by it. Required. |
| `brand`, `name` | At least one of them is required. Lists show "Brand Name (color)". |
| `fiber`, `weightClass`, `colorName`, `dyeLot`, `notes` | Free text. |
| `colorHex` | `#rrggbb` or empty (unknown). Used for the swatch. |
| `imagePath` | Path of a photo of the yarn; may be empty. |
| `metersPerSkein`, `gramsPerSkein` | Not negative; 0 = unknown. |
| `skeinsInStock` | Whole skeins, not negative. |
| `partialMeters` | Meters left on an opened skein, on top of `skeinsInStock`. |
| `gauges` | What the yarn knits to with a basic stitch. One entry per stitch type, no duplicates. |

The stock is `skeinsInStock × metersPerSkein + partialMeters`. When the guider takes used yarn from the stock it
uses the partial skein first, then opens whole skeins.

### Gauges

`stitchType` is the name of a basic stitch of the catalog. `stitchWidthMm` and `rowHeightMm` (both above 0) are
the size of one stitch and one row that a swatch of this yarn gave; "Use yarn gauge" in the editor copies them into the catalog.
`yarnPerStitchMm` is the measured yarn length of one stitch. With 0 the estimate takes 4 stitch widths per stitch;
in both cases 10% is added as margin.

## Rules for readers

- `format` must be `stitchly-yarns`.
- A file with a `version` newer than the program understands is refused, not read partly. A missing or zero version is refused too.
- The whole file is checked: if one yarn is not valid (no id, no name, bad color, negative number, double id or double gauge) the file is refused with the number of the yarn.
- A missing file is an empty inventory. A file that cannot be read is never overwritten without asking.
- Files are written through a temporary file, so a crash cannot leave half a file.

## Yarns in a project (`.stitchly`)

A project file may have an optional `yarns` key (the project file version stays 1; older files have none):

```json
"yarns": {
  "main": {"id": "6f1c0a52-...", "name": "Woolly Merino DK (red)", "colorHex": "#b22222"},
  "A":    {"id": "...", "name": "...", "colorHex": "#ffffff"}
}
```

The keys are yarn slots: `main` for the piece itself and the color names of color patterns (`A`, `B`, ...).
`name` and `colorHex` are a copy from when the yarn was chosen, so the guider can show the yarn even if the
inventory file is missing. The guider asks for the yarn only for slots a project does not name.
