/**
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

// Change EEPROM version if the structure changes
#define EEPROM_VERSION "V86"
#define EEPROM_OFFSET 100

// Check the integrity of data offsets.
// Can be disabled for production build.
//#define DEBUG_EEPROM_READWRITE

#include "filesettings.h"
#include "filesettings_strings.h"

#include "settings.h"
#include "endstops.h"
#include "planner.h"
#include "stepper.h"
#include "temperature.h"

#include "../lcd/marlinui.h"
#include "../libs/vector_3.h"   // for matrix_3x3
#include "../gcode/gcode.h"
#include "../MarlinCore.h"

#include "../lcd/tft/tft_string.h"
#include "../libs/numtostr.h"

#include "../feature/power.h"

#include "probe.h"

#if HAS_LEVELING
  #include "../feature/bedlevel/bedlevel.h"
  #if ENABLED(X_AXIS_TWIST_COMPENSATION)
    #include "../feature/x_twist.h"
  #endif
#endif

#if ENABLED(Z_STEPPER_AUTO_ALIGN)
  #include "../feature/z_stepper_align.h"
#endif

#if ENABLED(HOST_PROMPT_SUPPORT)
  #include "../feature/host_actions.h"
#endif

#if HAS_SERVOS
  #include "servo.h"
#endif

#if HAS_SERVOS && HAS_SERVO_ANGLES
  #define EEPROM_NUM_SERVOS NUM_SERVOS
#else
  #define EEPROM_NUM_SERVOS NUM_SERVO_PLUGS
#endif

#include "../feature/fwretract.h"

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../feature/powerloss.h"
#endif

#if HAS_POWER_MONITOR
  #include "../feature/power_monitor.h"
#endif

#include "../feature/pause.h"

#if ENABLED(BACKLASH_COMPENSATION)
  #include "../feature/backlash.h"
#endif

#if HAS_FILAMENT_SENSOR
  #include "../feature/runout.h"
  #ifndef FIL_RUNOUT_ENABLED_DEFAULT
    #define FIL_RUNOUT_ENABLED_DEFAULT true
  #endif
#endif

#if ENABLED(EXTRA_LIN_ADVANCE_K)
  extern float other_extruder_advance_K[EXTRUDERS];
#endif

#if HAS_MULTI_EXTRUDER
  #include "tool_change.h"
  void M217_report(const bool eeprom);
#endif

#if ENABLED(BLTOUCH)
  #include "../feature/bltouch.h"
#endif

#if HAS_TRINAMIC_CONFIG
  #include "stepper/indirection.h"
  #include "../feature/tmc_util.h"
#endif

#if HAS_PTC
  #include "../feature/probe_temp_comp.h"
#endif

#include "../feature/controllerfan.h"

#if ENABLED(CASE_LIGHT_ENABLE)
  #include "../feature/caselight.h"
#endif

#if ENABLED(PASSWORD_FEATURE)
  #include "../feature/password/password.h"
#endif

#if ENABLED(TOUCH_SCREEN_CALIBRATION)
  #include "../lcd/tft_io/touch_calibration.h"
#endif

#if HAS_ETHERNET
  #include "../feature/ethernet.h"
#endif

#if ENABLED(SOUND_MENU_ITEM)
  #include "../libs/buzzer.h"
#endif

#if HAS_FANCHECK
  #include "../feature/fancheck.h"
#endif

#if ENABLED(DGUS_LCD_UI_MKS)
  #include "../lcd/extui/dgus/DGUSScreenHandler.h"
  #include "../lcd/extui/dgus/DGUSDisplayDef.h"
#endif

#pragma pack(push, 1) // No padding between variables

FileSettings fileSettings;

#define DEBUG_OUT EITHER(EEPROM_CHITCHAT, DEBUG_LEVELING_FEATURE)
#include "../core/debug_out.h"




bool FileSettings::SaveSettings(char *fname /*= NULL*/)
{
  if (card.isFileOpen())
    return false;

  char *filename;
  char curline[512];
  bool wres = false;
  uint32_t len = 0;
  uint16_t lines = 0;

  if (fname == NULL || strlen(fname) < 2)
    filename = (char*)"/printer_settings.ini";
  else
    filename = fname;
  
  if (!card.openFileWrite(filename, true))
    return false;
  
  do
  {
    /******** ACCELERATION ***********/
    sprintf(curline, "%s", (char*)"# ====== ACCELERATION ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_ACCEL_MAX_X, planner.settings.max_acceleration_mm_per_s2[X_AXIS], FSSC_ACCEL_MAX_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_ACCEL_MAX_Y, planner.settings.max_acceleration_mm_per_s2[Y_AXIS], FSSC_ACCEL_MAX_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_ACCEL_MAX_Z, planner.settings.max_acceleration_mm_per_s2[Z_AXIS], FSSC_ACCEL_MAX_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_ACCEL_MAX_E, planner.settings.max_acceleration_mm_per_s2[E_AXIS], FSSC_ACCEL_MAX_E);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
     lines++;
   sprintf(curline, "%s = %.2f %s\r\n", FSS_PRINT_ACCEL, planner.settings.acceleration, FSSC_PRINT_ACCEL);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %.2f %s\r\n", FSS_RETRACT_ACCEL, planner.settings.retract_acceleration, FSSC_RETRACT_ACCEL);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %.2f %s\r\n", FSS_TRAVEL_ACCEL, planner.settings.travel_acceleration, FSSC_TRAVEL_ACCEL);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_MIN_SEGMENT_TIME, planner.settings.min_segment_time_us, FSSC_MIN_SEGMENT_TIME);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    
    /******** STEPS PER MM ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== STEPS PER MM ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_STEPS_PER_MM_X, planner.settings.axis_steps_per_mm[X_AXIS], FSSC_STEPS_PER_MM_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_STEPS_PER_MM_Y, planner.settings.axis_steps_per_mm[Y_AXIS], FSSC_STEPS_PER_MM_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_STEPS_PER_MM_Z, planner.settings.axis_steps_per_mm[Z_AXIS], FSSC_STEPS_PER_MM_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_STEPS_PER_MM_E, planner.settings.axis_steps_per_mm[E_AXIS], FSSC_STEPS_PER_MM_E);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
   
    /******** SPEED ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== SPEED ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_SPEED_X, planner.settings.max_feedrate_mm_s[X_AXIS], FSSC_MAX_SPEED_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_SPEED_Y, planner.settings.max_feedrate_mm_s[Y_AXIS], FSSC_MAX_SPEED_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_SPEED_Z, planner.settings.max_feedrate_mm_s[Z_AXIS], FSSC_MAX_SPEED_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_SPEED_E, planner.settings.max_feedrate_mm_s[E_AXIS], FSSC_MAX_SPEED_E);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    
    /******** JERKS ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== JERKS ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_JERK_X, planner.max_jerk.x, FSSC_MAX_JERK_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_JERK_Y, planner.max_jerk.y, FSSC_MAX_JERK_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_JERK_Z, planner.max_jerk.z, FSSC_MAX_JERK_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_MAX_JERK_E, planner.max_jerk.e, FSSC_MAX_JERK_E);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** STEPPERS ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== STEPPERS ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %s %s\r\n", FSS_STEPPER_INVERT_X, (planner.invert_axis.invert_axis[X_AXIS] ? "Yes" : "No"), FSSC_STEPPER_INVERT_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_STEPPER_INVERT_Y, (planner.invert_axis.invert_axis[Y_AXIS] ? "Yes" : "No"), FSSC_STEPPER_INVERT_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_STEPPER_INVERT_Z1, (planner.invert_axis.invert_axis[Z_AXIS] ? "Yes" : "No"), FSSC_STEPPER_INVERT_Z1);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    #if NUM_Z_STEPPERS == 2
      sprintf(curline, "%s = %s %s\r\n", FSS_STEPPER_INVERT_Z2, (planner.invert_axis.z2_vs_z_dir ? "Yes" : "No"), FSSC_STEPPER_INVERT_Z2);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
    #endif
    sprintf(curline, "%s = %s %s\r\n", FSS_STEPPER_INVERT_E, (planner.invert_axis.invert_axis[E_AXIS] ? "Yes" : "No"), FSSC_STEPPER_INVERT_E);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** HOME OFFSET ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== HOME OFFSET ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_HOME_OFFSET_X, home_offset.x, FSSC_HOME_OFFSET_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_HOME_OFFSET_Y, home_offset.y, FSSC_HOME_OFFSET_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_HOME_OFFSET_Z, home_offset.z, FSSC_HOME_OFFSET_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    
    /******** FILAMENT RUNOUT SENSOR ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== FILAMENT RUNOUT SENSOR ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    #if ENABLED(FILAMENT_RUNOUT_SENSOR)
      sprintf(curline, "%s = %s %s\r\n", FSS_FILAMENTSENSOR_ENABLED, (runout.enabled ? "Yes" : "No"), FSSC_FILAMENTSENSOR_ENABLED);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
      sprintf(curline, "%s = %0.2f %s\r\n", FSS_FILAMENTSENSOR_DISTANCE, runout.runout_distance(), FSSC_FILAMENTSENSOR_DISTANCE);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
    #endif
    
    /******** BED LEVELING ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== BED LEVELING ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_BEDLEVEL_FADE_HEIGHT, planner.z_fade_height, FSSC_BEDLEVEL_FADE_HEIGHT);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_BEDLEVEL_BLTOUCH_ENABLED, (bedlevel_settings.bltouch_enabled ? "Yes" : "No"), FSSC_BEDLEVEL_BLTOUCH_ENABLED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_BEDLEVEL_BLTOUCH_INVERT, (endstop_settings.Z_MIN_PROBE_INVERTING ? "Yes" : "No"), FSSC_BEDLEVEL_BLTOUCH_INVERT);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_BEDLEVEL_BLTOUCH_OFFSET_X, probe.offset.x, FSSC_BEDLEVEL_BLTOUCH_OFFSET_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_BEDLEVEL_BLTOUCH_OFFSET_Y, probe.offset.y, FSSC_BEDLEVEL_BLTOUCH_OFFSET_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.3f %s\r\n", FSS_BEDLEVEL_BLTOUCH_OFFSET_Z, probe.offset.z, FSSC_BEDLEVEL_BLTOUCH_OFFSET_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_BEDLEVEL_POINTS_X, bedlevel_settings.bedlevel_points.x, FSSC_BEDLEVEL_POINTS_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_BEDLEVEL_POINTS_Y, bedlevel_settings.bedlevel_points.y, FSSC_BEDLEVEL_POINTS_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    sprintf(curline, "%s = ", FSS_BEDLEVEL_VALUES);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    for (uint8_t iy = 0; iy < GRID_MAX_POINTS_Y; iy++)
    {
      for (uint8_t ix = 0; ix < GRID_MAX_POINTS_X; ix++)
      {
        sprintf(curline, "%0.3f%s ",
                    isnan(bedlevel.z_values[ix][iy]) ? 0 : bedlevel.z_values[ix][iy],
                    (iy == GRID_MAX_POINTS_Y-1 && ix == GRID_MAX_POINTS_X-1) ? "" : ","
                );
        len = strlen(curline);
        if (card.write(curline, len) != len)
          break;
      }
    }
    sprintf(curline, "%s\r\n", FSSC_BEDLEVEL_VALUES);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** ENDSTOPS ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== ENDSTOPS ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %s %s\r\n", FSS_ENDSTOP_INVERT_X, (endstop_settings.X_MIN_INVERTING ? "Yes" : "No"), FSSC_ENDSTOP_INVERT_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_ENDSTOP_INVERT_Y, (endstop_settings.Y_MIN_INVERTING ? "Yes" : "No"), FSSC_ENDSTOP_INVERT_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %s %s\r\n", FSS_ENDSTOP_INVERT_Z1, (endstop_settings.Z_MIN_INVERTING ? "Yes" : "No"), FSSC_ENDSTOP_INVERT_Z1);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    #if HAS_Z2_MIN
      sprintf(curline, "%s = %s %s\r\n", FSS_ENDSTOP_INVERT_Z2, (endstop_settings.Z2_MIN_INVERTING ? "Yes" : "No"), FSSC_ENDSTOP_INVERT_Z2);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
      sprintf(curline, "%s = %0.3f %s\r\n", FSS_ENDSTOP_ADJUST_Z2, endstops.z2_endstop_adj, FSSC_ENDSTOP_ADJUST_Z2);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
    #endif
    
    /******** TEMPERATURE ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== TEMPERATURE ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_HOTEND_1, ui.material_preset[0].hotend_temp, FSSC_PREHEAT_HOTEND_1);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_BED_1, ui.material_preset[0].bed_temp, FSSC_PREHEAT_BED_1);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_FAN_1, ui.material_preset[0].fan_speed, FSSC_PREHEAT_FAN_1);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_HOTEND_2, ui.material_preset[1].hotend_temp, FSSC_PREHEAT_HOTEND_2);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_BED_2, ui.material_preset[1].bed_temp, FSSC_PREHEAT_BED_2);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_PREHEAT_FAN_2, ui.material_preset[1].fan_speed, FSSC_PREHEAT_FAN_2);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_HOTEND_P, thermalManager.temp_hotend[0].pid.Kp, FSSC_PID_HOTEND_P);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_HOTEND_I, thermalManager.temp_hotend[0].pid.Ki, FSSC_PID_HOTEND_I);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_HOTEND_D, thermalManager.temp_hotend[0].pid.Kd, FSSC_PID_HOTEND_D);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    #if ENABLED(PIDTEMPBED)
      sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_BED_P, thermalManager.temp_bed.pid.Kp, FSSC_PID_BED_P);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
      sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_BED_I, thermalManager.temp_bed.pid.Ki, FSSC_PID_BED_I);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
      sprintf(curline, "%s = %0.5f %s\r\n", FSS_PID_BED_D, thermalManager.temp_bed.pid.Kd, FSSC_PID_BED_D);
      len = strlen(curline);
      if (card.write(curline, len) != len)
        break;
      lines++;
    #endif
    sprintf(curline, "%s = %d %s\r\n", FSS_THERMISTOR_TYPE_HOTEND, thermistors_data.heater_type[0], FSSC_THERMISTOR_TYPE_HOTEND);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_THERMISTOR_TYPE_BED, thermistors_data.bed_type, FSSC_THERMISTOR_TYPE_BED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
   
    /******** LCD ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== LCD ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %d %s\r\n", FSS_LCD_BRIGHTNESS, ui.brightness, FSSC_LCD_BRIGHTNESS);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_LCD_TOUCH_X, touch_calibration.calibration.x, FSSC_LCD_TOUCH_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_LCD_TOUCH_Y, touch_calibration.calibration.y, FSSC_LCD_TOUCH_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_LCD_TOUCH_OFFSET_X, touch_calibration.calibration.offset_x, FSSC_LCD_TOUCH_OFFSET_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_LCD_TOUCH_OFFSET_Y, touch_calibration.calibration.offset_y, FSSC_LCD_TOUCH_OFFSET_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %d %s\r\n", FSS_LCD_LANGUAGE, ui.language, FSSC_LCD_LANGUAGE);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** PSU ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== PSU ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %s %s\r\n", FSS_PSU_ENABLED, (psu_settings.psu_enabled ? "Yes" : "No"), FSSC_PSU_ENABLED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** FW RETRACT ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== FW RETRACT ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_LENGTH, fwretract.settings.retract_length, FSSC_FWRETRACT_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_SPEED, fwretract.settings.retract_feedrate_mm_s, FSSC_FWRETRACT_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_Z_HOP, fwretract.settings.retract_zraise, FSSC_FWRETRACT_Z_HOP);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_RECOVER_LENGTH, fwretract.settings.retract_recover_extra, FSSC_FWRETRACT_RECOVER_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_RECOVER_SPEED, fwretract.settings.retract_recover_feedrate_mm_s, FSSC_FWRETRACT_RECOVER_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_SWP_LENGTH, fwretract.settings.swap_retract_length, FSSC_FWRETRACT_SWP_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_RECOVER_SWP_LENGTH, fwretract.settings.swap_retract_recover_extra, FSSC_FWRETRACT_RECOVER_SWP_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.2f %s\r\n", FSS_FWRETRACT_RECOVER_SWP_SPEED, fwretract.settings.swap_retract_recover_feedrate_mm_s, FSSC_FWRETRACT_RECOVER_SWP_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    
    /******** LINEAR ADVANCE ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== LINEAR ADVANCE ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.4f %s\r\n", FSS_LA_KFACTOR, planner.extruder_advance_K[0], FSSC_LA_KFACTOR);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;

    /******** PARKING / FILAMENT CHANGE ***********/
    sprintf(curline, "%s", (char*)"\r\n# ====== PARKING / FILAMENT CHANGE ======\r\n");
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines += 2;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_POINT_X, moving_settings.pause.park_point_x, FSSC_PARK_POINT_X);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_POINT_Y, moving_settings.pause.park_point_y, FSSC_PARK_POINT_Y);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_POINT_Z, moving_settings.pause.park_point_z, FSSC_PARK_POINT_Z);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_MOVE_SPEED, moving_settings.pause.park_move_feedrate, FSSC_PARK_MOVE_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_RETR_SPEED, moving_settings.pause.retract_feedrate, FSSC_PARK_RETR_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_PARK_RETR_LENGTH, moving_settings.pause.retract_length, FSSC_PARK_RETR_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %ld %s\r\n", FSS_PARK_HEATER_TIMEOUT, moving_settings.pause.heater_timeout, FSSC_PARK_HEATER_TIMEOUT);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_UNLOAD_SPEED, moving_settings.filament_change.unload_feedrate, FSSC_UNLOAD_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_UNLOAD_LENGTH, moving_settings.filament_change.unload_length, FSSC_UNLOAD_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_SLOW_LOAD_SPEED, moving_settings.filament_change.slow_load_feedrate, FSSC_SLOW_LOAD_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_SLOW_LOAD_LENGTH, moving_settings.filament_change.slow_load_length, FSSC_SLOW_LOAD_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_FAST_LOAD_SPEED, moving_settings.filament_change.fast_load_feedrate, FSSC_FAST_LOAD_SPEED);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;
    sprintf(curline, "%s = %0.1f %s\r\n", FSS_FAST_LOAD_LENGTH, moving_settings.filament_change.fast_load_length, FSSC_FAST_LOAD_LENGTH);
    len = strlen(curline);
    if (card.write(curline, len) != len)
      break;
    lines++;


    wres = true;
  } while (0);

  card.closefile();

  if (wres)
    OKAY_BUZZ();
  else
    ERR_BUZZ();
  SERIAL_ECHOLNPGM("M5000: lines writed - ", lines);

  return wres;
}



bool FileSettings::LoadSettings(char *fname /*= NULL*/)
{
  if (card.isFileOpen())
    return false;

  char        *filename;
  char        msg[512];
	char			  lexem[128];
  char        *string;
  bool        wres = true;
	uint16_t	  cnt = 0;
	UINT		    readed = 0;
	int16_t		  lines = 0, params = 0, params_old = 0;
	PARAM_VALUE	pval;

  bool        fwretr_update = false;
  bool        bedlevel_update = false;

  if (fname == NULL || strlen(fname) < 2)
    filename = (char*)"/printer_settings.ini";
  else
    filename = fname;
  
  if (!card.openFileRead(filename, true))
    return false;
  
  do
  {
		// read one string
		cnt = 0;
		readed = 0;
		string = msg;
		while (cnt < sizeof(msg))
		{
      readed = card.read(string, 1);
			if (readed == 0 || *string == '\n')
			{
				*string = 0;
				break;
			}
			cnt++;
			string++;
		}
		if (cnt == sizeof(msg))
		{
			string--;
			*string = 0;
		}
		lines++;
		string = msg;
		
		// trim spaces/tabs at begin and end
		strtrim(string);
    		
		// if string is empty
		if (*string == 0)
		{
			// if end of file
			if (readed == 0)
      {
        wres = true;
				break;
      }
			else
      {
				continue;
      }
		}
		
		// upper all letters
		strupper_utf(string);
		
		// get parameter name
		string = _getParamName(string, lexem, sizeof(lexem));
		
		// skip comments
		if (*lexem == '#')
			continue;
		
		// get parameter value
		string = _getParamValue(string, &pval);
		if (pval.type == PARAMVAL_NONE)
		{
      wres = false;
			break;
		}
		
		// check and setup parameter
    switch (*lexem)
    {
      case 'A':   // ***************************  AAA  ********************************
        if (strcmp(lexem, FSS_ACCEL_MAX_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.max_acceleration_mm_per_s2[X_AXIS] = (uint32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_ACCEL_MAX_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.max_acceleration_mm_per_s2[Y_AXIS] = (uint32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_ACCEL_MAX_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.max_acceleration_mm_per_s2[Z_AXIS] = (uint32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_ACCEL_MAX_E) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.max_acceleration_mm_per_s2[E_AXIS] = (uint32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PRINT_ACCEL) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.acceleration = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_RETRACT_ACCEL) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.retract_acceleration = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_TRAVEL_ACCEL) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          planner.settings.travel_acceleration = (float)pval.float_val;
          params++;
          break;
        }
        break;

      case 'B':   // ***************************  BBB  ********************************
        if (strcmp(lexem, FSS_BEDLEVEL_FADE_HEIGHT) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          if (pval.float_val > 10000)
            pval.float_val = 10000;
          set_z_fade_height((float)pval.float_val, false);
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_BLTOUCH_ENABLED) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          bedlevel_settings.bltouch_enabled = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_BLTOUCH_INVERT) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          endstop_settings.Z_MIN_PROBE_INVERTING = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_BLTOUCH_OFFSET_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          probe.offset.x = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_BLTOUCH_OFFSET_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          probe.offset.y = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_BLTOUCH_OFFSET_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          probe.offset.z = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_POINTS_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 3)
            pval.float_val = 3;
          if (pval.float_val > GRID_MAX_POINTS_X)
            pval.float_val = GRID_MAX_POINTS_X;
          bedlevel_settings.bedlevel_points.x = (uint8_t)pval.float_val;
          bedlevel_update = true;;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_POINTS_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 3)
            pval.float_val = 3;
          if (pval.float_val > GRID_MAX_POINTS_Y)
            pval.float_val = GRID_MAX_POINTS_Y;
          bedlevel_settings.bedlevel_points.y = (uint8_t)pval.float_val;
          bedlevel_update = true;;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_BEDLEVEL_VALUES) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          uint8_t ix = 0, iy = 0;
          for (; iy < GRID_MAX_POINTS_Y; iy++)
          {
            for (; ix < GRID_MAX_POINTS_X; ix++)
            {
              bedlevel.z_values[ix][iy] = (float)pval.float_val;
              if (iy < GRID_MAX_POINTS_Y-1 || ix < GRID_MAX_POINTS_X-1)
              {
                string = _getParamValue(string, &pval);
                if (pval.type != PARAMVAL_NUMERIC)
                {
                  wres = false;
                  break;
                }
              }
            }
            if (!wres)
              break;
            ix = 0;
          }
          bedlevel_update = true;;
          params++;
          break;
        }
        break;
      
      case 'E':   // ***************************  EEE  ********************************
        if (strcmp(lexem, FSS_ENDSTOP_INVERT_X) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          endstop_settings.X_MIN_INVERTING = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_ENDSTOP_INVERT_Y) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          endstop_settings.Y_MIN_INVERTING = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_ENDSTOP_INVERT_Z1) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          endstop_settings.Z_MIN_INVERTING = pval.bool_val;
          params++;
          break;
        }
        #if HAS_Z2_MIN
          if (strcmp(lexem, FSS_ENDSTOP_INVERT_Z2) == 0)
          {
            if (pval.type != PARAMVAL_BOOL)
            {
              wres = false;
              break;
            }
            endstop_settings.Z2_MIN_INVERTING = pval.bool_val;
            params++;
            break;
          }
          if (strcmp(lexem, FSS_ENDSTOP_ADJUST_Z2) == 0)
          {
            if (pval.type != PARAMVAL_NUMERIC)
            {
              wres = false;
              break;
            }
            endstops.z2_endstop_adj = pval.float_val;
            params++;
            break;
          }
        #endif
        break;

      case 'F':   // ***************************  FFF  ********************************
        #if ENABLED(FILAMENT_RUNOUT_SENSOR)
          if (strcmp(lexem, FSS_FILAMENTSENSOR_ENABLED) == 0)
          {
            if (pval.type != PARAMVAL_BOOL)
            {
              wres = false;
              break;
            }
            runout.enabled = pval.bool_val;
            if (runout.enabled)
              runout.reset();
            params++;
            break;
          }
          if (strcmp(lexem, FSS_FILAMENTSENSOR_DISTANCE) == 0)
          {
            if (pval.type != PARAMVAL_NUMERIC)
            {
              wres = false;
              break;
            }
            if (pval.float_val < 0)
              pval.float_val = 0;
            runout.set_runout_distance((float)pval.float_val);
            params++;
            break;
          }
        #endif
        if (strcmp(lexem, FSS_FWRETRACT_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.retract_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.retract_feedrate_mm_s = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_Z_HOP) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.retract_zraise = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_RECOVER_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.retract_recover_extra = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_RECOVER_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.retract_recover_feedrate_mm_s = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_SWP_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.swap_retract_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_RECOVER_SWP_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.swap_retract_recover_extra = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FWRETRACT_RECOVER_SWP_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          fwretract.settings.swap_retract_recover_feedrate_mm_s = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FAST_LOAD_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          moving_settings.filament_change.fast_load_feedrate = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_FAST_LOAD_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.filament_change.fast_load_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        break;
      
      case 'H':   // ***************************  HHH  ********************************
        if (strcmp(lexem, FSS_HOME_OFFSET_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          home_offset.x = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_HOME_OFFSET_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          home_offset.y = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_HOME_OFFSET_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          home_offset.z = (float)pval.float_val;
          params++;
          break;
        }
        break;
      
      case 'J':   // ***************************  JJJ  ********************************
        if (strcmp(lexem, FSS_MAX_JERK_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          planner.max_jerk.x = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_JERK_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          planner.max_jerk.y = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_JERK_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          planner.max_jerk.z = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_JERK_E) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          planner.max_jerk.e = (float)pval.float_val;
          params++;
          break;
        }
        break;

      case 'L':   // ***************************  LLL  ********************************
        if (strcmp(lexem, FSS_LCD_BRIGHTNESS) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          if (pval.float_val > LCD_BRIGHTNESS_STEPS-1)
            pval.float_val = LCD_BRIGHTNESS_STEPS-1;
          ui.set_brightness((uint8_t)pval.float_val);
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LCD_TOUCH_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          touch_calibration.calibration.x = (int32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LCD_TOUCH_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          touch_calibration.calibration.y = (int32_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LCD_TOUCH_OFFSET_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          touch_calibration.calibration.offset_x = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LCD_TOUCH_OFFSET_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          touch_calibration.calibration.offset_y = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LCD_LANGUAGE) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val >= NUM_LANGUAGES || pval.float_val < 0)
            pval.float_val = 0;
          ui.language = (uint8_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_LA_KFACTOR) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          planner.extruder_advance_K[0] = (float)pval.float_val;
          params++;
          break;
        }
        break;
      
      case 'M':   // ***************************  MMM  ********************************
        if (strcmp(lexem, FSS_MIN_SEGMENT_TIME) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          planner.settings.min_segment_time_us = (uint32_t)pval.float_val;
          params++;
          break;
        }
        break;

      case 'P':   // ***************************  PPP  ********************************
        if (strcmp(lexem, FSS_PREHEAT_HOTEND_1) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[0].hotend_temp = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PREHEAT_HOTEND_2) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[1].hotend_temp = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PREHEAT_BED_1) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[0].bed_temp = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PREHEAT_BED_2) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[1].bed_temp = (int16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PREHEAT_FAN_1) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[0].fan_speed = (uint16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PREHEAT_FAN_2) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          ui.material_preset[1].fan_speed = (uint16_t)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PID_HOTEND_P) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          thermalManager.temp_hotend[0].pid.Kp = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PID_HOTEND_I) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          thermalManager.temp_hotend[0].pid.Ki = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PID_HOTEND_D) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          thermalManager.temp_hotend[0].pid.Kd = (float)pval.float_val;
          params++;
          break;
        }
        #if ENABLED(PIDTEMPBED)
          if (strcmp(lexem, FSS_PID_BED_P) == 0)
          {
            if (pval.type != PARAMVAL_NUMERIC)
            {
              wres = false;
              break;
            }
            thermalManager.temp_bed.pid.Kp = (float)pval.float_val;
            params++;
            break;
          }
          if (strcmp(lexem, FSS_PID_BED_I) == 0)
          {
            if (pval.type != PARAMVAL_NUMERIC)
            {
              wres = false;
              break;
            }
            thermalManager.temp_bed.pid.Ki = (float)pval.float_val;
            params++;
            break;
          }
          if (strcmp(lexem, FSS_PID_BED_D) == 0)
          {
            if (pval.type != PARAMVAL_NUMERIC)
            {
              wres = false;
              break;
            }
            thermalManager.temp_bed.pid.Kd = (float)pval.float_val;
            params++;
            break;
          }
        #endif
        if (strcmp(lexem, FSS_PSU_ENABLED) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          psu_settings.psu_enabled = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_POINT_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.park_point_x = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_POINT_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.park_point_y = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_POINT_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.park_point_z = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_MOVE_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.park_move_feedrate = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_RETR_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.retract_feedrate = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_RETR_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.retract_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_PARK_HEATER_TIMEOUT) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.pause.heater_timeout = (uint32_t)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        break;
      
      case 'S':   // ***************************  SSS  ********************************
        if (strcmp(lexem, FSS_STEPS_PER_MM_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.axis_steps_per_mm[X_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPS_PER_MM_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.axis_steps_per_mm[Y_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPS_PER_MM_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.axis_steps_per_mm[Z_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPS_PER_MM_E) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.axis_steps_per_mm[E_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_SPEED_X) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.max_feedrate_mm_s[X_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_SPEED_Y) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.max_feedrate_mm_s[Y_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_SPEED_Z) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.max_feedrate_mm_s[Z_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MAX_SPEED_E) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.max_feedrate_mm_s[E_AXIS] = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MIN_PRINT_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.min_feedrate_mm_s = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_MIN_TRAVEL_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0.1)
            pval.float_val = 0.1;
          planner.settings.min_travel_feedrate_mm_s = (float)pval.float_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPPER_INVERT_X) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          planner.invert_axis.invert_axis[X_AXIS] = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPPER_INVERT_Y) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          planner.invert_axis.invert_axis[Y_AXIS] = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_STEPPER_INVERT_Z1) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          planner.invert_axis.invert_axis[Z_AXIS] = pval.bool_val;
          params++;
          break;
        }
        #if NUM_Z_STEPPERS == 2
          if (strcmp(lexem, FSS_STEPPER_INVERT_Z2) == 0)
          {
            if (pval.type != PARAMVAL_BOOL)
            {
              wres = false;
              break;
            }
            planner.invert_axis.z2_vs_z_dir = pval.bool_val;
            params++;
            break;
          }
        #endif
        if (strcmp(lexem, FSS_STEPPER_INVERT_E) == 0)
        {
          if (pval.type != PARAMVAL_BOOL)
          {
            wres = false;
            break;
          }
          planner.invert_axis.invert_axis[E_AXIS] = pval.bool_val;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_SLOW_LOAD_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.filament_change.slow_load_feedrate = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_SLOW_LOAD_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.filament_change.slow_load_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        break;
      
      case 'T':   // ***************************  TTT  ********************************
        if (strcmp(lexem, FSS_THERMISTOR_TYPE_HOTEND) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          if (pval.float_val >= THERMISTORS_TYPES_COUNT)
            pval.float_val = 0;
          thermistors_data.heater_type[0] = (uint8_t)pval.float_val;
          thermistors_data.fan_auto_temp[0] = thermistor_types[(uint8_t)pval.float_val].fan_auto_temp;
          thermistors_data.high_temp[0] = thermistor_types[(uint8_t)pval.float_val].high_temp;
          thermalManager.hotend_maxtemp[0] = thermistor_types[(uint8_t)pval.float_val].max_temp;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_THERMISTOR_TYPE_BED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          if (pval.float_val >= THERMISTORS_TYPES_COUNT)
            pval.float_val = 0;
          thermistors_data.bed_type = (uint8_t)pval.float_val;
          params++;
          break;
        }
        break;

      case 'U':   // ***************************  UUU  ********************************
        if (strcmp(lexem, FSS_UNLOAD_SPEED) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.filament_change.unload_feedrate = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        if (strcmp(lexem, FSS_UNLOAD_LENGTH) == 0)
        {
          if (pval.type != PARAMVAL_NUMERIC)
          {
            wres = false;
            break;
          }
          if (pval.float_val < 0)
            pval.float_val = 0;
          moving_settings.filament_change.fast_load_length = (float)pval.float_val;
          fwretr_update = true;
          params++;
          break;
        }
        break;
      

      default:
        break;
    }

    if (params == params_old)
    {
      SERIAL_ECHOPGM("M5001: unknown parameter - ", lexem);
      SERIAL_ECHOLNPGM(" - in line - ", lines);
    }
    params_old = params;

		if (!wres)
			break;
		
  } while (1);
  

  card.closefile();

  if (fwretr_update)
    fwretract.refresh_autoretract();

  if (bedlevel_update)
  {
    set_bed_leveling_enabled(false);
    bedlevel.refresh_bed_level();
    if (planner.leveling_active)
      set_bed_leveling_enabled(true);
  }


  if (wres)
  {
    SERIAL_ECHOLNPGM("M5001: lines readed - ", lines);
    SERIAL_ECHOLNPGM("M5001: parameters readed - ", params);
    OKAY_BUZZ();
  }
  else
  {
    SERIAL_ECHOLNPGM("M5001: failure in line - ", lines);
    ERR_BUZZ();
  }

  return wres;
}



char* FileSettings::_getParamName(char *src, char *dest, uint16_t maxlen)
{
	if (src == NULL || dest == NULL)
		return src;
	
//	char *string = src;
	// skip spaces
	while (*src != 0 && maxlen > 0 && (*src == ' ' || *src == '\t' || *src == '\r' || *src > 126))
	{
		src++;
		maxlen--;
	}
	// until first space symbol
	while (maxlen > 0 && *src != 0 && *src != ' ' && *src != '\t' && *src != '\r' && *src != '\n' && *src != '=')
	{
		*dest = *src;
		dest++;
		src++;
		maxlen--;
	}
	
	if (maxlen == 0)
		dest--;
	
	*dest = 0;

  if (*src != 0)
  {
    // skip symbols to first value if one exists
    while (*src > 0 && (*src == ' ' || *src == '\t' || *src == '='))
      src++;
  }

	return src;
}
//==============================================================================




char* FileSettings::_getParamValue(char *src, PARAM_VALUE *val)
{
	val->type = PARAMVAL_NONE;
	val->float_val = 0;
	val->bool_val = false;
	val->char_val = (char*)"";
	
	if (src == NULL)
		return src;
	if (val == NULL)
		return src;
	
	// skip spaces
	while (*src != 0 && (*src == ' ' || *src == '\t' || *src == '\r'))
		src++;
  // End of line or begin comment
	if (*src == 0 || *src == '#')
		return src;

	// check param if it numeric
	if ((*src > 47 && *src < 58) || *src == '.' || (*src == '-' && ((*(src+1) > 47 && *(src+1) < 58) || *(src+1) == '.')))
	{
		val->type = PARAMVAL_NUMERIC;
		val->float_val = atof(src);
    val->bool_val = (val->float_val != 0);
		val->char_val = src;
	}
  // check param if it boolean
	else if (strncmp(src, "YES", 3) == 0 || strncmp(src, "NO", 2) == 0)
  {
		val->type = PARAMVAL_BOOL;
    if (strncmp(src, "YES", 3) == 0)
    {
		  val->bool_val = true;
  		val->float_val = 1;
  		val->char_val = src;
    }
    else
    {
		  val->bool_val = false;
  		val->float_val = 0;
  		val->char_val = src;
    }
	}
  // param is string
  else
  {
		val->type = PARAMVAL_STRING;
		val->char_val = src;
  }
	
  if (val->type != PARAMVAL_NONE)
  {
    // skip symbols to next value if one exists
    while (*src != ' ' && *src != '\t' && *src != '#' && *src != ',')
      src++;
    while (*src > 0 && (*src == ' ' || *src == '\t' || *src == ','))
      src++;
  }
	return src;
}
//==============================================================================



void FileSettings::_skipToNextLine(char *src)
{
  while (*src != 0 && *src != '\n')
    src++;
  if (*src == '\n')
    src++;
}
//==============================================================================




void FileSettings::postprocess()
{

}


#pragma pack(pop)
