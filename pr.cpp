#include "opentx.h"

#define ROOF 8                                   // y
#define FLOOR (LCD_H - ROOF - 1)                 // y adjustable, it needs to be an even number cus center
#define HCENTER (ROOF + ((FLOOR - 1 - ROOF) / 2)) // y
#define HORLLIM 0                                // x
#define HORDLIM 97                               // x divid, adjustable, it needs to be an odd number cus center
#define HORRLIM (LCD_W - 1)                      // x    
#define HORWCENTER ((HORDLIM - 1) / 2)           // x
#define FOV 42                                   // fe. 45°=30, 40°=32, 35°=36 ,30°=42, 25°=60 - when the horizon touches ROOF or FLOOR // TODO: need to recalculate
#define SENSORSENSE true                       // sensor identifier

static const uint8_t lut[] = { // sin table fixed SCALE 6
0,1,2,3,4,6,7,8,9,10,
11,12,13,14,15,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,48,
49,50,50,51,52,52,53,54,54,55,55,56,57,57,58,58,58,59,59,60,
60,61,61,61,62,62,62,62,63,63,63,63,63,64,64,64,64,64,64,64
};
bool tgErrFlag = 0; //tangens 90°, 270°
int8_t fSinCos(int16_t deg, bool f = 0) // 0=sin 1=cos
{
  uint16_t ab = abs(f ? deg + (uint8_t)90 : deg);
  int8_t ret;
  if (ab < 90) { ret = lut[ab % 90]; }
  else if (ab < 180) { ret = lut[90 - ab % 90]; }
  else if (ab < 270) { ret = -lut[ab % 90]; }
  else { ret = -lut[90 - ab % 90]; }
  if (ab == 90 || ab == 270) tgErrFlag = 1;
  return (deg >= 0) ? ret : -ret;
};
uint8_t hYlimit(int8_t val) // y over limit
{
  if (ROOF >= val) return ROOF + 1;
  if (val >= FLOOR) return FLOOR - 1;
  return val;
};

uint8_t i = 0;

void hello_draw() {

// ROOF bar:
lcdDrawSolidFilledRect(0, 0, HORRLIM, ROOF);
putsVolts(1, 1, g_vbat100mV, SMLSIZE | INVERS | (IS_TXBATT_WARNING() ? BLINK : 0));
lcdDrawText(lcdNextPos + 1, 1, "TW:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, 1, telemetryItems[6].value, SMLSIZE | INVERS);
putsModelName(lcdNextPos + 2, 1, g_model.header.name, g_eeGeneral.currModel, TINSIZE | INVERS);
//lcdDrawSizedText(lcdNextPos + 2, 1, g_model.flightModeData[mixerCurrentFlightMode].name, sizeof(g_model.flightModeData[mixerCurrentFlightMode].name), TINSIZE | INVERS);
//drawFlightMode(lcdNextPos + 2, 1, mixerCurrentFlightMode, TINSIZE | INVERS);
if (g_model.timers[0].mode){
  lcdDrawText(HORRLIM - 33, 1, "T1:", SMLSIZE | INVERS);
  drawTimer(HORRLIM - 24, 1, timersStates[0].val, SMLSIZE |INVERS | (timersStates[0].val < 0 ? BLINK : 0));
};
if (g_model.timers[1].mode){
  lcdDrawText(HORRLIM - 33, 1, "T2:", SMLSIZE | INVERS);
  drawTimer(HORRLIM - 24, 1, timersStates[1].val, SMLSIZE | INVERS | (timersStates[1].val < 0 ? BLINK : 0));
};

// margin line
lcdDrawLine(HORDLIM, ROOF, HORDLIM, FLOOR); // divid

// a.horizon:
// TODO: the known phenomenon when turning upside down, the orientation is reversed within 180 degrees , it needs to be treated somehow
// pitch
uint8_t ahcy = HCENTER;
ahcy = hYlimit(((uint16_t(FOV << 6) * -fSinCos(telemetryItems[19].value)) >> 12) + HCENTER);
// roll
tgErrFlag = 0; // tangens 90°, 270°
int8_t sin = fSinCos(telemetryItems[20].value);
int8_t cos = fSinCos(telemetryItems[20].value, 1);
int16_t tan = (sin * (1 << 6) / (tgErrFlag ? (uint8_t)tgErrFlag : cos));
tan = (telemetryItems[20].value < 0) ? -tan : tan;

// ground fill
for (uint8_t x = HORLLIM + 1; x < HORWCENTER; x++)
{
  lcdDrawLine(HORWCENTER - x, hYlimit(ahcy + ((tan * uint16_t(x << 6)) >> 12)), HORWCENTER - x, FLOOR - 1); // L
  lcdDrawLine(HORWCENTER + x, hYlimit(ahcy - ((tan * uint16_t(x << 6)) >> 12)), HORWCENTER + x, FLOOR - 1); // R
};
lcdDrawLine(HORWCENTER, ahcy, HORWCENTER, FLOOR - 1); // mid

// crosshair :D
lcdDrawLine(HORWCENTER, HCENTER - 1, HORWCENTER, HCENTER + 1);
lcdDrawLine(HORWCENTER - 1, HCENTER, HORWCENTER + 1, HCENTER);
lcdDrawLine(HORWCENTER - 6, HCENTER, HORWCENTER - 6, HCENTER + 3);
lcdDrawLine(HORWCENTER + 6, HCENTER, HORWCENTER + 6, HCENTER + 3);
lcdDrawLine(HORWCENTER + 7, HCENTER, HORWCENTER + 15, HCENTER);
lcdDrawLine(HORWCENTER - 15, HCENTER, HORWCENTER - 7, HCENTER);

// yaw
lcdDrawFilledRect(HORWCENTER - 2 * FWNUM + 2, FLOOR - FH, FWNUM * 3 + 1, 8, SOLID, ERASE);
lcdDrawNumber(HORWCENTER - 2 * FWNUM + 3, FLOOR - FH + 1, telemetryItems[21].value, SMLSIZE);

// side indicators
lcdDrawFilledRect(HORLLIM, HCENTER - 5, FWNUM * 3 + 3, 10, SOLID, FORCE);
lcdDrawFilledRect(HORLLIM + 1, HCENTER - 4, FWNUM * 3 + 1, 8, SOLID, ERASE);
lcdDrawNumber(HORLLIM + 2, HCENTER - 3, telemetryItems[11].value, SMLSIZE);
lcdDrawFilledRect(HORDLIM - FWNUM * 3 - 2, HCENTER - 5, FWNUM * 3 + 2, 10, SOLID, FORCE);
lcdDrawFilledRect(HORDLIM - FWNUM * 3 - 1, HCENTER - 4, FWNUM * 3 + 1, 8, SOLID, ERASE);
lcdDrawNumber(HORDLIM - FWNUM * 3, HCENTER - 3, telemetryItems[13].value, SMLSIZE);

// rigth block: // not completed
lcdDrawText(HORDLIM + 5, ROOF + 2, "S:", SMLSIZE);
drawValueWithUnit(lcdNextPos + 4, ROOF + 2, telemetryItems[14].value, UNIT_RAW, SMLSIZE | RIGHT| (telemetryItems[14].value < 5 ? BLINK : 0));
drawValueWithUnit(HORDLIM + 10, ROOF + 10, telemetryItems[16].value, UNIT_AMPS, SMLSIZE | RIGHT);
drawValueWithUnit(HORDLIM + 10, ROOF + 20, telemetryItems[17].value, UNIT_MAH, SMLSIZE | RIGHT);

// FLOOR bar:
lcdDrawSolidFilledRect(0, FLOOR + 1, HORRLIM, FLOOR + 1);
putsVolts(1, FLOOR + 2, telemetryItems[15].value, SMLSIZE | INVERS);
lcdDrawText(lcdNextPos + 2, FLOOR + 2, "RS:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos, FLOOR + 2, telemetryData.rssi.value, SMLSIZE | INVERS ); // | (telemetryData.rssi.value < g_model.rssiAlarms.getWarningRssi()) ? BLINK : 0);
lcdDrawText(lcdNextPos + 2, FLOOR + 2, "RQ:", SMLSIZE | INVERS);
lcdDrawNumber(lcdNextPos , FLOOR + 2, telemetryItems[2].value, SMLSIZE | INVERS);
drawValueWithUnit(HORRLIM, FLOOR + 2, telemetryItems[22].value, UNIT_TEXT, SMLSIZE | INVERS | RIGHT);

#if defined(SENSORSENSE)
if (isTelemetryFieldAvailable(i)){
  TelemetrySensor &sensor = g_model.telemetrySensors[i];
  char sensorName[TELEM_LABEL_LEN + 7];
  zchar2str(sensorName, sensor.label, TELEM_LABEL_LEN);
  lcdDrawNumber(5, 15, i, SMLSIZE);
  lcdDrawText(lcdNextPos + 3 , 15, sensorName, SMLSIZE);
  drawValueWithUnit(lcdNextPos, 15, telemetryItems[i].value, sensor.unit, SMLSIZE);
}else{
  lcdDrawNumber(5, 15, i, SMLSIZE);
  lcdDrawText(10, 15, "NENI", SMLSIZE);
};
#endif
}

void hello_stop()
{
  popMenu();
};
void hello_run(event_t event)
{
  if (event == EVT_KEY_FIRST(KEY_UP))
  {
     i++;
     AUDIO_TRIM_MAX();
  }
  if (event == EVT_KEY_FIRST(KEY_DOWN))
  {
    i--;
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
