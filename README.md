# Glucoscreen – CGM glucose display for Home Assistant

An ESPHome/LVGL firmware for the **Guition ESP32-S3-4848S040** (480×480 touch display) that
shows continuous glucose monitor (CGM) values from Home Assistant. The look is inspired by
common CGM smartphone apps (see [Design and third-party content](#design-and-third-party-content)).

It is **not tied to a specific sensor**: any CGM works whose glucose value, trend and reading
time are available in Home Assistant (e.g. via the Dexcom, Nightscout or LibreLink
integrations). It has been tested with a **Dexcom G7** via the Dexcom integration.

Features:

- Glucose value in a circle with trend tip and arrow(s), colored yellow/red when high/low
- History chart for 3 / 6 / 12 / 24 h with target range and warning zones; long-press a
  point to see its time and value
- Alert limits, brightness, night period, light/dark theme and language adjustable on the
  device **and** as Home Assistant entities
- Time-dependent dimming, optional dark mode during the night, tap-to-wake
- 24 h history stored on the device (survives reboots); gaps are refilled from Home
  Assistant automatically
- German and English UI, translation table for adding more languages

| File | Purpose |
|---|---|
| `example.yaml` | **The only file you copy** – your own device configuration |
| `glucoscreen.yaml` | ESPHome package with the actual firmware, loaded from GitHub |
| `components/glucoscreen_ui/` | External component with translations and color palettes |
| `ANLEITUNG.md` | This guide in German |

The device name in the configuration is `glucoscreen`. The entity and action names below are
derived from it (e.g. `light.glucoscreen_backlight`, `esphome.glucoscreen_glukose_setzen`).
If Home Assistant assigned different entity IDs on your system, use your own.

> Action, field and entity names are German (e.g. `glukose_setzen`, `wert`). They are
> identifiers – keep them as they are unless you also change them in the YAML.

## 1. Flashing

You do **not** copy the firmware. ESPHome loads it directly from this repository as a
[remote package](https://esphome.io/components/packages/), and you only keep a small device
file with your name and secrets:

1. Copy [`example.yaml`](example.yaml) into your ESPHome folder (any name, e.g.
   `glucoscreen.yaml`) and adjust `name` / `friendly_name` if you like.
2. Your `secrets.yaml` must contain: `wifi_ssid`, `wifi_password`, `ap_password`,
   `ota_password`, `glucoscreen_api_key` (when updating an existing installation keep its
   API key, otherwise the device has to be re-adopted in Home Assistant).
3. Install (OTA is fine). ESPHome downloads the package and the `glucoscreen_ui` component.
4. Create the automations from section 2 (required) and 3 (refilling gaps).

**Updates:** ESPHome checks for a new version at most once a day (`refresh: 1d`) when you
compile; installing again brings the device up to date. To stay on a fixed version, set
`ref:` in your device file to a release tag (e.g. `v1.0.0`) instead of `main`.

**Customizing:** anything from the package can be changed in your device file with
`!extend` / `!remove` (examples at the end of `example.yaml`), without touching the package.

**Private fork / repository:** ESPHome needs access to the repository. For a private one,
use an access token in the URL of `packages` and of `external_components` (in
`glucoscreen.yaml`), e.g. via `!secret`. When you fork the project, replace `Alohx/glucoscreen`
with your repository in `example.yaml` **and** in `external_components` of `glucoscreen.yaml`.

## 2. Sending glucose values (automation, required)

The YAML does not contain any entities of your Home Assistant. An automation sends the values
via the action `esphome.glucoscreen_glukose_setzen`. Replace the placeholders with the
entities of your glucose sensor (value in mg/dL, trend, timestamp of the reading) –
**in the triggers and in the `data` lines**:

```yaml
alias: Glucoscreen – send glucose values
mode: queued
max: 5
triggers:
  - trigger: state
    entity_id:
      - sensor.GLUCOSE_VALUE
      - sensor.GLUCOSE_TREND
      - sensor.READING_TIME
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
      wert: "{{ states('sensor.GLUCOSE_VALUE') | float(0) }}"
      trendrichtung: "{{ states('sensor.GLUCOSE_TREND') }}"
      zeitpunkt: "{{ as_timestamp(states('sensor.READING_TIME'), 0) | int }}"
```

- `wert` = 0 (e.g. sensor unavailable) → the display shows `---`.
- `zeitpunkt` = 0 → the display uses the current time as reading time. If your integration
  has no reading-time entity, remove that trigger and send `zeitpunkt: 0`.
- If no new reading arrives for more than 15 minutes, the value turns grey.

**Other sensors / integrations**

- **Unit:** the display expects mg/dL. If your sensor reports mmol/L, convert it:
  `wert: "{{ (states('sensor.GLUCOSE_VALUE') | float(0) * 18.016) | round(0) }}"`.
- **Trend:** these spellings are recognized (case, `_`, `-` and spaces are ignored):

  | Arrow | Accepted values |
  |---|---|
  | ↑↑ | `rising_quickly`, `DoubleUp`, `rapidly_rising`, `rising_fast` |
  | ↑ | `rising`, `SingleUp`, `up` |
  | ↗ | `rising_slightly`, `FortyFiveUp`, `slowly_rising` |
  | → | `steady`, `Flat`, `stable`, `constant` |
  | ↘ | `falling_slightly`, `FortyFiveDown`, `slowly_falling` |
  | ↓ | `falling`, `SingleDown`, `down` |
  | ↓↓ | `falling_quickly`, `DoubleDown`, `rapidly_falling`, `falling_fast` |

  Anything else shows no arrow. If your integration uses other values (e.g. numbers or a
  rate in mg/dL/min), map them in the template, for example:
  `trendrichtung: "{{ {'1': 'rising_quickly', '4': 'steady', '7': 'falling_quickly'}.get(states('sensor.GLUCOSE_TREND'), '') }}"`.

## 3. Refilling gaps automatically

When a value arrives after a reboot of the display or after more than 11 minutes without a
reading, the display fires the event **`esphome.glucoscreen_verlauf_anfordern`** (at most
every 2 minutes). Home Assistant answers with the values of the last 24 h. The display keeps
storing the last 24 h itself; existing values are never overwritten, only gaps are filled.
(Data that Home Assistant did not record itself – e.g. because HA or the sensor's cloud
service was unreachable – cannot be refilled, of course.)

The values come from the **long-term statistics** of the glucose sensor (5-minute means –
with one reading every 5 minutes that is the reading itself). No additional sensor is needed.

**a) Mark the glucose sensor as a measurement (once, if needed)**

Check the attributes of your glucose sensor (Settings → Entities → sensor → ⋮ → Attributes).
If it already has `state_class: measurement`, skip this step. Some integrations – including
the Dexcom integration – do not set a `state_class`, so Home Assistant does not keep
statistics for the sensor. This is added via `customize`. There is no UI dialog for it,
so edit `configuration.yaml` (e.g. with the *File editor* app; **make a copy of the file
first**). If there already is a `homeassistant:` or `customize:` section, add the lines to it
instead of creating a second one:

```yaml
homeassistant:
  customize:
    sensor.GLUCOSE_VALUE:
      state_class: measurement
```

Then go to **Tools → YAML** (before 2026.8: *Developer tools → YAML*), run
*Check configuration* and reload *Location & customizations* (or restart HA). From now on
HA collects the statistics – **values can be refilled from this point on**, so after one day
always the full last 24 h. HA keeps the 5-minute statistics for 10 days by default.

If you get `Integration 'customize' not found`, `customize:` is not indented under
`homeassistant:`.

**b) Automation** (replace `sensor.GLUCOSE_VALUE` in both places):

```yaml
alias: Glucoscreen – refill history
mode: single
triggers:
  - trigger: event
    event_type: esphome.glucoscreen_verlauf_anfordern
actions:
  - action: recorder.get_statistics
    data:
      statistic_ids:
        - sensor.GLUCOSE_VALUE
      start_time: "{{ now() - timedelta(hours=24) }}"
      period: 5minute
      types:
        - mean
    response_variable: statistik
  - variables:
      werte: "{{ statistik.statistics.get('sensor.GLUCOSE_VALUE', []) }}"
  - action: esphome.glucoscreen_verlauf_laden
    data:
      daten: >-
        {%- set ns = namespace(out=[]) -%}
        {%- for r in werte if r.mean is not none -%}
          {%- set ns.out = ns.out + [(as_timestamp(r.start) | int) ~ ':' ~ (r.mean | round(0) | int)] -%}
        {%- endfor -%}
        {{ ns.out | join(',') }}
```

You can run the automation manually for testing (“Run actions”); the ESPHome log then shows
`Verlauf geladen: … Einträge, … Lücken gefüllt` (history loaded: … entries, … gaps filled).
To clear the history: `esphome.glucoscreen_verlauf_loeschen`.

## 4. Using the device

| Page | Content |
|---|---|
| 1 Home | Glucose value in the circle, trend tip + arrow(s), age of the reading, clock |
| 2 History | 3 / 6 / 12 / 24 h. **Hold a point for about ½ s** → time and value appear; move the finger sideways → the display follows the point under the finger |
| 3 Alert limits | High, target high, target low, low, urgent low |
| 4 Display | Auto dimming, day/night brightness, night from/until (15-min steps), theme light/dark/auto, language |

Swiping wraps around (after page 4 comes page 1 again). Hold −/+ to change values quickly.

Colors: base color = in range · yellow = above “High” · red = below “Low” · dark red = below
“Urgent low” · grey value = reading older than 15 min / no data.

**Theme “Auto”:** dark during the night period (night from … night until), light otherwise –
i.e. together with dimming.

## 5. Entities in Home Assistant

| Entity | Purpose |
|---|---|
| `light.glucoscreen_backlight` | Display on/off + brightness (for automations) |
| `number.glucoscreen_grenze_hoch` / `_niedrig` / `_dringend_niedrig` | Alert limits high / low / urgent low |
| `number.glucoscreen_zielbereich_oben` / `_unten` | Target range (grey area in the chart) |
| `number.glucoscreen_helligkeit_tag` / `_nacht` | Day / night brightness in % |
| `time.glucoscreen_nacht_ab` / `_bis` | Night period start / end |
| `switch.glucoscreen_automatisch_dimmen` | Time-dependent dimming on/off |
| `select.glucoscreen_darstellung` | Theme: Hell (light) / Dunkel (dark) / Automatisch (Nacht) |
| `select.glucoscreen_sprache` | Language: Deutsch / English |
| `switch.glucoscreen_tippen_weckt_display` | When the backlight is off, a tap lights the display for 20 s |

Changes on the device and in HA always stay in sync. Automatic dimming only acts when
switching between day and night and when the backlight is turned on – a brightness set from
HA in between is kept until the next switch. The entity names are fixed in German (HA cannot
switch them with the display language).

## 6. Adding a language

Because the firmware is loaded from GitHub, changes like this go into a fork of the
repository (point your device file to it) – or, better, into a pull request here so that
everyone gets the new language.

In `components/glucoscreen_ui/glucoscreen_ui.h`:
1. Increase `LANG_COUNT` and append the name to `LANG_NAMES`.
2. Append a column with the translation to every row of `TEXTS`.

In `glucoscreen.yaml`:
3. Append the language name in the same order to `sel_language` → `options` **and** to the
   dropdown `dd_lang` → `options`. The dropdown uses the built-in Montserrat font (basic
   characters only), so write the language name without accents (e.g. “Francais”).
4. Add missing special characters of your translations to `glyphs` of `font_text` and
   `font_title` (common Western European accents are already included).

## 7. Troubleshooting

- **Audible whine when dimmed:** the backlight PWM runs at 25 kHz. If you still hear it, try
  30000Hz or 40000Hz under `output:` → `frequency:`. A whine at 100 % brightness usually comes
  from the power supply.
- **Touch offset:** the GT911 calibration is fixed to 0–480 under `touchscreen:` →
  `calibration:`.
- **Compile errors:** the UI uses the LVGL C API directly in several places (rotation, chart,
  animation, theming). Written for the LVGL 8 version bundled with current ESPHome; the
  LVGL 9 differences that are known are handled with `#if`.

## Design and third-party content

The layout is **inspired by** the general design language of common CGM smartphone apps,
in particular the Dexcom app: the value in a circle with a pointer showing the trend
direction, a dotted history chart with a shaded target range, and yellow/red for high/low
values. These are widespread conventions for glucose displays.

**No material from Dexcom or its apps is used** – no logos, images, icons, screenshots,
fonts, texts, source code or data files. Everything visible is drawn by this firmware itself:

| Element | Origin |
|---|---|
| Circle, trend pointer, chart, buttons | Drawn at runtime with LVGL primitives (own code) |
| Trend arrow | “Play” triangle of the Font Awesome symbol set bundled with LVGL |
| Fonts | [Roboto](https://fonts.google.com/specimen/Roboto) (via Google Fonts) and Montserrat (bundled with LVGL), both under the SIL Open Font License |
| Colors | Own choice of common colors, no values taken from any app |
| Texts and translations | Written for this project |

**Data source:** the device only talks to your Home Assistant. It never connects to
Dexcom or any other manufacturer's servers; the glucose values come from whatever
integration you use in Home Assistant.

## Disclaimer

This is a hobby project and **not a medical device**. Do not base treatment decisions on this
display – always use the official receiver or app of your CGM and its alarms.

Dexcom and Dexcom G7 are trademarks of Dexcom, Inc. They are used here only to describe
compatibility and design inspiration. This project is not affiliated with, endorsed by or
sponsored by Dexcom or any other CGM manufacturer.
