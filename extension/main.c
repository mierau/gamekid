// Gamekid
// main.c

#include "pd_api.h"
#include "app.h"

PlaydateAPI* playdate = NULL;

int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg) {
  if(event == kEventInit) {
    playdate = pd;
    GKAppRun();
  }

  return 0;
}