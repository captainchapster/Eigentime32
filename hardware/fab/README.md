# Eigentime32 fabrication files (JLCPCB)

Generated 2026-10-10 from `hardware/Eigentime32.kicad_pcb` on main (c302c13).
Before export (regenerated 2026-10-10 for the J2 paste change): DRC 0 violations, 0 unconnected, schematic parity clean; ERC 0 violations.

## Pre-order review (2026-10-10, main 8dfe562)

- KiCad 9.0.7: ERC 0, DRC 0 (0 unconnected, schematic parity clean). Gerbers and drills regenerated from the board match `gerbers/` line for line.
- Pinouts checked against datasheets: RV-3032-C7 (EVI tied to VDD; the manual only forbids floating or VBACKUP), TPS22916C YFP, TPS61099 DRV (402k/200k gives 3.01 V), CT8132SK SOT-23, ST25DV04KC UFDFPN8 (EP left floating as ST requires), DMG2305UX, SN74LV4T125 RGY, SN74LV1T34 DCK, LS013B7DH03 FPC, Tag-Connect (matches KiCad's footprint, mirrored on the back).
- J2 land pattern matches every number in Panasonic's Y5B pattern (0.3 x 0.8 pads, 0.4 x 0.8 terminals 1.0 mm outside the end pins, 3.30 mm overall depth). The Y5B side view confirms the contact tails are on the FPC insertion side and the lever on the far side, as the layout assumes (opening toward 6 o'clock).
- J2 paste follows Panasonic's metal mask pattern: 0.25 x 0.31 mm openings on the signal pads, 0.2 mm in from the outer end (was full pad), to keep solder out of the contacts. Only `F_Paste` changed; DRC/ERC re-run clean.
- Before paying: re-check stock for U9 (4 at JLC, need 5), U1 and U8, and step through every rotation below in JLC's preview.

## Files

| File | What |
|---|---|
| `Eigentime32_gerbers_JLC.zip` | Upload this to JLCPCB. It contains everything in `gerbers/` |
| `gerbers/*.gtl .g1 .g2 .gbl` | Copper F.Cu, In1.Cu, In2.Cu, B.Cu |
| `gerbers/*.gts .gbs`, `*.gtp .gbp`, `*.gto .gbo` | Solder mask, paste, silkscreen (front and back) |
| `gerbers/*.gm1` | Board outline (40 mm round) |
| `gerbers/*-PTH.drl`, `*-NPTH.drl` | Excellon drills: 77 plated 0.3 mm (75 vias + 2 coil terminals), 3 non-plated 0.99 mm (Tag-Connect) |
| `gerbers/*-drl_map.gbr` | Drill maps (reference only) |
| `Eigentime32_BOM_JLC.csv` | JLC assembly BOM: Comment, Designator, Footprint, LCSC Part # |
| `Eigentime32_CPL_JLC.csv` | JLC placement file: Designator, Mid X, Mid Y, Layer, Rotation (KiCad rotations, see the rotation checks below) |

Export settings follow JLCPCB's KiCad guide: Protel extensions, no X2 or netlist attributes, mask subtracted from silk, 4.6 format in mm, Excellon in mm with decimal zeros, alternate oval-hole mode, PTH and NPTH in separate files.

## Order settings

- 4 layers, 40 x 40 mm, **0.8 mm**, FR-4 TG135
- **Specify stackup: JLC04081H-3313**: 35 um / 3313 prepreg 0.0994 mm / 15.2 um / core 0.5 mm / 15.2 um / 3313 prepreg 0.0994 mm / 35 um
- Outer copper 1 oz, inner 0.5 oz
- Min via hole/diameter: 0.3 mm / (0.4/0.45 mm). This is the default, no surcharge
- Surface finish: **ENIG recommended**. U5 is a 0.4 mm pitch BGA and there are several QFN/DFN parts, and HASL pads aren't flat enough for them. Leaded HASL isn't offered for 4-layer 0.8 mm boards anyway
- Vias: tented. Remove or position the JLC order mark if you don't want it on the board

## Estimates (JLCPCB, 2026-10-10, 5 boards, before shipping and coupons)

| Option | Cost |
|---|---|
| Bare boards, lead-free HASL | $13.10 |
| Bare boards, ENIG | $24.80 |
| Bare boards ENIG + one stencil (top and bottom on a single foil) | $32.02 (+ DHL ~$37) |
| Double-sided Standard PCBA, all 48 placements, ENIG boards (estimate, see below) | ~$255-260 (+ shipping ~$35-40) |

The bare-board and stencil figures come from JLC's live quote calculator. JLC only prices assembly after the Gerbers are uploaded, and nothing was uploaded, so the PCBA line is worked out from JLC's published assembly price list:
- setup, double-sided Standard: $51.12
- assembly stencil: $16.42
- feeder loading for 25 unique parts at $1.53: $38.25
- 875 joints at $0.0016: $1.40
- X-ray for 30 BGA/QFN/DFN parts at $0.82: $24.60
- fixtures: ~$16.42
- packing and placement check: ~$1
- parts at JLC's live prices: $83.26
- ENIG boards: $24.80

Expect the real quote to differ by about +/- $30. Cheaper route: have JLC assemble only the back (single-sided setup $25.56, stencil $8.21) and hand-solder the 13 front parts.

## Part sourcing (JLC/LCSC stock checked 2026-10-10; 5 boards need 5 of each IC)

| Ref | Part | LCSC | Library | Stock | Note |
|---|---|---|---|---|---|
| U1 | RV-3032-C7 32.768kHz 2.5ppm TA-QC | C5127802 | extended | 27 | Low stock. The TA-QA grade (C5366550, 1804 in stock) is the AEC-Q qualified version of the same part |
| U2 | STM32L031G6U6TR | C883300 | extended | 7883 | Tape-and-reel version of STM32L031G6U6. The plain part (C96514) has 0 |
| U3 | TPS61099DRVR | C2842395 | extended | 3021 | |
| U5 | TPS22916CYFPR | C2680319 | extended | 3055 | 0.4 mm BGA: JLC Standard PCBA with X-ray |
| U6 | SN74LV4T125RGYR | C411879 | extended | 2009 | Front side |
| U7 | SN74LV1T34DCKR | C78541 | extended | 92380 | Front side |
| U8 | ST25DV04KC-IE8C3 (UFDFPN8) | C5221756 | extended | 11 | The schematic says -IE6C3. IE8 is the -40 to 105 C grade of the same part and pinout; the IE6C3 UFDFPN8 isn't stocked |
| U9 | CT8132SK-IS3 | C22423395 | extended | **4** | **Short by 1 for 5 boards** |
| Q1 | DMG2305UX-7 | C150470 | extended | 3501 | |
| L1 | Wurth 74438323010 1 uH WE-MAPI 2512 | C7038984 | extended | 2622 | Matches the MAPI-2512 footprint. The BOM's former WE-LQS 74404024010 had a different land pattern and 0 stock; bom.csv corrected |
| SW2, SW3 | Panasonic EVQ-P7A01P | C79167 | extended | 5605 | |
| BT1 | Keystone 3034 | C5213768 | extended | 1337 | Large retainer; JLC may hand-solder it (labour $3.58 per order) |
| J2 | Panasonic AYF531035 | C425133 | extended | 4953 | Front side. Sharp's recommended connector for the LS013B7DH03 |
| C2 | 100 uF 6.3 V 1206 | C15008 | basic | 1.77 M | |
| C4-C6 | 10 uF 10 V 0603 | C19702 | basic | 10.7 M | |
| C3, C9-C12, C15-C17, C20, C21 | 100 nF 16 V X7R 0402 | C1525 | basic | 23.8 M | |
| C7, C13, C18, C19, C23 | 1 uF 25 V X5R 0402 | C52923 | basic | 7.3 M | |
| C8 | 470 nF 10 V 0402 | C47339 | extended | 0.49 M | |
| R1 | 402k 1% 0402 | C25785 | extended | 29 k | |
| R2 | 200k 1% 0402 | C25764 | basic | 2.2 M | |
| R3, R5, R12-R14 | 1M 1% 0402 | C26083 | basic | 2.2 M | |
| R4 | 470R 1% 0402 | C25117 | basic | 0.78 M | |
| R6, R7, R9, R10 | 10k 1% 0402 | C25744 | basic | 19.8 M | |
| R8 | 470k 1% 0402 | C25790 | extended | 0.23 M | |
| R15 | 100k 1% 0402 | C25741 | basic | 8.1 M | |

The JLC-stocked Samsung and UniOhm passives replace the Murata and Yageo parts named in `hardware/bom.csv`. They have the same value, size and voltage rating, or a higher rating.

Not in the BOM or CPL:
- **DNP:** C14 (MCU bulk), C22 (NFC tuning; fit at tuning, C0G 0402)
- **Not parts:** TP1 (test pad), J1 (Tag-Connect pads), AE1 (PCB coil)
- **Off-board:** DS1, the Sharp LS013B7DH03 display. It plugs into J2 after assembly (Mouser/DigiKey)

## Hand-solder and sourcing plan

- **U9 CT8132SK-IS3:** JLC has 4. Either order 4 at JLC and hand-solder the fifth board's from DigiKey or Mouser (stock not checked here), or use JLC Global Sourcing to pre-order 5. It's a SOT-23 on the back, easy by hand.
- **U1 RV-3032-C7 (27) and U8 ST25DV04KC (11):** enough today, but low. Re-check on the order day, and pre-order through JLC Global Sourcing if they've gone.
- **DS1 display:** buy separately. Fold the FPC under the glass as described in docs/decisions.md.
- **C22:** only if the NFC coil tunes high; hand-fit after tuning.

## Rotation checks (verify in JLC's placement preview before paying)

The CPL uses KiCad's rotations. JLC's library zero orientation differs for some package families. These are the usual corrections from the community kicad-jlcpcb-tools table; confirm each one in the preview:
- **Q1, U9 (SOT-23) and U7 (SC-70-5 / SOT-353):** usually +180 deg
- **U2 (QFN-28), U6 (VQFN-14), U3 (WSON-6), U8 (UFDFPN8):** QFN/DFN parts often need +270 deg. Check the pin-1 dot
- **U1 (RV-3032 SON-8, project footprint):** no table entry. Check pin 1 (VBACKUP) by eye
- **U5 (BGA-4):** usually 0. Check the A1 corner
- **J2 (front, 180 deg), BT1, SW2/SW3:** connectors and switches often come in rotated by 90 or 180 deg. J2's contacts must face the board edge (6 o'clock), BT1's opening must face 12 o'clock, and the switch actuators must point outward
- **Back-side parts (35 placements):** JLC mirrors them; check the bottom-view preview for every IC above
- 0402/0603 passives and L1: no correction needed (non-polar)

## DFM notes

- Coil terminals (AE1 pads 1 and 2) are plated holes of 0.3 mm in 0.5 mm pads, a 0.1 mm ring. That's via-sized and fine as a via, but JLC's DFM may flag it as a pad annular ring below 0.15 mm. Accept it as a via if asked.
- 37 short track segments are 0.15-0.19 mm wide (fab minimum 0.09 mm); everything else is 0.2 mm signal or 0.25 mm power.
