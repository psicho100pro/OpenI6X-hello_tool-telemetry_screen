#include "opentx.h"

//!!! SMLSIZE writes only capital letters and nums !!! the raster is too coarse
#define TOP FH
#define FOOT (LCD_H - 1) // y adjustable
#define HRLIM (LCD_W - 1) // x
#define AHORLLIM 0
#define AHORRLIM 94 // x splitsc/divid, adjustable
#define AHORTOPLIM (TOP + 1)
#define AHORFOOTLIM (FOOT - 1)
#define AHORYCENTER (TOP + ((FOOT - 1 - TOP) / 2))
#define AHORXCENTER (AHORRLIM / 2)

#define FOV 44    //fe. 50°=32, 45°=36, 40°=40, , 35°=44, , 30°=48, , 25°=52 - when the horizon touches TOP or FOOT

#define ARWCX 10
#define ARWCY 20

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
#define yaw  (telemetryItems[21].value)
#define STAB (telemetryItems[22].text)

static const uint8_t lut[] = { // sin table fixed SCALE 6
0,1,2,3,4,6,7,8,9,10,
11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,48,
49,50,50,51,52,52,53,54,54,55,55,56,57,57,58,58,58,59,59,60,
60,61,61,61,62,62,62,62,63,63,63,63,63,64,64,64,64,64,64,64
};

static int8_t fSinCos(int16_t deg, bool f = false) // 0=sin 1=cos
{
  if (deg == 0) return f ? lut[89] : 0;
  deg = f ? deg + 157 : deg;
  deg = deg * 180 / 314;
  if (deg >= 360) deg -= 360;
  uint16_t ab = abs(deg);

  if (ab == 180) return 0;
  if (deg == -270 || deg == 90) return lut[89];
  if (deg == -90 || deg == 270) return -lut[89];

  if (ab < 90) { return lut[ab % 90] * (deg < 0 ? -1 : 1);
  } else if (ab < 180) { return lut[90 - ab % 90] * (deg < 0 ? -1 : 1);
  } else if (ab < 270) { return -lut[ab % 90] * (deg < 0 ? -1 : 1);
  } else { return -lut[90 - ab % 90] * (deg < 0 ? -1 : 1); }
};

static bool upSdo; // upsidedown 1bool upSdo;
static bool UaDdraw;
static uint8_t hoYdrLim(int32_t Y, bool UaD) // hor Y draw limit
{
  UaDdraw = 1;
  if (TOP >= Y){
    UaDdraw = !UaD;
    return AHORTOPLIM;
  }
  if (Y >= FOOT){
    UaDdraw = UaD;
    return AHORFOOTLIM;
  }
  return Y;
};


static void lcdDrawAngLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t cx, uint8_t cy, int8_t sin, int8_t cos, uint8_t pat = SOLID, LcdFlags att = 0)
{
  x1 = ((((x1 - cx) << 6) * cos) >> 12) + ((((y1 - cy) << 6) * sin) >> 12) + cx;
  y1 = ((((x1 - cx) << 6) * -sin) >> 12) + ((((y1 - cy) << 6) * cos) >> 12) + cy;
  x2 = ((((x2 - cx) << 6) * cos) >> 12) + ((((y2 - cy) << 6) * sin) >> 12) + cx;
  y2 = ((((x2 - cx) << 6) * -sin) >> 12) + ((((y2 - cy) << 6) * cos) >> 12) + cy;
  lcdDrawLine(x1, y1, x2, y2, pat, att);
};

void hello_draw() {

// TOP bar:
  lcdDrawFilledRect(0, 0, LCD_W, TOP, SOLID, 0);
  putsVolts(1, 1, g_vbat100mV, SMLSIZE | INVERS | (IS_TXBATT_WARNING() ? BLINK : 0));
  lcdDrawText(lcdNextPos + 1, 1, "TW:", SMLSIZE | INVERS);
  lcdDrawNumber(lcdNextPos, 1, TPWR, SMLSIZE | INVERS);
  putsModelName(lcdNextPos + 2, 1, g_model.header.name, g_eeGeneral.currModel, SMLSIZE | INVERS);
  if (g_model.timers[0].mode){
    lcdDrawText(AHORRLIM - FW - 3, 1, "T1", SMLSIZE | INVERS);
    drawTimer(lcdNextPos + FW, 1, timersStates[0].val, SMLSIZE | INVERS | (timersStates[0].val < 0 ? BLINK : 0));
  };
  if (g_model.timers[1].mode){
    lcdDrawText(AHORRLIM - FW - 3, 1, "T2:", SMLSIZE | INVERS);
    drawTimer(lcdNextPos + FW, 1, timersStates[1].val, SMLSIZE | INVERS | (timersStates[1].val < 0 ? BLINK : 0));
  };
  // margin line:
  lcdDrawLine(AHORRLIM + 1, TOP, AHORRLIM + 1, FOOT); // divid
  // vario ?
  lcdDrawLine(AHORRLIM + 4, TOP, AHORRLIM + 4, FOOT); // divid

// a.horizon:
  //top
    drawFlightMode(AHORXCENTER - 2, TOP + 2, mixerCurrentFlightMode, SMLSIZE | RIGHT);
  lcdDrawText(AHORXCENTER + 2, TOP + 2, STAB, SMLSIZE);
  // drawValueWithUnit(AHORXCENTER + 2, TOP + 2, STAB, UNIT_TEXT, SMLSIZE);
  lcdDrawChar(AHORRLIM - 3 * FWNUM, TOP + 2, '\xce', SMLSIZE);
  // lcdDrawText(AHORRLIM - 3 * FWNUM, TOP + 2, "S:", SMLSIZE);
  lcdDrawNumber(lcdNextPos, TOP + 2, Sats, SMLSIZE | (Sats < 5 ? BLINK : 0));
  
  
  // pitch
  // upSdo = abs(pitch) > 79; TODO:  notyet,, the orientation is reversed within 180 + giro ?
  int16_t ahCy = AHORYCENTER;
  ahCy += ((FOV << 6) * -fSinCos(pitch)) >> 12;
  // roll
  upSdo = abs(roll) > 157;
  int8_t ahSin = fSinCos(roll), ahCos = fSinCos(roll, true);
  int16_t ahTan = (ahSin << 6) / (ahCos == 0 ? 1 : ahCos);

// ground fill
  uint8_t ahDy, ahUy;
  for (uint8_t x = AHORLLIM + 1; x < AHORXCENTER; x++)
  {
    ahDy = hoYdrLim(ahCy + ((int32_t)(ahTan * (x << 6)) >> 12), upSdo);
    if (UaDdraw) lcdDrawLine(AHORXCENTER - x, ahDy, AHORXCENTER - x, (upSdo ? AHORTOPLIM : AHORFOOTLIM));
    ahUy = hoYdrLim(ahCy - ((int32_t)(ahTan * (x << 6)) >> 12), upSdo);
    if (UaDdraw) lcdDrawLine(AHORXCENTER + x, ahUy, AHORXCENTER + x, (upSdo ? AHORTOPLIM : AHORFOOTLIM));
  };
  ahCy = hoYdrLim(ahCy,upSdo);
  if (UaDdraw) lcdDrawLine(AHORXCENTER, ahCy, AHORXCENTER, (upSdo ? AHORTOPLIM : AHORFOOTLIM));
 
  // pitch scale line
  for (uint8_t l = 1; l < 4; l++)
  {
    ahUy = hoYdrLim(ahCy - uint8_t(FOV / 4 * l), upSdo);
    if (UaDdraw) lcdDrawAngLine(AHORXCENTER - ((l == 2) ? 11 : 3), ahUy, AHORXCENTER + ((l == 2) ? 11 : 3), ahUy, AHORXCENTER, ahCy, ahSin, ahCos, DOTTED, 0);
    ahDy = hoYdrLim(ahCy + uint8_t(FOV / 4 * l), upSdo);
    if (UaDdraw) lcdDrawAngLine(AHORXCENTER - ((l == 2) ? 11 : 3), ahDy, AHORXCENTER + ((l == 2) ? 11 : 3), ahDy, AHORXCENTER, ahCy, ahSin, ahCos, DOTTED, 0);
  };
  
// yaw arrow
lcdDrawAngLine(ARWCX, ARWCY - FW, ARWCX - FW, ARWCY + FW, ARWCX, ARWCY, fSinCos(yaw), fSinCos(yaw, 1), SOLID);
lcdDrawAngLine(ARWCX, ARWCY - FW, ARWCX + FW, ARWCY + FW, ARWCX, ARWCY, fSinCos(yaw), fSinCos(yaw, 1), SOLID);

// crosshair :D
  lcdDrawLine(AHORXCENTER, AHORYCENTER - 1, AHORXCENTER, AHORYCENTER + 1);
  lcdDrawLine(AHORXCENTER - 1, AHORYCENTER, AHORXCENTER + 1, AHORYCENTER);
  lcdDrawLine(AHORXCENTER - 6, AHORYCENTER, AHORXCENTER - 6, AHORYCENTER + 3);
  lcdDrawLine(AHORXCENTER + 6, AHORYCENTER, AHORXCENTER + 6, AHORYCENTER + 3);
  lcdDrawLine(AHORXCENTER + 7, AHORYCENTER, AHORXCENTER + 15, AHORYCENTER);
  lcdDrawLine(AHORXCENTER - 15, AHORYCENTER, AHORXCENTER - 7, AHORYCENTER);

// side indicators
  lcdDrawFilledRect(AHORLLIM, AHORYCENTER - 5, 3 * FWNUM + 3, FH + 2, SOLID, FORCE);
  lcdDrawFilledRect(AHORLLIM + 1, AHORYCENTER - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
  lcdDrawNumber(AHORLLIM + 2, AHORYCENTER - 3, Spd, SMLSIZE);
  lcdDrawFilledRect(AHORRLIM - 3 * FWNUM - 1, AHORYCENTER - 5, 3 * FWNUM + 3, FH + 2, SOLID, FORCE);
  lcdDrawFilledRect(AHORRLIM - 3 * FWNUM, AHORYCENTER - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
  lcdDrawNumber(AHORRLIM - 3 * FWNUM + 1, AHORYCENTER - 3, Alt, SMLSIZE);

// botoom
  /*
  //lower scales ||| W  |||  S  |||  // 0/360 N  45 NE  90 E  135 SE  180 S  225 SW  270 W  315 NW
  loop wide/10?
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  */                  
                 
 lcdDrawFilledRect(AHORRLIM + FW, AHORTOPLIM, 26, FW + 1, SOLID, 0); // B%
  putsVolts(AHORRLIM + FW, AHORTOPLIM + FH - 1, RxBt, MIDSIZE); //if 12v SMLSIZE
  lcdDrawNumber(AHORRLIM + FW, AHORTOPLIM + 2 * FH + 4, Curr, SMLSIZE);
  lcdDrawText(lcdNextPos, AHORTOPLIM + 2 * FH + 4, "A", SMLSIZE);
  lcdDrawNumber(AHORRLIM + FW, AHORTOPLIM + 3 * FH + 3, Cons, SMLSIZE);
  lcdDrawText(AHORRLIM + 3 * FW, AHORTOPLIM + 4 * FH + 1, "MAH", TINSIZE);
  lcdDrawText(AHORRLIM + FW + 1, FOOT - 2 * FH + 2, "R:", TINSIZE);
  lcdDrawFilledRect(AHORRLIM + FW, FOOT - 2 * FH + 1, 26, FW + 1, SOLID, 0);
  lcdDrawText(AHORRLIM + FW + 1, FOOT - FH + 2, "Q:", TINSIZE);
  lcdDrawFilledRect(AHORRLIM + FW, FOOT - FH + 1, 26, FW + 1, SOLID, 0);
  
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
