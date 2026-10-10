# Decade Time Piece: Power, Pins and Accuracy Plan (draft 2)

Builds on [original-design-notes.md](original-design-notes.md). Decided 2026-10-10: **3D-printed case, designed around the PCB.**
Draft 2 (2026-10-10) brings this in line with the Power, RTC and MCU schematic sheets. Items marked **(verify)** still need a datasheet check.

## 1. Board outline and size (compactness is the priority)
Confirmed by Alex 2026-10-10: round board, TPS61099 boost, keep NFC. **Size is the top priority.**

**The display sets the diameter, not the board.** The LS013B7DH03 outline is **26.6 × 30.3 × 0.746 mm** (Mouser listing), and the 30.3 mm side includes the glass ledge where the FPC leaves. A rectangle that size needs a circle of at least **40.3 mm** (its diagonal), so:
- Case inner diameter ~41 mm, outer ~**44 mm** with 1.5 mm printed walls. That is still small for a pocket watch (typical 45–50 mm).
- Since the diameter is fixed anyway, the PCB uses it: **40 mm round, 0.8 mm thick**. The front under the LCD carries only the FPC connector and LCD buffers (parts ≤ 1.1 mm high). Most parts sit on the back in the ring around the cell. The LCD's FPC folds 180° under the glass into a front-side connector, and the LCD buffers sit beside it, under the panel, which rests on a 1.2 mm frame (see decisions.md).
- **The real compactness lever is thickness**, mostly set by the cell.

| Layer | CR2032 build (mm) |
|---|---|
| Lid + window | ~1.5 |
| LCD | 0.75 |
| PCB | 0.8 |
| Cell in Keystone 3034 SMT retainer | 4.06 (DigiKey listing) |
| Back wall | ~1.2 |
| **Total (estimate)** | **~8.4** |

**Decided 2026-10-10: CR2032** (Alex). The Keystone 3034 retainer came out ~0.4 mm taller than the ~3.7 mm first assumed, so the total is ~8.4 mm rather than ~8 mm.

Cell budget: ~1 µA average ≈ 9 mAh/yr; 10 years plus ~1%/yr self-discharge ≈ 110 mAh. A CR2032 (~225 mAh) gives about 2× margin at ten years.

A smaller display is the only way to shrink the diameter. Sharp's 1.28" is the smallest square memory LCD it sells, and round watch-style panels (e.g. JDI MIP) are hard to buy in small quantities. The LS013B7DH03 is in production (Octopart, ~33,700 at DigiKey on 2026-10-10).

## 2. Power tree (as drawn)
```
CR2032 (BT1) ─ Q1 DMG2305UX P-FET reverse protection ─ VBAT (always on, 2.0–3.2 V over life)
   │   VBAT decoupling: 100 nF + 1 µF (power sheet), 100 nF (RTC), 3 × 100 nF + 1 µF (MCU)
   ├─ RV-3032-C7 VDD                (~160 nA typ, 210 nA max at 25 °C); VBACKUP via 10k to VSS
   ├─ STM32L031G6U6 VDD/VDDA        (stop mode, sub-µA)
   ├─ CT8132 lid sensor             (190 nA typ, 900 nA max)
   ├─ Button pull-ups               (1 MΩ each, draw only while pressed)
   ├─ I2C pull-ups 10k, INT 470k    (draw only while a line is low)
   ├─ ST25DV04K VCC                 (from MCU PB0, only while awake)
   └─ U5 TPS22916C load switch, ON = LCD_PWR_EN (ISD 10 nA typ, 100 nA max)
        └─ R4 470 Ω inrush limit ─ VBST_IN: C2 100 µF + C4 10 µF
             └─ U3 TPS61099 adjustable, EN = BST_EN (R5 1M / C8 470 nF delay) ─ VLCD 3.0 V ─ LS013B7DH03
```
- **Load switch confirmed by Alex.** The TPS61099 draws 0.5 µA typ / 1.6 µA max into VIN while disabled, and the 100 µF bulk's worst-case IR leakage (Samsung: 50 MΩ·µF, i.e. ~6 µA at 3 V) is far above the budget. The TPS22916C takes both off the cell while the lid is shut.
- **Inrush resistor.** The TPS22916C's own slew (~1–3 mV/µs) would draw 220–350 mA into 110 µF, which a CR2032 can't supply without dragging VBAT down and resetting the MCU. R4 (470 Ω) limits it to ~6.6 mA (τ ≈ 52 ms).
- **Boost EN delay.** R5/C8 (τ = 0.47 s) hold the boost off until C2 is charged: EN reaches 1.2 V after ~0.24 s (LCD_PWR_EN at 3.0 V) to ~0.37 s (2.2 V). The display therefore powers up ~0.3 s after the lid opens. A spare MCU pin driving EN directly would remove R5/C8 and let firmware set the delay.
- Each lid open costs ~330 µC (charging C2 + C4): ~0.7 mAh/yr at 20 opens a day.
- VOUT = 1.00 V × (1 + 402k/200k) = 3.01 V (≈3.10 V in PFM). When the cell is above ~3.0 V the boost passes VBAT through, so VLCD can reach ~3.2 V.

### LCD electrical limits (Sharp spec LCP-1112045)
- VDD and VDDA: 2.7 / 3.0 / 3.3 V (min/typ/max); absolute max 3.6 V. Logic inputs must not exceed VDD.
- **VIH = VDD − 0.1 V min, VIL = VSS + 0.1 V max** on SCLK, SI, SCS, DISP and EXTCOMIN.
- **LCD buffers are needed.** The MCU runs from VBAT (down to ~2.2 V) and can't meet VIH at VLCD = 3.0–3.1 V. All five lines, including RTC CLKOUT to EXTCOMIN, go through single-gate buffers powered from VLCD with Ioff (e.g. 74AUP1G/74LVC1G), which also keep inputs from sitting above VDD while VLCD is off.
- Power: 12 µW typ (50 max) static; 50 µW typ (130 max) with 1 Hz updates. Sharp recommends ≥1 µF on VDD and VDDA, 0.1 µF on DISP.

## 3. RTC does the display housekeeping
- The RV-3032 CLKOUT can run at **1 Hz**. Wire it to the LCD's **EXTCOMIN** (with EXTMODE tied high) so the polarity toggle needs no MCU wakes. CLKOUT also has a test pad (TP1) for accuracy measurement.
- Factory default CLKOUT is 32.768 kHz (~1 µA into 10 pF); firmware must set FD = 1 Hz (EEPROM C3h) or NCLKE = 1 (EEPROM C0h) once.
- The RV-3032 periodic interrupt wakes the MCU once a minute (or once a second if a seconds field is shown) to redraw. **A minutes-only display is the cheap default.**

### Cell-swap backup: dropped (confirmed by Alex 2026-10-10)
- VBACKUP is tied to VSS through 10k and switchover is off (BSM = 00, factory default).
- A ceramic bridge would need 36 µF (270 nA, 25 °C) to 100 µF (750 nA, −40 to 85 °C) to hold 60 s from 1.75 V down to VLOW 1.3 V. But an MLCC's IR spec (50 MΩ·µF) is a 50 s self-discharge time constant at any size, so none can be guaranteed to bridge 60 s, and worst-case standby leakage would be 2–5.5 µA.
- Without it, a cell swap (about once a decade) resets the time; NFC sets it again. The aging offset (EEPROM C1h) and the rest of the configuration EEPROM survive.

## 4. Lid sensor
- An NC reed would draw nothing with the lid shut, but SMD normally-closed reeds are Form C parts about 27 mm long (Littelfuse MDSM-DT), which don't fit the ring around the cell, and the small SMD reeds (Coto CT05/CT10) are normally-open only.
- **As drawn: Allegro CT8132SK-IS3**, an omnipolar TMR switch (CT813x datasheet Rev. 10): push-pull output, low while the lid magnet is near, 9 G operate point, 10 Hz sampling, **190 nA typ / 900 nA max** average at 3.3 V (145 / 700 nA at 1.8 V), 1 µF bypass. No pull-up needed. The Coto RedRock RR122 was the first choice but is obsolete (RR1yy EOL 2025); DigiKey lists the CT8132SK-IS3 as its direct substitute.
- Budget impact: ~0.19 µA typical out of the ~1 µA average, so the sensor is now the second-largest standing load after the RTC.
- Buttons use 1 MΩ external pull-ups (internal pull-ups off), so a button held down in a pocket costs 3 µA rather than ~75 µA.

## 5. MCU pin map (STM32L031G6U6, UFQFPN28, standard pinout)
The UFQFPN28 package has 21 I/O, 19 once SWD is reserved (draft 1's "~25" was wrong). Wake sources use distinct EXTI lines; any EXTI line wakes Stop mode.

| Pin | Port | Function | Notes |
|---|---|---|---|
| 6 | PA0 | RTC_INT | EXTI0, WKUP1 |
| 7 | PA1 | LCD_PWR_EN | to TPS22916C ON and boost EN delay |
| 8 | PA2 | LID_CLOSED_N | EXTI2, WKUP3; CT8132 push-pull output, no pull-up |
| 9 | PA3 | BTN1_N | EXTI3; 1M pull-up |
| 10 | PA4 | LCD_SCS | GPIO (SCS is active high) |
| 11 | PA5 | LCD_SCLK | SPI1_SCK, AF0 |
| 12 | PA6 | LCD_DISP | GPIO |
| 13 | PA7 | LCD_SI | SPI1_MOSI, AF0 |
| 18 | PA8 | NFC_GPO | EXTI8 |
| 21 | PA13 | SWDIO | TC2030 |
| 22 | PA14 | SWCLK | TC2030 |
| 14 | PB0 | NFC_VCC | ST25DV04K supply |
| 15 | PB1 | BTN2_N | EXTI1; 1M pull-up |
| 19 | PA9 | SCL | I2C1_SCL (moved from PB6 for routing: faces the RTC/NFC side) |
| 20 | PA10 | SDA | I2C1_SDA (moved from PB7) |
| 4 | NRST | NRST | 100 nF, TC2030 |
| 27 | BOOT0 | – | 10k to GND |

**Spare:** PB6, PB7 (USART2 TX/RX, good for a debug UART), PA15, PB3, PC14, PC15.
- Battery voltage is measured on VREFINT internally, so no pin is needed.
- Decoupling per ST: 100 nF on each VDD pin, 100 nF + 1 µF on VDDA. ST's 10 µF VDD bulk (C14) is fitted as **DNP**: its IR spec allows ~0.6 µA leakage on always-on VBAT. Fit it only if wake bursts sag VBAT too far.

## 6. Accuracy budget over ten years
1 ppm ≈ 31.5 s/yr. The RV-3032-C7 datasheet gives ±1.5 ppm from 0 to 50 °C (±2.5 ppm from −40 to 85 °C) and **aging of ±3 ppm max in the first year**.

| Scenario | Rough error |
|---|---|
| No calibration, first year | up to ~4.5 ppm, about 2.4 min/yr worst case |
| One NTP calibration at build time | initial offset removed; aging dominates, ≤1.5 min in year 1, then less as aging slows **(estimate)** |
| Recalibrate once a year | within about ±1 ppm, about 30 s/yr **(estimate)** |

- **Aging, not temperature, is the main error over a decade.** A body-warm pocket watch stays near the RTC's best temperature region.
- **Keep the NFC tag.** A phone writes the true time, the MCU compares it on the next lid open, then sets the time and updates the offset register. A once-a-year recalibration then takes a few seconds and no cable.
- Firmware can also log successive corrections and pre-apply the aging trend, so drift keeps shrinking between recalibrations.

## 7. Next steps
1. LCD sheet: FPC connector, VLCD-powered input buffers on SCLK/SI/SCS/DISP/EXTCOMIN, Sharp's recommended capacitors.
2. NFC sheet: ST25DV04K, GPO pull-up, and how its I2C pins behave while NFC_VCC is off **(verify)**.
3. Layout: placement done on the "layout" branch; routing next.
4. Then the case CAD around the finished board outline.
