# Firmware

Target: STM32L031 (proposed). Stop mode except while the lid is open.

Behaviour: reed switch interrupt → enable boost and RTC CLKOUT → draw time → redraw on the RTC minute interrupt → lid closed or 30–60 s timeout → everything off, back to stop. NFC: compare phone-written time on wake, set time, update the RV-3032 offset register.
