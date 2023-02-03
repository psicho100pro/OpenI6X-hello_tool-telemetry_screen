#include "opentx.h"

float hello_draw()
{
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
