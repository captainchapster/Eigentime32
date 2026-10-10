# Eigentime32

A self-contained digital pocket watch that keeps its own time for a decade. No GPS, one CR2032, one PCB.

*Eigentime* (proper time) is the time a clock measures along its own path, with no outside reference. That is the whole idea here.

## At a glance
| | |
|---|---|
| Timekeeping | Micro Crystal RV-3032-C7 TCXO RTC, ±1.5 ppm (0–50 °C), ~160 nA |
| Display | Sharp LS013B7DH03 1.28" 128×128 memory LCD, powered only while the lid is open |
| MCU | STM32L031G6 (proposed) |
| Wake | Normally-closed reed switch + magnet in the lid |
| Power | CR2032, ~1 µA average, ~10-year target |
| Recalibration | ST25DV04K NFC tag: set time and trim drift from a phone, about once a year |
| Size | 43.9 mm round × 10.8 mm thick with the lid shut (3D-printed hunter case, see `case/`) |

## Accuracy
Aging of the crystal, not temperature, dominates over ten years. With one calibration at build time and a yearly NFC recalibration the target is roughly ±30 s/yr. See [docs/power-pins-accuracy.md](docs/power-pins-accuracy.md).

## Repository layout
- `docs/` design notes and decisions
- `hardware/` KiCad project, BOM
- `firmware/` MCU firmware
- `case/` 3D-printed case: parametric CadQuery model, fit check, printable STL/STEP

## Status
Architecture defined, schematic not started. See [docs/decisions.md](docs/decisions.md).
