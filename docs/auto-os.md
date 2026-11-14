# Bluetoothの自動OS判定

純正Cornix右・CoreX右のどちらも、PCとのBluetooth接続を担当する右側で判定します。
全接続先で自動判定のみを使い、WIN／MACの手動選択・保存・復元はありません。
OSの判定結果は現在の接続中のRAMだけに保持し、切断すると破棄します。
Bluetoothのペアリング情報とStudioのキーマップ保存は通常どおり残ります。

## 動作

1. 起動・再接続時はWINです。古い`default_layer/settings`は読み込みません。
2. PCが読むHID Report Map、HID Information、PnP ID、GAP Appearanceを観察します。通信内容を変更したり、読み取りを拒否したりはしません。加えて、読み取りの判定が落ち着き、Report Map＋HID InformationがありAppearanceがない、iPad／Androidで重なるパターンの場合だけ、接続約2秒後以降に公開GATTサービス一覧のApple ANCS／AMSを確認します。従来のMacパターン、Appearanceを読むWindowsパターン、読み取りがない接続には問い合わせません。通知・メディアの内容は読まず、購読もしません。見つかればAppleを優先します。見つからない場合は約4秒間隔で最大3回まで確認し、停止します。
3. 新しい判定材料が来なくなってから1秒待って適用します。接続中は観察を続け、後から新しい材料が来れば再判定します。その待ち時間中は直前の判定を維持します。電池残量のポーリングや同じ情報の再読では再判定しません。
4. 選択中のBluetooth接続の判定を反映します。接続先を変えると、同時接続中の各端末の判定に追従します。左手との通信はOS判定の対象にしません。

| 観察した読み取り | 使用する基本レイヤー |
| --- | --- |
| ANCSまたはAMSのサービス宣言が見つかった | MAC（下記の読み取りパターンより優先） |
| PnP IDとGAP Appearance | WIN（Windows／Linux系のパターン） |
| Report MapとHID Information | WIN（Android系のパターン） |
| Report MapまたはPnP IDがあり、上記には該当しない | MAC（macOS／iOS／iPadOS系のパターン） |
| 判定前、情報不足、未接続、USB出力 | WIN |

NUM、FNなどの補助レイヤーはそのままで、MACレイヤー（内部ID 1）の有効／無効だけを変更します。
内部ID 0をWIN、1をMACとして維持してください。Studioでこの2レイヤーを削除・別用途に変更すると自動切替の意味も変わります。

## 制限

- OS名をPCから受け取るのではなく、Bluetooth接続時の通信からの推測です。全OS・全バージョンでの正確性は保証できません。
- iPadOS 27の実機ログで、Report Map＋HID Information＋PnP ID（`reads=0x07`）を読み、従来のルールでWINと誤分類する例を確認しました。この組み合わせはAndroidとも重なるため、組み合わせだけでApple扱いにはしません。ANCS／AMSの確認で補いますが、Appleの仕様上これらのサービスは常に公開されるとは限らず、見つからなければ読み取りパターンによる従来の判定になります。
- 参考元のMac実機記録はPnP IDのみを読む部分的な記録です。Report Mapの読み取りを必須にするとこのMacのパターンを取りこぼすため、PnP IDのみでもMAC候補とします。Windowsが情報を読み終わる前などには誤判定の可能性があります。
- iOSとmacOSは区別せずMACにまとめます。LinuxとWindowsも区別せずWINにまとめます。
- 接続後1秒で必ず判定できるわけではありません。OSが必要な情報を読む時刻によって変わります。
- GATT情報がPC側にキャッシュされ、再接続時に判定対象を読み直されなければ判定できずWINになります。読み取りがない接続では、従来動作を保つためANCS／AMSの問い合わせも始めません。保存しない設計なので、以前の判定で補うこともしません。
- USBのOS自動判定は対象外です。充電ケーブルでつながっていても、実際の出力がBluetoothならその接続を判定します。
- 以前の配列保存機能は削除済みです。Studioに直接`df`動作を割り当てていた場合は、そのキーを修正するかStudioでキーマップを初期状態へ戻してください。標準の接続先マクロは旧ノード名を保持し、接続先の選択だけに変更しています。

## 実機確認

通常UF2を更新した後、Mac、Windows、必要ならiPhone／iPadで、各接続先の文字入力・Command／Controlと左のレイヤー色を確認します。
接続先0も含め、どの番号にどのOSを登録しても自動判定します。
切断・再接続・電源の入れ直しでも判定し直すことを確認してください。
MACにならない場合は、PC側の登録削除と該当接続先のペアリング消去後に再ペアリングし、初回の情報読み取りと再接続時の挙動を比較します。
読み取りの観測とiPadOS 27での誤分類は実機ログで確認済みです。ANCS／AMSによる補正はホストテストに加え、iPad・iPhoneでMACレイヤーへ切り替わることを実機確認済みです。

### iPadなどで誤判定するときの診断

診断用UF2は通常のビルド対象から除外しています。以下は調査時の手順の記録です。再度必要になった場合のみ、`build.yaml`へ`board: cornix_right`、`shield: cornix_indicator`、`snippet: zmk-usb-logging cornix-os-debug`、`artifact-name: cornix_stock_right_os_debug`の項目を追加してビルドします。

`cornix_stock_right_os_debug.uf2`は純正Cornix右専用のUSBログ診断版です。
通常版と同じ判定ルールですが、USBはログ出力専用で、キー入力はBluetoothへ送ります。
Studioは無効です。左手の更新やペアリング消去は不要です。

1. 診断版を右手へ書き込み、右手をMacのUSBにつなぎます。
2. Macで`ls /dev/cu.usbmodem*`を実行し、表示された右手のポートを`screen /dev/cu.usbmodemXXXXX 115200`で開きます。
3. ログを開いたまま、iPad側でCornixの接続を一度切り、再接続します。初回登録時と再接続時の違いを調べる場合のみ、該当接続先を消去して再登録します。
4. `Bluetooth OS host connected`、`Bluetooth OS read`、`Bluetooth OS evidence`、`Bluetooth OS Apple service`の行を確認します。`reads`のビットは`01`=Report Map、`02`=HID Information、`04`=PnP ID、`08`=Appearance、`10`=Appleサービスです。`result`は`0`=不明、`1`=WIN、`2`=Appleです。`Apple service`の`found=1`ならMACへ切り替えます。接続行だけでread行がなければ、観測対象の読み取りが得られていません。
5. 診断後は通常の`Cornix.uf2`へ戻します。

ログにはBluetooth接続・判定情報が含まれます。診断版は常用せず、通常版を使ってください。

## 実装

`drivers/keymap/auto_os.c`で、固定済みのZMK／Zephyrが提供するGATT読み取りコールバックを使います。
外部モジュール追加、ZMK forkへの変更、独自Studio RPCは不要です。
`CONFIG_CORNIX_AUTO_OS_SETTLE_MS`（既定1000ms）で情報収集後の待ち時間を調整できます。
settings_reset画像と左Peripheralには判定処理を組み込みません。

判定方式は[cormoran/zmk-feature-os-detection](https://github.com/cormoran/zmk-feature-os-detection)のBLE実装を参考にしています。
分類ルールの根拠と限界は同プロジェクトの[実機記録](https://github.com/cormoran/zmk-feature-os-detection/blob/main/docs/fingerprints.md)にあります。
Appleサービスの仕様は[ANCS](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleNotificationCenterServiceSpecification/Specification/Specification.html)と[AMS](https://developer.apple.com/library/archive/documentation/CoreBluetooth/Reference/AppleMediaService_Reference/Specification/Specification.html)を参照してください。
ライセンスは`drivers/keymap/auto_os.LICENSE`に保持しています。
