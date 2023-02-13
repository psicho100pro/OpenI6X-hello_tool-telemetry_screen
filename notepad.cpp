#include "opentx.h"

/*
!!! TINSIZE writes only capital letters and nums !!! the raster is too coarse
due to the lack of space, the effort is to simplify everything
*/

#define ROOF FH                                  // y lcd.h FH 8  pak smažu
#define FLOOR (LCD_H - ROOF - 1)                  // y currently 55 , adjustable, it needs to be an even number cus center
#define HCENTER (ROOF + ((FLOOR - 1 - ROOF) / 2)) // y currently 31
#define HORLLIM 0                                // x
#define HORDLIM 97                               // x divid, adjustable, it needs to be an odd number cus center
#define HORRLIM (LCD_W - 1)
#define HORWCENTER ((HORDLIM - 1) / 2) // x currently 48

static const uint8_t lut[] = { // sin table fixed SCALE 6
0,1,2,3,4,6,7,8,9,10,
11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,48,
49,50,50,51,52,52,53,54,54,55,55,56,57,57,58,58,58,59,59,60,
60,61,61,61,62,62,62,62,63,63,63,63,63,64,64,64,64,64,64,64
};

int8_t fSinCos(float rad, bool f = 0) // 0=sin 1=cos // it is not very accurate, but the display is not very fine either
{
// the telemetry comes back in rads: 0°=0 ; 90°=1.57 ; 180°=3.14 ; 270°=4.71 ; 360°=6.28
// int16_t ang = 180/3.14159*(f? rad + 1.57:rad) ;
  int32_t ang = ((f ? ((rad * (1 << 6)) + (uint8_t)100) : (rad * (1 << 6))) * ((uint16_t)11520 * (1 << 6) / (uint8_t)201));
  uint16_t ab = abs(ang>>12);
  int8_t ret;
  if (ab < 90) { ret = lut[ab % 90]; }
  else if (ab < 180) { ret = lut[90 - ab % 90]; }
  else if (ab < 270) { ret = -lut[ab % 90]; }
  else { ret = -lut[90 - ab % 90]; }
  return (rad >= 0) ? ret : -ret;
};


uint8_t hYlimit(uint8_t val) // y over limit
{
  if (ROOF >= val) return ROOF + 1;
  if (val >= FLOOR) return FLOOR - 1;
  return val;
};

void hello_draw()
{

uint8_t ahcx = HORWCENTER;             // TODO: same , so
uint8_t ahcy = HCENTER;

// ROOF bar:
lcdDrawSolidFilledRect(0, 0, HORRLIM, ROOF);
putsVolts(1, 1, g_vbat100mV, SMLSIZE | INVERS | (IS_TXBATT_WARNING() ? BLINK : 0));
lcdDrawText(lcdNextPos + 1, 1, "TW:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, 1, telemetryItems[6].value, SMLSIZE | INVERS);
//putsModelName(lcdNextPos + 1, 1, g_model.header.name, g_eeGeneral.currModel, SMLSIZE | INVERS);
drawFlightMode(lcdNextPos + 1, 1, mixerCurrentFlightMode, SMLSIZE | INVERS);
lcdDrawText(90, 1, "T:", SMLSIZE | INVERS);
if (g_model.timers[0].mode)
{
  drawTimer(lcdNextPos + 2, 1, timersStates[0].val, SMLSIZE | INVERS | (timersStates[0].val < 0 ? BLINK : 0)); // -00:00 este pozjistovat trochu
};

// margin line
lcdDrawLine(HORDLIM, ROOF, HORDLIM, FLOOR); // divid

int8_t sin = fSinCos(telemetryItems[15].value); //30°°
int8_t cos = fSinCos(telemetryItems[15].value, 1);
int16_t tan = (sin * (1 << 6) / cos); // 90° vyřešit potom

  
// pitch
ahcy = hYlimit(((uint16_t(42 /*FOV*/ << 6) * -fSinCos(telemetryItems[14].value)) >> 12) + ahcy); // FOV fe. 45°=30, 40°=32, 35°=36 ,30°=42, 25°=60 - when the horizon touches ROOF or FLOOR
// roll

// ground fill // works // not yet => memspace
for (uint8_t x = HORLLIM + 1; x < ahcx; x++)
{
  lcdDrawLine(ahcx - x, hYlimit(ahcy + ((tan * uint16_t(x << 6)) >> 12)), ahcx - x, FLOOR - 1); // L
  lcdDrawLine(ahcx + x, hYlimit(ahcy - ((tan * uint16_t(x << 6)) >> 12)), ahcx + x, FLOOR - 1); // R
};
lcdDrawLine(ahcx, ahcy, ahcx, FLOOR - 1); // mid



  // crosshair :D
  lcdDrawLine(HORWCENTER, HCENTER - 1, HORWCENTER, HCENTER + 1);
  lcdDrawLine(HORWCENTER - 1, HCENTER, HORWCENTER + 1, HCENTER);
  lcdDrawLine(HORWCENTER - 6, HCENTER, HORWCENTER - 6, HCENTER + 3);
  lcdDrawLine(HORWCENTER + 6, HCENTER, HORWCENTER + 6, HCENTER + 3);
  lcdDrawLine(HORWCENTER + 7, HCENTER, HORWCENTER + 15, HCENTER);
  lcdDrawLine(HORWCENTER - 15, HCENTER, HORWCENTER - 7, HCENTER);
  // yaw
  lcdDrawNumber(HORWCENTER - 2 * FWNUM + 3, FLOOR - FH + 1, 365, SMLSIZE | INVERS);

  // side indicators
lcdDrawFilledRect(HORLLIM + 1, HCENTER - 5, FWNUM * 3 + 2, 10, SOLID, FORCE);
lcdDrawFilledRect(HORLLIM + 1, HCENTER - 4, FWNUM * 3 + 1, 8, SOLID, ERASE);
lcdDrawNumber(HORLLIM + 2, HCENTER - 3, 888, SMLSIZE);
lcdDrawFilledRect(HORDLIM - FWNUM * 3 - 2, HCENTER - 5, FWNUM * 3 + 2, 10, SOLID, FORCE);
lcdDrawFilledRect(HORDLIM - FWNUM * 3 - 1, HCENTER - 4, FWNUM * 3 + 1, 8, SOLID, ERASE);
lcdDrawNumber(HORDLIM - FWNUM * 3, HCENTER - 3, telemetryItems[16].value, SMLSIZE);

  // rigth block: // not completed
lcdDrawText(HORRLIM - 20, ROOF + 2, "S:", SMLSIZE);     // blink?
lcdDrawNumber(HORRLIM, ROOF + 2, 5, SMLSIZE | RIGHT);   // count of sats
lcdDrawNumber(HORRLIM, ROOF + 10, 10, SMLSIZE | RIGHT); //  drawValueWithUnit ??

  // FLOOR bar:
lcdDrawSolidFilledRect(0, FLOOR + 1, HORRLIM, FLOOR + 1);
putsVolts(1, FLOOR + 2, telemetryItems[10].value, SMLSIZE | INVERS); // 12 . 6
lcdDrawText(lcdNextPos + 1, FLOOR + 2, "RS:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, FLOOR + 2, 99, SMLSIZE | INVERS); // 99  TODO: blink telem off?
lcdDrawText(lcdNextPos + 1, FLOOR + 2, "RQ:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, FLOOR + 2, telemetryItems[2].value, SMLSIZE | INVERS); // 100

}

void hello_stop()
{
  popMenu();
};
void hello_run(event_t event)
{
  if (event == EVT_KEY_FIRST(KEY_UP))
  {
    AUDIO_TRIM_MIDDLE();
  }
  if (event == EVT_KEY_FIRST(KEY_DOWN))
  {
    AUDIO_TRIM_MIDDLE();
  }
  if (event == EVT_KEY_LONG(KEY_EXIT))
  {
    hello_stop();
  }
  lcdClear();
  hello_draw();
};

  
  
