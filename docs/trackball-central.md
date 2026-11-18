# トラックボール用ソース

XIAO BLE nRF52840をCentral、左右のCornixをPeripheralにする試作用ソースを残しています。通常ビルドには含めず、`build.yaml`から生成するのは純正Cornix用UF2 4種類だけです。通常の49キー配列は`config/Cornix.keymap`です。

## 配線案

中央シールドの設定は`boards/shields/cornix_tb_central/`、右Peripheralの設定は`boards/shields/cornix_tb_peripheral/`、PAW3222ドライバーは`drivers/trackball/`にあります。

| PAW3222信号 | XIAOピン |
| --- | --- |
| 自動判別対象（SCK／SDIO／MOTION／CS） | D7・D8・D9・D10 |

起動時に4本の信号ピンの24通りの役割割り当てを毎回調べ、標準配線（D10=SCK、D9=SDIO、D8=CS、D7=MOTION）から試します。検出結果はRAMだけに保持し、フラッシュには保存しません。3.3VとGNDは自動判別できないため、センサー基板の配列と電源電圧を確認して接続してください。

センサー解像度は1596 CPIです（PAW3222の38 CPI刻みに合わせた1600 CPIに最も近い値）。

探索時のGPIO出力はオープンドレインにして競合を抑え、センサーを識別した後は通常出力に切り替えます。IDレジスタを2回ずつ読み、検出IDもログに出します。実機での電気的な安全性・信号タイミングは未検証です。パーツ接続前にこのファームウェアを使わないでください。

## 操作案

- レイヤー3（PAN）でボールを動かすとスクロールします。
- レイヤー4（GESTURE）でボールを動かすとデスクトップ操作ジェスチャーを送ります。ESDFによるカーソル移動は使いません。
- マウスレイヤー（AML）はLANG1またはLANG2を押すと解除します。
- BLUETOOTHレイヤー（6）では、XIAO BLE内蔵RGB LEDにCentral側の電池状態を表示します。緑＝51〜100%、黄＝21〜50%、赤＝0〜20%。

## 確認範囲

`tests/cornix/trackball_wire_test.c`はGPIOモックでPAW3222の通信を確認します。自動ピン判別、実機の配線・センサーの向き、無線接続、スリープ復帰は未検証です。このソースからUF2は生成されません。
