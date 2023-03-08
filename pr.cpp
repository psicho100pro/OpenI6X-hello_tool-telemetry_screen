#include "opentx.h"

/*
!!! SMLSIZE writes only capital letters and nums !!! the raster is too coarse
due to the lack of space, the effort is to simplify everything
*/

#define TOP FH
#define FOOT (LCD_H - 1) // y adjustable
#define HRLIM (LCD_W - 1) 
#define HORLLIM 0
#define HORDLIM 94 // x divid, adjustable
#define HORTOPL (TOP + 1)
#define HORFOOTL (FOOT - 1)
#define HORHCENTER (TOP + ((FOOT - 1 - TOP) / 2))
#define HORWCENTER (HORDLIM / 2)

#define FOV 44    //fe. 50°=32, 45°=36, 40°=40,	, 35°=44,	, 30°=48,	, 25°=52 - when the horizon touches TOP or FOOT

#define RSSI  telemetryData.rssi.value
#define RQly (telemetryItems[2].value)
#define TPWR (telemetryItems[6].value)
//#define GPS  (telemetryItems[10].value) //coord
#define Spd  (telemetryItems[11].value)
//#define Hdg  (telemetryItems[12].value)
#define Alt  (telemetryItems[13].value)
#define Sats (telemetryItems[14].value)
#define RxBt (telemetryItems[15].value)
#define Curr (telemetryItems[16].value)
#define Cons (telemetryItems[17].value) // consum
#define Batp (telemetryItems[18].value) // Bat%
#define pitch (telemetryItems[19].value)
#define roll (telemetryItems[20].value)
//#define yaw  (telemetryItems[21].value)
#define STAB (telemetryItems[22].value)

static const uint16_t lut[] = { // sin table fixed SCALE 6
0,1,2,3,4,6,7,8,9,10,
11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,48,
49,50,50,51,52,52,53,54,54,55,55,56,57,57,58,58,58,59,59,60,
60,61,61,61,62,62,62,62,63,63,63,63,63,64,64,64,64,64,64,64
};
bool tgEfl; // tangens error flag
bool upSdo; // upsidedown 1
int8_t fSinCos(int16_t deg, bool f = false) // 0=sin 1=cos
{
  if (deg == 0) return f ? lut[89] : 0;
  deg = f ? deg + 157 : deg;
  deg = deg * 180 / 314;
  if (deg >= 360) deg -= 360;
  uint16_t ab = abs(deg);
  if (!f) tgEfl = (ab == 90 || ab == 270) ? true : 0;

  if (ab == 180) return 0;
  if (deg == -270 || (f ? deg == 90 : ab == 90)) return lut[89];
  if (deg == -90 || (f ? deg == 270 : ab == 270)) return -lut[89];

  if (ab < 90) { return lut[ab % 90] * (deg < 0 ? -1 : 1);
  } else if (ab < 180) { return lut[90 - ab % 90] * (deg < 0 ? -1 : 1);
  } else if (ab < 270) { return -lut[ab % 90] * (deg < 0 ? -1 : 1);
  } else { return -lut[90 - ab % 90] * (deg < 0 ? -1 : 1); }
};


uint8_t hYlimit(uint16_t val) // y over limit
{
  if (ROOF >= val) return ROOF + 1;
  if (val >= FLOOR) return FLOOR - 1;
  return val;
};

void lcdDrawAngLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t ahcy, int8_t sin, int8_t cos)
{ //coord_t ahcx = HORWCENTER
  x1 = ((((x1 - HORWCENTER) << 6) * cos) >> 12) + ((((y1 - ahcy) << 6) * sin) >> 12) + HORWCENTER;
  y1 = ((((x1 - HORWCENTER) << 6) * -sin) >> 12) + ((((y1 - ahcy) << 6) * cos) >> 12) + ahcy;
  x2 = ((((x2 - HORWCENTER) << 6) * cos) >> 12) + ((((y2 - ahcy) << 6) * sin) >> 12) + HORWCENTER;
  y2 = ((((x2 - HORWCENTER) << 6) * -sin) >> 12) + ((((y2 - ahcy) << 6) * cos) >> 12) + ahcy;
  lcdDrawLine(x1, y1, x2, y2, DOTTED);
};

void hello_draw() {

// TOP bar:
lcdDrawFilledRect(0, 0, HRLIM, TOP, SOLID);
putsVolts(1, 1, g_vbat100mV, SMLSIZE | INVERS | (IS_TXBATT_WARNING() ? BLINK : 0));
lcdDrawText(lcdNextPos + 1, 1, "TW:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, 1, TPWR, SMLSIZE | INVERS);
putsModelName(lcdNextPos + 2, 1, g_model.header.name, g_eeGeneral.currModel, SMLSIZE | INVERS);
if (g_model.timers[0].mode) {
    lcdDrawText(HORDLIM - FW - 4, 1, "T1:", SMLSIZE | INVERS);
    drawTimer(lcdNextPos + FW, 1, timersStates[0].val, SMLSIZE | INVERS | (timersStates[0].val < 0 ? BLINK : 0));
  };
if (g_model.timers[1].mode) {
    lcdDrawText(HORDLIM - FW - 4, 1, "T2:", SMLSIZE | INVERS);
    drawTimer(lcdNextPos + FW, 1, timersStates[1].val, SMLSIZE | INVERS | (timersStates[1].val < 0 ? BLINK : 0));
  };
// margin line:
lcdDrawLine(HORDLIM + 1, TOP, HORDLIM + 1, FOOT); // divid
// vario ?
lcdDrawLine(HORDLIM + 4, TOP, HORDLIM + 4, FOOT); // divid

// a.horizon:
// pitch
int16_t ahcy = HORHCENTER;
ahcy += (uint16_t(FOV << 6) * -fSinCos(pitch)) >> 12;
  
//  TODO: the known phenomenon when turning upside down, the orientation is reversed within 180 degrees roll and 90 pitch

//top
drawFlightMode(HORWCENTER - 2, TOP + 2, mixerCurrentFlightMode, SMLSIZE | RIGHT);
drawValueWithUnit(HORWCENTER + 2, TOP + 2, STAB, UNIT_TEXT , SMLSIZE);
lcdDrawText(HORDLIM - 3 * FWNUM, TOP + 2, "S:", SMLSIZE);
lcdDrawNumber(lcdNextPos, TOP + 2, Sats, SMLSIZE | (Sats < 5 ? BLINK : 0));

// roll
int8_t sin = fSinCos(roll);
int8_t cos = fSinCos(roll, true);
int16_t tan = (sin * (1 << 6) / (tgEfl ? 1 : cos));

// ground fill
for (uint8_t x = HORLLIM + 1; x < HORWCENTER; x++)
{
  lcdDrawLine(HORWCENTER - x, hYlimit(ahcy + ((tan * uint16_t(x << 6)) >> 12)), HORWCENTER - x, FLOOR - 1); // L
  lcdDrawLine(HORWCENTER + x, hYlimit(ahcy - ((tan * uint16_t(x << 6)) >> 12)), HORWCENTER + x, FLOOR - 1); // R
};
lcdDrawLine(HORWCENTER, ahcy, HORWCENTER, FLOOR - 1); // mid

// pitch scale line
for (uint8_t l = 1; l < 4; l++)
{
  lcdDrawAngLine(HORWCENTER - ((l == 2) ? 11 : 3), ahcy - uint8_t(FOV / 8 * l), HORWCENTER + ((l == 2) ? 11 : 3), ahcy - uint8_t(FOV / 8 * l), ahcy, sin, cos);
  lcdDrawAngLine(HORWCENTER - ((l == 2) ? 11 : 3), ahcy + uint8_t(FOV / 8 * l), HORWCENTER + ((l == 2) ? 11 : 3), ahcy + uint8_t(FOV / 8 * l), ahcy, sin, cos);
};
  
 // crosshair :D
lcdDrawLine(HORWCENTER, HORHCENTER - 1, HORWCENTER, HORHCENTER + 1);
lcdDrawLine(HORWCENTER - 1, HORHCENTER, HORWCENTER + 1, HORHCENTER);
lcdDrawLine(HORWCENTER - 6, HORHCENTER, HORWCENTER - 6, HORHCENTER + 3);
lcdDrawLine(HORWCENTER + 6, HORHCENTER, HORWCENTER + 6, HORHCENTER + 3);
lcdDrawLine(HORWCENTER + 7, HORHCENTER, HORWCENTER + 15, HORHCENTER);
lcdDrawLine(HORWCENTER - 15, HORHCENTER, HORWCENTER - 7, HORHCENTER);

// side indicators
lcdDrawFilledRect(HORLLIM, HORHCENTER - 5, 3 * FWNUM + 3, FH + 2, SOLID, FORCE);
lcdDrawFilledRect(HORLLIM + 1, HORHCENTER - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
lcdDrawNumber(HORLLIM + 2, HORHCENTER - 3, Spd, SMLSIZE);
lcdDrawFilledRect(HORDLIM - 3 * FWNUM - 1 , HORHCENTER - 5, 3 * FWNUM + 3, FH + 2 , SOLID, FORCE);
lcdDrawFilledRect(HORDLIM - 3 * FWNUM, HORHCENTER - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
lcdDrawNumber(HORDLIM - 3 * FWNUM + 1, HORHCENTER - 3, Alt, SMLSIZE);

// rigth block:
putsVolts(HORDLIM + FW, HORTOPL + FH, RxBt, SMLSIZE);
lcdDrawNumber(HORDLIM + FW, HORTOPL + 15, Curr, SMLSIZE);
lcdDrawText(lcdNextPos, HORTOPL + 15, "A", SMLSIZE);
lcdDrawNumber(HORDLIM + FW, HORTOPL + 22, Cons, SMLSIZE);
lcdDrawText(HORDLIM + 2 * FW , HORTOPL + 29, "mAh", SMLSIZE);
lcdDrawText(HORDLIM + FW, FOOT - 13, "S:", TINSIZE);
lcdDrawText(HORDLIM + FW, FOOT - 6, "Q:", TINSIZE);
  
void hello_stop()
{
  popMenu();
};
void hello_run(event_t event)
{
  if (event == EVT_KEY_FIRST(KEY_UP))
  {
    AUDIO_TRIM_MAX()
  }
  if (event == EVT_KEY_FIRST(KEY_DOWN))
  {
    AUDIO_TRIM_MIN();
  }
   if (event == EVT_KEY_FIRST(KEY_ENTER))
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
