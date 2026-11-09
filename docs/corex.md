# CoreX R A14 + PAW3222 / ZMK

CoreXの右交換基板 **R A14** とPAW3222トラックボールを、純正Cornix左＋ZMKで使う構成です。
右がCentralで、USB・Bluetooth・ZMK Studioの接続先になります。

## ビルド成果物

GitHub Actionsの`firmware`アーティファクトに次のCoreX構成用UF2を生成します。

| 用途 | artifact-name |
| --- | --- |
| 純正Cornix左 | `corex_stock_left` |
| 純正Cornix左のZMK設定消去 | `corex_stock_left_reset` |
| CoreX右・S140 v6 | `corex_r_a14_s140v6` |
| CoreX右・S140 v7 | `corex_r_a14_s140v7` |
| CoreX右のZMK設定消去・v6 / v7 | `corex_r_a14_reset_s140v6` / `corex_r_a14_reset_s140v7` |

**右には`corex_r_a14_*`、純正左には`corex_stock_left*`を使用します。左右のUF2は別物です。**
CoreX右用は既存のSoftDeviceを保持し、v6ではアプリを`0x26000`、v7では`0x27000`から配置します。
`nrf52840-nosd`スニペットは使用しません。UF2はブートローダー領域へ書き込みません。
設定領域はCoreX RMKと同じ`0xb0000–0xcffff`を予約しますが、設定の内容は互換ではありません。

RESETを2回押したときのドライブ内`INFO_UF2.TXT`にあるSoftDevice版に合わせてv6/v7を選びます。
記載が判別できない場合は書き込み前に確認してください。
センサーの接続先（J3／J4）は起動時と未検出中2秒ごとにJ4→J3の順で確認し、応答した方を使います。
両方に接続した場合はJ4だけを使います。

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
| 2 | NUM | 右手LANG1長押し。右端テンキーと記号・括弧マクロ |
| 3 | FN | 右手左下を押している間。右端4行×3列で上からF1〜F3、F4〜F6、F7〜F9、F10〜F12 |
| 4 | PAN | 左手LANG2長押しの間、ボールでスクロール、E／S／D／F＝↑／←／↓／→ |
| 5 | GESTURE | 左手の最も内側の親指キーを押している間、ボールでデスクトップ操作、E／S／D／F＝↑／←／↓／→ |
| 6 | MOUSE | ボール操作で有効、J／K／L／M／カンマ以外のキーを押すまで維持。J／K／L＝左／中／右クリック、M／カンマ＝戻る／進む |
| 7 | BLUETOOTH | 左の内側スイッチを押している間、接続先選択・登録消去・ブートローダー操作 |

- Iは通常の文字キーです。
- 親指キーはWIN／MAC共通で、Space、短押しLANG2／長押しスクロール、ジェスチャー専用、FN、短押しLANG1／長押しNUMです。
  OSごとのCtrl／GUI配置は維持しています。
- NUM／FN共通で、右手左側の上段に`\`・`()`・`[]`、その下に`{}`・`_`・`~`、`~`の下に`` ` ``を配置しています。
  FNレイヤーは右端4行×3列に、上からF1〜F3／F4〜F6／F7〜F9／F10〜F12を配置しています。右端は上から`1 2 3`／`4 5 6`／`7 8 9`／`0 無効 元の割当`のテンキー配置です。
  テンキー部分も通常の数字キーコードを使い、Enter・小数点は追加していません。
  括弧マクロは開閉を入力後、左矢印を1回送ってカーソルを間へ戻します。
  US配列のキーコードを使い、US／JIS切り替え機能は追加していません。
- 基本配列のセミコロン／引用符の押し分けはCoronLPを継承します。WIN／MAC共通で右下はスラッシュ・右Shift、最下段はマイナス・イコール・Deleteです。
- 同時押し（コンボ）はありません。
- 右の内側スイッチ（エンコーダー押し込み）はズーム率のリセット（WIN＝Ctrl＋0、MAC＝Command＋0）です。
  上位レイヤーでも基本配列の操作を引き継ぎます。対応アプリでズーム率を100%に戻します。
  WIN／MACとも左手の最も内側の親指キーは、押している間ジェスチャーです。このキーからLANG2入力は出しません。
- BluetoothレイヤーのY／U／I／O／セミコロンキーで接続先0〜4を選択します。
  接続先0はMAC、1〜4はWINを選び、接続先ごとの基本レイヤーを保存します。
  USB接続中でも選択したBluetooth枠へ保存し、USB用の配列設定は上書きしません。
  実際の出力先が切り替わると、その出力先に保存した配列を適用します。
- Bluetoothレイヤーの左上（Esc位置）で左、右上（Backspace位置）で右がUF2書き込みモードに入ります（実機未確認）。
  入らない場合はRESETの2回押しを使います。
- スクロールは共通のPANレイヤーで縦横の軸ロック付き。基本配列がWINかMACかを見て横スクロールの方向を切り替えます。
- ジェスチャーは左右でデスクトップ切り替え、上下で一覧表示。MacではControl＋上下、WindowsではWin＋Tabです。
- 左エンコーダーはスクリーンショット操作です（WIN／MACレイヤー）。
  MAC：押し込み＝⌘⇧3（全画面）、上回転＝⌘⇧4（範囲）、下回転＝⌘⇧9（前面ウィンドウ、macOS側で要設定）。
  WIN：押し込み＝PrtScn（全画面）、上回転＝Win＋Shift＋S（範囲）、下回転＝Alt＋PrtScn（前面ウィンドウ）。すべてクリップボードへコピーします。
  押し込みは長押しでBLUETOOTHレイヤーです。
- 右エンコーダー回転はWIN／MACでズームイン／アウトです。
- WIN／MAC・自動マウスレイヤー以外では、左右とも回転は無効です。自動マウスレイヤー中はWIN／MACのノブ操作がそのまま効きます。
- 通常カーソルは両軸反転・速度4/5、スクロールは1/12。センサーが異なるためCoronLPと物理的な感度まで同一にはなりません。

- LED表示（左はWS2812、右はPWM。色・明るさ・内外の対応は実機未確認）
  - 左・内側：左右接続時に青3秒、切断時に青400ms／1秒を最大10回。
    電池残量20%以下で赤400msを2回。USB給電中は緑で約2秒周期の呼吸表示を続け、残量95%以上で緑の常時点灯（90%未満で呼吸表示に戻る）。USBを外すと消灯。
    優先順位は電池低下、左右接続案内、充電表示。
  - 左・外側：右で現在有効なレイヤーの色を常時点灯（右から既存のsplit通信で送信）。
    WIN＝赤、MAC＝青、NUM＝緑、FN＝黄、PAN＝マゼンタ、GESTURE＝水色、BLUETOOTH＝白。MOUSE（自動マウスレイヤー）中は消灯。
    左右が切断中は消灯します。
  - 右・外側：PCのBluetoothプロファイル0／1／2／3／4＝緑／赤／青／黄／紫。
    接続時600ms、未接続でプロファイルがopenなら400ms／1秒を最大10回。
  - 右・内側：右の電池低下（赤2回）とUSB給電中の表示のみ。左右接続の案内は左だけに出します。
  - 操作が30秒ない（ZMKのIDLE）と、左右とも外側と左右接続の表示を消灯します。
    電池低下・充電表示はIDLE中も内側に出します。
    右はキー・エンコーダー・トラックボール、左は左のキー操作か右の操作中で通常表示に戻ります。
    ZMKはIDLE中に電池残量の測定を止めますが、USB給電中だけは60秒ごとに測定を続け、満充電の緑を表示します。
    電池駆動中のIDLEでは測定しないため、電池低下の赤は操作再開後の測定で表示します。
    電池駆動中のIDLEで全消灯から16秒たつと、LED処理の確認を1秒ごとに減らします（USB接続後の充電表示は最大1秒遅れます）。
  - 充電表示は充電ICの状態ではなくUSB給電と電池残量からの推定です。
    左右とも同じビルドのUF2へ更新してください。

ZMK Studioの保存済み配列はビルド内の初期配列より優先します。
旧9レイヤーなどの設定を保存している場合は、キー配列のリセットが必要です。Bluetoothも含めた全設定消去は任意です。
導入後はStudioでWIN／MAC／NUM／FN／PAN／GESTURE／MOUSE／BLUETOOTHの8レイヤーと割り当てを確認してください。
キー・レイヤー・スクロール・ジェスチャーは`config/corex_right.keymap`、
CPIは`boards/shields/corex_trackball/corex_trackball.overlay`内で`res-cpi = <1216>;`のように変更できます。
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

WIN／MAC配列の右手左下はFN専用、その右隣は短押しLANG1／長押しNUMです。左手のSpaceは通常の空白入力のみです。

通常配列（WIN／MAC）の右端下段にあった`/`は右Shiftに変更し、`/`はトラックボール横の空きキー（右Shiftの左下、位置47）へ移しています。

### ウィンドウ操作

レイヤー5（GESTURE）中、右手のI／J／K／Lで次のキー操作を送ります。

| キー | WIN | MAC |
| --- | --- | --- |
| I | 左のモニターへ移動（Win＋Shift＋←） | Command＋↑ |
| J | 左半分へ配置（Win＋←） | Command＋← |
| K | 右のモニターへ移動（Win＋Shift＋→） | Command＋↓ |
| L | 右半分へ配置（Win＋→） | Command＋→ |

MACの動作はPC側のショートカット設定・アプリに従います。Spaceは通常の空白入力のみです。GESTURE中のESDF矢印と、ボールによるデスクトップ切り替えは維持しています。

### Keymap Editorでの編集とCI

CIは空白・改行・コメント・数値の括弧や16進表記に依存せずにキー数とレイヤー参照を確認します。Spaceの個数やIJKLの固定位置を前提にしたテストはなく、キーの再割り当てでテストを修正する必要はありません。49キーの不足や存在しないレイヤーへの参照は引き続き検出し、DTSの構文・動作定義はファームウェアのビルドで検証します。レイヤーの追加・並べ替え時は、センサー処理とLED表示の番号対応も合わせて更新してください。
