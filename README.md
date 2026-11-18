# Cornix TB ZMK

XIAO BLEをCentral、左右のCornixをPeripheralにするトラックボール構成のZMKファームウェアです。PAW3222トラックボールをXIAOに接続し、ZMK StudioとKeymap Editorからキー配列を編集できます。

## 主な機能

- Windows／Mac配列の自動OS判定
- NUMFN、PAN、GESTURE、MOUSE、BLUETOOTHレイヤー
- PAW3222のカーソル・スクロール・デスクトップ操作
- XIAO内蔵RGB LEDの電池表示、左右のLED状態表示
- ZMK Studioによるキー配列編集

## ファームウェア

GitHub Actionsの成功した[ビルド](https://github.com/Hqzuki/cornix-zmk-custom/actions)から`firmware`成果物を取得します。

- XIAO Central：TB Central用UF2
- Cornix Peripheral：TB Peripheral用UF2

配線と操作は[トラックボール構成のガイド](docs/trackball-central.md)、自動OS判定は[こちら](docs/auto-os.md)を参照してください。

## 開発

キーマップは`config/Cornix_tb.keymap`、Keymap Editorの定義は`config/Cornix_tb.json`です。PAW3222ドライバーは`drivers/trackball/`、TB CentralとPeripheralの設定は`boards/shields/cornix_tb_*`にあります。
