# Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-10-10 | RV-3032-C7 TCXO RTC does all timekeeping; MCU sleeps | ±1.5 ppm at ~160 nA, offset register for drift trim |
| 2026-10-10 | Sharp LS013B7DH03 memory LCD, on only while lid is open | Microwatt power, sunlight-readable; in production (~33,700 at DigiKey) |
| 2026-10-10 | 3D-printed case, designed around the PCB | Build around the device, not the reverse |
| 2026-10-10 | Round PCB, optimised for compactness | LCD diagonal (40.3 mm) sets the diameter: 40 mm PCB, ~44 mm case |
| 2026-10-10 | TPS61099 boost at 3.0 V for the LCD | Keeps the LCD in spec as the cell sags |
| 2026-10-10 | TPS22916C load switch ahead of the TPS61099 (confirmed by Alex) | Takes the boost's 0.5–1.6 µA shutdown current and the 100 µF bulk's leakage off the cell while the lid is shut |
| 2026-10-10 | 470 Ω inrush resistor after the load switch; 1M/470 nF delay on the boost EN | The switch's slew alone would pull 220–350 mA from the coin cell; the delay lets the bulk charge before the boost starts (~0.3 s to display power) |
| 2026-10-10 | Keep the NFC tag: ST25DV04KC-IE6C3 (UFDFPN8); the ST25DV04K is NRND | Yearly phone recalibration controls crystal aging; GPO works from RF power, so a phone tap wakes the MCU |
| 2026-10-10 | CR2032 in a Keystone 3034 SMT retainer (4.06 mm) instead of CR2450 | ~8.4 mm total thickness; still ~2× capacity margin at ten years |
| 2026-10-10 | DMG2305UX P-FET reverse protection, gate to GND | Fully on at 1.8 V gate drive (200 mΩ max); off with a reversed cell |
| 2026-10-10 | STM32L031G6U6 (UFQFPN28) as the MCU | 19 usable I/O after SWD; pin map in power-pins-accuracy.md §5 |
| 2026-10-10 | External 1 MΩ pull-ups on the reed switch and both buttons; internal pull-ups off | Internal ~40 kΩ would draw ~75 µA while the lid is open or a button is held |
| 2026-10-10 | LCD inputs go through VLCD-powered buffers | LCD VIH is VDD − 0.1 V; the MCU on VBAT (down to ~2.2 V) can't meet it at VLCD 3.0 V |
| 2026-10-10 | SWD on Tag-Connect TC2030-NL pads | No connector height; smallest footprint |
| 2026-10-10 | Drop the RTC cell-swap bridge: VBACKUP to VSS via 10k, switchover off (confirmed by Alex 2026-10-10) | No MLCC can be guaranteed by its IR spec to bridge a 60 s swap, and worst-case leakage would be 2–5.5 µA; a swap resets the time, NFC restores it, and the aging offset survives in EEPROM |

## Proposed, not yet confirmed
- RV-3032 CLKOUT at 1 Hz drives the LCD EXTCOMIN (through a buffer)
- ST25DV04K VCC switched from an MCU GPIO (PB0 on the schematic)
- ST's 10 µF VDD bulk (C14) left unfitted (DNP) because of its leakage

## To verify against datasheets
- LS013B7DH03 outline drawing and FPC connector part
- ST25DV04KC I2C pins while its VCC is off (the bus pull-ups stay on VBAT); DS13519 doesn't specify it, so check on the first board
- Footprints for the reed switch and buttons
