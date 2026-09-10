# TC001-Oberfläche auf der TC002

Die lokale Geräteoberfläche übernimmt Logo, Farben, Typografie, Seitenleiste,
mobile Navigation, Karten, Eingabefelder und Speicherleisten der TC001.
Die Quelldateien der TC001 und owlanzi.com bleiben unverändert.

| Bereich | TC002-Umsetzung |
| --- | --- |
| Zugang | Initial ohne Anmeldung und ohne Einrichtungsschlüssel; optional eigenes Web-Passwort unter System, Benutzername `owlanzi` |
| Status | Puls, Sauerstoff, Sockenakku, Schlaf-/Ladezustand, Basisstation, Hardware, Cloud-Verbindung, letzte Fehler, Abrufzähler und Laufzeit |
| Liveansicht | Exakte 52×16-Pixel aus dem nativen Renderer, optionale Helligkeitssimulation im Browser |
| Display | Normale und Alarmhelligkeit, zusätzliche Vorschauhelligkeit, getrennte Farben für Messwerte, Schlaf, Akkurahmen/-füllstände, Warten, Offline und Alarme |
| Farbvorschau | Derselbe C++-Renderer für Browser und Uhr; Änderungen erscheinen zehn Sekunden auf der Uhr, ohne Speicherung; Einzel- und Gesamtreset der Farben |
| Paneltests | Vier farbige Ecken, Lauflicht durch 832 Pixel, vollflächiger Farbwechsel |
| Alarme | Eigene Grenzwerte und Dauer, Ton aktivieren, Lautstärke 0–6, Wiederholung, manueller Testton und Bestätigung |
| System | WLAN-Scan, Netzwerkwechsel und Einrichtungs-Hotspot; Owlet-Konto, echte Geräteauswahl, Region, Abrufintervall, Deutsch/Englisch, optionales Web-Passwort und Reset der Owlanzi-Einstellungen |
| Speicherung | Display, Alarme und System werden getrennt gespeichert; Änderungen in anderen Bereichen bleiben im Formular erhalten |

## Unterschiede der Plattform

Die TC002-Palette folgt ihren tatsächlichen Bildinhalten. 25 Farben steuern:

- Obere Messwertzeile: Herz, Pulswert, O2-Beschriftung und
  Sauerstoffwert einschließlich Prozentzeichen jeweils getrennt.
- Uhrzeit unten rechts: HH:MM mit eigener Farbe; Zeitzone und manuelle/automatische
  Zeit unter System. [Details](CLOCK.md).
- Schlafstatus unten links: Text und Zustandsbalken gemeinsam pro Schlafzustand.
- Akku: Umrandung, Füllung normal/ladend/mittel/niedrig, Prozentzahl und die
  Texte LAEDT/CHARGING bzw. SOCKE AUS/SOCK OFF getrennt. Die Akkuanzeige beim
  Laden bleibt erhalten; neben den Messwerten steht jetzt die Uhrzeit.
- Warten/Offline: Herz ohne Werte, O2 und Platzhalter, WARTE/WAITING,
  OFFLINE und VERBINDE/RETRYING getrennt.
- Alarme und Hinweise: jeweils beide Textzeilen zusammen. Die Einrichtung
  hat eine eigene OWLANZI-Titelfarbe und nutzt die Hinweisfarbe für LOCAL SETUP.

O2 beginnt in der Messwertansicht bei x=27; rechts unten steht die Uhrzeit
anstelle des kleinen Akkus. Der Trennpunkt und seine Farbeinstellung entfallen.
Alte gespeicherte Trennpunkt-Farben werden beim Laden ignoriert.

Zustandsschalter an den Farbkarten zeigen die passende echte 52×16-Anordnung.
Farbbearbeitung wählt automatisch den Zustand, in dem das Element sichtbar ist.
Die anderen Karten behalten ihre gewählte Vorschau. Die Zustandsschalter allein
ändern nur die Browseransicht; Farbbearbeitung aktiviert wie bisher kurz die
Vorschau auf der Uhr. Vorhandene gespeicherte Farben werden beim Laden auf neu
getrennte Elemente übertragen; bereits individuell gesetzte Farben bleiben
erhalten. Ein Akkustand über 0 % hat mindestens eine gefüllte Pixelspalte,
damit insbesondere die Farbe für niedrigen Akkustand sichtbar bleibt.

- Die TC002 nutzt 52×16 statt 32×8 Pixel. In der zweiten Zeile stehen Schlafstatus
  und Uhrzeit; alle Tests verwenden die vollständige neue Matrix.
- Lautstärke folgt der TC002-Skala 0–6. Helligkeit wird manuell bzw. am Drehknopf
  geregelt; ESP32-Lichtsensorwerte und dessen Schaltschwellen werden nicht vorgetäuscht.
- Eigene WLAN-Einrichtung wie bei der TC001: offener Hotspot `owlanzi`,
  `192.168.4.1`, Netzwerkauswahl und Passwort. Die TC002 wechselt mit einem
  Funkinterface zwischen Hotspot und Heim-WLAN; im Hotspot ist die Netzwerkliste
  zwischengespeichert. [Details](WIFI_ONBOARDING.md).
- Es gibt keinen TC001-OTA-/ESP32-Flasher auf der TC002. Dauerinstallation,
  Online-Updates und deren Recovery-Prüfung folgen separat.
- Reset entfernt nur die eigene Konfiguration inklusive Owlet-/Web-Passwort.
  System-WLAN und Hersteller-App bleiben erhalten. Die UI verlangt eine Bestätigung.

## Vorschau und Zugriff

Browser-Vorschaubilder verändern weder Cloud-Zustand noch Geräteeinstellungen.
Eine Geräte-Vorschau ist explizit markiert und endet automatisch, beim Speichern
oder mit „Zur normalen Anzeige“. Ein realer kritischer Alarm bricht die Vorschau
ab und blockiert neue Vorschauen. Beispielwerte werden niemals als Cloud-Messungen
oder echte Alarme in den Zustandskern geschrieben.

Ohne Web-Passwort ist die Oberfläche im eigenen Netz direkt erreichbar. Ein
gesetztes Passwort schützt anschließend Seite und API via HTTP Basic Auth.
Passwörter werden nicht über die Konfigurations-API zurückgeliefert. JSON-Schreib-
anfragen benötigen weiterhin den eigenen Request-Header; fremde Origin- und
Cross-Site-Anfragen werden abgewiesen. Es werden keine externen Skripte geladen.

## Display-Standardwerte

Die am 9. September 2026 auf der TC002 gewählte Palette ist der neue Standard
für Ersteinrichtung und Farb-Reset. Normale Helligkeit: 8/255, Vorschau: 15/255,
Alarm: 255/255. Die übernommenen Standardfarben und die ergänzte weiße Uhrzeit stehen in `Palette` in
`include/owlanzi/core.hpp`; `/api/defaults` liefert dieselben Werte an die WebUI.
Bestehende gespeicherte Einstellungen werden bei einem Update weiter verwendet.
