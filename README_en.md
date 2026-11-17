# Cornix ZMK

[日本語](README.md) | [中文](README_zh.md)

ZMK firmware for the stock Cornix split keyboard. The right half is central and the left half is peripheral. It uses the stock 49-key layout, editable through ZMK Studio or Keymap Editor.

## Features

- Automatic Windows/Mac layout detection
- NUM, FN, PAN, GESTURE, MOUSE, BLUETOOTH and NAV layers
- Left and right encoders, battery/charging indicators and split LED status
- Keymap editing through ZMK Studio

## Firmware

Download the `firmware` artifact from a successful [GitHub Actions build](https://github.com/Hqzuki/cornix-zmk-custom/actions).

- Left: `Cornix_left.uf2` | settings reset: `Cornix_left_reset.uf2`
- Right: `Cornix_right.uf2` | settings reset: `Cornix_right_reset.uf2`

See the [stock Cornix guide](docs/stock-cornix.md) for flashing, controls and keymap editing, and the [automatic OS detection guide](docs/auto-os.md) for its behavior.

## Development

`build.yaml` builds the stock left/right firmware and their settings-reset images. The keymap is `config/Cornix.keymap`; Keymap Editor metadata is in `config/Cornix.json`.
