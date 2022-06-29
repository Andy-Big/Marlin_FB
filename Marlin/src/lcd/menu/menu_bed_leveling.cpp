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

//
// Bed Leveling Menus
//

#include "../../inc/MarlinConfigPre.h"
#include "../tft/ui_common.h"

#if ENABLED(LCD_BED_LEVELING)

#include "menu_item.h"
#include "../../module/planner.h"
#include "../../feature/bedlevel/bedlevel.h"

#if HAS_BED_PROBE && DISABLED(BABYSTEP_ZPROBE_OFFSET)
  #include "../../module/probe.h"
#endif

#if HAS_GRAPHICAL_TFT
  #include "../tft/tft.h"
  #if ENABLED(TOUCH_SCREEN)
    #include "../tft/touch.h"
  #endif
#endif

//#if EITHER(PROBE_MANUALLY, MESH_BED_LEVELING)

  #include "../../module/motion.h"
  #include "../../gcode/queue.h"
  #include "../../module/settings.h"


#if ENABLED(ASSISTED_TRAMMING_WIZARD)
  void goto_tramming_wizard();
#endif

void change_point_edit_mesh();
void change_Z_edit_mesh();

  //
  // Motion > Level Bed handlers
  //

  // LCD probed points are from defaults
  static uint8_t total_probe_points;

  constexpr uint16_t   F_X = 10, F_Y = 10, F_WIDTH = 300, F_HEIGHT = 240;
  constexpr uint16_t   LEG_WIDTH = 240, LEG_HEIGHT = 40;
  constexpr uint16_t   MAX_COLOR = 240, MIN_COLOR = 120;

  static uint8_t xind = 0, yind = 0; // =0


  //
  // Bed leveling is done. Wait for G29 to complete.
  // A flag is used so that this can release control
  // and allow the command queue to be processed.
  //
  // When G29 finishes the last move:
  // - Raise Z to the "Z after probing" height
  // - Don't return until done.
  //
  // ** This blocks the command queue! **
  //
  void _lcd_level_bed_done() {
    if (!ui.wait_for_move) {
      #if Z_AFTER_PROBING > 0 && DISABLED(MESH_BED_LEVELING)
        // Display "Done" screen and wait for moves to complete
        line_to_z(Z_AFTER_PROBING);
        ui.synchronize(GET_TEXT(MSG_LEVEL_BED_DONE));
      #endif
      ui.goto_previous_screen_no_defer();
      ui.completion_feedback();
    }
    if (ui.should_draw()) MenuItem_static::draw(LCD_HEIGHT >= 4, GET_TEXT(MSG_LEVEL_BED_DONE));
    ui.refresh(LCDVIEW_CALL_REDRAW_NEXT);
  }

  void _lcd_level_goto_next_point();

  //
  // Step 7: Get the Z coordinate, click goes to the next point or exits
  //
  void _lcd_level_bed_get_z() {

    if (ui.use_click()) {

      //
      // Save the current Z position and move
      //

      // If done...
      if (++manual_probe_index >= total_probe_points) {
        //
        // The last G29 records the point and enables bed leveling
        //
        ui.wait_for_move = true;
        ui.goto_screen(_lcd_level_bed_done);
        #if ENABLED(MESH_BED_LEVELING)
          queue.inject(F("G29S2"));
        #else
          if (!bedlevel_settings.bltouch_enabled)
            queue.inject(F("G29V1"));
        #endif
      }
      else
        _lcd_level_goto_next_point();

      return;
    }

    //
    // Encoder knob or keypad buttons adjust the Z position
    //
    if (ui.encoderPosition) {
      const float z = current_position.z + float(int32_t(ui.encoderPosition)) * (MESH_EDIT_Z_STEP);
      line_to_z(constrain(z, -(LCD_PROBE_Z_RANGE) * 0.5f, (LCD_PROBE_Z_RANGE) * 0.5f));
      ui.refresh(LCDVIEW_CALL_REDRAW_NEXT);
      ui.encoderPosition = 0;
    }

    //
    // Draw on first display, then only on Z change
    //
    if (ui.should_draw()) {
      const float v = current_position.z;
      MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_MOVE_Z), ftostr43sign(v + (v < 0 ? -0.0001f : 0.0001f), '+'));
    }
  }

  //
  // Step 6: Display "Next point: 1 / 9" while waiting for move to finish
  //
  void _lcd_level_bed_moving() {
    if (ui.should_draw()) {
      char msg[10];
      sprintf_P(msg, PSTR("%i / %u"), int(manual_probe_index + 1), total_probe_points);
      MenuEditItemBase::draw_edit_screen(GET_TEXT(MSG_LEVEL_BED_NEXT_POINT), msg);
    }
    ui.refresh(LCDVIEW_CALL_NO_REDRAW);
    if (!ui.wait_for_move) ui.goto_screen(_lcd_level_bed_get_z);
  }

  //
  // Step 5: Initiate a move to the next point
  //
  void _lcd_level_goto_next_point() {
    ui.goto_screen(_lcd_level_bed_moving);

    // G29 Records Z, moves, and signals when it pauses
    ui.wait_for_move = true;
    #if ENABLED(MESH_BED_LEVELING)
      queue.inject(manual_probe_index ? F("G29S2") : F("G29S1"));
    #else
      if (!bedlevel_settings.bltouch_enabled)
        queue.inject(F("G29V1"));
    #endif
  }

  //
  // Step 4: Display "Click to Begin", wait for click
  //         Move to the first probe position
  //
  void _lcd_level_bed_homing_done() {
    total_probe_points = bedlevel_settings.bedlevel_points.x * bedlevel_settings.bedlevel_points.y;
    if (ui.should_draw()) {
      MenuItem_static::draw(1, GET_TEXT(MSG_LEVEL_BED_WAITING));
      // Color UI needs a control to detect a touch
      #if BOTH(TOUCH_SCREEN, HAS_GRAPHICAL_TFT)
        touch.add_control(CLICK, 0, 0, TFT_WIDTH, TFT_HEIGHT);
      #endif
    }
    if (ui.use_click()) {
      manual_probe_index = 0;
      _lcd_level_goto_next_point();
    }
  }

  //
  // Step 3: Display "Homing XYZ" - Wait for homing to finish
  //
  void _lcd_level_bed_homing() {
    _lcd_draw_homing();
    if (all_axes_homed()) ui.goto_screen(_lcd_level_bed_homing_done);
  }

  extern bool g29_in_progress;

  //
  // Step 2: Continue Bed Leveling...
  //
  void _lcd_level_bed_continue() {
    ui.defer_status_screen();
    set_all_unhomed();
    ui.goto_screen(_lcd_level_bed_homing);
    queue.inject_P(G28_STR);
  }

//#endif // PROBE_MANUALLY || MESH_BED_LEVELING

#if ENABLED(MESH_EDIT_MENU)

  inline void refresh_planner() {
    set_current_from_steppers_for_axis(ALL_AXES_ENUM);
    sync_plan_position();
  }



  void menu_edit_mesh() {

    START_MENU();

    float max_z = 0, min_z = 0, color_step_pos = 0, color_step_neg = 0;

    uint16_t xc = 0, yc = 0, wc = 0, hc = 0;
    float z_intencity;
    uint16_t z_color, cur_z_color;

//    tft.add_rectangle(0, 0, F_WIDTH+2, F_HEIGHT+2, COLOR_MESH_FRAME);

    // find min and max Z-values
    for (uint8_t ix = 0; ix < bedlevel_settings.bedlevel_points.x; ix++)
    {
      for (uint8_t iy = 0; iy < bedlevel_settings.bedlevel_points.y; iy++)
      {
        if (Z_VALUES(ix, iy) > 0 && Z_VALUES(ix, iy) > max_z)
          max_z = Z_VALUES(ix, iy);
        if (Z_VALUES(ix, iy) < 0 && Z_VALUES(ix, iy) < min_z)
          min_z = Z_VALUES(ix, iy);
      }
    }
    color_step_pos = (max_z == 0) ? 0 : (MAX_COLOR - MIN_COLOR) / max_z;
    color_step_neg = (min_z == 0) ? 0 : (MAX_COLOR - MIN_COLOR) / (-min_z);

    // fill color map
    tft.canvas(F_X, F_Y, F_WIDTH, F_HEIGHT);
    tft.set_background(COLOR_MESH_FRAME);
    tft.set_font(SMALL_FONT_NAME);
    wc = F_WIDTH / bedlevel_settings.bedlevel_points.x;
    hc = F_HEIGHT / bedlevel_settings.bedlevel_points.y;
    yc = hc * (bedlevel_settings.bedlevel_points.y - 1) + 1;
    uint16_t points_count = bedlevel_settings.bedlevel_points.y * bedlevel_settings.bedlevel_points.x;
    for (uint8_t iy = 0; iy < bedlevel_settings.bedlevel_points.y; iy++)
    {
      xc = 1;
      for (uint8_t ix = 0; ix < bedlevel_settings.bedlevel_points.x; ix++)
      {
        if (Z_VALUES(ix, iy) >= 0)
        {
          z_intencity = Z_VALUES(ix, iy) * color_step_pos;
          z_color = (z_intencity > 0) ? z_intencity : z_intencity * -1;
          z_color = MAX_COLOR - z_color;
          z_color = RGB(MAX_COLOR, z_color, z_color);
        }
        else
        {
          z_intencity = Z_VALUES(ix, iy) * color_step_neg;
          z_color = (z_intencity > 0) ? z_intencity : z_intencity * -1;
          z_color = MAX_COLOR - z_color;
          z_color = RGB(z_color, z_color, MAX_COLOR);
        }
        if (xind == ix && yind == iy)
        {
          tft.add_bar(xc, yc, wc-1, hc-1, COLOR_BLACK);
          tft.add_bar(xc+3, yc+3, wc-7, hc-7, z_color);
          cur_z_color = z_color;
        }
        else
        {
          tft.add_bar(xc, yc, wc-1, hc-1, z_color);
        }
        if (points_count < 21)
        {
          tft_string.set((uint8_t *)ftostr12(Z_VALUES(ix, iy)));
          tft.add_text(xc + tft_string.center(wc), yc + (hc - tft_string.font_height()) / 2, COLOR_BLACK, tft_string);
        }
        xc += wc;
      }
      yc -= hc;
    }
    touch.add_control(ACTIVE_REGION, F_X, F_Y, F_WIDTH, F_HEIGHT, (intptr_t)&change_point_edit_mesh);

    // draw color legend
    yc = F_X + F_HEIGHT + 14;
    
    // min Z color
    xc = F_WIDTH / 2 - LEG_WIDTH / 2 + F_X;
    tft.canvas(xc, yc, LEG_WIDTH / 3, LEG_HEIGHT);
    z_color = RGB(MIN_COLOR, MIN_COLOR, MAX_COLOR);
    tft.set_background(z_color);
    tft.add_bar(0, 0, LEG_WIDTH / 3, LEG_HEIGHT, z_color);
    tft_string.set((uint8_t *)ftostr43sign(min_z));
    tft.add_text(tft_string.center(LEG_WIDTH / 3), (LEG_HEIGHT - tft_string.font_height()) / 2, COLOR_BLACK, tft_string);
    
    // neitral Z color
    xc += LEG_WIDTH / 3;
    tft.canvas(xc, yc, LEG_WIDTH / 3, LEG_HEIGHT);
    z_color = RGB(MAX_COLOR, MAX_COLOR, MAX_COLOR);
    tft.set_background(z_color);
    tft.add_bar(0, 0, LEG_WIDTH / 3, LEG_HEIGHT, z_color);
    tft_string.set("0.000");
    tft.add_text(tft_string.center(LEG_WIDTH / 3), (LEG_HEIGHT - tft_string.font_height()) / 2, COLOR_BLACK, tft_string);
    
    // max Z color
    xc += LEG_WIDTH / 3;
    tft.canvas(xc, yc, LEG_WIDTH / 3, LEG_HEIGHT);
    z_color = RGB(MAX_COLOR, MIN_COLOR, MIN_COLOR);
    tft.set_background(z_color);
    tft.add_bar(0, 0, LEG_WIDTH / 3, LEG_HEIGHT, z_color);
    tft_string.set((uint8_t *)ftostr43sign(max_z));
    tft.add_text(tft_string.center(LEG_WIDTH / 3), (LEG_HEIGHT - tft_string.font_height()) / 2, COLOR_BLACK, tft_string);
    
    tft.set_font(MENU_FONT_NAME);
    #ifdef SYMBOLS_FONT_NAME
      tft.add_glyphs(SYMBOLS_FONT_NAME);
    #endif


    // current Z value
    xc = F_X + F_WIDTH + 10;
    yc = 15;
    tft.canvas(xc, yc, TFT_WIDTH - xc, 54);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set("Z: ");
    tft_string.add((uint8_t *)ftostr43sign(Z_VALUES(xind, yind)));
    tft.add_text(tft_string.center(TFT_WIDTH - xc), 4, cur_z_color, tft_string);

    // CHANGE button
    yc = 75;
    tft.canvas(xc, yc, TFT_WIDTH - xc, 54);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set((uint8_t *)GET_TEXT(MSG_CHANGE));
    tft.add_text(tft_string.center(TFT_WIDTH - xc), 4, COLOR_MENU_TEXT, tft_string);
    touch.add_control(MENU_SCREEN, xc, yc, TFT_WIDTH - xc, 54, (intptr_t)&change_Z_edit_mesh);

    // DONE button
    yc = 260;
    tft.canvas(xc, yc, TFT_WIDTH - xc, 54);
    tft.set_background(COLOR_BACKGROUND);
    tft_string.set((uint8_t *)GET_TEXT(MSG_BUTTON_DONE));
    tft.add_text(tft_string.center(TFT_WIDTH - xc), 4, COLOR_MENU_TEXT, tft_string);
    touch.add_control(BACK, xc, yc, TFT_WIDTH - xc, 54);


/*
    // BACK_ITEM(MSG_BED_LEVELING);
    EDIT_ITEM(uint8, MSG_MESH_X, &xind, 0, (bedlevel_settings.bedlevel_points.x) - 1);
    EDIT_ITEM(uint8, MSG_MESH_Y, &yind, 0, (bedlevel_settings.bedlevel_points.y) - 1);
    EDIT_ITEM_FAST(float43, MSG_MESH_EDIT_Z, &Z_VALUES(xind, yind), -(LCD_PROBE_Z_RANGE) * 0.5, (LCD_PROBE_Z_RANGE) * 0.5, refresh_planner);
*/
    END_MENU();
  }

  void change_point_edit_mesh()
  {
    uint16_t  coord_x = 0, coord_y = 0;
    uint16_t wc = 0, hc = 0;
    touch.get_last_point((int16_t*)&coord_x, (int16_t*)&coord_y);

    if (coord_x == 0 || coord_y == 0)
      return;
    
    coord_x -= F_X;
    coord_y -= F_Y;

    wc = F_WIDTH / bedlevel_settings.bedlevel_points.x;
    hc = F_HEIGHT / bedlevel_settings.bedlevel_points.y;

    xind = coord_x / wc;
    yind = (F_HEIGHT - coord_y) / hc;

    ui.refresh();
  }


  void change_Z_edit_mesh()
  {
    START_MENU();

    EDIT_ITEM_FAST(float43, MSG_MESH_EDIT_Z, &Z_VALUES(xind, yind), -(LCD_PROBE_Z_RANGE) * 0.5, (LCD_PROBE_Z_RANGE) * 0.5, refresh_planner);

    END_MENU();
  }


#endif // MESH_EDIT_MENU

/**
 * Step 1: Bed Level entry-point
 *
 * << Motion
 *    Auto Home           (if homing needed)
 *    Leveling On/Off     (if data exists, and homed)
 *    Fade Height: ---    (Req: ENABLE_LEVELING_FADE_HEIGHT)
 *    Mesh Z Offset: ---  (Req: MESH_BED_LEVELING)
 *    Z Probe Offset: --- (Req: HAS_BED_PROBE, Opt: BABYSTEP_ZPROBE_OFFSET)
 *    Level Bed >
 *    Level Corners >     (if homed)
 *    Load Settings       (Req: EEPROM_SETTINGS)
 *    Save Settings       (Req: EEPROM_SETTINGS)
 */
void menu_bed_leveling() {
  const bool is_homed = all_axes_trusted(),
             is_valid = leveling_is_valid();

  START_MENU();
  // BACK_ITEM(MSG_MOTION);

  // Auto Home if not using manual probing
  if (bedlevel_settings.bltouch_enabled)
    if (!is_homed) GCODES_ITEM(MSG_AUTO_HOME, G28_STR);

  // Level Bed
  if (!bedlevel_settings.bltouch_enabled)
  {
    // Manual leveling uses a guided procedure
    SUBMENU(MSG_LEVEL_BED_MANUAL, _lcd_level_bed_continue);
  }
  else
  {
    // Automatic leveling can just run the G-code
    GCODES_ITEM(MSG_LEVEL_BED_AUTO, is_homed ? PSTR("G29") : PSTR("G29N"));
  }
  
  #if ENABLED(MESH_EDIT_MENU)
    if (is_valid) SUBMENU(MSG_EDIT_MESH, menu_edit_mesh);
  #endif

  // Homed and leveling is valid? Then leveling can be toggled.
  if (is_homed && is_valid) {
    bool show_state = planner.leveling_active;
    EDIT_ITEM(bool, MSG_BED_LEVELING, &show_state, _lcd_toggle_bed_leveling);
  }

  // Z Fade Height
  #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
    // Shadow for editing the fade height
    editable.decimal = planner.z_fade_height;
    EDIT_ITEM_FAST(float3, MSG_Z_FADE_HEIGHT, &editable.decimal, 0, 100, []{ set_z_fade_height(editable.decimal); });
  #endif

  //
  // Mesh Bed Leveling Z-Offset
  //
  #if ENABLED(MESH_BED_LEVELING)
    #if WITHIN(Z_PROBE_OFFSET_RANGE_MIN, -9, 9)
      #define LCD_Z_OFFSET_TYPE float43    // Values from -9.000 to +9.000
    #else
      #define LCD_Z_OFFSET_TYPE float42_52 // Values from -99.99 to 99.99
    #endif
    EDIT_ITEM(LCD_Z_OFFSET_TYPE, MSG_BED_Z, &mbl.z_offset, Z_PROBE_OFFSET_RANGE_MIN, Z_PROBE_OFFSET_RANGE_MAX);
  #endif

  #if ENABLED(BABYSTEP_ZPROBE_OFFSET)
    SUBMENU(MSG_ZPROBE_ZOFFSET, lcd_babystep_zoffset);
  #else
    if (bedlevel_settings.bltouch_enabled)
      EDIT_ITEM(LCD_Z_OFFSET_TYPE, MSG_ZPROBE_ZOFFSET, &probe.offset.z, Z_PROBE_OFFSET_RANGE_MIN, Z_PROBE_OFFSET_RANGE_MAX);
  #endif

  #if ENABLED(LEVEL_BED_CORNERS)
    SUBMENU(MSG_BED_TRAMMING, _lcd_level_bed_corners);
  
    //
    // Assisted Bed Tramming
    //
    #if ENABLED(ASSISTED_TRAMMING_WIZARD)
      if (bedlevel_settings.bltouch_enabled)
        SUBMENU(MSG_TRAMMING_WIZARD, goto_tramming_wizard);
    #endif

#endif

  #if ENABLED(EEPROM_SETTINGS)
    ACTION_ITEM(MSG_LOAD_EEPROM, ui.load_settings);
    ACTION_ITEM(MSG_STORE_EEPROM, ui.store_settings);
  #endif
  END_MENU();
}

#endif // LCD_BED_LEVELING
