# CoreX ZMK

[English](README_en.md) | [中文](README_zh.md)

**CoreX右基板 R A14＋PAW3222トラックボールと、純正Cornix左基板**を使うためのZMKファームウェアです。
右をCentral、左をPeripheralとして動作させます。USB・Bluetooth・ZMK Studioの接続先は右です。

## 機能

- PAW3222によるカーソル移動
- CoronLP準拠の9レイヤー、Windows／Mac配列（文字入力用コンボ・単独矢印キーは削除）
- 自動マウスレイヤー：J／K／Lで左／中／右クリック、M／カンマで戻る／進む
- 親指キー長押しで縦横スクロール、専用親指キーでデスクトップ操作ジェスチャー
- 左右のキー、エンコーダー、電池測定、接続状態のRGB表示
- ZMK Studioでのキー編集

## ファームウェアの取得

pushするとGitHub Actionsでビルドします。[Actions](https://github.com/Hqzuki/cornix-zmk-custom/actions)の成功した実行から、Artifactsの`firmware`を取得してください。

- 左：`corex_stock_left.uf2`
- 右：`corex_r_a14_j3_s140v6.uf2`など。J3／J4と、搭載ブートローダーのS140 v6／v7に合わせて選びます。
- 設定消去：左は`corex_stock_left_reset.uf2`、右は対応する`corex_r_a14_reset_s140v6.uf2`または`v7`。

[導入・操作・設定・配線の詳細](docs/corex.md)を確認してください。実機動作は未確認です。

## 開発

`build.yaml`はCoreX構成に必要な成果物だけを生成します。`boards/jzf/cornix/`には純正左とCoreXから参照する共通定義を保持しています。
CoreXの基板定義は`boards/jzf/corex/`、センサードライバーは`drivers/corex/`、右のキーマップは`config/corex_right.keymap`です。

CoreX Studio／Vialの設定通信、RMKのジェスチャー設定、トラックポイント、トラックパッドには対応していません。
CornixメーカーおよびCoreX作者による公式ZMKファームウェアではありません。
