# CoreX R A14 + PAW3222 / ZMK

CoreXの右交換基板 **R A14** とPAW3222トラックボールを、純正Cornix左＋ZMKで使う構成です。
右がCentralで、USB・Bluetooth・ZMK Studioの接続先になります。

## ビルド成果物

GitHub Actionsの`firmware`アーティファクトに次のCoreX構成用UF2を生成します。

| 用途 | artifact-name |
| --- | --- |
| 純正Cornix左 | `corex_stock_left` |
| 純正Cornix左のZMK設定消去 | `corex_stock_left_reset` |
| CoreX右・J3・S140 v6 | `corex_r_a14_j3_s140v6` |
| CoreX右・J4・S140 v6 | `corex_r_a14_j4_s140v6` |
| CoreX右・J3・S140 v7 | `corex_r_a14_j3_s140v7` |
| CoreX右・J4・S140 v7 | `corex_r_a14_j4_s140v7` |
| CoreX右のZMK設定消去・v6 / v7 | `corex_r_a14_reset_s140v6` / `corex_r_a14_reset_s140v7` |

**右には`corex_r_a14_*`、純正左には`corex_stock_left*`を使用します。左右のUF2は別物です。**
CoreX右用は既存のSoftDeviceを保持し、v6ではアプリを`0x26000`、v7では`0x27000`から配置します。
`nrf52840-nosd`スニペットは使用しません。UF2はブートローダー領域へ書き込みません。
設定領域はCoreX RMKと同じ`0xb0000–0xcffff`を予約しますが、設定の内容は互換ではありません。

RESETを2回押したときのドライブ内`INFO_UF2.TXT`にあるSoftDevice版に合わせてv6/v7を選びます。
記載が判別できない場合は書き込み前に確認してください。J3/J4は基板のシルク印刷で確認します。
接続位置が分からない場合も両方の成果物は生成されますが、使用するのは接続先に合う1つです。

RMKとZMKは左右通信に互換性がないため、左右とも同じZMKビルドのファームへ切り替える必要があります。
初回はPCの旧Bluetooth登録を削除し、右には対応するCoreX用設定消去UF2、左には`corex_stock_left_reset.uf2`を書き込んだ後、
各側の本来のUF2を書き込み、左右を再起動・ペアリングします。右の設定消去UF2だけでは左側の登録は消えません。
本実装による実機確認は未実施です。基板の型番・ブートローダーを確認してから書き込みます。

## 操作

既存Cornixの4レイヤーを継承し、CoreX右のキーマップにMouse（4）とScroll（5）を追加しています。

- ボールでカーソルを移動。両軸反転、速度4/5、端数を保持します。
- 動かすとMouseレイヤーを700ms有効化。J＝左クリック、K＝中クリック、L＝右クリック。
- Iの短押しでI、長押し＋ボールで縦横スクロール。スクロール量は入力12カウントで1段階です。
- 右エンコーダー押し込みは中クリック。回転は既存のページ上下です。
- Mouseレイヤー中の通常キーで自動レイヤーを解除します。I・J・K・L・右エンコーダー押し込みは解除対象外です。
- RGBは外側がBluetoothプロファイル0/1/2＝緑/赤/青、内側が左右接続の青、右の電池低下は赤点滅です。
  接続案内は一定時間で消灯します。純正Cornix左は既存の`cornix_indicator`を使用します。

ZMK Studioの保存済み配列はビルド内の初期配列より優先します。
導入後にIの長押しやクリックが出ない場合は、StudioでMouse/Scrollレイヤーと割り当てを確認してください。
速度・反転・スクロール比率・自動レイヤーの時間は`boards/shields/corex_common.dtsi`、
CPIは`corex_j3.overlay`または`corex_j4.overlay`の`&corex_paw { res-cpi = <1216>; };`で変更できます。
CPIは608〜4826の38刻みです。

## 対応範囲

実装対象は単一PAW3222、キー行列、右エンコーダー、電池測定、RGB接続表示、USB/BLEポインティング、
自動マウスレイヤーとスクロール、ZMK Studioでのキー編集です。
CoreX Studio/Vialの設定プロトコル、RMKのジェスチャー設定、2台のセンサー同時使用、
トラックポイント、トラックパッド、CoreX左・旧プロトタイプ基板は対象外です。

## 配線と実装の根拠

- [CoreX-RMK R A14の行列・エンコーダー・電池](https://github.com/yuchamichami/CoreX-RMK/blob/main/source/corex-rmk-pair/right/keyboard-raytac.toml)
- [J3/J4とRGB LED配線](https://github.com/yuchamichami/CoreX-RMK/blob/main/source/corex-rmk-pair/right/src/main.rs)
- [半二重SDIOプロトコル](https://github.com/yuchamichami/CoreX-RMK/blob/main/source/corex-rmk-pair/right/src/paw_wire.rs)
- [初期化・スリープ動作](https://github.com/yuchamichami/CoreX-RMK/blob/main/source/corex-rmk-pair/right/src/paw3222.rs)
- [SoftDevice別の配置](https://github.com/yuchamichami/CoreX-RMK/blob/main/source/corex-rmk-pair/right/build.rs)

J4: SCK=P0.05 / SDIO=P1.08 / MOTION=P0.07 / AUX=P0.26。
J3: SCK=P0.08 / SDIO=P0.04 / MOTION=P0.06 / AUX=P1.09。
AUXは参照実装に合わせてLowを保持します。SDIOはセンサー応答中に入力へ切り替えます。
動作中のみ15ms間隔で読み取り、停止中はMOTION割り込みで復帰します。
未接続・不正な製品IDではマウス入力を出さず、2秒後に再検出します。

## ホスト上の通信テスト

```sh
cc -std=c11 -Wall -Wextra -Werror tests/corex/wire_test.c -o /tmp/corex-wire-test
/tmp/corex-wire-test
```

ドライバーと同じ通信コードで全256値の送受信、アドレスのRead/Writeビット、
受信時のSDIO解放、クロックの待機レベルをGPIOモックで検証します。
このテストは実機の信号品質・ピン配線・無線動作を保証するものではありません。
