# CivSwitchChat

Switch 1 で Civilization VII のロビー内に向けてチャットメッセージを送信するための自作ソフト

## *Pico不要、CFW導入済みSwitch 1があれば*

*CFWを導入した Switch 1 を持っていれば、Picoなどのハードウェアを追加で購入する必要はありません*

## 特徴

LDN(ローカル通信)を使用して、Civilization VII のロビーに直接チャットメッセージを送信します。

- キーボードマクロを使った入力よりも高速にメッセージを送信できます。
- **(未検証)** 日本語版・国内専用の Switch 2 にも対応しているはずです。

## 必要なもの

* **Switch 1**
    - CFW (Atmosphère)

* **Switch 2**
    - ファームウェア バージョン 23.0.1 以下
    - Civilization VII 起動時の **"supernag"** が発生していないこと

* **Civilization VII**
    - Switch 1版 
    - 物理カートリッジ 
    - バージョン 1.0.0

* **Node.js 実行環境**
    - Windows、macOS、Linux など

## 使い方

### 1. `payload.txt` の生成

Node.js が実行できる環境で、以下のコマンドを実行

```bash
git clone https://github.com/Gezine/CivSwitch.git
cd CivSwitch/PicoSwitch/
npm i terser
node compress.js
```

### 2. SDカードへファイルを配置

`.nro` ファイルと、生成された `payload.txt` をSDカードに配置

```text
sd card
└──📁 switch/
    └──📁 CivSwitchChat/
        ├── CivSwitchChat.nro
        └── payload.txt
```

`payload.txt` は任意のファイル名に変更してOK

### 3. Switch 2 で Civilization VII を起動

1. **マルチプレイヤー** を選択
2. **ワイヤレス** を選択
3. ゲームを作成
4. ロビーをホストする

### 4. Switch 1 で CivSwitchChat を起動

- **機内モードは解除してください。**
    - **Wi-Fi 接続は不要です。**
- **Applet mode で起動してください。**
    - **Application mode には対応していません。**

1. **ロビーをスキャン** (Yボタン)
2. **ロビーを選択** (十字キー 上下)
    - Switch 2 の MAC アドレスを探してください
3. ***ロビーに参加** (Aボタン)
4. **ファイル選択ダイアログを開く** (Yボタン)
5. **payload ファイルを選択** (十字キー 上下)
    - 先ほど配置した `payload.txt`（または変更後のファイル名）を選択してください
    - payload ではなく、適当なチャットメッセージを送信することもできます
6. **送信間隔を調整** (十字キー 左右)
    - 最短の送信間隔は 50ms です
    - メッセージが正しい順番で送信されない場合は、送信間隔を長くしてください
7. **payload の送信を開始** (A ボタン)

### 5. 以上

- Switch 2 の画面に Gezine 氏の payload 待機画面が表示されれば成功

---

## 免責事項

本ソフトウェアは、すべて自己責任でご使用ください。本ソフトウェアの使用によって生じたデバイスの故障・損傷、データの損失、アカウントの停止（BAN）等について、作者は一切の責任を負いません。

## ライセンス

This project is licensed under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

## クレジット

- **[CivSwitchLDN](https://github.com/ItsNerisma/CivSwitchLDN)** by ItsNerisma (GPLv3) — `ldn/` 内から、LDNの通信に関わる一部の定数を流用しています。
- **[CivSwitch](https://github.com/Gezine/CivSwitch)** by Gezine (MIT) — `PicoSwitch/compress.js` を引用しています。
