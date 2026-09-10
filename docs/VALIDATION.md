# Entwicklungsprüfung vom 9. September 2026

## Tatsächlich ausgeführt

- TC001-Oberfläche portiert: vier Tabs, gemeinsames Branding/CSS, 52×16-Livebild,
  18 Farben, Geräte-/Browser-Vorschauen, Paneltests und optionale Web-Anmeldung.
  Fünf CTest-Gruppen und elf Node-Prüfungen bestanden. HTTP-Integration prüft
  offenen Erstzugriff, optionalen Passwortschutz, fremde Origin-Abweisung,
  unveränderte gespeicherte Konfiguration während Vorschauen, automatisches
  Ende nach zehn Sekunden, Alarmvorrang und expliziten Reset.
- Browserprüfung im lokalen Simulator auf Desktop und 390-Pixel-Breite:
  DE/EN, Tabs, Helligkeit speichern, Panel-Vorschau, Rückkehr zur Liveanzeige und
  Sprache speichern. Keine erfassten JavaScript-/CSP-Fehler und kein horizontaler
  Überlauf. Die Vorschau wird von derselben C++-Renderfunktion wie die Uhr erzeugt.
- Aktualisierung auf echter TC002 nach ABI- und SHA-256-Prüfung installiert.
  Unauthentifizierte Status-/Konfigurationsanfragen liefern 200, Web-Passwort ist
  nicht gesetzt, vorhandenes Owlet-Passwort bleibt gespeichert. Nach sechs Abrufen:
  `connected=true`, kein Fehler, „Socke laedt“, 832 Pixel. Vorherige App-Bibliothek
  liegt ausschließlich lokal als Rückfallkopie; keine Zugangsdaten wurden exportiert.
- Echte Owlet-Anmeldung und Geräteauswahl durch Jannik; anschließender
  Cloud-Abruf auf der TC002 erfolgreich. Beim Laden lieferte das optionale
  Durchschnittsfeld `oxta` den Platzhalter 255. Die ursprüngliche Bereichsprüfung
  verwarf deshalb den gesamten Datensatz. Der Parser behandelt genau diesen
  Platzhalter nun wie das fehlende optionale Feld; primäre Messwertgrenzen und
  Alarmflags bleiben unverändert. Regressionen prüfen Laden, aktiven Zustand,
  erhaltene Flags und weiterhin abgelehnte ungültige Werte.
- Fix auf der Uhr installiert und per SHA-256-Rücklesen überprüft. Danach vier
  Abrufe, `connected=true`, `cloud_fresh=true`, kein Fehler und Bildschirm
  `battery` mit „Socke laedt“. `vitals_fresh=false` ist beim Laden beabsichtigt.
- Erster echter TC002-Start (Hersteller-App 1.0.1, MCU V1.0.16): temporäre
  FlyThings-Anwendung läuft, HTTP-Oberfläche und authentifizierter Status liefern
  200, Status meldet `target=tc002`, `mode=live` und 832 Matrix-Pixel. Mehrere
  Minuten fortlaufender Betrieb. Jannik bestätigt die physische LED-Anzeige
  „Owlanzi Local Setup“; Pixelgeometrie/Farben, Eingaben und Audio bleiben offen.
- Alle vier installierten Dateien zurückgelesen und gegen Manifest-SHA-256
  geprüft. Unveränderte Konfiguration über die geschützte API gespeichert (200).
- Reale Systembibliotheken vor dem Start geprüft. C++-Versionsunterschiede durch
  Link gegen die gelesene Gerätebibliothek behoben; Startabsturz beim
  Dateisystemzugriff durch POSIX-Aufrufe statt GCC-8/9-Mischung behoben.
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

## WebUI-Ladefehler behoben (9. September 2026)

Eine fehlgeschlagene Initialabfrage ließ bisher alle Einstellungen dauerhaft
deaktiviert, während erfolgreiche Statusabfragen den Fehler ausblendeten. Im
Browser mit einer gezielt abgefangenen `/api/config`-Antwort nachgestellt.
Die Initialisierung lädt Konfiguration und Standardwerte jetzt nacheinander,
wiederholt fehlgeschlagene Versuche und zeigt ihren Fehler unabhängig vom
Live-Status an. Vorschauen werden nacheinander gerendert. HTTP-Verbindungen
werden nach einer Antwort geschlossen, damit wartende Browser nicht die drei
Webserver-Worker zwischen Abfragen belegen. Abgelehnte Schreibanfragen werden
nach dem Einlesen des auf 16 KiB begrenzten Inhalts beantwortet, damit getrennt
gesendete Header und Nutzdaten keinen Verbindungsabbruch vor der JSON-Antwort
auslösen. API-Tests decken dies sowie weiterhin den optionalen Zugriffsschutz
für Oberfläche, Assets und schreibende Anfragen ab.

- Alle fünf CTest-Gruppen und elf Node-Tests bestanden. Der API-Test prüft
  zusätzlich drei offene Browser-Verbindungen und einen weiteren Client.
- `tests/browser-startup-fault.js` als Navigation-`initScript` laden und danach
  `window.checkSettingsRecovery()` ausführen: wiederholte 503-Antworten,
  sichtbare Fehlermeldung trotz erfolgreichem Live-Status und automatische
  Wiederherstellung. Auf der realen TC002 bei 390 Pixeln Breite bestanden.
- `tests/browser-ui-check.js` als Browserfunktion auf einer separaten
  `--demo`-Instanz ausführen: Farbauswahl, Canvas-Pixel, temporäre Geräteanzeige,
  Speichern, Alarm-Schalter und Grenzwert, Passwort-Sichtbarkeit und Speichern
  fiktiver Owlet-Zugangsdaten bestanden. Der Test verweigert reale Geräte.
- Aktualisiertes ARM-Paket auf der lokalen TC002 installiert und nach Transfer
  per SHA-256 geprüft. Dort alle 18 Farbeingaben geladen; Farbänderung sowohl
  im Canvas als auch im Geräte-Framebuffer nachgewiesen. Display gespeichert,
  Alarm-Schalter betätigt und ursprüngliche Alarmkonfiguration gespeichert.
  Owlet-Eingaben im Browser geprüft, ohne die Zugangsdaten zu überschreiben.
  Öffentliche Konfiguration vor und nach dem Test identisch; keine aktive
  Testvorschau zurückgelassen. Kein horizontaler Überlauf bei 390 Pixeln.

Die anschließende TC002-Farbzuordnung wurde mit allen 25 Paletteinträgen gegen
den echten Renderer geprüft: Jede Farbe muss in ihrem zugehörigen Zustand
sichtbar sein; neu getrennte Texte/Werte dürfen keine fremden Bildbereiche
einfärben. API-Tests prüfen außerdem die Übernahme alter gespeicherter Farben
und den Erhalt bereits getrennter Farben. `tests/browser-palette-check.js`
prüft auf einer Demo-Instanz jeden Farbeingang bis zum passenden Canvas-Pixel,
den Erhalt gewählter Vorschauzustände und unveränderte gespeicherte Einstellungen.
Mobile Darstellung mit 390 Pixeln Breite visuell kontrolliert.

## WLAN-Onboarding

Eigener WLAN-Assistent ergänzt; Ablauf und Hardwaregrenzen stehen in
[WIFI_ONBOARDING.md](WIFI_ONBOARDING.md). Sechs CTest-Gruppen und elf Node-Tests
bestanden. HTTP-Tests prüfen auch WLAN-Zugriffsschutz, fremde Origin, ungültige
Daten, Alarmvorrang, Captive-Portal-Redirect, Fehlversuche und unveränderte
Owlet-Konfiguration. `tests/browser-wifi-check.js` auf der lokalen Demo-Instanz:
Scan-Rückmeldung, SSID-Auswahl, Passwortanzeige/-leerung, Verbindung, Rückkehr
nach falschem Passwort, Konfigurationsisolation und Layout bestanden.

Auf der echten TC002: ARM-ABI und Transfer-Hash geprüft, aktualisierte Anwendung
gestartet, vorhandene WLAN-Verbindung erhalten. HTTP auf 80 und 8080 sowie Scan
mit neun gefundenen Netzwerken erfolgreich. Ein zehn Sekunden geöffneter
Hotspot kehrte nach insgesamt etwa 21 Sekunden ohne Fehler ins vorherige WLAN
zurück. Der abschließende Build läuft wieder im Live-Modus mit verbundenem WLAN.
Keine Handy-DHCP-/DNS-Abnahme oder echte neue Passworteingabe durchgeführt.

## Gemeinsame Einrichtung und Uhrzeit

WLAN und Owlet werden im Hotspot gemeinsam gespeichert, bevor die Verbindung
wechselt. Die Uhrzeit ersetzt den kleinen Akku in der Messwertansicht.
Sieben CTest-Gruppen, elf Node-Tests und die mobile Browserprüfung
`tests/browser-clock-setup-check.js` bestanden. Auf der echten TC002 sind
NTP-Synchronisation, Europe/Berlin und die erhaltenen bisherigen Einstellungen
bestätigt; WLAN und Owlet sind verbunden. Details: [CLOCK.md](CLOCK.md).

## OTA-App-Updates · 9. September 2026

- Acht CTest-Gruppen und zwölf Node-Tests bestanden. Neu geprüft: strikte
  Versions-/Manifestprüfung, Tagesmarkierung vor dem Abruf, Erhalt über Neustart,
  zurückgestellte Uhr, ausgeschaltete Suche, manuelle Suche, Alarm-/WLAN-Sperre,
  Abbruch während Download und manueller Rückfall.
- Website: 16 HTTP-/Statistiktests und drei gezielte Publisher-Tests bestanden.
  TC002-Downloads, Tageszähler, DNT/GPC/Opt-out, HEAD, Range, Dateigrenzen und
  unveränderte Nutzdaten sind abgedeckt.
- Echter Geräteweg: OTA-Starter 0.2.0 im Slot `a`, Geräteabruf des HTTPS-
  Manifests auf owlanzi.com, Installation von 0.2.1 in Slot `b`, Startbestätigung.
  Weboberfläche lädt nach dem App-Wechsel neu. Mobile Darstellung mit 390 Pixeln
  Breite ohne horizontalen Überlauf geprüft.
- Manueller Rückfall auf 0.2.0 erfolgreich. Danach automatischer Rückfall
  getestet: Zwei intakte App-Versionen, absichtlich unpassende erwartete
  Versionskennung; nach ausbleibender Bestätigung Rückkehr zur vorherigen App.
  Keine Binärdatei oder Nutzereinstellung für diesen Test beschädigt.
- App-Konfiguration einschließlich Zugangsdaten, Farben, Helligkeit, Alarmen
  und Zeitzone sowie WLAN-Konfiguration per SHA-256 vor/nach OTA identisch.
  Nur Prüfsummen dokumentiert. WLAN, Owlet-Cloud und NTP verbunden.
- Paket 0.2.1: 2.572.354 Bytes; SHA-256
  `63ab47d2d9582577a8f6df79c8a5835b3541e7733a1aa19c5e1a30db773c6f22`.
  Metadaten, Image und GPL-Quellarchiv per SSH veröffentlicht, anschließend
  über öffentliches HTTPS bytegenau geprüft. 13 bestehende Website-/TC001-
  Dateien per Prüfsumme unverändert. Release-Backup bleibt HTTP-gesperrt.
- Statische SDK-Audio-/FFmpeg-Abhängigkeiten entfernt. Eigene PCM-Ausgabe
  über die vorhandene Systembibliothek; leiser Testton ausgelöst, App lief weiter.
  Die Standardfehlerausgabe des App-Prozesses liegt nicht in logcat vor;
  subjektive Lautstärke und vollständige akustische Abnahme bleiben offen.

## Noch nicht ausgeführt

Noch keine echte Messreihe beim Tragen und keine physikalische Messung der
Hardwareausgabe. Speicherbedarf, Systemzeit/WLAN nach Geräte-Neustart,
MCU-Antworten, vollständige Pixel-/Farbprüfung und Ton bleiben offen.
Das Manifest kennzeichnet `hardware_verified: false`.

Keine dauerhafte Imageinstallation und kein vollständiger Stromausfalltest.
Der geprüfte App-Rückfall ist keine Linux-/Boot-Recovery. Die verbleibenden
Abnahmen stehen in [LOCAL_TESTING.md](LOCAL_TESTING.md).

Paketgrenzen, eingebundene Open-Source-Komponenten und ausgeschlossene
SDK-/Systembibliotheken sind in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)
und [OTA.md](OTA.md) dokumentiert.
