# Uhrzeit auf der TC002

In der Messwertansicht ersetzt **HH:MM** den kleinen Akku unten rechts. Die
Uhrzeit belegt x=33 bis 51, y=9 bis 13 und hat eine eigene Farbe `clock`.
Die große Akkuanzeige beim Laden bzw. bei ausgeschalteter Socke bleibt erhalten.
Der Sockenakku bleibt ebenfalls als Statuswert im Webinterface verfügbar.

Unter **System → Uhrzeit**:

- **Automatisch:** Die TC002 fragt `pool.ntp.org` per SNTP ab, nach erfolgreicher
  Synchronisation alle sechs Stunden, bei Fehlern frühestens nach einer Minute.
  Dazwischen läuft die Anzeige mit der monotonen Uhr weiter. Vor der ersten
  Synchronisation dient die vorhandene Gerätezeit als Grundlage. Quelle und
  Synchronisationsfehler werden im Webinterface angezeigt.
- **Zeitzone:** Vorauswahl Europe/Berlin; Auswahl aus 598 IANA-Namen oder Übernahme
  der im Browser eingestellten Zeitzone. Bei der erstmaligen gemeinsamen
  WLAN-/Owlet-Einrichtung wird die Browserzeitzone übernommen, sofern unterstützt.
  Diese Erkennung benötigt weder Standortberechtigung noch einen externen Dienst.
- **Manuell:** Datum und Uhrzeit in der ausgewählten Zeitzone eingeben oder die
  aktuelle Handyzeit übernehmen und speichern. Ungültige Daten und beim
  Frühlingswechsel übersprungene Zeiten werden abgewiesen. Bei einer doppelt
  vorkommenden Herbstzeit wird das erste Vorkommen verwendet.

Die Anzeigezeit ist von der System-UTC für TLS und Owlet-Messwertalter getrennt.
Manuelle Änderungen verändern keine Messzeitstempel, Alarmdauern oder
Aktualitätsprüfungen. Im laufenden Betrieb zählt die manuelle Zeit monoton weiter.
Für einen späteren App-Start werden Zeitanker gespeichert; die zwischenzeitlich
vergangene Zeit wird aus der Geräte-Systemzeit abgeleitet. Der Erhalt der
Systemzeit bei vollständigem Stromausfall bleibt Teil der Hardwareabnahme.

Die Zeitzonenregeln stammen aus [IANA tzdata 2026c](https://www.iana.org/time-zones).
Sommer-/Winterzeitwechsel für 2020–2099 sind als kompakte, gemeinsam verwendete
Tabellen eingebettet. Ein Browser muss dazu nicht geöffnet bleiben. Politische
Regeländerungen benötigen aktualisierte Daten mit einer neuen App-Version.
Regenerierung: `python scripts/generate-timezones.py` mit `tzdata==2026.3`.
Normale Builds benötigen dieses Python-Paket nicht und verwenden die Tabelle
aus dem Repository. Zeitberechnungen berücksichtigen die 32-Bit-TC002-Plattform.

## Prüfung am 9. September 2026

Sieben CTest-Gruppen und elf Node-Tests bestanden. Tests decken EU-/US-Umstellungen,
halbstündige Offsets, Tageswechsel, 2050/2100-Kalendergrenzen, ungültige manuelle
Zeit, Wiederherstellung der gespeicherten Anker und die Farb-/Pixelzuordnung ab.
API- und mobile Browserprüfungen bestätigen manuelle/automatische Einstellung
sowie den gemeinsamen WLAN-/Owlet-Abschluss.

Der ARM-Build wurde auf der echten TC002 installiert und per Transfer-Hash und
Geräte-ABI geprüft. Dort ist automatische NTP-Synchronisation mit Europe/Berlin
nachgewiesen; WLAN und Owlet sind verbunden. Bestehende Zugangsdaten, Palette
und Helligkeit wurden beim Update erhalten. Die physische Matrix wird über
denselben Renderer wie die geprüfte Geräte-API ausgegeben.
