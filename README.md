# Owlanzi TC002 App

Eine eigenständige Owlanzi-Anwendung für die **Ulanzi TC002**. Sie läuft auf der
Uhr und fragt die Owlet-Cloud direkt ab. Im Betrieb werden weder Computer noch
Home Assistant, MQTT-Broker oder zusätzlicher Server benötigt.

**Entwicklungsstand: 0.1.0-dev.** Native PC-Anwendung und ARM-/FlyThings-Anwendung
werden aus derselben C++17-Logik gebaut. Die erste Prüfung auf einer echten TC002
steht noch aus. Ein erfolgreiches Cross-Compile bestätigt nicht die Funktion der
Matrix, Tasten, Audioausgabe, WLAN-Verbindung oder Wiederherstellung am Gerät.

Die Uhr ergänzt die Owlet-Basisstation. Sie ersetzt weder deren Alarme noch die
Basisstation und ist kein zertifiziertes medizinisches Gerät.

## Lokal ausprobieren

Windows benötigt Visual Studio Build Tools mit C++ und CMake, Python 3 und Node.js
für die Tests. Auf dem vorhandenen Entwicklungsrechner sind sie verfügbar.

```powershell
./scripts/build-local.ps1 -Run
```

Dann `http://127.0.0.1:8080` öffnen und den im lokalen Terminal angezeigten
Kopplungscode einfügen. Mit `Strg+C` beenden. Die Simulation verwendet ausschließlich
Beispieldaten und kontaktiert die Owlet-Cloud auch dann nicht, wenn Zugangsdaten
eingegeben werden. Über die Oberfläche lassen sich Messwerte, Laden, Warten,
Offline und Alarm simulieren. **Vorschau aufhellen** verändert nur die Darstellung
im Browser; die Helligkeit der Uhr bleibt davon unabhängig.

Nach einem Build reicht:

```powershell
./build/owlanzi-tc002.exe --demo --show-token
```

Linux, beispielsweise für Entwicklung oder CI:

```sh
# C++17 compiler, CMake, Python 3, libcurl development headers, Node.js
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
node --test tests/web-tests.mjs
./build/owlanzi-tc002 --demo --show-token
```

Ein bewusster Start mit `--live --data-dir .local/live --show-token` aktiviert
echte Cloud-Abfragen, sobald ein Konto in der lokalen Oberfläche gespeichert ist.
Die Entwicklungsprüfung dieses Repositories verwendet diesen Modus nicht.

## Funktionsumfang

- Direkte Owlet-Anmeldung für Europa und die internationale Region, Token-Erneuerung
  und ausdrückliche Auswahl bei mehreren gekoppelten Geräten.
- Puls, Sauerstoff, Schlafstatus und Sockenakku auf einer eigenen 52×16-Anzeige.
- Datenalter anhand des Cloud-Messzeitpunkts, neue Sitzung nach Laden/Abnehmen,
  Ausblenden veralteter Messwerte und Beibehalten des letzten kritischen Alarms.
- Owlet-Alarmhinweise und optionale eigene Schwellwertregeln; eigene Regeln sind
  standardmäßig ausgeschaltet. Bestätigung über Weboberfläche oder Hardwaretaste.
- Lokale deutsche/englische Oberfläche, Live-Spiegel, Farben, Helligkeit, Klang,
  Abfrageintervall und Kontoeinrichtung.
- TC002-Adapter für MCU-Initialisierung, SPI-Matrix, Tasten/Drehregler und PCM-Ton.
- Persistente Einstellungen; atomarer Austausch der Konfigurationsdatei,
  unter Linux Verzeichnisrechte `0700` und Dateirechte `0600`.

WLAN wird für den ersten Gerätetest über Ulanzi eingerichtet; die Anwendung nutzt
die gespeicherte Systemkonfiguration. Ein eigener WLAN-Einrichtungsassistent,
verifizierte Dauerinstallation und Online-Updates sind **noch nicht freigegeben**.
Die TC001-Website und ihre Installer-/OTA-Dateien wurden nicht verändert.

## TC002 bauen und später testen

Der offizielle Windows-Cross-Compiler und benötigte FlyThings-Pakete werden nach
festgehaltenen SHA-256-Prüfsummen lokal bezogen. Details und exakte Befehle:
[TC002_PLATFORM.md](docs/TC002_PLATFORM.md).

Das Ergebnis ist eine native ARM-Bibliothek `libzkgui.so` mit den benötigten
UI-Ressourcen und einem Paketmanifest. Es ist **kein ESP32-Image** und darf nicht
mit dem TC001-Webflasher oder dessen OTA-Funktion installiert werden.

Der erste Hardwareversuch verwendet ausschließlich das temporäre FlyThings-
Debugverfahren. Die Schritte und Abnahmepunkte stehen in
[LOCAL_TESTING.md](docs/LOCAL_TESTING.md). Es wurden keine Geräte verbunden,
verändert oder geflasht. Eine spätere dauerhafte Installation braucht ihren
eigenen Stromausfall-, Update- und Recovery-Test.

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

Der Browser muss sich mit dem lokalen Kopplungscode authentifizieren; APIs geben
keine gespeicherten Passwörter oder Cloud-Tokens zurück. Das Gerät verwendet für
die Einrichtung derzeit HTTP im eigenen WLAN, auf Port `8080`. Kopplung ersetzt
keine Transportverschlüsselung. Der Dienst gehört nicht ins Internet und braucht
keine Portfreigabe. Auf dem PC ist er standardmäßig nur an `127.0.0.1` gebunden.
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
