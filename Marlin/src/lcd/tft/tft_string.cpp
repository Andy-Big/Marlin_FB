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

#include "../../inc/MarlinConfig.h"

#if HAS_GRAPHICAL_TFT

#include "tft_string.h"
#include "../fontutils.h"
#include "../marlinui.h"

//#define DEBUG_TFT_FONT
#define DEBUG_OUT ENABLED(DEBUG_TFT_FONT)
#include "../../core/debug_out.h"

glyph_t *TFT_String::glyphs[256];
font_t *TFT_String::font_header;

uint8_t TFT_String::data[];
uint16_t TFT_String::span;
uint8_t TFT_String::length;

void TFT_String::set_font(const uint8_t *font) {
  font_header = (font_t *)font;
  uint32_t glyph;

  for (glyph = 0; glyph < 256; glyph++) glyphs[glyph] = nullptr;

  DEBUG_ECHOLNPGM("Format: ",            font_header->Format);
  DEBUG_ECHOLNPGM("BBXWidth: ",          font_header->BBXWidth);
  DEBUG_ECHOLNPGM("BBXHeight: ",         font_header->BBXHeight);
  DEBUG_ECHOLNPGM("BBXOffsetX: ",        font_header->BBXOffsetX);
  DEBUG_ECHOLNPGM("BBXOffsetY: ",        font_header->BBXOffsetY);
  DEBUG_ECHOLNPGM("CapitalAHeight: ",    font_header->CapitalAHeight);
  DEBUG_ECHOLNPGM("Encoding65Pos: ",     font_header->Encoding65Pos);
  DEBUG_ECHOLNPGM("Encoding97Pos: ",     font_header->Encoding97Pos);
  DEBUG_ECHOLNPGM("FontStartEncoding: ", font_header->FontStartEncoding);
  DEBUG_ECHOLNPGM("FontEndEncoding: ",   font_header->FontEndEncoding);
  DEBUG_ECHOLNPGM("LowerGDescent: ",     font_header->LowerGDescent);
  DEBUG_ECHOLNPGM("FontAscent: ",        font_header->FontAscent);
  DEBUG_ECHOLNPGM("FontDescent: ",       font_header->FontDescent);
  DEBUG_ECHOLNPGM("FontXAscent: ",       font_header->FontXAscent);
  DEBUG_ECHOLNPGM("FontXDescent: ",      font_header->FontXDescent);

  add_glyphs(font);
}

void TFT_String::add_glyphs(const uint8_t *font) {
  uint32_t glyph;
  uint8_t *pointer = (uint8_t *)font + sizeof(font_t);

  for (glyph = ((font_t *)font)->FontStartEncoding; glyph <= ((font_t *)font)->FontEndEncoding; glyph++) {
    if (*pointer != NO_GLYPH) {
      glyphs[glyph] = (glyph_t *)pointer;
      pointer += sizeof(glyph_t) + ((glyph_t *)pointer)->DataSize;
    }
    else
      pointer++;
  }
}

glyph_t *TFT_String::get_font_glyph(font_t *gfont, uint8_t character)
{
  if (gfont == 0)
    return glyphs[0x3F];

  uint32_t glyph;
  uint8_t *pointer = (uint8_t *)gfont + sizeof(font_t);

  for (glyph = ((font_t *)gfont)->FontStartEncoding; glyph <= ((font_t *)gfont)->FontEndEncoding; glyph++) {
    if (*pointer != NO_GLYPH)
    {
      if (glyph == character)
        return (glyph_t *)pointer;
      pointer += sizeof(glyph_t) + ((glyph_t *)pointer)->DataSize;
    }
    else
      pointer++;
  }
  return glyphs[character] ?: glyphs[0x3F];
}

void TFT_String::set() {
  *data = 0x00;
  span = 0;
  length = 0;
}

uint8_t read_byte(uint8_t *byte) { return *byte; }

/**
 * Add a string, applying substitutions for the following characters:
 *
 *   $ displays an inserted C-string given by the itemString parameter
 *   = displays  '0'....'10' for indexes 0 - 10
 *   ~ displays  '1'....'11' for indexes 0 - 10
 *   * displays 'E1'...'E11' for indexes 0 - 10 (By default. Uses LCD_FIRST_TOOL)
 *   @ displays an axis name such as XYZUVW, or E for an extruder
 */
void TFT_String::add(uint8_t *string, int8_t index, uint8_t *itemString) {

  while (*string) {

/*      string = get_utf8_value_cb(string, read_byte, &wchar);
    if (wchar > 255) wchar |= 0x0080;
    uint8_t ch = uint8_t(wchar & 0x00FF);
 */
    uint8_t ch = *string;
    string++;



    if (ch == '=' || ch == '~' || ch == '*') {
      if (index >= 0) {
        int8_t inum = index + ((ch == '=') ? 0 : LCD_FIRST_TOOL);
        if (ch == '*') add_character('E');
        if (inum >= 10) { add_character('0' + (inum / 10)); inum %= 10; }
        add_character('0' + inum);
      }
      else
        add(index == -2 ? GET_TEXT(MSG_CHAMBER) : GET_TEXT(MSG_BED));
    }
    else if (ch == '$' && itemString)
      add(itemString);
    else if (ch == '@')
      add_character(axis_codes[index]);
    else
      add_character(ch);
  }
  eol();
}

void TFT_String::add(uint8_t *string, uint8_t max_len) {
  wchar_t wchar;
  uint8_t *string1 = string, *string2;
  while (*string1 && max_len) {
/* 
    string = get_utf8_value_cb(string, read_byte, &wchar);
    if (wchar > 255) wchar |= 0x0080;
    uint8_t ch = uint8_t(wchar & 0x00FF);
 */

    string2 = get_utf8_value_cb(string1, read_byte, &wchar);
    uint32_t wlen = string2 - string1;
    if (wlen > max_len)
      break;
    memcpy(&data[length], string1, wlen);
    length += wlen;
    max_len -= wlen;

    if (wchar > 255)
      wchar |= 0x0080;
    uint8_t ch = uint8_t(wchar & 0x00FF);
    span += glyph(ch)->DWidth;

    string1 = string2;
  }
  eol();
}

void TFT_String::add_character(uint8_t character) {
  if (length < MAX_STRING_LENGTH) {
    data[length] = character;
    length++;
    span += glyph(character)->DWidth;
  }
}

void TFT_String::rtrim(uint8_t character) {
  while (length) {
    if (data[length - 1] == 0x20 || data[length - 1] == character) {
      length--;
      span -= glyph(data[length])->DWidth;
      eol();
    }
    else {
      break;
    }
  }
}

void TFT_String::ltrim(uint8_t character) {
  uint16_t i, j;
  for (i = 0; (i < length) && (data[i] == 0x20 || data[i] == character); i++) {
    span -= glyph(data[i])->DWidth;
  }
  if (i == 0) return;
  for (j = 0; i < length; data[j++] = data[i++]);
  length = j;
  eol();
}

void TFT_String::trim(uint8_t character) {
  rtrim(character);
  ltrim(character);
}

TFT_String tft_string;

#endif // HAS_GRAPHICAL_TFT
