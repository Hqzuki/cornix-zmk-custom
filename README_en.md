# CoreX ZMK

[日本語](README.md) | [中文](README_zh.md)

ZMK firmware for a **CoreX R A14 right PCB with a PAW3222 trackball**, paired with a **stock Cornix left PCB**.
The right half is the central and provides USB, Bluetooth and ZMK Studio connectivity.

Features include cursor movement, a CoronLP-style nine-layer keymap (without standalone arrows), an automatic mouse layer (J/K/L clicks), thumb-key scrolling and desktop gestures (no combos), encoders, battery reporting and RGB connection indicators.

Push to any branch to build automatically. Download `firmware` from the successful [GitHub Actions run](https://github.com/Hqzuki/cornix-zmk-custom/actions).

- Left: `corex_stock_left.uf2`
- Right: `corex_r_a14_s140v6.uf2` or `corex_r_a14_s140v7.uf2`, matching the installed S140 SoftDevice (the J3/J4 sensor connector is detected automatically).
- Settings reset: `corex_stock_left_reset.uf2` for the left, or the matching `corex_r_a14_reset_s140v6.uf2` / `v7` for the right.

See [installation, controls, configuration and wiring](docs/corex.md). Hardware operation has not yet been verified.

`build.yaml` contains only the CoreX pair and its reset images. Shared Cornix definitions remain as dependencies.
CoreX Studio/Vial configuration, RMK gesture configuration, TrackPoint and trackpad support are not implemented.
This is an unofficial firmware project, derived from the Cornix ZMK configuration and referencing CoreX's published hardware definitions.
