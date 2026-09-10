# Lokaler Test mit der ersten TC002

Stand 9. September 2026: temporärer Start auf echter TC002 erfolgreich.
Die native Web-API antwortet im Live-Modus, Einstellungen lassen sich speichern.
Die physischen Ausgaben und die Owlet-Anmeldung sind noch nicht abgenommen.

## Besonderheiten der ersten Uhr

Hersteller-App `1.0.1`, MCU `V1.0.16`, Z21/ARMv7, glibc 2.30. Diese Firmware
liefert in `/getBase` kein Modellfeld. Der Helfer prüft deshalb gemeinsam das
Hersteller-Antwortschema, Board/Hardware, die statische Ulanzi-Clock-Oberfläche
und die erwarteten SPI-, UART- und Eingabegeräte. Legacy-ADB meldet Remote-Fehler
nicht als Prozessstatus; der Helfer wertet einen expliziten Shell-Status aus.
Dateien werden zurückgelesen und lokal gehasht, da `sha256sum` fehlt.

Wenn die ABI-Prüfung bei den `exception_ptr`-Symbolen stoppt, wurden die nötigen
Gerätebibliotheken bereits nach `.local/device-<IP>/abi` gelesen. Dann bauen mit:

```powershell
python scripts/build-tc002.py --offline --device-abi-dir .local/device-<IP>/abi
python scripts/local-device.py run --ip <IP>
```

Dadurch wird ausschließlich gegen die echte `libstdc++.so.6` der Uhr gelinkt;
sie wird nicht mitgeliefert oder ersetzt. Die Dateispeicherung verwendet unter
Linux POSIX-Aufrufe, um die inkompatiblen GCC-8/9-Dateisystemtypen zu vermeiden.
Schlägt die Startprüfung nach Aktivierung fehl, stellt der Helfer die
Herstelleranwendung automatisch wieder her.

## Auf dem PC

`scripts/build-local.ps1 -Run` baut, testet und startet die isolierte Simulation.
Browser unter `http://127.0.0.1:8080` direkt ohne Kopplungsschritt öffnen.

Prüfen: Messwerte, Ladeanzeige, unbekannter Schlafzustand, Offline-Übergang,
Alarm bestätigen, Kontoeinstellungen speichern, Passwort leer lassen/gezielt
löschen, deutsche/englische Ansicht, Helligkeit 0 und schmale Bildschirmbreite.
Simulation ist deutlich markiert; sie führt niemals Cloud-Anfragen aus.

## Wenn die Uhr da ist

1. Ulanzi-Firmwareversion und Gerätemodell notieren. Für den derzeitigen
   temporären ADB-Installer müssen Uhr und PC bereits im selben lokalen Netz
   sein. Nach Installation kann Owlanzi WLAN selbst einrichten. Der
   Installationsweg für ein noch nicht eingerichtetes Gerät ist separat offen.
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
6. Die lokale Oberfläche der Uhr auf `http://<IP>:8080` ohne Einrichtungsschlüssel
   öffnen. Unter System bei Bedarf WLAN, danach das Owlet-Konto einrichten. Ein eigenes Web-Passwort
   ist optional; bestehende alte Pairing-Dateien werden nicht mehr verwendet.
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

Ein permanentes `update.img`, automatischer Start nach Stromverlust und ein
öffentlicher Erstinstallations-Assistent bleiben offen. Der separate TC002-
OTA-App-Kanal ist inzwischen implementiert: [OTA.md](OTA.md). Installation,
manueller Rückfall und ausbleibende Startbestätigung wurden auf der Uhr geprüft.
Das ersetzt keinen Stromausfalltest des vollständigen Linux-/Bootsystems.
