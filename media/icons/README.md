# Icon sources

`AnyFSE.svg` is the vector master for the application logo, package artwork and ICO frames.
SVG files stay outside `Assets` so the package only contains runtime PNG resources.

From the repository root on Windows, regenerate the AnyFSE logo assets with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/Generate-Icons.ps1
```

The generator uses built-in WPF and rasterizes vector geometry directly at each requested
PNG or ICO frame size. Native vector edge antialiasing retains partially transparent edge
pixels. There is no intermediate raster size, supersampling, or raster resizing in the
generator. Supported source elements are paths,
solid fills, linear gradients, and round strokes; this is intentionally not a general-purpose SVG renderer.

- App-list logo: base 44 px plus scale 125, 150 and 200 (maximum supported display scale: 200%).
  The base file is the 100% fallback, without a duplicate scale-100 file.
- App-list target sizes: 16, 20, 24, 30, 32, 36, 40, 48, 64 and 256,
  with default, unplated and light-unplated forms. Qualifier order is consistent to avoid
  duplicate PRI candidates. Both unplated variants retain the existing white logo artwork.
- Both ICO files: 15 sizes from 16 to 256 px, with 32-bit PNG frames and transparency. The installer
  retains its white logo on a rounded purple gradient background.
- Required manifest images: one base `Square150x150Logo.png` and one `StoreLogo.png`.
  The manifest omits the optional wide tile and splash screen, so neither is generated.

This produces 36 logo PNGs. The 256px target-size images are required for large shell
previews, including the large Windows Search result card, even at display scales below
200%. They allow Windows to downsample a large source instead of enlarging a small icon.
They are independent of the scale qualifiers; no display scales above 200% are generated.
ICO frames retain their separate size set for EXE icons in
Explorer and other desktop surfaces, including large icon views.

The original ASUS button and Jump List PNG icons are maintained separately and are not
modified by this generator.

The package project already copies every file in `Assets` and indexes the qualifiers
with MakePri. Rebuild the application and installer to embed the new ICO files, and
rebuild/reinstall the identity package for Start menu artwork.
Windows may keep the previous icon in its shell cache until the app/package is updated.

Size reference: [Microsoft: Construct your Windows app's icon](https://learn.microsoft.com/en-us/windows/apps/design/iconography/app-icon-construction).
