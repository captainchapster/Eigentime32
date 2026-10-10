# Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-10-10 | RV-3032-C7 TCXO RTC does all timekeeping; MCU sleeps | ±1.5 ppm at ~160 nA, offset register for drift trim |
| 2026-10-10 | Sharp LS013B7DH03 memory LCD, on only while lid is open | Microwatt power, sunlight-readable |
| 2026-10-10 | 3D-printed case, designed around the PCB | Build around the device, not the reverse |
| 2026-10-10 | Round PCB, optimised for compactness | LCD diagonal (40.3 mm) sets the diameter: ~40 mm PCB, ~44 mm case |
| 2026-10-10 | TPS61099 boost at 3.0 V replaces the TPS22916 load switch | Keeps the LCD in spec as the cell sags; true disconnect also gates it |
| 2026-10-10 | Keep the ST25DV04K NFC tag | Yearly phone recalibration controls crystal aging |
| 2026-10-10 | CR2032 instead of CR2450 | ~8 mm vs ~10 mm thick; still ~2× capacity margin at ten years |

## Proposed, not yet confirmed
- STM32L031G6 (UFQFPN28) as the MCU
- RV-3032 CLKOUT at 1 Hz drives the LCD EXTCOMIN
- External 1 MΩ pull-up on the reed switch (internal pull-up would draw ~75 µA while the lid is open)
- ST25DV04K VCC switched from an MCU GPIO

## To verify against datasheets
- LS013B7DH03 supply range and logic VIH at 3.0 V vs MCU output late in cell life
- LS013B7DH03 outline drawing and FPC connector part
