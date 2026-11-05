# ZMK Firmware for Cornix (Japan custom build)

**Languages:** English | [日本語](./README_ja.md) | [中文](./README_zh.md)

This repository (`numachang/cornix-zmk-custom`) is a **Japan-focused downstream fork** of
[`hitsmaxft/zmk-keyboard-cornix`](https://github.com/hitsmaxft/zmk-keyboard-cornix). It contains the ZMK
firmware configuration plus the board/shield module for the Cornix split keyboard.

It intentionally diverges from upstream in a few narrow ways, because in Japan the Cornix is sold and used as a
**plain Bluetooth split keyboard** (no dongle):

- **Dongle build targets are disabled.** The keyboard runs as a standard split with the right half as the
  central. The dongle shields still exist on disk but are not part of the active build matrix.
- **ZMK Studio is enabled on `cornix_right`**, so you can edit the keymap live over USB with
  [ZMK Studio](https://zmk.dev/docs/features/studio).
- **RGB indicator (`cornix_indicator`)** is built into the active targets.

> ### Looking for the RMK firmware instead?
> The stock Cornix firmware is **RMK**. A matching RMK custom build is maintained here:
> **https://github.com/numachang/cornix-rmk-custom**
>
> Use that repository if you want to stay on RMK; use *this* repository if you want to run ZMK.

> ### Editing keymaps: ZMK Studio Tweaks (custom web editor)
> A tweaked build of ZMK Studio is hosted at **https://zmk-studio-tweaks.numachang.com/**
> (source: **https://github.com/numachang/zmk-studio-tweaks**). It runs entirely in a Chromium browser over the
> Web Serial API — no data leaves your machine — so you can open `cornix_right` and edit the keymap there directly.
>
> It is a fork of the official ZMK Studio that adds, on top of the upstream editor:
> - **Keymap import/export** as JSON files, with firmware error feedback.
> - **Visual key picker** — a grid HID usage picker with search, categorized behaviors, and physical keyboard
>   layout rendering (ANSI/ISO/**JIS**) you can click on.
> - **Host-layout localization** for 22 OS layouts including **Japanese**, so the picker and labels match your
>   keyboard layout without changing the underlying HID bindings.
>
> This is a place to prototype features without blocking on upstream review cycles, with the intent of sending
> them back upstream as PRs where they land cleanly. It is **not affiliated with or endorsed by** the ZMK project —
> for stable, supported use, prefer the [official ZMK Studio](https://zmk.studio/).

![image](images/cornix_with_dongle.png)
![image](images/cornix_layout.png)

## Boards and shields

### Boards

- **`cornix_left`**: Left half of the Cornix split. Acts as the peripheral (slave) side.
- **`cornix_right`**: Right half of the Cornix split. Acts as the central (master) side. ZMK Studio is enabled here.
- **`cornix_ph_left`**: Alternative left-half board intended for a dongle setup. **Unused in this fork** (kept for
  parity with upstream).

### Shields

- **`cornix_indicator`**: Enables the RGB LED indicators for battery and connection status. Consumes more power.
- **`cornix_dongle_adapter`** / **`cornix_dongle_eyelash`**: Dongle-related shields from upstream. They exist on
  disk but are **not referenced by the active build matrix** in this fork.

This community firmware has been tested with Cornix using ZMK and provides full split-role configuration, battery
power management, and Bluetooth central/peripheral setup per ZMK split guidelines.

## Supported hardware: Cornix split keyboard

Cornix Split Tented Low-Profile Ergo Keyboard (Jezail Funder)

Cornix is a Corne-inspired split ergonomic keyboard featuring a compact 3×6 column-staggered layout with six
thumb-cluster keys (three per half). It offers adjustable tenting angles at 10°, 18°, and 25°, allowing users to
reduce wrist strain and find a custom ergonomic alignment.

- **Split, column-staggered layout** (3×6 + thumb cluster).
- **Adjustable tenting support** at 10°, 18°, 25° (hardware-based, no firmware hacks).
- **Kailh Choc V2 hot-swap sockets** and support for LAK or LCK low-profile keycaps.
- **Dual-mode connectivity**: wired USB-C or Bluetooth wireless (right half as central).
- **Firmware**: stock firmware is RMK (VIAL-supported); this repository provides a ZMK alternative.
- Premium **CNC-machined aluminum chassis**, custom damping foam, and portable storage pouch.

> The upstream project owner is an RMK contributor too — please support RMK https://rmk.rs/

## Bootloader / recovery notes

Since **v2.3** of the board the flash partition layout was updated and the SoftDevice partition was shrunk (150K →
4K), so you can flash `.uf2` files directly — **no SoftDevice restore is required**.

- If you come from a very old firmware, you may first need to reset with a `reset.uf2` build.
- You can roll back to stock RMK firmware by flashing the original `.uf2` files; backups live under `rmkfw/`.
- Deeper bootloader recovery instructions (for the legacy SoftDevice-restore path) are in
  [bootloader/README.md](./bootloader/README.md).

## 🔰 Easy method: clone this repo and build with GitHub Actions

If you're new to ZMK and don't want to deal with `west.yml` or module management, you can use this repository
directly to customize your firmware.

### Steps

1. **Fork or clone this repository**
   - Click **Fork** in the top right to copy this repo to your GitHub account, or run `git clone` locally.

   > Forking is recommended, because GitHub Actions will automatically build your firmware.

2. **Edit your keymap**
   - The keymap lives in `config/cornix.keymap`. Edit it directly, or use ZMK Studio (enabled on `cornix_right`) —
     either the [official editor](https://zmk.studio/) or the tweaked build at
     [zmk-studio-tweaks.numachang.com](https://zmk-studio-tweaks.numachang.com/) (see above) — or the
     [ZMK Keymap Editor](https://nickcoutsos.github.io/keymap-editor/).
   - Commit and push your changes to GitHub.

3. **Build with GitHub Actions**
   - Pushing changes under `boards/*` or `config/*` triggers the build automatically. You can also start a build
     manually from **Actions → "Build ZMK firmware" → Run workflow** (use this after editing `build.yaml`, which
     does not trigger a build on its own).
   - When the workflow finishes, go to **Actions → your latest run → Artifacts** and download the firmware
     (`firmware.zip`, containing one `.uf2` per target).

4. **Flash your keyboard**
   - Put each half into UF2 bootloader mode (usually by double-tapping the reset button).
   - Drag and drop the matching `.uf2` onto the mounted drive.
   - Use a **data-capable USB-C cable**, and flash **both halves from the same build** to avoid a split-protocol
     mismatch.

### Who is this for?

- Beginners to ZMK
- Users who only want to customize keymaps
- Anyone who doesn't need to modify drivers or hardware definitions

## Build targets (`build.yaml`)

This fork ships the following active targets (dongle targets are commented out):

```yaml
include:
  # Left half (peripheral) — RGB indicator
  - board: cornix_left
    shield: cornix_indicator
    artifact-name: cornix_left_default_nosd

  # Right half (central) — RGB indicator + ZMK Studio over USB
  - board: cornix_right
    shield: cornix_indicator
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_right_nosd

  # Settings reset (flash to both halves when re-pairing)
  - board: cornix_right
    shield: settings_reset
    snippet: studio-rpc-usb-uart nrf52840-nosd
    artifact-name: cornix_reset
```

> [!NOTE]
> - The `cornix_indicator` shield enables RGB LEDs but consumes more power. Remove it from a target if you don't
>   want the indicators.
> - To re-pair the halves: flash `cornix_reset` to both sides, then flash `cornix_left` and `cornix_right` and
>   reset both at the same time.

## How to add the Cornix module to an existing ZMK project

For users with an existing zmk-config, add this repository as a dependency via `west.yml` and pull it with
`west update`.

### 1. Modify `west.yml`

Add to the `manifest/remotes` section:

```yaml
remotes:
  - name: zmkfirmware
    url-base: https://github.com/zmkfirmware
  - name: numachang
    url-base: https://github.com/numachang
  - name: urob
    url-base: https://github.com/urob
```

Add to the `manifest/projects` section:

```yaml
projects:
  - name: zmk
    remote: zmkfirmware
    revision: main
    import: app/west.yml
  - name: cornix-zmk-custom
    remote: numachang
    revision: main
  - name: zmk-helpers
    remote: urob
    revision: main
```

> Want upstream instead of the Japan fork? Use `url-base: https://github.com/hitsmaxft` with project name
> `zmk-keyboard-cornix`.

### 2. Update dependencies

```bash
west update
```

### 3. Configure the build

Add the targets shown in [Build targets](#build-targets-buildyaml) to your `build.yaml`.

### 4. Build and flash

- No SoftDevice restore is needed (since board v2.3).
- Flash `cornix_reset` to both sides if you are re-pairing.
- Flash the left and right `.uf2` files and reset both sides at the same time.

## Build this project locally (without a `west.yml` dependency)

If you prefer to build locally without adding this as a `west.yml` dependency, you can use the
`ZMK_EXTRA_MODULES` cmake argument.

### Prerequisites

1. A working ZMK development environment.
2. This repository cloned to a local directory.

### Build steps

1. **Clone this repository**:
   ```bash
   git clone https://github.com/numachang/cornix-zmk-custom.git
   ```

2. **Configure your ZMK build with the extra module** — edit your `.west/config` and add a cmake argument under
   `[build]`:

   ```ini
   [build]
   cmake-args = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DZMK_EXTRA_MODULES=/full/absolute/path/to/cornix-zmk-custom
   ```

   Replace the path with the actual absolute path where you cloned this repository.

3. **Build the firmware**:
   ```bash
   west build -b cornix_left
   west build -b cornix_right
   ```

This method lets you use the Cornix module without modifying your existing ZMK configuration's `west.yml`.

### Local build with Nix (Linux/macOS)

A Nix-based flow is provided via the `Justfile`:

```bash
nix develop          # provides zephyr-sdk, west, just, yq, keymap-drawer
just init            # west init -l config && west update && west zephyr-export
just list            # show parsed build targets from build.yaml
just build cornix_left
just clean
just draw cornix     # render keymap SVG via keymap-drawer
```

Windows users should rely on GitHub Actions instead.

## About RGB

The Cornix shield has 2 RGB LEDs on each side, controlled by PWM in the stock firmware. In ZMK these are driven by
the `cornix_indicator` shield to indicate battery and connection status. A fully native out-of-tree RGB module is
planned (see issue #2).

### CoreX R A14 + PAW3222

See [CoreX build and usage notes](docs/corex.md) for the dedicated board, connector and bootloader variants.
