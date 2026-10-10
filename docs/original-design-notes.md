# Digital Pocket Watch — Design Notes

## Goal
Single compact PCB, coin-cell powered, accurate timekeeping without GPS, ~10-year life. Display wakes only when the lid is opened.

## Architecture
- RTC does all timekeeping; MCU stays in deep sleep except while the lid is open.
- Opening the lid → reed switch interrupt → MCU wakes → powers display via load switch → shows time.
- Lid closed (or 30–60 s firmware timeout) → display off, MCU back to stop mode.

## Core BOM
| Function | Part | Notes |
|---|---|---|
| Timekeeping | Micro Crystal RV-3032-C7 | DTCXO, ±1.5 ppm 0–50 °C (~0.13 s/day), ~160 nA, backup switchover, offset register |
| Display | Sharp LS013B7DH03 (1.28", 128×128 Memory LCD) | Microwatts, sunlight-readable, reflective (no glow) |
| MCU | STM32L0 (L010/L031) or MSP430FR2433 | Sub-µA stop, SPI (LCD) + I²C (RTC) |
| Lid sensor | Form B (normally-closed) reed switch + magnet in lid | Zero standby current (magnet holds it open when lid shut) |
| Display power gate | TI TPS22916 load switch | ~10 nA leakage |
| Battery | CR2450 + SMT holder | ~620 mAh, better pulse headroom than CR2032 |
| Swap bridge | 47–100 µF low-leakage MLCC/tantalum on RTC backup pin | Keeps time during cell change (avoid leaky supercaps) |
| Reverse protection | P-FET ideal-diode | No drop |
| Time setting | 2× tactile switches | — |
| Optional | ST25DV04K NFC tag | Set time from phone, no idle draw |
| Programming | Tag-Connect pads | — |

## Power budget
- Average draw ~1 µA (RTC + sleeping MCU + brief display bursts) ≈ 9 mAh/yr.
- On paper a CR2450 lasts decades; realistically ~10–15 yrs, limited by cell self-discharge (~1%/yr), shelf life, and leakage risk.
- Plan: ~decade per cell, then a quick swap with time preserved by the backup cap.

## Alternative displays (lid-open only, 20 checks/day × 10 s, CR2032)
| Display | Current on | Rough life |
|---|---|---|
| Sharp Memory LCD | ~10s of µA | 10+ yrs |
| LED bubble, 4 digits | ~5 mA | ~2 yrs |
| Small OLED, digits only | ~5–15 mA | ~8–24 months |
LED/OLED options need a bulk cap (100–220 µF) and ideally a CR2450 (~3× life).

## Backup cap sizing
C = I·t/ΔV → 200 nA × 60 s / 1.7 V ≈ 7 µF. 47–100 µF gives several minutes. Supercap leakage (µA) can exceed RTC draw — avoid.

## Gotchas
- LCD rated ~3 V — confirm range; may need a small boost enabled only while lid open as the cell sags.
- Memory LCD needs ~1 Hz EXTCOMIN polarity toggle while powered (MCU generates it).
- I²C lines idle high; make sure MCU doesn't drive them low in stop mode.
- Calibrate drift vs NTP over a few weeks, then write correction to RV-3032 offset register (EEPROM).

## Open decisions / next steps
- Case choice (repurposed hunter-case vs 3D-printed) → sets PCB outline and display size.
- Schematic block by block.
