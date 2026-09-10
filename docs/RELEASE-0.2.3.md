# TC002 0.2.3

The selected display layout uses original 4x7 digits for pulse and oxygen,
a 7x7 heart, and centered sleep text and time in the lower half. It removes
the O2 label and the sleep bar. Colors and brightness remain user settings.

Coordinates (zero-based, 52x16): heart x1–7/y0–6, pulse right edge x22/y0–6,
oxygen digits right edge x43/y0–6, percent x45–47/y2–6. Sleep text is centered
in x0–25 at y10; the clock occupies x29–47/y10–14. The waiting screen uses
the same heart and measurement positions with placeholders.

The web previews use the device renderer. The obsolete label color control is
removed from the interface; its saved configuration field remains compatible
with previous versions and rollback. Existing settings are not migrated or reset.

Validation includes an exact 832-pixel comparison against the approved preview
(132 bpm, 100%, LIGHT, 21:48), bounds checks for two- and three-digit readings,
API preview color isolation, and matching web palette controls. Native and ARM
builds use the same renderer. Physical LED appearance requires device testing;
this release publication does not install the update on a clock.
