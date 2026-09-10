# TC002 0.2.4

The two auxiliary LEDs documented by Ulanzi (`GPIO_06` and `GPIO_85`) are
switched off on each Owlanzi app start, before the matrix handshake. Both
outputs are active-high, so the app sets them to zero. A failed GPIO write
is logged and does not prevent the display and web setup from starting.

Matrix content and brightness continue to follow the saved settings. The
update uses the existing TC002 OTA slots and preserves the Owlet account,
Wi-Fi, display and alarm settings. It does not change the manufacturer boot
sequence or add permanent startup after a complete power loss.

Hardware basis: [Ulanzi GPIO documentation](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/Z21_TC002_Demo/README.md#gpio-接口).
