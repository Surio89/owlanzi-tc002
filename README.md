# Owlanzi TC002 App

Eine eigenständige Owlanzi-Anwendung für die **Ulanzi TC002**. Sie läuft auf der
Uhr und fragt die Owlet-Cloud direkt ab. Im Betrieb werden weder Computer noch
Home Assistant, MQTT-Broker oder zusätzlicher Server benötigt.

**Entwicklungsstand: 0.2.2.** Native PC-Anwendung und ARM-/FlyThings-Anwendung
werden aus derselben C++17-Logik gebaut. Die Anwendung wurde auf einer echten TC002 gestartet; Setup-Anzeige, Owlet-Anmeldung
und Cloud-Abruf im Ladezustand sind geprüft. Vollständige Hardwareabnahme und
Dauerinstallation stehen noch aus.

Die Uhr ergänzt die Owlet-Basisstation. Sie ersetzt weder deren Alarme noch die
Basisstation und ist kein zertifiziertes medizinisches Gerät.

## Lokal ausprobieren

Windows benötigt Visual Studio Build Tools mit C++ und CMake, Python 3 und Node.js
für die Tests. Auf dem vorhandenen Entwicklungsrechner sind sie verfügbar.

```powershell
./scripts/build-local.ps1 -Run
```

Dann `http://127.0.0.1:8080` öffnen – ohne Passwort oder Einrichtungsschlüssel. Mit `Strg+C` beenden. Die Simulation verwendet ausschließlich
Beispieldaten und kontaktiert die Owlet-Cloud auch dann nicht, wenn Zugangsdaten
eingegeben werden. Über die Oberfläche lassen sich Messwerte, Laden, Warten,
Offline und Alarm simulieren. **Helligkeit simulieren** verändert nur die Darstellung
im Browser; die Helligkeit der Uhr bleibt davon unabhängig.

Nach einem Build reicht:

```powershell
./build/owlanzi-tc002.exe --demo
```

Linux, beispielsweise für Entwicklung oder CI:

```sh
# C++17 compiler, CMake, Python 3, libcurl development headers, Node.js
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
node --test tests/web-tests.mjs
./build/owlanzi-tc002 --demo
```

Ein bewusster Start mit `--live --data-dir .local/live` aktiviert
echte Cloud-Abfragen, sobald ein Konto in der lokalen Oberfläche gespeichert ist.
Die Entwicklungsprüfung dieses Repositories verwendet diesen Modus nicht.

## Funktionsumfang

Eine durchklickbare Vorschau des geplanten Browser-Installers liegt unter
[installer-demo/dist/index.html](installer-demo/dist/index.html). Sie lässt sich
direkt im Browser öffnen oder mit `node installer-demo/serve.mjs` lokal auf
Port 8091 ansehen. Alle Verbindungen und Installationsschritte dort sind simuliert;
Details zum künftigen lokalen Web-Helfer stehen in
[installer-demo/README.md](installer-demo/README.md).

- Direkte Owlet-Anmeldung für Europa und die internationale Region, Token-Erneuerung
  und ausdrückliche Auswahl bei mehreren gekoppelten Geräten.
- Puls, Sauerstoff, Schlafstatus und Sockenakku auf einer eigenen 52×16-Anzeige.
- Uhrzeit unten rechts neben den Messwerten, automatische Internetzeit,
  Zeitzone inklusive Sommer-/Winterzeit und manuelle Einstellung. Der Akku
  bleibt in der Ladeanzeige sichtbar. [Details zur Uhrzeit](docs/CLOCK.md).
- Datenalter anhand des Cloud-Messzeitpunkts, neue Sitzung nach Laden/Abnehmen,
  Ausblenden veralteter Messwerte und Beibehalten des letzten kritischen Alarms.
- Owlet-Alarmhinweise und optionale eigene Schwellwertregeln; eigene Regeln sind
  standardmäßig ausgeschaltet. Bestätigung über Weboberfläche oder Hardwaretaste.
- Lokale deutsche/englische Oberfläche, Live-Spiegel, Farben, Helligkeit, Klang,
  Abfrageintervall und Kontoeinrichtung.
- TC002-Adapter für MCU-Initialisierung, SPI-Matrix, Tasten/Drehregler und PCM-Ton.
- Persistente Einstellungen; atomarer Austausch der Konfigurationsdatei,
  unter Linux Verzeichnisrechte `0700` und Dateirechte `0600`.

Die Anwendung besitzt eine eigene WLAN-Einrichtung mit dem offenen Hotspot
`owlanzi` und der Adresse `http://192.168.4.1`. Im Hotspot WLAN und Owlet eingeben
und gemeinsam speichern; erst dann wechselt die Uhr ins Heim-WLAN.
Bestehende WLAN-Profile werden weiterverwendet. Details:
[WIFI_ONBOARDING.md](docs/WIFI_ONBOARDING.md).
Der bisherige temporäre ADB-Installer benötigt weiterhin eine erreichbare Uhr;
die Erstinstallation ohne Hersteller-Onboarding und Dauerinstallation sind
noch offen. **OTA-App-Updates** mit täglicher Suche, erhaltenen Einstellungen
und Rückfall zur vorherigen Version sind auf der TC002 geprüft und auf
owlanzi.com verfügbar. Einrichtung und Grenzen: [OTA.md](docs/OTA.md).
Die TC001-Installer-/OTA-Dateien und die Website-Inhalte bleiben unverändert;
der zusätzliche TC002-Kanal verwendet die bestehende aggregierte Statistik.

## TC002 bauen und lokal testen

Der offizielle Windows-Cross-Compiler und benötigte FlyThings-Pakete werden nach
festgehaltenen SHA-256-Prüfsummen lokal bezogen. Details und exakte Befehle:
[TC002_PLATFORM.md](docs/TC002_PLATFORM.md).

Das Ergebnis ist eine native ARM-Bibliothek `libzkgui.so` mit den benötigten
UI-Ressourcen und einem Paketmanifest. Es ist **kein ESP32-Image** und darf nicht
mit dem TC001-Webflasher oder dessen OTA-Funktion installiert werden.

Der erste Hardwareversuch verwendet ausschließlich das temporäre FlyThings-
Debugverfahren. Die Schritte und Abnahmepunkte stehen in
[LOCAL_TESTING.md](docs/LOCAL_TESTING.md). Am 9. September 2026 wurde die Anwendung
erstmals temporär auf einer TC002 gestartet; Web-API und Speicherung funktionieren.
Die Setup-Anzeige wurde auf der Matrix bestätigt. Farben/Geometrie, Tasten,
Audio und Messungen während des Tragens müssen noch praktisch geprüft werden.
Eine spätere dauerhafte Installation braucht ihren
eigenen Stromausfall-, Update- und Recovery-Test.

## Oberfläche wie bei der TC001

Status, Display, Alarme und System verwenden das TC001-Layout und Branding.
Die 52×16-Liveansicht und Farbvorschauen kommen aus demselben TC002-Renderer.
Vorschauen auf der Uhr enden nach zehn Sekunden; kritische Alarme haben Vorrang.
Details und bewusste Geräteunterschiede: [UI_PARITY.md](docs/UI_PARITY.md).

## Aufbau

| Bereich | Aufgabe |
| --- | --- |
| `src/core.cpp`, `src/render.cpp` | Plattformunabhängige Zustände und Anzeige |
| `src/owlet.cpp`, `src/http.cpp` | Owlet-Protokoll und HTTPS mit Zertifikatsprüfung |
| `src/config.cpp`, `src/runtime.cpp` | Speicherung, Threads, lokale Web-API |
| `web/` | Eingebettete Oberfläche ohne CDN oder externe Assets |
| `platform/tc002/` | Native FlyThings-Anbindung der TC002 |
| `tests/` | Offline-Regressions- und Integrationstests |
| `.cache/`, `build/`, `.local/` | Ignorierte Abhängigkeiten, Builds und private Laufzeitdaten |

Die Oberfläche ist initial ohne Passwort erreichbar. Unter **System → Zugriff**
lässt sich optional ein eigenes Web-Passwort setzen (Benutzername `owlanzi`).
Es gibt keinen Einrichtungsschlüssel mehr. APIs geben gespeicherte Passwörter
oder Cloud-Tokens nicht zurück; schreibende Browser-Anfragen sind auf dieselbe
Herkunft beschränkt. Die lokale Oberfläche verwendet HTTP auf Port `80` und `8080` auf der TC002.
Der Dienst ist für das eigene WLAN gedacht und benötigt keine Portfreigabe.
Auf dem PC ist er standardmäßig an `127.0.0.1` gebunden.
Unter Windows gelten die Rechte des gewählten lokalen Datenverzeichnisses.

## Lizenz

**GPL-3.0-or-later**, siehe [LICENSE](LICENSE). Die Lizenzwahl gilt für dieses
eigenständige Repository. Das bestehende TC001-Repository behält seine Lizenz.
Die vom Projekteigentümer autorisierte Portierung übernimmt Owlanzi-Verhalten
und Teile seiner bestehenden Implementierung in diese getrennte Anwendung.

Abhängigkeiten behalten ihre eigenen Lizenzen, siehe
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Das Hersteller-SDK wird separat
bezogen und nicht als Quellbestandteil dieses Repositories ausgegeben. Vor einer
öffentlichen Binärveröffentlichung müssen die Weitergabebedingungen der konkret
verlinkten SDK-Komponenten geklärt und die erforderlichen Quellen bereitgestellt
werden. Das aktuelle Paket dient der lokalen Entwicklung.
