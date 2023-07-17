/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

// TODO: Make AVR-compatible with separate ROM / RAM string methods

#include <stdint.h>

#include "../utf8.h"

extern const uint8_t ISO10646_1_5x7[];
extern const uint8_t font10x20[];

extern const uint8_t Helvetica12Bold[];

extern const uint8_t Helvetica14[], Helvetica14ru[], Tahoma14[], Helvetica14_symbols[];
extern const uint8_t Helvetica18[], Helvetica18ru[], Tahoma18[], Helvetica18_symbols[];
extern const uint8_t Tahoma36bold_num[];

/**
 * Marlin fonts with optional antialiasing. Fonts use unifont_t font header and glyph_t glyphs headers.
 * Number of glyphs (fontEndEncoding - fontStartEncoding) can not exceed 256 (TBD).
 * Some glyphs may be left undefined with NO_GLYPH
 */
#define FONT_MARLIN_GLYPHS      0x80
#define FONT_MARLIN_GLYPHS_1BPP 0x81
#define FONT_MARLIN_GLYPHS_2BPP 0x82
#define FONT_MARLIN_GLYPHS_4BPP 0x84

/**
 * TFT fonts with optional antialiasing. Fonts use unifont_t font header and uniglyph_t glyphs headers.
 * Each glyph is prepended with its unicode.
 * Designed to be used for Japanese, Korean, Simplified Chinese and Traditional Chinese glyphs.
 *
 * IMPORTANT NOTES:
 *   - glyphs fast search method REQUIRES glyphs to be ordered by unicode
 *   - last glyph's code MUST be fontEndEncoding
 */
#define FONT_MARLIN_HIEROGLYPHS      0xA0
#define FONT_MARLIN_HIEROGLYPHS_1BPP 0xA1
#define FONT_MARLIN_HIEROGLYPHS_2BPP 0xA2
#define FONT_MARLIN_HIEROGLYPHS_4BPP 0xA4

#define NO_GLYPH          0xFF

typedef struct __attribute__((__packed__)) {
  uint8_t format;
  uint8_t bbxWidth;
  uint8_t bbxHeight;
   int8_t bbxOffsetX;
   int8_t bbxOffsetY;
  uint8_t capitalAHeight;
 uint16_t Encoding65Pos;
 uint16_t Encoding97Pos;
  uint8_t fontStartEncoding;
  uint8_t fontEndEncoding;
   int8_t LowerGDescent;
   int8_t fontAscent;
   int8_t fontDescent;
   int8_t FontXAscent;
   int8_t FontXDescent;
} font_t;

typedef struct __attribute__((__packed__)) {
  uint8_t bbxWidth;
  uint8_t bbxHeight;
  uint8_t dataSize;
   int8_t dWidth;
   int8_t bbxOffsetX;
   int8_t bbxOffsetY;
} glyph_t;

#define MAX_STRING_LENGTH   128

class TFT_String {
  private:
    static glyph_t *glyphs[256];
    static font_t *font_header;

    static char data[MAX_STRING_LENGTH + 1];
    static uint16_t span;   // in pixels

    static void add_character(const char character);
    static void eol() { data[length] = '\0'; }

  public:
    static uint8_t length;  // in characters

    static void set_font(const uint8_t *font);
    static void add_glyphs(const uint8_t *font);

    static font_t *font() { return font_header; };
    static uint16_t font_height() { return font_header->fontAscent - font_header->fontDescent; }
    static glyph_t *glyph(uint8_t character) { return glyphs[character] ?: glyphs[0x3F]; }  /* Use '?' for unknown glyphs */
    static glyph_t *glyph(uint8_t *character) { return glyph(*character); }
    static glyph_t *get_font_glyph(font_t *font, uint8_t character);
    static void set();

    /**
     * @brief Append an ASCII character and EOL
     *
     * @param character The ASCII character
     */
    static void add(const char character) { add_character(character); eol(); }
    static void set(const lchar_t &character) { set(); add(character); }

    /**
     * @brief Append / Set C-string
     *
     * @param cstr The string
     * @param max_len Character limit
     */
    static void add(const char *cstr, uint8_t max_len=MAX_STRING_LENGTH);
    static void set(const char *cstr, uint8_t max_len=MAX_STRING_LENGTH) { set(); add(cstr, max_len); }

    /**
     * @brief Append / Set F-string
     *
     * @param fstr The string
     * @param max_len Character limit
     */
    static void add(FSTR_P const fstr, uint8_t max_len=MAX_STRING_LENGTH) { add(FTOP(fstr), max_len); }
    static void set(FSTR_P const fstr, uint8_t max_len=MAX_STRING_LENGTH) { set(FTOP(fstr), max_len); }


    /**
     * @brief Append / Set C-string with optional substitution
     *
     * @param tpl A string with optional substitution
     * @param index An index
     * @param cstr An SRAM C-string to use for $ substitution
     * @param fstr A ROM F-string to use for $ substitution
     */
    static void add(const char *tpl, const int8_t index, const char *cstr=nullptr, FSTR_P const fstr=nullptr);
    static void set(const char *tpl, const int8_t index, const char *cstr=nullptr, FSTR_P const fstr=nullptr) { set(); add(tpl, index, cstr, fstr); }

    /**
     * @brief Append / Set F-string with optional substitution
     *
     * @param ftpl A ROM F-string with optional substitution
     * @param index An index
     * @param cstr An SRAM C-string to use for $ substitution
     * @param fstr A ROM F-string to use for $ substitution
     */
    static void add(FSTR_P const ftpl, const int8_t index, const char *cstr=nullptr, FSTR_P const fstr=nullptr) { add(FTOP(ftpl), index, cstr, fstr); }
    static void set(FSTR_P const ftpl, const int8_t index, const char *cstr=nullptr, FSTR_P const fstr=nullptr) { set(); add(ftpl, index, cstr, fstr); }

    // Common string ops
    static void trim(const char character=' ');
    static void rtrim(const char character=' ');
    static void ltrim(const char character=' ');
    static void truncate(const uint8_t maxlen) { if (length > maxlen) { length = maxlen; eol(); } }

    // Accessors
    static char *string() { return data; }
    static uint16_t width() { return span; }
    static uint16_t center(const uint16_t width) { return span > width ? 0 : (width - span) / 2; }
    static uint16_t vcenter(const uint16_t height) { return (height + font_header->capitalAHeight + 1) / 2 > font_header->fontAscent ? (height + font_header->capitalAHeight + 1) / 2 - font_header->fontAscent : 0 ; }
};

extern TFT_String tft_string;
