# Bundled fonts

## Lato-*.woff2

The five Latin faces the theme uses (Regular / Italic / Bold / Bold Italic /
Black), `@font-face`'d by `themes/basic-office.css` and copied into every
rendered bundle beside `theme.css`. They replace an `@import` of
fonts.googleapis.com: a remote stylesheet is render-blocking on every deck load,
so on a network that is up but unreachable the browser stalls on it and keeps
painting the previous slide — which reads as a slow live reload. Each face lists
`local(...)` first, so a machine with Lato installed never loads these at all.

- Source: the `latin` subset Google Fonts serves for
  `https://fonts.googleapis.com/css2?family=Lato:ital,wght@0,400;0,700;0,900;1,400;1,700`
  (v25). ~70 KB total. Glyphs outside that subset fall through to the next
  family in `--font-base`, as they did before.
- License: SIL Open Font License 1.1 — see `Lato-OFL.txt`.

## NotoSansSC-Regular.woff2

A static Regular instance of **Noto Sans SC** (Simplified Chinese), used only by
the PDF deck build. Marp renders PDFs with a headless Chromium that cannot see
the host's system fonts, so Chinese text silently drops out unless a CJK font is
embedded via `@font-face`. `lecturekit/renderers/viewer/marp.py` copies this file
into the bundle and references it from a PDF-only theme. The live HTML viewer
(a real browser) is unaffected and keeps using system fonts.

- Source: <https://github.com/google/fonts/tree/main/ofl/notosanssc>
- Instanced to `wght=400` and compressed to woff2 with `fonttools`.
- License: SIL Open Font License 1.1 — see `NotoSansSC-OFL.txt`.
