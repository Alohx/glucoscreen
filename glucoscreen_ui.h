// =====================================================================
//  Glucoscreen – Übersetzungen und Farbpaletten
//  Wird von glucoscreen.yaml über "esphome: includes:" eingebunden.
//  Diese Datei muss im selben Ordner wie glucoscreen.yaml liegen.
// =====================================================================
#pragma once
#include <cstdint>

namespace ui {

// ---------------------------------------------------------------------
//  Farbpaletten (Hell / Dunkel)
// ---------------------------------------------------------------------
struct Palette {
  uint32_t page;        // Seitenhintergrund
  uint32_t card;        // Karten (Diagramm, Einstellungen)
  uint32_t text;        // Haupttext
  uint32_t text2;       // Nebentext (Achsen, Einheit, Uhrzeit)
  uint32_t btn;         // Schaltflächen
  uint32_t face;        // Kreis, Ring und Trendspitze im Zielbereich
  uint32_t target;      // Zielbereich im Diagramm
  uint32_t dot;         // Messpunkte im Diagramm
  uint32_t stale;       // Wert, wenn veraltet
  uint32_t page_dot_on;
  uint32_t page_dot_off;
  uint32_t tip_bg;      // Uhrzeit-Anzeige im Diagramm
  uint32_t tip_text;
  uint8_t shadow_opa;   // Schatten des Kreises (0-255)
};

static const Palette LIGHT = {
    0xEEEEEE, 0xFFFFFF, 0x111111, 0x5A6474, 0xE6E6E6, 0xFFFFFF, 0xEEEEEE,
    0x1A1A1A, 0xA0A4AA, 0x5A6474, 0xC8CCD2, 0x2B2F36, 0xFFFFFF, 56};

static const Palette DARK = {
    0x121212, 0x1E1F22, 0xF2F2F2, 0x9AA3AE, 0x2C2E33, 0x2A2C30, 0x2A2C30,
    0xE8E8E8, 0x6B7078, 0xB0B6BE, 0x3A3D42, 0xE8E8E8, 0x111111, 0};

inline const Palette &pal(bool dark) { return dark ? DARK : LIGHT; }

// Warnfarben (in beiden Modi gleich; eigene Farbwahl, angelehnt an das
// übliche Schema von CGM-Apps: gelb = hoch, rot = niedrig)
static const uint32_t COL_HIGH = 0xFFC72C;
static const uint32_t COL_LOW = 0xF2203E;
static const uint32_t COL_URGENT_LOW = 0xC8102E;

// ---------------------------------------------------------------------
//  Übersetzungen
//
//  Weitere Sprache hinzufügen:
//   1. LANG_COUNT erhöhen und den Namen in LANG_NAMES ergänzen
//   2. in TEXTS jeder Zeile eine Spalte anhängen
//   3. in glucoscreen.yaml den Namen bei "sel_language" (options) und
//      beim Dropdown "dd_lang" (options) in derselben Reihenfolge ergänzen
//   4. fehlende Sonderzeichen ggf. in font_text / font_title (glyphs) ergänzen
// ---------------------------------------------------------------------
enum Text : uint8_t {
  T_AGE_NOW,
  T_AGE_MIN,  // enthält %d
  T_NO_HISTORY,
  T_LIMITS_TITLE,
  T_HIGH,
  T_TARGET_HIGH,
  T_TARGET_LOW,
  T_LOW,
  T_URGENT_LOW,
  T_DISPLAY_TITLE,
  T_AUTO_DIM,
  T_BRIGHT_DAY,
  T_BRIGHT_NIGHT,
  T_NIGHT_FROM,
  T_NIGHT_TO,
  T_THEME,
  T_THEME_LIGHT,
  T_THEME_DARK,
  T_THEME_AUTO,
  T_LANGUAGE,
  TEXT_COUNT
};

static const uint8_t LANG_COUNT = 2;
static const char *const LANG_NAMES[LANG_COUNT] = {"Deutsch", "English"};

static const char *const TEXTS[TEXT_COUNT][LANG_COUNT] = {
    /* T_AGE_NOW       */ {"gerade eben", "just now"},
    /* T_AGE_MIN       */ {"vor %d Min.", "%d min ago"},
    /* T_NO_HISTORY    */ {"Noch keine Verlaufsdaten", "No history yet"},
    /* T_LIMITS_TITLE  */ {"Grenzwerte (mg/dL)", "Alert limits (mg/dL)"},
    /* T_HIGH          */ {"Hoch", "High"},
    /* T_TARGET_HIGH   */ {"Ziel oben", "Target high"},
    /* T_TARGET_LOW    */ {"Ziel unten", "Target low"},
    /* T_LOW           */ {"Niedrig", "Low"},
    /* T_URGENT_LOW    */ {"Dringend niedrig", "Urgent low"},
    /* T_DISPLAY_TITLE */ {"Anzeige", "Display"},
    /* T_AUTO_DIM      */ {"Auto-Dimmen", "Auto dimming"},
    /* T_BRIGHT_DAY    */ {"Helligkeit Tag", "Day brightness"},
    /* T_BRIGHT_NIGHT  */ {"Helligkeit Nacht", "Night brightness"},
    /* T_NIGHT_FROM    */ {"Nacht ab", "Night from"},
    /* T_NIGHT_TO      */ {"Nacht bis", "Night until"},
    /* T_THEME         */ {"Darstellung", "Theme"},
    /* T_THEME_LIGHT   */ {"Hell", "Light"},
    /* T_THEME_DARK    */ {"Dunkel", "Dark"},
    /* T_THEME_AUTO    */ {"Auto", "Auto"},
    /* T_LANGUAGE      */ {"Sprache", "Language"},
};

inline const char *tr(Text t, int lang) {
  if (lang < 0 || lang >= LANG_COUNT) lang = 0;
  return TEXTS[t][lang];
}

}  // namespace ui
