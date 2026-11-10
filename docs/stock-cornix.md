# 純正Cornixだけで使う

CoreXを使わず、純正Cornixの左・右で使う構成です。右がCentral、左がPeripheralです。USB、Bluetooth、ZMK Studioは右に接続します。メーカー公式のファームウェアではありません。

## 書き込むファイル

成功したGitHub Actionsの`firmware`成果物から取得します。

| 対象 | 通常 | 設定消去 |
| --- | --- | --- |
| 純正Cornix左 | `corex_stock_left.uf2` | `corex_stock_left_reset.uf2` |
| 純正Cornix右 | `cornix_stock_right.uf2` | `cornix_stock_right_reset.uf2` |

左のファイル名にCoreXが入っていますが、純正Cornix左のためのものです。CoreX構成と共用しています。純正右には`corex_r_a14_*`を書き込まず、`cornix_stock_right`を使ってください。

初めてこのZMK構成へ移行する場合は、PC側の旧Bluetooth登録を削除し、左右それぞれの設定消去UF2、通常UF2の順に書き込みます。リセットを2回押してUF2ドライブを開きます。既にこの構成でペアリング済みなら、毎回の設定消去は不要です。以前のファームウェアへ戻したい場合は、先にその復元手順・ファイルを手元に用意してください。

## キー配列と操作

`config/cornix_right.keymap`と同じディレクトリの`cornix_right.json`が純正50キー用です。Keymap Editorでは`cornix_right`を選びます。CoreX版とは独立して編集できます。

- WIN／MAC、NUM、FN、PAN、GESTURE、MOUSE、BLUETOOTHの8レイヤーで、基本配列は追加時点のCoreX版を基にしています。
- 左LANG2、右LANG1、Spaceは通常の空白です。NUM／FN、PANとGESTUREのESDF矢印、GESTUREのIJKLウィンドウ操作を使えます。
- 純正右にある追加の1キー（位置46）は基本配列では未割り当てです。NUM／FNなどでは透過です。自由に編集できます。
- 左右のエンコーダーと押し込みを使えます。スクリーンショットとズーム操作はCoreX版のキー設定を引き継いでいます。
- Bluetooth接続先ごとのWIN／MAC保存・復元、ZMK Studio、左右のWS2812表示に対応します。
- トラックボール用のセンサー・自動マウスレイヤー・ボールジェスチャーはありません。PANは矢印用、GESTUREは矢印とウィンドウキー用です。MOUSEレイヤー番号は互換性のため残していますが、自動では入りません。

CoreX用の49キーと純正50キーはStudioの保存配列を共用できません。旧配列が保存されている場合は、Studioでキー配列をリセットして初期配列を読み直します。

実機のキー位置、回転方向、LED、無線接続は未確認です。左右のキーとエンコーダー、USB／Bluetooth入力、電池表示を実機で確認してください。
