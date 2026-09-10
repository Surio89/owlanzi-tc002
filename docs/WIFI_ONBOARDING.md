# WLAN direkt mit Owlanzi einrichten

Sobald Owlanzi auf der TC002 installiert ist, braucht die WLAN-Einrichtung keine
Herstelleroberfläche mehr. Ein bereits eingerichtetes WLAN wird weiterverwendet.
Ohne Verbindung öffnet die Anwendung nach etwa 20 Sekunden und der Netzwerksuche
den offenen Hotspot **owlanzi**. Einrichten ohne Schlüssel oder initiales Web-Passwort:

1. Handy mit `owlanzi` verbinden; trotz fehlendem Internet verbunden bleiben.
2. Die Einrichtungsseite öffnen. Falls das Handy sie nicht automatisch anzeigt:
   `http://192.168.4.1` im Browser öffnen.
3. Unter **System → 1 · WLAN verbinden** Netzwerk auswählen oder SSID eingeben,
   WLAN-Passwort eintragen und **Weiter zum Owlet-Konto** drücken. Der Hotspot
   bleibt dabei geöffnet. Unter **2 · Owlet-Konto** E-Mail, Passwort und Region
   eintragen. **WLAN und Owlet speichern · verbinden** schließt beide Schritte ab.
4. Die Owlet-Zugangsdaten werden vor dem WLAN-Wechsel gespeichert; die Anmeldung
   wird nach der Verbindung mit dem Heim-WLAN automatisch versucht.
   Der Hotspot endet während des Verbindungsversuchs. Handy wieder mit dem
   Heim-WLAN verbinden. Die neue IP erscheint für 20 Sekunden auf der Matrix.
5. Die Uhr über diese Adresse öffnen. Bei mehreren Owlet-Geräten das gewünschte
   Gerät auswählen; bei falschen Owlet-Zugangsdaten diese unter System korrigieren.

Der TC002-Webserver ist im Heim-WLAN auf Port 80 und weiterhin 8080 erreichbar.
Alle Oberflächen-Dateien liegen auf der Uhr; der Hotspot benötigt kein Internet.

## Erneut einrichten und Fehlerbehandlung

Unter **WLAN neu einrichten** lässt sich der Hotspot für fünf Minuten öffnen.
Alternativ mittlere Taste sechs Sekunden halten und loslassen. Ohne Eingabe
kehrt die Uhr danach zur bisherigen Verbindung zurück. Beim ersten Start ohne
erreichbares Profil bleibt der automatisch geöffnete Hotspot ohne Zeitlimit offen.

Ein Verbindungsversuch wartet bis zu 45 Sekunden. Bei einem Fehlschlag stellt
Owlanzi das vorherige WLAN-Profil wieder her und versucht es erneut. Ist dieses
nicht erreichbar, öffnet sich nach weiteren etwa 20 Sekunden wieder der Hotspot.
Auch ein unterbrochener Wechsel wird beim nächsten App-Start zurückgenommen.

Kritische Alarme haben Vorrang vor der WLAN-Anzeige und blockieren einen manuell
ausgelösten Netzwerkwechsel. Ein Reset der Owlanzi-Einstellungen löscht das WLAN
nicht. Ein später gesetztes optionales Web-Passwort gilt auch im Hotspot.

## Technische Umsetzung und Grenzen

- `WifiService` steuert die Abläufe in einem eigenen Thread; HTTP und Anzeige
  warten nicht auf WLAN-Aufrufe. Passwörter fehlen in Status und Konfigurations-API.
- Der native Adapter verwendet den installierten SDK-`WifiManager` für WLAN.
  Eigene `hostapd`-/`dnsmasq`-Prozesse übernehmen den offenen AP, DHCP und DNS.
  Eigene Konfigurationen liegen mit restriktiven Rechten unter `/data/owlanzi`.
  Systemprogramme und Hersteller-AP-Konfiguration werden nicht ersetzt.
- Die TC002 verwendet ein Funkinterface. Während des Hotspots ist keine neue
  Suche vorgesehen: Die Liste stammt von der Suche unmittelbar vor dem Wechsel.
  Manuelle SSID-Eingabe ist immer möglich. Unterstützt werden offene Netze und
  WPA/WPA2-Personal; keine Unternehmensnetze, WEP oder reines WPA3.
- Verbindung und Hotspot wechseln sich ab; das Handy verliert dabei kurzzeitig
  die Verbindung zur Oberfläche. DNS zeigt im Hotspot auf `192.168.4.1`, unbekannte
  HTTP-Prüfpfade leiten dorthin um. Automatisches Öffnen hängt vom Handy ab.
- **Erstinstallation bleibt getrennt:** Der aktuelle temporäre ADB-Installer
  benötigt eine bereits im LAN erreichbare Uhr. Ein dauerhaft installierbares
  Paket für fabrikneue Geräte ohne vorherige Hersteller-Einrichtung muss noch
  umgesetzt und geprüft werden. Diese Änderung löst das WLAN-Onboarding der App,
  nicht diesen ersten Übertragungsweg. owlanzi.com wird nicht geändert.

SDK-Referenzen: [WLAN](https://docs.flythings.cn/zh-hans/wifi.html),
[Hotspot](https://docs.flythings.cn/zh-hans/wifi_ap.html). Der SDK-Hotspotmanager
verlangt ein Passwort mit mindestens acht Zeichen und verwendet ein anderes
Subnetz; deshalb verwendet Owlanzi die vorhandenen Systemdienste direkt.

## Nachgewiesene Prüfung am 9. September 2026

- Sechs CTest-Gruppen und elf Node-Tests bestanden, ARM-Build und Prüfung gegen
  die exportierten Symbole der tatsächlichen Gerätebibliotheken bestanden.
- Simulation: automatischer Hotspot ohne Profil, Auswahl/Verbindung, falsches
  Passwort mit Rückkehr, Abbruch, Passwortausblendung und HTTP-Zugriffsschutz.
- Echte TC002: bestehendes WLAN nach App-Update erhalten, Oberfläche auf Port 80
  und 8080 erreichbar, WLAN-Scan liefert Netzwerke. Hotspot für zehn Sekunden
  aktiviert; automatische Rückkehr nach insgesamt etwa 21 Sekunden, kein Fehler.
- Offen: vollständige Handy-Verbindung inklusive DHCP/DNS und automatischem
  Öffnen, Eingabe eines echten WLAN-Passworts, physischer Langdruck, Erststart
  ohne Profil und Stromausfall während eines Wechsels auf echter Hardware.

`tests/browser-wifi-check.js` prüft ausschließlich auf einer Demo-Instanz die
Bedienung einschließlich Scan-Rückmeldung, Auswahl, Passwortsichtbarkeit,
Verbindungsfehler und unveränderten Owlet-/Display-Einstellungen.
