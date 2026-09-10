# TC002 App-Updates

Die TC002 sucht standardmäßig einmal täglich bei owlanzi.com nach einer neuen
App-Version. Unter **System → App-Updates** lässt sich die Suche abschalten,
manuell ausführen und ein gefundenes Update installieren. Die Installation
startet erst nach Bestätigung. WLAN, Owlet-Konto, Display-Farben, Helligkeit,
Alarme, Zeitzone und Web-Passwort bleiben erhalten.

## Aktuelle lokale Installation

Die Version 0.2.x ist ein App-Update für die bereits getestete TC002 mit
Z21/Stock-Firmware 1.0.1. Sie ist kein `update.img` und kein TC001-ESP32-Image.
Ein kompletter Stromausfall startet weiterhin die Hersteller-App. Der
dauerhafte Boot-Einstieg ist eine separate, noch nicht abgeschlossene Aufgabe.
Die beiden App-Versionen und die Einstellungen liegen trotzdem dauerhaft in
`/data`. Ein App-Neustart während OTA funktioniert ohne Computer.

Den OTA-Starter einmalig nach einer erfolgreichen lokalen Testinstallation
einrichten (auf dem Rechner mit Python und ADB):

```powershell
python scripts/build-tc002.py --offline --device-abi-dir .local/device-192.168.100.235/abi
python scripts/bootstrap-ota.py install --ip 192.168.100.235 --adb <Pfad-zu-adb.exe>
```

Nach vollständigem Stromverlust die zuletzt bestätigte Version wieder starten:

```powershell
python scripts/bootstrap-ota.py resume --ip 192.168.100.235 --adb <Pfad-zu-adb.exe>
```

Der Starter prüft Board, Peripherie, verfügbare Bibliothekssymbole und Platz für
zwei Versionen. Er installiert ausschließlich unter `/data/owlanzi-app` und
wechselt die temporäre `/tmp/EasyUI.cfg`. Er flasht keine Partitionen.

## Update und Rückfall

- Metadaten: `https://owlanzi.com/firmware/ota-tc002.json`.
- Images: `owlanzi-tc002-VERSION-ota.bin`, maximal 4 MiB.
- Eigener Pakettyp `tc002-app-bundle`, ABI `z21-stock-1`, Loader-Protokoll 1.
- Feste Dateiliste: `lib/libzkgui.so`, `ui/main.ftu`, `ui/cacert.pem`.
- TLS prüft Zertifikat und Hostnamen; keine Weiterleitungen oder fremden URLs.
- Größen, vollständige SHA-256-Prüfsumme, Einzeldatei-Hashes, ARM/Hardfloat-ELF
  und nach dem Schreiben erneut die Dateien werden geprüft.
- Downloads liegen vorübergehend unter `/tmp`; geschrieben wird ausschließlich
  in den inaktiven Slot `a` oder `b`. Benutzerdaten liegen unter `/data/owlanzi`
  und `/data/misc/wifi` und sind niemals Bestandteil des Pakets.
- Ein unabhängiger Starter wechselt die App. Die neue Version muss nach zehn
  Sekunden laufender Anzeige regelmäßig ihre Slot-/Versionskennung bestätigen.
  Fehlt sie 45 Sekunden lang, startet der Starter die vorherige App wieder.
- **Vorherige Version wiederherstellen** erlaubt auch einen manuellen Rückfall.
  Bei kritischem Alarm oder fehlendem Heim-WLAN ist Installation gesperrt;
  ein Download wird bei einem neuen Alarm oder WLAN-Verlust abgebrochen.

Prüfsummen sichern Integrität im durch HTTPS authentifizierten Updatekanal.
Es gibt noch keine davon unabhängige Offline-Signatur oder Secure-Boot-Kette.
Die garantierte Rückkehr betrifft einen fehlgeschlagenen App-Start, nicht einen
Ausfall des gesamten Linux-Systems oder einen Stromverlust.

## Tägliche Suche und Zählung

Die erste automatische Suche erfolgt frühestens nach 60 Sekunden Laufzeit,
bei gültiger Internet-/Gerätezeit und Verbindung mit dem Heim-WLAN. Der Tag
nach Europe/Berlin wird **vor** dem Abruf in `update-check.json` gespeichert.
Ein Neustart, Rückstellen der Uhr oder fehlgeschlagener Abruf wiederholt daher
die Tagesmarkierung nicht. Die manuell eingestellte Anzeigezeit beeinflusst
diese Prüfung nicht. Manuelle Prüfungen bleiben jederzeit möglich.

Ab App 0.2.2 tragen automatische Abrufe `daily-update-check=1`, `model=tc002`
und die installierte App-Version als `version`. Die Website zählt
aggregierte Tagesprüfungen, OTA-Prüfungen und Downloads je Dateiname, ohne
Geräte-ID, Konto, Cookies oder individuelle Nutzungspfade. Bestehende
`DNT`, `Sec-GPC` und `no_stats=1`-Ausschlüsse bleiben erhalten. Abgeschaltete
Suche löst keine automatischen Abrufe aus; manuelle Downloads werden gezählt.

## Paket und Veröffentlichung

```powershell
python scripts/build-tc002.py --offline --app-version 0.2.1 --device-abi-dir .local/device-192.168.100.235/abi
python scripts/package-ota.py
# Im eigenständigen Website-Repository:
python website-tools/deploy-tc002.py
python website-tools/deploy-tc002.py --publish
```

Der Paketierer erzeugt Image, Metadaten und GPL-Quellarchiv unter `dist/ota`.
Er nimmt nur explizite Quellverzeichnisse auf; Gerätebibliotheken, Zugangsdaten,
SDK-Archive, Build-Verzeichnisse und lokale Arbeitsdaten sind ausgeschlossen.
Der Linker-Map-Nachweis und `/licenses.txt` dokumentieren eingebundene Komponenten.

Der Publisher verwendet SSH-Host `allinkl`, erstellt zuerst einen lokalen Plan,
prüft erneut alle Live-Prüfsummen, sichert die bisherigen Dateien unter dem
HTTP-gesperrten `.release-history` und schaltet das TC002-Manifest zuletzt um.
Veröffentlichte Versionsdateien sind unveränderlich. TC001-/ESP32-Manifeste,
Website-Inhalte und Betreiberkonfiguration werden als unverändert geprüft.
