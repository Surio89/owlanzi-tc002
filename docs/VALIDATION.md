# Entwicklungsprüfung vom 9. September 2026

## Tatsächlich ausgeführt

- Windows/MSVC-C++17-Build der vollständigen Anwendung einschließlich eingebetteter
  Oberfläche und nativer HTTPS-Implementierung.
- Fünf CTest-Gruppen erfolgreich: Zustandskern, Renderer, Konfiguration/Owlet-
  Client, echte lokale HTTP-API und lokaler Gerätehelfer. Kern und Renderer umfassen
  132 bzw. 12 explizite Prüfungen. Die Gerätehelfer-Tests verwenden nur Fixtures.
- Zehn Node-Tests für Konfiguration, Übersetzungen, Anzeige, Vorschauhelligkeit,
  Passwortbehandlung und authentifizierte API-Anfragen erfolgreich.
- Browserprüfung gegen die echte PC-Anwendung auf Desktop- und 390-Pixel-
  Smartphonebreite: Pairing, Sprachwechsel, Schwellwerte, Speichern, leeres
  Passwort beibehalten, ausdrückliches Löschen, Alarm bestätigen und Offline-
  Darstellung. Keine Browserfehler, CSP-Verletzungen oder horizontaler Überlauf.
- Vollständiger ARMv7/EABI5-Hard-Float-Build mit der offiziellen Z21-GCC-8.3-
  Toolchain. Alle benötigten Anwendungssymbole beim Link geprüft (`-z,defs`).
  FlyThings-Startfunktionen sind in `libzkgui.so` exportiert.
- Manifestgrößen und SHA-256 der vier Geräte-Payload-Dateien überprüft. Das ZIP
  enthält genau diese vier Dateien plus Manifest, keine Laufzeitkonfiguration.
- Separater lokaler Paketprüfer akzeptiert das erzeugte ARM-Paket. Tests lehnen
  manipulierte Dateien, fremde Targets und unerwartete Payload-Pfade ab.
- Die beiden vorhandenen Repositories `owlanzi-firmware` und `owlanzi-website`
  blieben unverändert.

## Wesentliche abgedeckte Fehlerfälle

Cloud-Messzeit statt Werteänderung; identische neue Messwerte; alte oder zukünftige
Zeitstempel; Sitzungswechsel nach Laden/Abnehmen; 20-Sekunden-Datenablauf und
30-Sekunden-Offlineanzeige; kritische letzte Alarme; Bestätigung und neue Ursachen;
eigene Alarmdauer über verschiedene Messzeitpunkte; unbekannte Schlafwerte;
abgelehnte Zahlenüberläufe und falsche JSON-Typen; Mehrgeräteauswahl;
Token-Erneuerung; Kontowechsel; Teilfehler bei APP_ACTIVE; Abbruch zwischen
Anmeldeschritten; geschützte API-Schreibzugriffe; atomare Konfiguration und
fehlende Kennwörter in API/Logs.

## Noch nicht ausgeführt

Keine Verbindung zu einer TC002, keine echte Owlet-Anmeldung, keine Messung der
Hardwareausgabe. Speicherbedarf, Systemzeit/WLAN nach Geräte-Neustart, reale
Bibliotheks-ABI, MCU-Antworten, physische Anzeige und Ton müssen lokal geprüft
werden. Das Manifest kennzeichnet `hardware_verified: false`.

Keine dauerhafte Imageinstallation, kein Stromausfall-/Recovery-Test, keine
Online-Updateinstallation und keine Veröffentlichung auf owlanzi.com. Der nächste
Schritt ist der Ablauf in [LOCAL_TESTING.md](LOCAL_TESTING.md), sobald die Uhr da ist.

Die öffentlich zu klärenden SDK-Weitergabebedingungen sind in
[THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) und
[TC002_PLATFORM.md](TC002_PLATFORM.md) festgehalten.
