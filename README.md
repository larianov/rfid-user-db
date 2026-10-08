<div align="center">

# 🪪 card_anl

**An RFID access control system for the Raspberry Pi Pico — tap a card on an
MFRC522 reader, get *allowed*, *blocked* or an admin console over USB, with the
user database kept in on-chip flash.**

[![Language](https://img.shields.io/badge/language-C%2B%2B20-00599C)](https://isocpp.org/)
[![Board](https://img.shields.io/badge/board-Raspberry%20Pi%20Pico-green)](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
[![SDK](https://img.shields.io/badge/Pico%20SDK-%E2%89%A5%202.2.0-blueviolet)](https://github.com/raspberrypi/pico-sdk)
[![Chip](https://img.shields.io/badge/reader-MFRC522-orange)](https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf)
[![License](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)

</div>

---

## ✨ Features

- **Three access levels** — every card is `ADMIN`, `USER` or `BLACK_LIST`, and
  each level has its own LED and message
- **Admin console over USB** — tap an admin card and a serial shell opens:
  add, remove and re-rank users, list the database, save it
- **Enroll by tapping** — `insert` asks you to place the new card on the reader;
  no need to type UIDs
- **Survives power loss** — the database is written to the last flash sector and
  protected with CRC32; a broken or empty sector is ignored at boot
- **Can't lock yourself out** — the last admin can't be removed or demoted
- **Sleeps between cards** — the reader is polled only on its `IRQ` line, the
  core waits in `__wfi()` the rest of the time
- **No heap** — users live in a fixed-size `etl::unordered_map` (100 entries),
  strings and buffers are [ETL](https://github.com/ETLCPP/etl) containers
- **Built on [`mfrc522-nonblocking`](https://github.com/larianov/mfrc522-nonblocking)**
  — the non-blocking RC522 driver, fetched automatically by CMake

---

## 🧠 How It Works

```
 card ──▶ RC522 ──IRQ──▶ main loop ──▶ list_users ──┬─▶ BLACK_LIST → red LED,   "GET OUT <name>"
                                       (UID lookup)  ├─▶ USER       → green LED, "Hello, <name>!"
                                                     └─▶ ADMIN      → admin_session (USB shell)
                                                                          │
                                                                          └─ save ─▶ flash (last sector)
```

| Module                                          | What it does                                               |
| ----------------------------------------------- | ---------------------------------------------------------- |
| [`main.cpp`](emb/src/main.cpp)                  | hardware setup, the card loop, LEDs                        |
| [`list_u`](emb/include/list_u.hpp)              | the user database: insert, remove, change status, lookup   |
| [`admin_panel`](emb/include/admin_panel.hpp)    | the USB admin shell and its command parser                 |
| [`memory_map`](emb/include/memory_map.hpp)      | saving / restoring the database to and from flash          |

### The card loop

1. Start a UID transaction (`REQA`) on the reader
2. Sleep in `__wfi()` until the RC522 pulls `IRQ` low, then `poll()` the driver
3. When a UID arrives, pack its bytes into a `uint64_t` and look it up
4. Unknown cards are silently ignored. Known ones light their LED for 1.5 s —
   or, for an admin, open the console until it's closed

Because the reader uses `REQA`, each card is read **once per tap**. To read it
again, take it away and bring it back.

### At boot

1. The database is created with one built-in admin (`ADMIN_KEY` in
   [`main.cpp`](emb/src/main.cpp))
2. The flash sector is checked: if it holds a valid record, the saved users are
   loaded on top. USB serial prints `SUCC ON RECOVERING` or
   `RECOVERING NOT FOUND`

---

## 🛠 Hardware

- Raspberry Pi Pico (RP2040)
- MFRC522 module (the common blue "RC522" board)
- 3 LEDs + resistors
- ISO 14443A cards or tags — any card whose UID the reader can get

### Wiring

**RC522** (SPI0, 5 MHz):

| RC522  | `SDA` (CS) | `SCK` | `MOSI` | `MISO` | `IRQ` | `RST` | `3.3V` | `GND` |
| ------ | :--------: | :---: | :----: | :----: | :---: | :---: | :----: | :---: |
| Pico   | `GP5`      | `GP2` | `GP3`  | `GP4`  | `GP6` | `3V3` | `3V3`  | `GND` |

**LEDs** (active high):

| Pin    | Lights up for |
| ------ | ------------- |
| `GP13` | blocked card  |
| `GP14` | user          |
| `GP15` | admin — stays on while the console is open |

> The RC522 runs at **3.3 V only**. Don't connect it to `VBUS`.

---

## 🚀 Getting Started

### Requirements

- CMake ≥ 3.25 and Ninja
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) **≥ 2.2.0** — set
  `PICO_SDK_PATH`, or let `pico_sdk_import.cmake` fetch it
- `arm-none-eabi-gcc`
- [`picotool`](https://github.com/raspberrypi/picotool) — optional, for the
  `load` target

ETL and the RC522 driver are downloaded by CMake (`FetchContent`) on the first
configure.

### 1. Set your admin card

The first admin is built into the firmware. Read your card's UID (for example
with the [`uid` example](https://github.com/larianov/mfrc522-nonblocking/tree/main/examples/uid)
of the driver), and put it into [`emb/src/main.cpp`](emb/src/main.cpp), most
significant byte first — in the same order the reader returns the UID bytes:

```cpp
constexpr uint64_t ADMIN_KEY = 0x572A9331;   // UID bytes 57 2A 93 31
```

### 2. Build

| Preset | Build type | Output directory           |
| ------ | ---------- | -------------------------- |
| `e`    | Debug      | `build/embedded-debug/`    |
| `er`   | Release    | `build/embedded-release/`  |

```bash
cmake --preset e
cmake --build --preset e
```

The firmware is `build/embedded-debug/emb/card_anl.uf2`.

### 3. Flash

With `picotool` (the board must be in BOOTSEL mode, or running firmware with USB
stdio):

```bash
cmake --build --preset e --target load
```

Or hold **BOOTSEL**, plug the Pico in and copy `card_anl.uf2` to the drive.

### 4. Open the console

Connect to the Pico's USB serial port with any terminal (`picocom`, `minicom`,
`screen`, PuTTY…):

```bash
picocom /dev/ttyACM0
```

Tap the admin card — the admin LED lights up and you get the prompt:

```
admin>
```

---

## 💻 Admin Console

| Command                       | Short | What it does                                                        |
| ----------------------------- | :---: | ------------------------------------------------------------------- |
| `insert <name> <status>`      | `i`   | add a user — then place the new card on the reader within 7 s       |
| `change <status> [id]`        | `c`   | change a user's status — by `id`, or by placing the card            |
| `remove [id]`                 | `r`   | remove a user — by `id`, or by placing the card                     |
| `list`                        | `l`   | print all users: id, name, status                                   |
| `save`                        | `s`   | write the database to flash                                         |
| `help`                        | `h`   | print the command list                                              |
| `q`                           |       | close the console                                                   |

**Statuses:** `0` — blocked, `1` — user, `2` — admin.

**`id`** is the UID as a **decimal** number, exactly as `list` prints it.

```
admin> i alice 1
Please, place the card
Success
admin> list
User id: 1462407985, User name: admin, User status: 2
User id: 2864434397, User name: alice, User status: 1
admin> c 0 2864434397
Success
admin> save
Success backloged
admin> q
CON
```

The console closes on `q` or after **5 minutes**, counted from the moment the
admin card was tapped. The session prints `CON` and the reader goes back to
checking cards.

> ⚠️ **Changes are kept in RAM until you `save`.** If the Pico loses power
> before that, every change since the last save is gone.

---

## 💾 Flash Storage

The database takes the **last 4 KB sector** of flash
(`XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE`), so it doesn't collide
with the firmware. Saving erases and rewrites the whole sector through
`flash_safe_execute`.

### Layout

| Offset             | Size         | Content                                    |
| ------------------ | ------------ | ------------------------------------------ |
| `0`                | 1            | number of users `N`                        |
| `1`                | 7            | zero padding                               |
| `8 + 32·i`         | 32 × `N`     | user records                               |
| `8 + 32·N`         | 4            | CRC32 of everything above                  |

**One user record — 32 bytes:**

| Bytes | 0–7                    | 8–23                          | 24     | 25–31   |
| ----- | ---------------------- | ----------------------------- | ------ | ------- |
|       | UID (`uint64_t`, LE)   | name, zero-padded if shorter  | status | padding |

At boot the record is accepted only if `N` isn't `0x00` / `0xFF`, isn't bigger
than the database, and the CRC32 matches. Otherwise the firmware starts with only
the built-in admin.

---

## ⚠️ Things to Keep in Mind

<details>
<summary><b>The built-in admin always comes back</b></summary>

`ADMIN_KEY` is added on every boot, before the saved database is loaded. If you
removed or blocked it and saved, it will still be an admin after a reboot. To
change the master card, change `ADMIN_KEY` and reflash.
</details>

<details>
<summary><b>Names are up to 16 characters</b></summary>

Longer names are refused with `Name is too long`. A name is one word — the
parser splits the command on spaces.
</details>

<details>
<summary><b>Up to 100 users</b></summary>

The database is a fixed-size map (`size_of_map` in
[`list_u.hpp`](emb/include/list_u.hpp)). When it's full, `insert` answers
`Database is full`.
</details>

<details>
<summary><b>The console needs a USB host</b></summary>

After an admin tap, the firmware waits for a USB serial connection. Until one
appears (or the 5 minutes pass), **no other cards are checked**.
</details>

<details>
<summary><b>Input line is up to 42 characters</b></summary>

Typing more clears the line with `Buffer is full, try again`. Backspace works.
</details>

---

## 📂 Repository Structure

```
.
├── emb/
│   ├── include/
│   │   ├── admin_panel.hpp    # admin_session — USB console, uid_converter
│   │   ├── list_u.hpp         # list_users — user database, statuses
│   │   └── memory_map.hpp     # flash address, write_save / reader_after_wc
│   ├── src/
│   │   ├── main.cpp           # setup, card loop, LEDs, ADMIN_KEY
│   │   ├── admin_panel.cpp
│   │   ├── list_u.cpp
│   │   └── memory_map.cpp
│   └── CMakeLists.txt         # firmware target, ETL + RC522 driver via FetchContent
├── CMakeLists.txt             # project setup, warnings, `load` target
├── CMakePresets.json          # e (Debug), er (Release)
├── pico_sdk_import.cmake
├── .clang-format
└── .clang-tidy
```

---

## 🗺 Roadmap

- [x] Card check with three access levels
- [x] Admin console over USB
- [x] Saving the database to flash with CRC32
- [x] IRQ-driven reader polling with `__wfi()` sleep
- [ ] Set the master admin card without reflashing
- [ ] Hex UIDs in the console

---

## 📜 License

Released under the MIT License. See [`LICENSE`](LICENSE) for details.
