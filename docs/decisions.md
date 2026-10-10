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
| 2026-10-10 | Lid sensor: Allegro CT8132SK-IS3 TMR switch (push-pull, no pull-up) instead of an NC reed | SMD normally-closed reeds are Form C parts ~27 mm long (Littelfuse MDSM-DT) and don't fit; the Coto RR122 is obsolete. Costs 190 nA typ / 900 nA max average |
| 2026-10-10 | Buttons: Panasonic EVQ-P7A01P side-push, 1.35 mm high, actuators at the board edge; 1 MΩ external pull-ups, internal pull-ups off | Operated through the case wall; a held button costs 3 µA, not ~75 µA |
| 2026-10-10 | Case orientation (front view): buttons at ~10 and ~2 o'clock, hinge at 9, lid magnet / sensor at ~4, LCD FPC at 6, CR2032 slides in from 12 | Lid sensor roughly opposite the hinge; FPC folds round the 6 o'clock edge |
| 2026-10-10 | Keep a 21 mm wide no-parts channel from the holder to the 12 o'clock edge (rule area on the back) | The Keystone 3034 is a tunnel retainer with ~0.5 mm headroom over the cell, so the CR2032 slides in flat and its trailing edge starts beyond the board edge: the case needs a slot at 12 o'clock, or the board comes out for a swap |
| 2026-10-10 | NFC coil: 8-turn F.Cu spiral, 38.1 mm OD, 0.15/0.15 mm, terminals at 4-5 o'clock | 0.9 mm under the planned 39 mm so the outer terminal via fits inside the 0.3 mm edge clearance; ~5.1 uH free air |
| 2026-10-10 | CR2032 negative-contact pad reduced from the KiCad footprint's 17.8 mm to 5 mm (project footprint) | Keystone p.11 asks for 0.156 in (3.96 mm) square minimum; frees the area under the cell for mask-covered tracks |
| 2026-10-10 | Board rules: 0.127 mm clearance, 0.2 mm signal / 0.25 mm power tracks, 0.5/0.3 mm vias, 0.2 mm hole-to-copper, 0.3 mm copper-to-edge | Standard 2-layer fab limits; check against the chosen fab before ordering |
| 2026-10-10 | No copper pour anywhere on the board | The coil encloses the whole board; a ground pour inside the loop would kill the NFC coupling |
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
