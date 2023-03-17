#include "opentx.h"

/*
!!! SMLSIZE writes only capital letters and nums !!! the raster is too coarse
due to the lack of space, the effort is to simplify everything
*/

#define TOP FH
#define FOOT (LCD_H - 1) // y adjustable
#define HRLIM (LCD_W - 1) // x
#define HORLLIM 0
#define HORRLIM 94 // x splitsc/divid, adjustable
#define HORTOPLIM (TOP + 1)
#define HORFOOTLIM (FOOT - 1)
#define HORYCENTER (TOP + ((FOOT - 1 - TOP) / 2))
#define HORXCENTER (HORRLIM / 2)

#define FOV 44    //fe. 50°=32, 45°=36, 40°=40,	, 35°=44,	, 30°=48,	, 25°=52 - when the horizon touches TOP or FOOT

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
#define yaw  (telemetryItems[21].value) // budu potřebovat až na polohu
#define STAB (telemetryItems[22].text)

static const uint8_t sLut[] = { // sin table fixed SCALE 6
0,1,2,3,4,6,7,8,9,10,
11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,48,
49,50,50,51,52,52,53,54,54,55,55,56,57,57,58,58,58,59,59,60,
60,61,61,61,62,62,62,62,63,63,63,63,63,64,64,64,64,64,64,64
};
static int8_t fSinCos(int16_t angle, bool f = false, bool polar = 0) // 0=sin 1=cos
{
  if (angle == 0) return f ? sLut[89] : 0;
  polar = (angle < 0);
  angle = f ? angle += (uint8_t)157 : angle;
  angle = angle * (uint8_t)180 / (uint8_t)314;
  if (angle >= 360) angle -= (uint8_t)360;
  uint16_t ab = abs(angle);
  if (ab == 180) return 0;
  if (angle == -270 || angle == 90) return sLut[89];
  if (angle == -90 || angle == 270) return -sLut[89];
  if (ab < 90) { return sLut[ab % 90] * (polar ? -1 : 1);
  } else if (ab < 180) { return sLut[90 - ab % 90] * (polar ? -1 : 1);
  } else if (ab < 270) { return -sLut[ab % 90] * (polar ? -1 : 1);
  } else { return -sLut[90 - ab % 90] * (polar ? -1 : 1); }
};

static const int16_t sLut[64] = {
0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,98,104,109,
115,121,126,132,137,142,147,152,157,162,167,172,177,181,
185,190,194,198,202,206,209,213,216,220,223,226,229,231,
234,237,239,241,243,245,247,248,250,251,252,253,254,255,255,255,255
};

int16_t sin(int16_t angle,bool polar = 0)
{
    int i = angle & (256 - 1);
    if (i >= 128){
        i -= 128;
        polar = 1;
    };
    if (i >= 64){
        i = 128 - 1 - i;
    };
    return polar ? -sLut[i] : sLut[i];
};

int16_t cos(int16_t angle)
{
    return sin(angle + 64);
};

bool upSdo;
bool UaDdraw;
static uint8_t hoYdrLim(int16_t Y, bool UaD) // hor Y draw limit
{
  UaDdraw = 1;
  if (TOP >= Y){
    UaDdraw = !UaD;
    return HORTOPLIM;
  }
  if (Y >= FOOT){
    UaDdraw = UaD;
    return HORFOOTLIM;
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
// pitch // TODO: the known phenomenon when turning upside down, the orientation is reversed within 180 anglerees roll and 90 pitch
  int16_t ahCy = HORYCENTER;
  
  ahCy += (uint16_t(FOV << 6) * -fSinCos(pitch)) >> 12;
  // top
  drawFlightMode(HORXCENTER - 2, TOP + 2, mixerCurrentFlightMode, SMLSIZE | RIGHT);
  lcdDrawText(HORXCENTER + 2, TOP + 2, STAB, SMLSIZE);
  // drawValueWithUnit(HORXCENTER + 2, TOP + 2, STAB, UNIT_TEXT, SMLSIZE);
  lcdDrawChar(HORRLIM - 3 * FWNUM, TOP + 2, '\xce', SMLSIZE);
  // lcdDrawText(HORRLIM - 3 * FWNUM, TOP + 2, "S:", SMLSIZE);
  lcdDrawNumber(lcdNextPos, TOP + 2, Sats, SMLSIZE | (Sats < 5 ? BLINK : 0));

// roll
  upSdo = (roll < -157 || roll > 157);
  int8_t ahSin = fSinCos(roll);
  int8_t ahCos = fSinCos(roll, true);
  int16_t ahTan = ((ahSin * (uint8_t)64 / (ahCos == 0 ? (uint8_t)1 : ahCos));

  // ground fill
  uint8_t ahLy;
  uint8_t ahRy;
  for (uint8_t x = HORLLIM + 1; x < HORXCENTER; x++)
  {
    ahLy = hoYdrLim(ahCy + ((ahTan * uint16_t(x << 6)) >> 12), upSdo);
    if (UaDdraw) lcdDrawLine(HORXCENTER - x, ahLy, HORXCENTER - x, (upSdo ? HORTOPLIM : HORFOOTLIM));
    ahRy = hoYdrLim(ahCy - ((ahTan * uint16_t(x << 6)) >> 12), upSdo);
    if (UaDdraw) lcdDrawLine(HORXCENTER + x, ahRy, HORXCENTER + x, (upSdo ? HORTOPLIM : HORFOOTLIM));
  };
  ahCy = hoYdrLim(ahCy);
  if (UaDdraw) lcdDrawLine(HORXCENTER, ahCy, HORXCENTER, (upSdo ? HORTOPLIM : HORFOOTLIM));

  // pitch scale line
  for (uint8_t l = 1; l < 4; l++)
  {
    lcdDrawAngLine(HORXCENTER - ((l == 2) ? 11 : 3), ahCy - uint8_t(FOV / 4 * l), HORXCENTER + ((l == 2) ? 11 : 3), ahCy - uint8_t(FOV / 4 * l), HORXCENTER, ahCy, ahSin, ahCos, DOTTED, 0);
    lcdDrawAngLine(HORXCENTER - ((l == 2) ? 11 : 3), ahCy + uint8_t(FOV / 4 * l), HORXCENTER + ((l == 2) ? 11 : 3), ahCy + uint8_t(FOV / 4 * l), HORXCENTER, ahCy, ahSin, ahCos, DOTTED, 0);
  };

  // yaw arrow
  lcdDrawAngLine(ARWCX, ARWCY - FW, ARWCX - FW, ARWCY + FW, ARWCX, ARWCY, fSinCos(yaw), fSinCos(yaw, 1));
  lcdDrawAngLine(ARWCX, ARWCY - FW, ARWCX + FW, ARWCY + FW, ARWCX, ARWCY, fSinCos(yaw), fSinCos(yaw, 1), SOLID, FORCE);
  
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

// botoom
  /*
  //lower scales ||| W  |||  S  |||  // 0/360 N  45 NE  90 E  135 SE  180 S  225 SW  270 W  315 NW
  loop wide/10?
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  */                  
                 
// rigth block:
  lcdDrawFilledRect(HORRLIM + FW, HORTOPLIM, 26, FW + 1, DOTTED, 0); // B%
  putsVolts(HORRLIM + FW, HORTOPLIM + FH - 1, RxBt, MIDSIZE); //if 12v SMLSIZE
  lcdDrawNumber(HORRLIM + FW, HORTOPLIM + 2 * FH + 4, Curr, SMLSIZE);
  lcdDrawText(lcdNextPos, HORTOPLIM + 2 * FH + 4, "A", SMLSIZE);
  lcdDrawNumber(HORRLIM + FW, HORTOPLIM + 3 * FH + 3, Cons, SMLSIZE);
  lcdDrawText(HORRLIM + 3 * FW, HORTOPLIM + 4 * FH + 1, "MAH", TINSIZE);
  lcdDrawText(HORRLIM + FW + 1, FOOT - 2 * FH + 2, "R:", TINSIZE);
  lcdDrawFilledRect(HORRLIM + FW, FOOT - 2 * FH + 1, 26, FW + 1, SOLID, (RSSI < g_model.rssiAlarms.getWarningRssi()) ? BLINK : 0);
  lcdDrawText(HORRLIM + FW + 1, FOOT - FH + 2, "Q:", TINSIZE);
  lcdDrawFilledRect(HORRLIM + FW, FOOT - FH + 1, 26, FW + 1, SOLID, (RQly < 50 /**/) ? BLINK : 0);
  
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
