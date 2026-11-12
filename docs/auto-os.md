# Bluetoothの自動OS判定

純正Cornix右・CoreX右のどちらも、PCとのBluetooth接続を担当する右側で判定します。
全接続先で自動判定のみを使い、WIN／MACの手動選択・保存・復元はありません。
OSの判定結果は現在の接続中のRAMだけに保持し、切断すると破棄します。
Bluetoothのペアリング情報とStudioのキーマップ保存は通常どおり残ります。

## 動作

1. 起動・再接続時はWINです。古い`default_layer/settings`は読み込みません。
2. PCが読むHID Report Map、HID Information、PnP ID、GAP Appearanceを観察します。通信内容を変更したり、読み取りを拒否したりはしません。
3. 新しい判定材料が来なくなってから1秒待って適用します。接続中は観察を続け、後から新しい材料が来れば再判定します。その待ち時間中は直前の判定を維持します。電池残量のポーリングや同じ情報の再読では再判定しません。
4. 選択中のBluetooth接続の判定を反映します。接続先を変えると、同時接続中の各端末の判定に追従します。左手との通信はOS判定の対象にしません。

| 観察した読み取り | 使用する基本レイヤー |
| --- | --- |
| PnP IDとGAP Appearance | WIN（Windows／Linux系のパターン） |
| Report MapとHID Information | WIN（Android系のパターン） |
| Report MapまたはPnP IDがあり、上記には該当しない | MAC（macOS／iOS／iPadOS系のパターン） |
| 判定前、情報不足、未接続、USB出力 | WIN |

NUM、FNなどの補助レイヤーはそのままで、MACレイヤー（内部ID 1）の有効／無効だけを変更します。
内部ID 0をWIN、1をMACとして維持してください。Studioでこの2レイヤーを削除・別用途に変更すると自動切替の意味も変わります。

## 制限

- OS名をPCから受け取るのではなく、Bluetooth接続時の通信からの推測です。全OS・全バージョンでの正確性は保証できません。
- 参考元のMac実機記録はPnP IDのみを読む部分的な記録です。Report Mapの読み取りを必須にするとこのMacのパターンを取りこぼすため、PnP IDのみでもMAC候補とします。Windowsが情報を読み終わる前などには誤判定の可能性があります。
- iOSとmacOSは区別せずMACにまとめます。LinuxとWindowsも区別せずWINにまとめます。
- 接続後1秒で必ず判定できるわけではありません。OSが必要な情報を読む時刻によって変わります。
- GATT情報がPC側にキャッシュされ、再接続時に読み直されなければ判定できずWINになります。保存しない設計なので、以前の判定で補うこともしません。
- USBのOS自動判定は対象外です。充電ケーブルでつながっていても、実際の出力がBluetoothならその接続を判定します。
- 以前の配列保存機能は削除済みです。Studioに直接`df`動作を割り当てていた場合は、そのキーを修正するかStudioでキーマップを初期状態へ戻してください。標準の接続先マクロは旧ノード名を保持し、接続先の選択だけに変更しています。

## 実機確認

通常UF2を更新した後、Mac、Windows、必要ならiPhone／iPadで、各接続先の文字入力・Command／Controlと左のレイヤー色を確認します。
接続先0も含め、どの番号にどのOSを登録しても自動判定します。
切断・再接続・電源の入れ直しでも判定し直すことを確認してください。
MACにならない場合は、PC側の登録削除と該当接続先のペアリング消去後に再ペアリングし、初回の情報読み取りと再接続時の挙動を比較します。
この処理の実機検証は未実施です。

## 実装

`drivers/keymap/auto_os.c`で、固定済みのZMK／Zephyrが提供するGATT読み取りコールバックを使います。
外部モジュール追加、ZMK forkへの変更、独自Studio RPCは不要です。
`CONFIG_CORNIX_AUTO_OS_SETTLE_MS`（既定1000ms）で情報収集後の待ち時間を調整できます。
settings_reset画像と左Peripheralには判定処理を組み込みません。

判定方式は[cormoran/zmk-feature-os-detection](https://github.com/cormoran/zmk-feature-os-detection)のBLE実装を参考にしています。
分類ルールの根拠と限界は同プロジェクトの[実機記録](https://github.com/cormoran/zmk-feature-os-detection/blob/main/docs/fingerprints.md)にあります。
ライセンスは`drivers/keymap/auto_os.LICENSE`に保持しています。
