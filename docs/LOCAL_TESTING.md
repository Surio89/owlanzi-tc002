# Lokaler Test mit der ersten TC002

Stand: vorbereitete Entwicklungsversion. Ohne echte TC002 sind bisher nur die
Softwaretests und der ARM-Link geprüft. Es wurde keine Hardware kontaktiert.

## Auf dem PC

`scripts/build-local.ps1 -Run` baut, testet und startet die isolierte Simulation.
Browser unter `http://127.0.0.1:8080` öffnen, lokalen Kopplungscode eingeben.

Prüfen: Messwerte, Ladeanzeige, unbekannter Schlafzustand, Offline-Übergang,
Alarm bestätigen, Kontoeinstellungen speichern, Passwort leer lassen/gezielt
löschen, deutsche/englische Ansicht, Helligkeit 0 und schmale Bildschirmbreite.
Simulation ist deutlich markiert; sie führt niemals Cloud-Anfragen aus.

## Wenn die Uhr da ist

1. Ulanzi-Firmwareversion und Gerätemodell notieren. WLAN über die
   Hersteller-Einrichtung konfigurieren. Uhr und PC im selben lokalen Netz.
2. Originalfunktion und den dokumentierten Recovery-Weg prüfen, bevor die
   Geräteanwendung ersetzt wird. Ein Reset kann gespeicherte Einstellungen löschen;
   deshalb zuerst die tatsächliche Geräteversion und Herstellerhinweise prüfen.
3. Mit `scripts/local-device.py inspect --ip <IP>` ausschließlich Modellinformationen
   über HTTP und WLAN-ADB auslesen. Das Skript sucht keine anderen Netzwerkgeräte.
4. `scripts/build-tc002.py` ausführen und Manifest/ARM-Bibliothek prüfen.
5. `scripts/local-device.py run --ip <IP> --bundle build/tc002/device` startet
   das geprüfte Paket temporär. Bei unbekannter Modellantwort stoppt der Helfer;
   dann erst die konkrete Geräteantwort prüfen und die Modellzuordnung ergänzen.
   Vor dem Start liest er die im Manifest benötigten Systembibliotheken aus
   festen Bibliotheksverzeichnissen der Uhr und prüft deren exportierte Symbole
   gegen die ARM-Anwendung. Fehlende Bibliotheken oder Symbolversionen stoppen
   den Start. SDK-Linkbibliotheken dürfen niemals die echten Gerätedateien ersetzen.
6. Die lokale Oberfläche der Uhr auf `http://<IP>:8080` öffnen. Der Helfer legt
   den lokalen Kopplungscode unter `.local/device-<IP>/pairing-token` ab. Erst
   dort bei Bedarf das eigene Owlet-Konto einrichten.
7. Mit `scripts/local-device.py restore --ip <IP>` die temporäre Owlanzi-
   Anwendung entfernen und die Herstelleranwendung neu starten. Alternativ
   verwirft ein vollständiger Neustart die temporären Dateien. Kontodaten unter
   `/data/owlanzi` sind davon getrennt und bleiben zur nächsten Prüfung bestehen.

Der Helfer benötigt Android Platform Tools (`adb`) im PATH oder `--adb <Pfad>`.
Er schreibt keine Flash-Images und setzt keine Upgrade-Properties. Er weigert
sich, eine fremde Debugkonfiguration zu ersetzen. ADB ist ein mächtiger
Entwicklungszugang; für die spätere öffentliche Installation ist ein eigener,
geprüfter Installationsweg vorgesehen.

## Abnahmepunkte auf echter Hardware

- **Start und Matrix:** MCU-Version erfolgreich abgefragt, richtige Pixelreihenfolge,
  alle vier Ecken korrekt, keine verschobenen Farben, kein Flackern.
- **Bedienung:** Drehregler verändert Helligkeit in beide Richtungen; Bestätigung
  wirkt auf denselben Alarm wie in der Weboberfläche. Links/rechts prüfen.
- **Audio:** Lautstärke 0 bleibt still, kurze Testtöne funktionieren, lokale/Web-
  Bestätigung beendet die Ausgabe. Erst mit künstlichen Alarmdaten testen.
- **WLAN und Zeit:** Verbindung nach Neustart, gültige UTC-Zeit vor HTTPS, Verbindung
  nach WLAN-Unterbrechung wiederhergestellt. Hostname und Zertifikat werden geprüft.
- **Owlet:** EU-/internationale Anmeldung, Token-Erneuerung und Mehrgeräteauswahl;
  gleiche neue Messwerte bleiben gültig, gecachte alte Messwerte werden verborgen.
- **Zustände:** Socke laden, abnehmen, wieder anlegen, Basisstation ausschalten,
  Cloud nicht erreichbar; letzte kritische Hinweise bleiben als solche erkennbar.
- **Speicherung:** Einstellungen bleiben nach Neustart erhalten; Abbruch während
  Speichern hinterlässt eine ganze alte oder neue Datei. Kennwörter fehlen in API
  und Logs. Kein Geheimnis in Build, Manifest oder Fehlerbericht.
- **Ressourcen:** CPU/RAM, Dateideskriptoren, Temperatur, Audio-/MCU-Verhalten und
  Oberfläche im mindestens mehrstündigen Betrieb beobachten.
- **Rückkehr:** Wiederherstellung der Herstelleranwendung und Verhalten nach
  vollständigem Stromverlust tatsächlich überprüfen.

## Noch keine Freigabe

Ein permanentes `update.img`, automatischer Start nach Stromverlust,
unterbrechungssichere Aktualisierung, Online-Updatekanal und nutzerfreundlicher
öffentlicher Installer folgen erst nach diesen Prüfungen. Dafür keine TC001-
Dateien oder vorhandenen owlanzi.com-Endpunkte wiederverwenden.
