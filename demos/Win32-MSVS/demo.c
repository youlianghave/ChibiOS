/*
    ChibiOS/RT - Copyright (C) 2006-2007 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdio.h>

#include <ch.h>

static ULONG32 wdguard;
static BYTE8 wdarea[UserStackSize(2048)];

static ULONG32 cdguard;
static BYTE8 cdarea[UserStackSize(2048)];
static Thread *cdtp;

static t_msg WatchdogThread(void *arg);
static t_msg ConsoleThread(void *arg);

t_msg TestThread(void *p);

void InitCore(void);
extern FullDuplexDriver COM1, COM2;

#define cprint(msg) chMsgSend(cdtp, (t_msg)msg)

/*
 * Watchdog thread, it checks magic values located under the various stack
 * areas. The system is halted if something is wrong.
 */
static t_msg WatchdogThread(void *arg) {
  wdguard = 0xA51F2E3D;
  cdguard = 0xA51F2E3D;
  while (TRUE) {

    if ((wdguard != 0xA51F2E3D) ||
        (cdguard != 0xA51F2E3D)) {
      printf("Halted by watchdog");
      chSysHalt();
    }
    chThdSleep(50);
  }
  return 0;
}

/*
 * Console print server done using synchronous messages. This makes the access
 * to the C printf() thread safe and the print operation atomic among threads.
 * In this example the message is the zero termitated string itself.
 */
static t_msg ConsoleThread(void *arg) {

  while (!chThdShouldTerminate()) {
    printf((char *)chMsgWait());
    chMsgRelease(RDY_OK);
  }
  return 0;
}

static void PrintLineFDD(FullDuplexDriver *sd, char *msg) {

  while (*msg)
    chFDDPut(sd, *msg++);
}

static BOOL GetLineFDD(FullDuplexDriver *sd, char *line, int size) {
  char *p = line;

  while (TRUE) {
    short c = chIQGet(&sd->sd_iqueue);
    if (c < 0)
      return TRUE;
    if (c == 4) {
      PrintLineFDD(sd, "^D\r\n");
      return TRUE;
    }
    if (c == 8) {
      if (p != line) {
        chFDDPut(sd, (BYTE8)c);
        chFDDPut(sd, 0x20);
        chFDDPut(sd, (BYTE8)c);
        p--;
      }
      continue;
    }
    if (c == '\r') {
      PrintLineFDD(sd, "\r\n");
      *p = 0;
      return FALSE;
    }
    if (c < 0x20)
      continue;
    if (p < line + size - 1) {
      chFDDPut(sd, (BYTE8)c);
      *p++ = (BYTE8)c;
    }
  }
}

/*
 * Example thread, not much to see here. It simulates the CTRL-C but there
 * are no real signals involved.
 */
static t_msg HelloWorldThread(void *arg) {
  int i;
  short c;
  FullDuplexDriver *sd = (FullDuplexDriver *)arg;

  for (i = 0; i < 100; i++) {

    PrintLineFDD(sd, "Hello World\r\n");
    c = chFDDGetTimeout(sd, 333);
    switch (c) {
    case -1:
      continue;
    case -2:
      return 1;
    case 3:
      PrintLineFDD(sd, "^C\r\n");
      return 0;
    default:
      chThdSleep(333);
    }
  }
  return 0;
}

static BOOL checkend(FullDuplexDriver *sd) {

  char * lp = strtok(NULL, " \009"); /* It is not thread safe but this is a demo.*/
  if (lp) {
    PrintLineFDD(sd, lp);
    PrintLineFDD(sd, " ?\r\n");
    return TRUE;
  }
  return FALSE;
}

/*
 * Simple command shell thread, the argument is the serial line for the
 * standard input and output. It recognizes few simple commands.
 */
static t_msg ShellThread(void *arg) {
  FullDuplexDriver *sd = (FullDuplexDriver *)arg;
  char *lp, line[64];
  Thread *tp;
  BYTE8 tarea[UserStackSize(1024)];

  chIQReset(&sd->sd_iqueue);
  chOQReset(&sd->sd_oqueue);
  PrintLineFDD(sd, "ChibiOS/RT Command Shell\r\n\n");
  while (TRUE) {
    PrintLineFDD(sd, "ch> ");
    if (GetLineFDD(sd, line, sizeof(line))) {
      PrintLineFDD(sd, "\nlogout");
      break;
    }
    lp = strtok(line, " \009"); // Note: not thread safe but it is just a demo.
    if (lp) {
      if ((stricmp(lp, "help") == 0) ||
          (stricmp(lp, "h") == 0) ||
          (stricmp(lp, "?") == 0)) {
        if (checkend(sd))
          continue;
        PrintLineFDD(sd, "Commands:\r\n");
        PrintLineFDD(sd, "  help,h,? - This help\r\n");
        PrintLineFDD(sd, "  exit     - Logout from ChibiOS/RT\r\n");
        PrintLineFDD(sd, "  time     - Prints the system timer value\r\n");
        PrintLineFDD(sd, "  hello    - Runs the Hello World demo thread\r\n");
        PrintLineFDD(sd, "  test     - Runs the System Test thread\r\n");
      }
      else if (stricmp(lp, "exit") == 0) {
        if (checkend(sd))
          continue;
        PrintLineFDD(sd, "\nlogout");
        break;
      }
      else if (stricmp(lp, "time") == 0) {
        if (checkend(sd))
          continue;
        sprintf(line, "Time: %d\r\n", chSysGetTime());
        PrintLineFDD(sd, line);
      }
      else if (stricmp(lp, "hello") == 0) {
        if (checkend(sd))
          continue;
        tp = chThdCreate(NORMALPRIO, 0, tarea, sizeof(tarea),
                         HelloWorldThread, sd);
        if (chThdWait(tp))
          break;  // Lost connection while executing the hello thread.
      }
      else if (stricmp(lp, "test") == 0) {
        if (checkend(sd))
          continue;
        tp = chThdCreate(NORMALPRIO, 0, tarea, sizeof(tarea),
                         TestThread, arg);
        if (chThdWait(tp))
          break;  // Lost connection while executing the hello thread.
      }
      else {
        PrintLineFDD(sd, lp);
        PrintLineFDD(sd, " ?\r\n");
      }
    }
  }
  return 0;
}

static BYTE8 s1area[UserStackSize(4096)];
static Thread *s1;
EventListener s1tel;

static void COM1Handler(t_eventid id) {
  t_dflags flags;

  if (s1 && chThdTerminated(s1)) {
    s1 = NULL;
    cprint("Init: disconnection on COM1\n");
  }
  flags = chFDDGetAndClearFlags(&COM1);
  if ((flags & SD_CONNECTED) && (s1 == NULL)) {
    cprint("Init: connection on COM1\n");
    s1 = chThdCreate(NORMALPRIO, P_SUSPENDED, s1area, sizeof(s1area),
                     ShellThread, &COM1);
    chEvtRegister(chThdGetExitEventSource(s1), &s1tel, 0);
    chThdResume(s1);
  }
  if ((flags & SD_DISCONNECTED) && (s1 != NULL))
    chIQReset(&COM1.sd_iqueue);
}

static BYTE8 s2area[UserStackSize(4096)];
static Thread *s2;
EventListener s2tel;

static void COM2Handler(t_eventid id) {
  t_dflags flags;

  if (s2 && chThdTerminated(s2)) {
    s2 = NULL;
    cprint("Init: disconnection on COM2\n");
  }
  flags = chFDDGetAndClearFlags(&COM2);
  if ((flags & SD_CONNECTED) && (s2 == NULL)) {
    cprint("Init: connection on COM2\n");
    s2 = chThdCreate(NORMALPRIO, P_SUSPENDED, s2area, sizeof(s1area),
                     ShellThread, &COM2);
    chEvtRegister(chThdGetExitEventSource(s2), &s2tel, 1);
    chThdResume(s2);
  }
  if ((flags & SD_DISCONNECTED) && (s2 != NULL))
    chIQReset(&COM2.sd_iqueue);
}

static t_evhandler fhandlers[2] = {
  COM1Handler,
  COM2Handler
};

/*------------------------------------------------------------------------*
 * Simulator main, start here your threads, examples inside.              *
 *------------------------------------------------------------------------*/
int main(void) {
  EventListener c1fel, c2fel;

  InitCore();

  // Startup ChibiOS/RT.
  chSysInit();

  chThdCreate(NORMALPRIO + 2, 0, wdarea, sizeof(wdarea), WatchdogThread, NULL);
  cdtp = chThdCreate(NORMALPRIO + 1, 0, cdarea, sizeof(cdarea), ConsoleThread, NULL);

  cprint("Console service started on COM1, COM2\n");
  cprint("  - Listening for connections on COM1\n");
  chFDDGetAndClearFlags(&COM1);
  chEvtRegister(&COM1.sd_sevent, &c1fel, 0);
  cprint("  - Listening for connections on COM2\n");
  chFDDGetAndClearFlags(&COM2);
  chEvtRegister(&COM2.sd_sevent, &c2fel, 1);
  while (!chThdShouldTerminate())
    chEvtWait(ALL_EVENTS, fhandlers);
  chEvtUnregister(&COM2.sd_sevent, &c2fel); // Never invoked but this is an example...
  chEvtUnregister(&COM1.sd_sevent, &c1fel); // Never invoked but this is an example...
  return 0;
}
