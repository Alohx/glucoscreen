# Glucoscreen – Glukose-Anzeige (CGM) für Home Assistant

Englische Fassung: [README.md](README.md).

Glucoscreen ist **nicht an einen bestimmten Sensor gebunden**: Es funktioniert mit jedem
Glukosesensor (CGM), dessen Wert, Trend und Messzeitpunkt in Home Assistant verfügbar sind
(z. B. über die Integrationen Dexcom, Nightscout oder LibreLink). Getestet wurde es mit
einem **Dexcom G7** über die Dexcom-Integration.

Gerätename in der Konfiguration: `glucoscreen`. Die Entitäts- und Aktionsnamen unten
folgen daraus (z. B. `light.glucoscreen_backlight`, `esphome.glucoscreen_glukose_setzen`).
Falls HA bei dir andere Entitäts-IDs vergeben hat, die eigenen verwenden.

## 1. Flashen

1. **Beide Dateien** in den ESPHome-Ordner legen: `glucoscreen.yaml` und `glucoscreen_ui.h`
   (Übersetzungen + Farben, wird von der YAML eingebunden).
2. In `secrets.yaml` müssen vorhanden sein: `wifi_ssid`, `wifi_password`, `ap_password`,
   `ota_password`, `glucoscreen_api_key` (bei einem Update den API-Schlüssel der
   bisherigen Installation verwenden, sonst muss das Gerät in HA neu gekoppelt werden).
3. Installieren (OTA reicht).
4. Die Automationen aus Abschnitt 2 (Pflicht) und 3 (Lücken nachladen) anlegen.

## 2. Glukosewerte senden (Automation, Pflicht)

Die YAML enthält keine Entitäten aus deinem Home Assistant. Die Werte schickt eine
Automation über die Aktion `esphome.glucoscreen_glukose_setzen`. Platzhalter durch die
Entitäten deines Glukosesensors ersetzen (Wert in mg/dL, Trend, Zeitstempel der Messung) –
**in den Auslösern und in den `data`-Zeilen**:

```yaml
alias: Glucoscreen – Glukosewerte senden
mode: queued
max: 5
triggers:
  - trigger: state
    entity_id:
      - sensor.GLUKOSEWERT
      - sensor.GLUKOSETREND
      - sensor.MESSZEITPUNKT
  - trigger: state
    entity_id: light.glucoscreen_backlight
    from: unavailable
    for: "00:00:10"
  - trigger: homeassistant
    event: start
  - trigger: time_pattern
    minutes: "/5"
actions:
  - action: esphome.glucoscreen_glukose_setzen
    continue_on_error: true
    data:
      wert: "{{ states('sensor.GLUKOSEWERT') | float(0) }}"
      trendrichtung: "{{ states('sensor.GLUKOSETREND') }}"
      zeitpunkt: "{{ as_timestamp(states('sensor.MESSZEITPUNKT'), 0) | int }}"
```

- `wert` 0 (z. B. Sensor nicht verfügbar) → Display zeigt `---`.
- `zeitpunkt` 0 → das Display nimmt die aktuelle Uhrzeit. Hat deine Integration keine
  Entität für den Messzeitpunkt, den Auslöser entfernen und `zeitpunkt: 0` senden.

**Andere Sensoren / Integrationen**

- **Einheit:** Das Display erwartet mg/dL. Liefert dein Sensor mmol/L, umrechnen:
  `wert: "{{ (states('sensor.GLUKOSEWERT') | float(0) * 18.016) | round(0) }}"`.
- **Trend:** Erkannt werden diese Schreibweisen (Groß/Klein, `_`, `-` und Leerzeichen egal):

  | Pfeil | Werte |
  |---|---|
  | ↑↑ | `rising_quickly`, `DoubleUp`, `rapidly_rising`, `rising_fast` |
  | ↑ | `rising`, `SingleUp`, `up` |
  | ↗ | `rising_slightly`, `FortyFiveUp`, `slowly_rising` |
  | → | `steady`, `Flat`, `stable`, `constant` |
  | ↘ | `falling_slightly`, `FortyFiveDown`, `slowly_falling` |
  | ↓ | `falling`, `SingleDown`, `down` |
  | ↓↓ | `falling_quickly`, `DoubleDown`, `rapidly_falling`, `falling_fast` |

  Alles andere zeigt keinen Pfeil. Nutzt deine Integration andere Werte (z. B. Zahlen),
  in der Vorlage umsetzen, etwa:
  `trendrichtung: "{{ {'1': 'rising_quickly', '4': 'steady', '7': 'falling_quickly'}.get(states('sensor.GLUKOSETREND'), '') }}"`.

## 3. Lücken automatisch nachladen

Kommt nach einem Neustart des Displays oder nach mehr als 11 Minuten ohne Messung wieder ein
Wert an, sendet das Display das Ereignis **`esphome.glucoscreen_verlauf_anfordern`**
(höchstens alle 2 Minuten). Home Assistant antwortet mit den Werten der letzten 24 h.
Das Display speichert weiterhin selbst die letzten 24 h; vorhandene Werte werden nie
überschrieben, nur Lücken gefüllt.
(Was HA selbst nicht aufgezeichnet hat – z. B. weil HA oder der Cloud-Dienst des Sensors
nicht erreichbar war – kann natürlich nicht nachgeliefert werden.)

Die Werte kommen aus der **Langzeitstatistik** des Glukosesensors (5-Minuten-Mittelwerte –
bei einer Messung alle 5 Minuten ist das der Messwert selbst). Ein zusätzlicher Sensor ist
nicht nötig.

**a) Glukosesensor als Messwert markieren (einmalig, falls nötig)**

In den Attributen des Glukosesensors nachsehen: Steht dort schon `state_class: measurement`,
diesen Schritt überspringen. Manche Integrationen – auch die Dexcom-Integration – setzen
keine `state_class`, dann führt Home Assistant für den Sensor keine Statistik. Das wird
über `customize` nachgeholt. Dafür gibt es
keinen UI-Dialog, daher in der `configuration.yaml` (z. B. mit der App *File editor*;
**vorher eine Kopie der Datei anlegen**). Steht dort schon ein Abschnitt
`homeassistant:` bzw. `customize:`, die Zeilen darin ergänzen statt ihn doppelt anzulegen:

```yaml
homeassistant:
  customize:
    sensor.GLUKOSEWERT:
      state_class: measurement
```

Danach unter **Tools → YAML** (vor 2026.8: *Entwicklerwerkzeuge → YAML*) zuerst
*Konfiguration prüfen* und dann *Ort & Anpassungen* neu laden (bzw. HA neu starten).
Ab jetzt sammelt HA die Statistik – **nachladen lassen sich Werte ab diesem Zeitpunkt**,
nach einem Tag also immer die vollen letzten 24 h. Die 5-Minuten-Werte hebt HA
standardmäßig 10 Tage auf.

**b) Automation** (`sensor.GLUKOSEWERT` an beiden Stellen ersetzen):

```yaml
alias: Glucoscreen – Verlauf nachliefern
mode: single
triggers:
  - trigger: event
    event_type: esphome.glucoscreen_verlauf_anfordern
actions:
  - action: recorder.get_statistics
    data:
      statistic_ids:
        - sensor.GLUKOSEWERT
      start_time: "{{ now() - timedelta(hours=24) }}"
      period: 5minute
      types:
        - mean
    response_variable: statistik
  - variables:
      werte: "{{ statistik.statistics.get('sensor.GLUKOSEWERT', []) }}"
  - action: esphome.glucoscreen_verlauf_laden
    data:
      daten: >-
        {%- set ns = namespace(out=[]) -%}
        {%- for r in werte if r.mean is not none -%}
          {%- set ns.out = ns.out + [(as_timestamp(r.start) | int) ~ ':' ~ (r.mean | round(0) | int)] -%}
        {%- endfor -%}
        {{ ns.out | join(',') }}
```

Die Automation lässt sich zum Testen auch von Hand ausführen („Aktionen ausführen“); im
ESPHome-Log erscheint dann `Verlauf geladen: … Einträge, … Lücken gefüllt`. Zum Zurücksetzen des Verlaufs:
`esphome.glucoscreen_verlauf_loeschen`.

## 4. Bedienung am Gerät

| Seite | Inhalt |
|---|---|
| 1 Start | Glukosewert im Kreis, Trendspitze + Pfeil(e), Alter der Messung, Uhrzeit |
| 2 Verlauf | 3 / 6 / 12 / 24 h. **Punkt ca. ½ s gedrückt halten** → Uhrzeit und Wert erscheinen; Finger seitlich bewegen → Anzeige folgt dem Punkt unter dem Finger |
| 3 Grenzwerte | Hoch, Ziel oben, Ziel unten, Niedrig, Dringend niedrig |
| 4 Anzeige | Auto-Dimmen, Helligkeit Tag/Nacht, Nacht ab/bis (15-min-Schritte), Darstellung Hell/Dunkel/Auto, Sprache |

Wischen blättert im Kreis (nach Seite 4 kommt wieder Seite 1). −/+ gedrückt halten = schnell.

**Darstellung „Auto“:** dunkel während der Nachtzeit (Nacht ab … Nacht bis), sonst hell –
also gleichzeitig mit dem Abdimmen.

## 5. Entitäten in Home Assistant

| Entität | Zweck |
|---|---|
| `light.glucoscreen_backlight` | Display an/aus + Helligkeit (für Automationen) |
| `number.glucoscreen_grenze_hoch` / `_niedrig` / `_dringend_niedrig` | Farbgrenzen |
| `number.glucoscreen_zielbereich_oben` / `_unten` | grau hinterlegter Bereich im Diagramm |
| `number.glucoscreen_helligkeit_tag` / `_nacht` | Helligkeit in % |
| `time.glucoscreen_nacht_ab` / `_bis` | Nachtzeitraum |
| `switch.glucoscreen_automatisch_dimmen` | tageszeitabhängiges Dimmen an/aus |
| `select.glucoscreen_darstellung` | Hell / Dunkel / Automatisch (Nacht) |
| `select.glucoscreen_sprache` | Deutsch / English |
| `switch.glucoscreen_tippen_weckt_display` | ist das Backlight aus, leuchtet das Display nach Tippen für 20 s |

Änderungen am Gerät und in HA bleiben immer synchron. Die Namen der Entitäten selbst sind
fest auf Deutsch (HA kann sie nicht mit der Display-Sprache umschalten).

## 6. Weitere Sprache hinzufügen

In `glucoscreen_ui.h`:
1. `LANG_COUNT` erhöhen, Namen in `LANG_NAMES` anhängen.
2. In `TEXTS` jeder Zeile eine Spalte mit der Übersetzung anhängen.

In `glucoscreen.yaml`:
3. Den Sprachnamen in derselben Reihenfolge bei `sel_language` → `options` **und** beim
   Dropdown `dd_lang` → `options` anhängen. Das Dropdown nutzt die eingebaute Schrift
   Montserrat (nur Grundzeichen) – den Sprachnamen daher ohne Akzente schreiben
   (z. B. „Francais“).
4. Fehlende Sonderzeichen der Übersetzungen bei `font_text` und `font_title` unter
   `glyphs` ergänzen (gängige west­europäische Akzente sind schon enthalten).

## 7. Falls etwas nicht kompiliert

Die Oberfläche nutzt an mehreren Stellen direkt die LVGL-C-API (Rotation, Diagramm,
Animation, Farbwechsel). Bei Fehlermeldungen die Zeilen mit `error:` schicken.

## Gestaltung und verwendete Fremdinhalte

Die Gestaltung ist **inspiriert** von der üblichen Darstellung in CGM-Smartphone-Apps,
insbesondere der Dexcom-App: Wert im Kreis mit Spitze in Trendrichtung, Punktediagramm mit
hinterlegtem Zielbereich, gelb/rot für hoch/niedrig. Das sind verbreitete Konventionen für
Glukoseanzeigen.

**Es werden keine Inhalte von Dexcom oder deren Apps verwendet** – keine Logos, Bilder,
Icons, Screenshots, Schriften, Texte, Quellcode oder Datendateien. Alles Sichtbare erzeugt
die Firmware selbst:

| Element | Herkunft |
|---|---|
| Kreis, Trendspitze, Diagramm, Schaltflächen | zur Laufzeit mit LVGL gezeichnet (eigener Code) |
| Trendpfeil | „Play“-Dreieck aus dem in LVGL enthaltenen Font-Awesome-Symbolsatz |
| Schriften | Roboto (über Google Fonts) und Montserrat (in LVGL enthalten), beide unter der SIL Open Font License |
| Farben | eigene Wahl gängiger Farben, keine aus einer App übernommenen Werte |
| Texte und Übersetzungen | für dieses Projekt geschrieben |

**Datenquelle:** Das Gerät kommuniziert ausschließlich mit deinem Home Assistant und
verbindet sich nie mit Servern von Dexcom oder anderen Herstellern. Die Glukosewerte kommen
aus der Integration, die du in Home Assistant nutzt.

Dexcom und Dexcom G7 sind Marken der Dexcom, Inc. Sie werden hier nur zur Beschreibung der
Kompatibilität und der Design-Inspiration genannt. Das Projekt steht in keiner Verbindung zu
Dexcom oder anderen CGM-Herstellern und wird von ihnen weder unterstützt noch gesponsert.

## Hinweis

Hobbyprojekt und **kein Medizinprodukt**. Therapieentscheidungen nicht anhand dieser Anzeige
treffen – immer den offiziellen Empfänger bzw. die App des Sensors mit ihren Alarmen nutzen.
