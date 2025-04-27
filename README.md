# Need for Speed Heat - H-Shifter v2

### Full H-Shifter Support for NFS Heat

[**👉Download the Latest Release**](https://github.com/x0reaxeax/nfsheat-hshifter/releases/latest)

## 🚗 What's New in v2:

All the annoying manual memory scans from v1 are gone. The new version automatically locates and updates gear addresses, supports custom gear mappings, and includes an ASCII gear display inside the console.

### ✨ Features:

- **Automatic memory scanning**: Launch the program, select single or multiplayer, and it does the rest.
- **Supports up to 8 gears + Reverse (Neutral possible, but doesn't rev in the game)**.
- **Customizable key mappings** (default keys: `0–9`).
- **Built-in Gear Display Console**: See the current gear in a goofy ASCII "art".
- **Easy re-scan**: Just press a key after switching cars or leaving the garage.

---

## 🛠️ Getting Started

### 📌 Setup:

1. **Download and Extract** the latest release from the [releases page](https://github.com/x0reaxeax/nfsheat-hshifter/releases/latest).

2. **Configure Your Shifter**:

   - Map your shifter gears to keyboard keys using your shifter’s software (e.g., Logitech G HUB).
   - For customizing key mappings, see [Customizing Keybindings](#customizing-keybindings) section below.
   - Default key mapping (can be customized in the config):

| Key | Gear        |
| --- | ----------- |
| `0` | Reverse (R) |
| `1` | Neutral (N) |
| `2` | Gear 1      |
| `3` | Gear 2      |
| `4` | Gear 3      |
| `5` | Gear 4      |
| `6` | Gear 5      |
| `7` | Gear 6      |
| `8` | Gear 7      |
| `9` | Gear 8      |

**Example mapping for Logitech Driving Force Shifter:**
![ghub](https://i.imgur.com/eTj3Fx6.png)

3. **Run NFS Heat**:

   - Launch the game, set transmission to manual (also disable auto-reverse), and load into a session **(must NOT be inside the garage)**.

4. **Run the H-Shifter Program**:

   - Launch `Heat-HShifter2.exe`.
   - The program will automatically scan for gear addresses.
   - If your game is minimized, the program will automatically maximize it for the duration of the scan.
   - Once completed, you can immediately start using your shifter!

---

## ⚙️ Controls

- **Gear Keys** (`0–9` by default): Change gears.
- **INSERT**: Toggle between the main and gear-display console windows.
- **DELETE**: Rescan gear addresses (use this if you change cars or leave the garage).
- **END**: Exit the program safely.

---

## 🎨 Customizing Keybindings

You can customize your keybindings easily by editing the `config.ini` file located at:

```
%USERPROFILE%\Documents\Heat-HShifter2\config.ini
```

Example configuration:

```ini
[CONFIG]
GEAR_REVERSE=0x30
GEAR_NEUTRAL=0x31
GEAR_1=0x32
...
GEAR_8=0x39
```

Key values can be found at [Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes).  
**Note:** Make sure the keycodes are properly mapped in your shifter's software as well.

---

## 🐞 Known Issues & Solutions

- **Console lag on Windows 11**: Set your system's Power Plan to "High Performance".
- **Memory scan takes too long**: The scan speed may vary due to game protections like Denuvo. Just give it some time, usually takes between 10-40 seconds tops.
- **Gear not responding**: Press `DELETE` to rescan gear addresses.
- **Gear addresses not found**: Please see the [Troubleshooting & Support](#troubleshooting--support) section.

---

## 📣 Troubleshooting & Support

If you encounter any issues, please open an issue on the [GitHub Issues page](https://github.com/x0reaxeax/nfsheat-hshifter/issues).  
If possible, it would be great if you could provide either a screenshot or a copy-paste of the console output (main window, not gear display window), and if extra possible, a memory dump of the game process. This will be incredibly helpful when I try to identify the issue.  
I have a limited number of machines to test on, so I cannot guarantee the program will work out of the box on all systems. However, opening a new issue and documenting the program/game behavior will help shaping the program for everyone 🧡

---

## 💖 Credits & License

Created by [x0reaxeax](https://github.com/x0reaxeax).  
ChatGPT for README.md generation 🧡  
Licensed under **GPLv3**.  

---

**Enjoy your H-Shifter in Need for Speed Heat - Happy racing! 🚗💨🔥**

