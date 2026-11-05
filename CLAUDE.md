# Project guidance

## Scope

This repository targets CoreX R A14 right + PAW3222, paired with stock Cornix left, running ZMK.
Right is central; USB/Bluetooth/ZMK Studio attach to the right.
The origin is Hqzuki/cornix-zmk-custom. Never push, open PRs or issues against upstream repositories without explicit user instructions.

## Build

Push to any branch triggers firmware builds. `build.yaml` generates only the CoreX pair and reset images.
Left artifacts: `corex_stock_left`, `corex_stock_left_reset`.
Right artifacts: one per S140 v6/v7 (J3/J4 sensor connector is auto-detected), plus reset for each SoftDevice revision.
`config/west.yml` and `.github/workflows/{build,smoke-build,release_with_tag}.yml` pin ZMK to the same commit SHA; bump them together and confirm CI.
Do not restore stock Cornix right or dongle targets. Keep shared Cornix source definitions required by the CoreX board and stock left.
Never add `nrf52840-nosd` to CoreX right targets: board revisions 6.0.0/7.0.0 select offsets 0x26000/0x27000.

## Layout

- `boards/jzf/corex/`: R A14 board and SoftDevice revisions.
- `boards/jzf/cornix/`: shared hardware/layout definitions and stock left.
- `drivers/corex/`: PAW3222 GPIO half-duplex transport, battery enable.
- `drivers/keymap/`: CoronLP default base-layer persistence.
- `drivers/status/`: shared LED patterns, right PWM / left WS2812 and split profile synchronization.
- `boards/shields/corex_trackball`: enables the PAW3222 (J4/J3 probed at runtime) and pointing listener.
- `config/corex_right.keymap`: nine CoreX layers; no standalone arrows.
- `docs/corex.md`: installation, controls, known limitations and hardware sources.

## Validation

Compile `tests/corex/wire_test.c` with `cc -std=c11 -Wall -Wextra -Werror`, then run it.
Use GitHub Actions for full firmware builds. Report build validation separately from hardware validation.

## Flashing

Wait for the user's explicit authorization before flashing hardware.
Select UF2s by board, connector and installed SoftDevice. Full updates require both halves from the same build.
CoreX right and stock Cornix left reset images are different. RMK and ZMK split protocols are incompatible.
