# TC002-Plattform und Gerätebuild

Der Gerätebuild ist eine native C++17-Anwendung für das vorhandene Linux-/FlyThings-System des TC002. `libzkgui.so` enthält Owlet-Abfragen, Konfiguration, lokale Weboberfläche, Alarmregeln und Anzeige. Nach der Installation ist kein Rechner oder externer MQTT-Dienst erforderlich. Ein ESP32-Binary für das TC001 ist auf dieser Plattform nicht verwendbar.

Die Anwendung wurde mit der offiziellen Z21-Toolchain als ARM-ELF erfolgreich kompiliert und vollständig gelinkt. Auf echter Hardware wurde sie noch nicht ausgeführt. Die erste Installation, die Ausgabe auf der Matrix, Audio, WLAN-Wiederverbindung und der Start nach Stromtrennung sind deshalb offene Gerätetests.

## Quellen und feste Versionen

Hardwaregrundlage ist das [offizielle Ulanzi-Projekt](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/tree/fa9d85d8e639430332c117cac71ce08dd6beb3f7), Commit `fa9d85d8e639430332c117cac71ce08dd6beb3f7`. Dessen [Hardwarebeschreibung](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/Z21_TC002_Demo/README.md) und [IDE-/Installationsdokumentation](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/IDE%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E/%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.md) beschreiben die verwendeten Schnittstellen.

`cmake/tc002-dependencies.json` fixiert Toolchain, alle verwendeten FlyThings-Pakete und den Mozilla-CA-Bestand mit HTTPS-Quelle und SHA-256. Downloads werden unter `.cache/tooling/` gespeichert; sie gehören nicht ins Git-Repository. Das Skript prüft Archive vor dem Entpacken und bricht bei geänderten Dateien ab.

| Komponente | Verwendete Version |
| --- | --- |
| Offizielle Z21-Toolchain für Windows | Linaro GCC 8.3.0, `arm-pc-linux-gnueabihf` |
| CPU/ABI | ARMv7-A, NEON, EABI5, Hard-Float, unsigned `char` |
| FlyThings EasyUI | 2.6.0 |
| base-utility / audio-utility | 10.9.3 / 5.1.1 |
| HTTPS | libcurl 8.12.1-mbedtls, mbedTLS 3.6.5 |
| DNS-Auflösung im SDK-curl | c-ares 1.17.2 |
| CA-Bestand | Mozilla/curl, 2026-08-13 |

Die Versionen stammen aus dem Hersteller-SDK. Insbesondere c-ares ist alt; vor einer öffentlichen Freigabe sollten die Netzwerkbibliotheken aus aktuellen, geprüften Quellen gebaut werden. Die alternative OpenSSL-Ausgabe von SDK-curl wurde gegen OpenSSL 1.1 gebaut und ist nicht mit dem dort angebotenen OpenSSL-3-Archiv austauschbar. Deshalb verwendet dieser Build ausdrücklich die mbedTLS-Ausgabe.

## Bauen

Voraussetzungen auf dem Build-PC: Windows, Python ab 3.11, CMake ab 3.20 und Ninja. CMake/Ninja werden auch in einer vorhandenen Visual-Studio-Build-Tools-Installation gefunden. Es wird weder die FlyThings-IDE installiert noch ein Treiber benötigt.

```powershell
python scripts/build-tc002.py
```

Der erste Aufruf lädt die festgelegten Herstellerpakete und die Toolchain. Danach konfiguriert er CMake, kompiliert `zkgui`, prüft beim Linken alle Anwendungsreferenzen und erzeugt ein lokales Testpaket. Weitere Optionen:

```powershell
python scripts/build-tc002.py --prepare
python scripts/build-tc002.py --offline
python scripts/build-tc002.py --cmake C:/Tools/CMake/bin/cmake.exe --ninja C:/Tools/ninja.exe
```

`--offline` benutzt bereits entpackte Dateien und prüft deren Archive nicht erneut. Für eine erneute vollständige Hashprüfung zuerst `--prepare` ausführen. Gleiche Eingaben und Werkzeugversionen sind fixiert; bitgenaue Reproduzierbarkeit auf einem zweiten Rechner wurde noch nicht geprüft.

Der Hersteller dokumentiert auch eine Linux-SSD-Toolchain. Der hier verifizierte automatische Build verwendet die Windows-Z21-Toolchain. Ein Linux-Build wurde nicht überprüft.

## Start, Display und Bedienung

FlyThings lädt `libzkgui.so` und ruft `onEasyUIInit`, `onStartupApp` und `onEasyUIDeinit` auf. Direkt im Init-Hook wird die vom Hersteller verlangte Eigenschaft `sys.zkapp.state=running` gesetzt. Die Anwendung startet anschließend ihre Arbeit in einem eigenen Thread; lange Netzwerkaufrufe blockieren nicht die EasyUI-Ereignisschleife. Eine minimale, registrierte `mainActivity` verwendet die unveränderte leere `main.ftu` aus dem Ulanzi-Projekt.

Die Geräteanbindung verwendet:

- MCU `/dev/ttyS1`, 1.500.000 Baud, 8N1 ohne Hardware-Flusskontrolle. Nach jedem Start muss die Versionsabfrage `ff 55 11 00 01 65` erfolgreich beantwortet werden, bevor die Matrix angesprochen wird. Antworten werden begrenzt und mit Prüfsumme gelesen.
- Logische 52×16 RGB888-Pixel werden auf ein 64×16-RGB-SPI-Paket mit schwarzen Zusatzspalten abgebildet. SPI0 arbeitet mit 10 MHz, Mode 0; GPIO_35 wird vor dem Schreiben auf 0 und danach auf 1 gesetzt. Zwischen Frames liegen mindestens 16 ms.
- Eingänge `/dev/input/event67` und `/dev/input/event68`. Drehung rechts oder rechte Taste erhöht die Helligkeit, Drehung links oder linke Taste senkt sie. Drehreglerdruck und mittlere Taste quittieren den Alarm.
- Audio über die öffentliche SDK-`AudioPlayer`-Schnittstelle: eigener kurzer Doppelton, PCM mono/16 kHz/S16; Lautstärke 0–6 gemäß Herstellerzuordnung. Quittierung und deaktivierter Ton stoppen die Wiedergabe.
- WLAN läuft über `WifiManager` für Stationsbetrieb und die vorhandenen Systemprogramme `hostapd`/`dnsmasq` für den eigenen offenen Hotspot. Die SDK-Funktion `wifi_load_driver()` lädt den vorhandenen Treiber nach dem Abschalten des Stationsbetriebs. Eigene AP-Konfiguration unter `/data/owlanzi`, keine ersetzten Systemprogramme. Ablauf und Grenzen: [WIFI_ONBOARDING.md](WIFI_ONBOARDING.md).

Ein nicht ansprechbares Panel verhindert den Start der lokalen Weboberfläche nicht. Das erlaubt bei einem ersten Hardwaretest Diagnose und Konfiguration trotz abweichender Hardwaredetails. Ein Startfehler wird ohne Zugangsdaten in das Prozesslog geschrieben.

## Daten und lokale Testdateien

Die Anwendung verwendet standardmäßig `/data/owlanzi`; `OWLANZI_DATA_DIR` kann das für einen Debug-Start ersetzen. `/data` wird vom SDK für persistente Daten verwendet, etwa `/data/misc/wifi/wpa_supplicant.conf`. Die konkrete freie Kapazität und der Erhalt nach Hersteller-Updates müssen auf dem eigenen TC002 geprüft werden. Konfiguration und Pairing-Datei gehören nicht in das Anwendungspaket.

Der Build erzeugt:

```text
build/tc002/device/
  EasyUI.cfg
  lib/libzkgui.so
  ui/main.ftu
  ui/cacert.pem
  manifest.json
build/tc002/owlanzi-tc002-app-0.2.1-local.zip
```

`EasyUI.cfg` ist für einen flüchtigen Test unter `/tmp/owlanzi-tc002-app` vorbereitet. Es verändert allein durch seine Erstellung keine Einstellung auf der Uhr. `manifest.json` enthält ausschließlich die vier Paketdateien mit Größen und Hashes, die Zielplattform, die offenen Hardwaretests und alle vom ELF benötigten Systembibliotheken. Das ZIP wird aus einer festen Dateiliste gebaut, niemals aus einem Arbeits- oder Konfigurationsverzeichnis.

Die `.so`-Dateien im heruntergeladenen FlyThings-SDK sind Bibliotheken zum Linken gegen das vorhandene System. Sie werden **nicht** ins Gerätepaket kopiert. Auf dem TC002 müssen insbesondere `libeasyui.so`, `liblog.so`, `libzkhardware.so`, `libzknet.so`, `libmi_ao.so`, `libmi_sys.so`, `libmi_common.so`, `libcam_os_wrapper.so` und die in der Manifestdatei aufgeführten Linux-/GCC-Laufzeitbibliotheken vorhanden und ABI-kompatibel sein. Das lokale Testskript muss das vor dem Start prüfen.

Der erzeugte ELF benötigt unter anderem `GLIBC_2.28` und `GLIBCXX_3.4.22`; die vollständige Versionsliste steht in `required_symbol_versions`. Außerdem muss die Systemzeit stimmen, damit die CA-Prüfung funktioniert. Ob die originale Systeminstallation die Zeit unabhängig von der Ulanzi-Oberfläche synchronisiert, wird am Gerät geprüft. Ein fehlgeschlagener Zertifikatstest wird nicht durch abgeschaltete TLS-Prüfung umgangen.

## Dauerhafte Installation später

Das lokale Paket ist kein `update.img` und wird nicht als flashbares Image bezeichnet. Die Herstellerdokumentation unterscheidet flüchtiges WLAN-ADB-Debugging von einem dauerhaft installierten Image. Für die spätere feste Installation nennt sie eine IDE-Imageerstellung und einen ADB-Upgradeablauf über `sys.zkupgrade.flag`, `sys.zkupgrade.dir` und Neustart von `zkswe`. Diese Aktionen werden vom Buildskript nicht ausgeführt.

Nach Lieferung des Geräts zuerst den flüchtigen Start sowie die Rückkehr zur Originalsoftware prüfen. Erst danach wird die dauerhafte Imageerstellung passend zur tatsächlichen Firmwareversion freigegeben. Die physische Wiederherstellung über die Reset-Taste neben USB-C ist vom Hersteller beschrieben, aber von uns noch nicht auf diesem Gerät getestet. Der generische TF-Kartenweg in der IDE-Dokumentation allein belegt keine beim TC002 zugängliche TF-Schnittstelle.

## Lizenz und Verteilung

Owlanzi-TC002 und die übernommenen Ulanzi-Anteile stehen unter GPL-3.0-or-later. `platform/tc002/resources/main.ftu` wurde unverändert aus dem fixierten GPL-Ulanzi-Projekt übernommen; die Hardwareadapter sind neue Implementierungen auf Basis dessen dokumentierter Protokolle und Belegung. Lizenztexte und Hinweise im Repository gelten zusätzlich für die separat eingebundenen Open-Source-Bestandteile.

Die [Herstellerhinweise](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/THIRD_PARTY_NOTICES.md) nennen für FlyThings-eigene Pakete keine vollständigen Lizenzbedingungen. Einige davon werden statisch in das lokale Binary eingebunden. Vor einer öffentlichen Binärverteilung müssen dafür die Nutzungsrechte und gegebenenfalls vollständigen korrespondierenden Quellen geklärt werden. Das Repository verteilt diese SDK-Binaries nicht; jeder lokale Build lädt sie direkt beim Hersteller. Das ist eine offene Voraussetzung für die spätere öffentliche Veröffentlichung, unabhängig vom gewählten GPL-Lizenztext für unsere eigenen Quellen.
