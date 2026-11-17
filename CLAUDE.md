# Project guidance

## Scope

This repository builds firmware for the stock Cornix split keyboard. The right half is central; the left half is peripheral. Do not add support for non-stock boards or hardware without an explicit request.

The upstream project is Hqzuki/cornix-zmk-custom. Never push, open PRs or issues against upstream repositories without explicit user instructions.

## Build

`build.yaml` generates four artifacts: `Cornix_left`, `Cornix_left_reset`, `Cornix_right`, and `Cornix_right_reset`. The right target includes the `studio-rpc-usb-uart` snippet.
`config/west.yml` and `.github/workflows/{build,smoke-build,release_with_tag}.yml` pin ZMK to the same commit SHA; bump them together and confirm CI.
Dongle shields (`cornix_dongle_*`, `zmk-dongle-display`) are kept for possible future use but are not built.

## Layout

- `boards/jzf/cornix/`: stock Cornix board and split definitions.
- `drivers/keymap/`: shared Cornix key behavior and automatic OS detection.
- `drivers/status/`: shared LED patterns, left WS2812 status, and split synchronization.
- `config/Cornix.keymap`: stock 49-key layout.
- `docs/stock-cornix.md`: installation, controls, and keymap editing.

## Validation

Run the existing checks in `.github/workflows/stock-checks.yml` for keymap behavior, automatic OS detection, and shared status behavior. Use GitHub Actions for full firmware builds. Report build validation separately from hardware validation.

## Flashing

Wait for the user's explicit authorization before flashing hardware. Update both halves with matching firmware. The reset images clear each half's ZMK settings.
