#include "opentx.h"

#define RSSI telemetryData.rssi.value
#define RSS1 telemetryData.rssi.value //RX_RSSI1_INDEX
#define RSS2 telemetryData.rssi.value //RX_RSSI2_INDEX
#define RQly (telemetryItems[2].value)  //RX_QUALITY_INDEX
#define TPWR (telemetryItems[6].value)  //TX_POWER_INDEX
//#define GPS  (telemetryItems[10].value) //coord  telemetryItem.gps.longitude telemetryItem.gps.latitude
#define Sped (telemetryItems[11].value) //GPS_GROUND_SPEED_INDEX
#define Hedg (telemetryItems[12].value) //GPS_HEADING_INDEX
#define Alti (telemetryItems[13].value) //GPS_ALTITYE_INDEX
#define Sats (telemetryItems[14].value) //GPS_SATELLITES_INDEX
#define RxBt (telemetryItems[15].value) //BATT_VOLTAGE_INDEX
#define Curr (telemetryItems[16].value) //BATT_CURRENT_INDEX
#define Cons (telemetryItems[17].value) //BATT_CAPACITY_INDEX
#define Batp (telemetryItems[18].value) //BATT_REMAINING_INDEX
#define pith (telemetryItems[19].value) //ATTITYE_PITCH_INDEX
#define roll (telemetryItems[20].value) //ATTITYE_ROLL_INDEX
#define yawS (telemetryItems[21].value) //ATTITYE_YAW_INDEX
#define STAB (telemetryItems[22].text)  //FLIGHT_MODE_INDEX


#define Top FH
#define Foot (LCD_H - 1)
//#define HRLim (LCD_W - 1)
#define AHorLLim 0
#define AHorRLim 94 // disp split
#define AHorTopLim (Top + 1)
#define AHorFootLim (Foot - 1)
#define AHorYcent (Top + ((Foot - 1 - Top) / 2))
#define AHorXcent (AHorRLim / 2)
#define FOV 44  // When the horizon touches Top or Foot - fe. 50°=32, 45°=36, 40°=40,	, 35°=44,	, 30°=48,	, 25°=52

#define CH2Bit(n) channelOutputs[n] / 1024.0f * 128
#define Rad2Bit(r) r / 628.0f * 128
#define Rad2Deg(r) r * 180 / 314
#define Deg2Rad(d) d / 180 * 3.1415f
// compass
#define CMPnFielGAU (AHorRLim / 4)
#define CMPradNwrap(n) channelOutputs[n] / 1024.0f * CMPnFielGAU * 8
//#define CMPradNwrap (Hedg / 628.0f * CMPnFielGAU * 8)
#define CMPFielAlig  (AHorXcent  - AHorRLim / 8)

// yaw arrow
#define ArrwCx 10
#define ArrwCy 20

static bool upSdo, UaDdraw;
static uint8_t x, y;
static int16_t fSin, fCos;
static int16_t CY;
static int32_t YY;

static const uint8_t sLut[32]{
0,6,13,19,25,31,37,43,49,55,60,66,71,76,81,86,91,95,
99,103,106,110,113,116,118,121,122,124,126,127,127,127
};

static void fSinCos(int16_t angl) {
  upSdo = 0;
  x = angl & 127;
  if (x >= 64) {
    x -= 64;
    upSdo = 1;
  };
  if (x >= 32) x = 63 - x;
  fSin = upSdo ? -sLut[x] : sLut[x];
  upSdo = 0;
  x = (angl + 32) & 127;
  if (x >= 64) {
    x -= 64;
    upSdo = 1;
  };
  if (x >= 32) x = 63 - x;
  fCos = upSdo ? -sLut[x] : sLut[x];
}; // Linear Interpolation  yp = y0 + ((y1-y0)/(x1-x0)) * (xp - x0);

static void hoYdrLim(int16_t Y) { // hor Y draw limit
  if (Top >= Y) {
    UaDdraw = !upSdo;
    y = AHorTopLim;
  } else if (Y >= Foot) {
    UaDdraw = upSdo;
    y = AHorFootLim;
  } else {
    UaDdraw = 1;
    y = Y;
  }
};

static const char * timer[3][3] = {"T1:","T2:","T3:"};
static const char * compRose[8][2] = { "N\0", "NE", "E\0", "SE", "S\0", "SW", "W\0", "NW" };

static void hello_draw() {
// Top bar: ~280 Flash
  lcdDrawFilledRect(0, 0, LCD_W, Top, SOLID, 0);
  putsVolts(1, 1, g_vbat100mV, SMLSIZE | INVERS | (IS_TXBATT_WARNING() ? BLINK : 0));
  lcdDrawText(lcdNextPos + 1, 1, "TW:", SMLSIZE | INVERS);
  lcdDrawNumber(lcdNextPos, 1, TPWR, SMLSIZE | INVERS);
  putsModelName(lcdNextPos + 2, 1, g_model.header.name, g_eeGeneral.currModel, SMLSIZE | INVERS);
  for (x = 0; x < 3; x++) {
    if (g_model.timers[x].mode) {
      lcdDrawText(AHorRLim - FW - 3, 1, timer[0][x], SMLSIZE | INVERS);
      drawTimer(lcdNextPos + FW, 1, timersStates[x].val, SMLSIZE | INVERS | (timersStates[x].val < 0 ? BLINK : 0));
    };
  };

  // margin line:
  lcdDrawLine(AHorRLim + 1, Top, AHorRLim + 1, Foot); // divid
  // vario ?
  lcdDrawLine(AHorRLim + 4, Top, AHorRLim + 4, Foot); // divid

// a.horizon:
  // top ~100 Flash
  drawFlightMode(AHorXcent - 2, Top + 2, mixerCurrentFlightMode, SMLSIZE | RIGHT);
  lcdDrawText(AHorXcent + 2, Top + 2, STAB, SMLSIZE);
  lcdDrawText(AHorRLim - 3 * FWNUM, Top + 2, "\xd1");
  lcdDrawNumber(lcdNextPos, Top + 2, Sats, SMLSIZE | (Sats < 5 ? BLINK : 0));
  
  // upSdo = abs(pith) > 79; TODO:  notyet,, the orientation is reversed within 90 + giro ?
  CY = AHorYcent;
  //fSinCos(Rad2Bit(pith));
  fSinCos(CH2Bit(1) / 4);
  CY += ((FOV << 7) * -fSin) >> 14;

  //fSinCos(Rad2Bit(roll));
  //upSdo = abs(roll) > 157;
  fSinCos(CH2Bit(0) / 2);
  upSdo = abs(CH2Bit(0) / 2) > 32;
  // ground fill  ~2228 Flash - 16 RAM
  for (x = AHorLLim + 1; x < AHorXcent; x++) {
    YY = ((fSin << 7) / (fCos == 0 ? 1 : fCos) * (x << 7)) >> 14;
    hoYdrLim(CY + YY);
    if (UaDdraw) lcdDrawLine(AHorXcent - x, y, AHorXcent - x, (upSdo ? AHorTopLim : AHorFootLim));
    hoYdrLim(CY - YY);
    if (UaDdraw) lcdDrawLine(AHorXcent + x, y, AHorXcent + x, (upSdo ? AHorTopLim : AHorFootLim));
  };
  hoYdrLim(CY);
  if (UaDdraw) lcdDrawLine(AHorXcent , y, AHorXcent, (upSdo ? AHorTopLim : AHorFootLim));

   // crosshair :D  ~228 Flash
    lcdDrawLine(AHorXcent , AHorYcent - 1, AHorXcent, AHorYcent + 1);
    lcdDrawLine(AHorXcent - 1, AHorYcent, AHorXcent + 1, AHorYcent);
    lcdDrawLine(AHorXcent - 6, AHorYcent, AHorXcent - 6, AHorYcent + 3);
    lcdDrawLine(AHorXcent + 6, AHorYcent, AHorXcent + 6, AHorYcent + 3);
    lcdDrawLine(AHorXcent + 7, AHorYcent, AHorXcent + 15, AHorYcent);
    lcdDrawLine(AHorXcent - 15, AHorYcent, AHorXcent - 7, AHorYcent);

  // side indicators ~116 Flash
    lcdDrawFilledRect(AHorLLim, AHorYcent - 5, 3 * FWNUM + 3, FH + 2, SOLID, FORCE);
    lcdDrawFilledRect(AHorLLim + 1, AHorYcent - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
    lcdDrawNumber(AHorLLim + 2, AHorYcent - 3, Sped, SMLSIZE);
    lcdDrawFilledRect(AHorRLim - 3 * FWNUM - 1, AHorYcent - 5, 3 * FWNUM + 3, FH + 2, SOLID, FORCE);
    lcdDrawFilledRect(AHorRLim - 3 * FWNUM, AHorYcent - 4, 3 * FWNUM + 1, FH, SOLID, ERASE);
    lcdDrawNumber(AHorRLim - 3 * FWNUM + 1, AHorYcent - 3, Alti, SMLSIZE);
  
  // botoom:
  // linear compass rose ~1276 Flash - 8 RAM
  YY = CMPradNwrap(3);  // head nw wrap
  YY = (YY < 0) ? (CMPnFielGAU * 8 - abs(YY)) : YY; // -360
  y = (YY > CMPnFielGAU * 7) ? 0 : ((YY + CMPnFielGAU / 2) / CMPnFielGAU); // compRose[n] alig
  x = CMPnFielGAU - ((YY + CMPnFielGAU / 2) % CMPnFielGAU); // field centr pos wrap
  if ((CMPFielAlig + x - 2 * CMPnFielGAU) > (AHorLLim + 3)) lcdDrawText(CMPFielAlig + x - 2 * CMPnFielGAU, Foot - FH + 1, compRose[0][(y - 2 < 0) ? 6 : y - 2], SMLSIZE | INVERS | CENTERED);
    lcdDrawText(CMPFielAlig + x - CMPnFielGAU, Foot - FH + 1, compRose[0][(y - 1 < 0) ? 7 : y - 1], SMLSIZE | INVERS | CENTERED);
    lcdDrawText(CMPFielAlig + x, Foot - FH + 1, compRose[0][y], SMLSIZE | INVERS | CENTERED);
    lcdDrawText(CMPFielAlig + x + CMPnFielGAU, Foot - FH + 1, compRose[0][(y + 1 > 7) ? 0 : y + 1], SMLSIZE | INVERS | CENTERED);
  if ((CMPFielAlig + x + 2 * CMPnFielGAU) < (AHorRLim - FW)) lcdDrawText(CMPFielAlig + x + 2 * CMPnFielGAU, Foot - FH + 1, compRose[0][(y + 2 > 7) ? 0 : y + 2], SMLSIZE | INVERS | CENTERED);
  // ladder
  // home simbol
  
  // rigth block: ~200 Flash
  lcdDrawFilledRect(AHorRLim + FW, AHorTopLim, 26, FW + 1, SOLID); // B%
  putsVolts(AHorRLim + FW, AHorTopLim + FH - 1, RxBt, MIDSIZE); //if 12v SMLSIZE
  lcdDrawNumber(AHorRLim + FW, AHorTopLim + 2 * FH + 4, Curr, SMLSIZE);
  lcdDrawText(lcdNextPos, AHorTopLim + 2 * FH + 4, "A", SMLSIZE);
  lcdDrawNumber(AHorRLim + FW, AHorTopLim + 3 * FH + 3, Cons, SMLSIZE);
  lcdDrawText(AHorRLim + 3 * FW, AHorTopLim + 4 * FH + 1, "MAH", TINSIZE);
  lcdDrawText(AHorRLim + FW + 1, Foot - 2 * FH + 2, "R:", TINSIZE | (RSSI < g_model.rssiAlarms.getWarningRssi()) ? BLINK | INVERS | TINSIZE : 0);
  lcdDrawFilledRect(AHorRLim + FW, Foot - 2 * FH + 1, 26, FW + 1, SOLID);
  lcdDrawText(AHorRLim + FW + 1, Foot - FH + 2, "Q:", TINSIZE);
  lcdDrawFilledRect(AHorRLim + FW, Foot - FH + 1, 26, FW + 1, SOLID);
}
void hello_run(event_t event) {
//globalData.cToolRunning = 1;
if (event == EVT_KEY_FIRST(KEY_UP)) {
  AUDIO_TRIM_MAX();
}
if (event == EVT_KEY_FIRST(KEY_DOWN)) {
  AUDIO_TRIM_MIN();
}
if (event == EVT_KEY_FIRST(KEY_ENTER)) {
  AUDIO_TRIM_MIDDLE();
}
if (event == EVT_KEY_LONG(KEY_EXIT)) {
  //globalData.cToolRunning = 0;
  popMenu();
}
lcdClear();
hello_draw();
};


