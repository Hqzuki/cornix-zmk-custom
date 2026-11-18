# Cornix ZMK

純正Cornix左右分割キーボード向けのZMKファームウェアです。右がCentral、左がPeripheralです。キー配列は純正Cornixの49キー構成で、ZMK StudioとKeymap Editorから編集できます。

## 主な機能

- Windows／Mac配列の自動OS判定
- NUM、FN、PAN、GESTURE、MOUSE、BLUETOOTH、NAVレイヤー
- 左右エンコーダー、電池・充電表示、左右のLED状態表示
- ZMK Studioによるキー配列編集

## ファームウェア

GitHub Actionsの成功した[ビルド](https://github.com/Hqzuki/cornix-zmk-custom/actions)から`firmware`成果物を取得します。

- 左：`Cornix_left.uf2` ／ 設定消去：`Cornix_left_reset.uf2`
- 右：`Cornix_right.uf2` ／ 設定消去：`Cornix_right_reset.uf2`

書き込み、操作、配列の編集方法は[純正Cornixのガイド](docs/stock-cornix.md)を参照してください。自動OS判定の説明は[こちら](docs/auto-os.md)です。

## 開発

`build.yaml`は純正Cornix左右と各設定消去用の4種類をビルドします。キーマップは`config/Cornix.keymap`、Keymap Editorの定義は`config/Cornix.json`です。
