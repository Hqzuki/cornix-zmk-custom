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

[CoronLPのキーマップ](https://github.com/Hqzuki/zmk-config-coron/blob/LowprofileTrackBallcase/config/coron.keymap)をCoreXの49キーへ移植しています。
単独の上下左右キーは削除しています。トラックボール位置のキー枠（旧Delete）も削除しています。LED・センサー配線・CPI・通常カーソル速度はCoreXの設定です。

| 番号 | レイヤー | 操作 |
| --- | --- | --- |
| 0 / 1 | WIN / MAC | Windows / Mac用の基本配列 |
| 2 | NUM | 右手LANG2長押し。右端テンキーと記号・括弧マクロ |
| 3 / 4 | PAN_WIN / PAN_MAC | J＋Kを押している間、ボールでスクロール |
| 5 | GESTURE | K＋Lを押している間、ボールでデスクトップ操作 |
| 6 | MOUSE | ボール操作で300ms有効。J／K／L＝左／中／右クリック、M／カンマ＝戻る／進む |
| 7 | BLUETOOTH | 左の内側スイッチを押している間、接続先選択・登録消去・ブートローダー操作 |
| 8 | FN | 右手左下を押している間。右端4行×3列で上からF1〜F3、F4〜F6、F7〜F9、F10〜F12 |

- Iは通常の文字キーです。スクロールはJ＋Kで起動します。
- 親指キーはWIN／MAC共通で、Space、短押しLANG1／長押しスクロール、LANG1、短押しLANG2／長押しジェスチャー、LANG2です。
  OSごとのCtrl／GUI配置は維持しています。
- NUMレイヤーの左最上段は左から`\`、`()`、`[]`、`{}`、`_`、`~`です。
  FNレイヤーは右端4行×3列に、上からF1〜F3／F4〜F6／F7〜F9／F10〜F12を配置しています。右端は上から`7 8 9`／`4 5 6`／`1 2 3`／`0 無効 元の割当`のテンキー配置です。
  テンキー部分も通常の数字キーコードを使い、Enter・小数点は追加していません。
  括弧マクロは開閉を入力後、左矢印を1回送ってカーソルを間へ戻します。
  US配列のキーコードを使い、US／JIS切り替え機能は追加していません。
- 基本配列のセミコロン／引用符、イコール／マイナスの押し分けもCoronLPを継承します。
- 同時押しはJ＋KのスクロールとK＋Lのジェスチャー起動だけです。文字入力用の5つのコンボは削除しています。
- 右の内側スイッチ（エンコーダー押し込み）はズーム率のリセット（WIN＝Ctrl＋0、MAC＝Command＋0）です。
  上位レイヤーでも基本配列の操作を引き継ぎます。対応アプリでズーム率を100%に戻します。
  WIN配列の中央親指キー（EditorでNoneだった位置）は、押している間ジェスチャー（K＋L相当）です。MAC側のLANG1は維持します。
- BluetoothレイヤーのY／U／I／O／セミコロンキーで接続先0〜4を選択します。
  接続先0はMAC、1〜4はWINを選び、接続先ごとの基本レイヤーを保存します。
  USB接続中でも選択したBluetooth枠へ保存し、USB用の配列設定は上書きしません。
  実際の出力先が切り替わると、その出力先に保存した配列を適用します。
- スクロールは縦横の軸ロック付き。WIN／MACで横スクロールの方向を切り替えます。
- ジェスチャーは左右でデスクトップ切り替え、上下で一覧表示。MacではControl＋上下、WindowsではWin＋Tabです。
- エンコーダー回転は全レイヤー共通で左＝音量、右＝ズームイン／アウトです。
- 通常カーソルは両軸反転・速度4/5、スクロールは1/12。センサーが異なるためCoronLPと物理的な感度まで同一にはなりません。

- 左右のLED表示は純正Cornixのタイミングを基準に共通化しています。
  - 内側：左右接続時に青3秒、切断時に青400ms／1秒を最大10回。
  - 各側の電池残量20%以下：内側が赤400msを2回（間隔400ms）。
  - 外側：PCのBluetoothプロファイル0/1/2＝緑/赤/青を両側に表示。
    接続時600ms、未接続でプロファイルがopenなら400ms／1秒を最大10回。
    右から左へ既存のsplit通信で状態を送り、再接続時にも再同期します。
  - 各側のUSB給電中：内側が緑で約2秒周期の呼吸表示。残量95%以上で緑3秒後に消灯。
    充電ICの状態ではなくUSB給電と電池残量からの推定です。
  - 内側の優先順位：電池低下、左右接続案内、充電表示。
  - 右はPWM、左はWS2812を使います。LEDの内外の対応・色・明るさ・左右同期は実機未確認です。
    左右とも同じビルドのUF2へ更新してください。

ZMK Studioの保存済み配列はビルド内の初期配列より優先します。
旧9レイヤーなどの設定を保存している場合は、キー配列のリセットが必要です。Bluetoothも含めた全設定消去は任意です。
導入後はStudioでWIN〜FNの9レイヤーと割り当てを確認してください。
キー・レイヤー・スクロール・ジェスチャーは`config/corex_right.keymap`、
CPIはJ3／J4のoverlay内で`&corex_paw { res-cpi = <1216>; };`のように変更できます。
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

WIN／MAC配列の右手左下はFN専用、その右隣は短押しLANG2／長押しNUMです。左手のSpaceは通常のSpace入力のみです。

通常配列（WIN／MAC）の右端下段にあった`/`は右Shiftに変更し、`/`はトラックボール横の空きキー（右Shiftの左下、位置47）へ移しています。
