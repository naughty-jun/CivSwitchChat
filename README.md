[English](README.md) | [日本語](README.ja.md)

# CivSwitchChat

A homebrew app for sending chat messages to Civilization VII lobbies from a Switch 1.

## *No Pico, Just a Switch 1 Running CFW*

*No need to buy a Pico or any additional hardware if you already have a Switch 1 running CFW*

## Features

Uses LDN (Local Wireless) to send chat messages directly to Civilization VII lobbies.

- Sends messages faster than using keyboard macros.
- **(Untested)** Should also work with Switch 2 Japanese-Language System (Japan Only).

## Requirements

* **Switch 1**
    - CFW (Atmosphère)

* **Switch 2**
    - Firmware version 23.0.1 or lower
    - Must not have triggered the Civilization VII startup **"supernag"** yet

* **Civilization VII**
    - Switch 1 edition
    - Physical cartridge
    - Version 1.0.0

* **Node.js environment**
    - Windows, macOS, Linux, etc.

## Usage

### 1. Generate `payload.txt`

Run the following commands in your Node.js environment:

```bash
git clone https://github.com/Gezine/CivSwitch.git
cd CivSwitch/PicoSwitch/
npm i terser
node compress.js
```

### 2. Copy the files to your SD card

Place the `.nro` file (from [releases](https://github.com/naughty-jun/CivSwitchChat/releases)) and the generated `payload.txt` on your SD card:

```text
sd card
└──📁 switch/
    └──📁 CivSwitchChat/
        ├── CivSwitchChat.nro
        └── payload.txt
```

You can rename `payload.txt` to anything you like.

### 3. Launch Civilization VII on your Switch 2

1. Select MULTIPLAYER.
2. Select **WIRELESS**.
3. Create a game.
4. Host a lobby.

### 4. Launch CivSwitchChat on your Switch 1

1. **Turn off Airplane Mode.**
    - **No Wi-Fi connection is required.**
2. **Launch the app in Applet Mode.**
    - **Application Mode is not supported.**

    <img width="300" alt="01" src="https://github.com/user-attachments/assets/463f9df1-ea4d-4eda-83a0-558fe1dad3e7" />

3. **Scan for lobbies** (press Y).

4. **Select a lobby** (D-Pad Up/Down).
    - Find your Switch 2's MAC address.

    <img width="300" alt="02" src="https://github.com/user-attachments/assets/d40175ae-2170-4b35-ae97-714582c39569" />

5. **Join the lobby** (press A).

    <img width="300" alt="03" src="https://github.com/user-attachments/assets/a4614303-2a32-4ca1-b6f7-46f6b36d2a43" />

6. **Open the file selection dialog** (press Y).

7. **Select the payload file** (D-Pad Up/Down).
    - Select the `payload.txt` file you copied earlier (or its renamed equivalent).
    - You can also send arbitrary chat messages instead of a payload.

    <img width="300" alt="04" src="https://github.com/user-attachments/assets/95265c2b-037a-483a-94aa-febc3372e9e3" />

8. **Adjust the delay** (D-Pad Left/Right).
    - `50 ms` is the minimum delay.
    - If messages are not sent in the correct order, try increasing the delay.

9. **Start sending the payload** (press A).

    <img width="300" alt="05" src="https://github.com/user-attachments/assets/bf340a5f-f419-49ba-a164-e34fdf2e935a" />
    <img width="300" alt="06" src="https://github.com/user-attachments/assets/a2d2edc7-a1a0-4572-9b4c-daf1e63215f1" />

### 5. That's it

- If your Switch 2 displays Gezine's payload waiting screen, everything worked

---

## Disclaimer

Use this software at your own risk. The author assumes no responsibility for any device damage, data loss, or account suspensions (bans) resulting from the use of this software.

## License

This project is licensed under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

## Credits

- **[CivSwitchLDN](https://github.com/ItsNerisma/CivSwitchLDN)** by ItsNerisma (GPLv3) — Reuses some of the LDN communication constants from its `ldn/` directory.
- **[CivSwitch](https://github.com/Gezine/CivSwitch)** by Gezine (MIT) — Uses the `PicoSwitch/compress.js` script.
