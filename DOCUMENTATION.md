---
title: "Ambulance Simulation System"
subtitle: "ESP32-S3 Based Multi-Mode Ambulance Light and Siren Simulator"
author: "Yakup Eroğlu"
date: "June 2026"
institution: "Recep Tayyip Erdogan University"
studentid: "221401045"
course: "Embedded Systems"
geometry: margin=2.5cm
fontsize: 11pt
toc: true
toc-depth: 2
colorlinks: true
linkcolor: blue
numbersections: true
header-includes:
  - \usepackage{fancyhdr}
  - \usepackage{graphicx}
  - \usepackage{xcolor}
  - \usepackage{listings}
  - \usepackage{booktabs}
  - \definecolor{codebackground}{RGB}{245,245,245}
  - \lstset{backgroundcolor=\color{codebackground}, basicstyle=\ttfamily\small, breaklines=true, frame=single}
  - \pagestyle{fancy}
  - \fancyhead[L]{Embedded Systems Project}
  - \fancyhead[R]{Yakup Eroğlu — 221401045}
  - \fancyfoot[C]{\thepage}
  - \renewcommand{\headrulewidth}{0.4pt}
---

\begin{center}
\vspace*{2cm}
{\LARGE \textbf{Recep Tayyip Erdogan University}}\\[0.5cm]
{\large Faculty of Engineering and Architecture}\\[0.5cm]
{\large Department of Computer Engineering}\\[2cm]
{\Huge \textbf{Ambulance Simulation System}}\\[0.5cm]
{\large ESP32-S3 Based Multi-Mode Ambulance Light and Siren Simulator}\\[2cm]
\begin{tabular}{ll}
\textbf{Student Name:} & Yakup Eroğlu \\[0.3cm]
\textbf{Student Number:} & 221401045 \\[0.3cm]
\textbf{Course:} & Embedded Systems \\[0.3cm]
\textbf{Date:} & June 2026 \\
\end{tabular}
\end{center}

\newpage

# Project Overview

This project is a **multi-mode ambulance light and siren simulation system** developed using the ESP32-S3 microcontroller on the PlatformIO platform with the Arduino framework. The system cycles through four distinct operating modes controlled by a joystick push button, providing realistic ambulance-style LED animations and siren sounds.

## Objectives

- Design and implement a real-time embedded system using the ESP32-S3 microcontroller
- Demonstrate non-blocking, timer-based software architecture using `millis()`
- Control addressable RGB LEDs (WS2812B) with multiple animation patterns
- Generate PWM-based audio tones simulating siren sounds
- Implement reliable button input with hardware debounce algorithm
- Structure a modular, readable codebase following embedded C++ best practices

## System Overview

```
Power On
   |
   v
setup() -> Initialize Hardware -> Startup Animation -> MODE 0 (Off)
   |
   v
loop() [runs continuously] ─────────────────────────────────────┐
   |                                                             |
   ├─ Button pressed? ──Yes──> enterMode(nextMode)              |
   |         |                                                   |
   |        No                                                   |
   |                                                             |
   ├─ MODE 0? ──> Do nothing                                     |
   ├─ MODE 1? ──> updateMode1(now)  [LED + Wee-Woo siren]       |
   ├─ MODE 2? ──> updateMode2(now)  [LED + Double beep]         |
   └─ MODE 3? ──> updateMode3(now)  [LED + Heavy siren] ────────┘
```

---

# Hardware Components

## ESP32-S3-DevKitC-1

The ESP32-S3-DevKitC-1 is the central processing unit of this project. It is a powerful, low-cost development board built around the Espressif ESP32-S3 System-on-Chip (SoC).

| Specification | Value |
|---|---|
| Processor | Xtensa LX7 dual-core, 240 MHz |
| RAM | 320 KB SRAM |
| Flash | 8 MB (Quad SPI) |
| Built-in RGB LED | GPIO48 (WS2812B type) |
| Operating Voltage | 3.3V logic, 5V USB input |
| Wi-Fi | 802.11 b/g/n (not used in this project) |
| Bluetooth | BLE 5.0 (not used in this project) |
| GPIO Count | 45 usable pins |
| ADC | 12-bit, 20 channels |
| PWM (LEDC) | 8 channels, up to 14-bit resolution |
| USB | USB-OTG (programming and serial monitor) |

The ESP32-S3-DevKitC-1 board includes a single **WS2812B NeoPixel LED** connected to GPIO48 from the factory. This LED is controlled using the `neopixelWrite()` function built into the Arduino ESP32 core, with no external library required.

---

## Deneyap Joystick Module

The Deneyap Joystick Module is a two-axis analog joystick with an integrated push button (SW). Only the push button (SW pin) is used in this project; the X and Y analog outputs are wired but not read by the firmware.

| Specification | Value |
|---|---|
| Axes | 2 (X, Y analog) |
| Operating Voltage | 3.3V |
| X / Y Output | Analog (0 to 3.3V) |
| SW (Button) | Digital, active LOW |
| Internal Pull-up | None (ESP32 internal pull-up used) |

---

## Deneyap Speaker Module

The Deneyap Speaker Module integrates a small speaker and amplifier on a single board. It is driven by a PWM signal from the ESP32-S3.

| Specification | Value |
|---|---|
| Pins | GND, SD (shutdown/enable), IN+ (signal input) |
| Operating Voltage | 3.3V |
| Input Signal | PWM or analog audio |
| SD Pin | HIGH = active, LOW = shutdown |
| IN+ Pin | PWM signal input |

The SD (shutdown) pin is actively controlled in firmware. When silence is required (MODE 0), SD is driven LOW to completely mute the amplifier, eliminating idle noise and reducing power consumption.

---

## WS2812B NeoPixel LED Ring (8 LEDs)

| Specification | Value |
|---|---|
| LED Count | 8 |
| Protocol | WS2812B (single-wire, 800 KHz) |
| Color Depth | 24-bit RGB (8-bit per channel) |
| Operating Voltage | 5V (signal is 3.3V tolerant) |
| Maximum Current | ~480 mA (all LEDs at full white) |
| Library | Adafruit NeoPixel |

> **Important:** The VCC pin of the WS2812B ring requires 5V. Connect it to the 5V pin of the ESP32-S3 board (sourced from USB). The data signal (DIN) is 3.3V tolerant and connects directly to a GPIO pin.

---

# Circuit Connections

## Full Pin Assignment Table

| Module | Module Pin | ESP32-S3 GPIO | Mode | Description |
|---|---|---|---|---|
| **Joystick** | 3V3 | 3V3 | Power | Supply voltage |
| **Joystick** | GND | GND | Ground | Common ground |
| **Joystick** | SW | GPIO3 | INPUT_PULLUP | Push button |
| **Joystick** | X | GPIO1 | ADC (unused) | X-axis analog |
| **Joystick** | Y | GPIO2 | ADC (unused) | Y-axis analog |
| **Joystick** | RES | — | — | Not connected |
| **Speaker** | GND | GND | Ground | Common ground |
| **Speaker** | SD | GPIO4 | OUTPUT | Enable/shutdown |
| **Speaker** | IN+ | GPIO5 | PWM CH0 | Audio signal |
| **LED Ring** | GND | GND | Ground | Common ground |
| **LED Ring** | VCC | 5V | Power | 5V supply |
| **LED Ring** | DIN | GPIO6 | NeoPixel | Data signal |
| **Built-in RGB** | — | GPIO48 | Output | On-board LED |

## Wiring Diagram (ASCII)

```
                    ESP32-S3-DevKitC-1
          ┌──────────────────────────────────────┐
          │  3V3 ──────────────────── Joystick 3V3
          │  GND ──────────────────── Joystick GND
          │  GPIO1 ─────────────────── Joystick X
          │  GPIO2 ─────────────────── Joystick Y
          │  GPIO3 ──[INPUT_PULLUP]─── Joystick SW
          │                                      │
          │  GPIO4 ─────────────────── Speaker SD
          │  GPIO5 ──[PWM CH0]─────── Speaker IN+
          │  GND ──────────────────── Speaker GND
          │                                      │
          │  GPIO6 ─────────────────── LED Ring DIN
          │  5V ───────────────────── LED Ring VCC
          │  GND ──────────────────── LED Ring GND
          │                                      │
          │  GPIO48 ──[Built-in RGB] ────────────│
          └──────────────────────────────────────┘
                          │
                         USB
                          │
                         PC
                  (PlatformIO / Serial Monitor)
```

## Key Electrical Notes

- **GPIO3 (SW pin):** Configured as `INPUT_PULLUP`. Reads `LOW` when button is pressed, `HIGH` when released.
- **GPIO4 (SD pin):** Configured as `OUTPUT`. Driven `LOW` in MODE 0 to fully shut down the speaker amplifier.
- **GPIO5 (PWM):** Controlled via LEDC channel 0. `ledcWriteTone()` generates a square wave at the specified frequency.
- **LED Ring VCC:** Must be connected to 5V. Using 3.3V causes insufficient brightness and color distortion.

---

# Software Architecture

## File Structure

```
HeartRate_Check_Embedded/
├── src/
│   └── main.cpp              <- Main source code (this project)
├── platformio.ini            <- PlatformIO configuration
├── DOCUMENTATION.md          <- Project documentation (English)
├── DOKUMANTASYON.md          <- Project documentation (Turkish)
├── photos/                   <- Hardware photographs
├── heart_rate_monitor/
│   └── heart_rate_monitor.ino  <- Previous project (Arduino IDE)
└── micropython/
    ├── main.py               <- MicroPython version (previous)
    ├── max30102.py           <- MAX30102 MicroPython driver
    └── heartrate.py          <- Heart rate detector (MicroPython)
```

## Global Variables and Their Roles

| Variable | Type | Role |
|---|---|---|
| `currentMode` | `int` | Active mode (0–3) |
| `lastBtnState` | `bool` | Previous raw button reading |
| `debouncedBtn` | `bool` | Stable debounced button state |
| `lastDebounce` | `unsigned long` | Debounce timer (ms) |
| `lastLedUpdate` | `unsigned long` | LED update timer (ms) |
| `flashState` | `bool` | LED color phase (blue/red alternation) |
| `lastSoundUpdate` | `unsigned long` | Sound update timer (ms) |
| `currentFreq` | `int` | Current speaker frequency (Hz) |
| `sirenDir` | `int` | Sweep direction (+1 rising, -1 falling) |
| `bipPhase` | `int` | Mode 2 double-beep phase counter (0–3) |
| `bipTimer` | `unsigned long` | Beep phase timer (ms) |
| `blockStart` | `unsigned long` | Mode 3 block timer (ms) |
| `blockRed` | `bool` | Mode 3 color block (true = red) |

---

# Operating Modes

The system cycles through four modes with each button press:

```
MODE 0 --> MODE 1 --> MODE 2 --> MODE 3 --> MODE 0 --> ...
  (Off)   (Classic)  (Full Color)  (Slow Flash)
```

## MODE 0 — Off (Standby)

| Feature | Value |
|---|---|
| LED Ring | Fully off |
| Built-in RGB | Off |
| Speaker | Shutdown (SD pin LOW) |
| Power Consumption | Minimum |

The system starts in MODE 0. All outputs are inactive. Only the button input is polled each loop iteration.

---

## MODE 1 — Classic Ambulance

| Feature | Value |
|---|---|
| LED Ring | Left 4 LEDs: Blue / Right 4 LEDs: Red — swaps every 120 ms |
| Built-in RGB | Synchronized blue or red |
| Speaker | 700 Hz ↔ 1200 Hz frequency sweep |
| Sound Character | Wee-Woo (European ambulance style) |

**LED Behavior:**
```
t = 0 ms  : [BLUE  | RED  ]  (LEDs 0-3 blue, LEDs 4-7 red)
t = 120ms : [RED   | BLUE ]  (sides swap)
t = 240ms : [BLUE  | RED  ]  (repeats)
```

**Siren Sweep Calculation:**
- Update interval: 8 ms
- Step size: 14 Hz per update
- 700 → 1200 Hz duration: `(1200 - 700) / 14 × 8 ms ≈ 286 ms`
- Full wee-woo cycle: ≈ 572 ms (~1.75 Hz)

---

## MODE 2 — Full Color Loop

| Feature | Value |
|---|---|
| LED Ring | All 8 LEDs: full BLUE ↔ full RED, every 200 ms |
| Built-in RGB | Synchronized |
| Speaker | Double-beep pattern: 900 Hz + 600 Hz |
| Sound Character | Two-tone "beep-beep" |

**Double-Beep Pattern:**

| Phase | State | Duration |
|---|---|---|
| 0 | 900 Hz tone | 150 ms |
| 1 | Silent | 80 ms |
| 2 | 600 Hz tone | 150 ms |
| 3 | Silent | 400 ms |
| **Total cycle** | — | **780 ms** |

---

## MODE 3 — Slow Flash

| Feature | Value |
|---|---|
| LED Ring | All 8 LEDs: 3 seconds RED → 3 seconds BLUE (hard switch) |
| Built-in RGB | Synchronized red or blue |
| Speaker | 500 Hz ↔ 1000 Hz slow sweep |
| Sound Character | Heavy, slow siren |

**Siren Sweep Calculation:**
- Update interval: 18 ms
- Step size: 6 Hz per update
- 500 → 1000 Hz duration: `(1000 - 500) / 6 × 18 ms = 1500 ms`
- Full sweep cycle: ≈ 3000 ms (3 seconds, matching LED block duration)

---

# Function Reference

## `setRGB(r, g, b)`

Controls the built-in WS2812B RGB LED on GPIO48 using the `neopixelWrite()` function built into the Arduino ESP32 core. No external library is required.

## `speakerOn(freq)`

Sets the PWM frequency on LEDC channel 0 using `ledcWriteTone()`. A square wave at 50% duty cycle is output on GPIO5. A minimum frequency guard of 20 Hz prevents potential speaker damage from infrasonic signals.

## `speakerOff()`

Sets LEDC channel duty to zero via `ledcWrite(PWM_CHANNEL, 0)`. This silences the speaker output while keeping the SD pin state unchanged.

## `allLed(r, g, b)`

Sets all 8 NeoPixel LEDs to the same color using `ring.fill()`, then commits the update to the physical LEDs with `ring.show()`.

## `ledOff()`

Clears the NeoPixel ring (`ring.clear()` + `ring.show()`) and turns off the built-in RGB LED. Called on entry to MODE 0.

## `enterMode(mode)`

Manages all mode transitions. Resets every timing and state variable before activating the new mode to ensure a clean start. Enables or disables the speaker SD pin based on the target mode.

**Variables reset on each mode entry:**

| Variable | Reset Value |
|---|---|
| `lastLedUpdate`, `lastSoundUpdate` | `millis()` (current time) |
| `flashState` | `false` |
| `currentFreq` | 700 Hz |
| `sirenDir` | +1 (rising) |
| `bipPhase` | 0 |
| `bipTimer`, `blockStart` | `millis()` |
| `blockRed` | `true` (red block first) |

## `updateMode1(now)`

Runs MODE 1 logic each loop iteration. Contains two independent timer checks:
- LED update: every 120 ms
- Sound update: every 8 ms

No blocking `delay()` calls are used.

## `updateMode2(now)`

Runs MODE 2 logic. LED alternates every 200 ms. Sound is managed by a 4-phase state machine with individual duration values per phase.

## `updateMode3(now)`

Runs MODE 3 logic. LED block changes are tracked with `blockStart` over 3000 ms. A `static` local variable (`lastBlockRed`) ensures `allLed()` is only called when the color actually changes, avoiding redundant writes each loop.

## `readButtonPressed()`

Returns `true` only once per physical button press (on the HIGH→LOW transition after debounce). Holding the button does not produce repeated events.

## `setup()`

Arduino framework initialization function, runs once:

1. Serial communication at 115200 baud
2. GPIO pin mode configuration
3. LEDC PWM channel setup
4. NeoPixel ring initialization at 51% brightness
5. Built-in RGB LED initialization
6. Startup animation (blue sweep → red sweep)
7. System enters MODE 0

## `loop()`

Arduino framework main loop, runs indefinitely:

1. Capture `millis()` as `now`
2. Check for button press via `readButtonPressed()`
3. Dispatch to `updateModeX(now)` based on `currentMode`
4. `delay(1)` to yield CPU for 1 ms

---

# Timing and Non-Blocking Design

## Why `delay()` Is Avoided

The `delay()` function halts **all program execution** for its duration. In this project, LEDs, sound, and button input must all be managed simultaneously. Using `delay()` would cause:

- Missed button presses during LED or sound delays
- LED animation halting while sound is updated
- Unresponsive system behavior

Instead, **non-blocking millis()-based timing** is used throughout:

```cpp
// WRONG — button cannot be read during delay:
ring.fill(blue);
ring.show();
delay(120);

// CORRECT — all tasks continue running:
if (now - lastLedUpdate >= 120) {
    lastLedUpdate = now;
    // Update LED
}
```

## Timing Summary Table

| Task | Interval | Timer Variable |
|---|---|---|
| Mode 1 LED flash | 120 ms | `lastLedUpdate` |
| Mode 1 siren sweep | 8 ms | `lastSoundUpdate` |
| Mode 2 LED flash | 200 ms | `lastLedUpdate` |
| Mode 2 beep phase | 80–400 ms | `bipTimer` |
| Mode 3 LED block | 3000 ms | `blockStart` |
| Mode 3 siren sweep | 18 ms | `lastSoundUpdate` |
| Button debounce | 50 ms | `lastDebounce` |

---

# PWM Audio Generation

## LEDC Module

The ESP32-S3's LEDC (LED Control) peripheral is used to generate audio tones without an external DAC or I2S module. LEDC generates a square wave on a GPIO pin at a configurable frequency and duty cycle.

**Initialization:**
```cpp
#define PWM_CHANNEL    0    // LEDC channel (0-7)
#define PWM_RESOLUTION 8    // 8-bit resolution (0-255 duty range)

ledcSetup(PWM_CHANNEL, 2000, PWM_RESOLUTION);
ledcAttachPin(SPEAKER_PWM_PIN, PWM_CHANNEL);
```

**Tone generation:**
```cpp
ledcWriteTone(PWM_CHANNEL, 880);  // Output 880 Hz square wave
```

**Silence:**
```cpp
ledcWrite(PWM_CHANNEL, 0);  // Duty cycle = 0, no output
```

> **Note:** This project uses the **legacy LEDC API** (`ledcSetup` + `ledcAttachPin`) because the installed platform version (`espressif32 @ 7.0.1`) does not include the newer unified `ledcAttachChannel()` function.

## Siren Sweep Algorithm

```cpp
currentFreq += sirenDir * STEP_SIZE;

if (currentFreq >= MAX_FREQ) {
    currentFreq = MAX_FREQ;
    sirenDir = -1;   // Start descending
}
if (currentFreq <= MIN_FREQ) {
    currentFreq = MIN_FREQ;
    sirenDir = +1;   // Start ascending
}
speakerOn(currentFreq);
```

## Audio Profile Comparison

| Mode | Min (Hz) | Max (Hz) | Step (Hz) | Interval (ms) | Character |
|---|---|---|---|---|---|
| Mode 1 | 700 | 1200 | 14 | 8 | Fast wee-woo |
| Mode 3 | 500 | 1000 | 6 | 18 | Slow, heavy siren |

## Sound Quality Considerations

PWM-based audio outputs a **square wave**, not a sine wave. This introduces harmonic distortion — the sound contains multiples of the fundamental frequency. For siren simulation, this characteristic actually enhances the perceived harshness of the sound, making it more realistic. For high-fidelity audio, an I2S DAC module (e.g., MAX98357A) would be required.

---

# NeoPixel LED Control

## WS2812B Protocol

WS2812B LEDs communicate over a **single data wire** at 800 KHz. Each LED receives 24 bits (8 bits each for R, G, B). Data is transmitted serially through the chain — the first LED reads the first 24 bits and passes remaining data downstream.

## Adafruit NeoPixel Library Usage

```cpp
// Initialize
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ring.begin();
ring.setBrightness(130);   // ~51% of maximum

// Set individual LED
ring.setPixelColor(index, ring.Color(r, g, b));

// Fill all LEDs with one color
ring.fill(ring.Color(255, 0, 0));

// Clear all LEDs
ring.clear();

// Commit changes to physical LEDs (REQUIRED)
ring.show();
```

> **Critical:** `ring.show()` must always be called after modifying LED values. Changes remain in the buffer until `show()` is called.

## Brightness and Power Budget

`ring.setBrightness(130)` sets brightness to approximately 51% of maximum.

| Scenario | Current Draw |
|---|---|
| All LEDs off | ~1 mA |
| All LEDs full white (255,255,255) | ~480 mA |
| All LEDs at 51% brightness | ~245 mA |
| Typical animation (mixed colors) | ~50–120 mA |

At typical operating current, the 5V USB supply (500 mA limit for USB 2.0) is sufficient.

---

# Button Debounce

## The Bounce Problem

Mechanical buttons produce rapid, noisy transitions when pressed or released — a phenomenon called **contact bounce**. A single press can generate dozens of spurious HIGH/LOW transitions within the first 1–50 ms. Without debounce, each physical press would register as multiple events.

## Implemented Algorithm: Millis-Based Debounce

```cpp
bool readButtonPressed() {
    bool reading = digitalRead(JOY_SW_PIN);

    // If signal changed, reset the debounce timer
    if (reading != lastBtnState) {
        lastDebounce = millis();
        lastBtnState = reading;
    }

    // If signal has been stable for > 50ms, accept it
    if ((millis() - lastDebounce) > DEBOUNCE_MS) {
        if (reading != debouncedBtn) {
            debouncedBtn = reading;
            if (debouncedBtn == LOW) {
                return true;  // Valid press detected!
            }
        }
    }
    return false;
}
```

## Algorithm Explanation

1. The raw GPIO value is read every loop iteration.
2. If the reading differs from the previous raw value, the debounce timer is reset.
3. If the signal remains stable for more than 50 ms, it is accepted as the new stable state.
4. If the stable state transitions from HIGH to LOW (button pressed), the function returns `true` once.
5. Holding the button does **not** generate repeated `true` returns.

The 50 ms window is selected to filter typical mechanical bounce (1–20 ms) while remaining imperceptible to users.

---

# Development Environment

## Required Software

| Software | Version | Purpose |
|---|---|---|
| Visual Studio Code | Any | Code editor |
| PlatformIO IDE Extension | Any | Build system |
| Espressif32 Platform | 7.0.1 | ESP32 toolchain |
| Arduino Framework | 3.20017.241212 | Arduino compatibility layer |
| Adafruit NeoPixel Library | 1.15.5 | WS2812B driver |

## Setup Steps

1. Install **Visual Studio Code** from [code.visualstudio.com](https://code.visualstudio.com)
2. Install the **PlatformIO IDE** extension from the VS Code Extensions marketplace
3. Open the project folder: `File → Open Folder → HeartRate_Check_Embedded/`
4. All libraries are downloaded automatically on first build
5. **Upload firmware:** PlatformIO: Upload button or `Ctrl+Alt+U`
6. **Open Serial Monitor:** PlatformIO: Serial Monitor or `Ctrl+Alt+S` (115200 baud)

## platformio.ini Configuration

```ini
[env:esp32-s3-devkitc-1]
platform  = espressif32          ; Espressif ESP32 platform package
board     = esp32-s3-devkitc-1  ; Target board definition
framework = arduino              ; Arduino compatibility layer

lib_deps =
    adafruit/Adafruit NeoPixel @ ^1.12.3

monitor_speed = 115200           ; Serial monitor baud rate
upload_speed  = 921600           ; Flash write speed
```

---

# Serial Monitor Output

On startup (115200 baud):

```
╔══════════════════════════════════════╗
║   Ambulance v2 — 3 Modes + Off      ║
╠══════════════════════════════════════╣
║  Button: MODE 0→1→2→3→0→...         ║
║  MODE 0: Off                         ║
║  MODE 1: Classic ambulance           ║
║  MODE 2: Full blue-red + double beep ║
║  MODE 3: 3s blocks + heavy siren     ║
╚══════════════════════════════════════╝

Ready! MODE 0 (Off). Press button.
```

On mode transitions:

```
MODE 1 active
MODE 2 active
MODE 3 active
  -> RED
  -> BLUE
  -> RED
MODE 0 — Off
```

---

# Known Limitations

## LEDC API Version

The installed Arduino ESP32 framework uses the **legacy LEDC API**. If the platform is updated to version 3.x or later, the following changes will be required:

| Legacy API (currently used) | New API (3.x+) |
|---|---|
| `ledcSetup(ch, freq, res)` | Removed |
| `ledcAttachPin(pin, ch)` | Removed |
| — | `ledcAttachChannel(pin, freq, res, ch)` |

## Joystick Axes Not Used

GPIO1 and GPIO2 are physically wired but not read by the firmware. They are reserved for future enhancements.

## Square Wave Audio

PWM audio produces harmonic distortion. For professional audio quality, an I2S DAC (e.g., MAX98357A) with sine wave synthesis would be required.

---

# Future Development

## Short Term

- [ ] **Joystick X/Y reading** — control siren speed or LED brightness
- [ ] **Mode 4:** Rainbow LED animation across the full color spectrum
- [ ] **Variable brightness** — adjust using joystick Y axis

## Medium Term

- [ ] **OLED display** (SSD1306) — show active mode number and frequency
- [ ] **Bluetooth BLE** — control modes from a smartphone app
- [ ] **I2S audio** (MAX98357A + sine wave lookup table) — higher quality siren

## Long Term

- [ ] **MAX30102 integration** — combine ambulance siren with heart rate monitor
- [ ] **Web interface** — ESP32 Wi-Fi + AsyncWebServer for browser control
- [ ] **Battery power** + deep sleep for portable operation

---

# Quick Reference Card

```
┌────────────────────────────────────────────────────────┐
│          AMBULANCE SYSTEM — QUICK REFERENCE            │
├──────────────────┬─────────────────────────────────────┤
│  BUTTON          │  GPIO3  (INPUT_PULLUP)              │
│  SPEAKER ENABLE  │  GPIO4  (OUTPUT)                    │
│  SPEAKER SIGNAL  │  GPIO5  (PWM CH0)                   │
│  LED DATA        │  GPIO6  (NeoPixel)                  │
│  BUILT-IN RGB    │  GPIO48                             │
├──────────────────┴─────────────────────────────────────┤
│  MODE 0  │  Off — all outputs inactive                 │
│  MODE 1  │  Split blue/red fast flash + wee-woo        │
│  MODE 2  │  Full blue/red alternating + double beep    │
│  MODE 3  │  3-second color blocks + heavy siren        │
├────────────────────────────────────────────────────────┤
│  Upload  :  platformio run --target upload             │
│  Monitor :  platformio device monitor                  │
│  Baud    :  115200                                     │
└────────────────────────────────────────────────────────┘
```

---

*Documentation prepared for the Embedded Systems course.*  
*Recep Tayyip Erdogan University — June 2026.*  
*ESP32-S3-DevKitC-1 | PlatformIO | Arduino Framework | Adafruit NeoPixel*
