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

#include "canvas.h"
//#include "../fontutils.h"

uint16_t Canvas::width, Canvas::height;
uint16_t Canvas::startLine, Canvas::endLine;
//uint16_t Canvas::background_color;
uint16_t *Canvas::buffer = TFT::buffer;

void Canvas::instantiate(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  Canvas::width = width;
  Canvas::height = height;
  startLine = 0;
  endLine = 0;

  tft.set_window(x, y, x + width - 1, y + height - 1);
}

void Canvas::next() {
  startLine = endLine;
  endLine = TFT_BUFFER_SIZE < width * (height - startLine) ? startLine + TFT_BUFFER_SIZE / width : height;
}

bool Canvas::toScreen() {
  tft.write_sequence(buffer, width * (endLine - startLine));
  return endLine == height;
}

void Canvas::setBackground(uint16_t color) {
  /* TODO: test and optimize performance */
  /*
  uint32_t count = (endLine - startLine) * width;
  uint16_t *pixel = buffer;
  while (count--)
    *pixel++ = color;
  */
  const uint32_t two_pixels = (((uint32_t )color) << 16) | color;
  uint32_t count = ((endLine - startLine) * width + 1) >> 1;
  uint32_t *pointer = (uint32_t *)buffer;
  while (count--) *pointer++ = two_pixels;
}

uint8_t canvas_read_byte(const uint8_t *byte) { return *byte; }

void Canvas::addText(uint16_t x, uint16_t y, uint16_t color, uint8_t *string, uint16_t maxWidth, font_t *pFont) {
  if (endLine < y || startLine > y + getFontHeight()) return;

  if (maxWidth == 0) maxWidth = width - x;

  uint16_t stringWidth = 0;
  /*
  if (getFontType() == FONT_MARLIN_GLYPHS_2BPP) {
    for (uint8_t i = 0; i < 3; i++) {
      colors[i] = gradient(ENDIAN_COLOR(color), ENDIAN_COLOR(background_color), ((i+1) << 8) / 3);
      colors[i] = ENDIAN_COLOR(colors[i]);
    }
  }
  for (uint16_t i = 0 ; *(string + i) ; i++) {
    glyph_t *pGlyph = glyph(string + i);
    if (stringWidth + pGlyph->bbxWidth > maxWidth) break;
    switch (getFontType()) {
      case FONT_MARLIN_GLYPHS_1BPP:
        addImage(x + stringWidth + pGlyph->bbxOffsetX, y + getFontAscent() - pGlyph->bbxHeight - pGlyph->bbxOffsetY, pGlyph->bbxWidth, pGlyph->bbxHeight, GREYSCALE1, ((uint8_t *)pGlyph) + sizeof(glyph_t), &color);
        break;
      case FONT_MARLIN_GLYPHS_2BPP:
        addImage(x + stringWidth + pGlyph->bbxOffsetX, y + getFontAscent() - pGlyph->bbxHeight - pGlyph->bbxOffsetY, pGlyph->bbxWidth, pGlyph->bbxHeight, GREYSCALE2, ((uint8_t *)pGlyph) + sizeof(glyph_t), colors);
        break;
    }
    stringWidth += pGlyph->dWidth;
  }
 */

    lchar_t wchar;
    while (*string)
    {
      string = (uint8_t*)get_utf8_value_cb(string, canvas_read_byte, wchar);
      if (wchar > 255)
        wchar |= 0x0080;
      uint8_t ch = uint8_t(wchar & 0x00FF);
      // uint8_t ch = 33;
      glyph_t *pGlyph = fontGlyph(pFont, &ch);
      if (stringWidth + pGlyph->bbxWidth > maxWidth)
        break;
      addImage(x + stringWidth + pGlyph->bbxOffsetX, y + font()->fontAscent - pGlyph->bbxHeight - pGlyph->bbxOffsetY, pGlyph->bbxWidth, pGlyph->bbxHeight, GREYSCALE1, ((uint8_t *)pGlyph) + sizeof(glyph_t), &color);
      stringWidth += pGlyph->dWidth;
    }


}

void Canvas::addImage(int16_t x, int16_t y, MarlinImage image, uint16_t *colors) {
  uint16_t *data = (uint16_t *)images[image].data;
  if (!data) return;

  uint16_t image_width = images[image].width,
           image_height = images[image].height;
  colorMode_t color_mode = images[image].colorMode;

  if (color_mode != HIGHCOLOR)
    return addImage(x, y, image_width, image_height, color_mode, (uint8_t *)data, colors);

  // HIGHCOLOR - 16 bits per pixel

  for (int16_t i = 0; i < image_height; i++) {
    int16_t line = y + i;
    if (line >= startLine && line < endLine) {
      uint16_t *pixel = buffer + x + (line - startLine) * width;
      for (int16_t j = 0; j < image_width; j++) {
        if ((x + j >= 0) && (x + j < width)) *pixel = ENDIAN_COLOR(*data);
        pixel++;
        data++;
      }
    }
    else
      data += image_width;
  }
}

void Canvas::addImage(int16_t x, int16_t y, uint8_t image_width, uint8_t image_height, colorMode_t color_mode, uint8_t *data, uint16_t *colors) {
  uint8_t bitsPerPixel;
  switch (color_mode) {
    case GREYSCALE1: bitsPerPixel = 1; break;
    case GREYSCALE2: bitsPerPixel = 2; break;
    case GREYSCALE4: bitsPerPixel = 4; break;
    default: return;
  }

  uint8_t mask = 0xFF >> (8 - bitsPerPixel),
          pixelsPerByte = 8 / bitsPerPixel;

  colors--;

  for (int16_t i = 0; i < image_height; i++) {
    int16_t line = y + i;
    if (line >= startLine && line < endLine) {
      uint16_t *pixel = buffer + x + (line - startLine) * width;
      uint8_t offset = 8 - bitsPerPixel;
      for (int16_t j = 0; j < image_width; j++) {
        if (offset > 8) {
          data++;
          offset = 8 - bitsPerPixel;
        }
        if ((x + j >= 0) && (x + j < width)) {
          const uint8_t color = ((*data) >> offset) & mask;
          if (color) *pixel = *(colors + color);
        }
        pixel++;
        offset -= bitsPerPixel;
      }
      data++;
    }
    else
      data += (image_width + pixelsPerByte - 1) / pixelsPerByte;
  }
}

void Canvas::addRect(uint16_t x, uint16_t y, uint16_t rectangleWidth, uint16_t rectangleHeight, uint16_t color) {
  if (endLine < y || startLine > y + rectangleHeight) return;

  for (uint16_t i = 0; i < rectangleHeight; i++) {
    uint16_t line = y + i;
    if (line >= startLine && line < endLine) {
      uint16_t *pixel = buffer + x + (line - startLine) * width;
      if (i == 0 || i == rectangleHeight - 1) {
        for (uint16_t j = 0; j < rectangleWidth; j++) *pixel++ = color;
      }
      else {
        *pixel = color;
        pixel += rectangleWidth - 1;
        *pixel = color;
      }
    }
  }
}

void Canvas::addBar(uint16_t x, uint16_t y, uint16_t barWidth, uint16_t barHeight, uint16_t color) {
  if (endLine < y || startLine > y + barHeight) return;

  for (uint16_t i = 0; i < barHeight; i++) {
    uint16_t line = y + i;
    if (line >= startLine && line < endLine) {
      uint16_t *pixel = buffer + x + (line - startLine) * width;
      for (uint16_t j = 0; j < barWidth; j++) *pixel++ = color;
    }
  }
}

Canvas tftCanvas;

#endif // HAS_GRAPHICAL_TFT
