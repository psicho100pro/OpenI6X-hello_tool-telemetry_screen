#include "opentx.h"

/*
the layout of everything is according to the TINSIZE
!!! TINSIZE writes only capital letters and nums !!! the raster is too coarse
TINSIZE font SQT5 does not write a colon
due to the lack of space, the effort is to simplify everything
*/

#define ROOF 7                                                           // y
#define FLOOR(LCD_H - ROOF - 1)                                        // y currently 56 , adjustable, it needs to be an even number cus center
#define HCENTER(ROOF + ((FLOOR - 1 - ROOF) / 2))                   // y currently 31
#define HORLLIM 0                                                        // x
#define HORDLIM 97                                                       // x divid, adjustable, it needs to be an odd number cus center
#define HORRLIM(LCD_W - 1)
#define HORWCENTER((HORDLIM - 1) / 2) // x currently 48
uint8_t ahcx = HORWCENTER;                                           // TODO: same , so then delete
uint8_t ahcy = HCENTER;
/* icons replaced by letters => memspace
void sIcon(coord_t x, coord_t y, LcdFlags att = BLINK) // sat   //TODO: maybe a scale
{
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  lcdDrawLine(x + 1, y + 1, x + 4, y + 4, SOLID, att);
  lcdDrawPoint(x + 4, y + 1, att);
  lcdDrawLine(x + 1, y + 2, x + 3, y + 4, SOLID, att);
  lcdDrawPoint(x + 1, y + 4, att);
  lcdDrawLine(x, y + 5, x + 2, y + 5, SOLID, att);
};
void hIcon(coord_t x, coord_t y, LcdFlags att = BLINK) // home
{
  lcdDrawPoint(x + 2, y, att);
  lcdDrawSolidFilledRect(x + 1, y + 1, 3, 4, att);
  lcdDrawPoint(x, y + 2, att);
  lcdDrawPoint(x + 4, y + 2, att);
};
void altIcon(coord_t x, coord_t y, LcdFlags att = 0) // alti
{
  lcdDrawLine(x + 1, y, x + 1, y + 5, SOLID, att);
  lcdDrawLine(x, y + 1, x + 2, y + 1, SOLID, FORCE | att);
  lcdDrawLine(x, y + 4, x + 2, y + 4, SOLID, FORCE | att);
};
void disIcon(coord_t x, coord_t y, LcdFlags att = 0) // dist
{
  lcdDrawLine(x, y + 1, x + 5, y + 1, SOLID, att);
  lcdDrawLine(x + 1, y, x + 1, y + 2, SOLID, FORCE | att);
  lcdDrawLine(x + 4, y, x + 4, y + 2, SOLID, FORCE | att);
};*/
float fSinCos(float rad, bool f = 0) // 0=sin 1=cos // it is not very accurate, but the display is not very fine either
{
static const float lut[] = { // sin table
0.00, 0.02, 0.03, 0.05, 0.07, 0.09, 0.10, 0.12, 0.14, 0.16,
0.17, 0.19, 0.21, 0.22, 0.24, 0.26, 0.28, 0.29, 0.31, 0.33,
0.34, 0.36, 0.37, 0.39, 0.41, 0.42, 0.44, 0.45, 0.47, 0.48,
0.50, 0.52, 0.53, 0.54, 0.56, 0.57, 0.59, 0.60, 0.62, 0.63,
0.64, 0.66, 0.67, 0.68, 0.69, 0.71, 0.72, 0.73, 0.74, 0.75,
0.77, 0.78, 0.79, 0.80, 0.81, 0.82, 0.83, 0.84, 0.85, 0.86,
0.87, 0.87, 0.88, 0.89, 0.90, 0.91, 0.91, 0.92, 0.93, 0.93,
0.94, 0.95, 0.95, 0.96, 0.96, 0.97, 0.97, 0.97, 0.98, 0.98,
0.98, 0.99, 0.99, 0.99, 0.99, 1.00, 1.00, 1.00, 1.00, 1.00
};

/*
something is wrong, the table in the radio returns only 0
return lut[5] ; // = 0
here it works https://cpp.sh/
*/

  // the telemetry comes back in rads: 0.25 - rad 0°=0 ; 90°=1.57 ; 180°=3.14 ; 270°=4.71 ; 360°=6.28
int16_t ang = (f ? rad + 1.57 : rad) *180/3.14159 ;
  uint16_t ab = abs(ang);
  float ret;
  if (ab < 90) { ret = lut[ab % 90]; }
  else if (ab < 180) { ret = lut[90 - ab % 90]; }
  else if (ab < 270) { ret = -lut[ab % 90]; }
  else { ret = -lut[90 - ab % 90];
  }
  return (rad >= 0) ? ret : -ret;
};

/*
uint8_t *GetDisplayBytes(char c)
{
  if (isdigit(c))
    return &displayBytes[5*(c - '0')];
  else if (isupper(c))
    return &displayBytes[5*(c - 'A')];
  else
    return NULL;
}
*/

/*float fSin(int16_t ang, float rad, bool f=0) // Taylor series
{
  float rad = 180/3.14159*(f? rad + 1.57:rad);
  int16_t pow7 = (rad * rad * rad * rad * rad * rad * rad);
  int16_t pow5 = (rad * rad * rad * rad * rad);
  int16_t pow3 = (rad * rad * rad);
  return (rad - (pow3 / 6) + (pow5 / 120) - (pow7 / 5040));
};
// fTan => memspace use just fSinCos(ang)/fSinCos(ang,1)*/

uint8_t hYlimit(uint8_t val) // this limit ,,,,, I'll fix it later
{ // y over limit
  if (ROOF <= val)
    return = ROOF + 1;
  if (val >= FLOOR)
    return = FLOOR - 1;
  return val;
};
float hello_draw()
{
  // ROOF bar:
  lcdDrawSolidFilledRect(0, 0, HORRLIM, ROOF - 1); //wrong to fix
  /* tx batt shape  => memspace
    lcdDrawLine(1, 1, 12, 1);
    lcdDrawLine(13, 2, 13, 4);
    lcdDrawLine(x, 2, x, 4); // x -> ?? g_vbat100mV - g_eeGeneral.vBatMax - g_eeGeneral.vBatMin - g_eeGeneral.vBatWarn
    lcdDrawPoint(x, 3);      // x -> g_eeGeneral.vBatWarn
    lcdDrawLine(1, 5, 1, 12);
  */
  putsVolts(15 , 1, g_vbat100mV, TINSIZE | (IS_TXBATT_WARNING() ? BLINK | INVERS : 0)); // tx batt  4 . 5V
  lcdDrawText(31 , 1, "TW:", TINSIZE | INVERS);
  lcdDrawNumber(41 , 1, telemetryItems[/*TPWR*/].value, TINSIZE | INVERS); // 100
  drawFlightMode(53 /*TODO:????*/, 1, mixerCurrentFlightMode, TINSIZE | INVERS);         // FM0-x
  lcdDrawText(105 , 1, "T:", TINSIZE | INVERS);
  if (g_model.timers[0].mode) // for timer number one, no switching yet  => memspace
  {
    drawTimer(LCD_W - 1, 1, timersStates[0].val, TINSIZE | INVERS | RIGHT | (timersStates[0].val < 0 ? BLINK : 0)); // -00:00 este pozjistovat trochu
  };

  // margin line
  // L ?
  lcdDrawLine(HORDLIM, ROOF, HORDLIM, FLOOR); // divid
  // P ?

  // a.horizon:
  // TODO: the known phenomenon when turning upside down, the orientation is reversed within 180 degrees , it needs to be treated somehow  -one g sensor? + over 45°?

  // pitch
  ahcy = hYlimit(/*FOV*/ 42 * -fSinCos(telemetryItems[/*pitch*/].value) + ahcy); // FOV fe. 45°=30, 40°=32, 35°=36 ,30°=42, 25°=60 - when the horizon touches ROOF or FLOOR*/
  // roll
  float sin = fSinCos(telemetryItems[/*roll*/].value);
  float cos = fSinCos(telemetryItems[/*roll*/].value, 1);
  float tan = sin / cos;
  // just horizon line // => memspace
  lcdDrawLine(HORLLIM, ahcy + tan * HORLLIM + 1, HORDLIM - 1, ahcy - tan * HORDLIM - 1); // currently just -45° to + 45° limits are not completed
  
  // ground fill // works // not yet => memspace
  for (uint8_t x = HORLLIM + 1; x < ahcx - 1; x++)
  {
    lcdDrawLine(ahcx - x, hYlimit(ahcy + tan * x), ahcx - x, FLOOR - 1); // L
    lcdDrawLine(ahcx + x, hYlimit(ahcy - tan * x), ahcx + x, FLOOR-1); // R
  };
  lcdDrawLine(ahcx, ahcy, ahcx, FLOOR - 1); // mid

  // auxiliary scales of °pitch not yet => memspace
  //30 10 25 13 20 18 15 20 10 24 x = (x - ahcx) * cos + (y - ahcy) * sin + ahcx;
  //y = (x - ahcx) * -sin + (y - ahcy) * cos + ahcy;

  // crosshair :D
  lcdDrawLine(HORWCENTER, HCENTER - 1, HORWCENTER, HCENTER + 1);
  lcdDrawLine(HORWCENTER - 1, HCENTER, HORWCENTER + 1, HCENTER);
  // lcdDrawLine(HORWCENTER + 6, HCENTER, HORWCENTER + 6, HCENTER + 3);  // => memspace
  // lcdDrawLine(HORWCENTER - 6, HCENTER, HORWCENTER + 6, HCENTER + 3);  // => memspace
  lcdDrawLine(HORWCENTER + 7, HCENTER, HORWCENTER + 15, HCENTER);
  lcdDrawLine(HORWCENTER - 15, HCENTER, HORWCENTER - 7, HCENTER);
  // yaw
  lcdDrawNumber(HORWCENTER - 1 /*TODO: align*/, FLOOR - 5, telemetryItems[/*yaw*/].value, TINSIZE | INVERS | CENTERED);
  
  /* compass arow 2line angel 30° rotate //have not started yet => memspace
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  //lower scale ||| W  |||  S  |||
  loop wide/10?
  lcdDrawLine(x + 1, y, x + 5, y + 4, SOLID, att);
  */
  
  // side indicators
  // L // ticks not yet => memspace
  lcdDrawSolidRect(HORLLIM, HCENTER - 4, HORLLIM + 14 /*TODO: width*/, HCENTER + 4, ERASE); //wrong to fix
  lcdDrawNumber(HORLLIM + 2, HCENTER - 2, telemetryItems[/*speed*/].value, TINSIZE);
  // R // ticks not yet => memspace
  lcdDrawSolidRect(HORDLIM - 14 /*TODO: width*/, HCENTER - 4, HORDLIM, HCENTER + 4, ERASE); //wrong to fix
  lcdDrawNumber(HORDLIM, HCENTER - 2, telemetryItems[/*alti*/].value, TINSIZE | RIGHT);

  // rigth block: // not completed
  lcdDrawText(HORRLIM - 20, ROOF + 2, 'S', TINSIZE);                                  // blink?
  lcdDrawNumber(HORRLIM, ROOF + 2, telemetryItems[/*GPS*/].value, TINSIZE | RIGHT);   // count of sats
  lcdDrawNumber(HORRLIM, ROOF + 10, telemetryItems[/*amp*/].value, TINSIZE | RIGHT);  //  drawValueWithUnit ??
  lcdDrawNumber(HORRLIM, ROOF + 15, telemetryItems[/*amph*/].value, TINSIZE | RIGHT); //  drawValueWithUnit ??
  // lcdDrawGauge(x,y,val);  //batt cap gauge telemetryItems[i].value not yet // +  ticks not yet => memspace

  // FLOOR bar:
  lcdDrawSolidFilledRect(0, FLOOR + 1, LCD_H - 1, HORRLIM - 1); //wrong to fix
  /* rx batt shape  => memspace
  lcdDrawLine(1, FLOOR + 2, 14, FLOOR + 2);
  lcdDrawLine(x, FLOOR + 3, x, FLOOR + 5);  // x ->  telemetryItems[i].value
  lcdDrawPoint(x, FLOOR + 4);               // x -> .vBatWarn
  lcdDrawLine(1, FLOOR + 6, 14, FLOOR + 6);
*/
  putsVolts(16, FLOOR + 2, telemetryItems[/*rx batt*/].value, TINSIZE | INVERS); // 12 . 6V  TODO: asi dořešit //  drawValueWithUnit ??
  lcdDrawText(lcdNextPos, FLOOR + 2, 'RS:', TINSIZE | INVERS);
  lcdDrawNumber(lcdNextPos, FLOOR + 2, telemetryItems[/*RSSI*/].value, TINSIZE | INVERS); // 99  TODO: blink telem off?
  lcdDrawText(lcdNextPos, FLOOR + 2, 'RQ:', TINSIZE | INVERS);
  lcdDrawNumber(lcdNextPos, FLOOR + 2, telemetryItems[/*RQLy*/].value, TINSIZE | INVERS); // 100

  drawValueWithUnit(HORRLIM, FLOOR + 2, telemetryItems[/*STAB*/].value, /*TODO: unit*/'', TINSIZE | INVERS | RIGHT); // STAB/HOR/ACRO

  // TODO: alert window > no telemetr, lowbat
  // TODO: menu?

  // drawGPSSensorValue
  //  GET_TXBATT_BARS()
  // editSlider  ????
};
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
