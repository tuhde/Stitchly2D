# Pattern file format (`.stpat`)

Special stitch patterns and color patterns are exchanged as `.stpat` files. A file is UTF-8 JSON and holds one or
several patterns. The same encoding of a single pattern is used inside the catalog file.

```json
{
  "format": "stitchly-patterns",
  "version": 1,
  "author": "Your name",
  "description": "Optional text",
  "patterns": [
    {
      "name": "Seed stitch",
      "kind": "special",
      "width": 2,
      "height": 2,
      "legend": ["k", "p"],
      "rows": [[1, 0],
               [0, 1]]
    },
    {
      "name": "Stripes 2 rows",
      "kind": "color",
      "width": 1,
      "height": 4,
      "legend": ["A", "B"],
      "rows": [[1], [1], [0], [0]]
    }
  ]
}
```

| Field | Meaning |
|---|---|
| `format` | Always `"stitchly-patterns"`. |
| `version` | Format version, currently `1`. A file with a newer version is refused, not half-read. |
| `author`, `description` | Optional, file level. |
| `name` | Unique inside the file. The catalog decides what happens on a clash when importing. |
| `kind` | `"special"` (stitch symbols) or `"color"` (yarn colors). |
| `width`, `height` | Size of one repeat in stitches and rows. |
| `legend` | Special: stitch symbols such as `k`, `p`, `C4F`. Color: yarn names or colors such as `MC`, `CC1`, `#c03030`. |
| `rows` | `height` arrays of `width` numbers each. A number is an index into `legend`; `-1` leaves the base stitch alone. |

**Row order:** `rows` is written the way a chart is read, so the **first row in the file is the top row** of the
repeat and the last one is the bottom row (the first one knitted).

**Import** checks the whole file before anything is added: wrong format id, newer version, empty file, ragged rows,
a number outside the legend, or two patterns with the same name make the import fail with a message, and nothing is
changed. When adding to a catalog, a name that already exists can be skipped, replaced, or kept under a new name
(`Seed stitch (2)`).

**Export** refuses empty selections, invalid patterns and duplicate names, and writes the file atomically.
