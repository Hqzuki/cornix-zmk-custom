# トラックボール用ソース

XIAO BLE nRF52840をCentral、左右のCornixをPeripheralにする試作用ソースを残しています。通常ビルドには含めず、`build.yaml`から生成するのは純正Cornix用UF2 4種類だけです。通常の49キー配列は`config/Cornix.keymap`です。

## 配線案

中央シールドの設定は`boards/shields/cornix_tb_central/`、右Peripheralの設定は`boards/shields/cornix_tb_peripheral/`、PAW3222ドライバーは`drivers/trackball/`にあります。

| PAW3222信号 | XIAOピン |
| --- | --- |
| SCK | D10 |
| SDIO | D9 |
| MOTION | D7 |
| CS／AUX | D8 |

センサー基板のコネクター配列と電源電圧は基板ごとに確認してください。

## 操作案

- レイヤー4（PAN）でボールを動かすとスクロールします。
- レイヤー5（GESTURE）でボールを動かすとデスクトップ操作ジェスチャーを送ります。ESDFによるカーソル移動は使いません。
- マウスレイヤー（AML）はLANG1またはLANG2を押すと解除します。

## 確認範囲

`tests/cornix/trackball_wire_test.c`はGPIOモックでPAW3222の通信を確認します。実機の配線、センサーの向き、無線接続、スリープ復帰は確認していません。このソースからUF2は生成されません。
