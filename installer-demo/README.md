# Lokale HTML-Installationsdemo

`dist/index.html` ist eine vollständige, eigenständige HTML-Datei. Sie funktioniert
auch direkt als Datei im Browser, ohne Paketinstallation oder Internetzugang.

Alternativ im Repository:

```powershell
node installer-demo/serve.mjs
```

Dann `http://127.0.0.1:8091` öffnen. Der Server lauscht ausschließlich lokal und
liefert die HTML-Datei aus. Er hat keine API, ADB-Verbindung oder Installationsroute.

Die Demo zeigt Vorbereitung, Start eines künftigen Installationshelfers,
IP-Eingabe, Prüfungen, temporären Start und anschließende Einrichtung. Alle
Prüfergebnisse, Fortschritte und Geräteanzeigen sind simuliert. Über
„Beispielablauf“ lassen sich eine nicht erreichbare Uhr, ein inkompatibles System
und eine nicht antwortende Anwendung nachvollziehen. Keine echten Zugangsdaten
eingeben. Die Beispieldaten werden nicht gespeichert oder übertragen.

## So würde daraus ein echter Installer

1. Ein einmalig heruntergeladener lokaler Helfer startet auf dem Computer und
   stellt den Assistenten selbst unter `127.0.0.1` bereit. owlanzi.com könnte
   später den Download anbieten; die Website bleibt derzeit unverändert.
2. Der Nutzer gibt die IP seiner TC002 ein. Der Helfer bindet die Prüfungen aus
   `scripts/local-device.py` an: Modell, WLAN-ADB, ARM-Schnittstellen, Dateiliste
   und SHA-256. Die jetzige Demo verwendet diesen Helfer ausdrücklich nicht.
3. Eine an IP, Paket-Hash und aktuellen Prüfstand gebundene Freigabe entsteht erst
   beim Klick auf den Teststart. Der Helfer akzeptiert keine beliebigen Befehle,
   URLs oder Dateipfade aus der Webseite. API-Aufrufe werden lokal authentifiziert
   und auf die eigene Origin begrenzt.
4. Der Helfer überträgt das geprüfte Paket temporär, startet es und bestätigt
   Erfolg erst nach einer authentifizierten Antwort der Geräteoberfläche. Bei
   Fehlern bleiben Diagnose und gezielte Wiederherstellung zugänglich.
5. Die lokale Geräteoberfläche erhält den Kopplungscode über einen vorgesehenen
   lokalen Ablauf; kein Token wird in öffentliche URLs geschrieben. Owlet-
   Zugangsdaten werden ausschließlich in der Geräteoberfläche eingegeben.

Dieser Browser-Adapter des Helfers ist noch zu implementieren. Der vorhandene
CLI-Helfer und das ARM-Paket sind die technische Grundlage. Ein permanentes
Firmware-Image und die Rückkehr nach Stromverlust müssen an der echten Uhr
gesondert geprüft werden.

Normale Webseiten können keine freien TCP-Verbindungen für WLAN-ADB öffnen;
der lokale Helfer überbrückt diese Browsergrenze. Siehe
[Chrome: Direct Sockets](https://developer.chrome.com/docs/iwa/direct-sockets).
Der native TC002-Entwicklungsweg ist im
[offiziellen Ulanzi-Projekt](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002)
dokumentiert.

Lizenz: GPL-3.0-or-later wie die App.
