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
// Bed Tramming Wizard
//

#include "../../inc/MarlinConfigPre.h"

#if BOTH(HAS_MARLINUI_MENU, ASSISTED_TRAMMING_WIZARD)

#include "menu_item.h"

#include "../../feature/tramming.h"

#include "../../module/motion.h"
#include "../../module/probe.h"
#include "../../gcode/queue.h"

#if ENABLED(BLTOUCH)
  #include "../../feature/bltouch.h"
#endif

//#define DEBUG_OUT 1
#include "../../core/debug_out.h"

static float    z_measured[G35_PROBE_COUNT];
static Flags<G35_PROBE_COUNT> z_isvalid;
static uint8_t  tram_index = 0;
static bool     ismoved = false;
static int8_t   reference_index; // = 0
static char     turn_screw_msg[64];

#if HAS_LEVELING
  #include "../../feature/bedlevel/bedlevel.h"
#endif

static bool probe_single_point() {
  do_blocking_move_to_z(TERN(BLTOUCH, Z_CLEARANCE_DEPLOY_PROBE, Z_CLEARANCE_BETWEEN_PROBES));
  // Stow after each point with BLTouch "HIGH SPEED" mode for push-pin safety
  set_bed_leveling_enabled(false);
  const float z_probed_height = probe.probe_at_point(tramming_points[tram_index], TERN0(BLTOUCH, bltouch.high_speed_mode) ? PROBE_PT_STOW : PROBE_PT_RAISE, 0, true);
  set_bed_leveling_enabled(true);
  z_measured[tram_index] = z_probed_height;
  if (reference_index < 0) reference_index = tram_index;
  move_to_tramming_wait_pos();

  DEBUG_ECHOLNPGM("probe_single_point(", tram_index, ") = ", z_probed_height, "mm");

  const bool v = !isnan(z_probed_height);
  z_isvalid.set(tram_index, v);

//  float z_val = z_measured[reference_index] - z_measured[tram_index];
  z_measured[tram_index] *= -1;
  float z_val = z_measured[tram_index];
  float turns = 0;
  turn_screw_msg[0] = 0;
  if (z_val > 0.02)
  {
    switch (TRAMMING_SCREW_THREAD)
    {
      case 30:
        turns = z_val / 0.5;
        break;
      case 31:
        turns = z_val / -0.5;
        break;
      case 40:
        turns = z_val / 0.7;
        break;
      case 41:
        turns = z_val / -0.7;
        break;
      case 50:
        turns = z_val / 0.8;
        break;
      case 51:
        turns = z_val / -0.8;
        break;
    }
    if (turns >= 0.02)
      sprintf_P(turn_screw_msg, GET_TEXT(MSG_TURN_SCREW), ftostr12ns(turns), GET_TEXT(MSG_CLOCKWISE));
    else if (turns <= -0.02)
      sprintf_P(turn_screw_msg, GET_TEXT(MSG_TURN_SCREW), ftostr12ns(-turns), GET_TEXT(MSG_COUNTERCLOCKWISE));
  }
  else if (z_val < -0.02)
  {
    switch (TRAMMING_SCREW_THREAD)
    {
      case 30:
        turns = z_val / 0.5;
        break;
      case 31:
        turns = z_val / -0.5;
        break;
      case 40:
        turns = z_val / 0.7;
        break;
      case 41:
        turns = z_val / -0.7;
        break;
      case 50:
        turns = z_val / 0.8;
        break;
      case 51:
        turns = z_val / -0.8;
        break;
    }
    if (turns >= 0.02)
      sprintf_P(turn_screw_msg, GET_TEXT(MSG_TURN_SCREW), ftostr12ns(turns), GET_TEXT(MSG_CLOCKWISE));
    else if (turns <= -0.02)
      sprintf_P(turn_screw_msg, GET_TEXT(MSG_TURN_SCREW), ftostr12ns(-turns), GET_TEXT(MSG_COUNTERCLOCKWISE));
  }
  return (z_isvalid[tram_index]);
}

static void _menu_single_probe()
{
  DEBUG_ECHOLNPGM("Screen: single probe screen Arg:", tram_index);
  START_MENU();
  STATIC_ITEM(MSG_BED_TRAMMING, SS_LEFT);
/*
  STATIC_ITEM(MSG_LAST_VALUE_SP, SS_LEFT, z_isvalid[tram_index] ? ftostr42_52(z_measured[reference_index] - z_measured[tram_index]) : "---");
  if (turn_screw_msg[0] != 0)
    STATIC_ITEM_F(FPSTR(turn_screw_msg), SS_LEFT);
  else
    STATIC_ITEM_F(FPSTR("--"), SS_LEFT);
*/

  STATIC_ITEM(MSG_LAST_VALUE_SP, SS_LEFT, z_isvalid[tram_index] ? ftostr42_52(z_measured[tram_index]) : "---");
  if (turn_screw_msg[0] != 0)
    STATIC_ITEM_F(FPSTR(turn_screw_msg), SS_LEFT);
  else
    STATIC_ITEM_F(FPSTR("--"), SS_LEFT);

  ACTION_ITEM(MSG_UBL_BC_INSERT2, []{ if (probe_single_point()) ui.refresh(); });
  ACTION_ITEM(MSG_BUTTON_DONE, ui.goto_previous_screen);
  END_MENU();
  if (ismoved == false)
  {
    ismoved = true;
//    ui.refresh();
    probe_single_point();
    ui.refresh();
  }

}

static void tramming_wizard_menu()
{
  ismoved = false;
  START_MENU();
  STATIC_ITEM(MSG_SELECT_ORIGIN);

  turn_screw_msg[0] = 0;
  // Draw a menu item for each tramming point
  for (tram_index = 0; tram_index < G35_PROBE_COUNT; tram_index++)
    SUBMENU_F(FPSTR(pgm_read_ptr(&tramming_point_name[tram_index])), _menu_single_probe);

  ACTION_ITEM(MSG_BUTTON_DONE, []{
    probe.stow(); // Stow before exiting Tramming Wizard
    ui.goto_previous_screen_no_defer();
  });
  END_MENU();
}

// Init the wizard and enter the submenu
void goto_tramming_wizard() {
  DEBUG_ECHOLNPGM("Screen: goto_tramming_wizard", 1);
  ui.defer_status_screen();

  // Initialize measured point flags
  z_isvalid.reset();
  reference_index = -1;

  // Inject G28, wait for homing to complete,
  set_all_unhomed();
  queue.inject(TERN(CAN_SET_LEVELING_AFTER_G28, F("G28L0"), FPSTR(G28_STR)));

  ui.goto_screen([]{
    _lcd_draw_homing();
    if (all_axes_homed())
    {
      set_bed_leveling_enabled(false);
      ui.goto_screen(tramming_wizard_menu);
    }
  });
}

#endif // HAS_MARLINUI_MENU && ASSISTED_TRAMMING_WIZARD
