# Decade Time Piece: Power, Pins and Accuracy Plan (draft 1)

Builds on [original-design-notes.md](original-design-notes.md). Decided 2026-10-10: **3D-printed case, designed around the PCB.**
Items marked **(verify)** come from memory, not a datasheet, and should be checked before the schematic is final.

## 1. Board outline and size (compactness is the priority)
Confirmed by Alex 2026-10-10: round board, TPS61099 boost, keep NFC. **Size is the top priority.**

**The display sets the diameter, not the board.** The LS013B7DH03 outline is **26.6 × 30.3 × 0.746 mm** (Mouser listing), and the 30.3 mm side includes the glass ledge where the FPC leaves. A rectangle that size needs a circle of at least **40.3 mm** (its diagonal), so:
- Case inner diameter ~41 mm, outer ~**44 mm** with 1.5 mm printed walls. That is still small for a pocket watch (typical 45–50 mm).
- Since the diameter is fixed anyway, the PCB uses it: **~40 mm round, 0.8 mm thick**. The front is kept bare under the LCD (foam tape mount). All parts sit on the back in the ring around the cell, and the FPC folds round the edge to a back-side connector.
- **The real compactness lever is thickness**, mostly set by the cell.

| Layer | CR2450 build | CR2032 build |
|---|---|---|
| Lid + window | ~1.5 | ~1.5 |
| LCD | 0.75 | 0.75 |
| PCB | 0.8 | 0.8 |
| Cell in low-profile holder | ~5.5 | ~3.7 |
| Back wall | ~1.2 | ~1.2 |
| **Total (mm, estimate)** | **~10** | **~8** |

**Decided 2026-10-10: CR2032** (Alex), for ~8 mm total thickness.

Cell trade-off (budget ~1 µA average ≈ 9 mAh/yr; 10 years plus ~1%/yr self-discharge ≈ 110 mAh):
- **CR2032 (~225 mAh):** about 2× margin at ten years. It's ~1.8 mm thinner, and the boost converter takes care of its higher internal resistance late in life.
- **CR2450 (~620 mAh):** about 5× margin, at the cost of ~1.8 mm more thickness.

A smaller display is the only way to shrink the diameter. Sharp's 1.28" is the smallest square memory LCD it sells, and round watch-style panels (e.g. JDI MIP) are hard to buy in small quantities. Worth a check later, not a blocker.

## 2. Power tree
```
CR2032 ─ P-FET reverse protection ─ VBAT (always on, 2.0–3.2 V over life)
   ├─ RV-3032-C7 VDD        (~160 nA typ, 210 nA max)
   │    └─ VBACKUP ── 47–100 µF X5R MLCC (cell-swap bridge, charged by RV-3032 trickle charger)
   ├─ STM32L031 VDD         (stop mode, sub-µA)
   ├─ Reed switch pull-up   (external 1 MΩ, see §4)
   ├─ ST25DV04K VCC         (fed from an MCU GPIO only while awake; the phone side works off RF power)
   └─ TPS61099 boost, EN from MCU ─ VLCD 3.0 V ─ LS013B7DH03
```
- **Proposed change: the TPS61099 boost replaces the TPS22916 load switch.** It has true disconnect when disabled, so it is also the display power gate. Iq is ~1 µA while running and it starts from 0.7 V.
- Use the **adjustable** TPS61099 with a divider set to 3.0 V. The divider sits on VOUT, which is disconnected in shutdown, so it costs nothing while the lid is closed.
- Why boost: the LCD wants ~3.0 V (2.7–3.3 V range **(verify)**) and a CR2450 sits at ~2.9 V early in life and keeps falling. Without a boost, the display dies years before the cell is empty.
- **Logic-level risk (verify):** once VBAT falls below roughly 2.4–2.5 V, the MCU's SPI high level may fall below the LCD's VIH at 3.0 V. Check the LCD's VIH. If it's a problem, add a small level shifter (e.g. 74AUP/LVC single-gate buffers powered from VLCD) on SCLK/SI/SCS/EXTCOMIN.

## 3. RTC does the display housekeeping
- The RV-3032 CLKOUT can run at **1 Hz**. Wire it to the LCD's **EXTCOMIN** (with EXTMODE tied high) so the polarity toggle needs no MCU wakes.
- The RV-3032 periodic interrupt wakes the MCU once a minute (or once a second if a seconds field is shown) to redraw. **A minutes-only display is the cheap default; seconds are an option to cost later.**
- CLKOUT is enabled only while the lid is open.

## 4. Lid sensor current trap
- With an NC reed switch the contact is open when the lid is shut, so there's no draw. **When the lid is open the switch shorts the pull-up to ground.**
- With the MCU's internal ~40 kΩ pull-up that's ~75 µA while open. At 20 opens a day × 45 s it adds **~7 mAh/yr, nearly doubling the whole budget.**
- Fix: an external **1 MΩ** pull-up (~3 µA while open, negligible), with the internal pull-up disabled.

## 5. MCU and pin budget (STM32L031)
| Function | Pins |
|---|---|
| LCD SPI (SCLK, SI, SCS) + DISP | 4 |
| I²C to RTC and NFC (SCL, SDA) | 2 |
| RTC INT, NFC GPO, reed switch | 3 |
| 2 buttons | 2 |
| Boost EN, NFC VCC | 2 |
| SWD (SWDIO, SWCLK) + NRST | 3 |
| **Total** | **16** |
- Battery voltage is measured on VREFINT internally, so no pin is needed.
- **Recommend STM32L031G6 (UFQFPN28, 4 × 4 mm)**: ~25 I/O, room for spares and debug. The TSSOP20 F6 part fits but leaves almost no spare pins.

## 6. Accuracy budget over ten years
1 ppm ≈ 31.5 s/yr. The RV-3032-C7 datasheet gives ±1.5 ppm from 0 to 50 °C (±2.5 ppm from −40 to 85 °C) and **aging of ±3 ppm max in the first year**.

| Scenario | Rough error |
|---|---|
| No calibration, first year | up to ~4.5 ppm, about 2.4 min/yr worst case |
| One NTP calibration at build time | initial offset removed; aging dominates, ≤1.5 min in year 1, then less as aging slows **(estimate)** |
| Recalibrate once a year | within about ±1 ppm, about 30 s/yr **(estimate)** |

- **Aging, not temperature, is the main error over a decade.** A body-warm pocket watch stays near the RTC's best temperature region.
- **Recommendation: keep the optional NFC tag.** A phone writes the true time, the MCU compares it on the next lid open, then sets the time and updates the offset register. A once-a-year recalibration then takes a few seconds and no cable.
- Firmware can also log successive corrections and pre-apply the aging trend, so drift keeps shrinking between recalibrations.

## 7. Next steps
1. Check the items marked (verify) against the LCD and STM32 datasheets.
2. Lock the outline (~40 mm round, CR2032) and back-side placement sketch.
3. Schematic in KiCad, block by block: power → RTC → MCU → LCD → lid/buttons → NFC.
4. Then the case CAD around the finished board outline.
