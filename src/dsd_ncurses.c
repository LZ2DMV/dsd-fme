/*-------------------------------------------------------------------------------
 * dsd_ncurses.c
 * A dsd ncurses terminal printer
 *
 * ASCII art generated by:
 * https://fsymbols.com/generators/carty/
 *
 * LWVMOBILE
 * 2022-02 DSD-FME Florida Man Edition
 *-----------------------------------------------------------------------------*/
 /*
  * Copyright (C) 2010 DSD Author
  * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
  *
  * Permission to use, copy, modify, and/or distribute this software for any
  * purpose with or without fee is hereby granted, provided that the above
  * copyright notice and this permission notice appear in all copies.
  *
  * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
  * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
  * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
  * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
  * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
  * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
  * PERFORMANCE OF THIS SOFTWARE.
  */

#include "dsd.h"
#include "git_ver.h"

int reset = 0;
char c; //getch key
int tg; //last tg
int rd; //last rid
int rn; //last ran
int nc; //nac
int src; //last rid? double?
int lls = -1; //last last sync type
int dcc = 0; //initialize with 0 to prevent core dump
int i = 0;
char * s0last = "     ";
char * s1last = "     ";
char versionstr[25];
unsigned long long int call_matrix[33][6]; //0 - sync type; 1 - tg/ran; 2 - rid; 3 - slot; 4 - cc; 5 - time(NULL) ;
char * FM_bannerN[9] = {
  "                                 CTRL + C twice to exit",
  " ██████╗  ██████╗██████╗     ███████╗███╗   ███╗███████╗",
  " ██╔══██╗██╔════╝██╔══██╗    ██╔════╝████╗ ████║██╔════╝",
  " ██║  ██║╚█████╗ ██║  ██║    █████╗  ██╔████╔██║█████╗  ",
  " ██║  ██║ ╚═══██╗██║  ██║    ██╔══╝  ██║╚██╔╝██║██╔══╝  ",
  " ██████╔╝██████╔╝██████╔╝    ██║     ██║ ╚═╝ ██║███████╗",
  " ╚═════╝ ╚═════╝ ╚═════╝     ╚═╝     ╚═╝     ╚═╝╚══════╝",
  "https://github.com/lwvmobile/dsd-fme/tree/pulseaudio    "
};

char * SyncTypes[20] = {
  "+P25P1",
  "-P25P1",
  "+X2TDMA DATA",
  "-X2TDMA DATA",
  "+X2TDMA VOICE",
  "-X2TDMA VOICE",
  "+DSTARC",
  "-DSTAR",
  "+NXDN VOICE",      //8
  "-NXDN VOICE",  //9
  "+DMR DATA",     //10
  "-DMR DATA",    //11
  "+DMR VOICE",     //12
  "-DMR VOICE", //13
  "+PROVOICE",            //14
  "-PROVOICE",        //15
  "+NXDN DATA",        //16
  "-NXDN DATA",    //17
  "+DSTAR HD",
  "-DSTAR HD"

};

time_t nowN;
char * getTimeN(void) //get pretty hh:mm:ss timestamp
{
  time_t t = time(NULL);

  char * curr;
  char * stamp = asctime(localtime( & t));

  curr = strtok(stamp, " ");
  curr = strtok(NULL, " ");
  curr = strtok(NULL, " ");
  curr = strtok(NULL, " ");

  return curr;
}

void ncursesOpen ()
{
  mbe_printVersion (versionstr);
  setlocale(LC_ALL, "");
  initscr(); //Initialize NCURSES screen window
  start_color();
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);      //Yellow/Amber for frame sync/control channel, NV style
  init_pair(2, COLOR_RED, COLOR_BLACK);        //Red for Terminated Calls
  init_pair(3, COLOR_GREEN, COLOR_BLACK);     //Green for Active Calls
  init_pair(4, COLOR_CYAN, COLOR_BLACK);     //Cyan for Site Extra and Patches
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK); //Magenta for no frame sync/signal
  noecho();
  cbreak();
  //fprintf (stderr, "Opening NCurses Terminal. \n");

}

void
ncursesPrinter (dsd_opts * opts, dsd_state * state)
{
  int level;

  //level = (int) state->max / 164;
  level = 0; //start each cycle with 0
  erase();
  //disabling until wide support can be built for LM, etc. $(ncursesw5-config --cflags --libs)
  printw ("%s \n", FM_bannerN[0]); //top line in white
  attron(COLOR_PAIR(4));
  for (short int i = 1; i < 7; i++) //following lines in cyan
  {
    printw("%s \n", FM_bannerN[i]);
  }
  attroff(COLOR_PAIR(4));
  printw ("--Build Info------------------------------------------------------------------\n");
  printw ("| %s \n", FM_bannerN[7]); //http link
  printw ("| Digital Speech Decoder: Florida Man Edition\n");
  printw ("| Github Build Version: %s \n", GIT_TAG);
  printw ("| mbelib version %s\n", versionstr);
  //printw ("| Press CTRL+C twice to exit\n");
  printw ("------------------------------------------------------------------------------\n");

  if (state->carrier == 0) //reset these to 0 when no carrier
  {
    //state->payload_algid = 0;
    //state->payload_keyid = 0;
    //state->payload_mfid = 0;
  }

  if ( (lls == 14 || lls == 15) && (time(NULL) - call_matrix[9][5] > 5) && state->carrier == 1) //honestly have no idea how to do this for pV, just going time based? only update on carrier == 1.
  {
    for (short int k = 0; k < 9; k++)
    {
      call_matrix[k][0] = call_matrix[k+1][0];
      call_matrix[k][1] = call_matrix[k+1][1];
      call_matrix[k][2] = call_matrix[k+1][2];
      call_matrix[k][3] = call_matrix[k+1][3];
      call_matrix[k][4] = call_matrix[k+1][4];
      call_matrix[k][5] = call_matrix[k+1][5];
    }

    call_matrix[9][0] = lls;
    call_matrix[9][1] = 1;
    call_matrix[9][2] = 1;
    call_matrix[9][3] = 1;
    call_matrix[9][4] = 1;
    call_matrix[9][5] = time(NULL);
    //nowN = time(NULL);
  }

  //if ( (state->nxdn_last_rid != src && src > 0) || (state->nxdn_last_ran != rn && rn > 0) ) //find condition to make this work well, probably if last != local last variables
  //if ( (call_matrix[9][2] != src && src > 0) || (call_matrix[9][1] != rn && rn > 0) ) //NXDN working well now with this, updates immediately and only once
  if ( call_matrix[9][2] != src && src > 0 && rn > 0 ) //NXDN working well now with this, updates immediately and only once
  {
    for (short int k = 0; k < 9; k++)
    {
      call_matrix[k][0] = call_matrix[k+1][0];
      call_matrix[k][1] = call_matrix[k+1][1];
      call_matrix[k][2] = call_matrix[k+1][2];
      call_matrix[k][3] = call_matrix[k+1][3];
      call_matrix[k][4] = call_matrix[k+1][4];
      call_matrix[k][5] = call_matrix[k+1][5];
    }
    call_matrix[9][0] = lls;
    call_matrix[9][1] = rn;
    call_matrix[9][2] = src;
    call_matrix[9][3] = 0;
    call_matrix[9][4] = 0;
    call_matrix[9][5] = time(NULL);

  }

  //if (state->dmr_color_code != dcc && (lls == 11 || lls == 13 || lls == 10 || lls == 12) ) //DMR Voice + last two is data
  //if ( (call_matrix[9][4] != dcc || call_matrix[9][2] != rd) && (lls == 10 || lls == 11 || lls == 12 || lls == 13) ) //DMR
  if ( (call_matrix[9][4] != dcc || call_matrix[9][2] != rd) && (lls == 12 || lls == 13) ) //DMR
  {
    //dcc = state->dmr_color_code;
    for (short int k = 0; k < 9; k++)
    {
      call_matrix[k][0] = call_matrix[k+1][0];
      call_matrix[k][1] = call_matrix[k+1][1];
      call_matrix[k][2] = call_matrix[k+1][2];
      call_matrix[k][3] = call_matrix[k+1][3];
      call_matrix[k][4] = call_matrix[k+1][4];
      call_matrix[k][5] = call_matrix[k+1][5];
    }

    call_matrix[9][0] = lls;
    call_matrix[9][1] = tg;
    call_matrix[9][2] = rd;
    call_matrix[9][3] = 0;
    call_matrix[9][4] = dcc;
    call_matrix[9][5] = time(NULL);
    //i++;
  }

  //if ( (lls == 0 || lls == 1) && state->lastsrc != rd && state->lastsrc > 0) //find condition to make this work well, probably if last != local last variables
  if ( (lls == 0 || lls == 1) && call_matrix[9][2] != rd && nc > 0 && tg > 0) //P25
  {
    for (short int k = 0; k < 9; k++)
    {
      call_matrix[k][0] = call_matrix[k+1][0];
      call_matrix[k][1] = call_matrix[k+1][1];
      call_matrix[k][2] = call_matrix[k+1][2];
      call_matrix[k][3] = call_matrix[k+1][3];
      call_matrix[k][4] = call_matrix[k+1][4];
      call_matrix[k][5] = call_matrix[k+1][5];
    }

    call_matrix[9][0] = lls;
    call_matrix[9][1] = tg;
    call_matrix[9][2] = rd;
    call_matrix[9][3] = 0;
    call_matrix[9][4] = nc;
    call_matrix[9][5] = time(NULL);
    //i++;
  }
  //printw ("Time: ");
  //printw ("%s \n", getTimeN());
  attron(COLOR_PAIR(4));
  printw ("--Input Output----------------------------------------------------------------\n");
  if (opts->audio_in_type == 0)
  {
    printw ("| Pulse Audio  Input [%i] kHz [%i] Channel\n", opts->pulse_digi_rate_in/1000, opts->pulse_digi_in_channels);
  }
  if (opts->audio_in_type == 1)
  {
    printw ("| STDIN - Input\n");
  }
  if (opts->audio_in_type == 3)
  {
    printw ("| RTL2838 Device #[%d]", opts->rtl_dev_index);
    printw (" Gain [%i] dB -", opts->rtl_gain_value);
    printw (" Squelch [%i]", opts->rtl_squelch_level);
    printw (" VFO [%i] kHz\n", opts->rtl_bandwidth);
    printw ("| Freq: [%lld] Hz", opts->rtlsdr_center_freq);
    printw (" - Tuning available on UDP Port [%i]\n", opts->rtl_udp_port);
  }
  if (opts->audio_out_type == 0)
  {
    printw ("| Pulse Audio Output [%i] kHz [%i] Channel\n", opts->pulse_digi_rate_out/1000, opts->pulse_digi_out_channels);
  }
  if (opts->mbe_out_dir[0] != 0)
  {
    printw ("| Writing MBE data files to directory %s\n", opts->mbe_out_dir);
  }
  if (opts->wav_out_file[0] != 0)
  {
    printw ("| Writing Audio WAV to file %s\n", opts->wav_out_file);
  }

  printw ("------------------------------------------------------------------------------\n");
  attroff(COLOR_PAIR(4));

  if (state->carrier == 1){ //figure out method that will tell me when is active and when not active, maybe carrier but this doesn't print anyways unless activity
    attron(COLOR_PAIR(3));
    level = (int) state->max / 164; //only update on carrier present
    reset = 1;
  }
  if (state->carrier == 0 && opts->reset_state == 1 && reset == 1)
  {
    resetState (state);
    reset = 0;
  }


  printw ("--Audio Decode----------------------------------------------------------------\n");
  printw ("| In Level:    [%3i%%] \n", level);
  printw ("| Voice Error: [%i][%i] \n| Error Bars:  [%s] \n", state->errs, state->errs2, state->err_str); //
  //printw ("| Carrier = %i\n", state->carrier);
  printw ("------------------------------------------------------------------------------\n");


  printw ("--Call Info-------------------------------------------------------------------\n");
  if (state->lastsynctype != -1) //not sure if this will be okay
  {
    lls = state->lastsynctype;
  }
  if (state->nxdn_last_rid > 0 && state->nxdn_last_rid != src);
  {
    src = state->nxdn_last_rid;
    //rn = state->nxdn_last_ran;
  }
  if (state->nxdn_last_ran > 0 && state->nxdn_last_rid != rn);
  {
    //src = state->nxdn_last_rid;
    rn = state->nxdn_last_ran;
  }

  if (state->dmr_color_code > 0 && (lls == 10 || lls == 11 || lls == 12 || lls == 13) ) //DMR, DCC only carried on Data?
  {
    dcc = state->dmr_color_code;
  }

  if (state->lastsrc > 0 && (lls == 12 || lls == 13))
  {
    rd = state->lastsrc;
  }

  if (state->lasttg > 0 && (lls == 12 || lls == 13))
  {
    tg = state->lasttg;
  }

  //if (state->lastsynctype == 8 || state->lastsynctype == 9 || state->lastsynctype == 16 || state->lastsynctype == 17) //change this to NXDN syncs later on
  if (lls == 8 || lls == 9 || lls == 16 || lls == 17)
  {
    //printw ("| RAN: [%02d] \n", state->nxdn_last_ran);
    printw ("| RAN: [%02d] \n", rn);
    printw ("| RID: [%d] \n", src); //maybe change to nxdn_last_rid
  }

  //printw ("Error?: [%i] [%i] \n", state->errs, state->errs2); //what are these?

  if (state->lasttg > 0 && state->lastsrc > 0)
  {
    tg = state->lasttg;
    rd = state->lastsrc;
  }
  if (state->nac > 0)
  {
    nc = state->nac;
  }
  if ((lls == 0 || lls == 1)) //1 for P25 P1 Audio
  {
    //printw("| TID:[%i] | RID:[%i] \n", tg, rd);
    //printw("| NAC: [0x%X] \n", nc);
    printw("| TID:[%i] RID:[%i] ", tg, rd);
    printw("NAC: [0x%X] \n", nc);
    printw("| ALG: [0x%02X] ", state->payload_algid);
    printw("KEY: [0x%04X] ", state->payload_keyid);
    //printw("MFG: [0x%X] ", state->payload_mfid); //no way of knowing if this is accurate info yet
    if (state->payload_keyid != 0 && state->carrier == 1)
    {
      attron(COLOR_PAIR(2));
      printw("**ENC**");
      attroff(COLOR_PAIR(2));
      attron(COLOR_PAIR(3));
    }
    printw("\n");
    //printw("| System Type: %s \n", ALGIDS[state->payload_keyid] );
  }
  //if (state->lastsynctype == 12 || state->lastsynctype == 13)  //DMR Voice Types
  if (lls == 12 || lls == 13)  //DMR Voice Types
  {
    printw ("| DCC: [%i]\n", dcc);
    //printw ("%s ", state->slot0light);
    printw ("| SLOT 0 ");
    if (state->currentslot == 0) //find out how to tell when slot0 has voice
    {
      s0last = "Voice";
      //printw("Voice");
    }
    printw ("%s ", s0last);

    //printw ("%s ", state->slot1light);
    printw ("SLOT 1 ");
    if (state->currentslot == 1) //find out how to tell when slot1 has voice
    {
      s1last = "Voice";
      //printw("Voice");
    }
    printw ("%s \n", s1last);
  }


  //if (state->lastsynctype == 10 || state->lastsynctype == 11)  //DMR Data Types
  if (lls == 10 || lls == 11)  //DMR Data Types
  {
    printw ("| DCC: [%i]\n", dcc);
    //printw ("%s ", state->slot0light);
    printw ("| SLOT 0 ");
    if (state->currentslot == 0) //find out how to tell when slot0 has voice
    {
      s0last = "Data ";
      if (strcmp (state->fsubtype, " Slot idle    ") == 0)
      {
        s0last = "Idle ";
      }
    }
    printw ("%s ", s0last);
    //printw ("%s ", state->slot1light);
    printw ("SLOT 1 ");
    if (state->currentslot == 1) //find out how to tell when slot1 has voice
    {
      s1last = "Data ";
      if (strcmp (state->fsubtype, " Slot idle    ") == 0)
      {
        s1last = "Idle ";
      }
    }
    printw ("%s \n", s1last);
  }
  if (lls != -1) //is there a synctype 0?
  {
    printw ("| %s \n", SyncTypes[lls]);
  }
  printw ("------------------------------------------------------------------------------\n");
  //colors off
  if (state->carrier == 1){ //same as above
    attroff(COLOR_PAIR(3));
  }
  //if (state->carrier == 0){ //same as above
    //attroff(COLOR_PAIR(1));
  //}

  attron(COLOR_PAIR(4)); //cyan for history
  //add other interesting info to put here
  //make Call_Matrix
  //0 - sync type; 1 - tg/ran; 2 - rid; 3 - slot; 4 - cc; 5 - time(NULL) ;

  printw ("--Call History----------------------------------------------------------------\n");
  for (short int j = 0; j < 10; j++)
  {
    if ( ((time(NULL) - call_matrix[9-j][5]) < 9999)  )
    //if (1 == 1)
    {
    printw ("| #%d %s ", j, SyncTypes[call_matrix[9-j][0]]);
    if (lls == 8 || lls == 9 || lls == 16 || lls == 17)
    {
      printw ("RAN [%2d] ", call_matrix[9-j][1]);
    }
    if (lls == 0 || lls == 1 || lls == 12 || lls == 13 || lls == 10 || lls == 11 ) //P25 P1 and DMR
    {
      printw ("TID [%2d] ", call_matrix[9-j][1]);
    }

    printw ("RID [%4d] ", call_matrix[9-j][2]);
    //printw ("S %d - ", call_matrix[j][3]);
    if (call_matrix[9-j][0] == 0 || call_matrix[9-j][0] == 1) //P25P1 Voice
    {
      printw ("NAC [0x%X] ", call_matrix[9-j][4]);
    }
    if (call_matrix[9-j][0] == 12 || call_matrix[9-j][0] == 13 || call_matrix[9-j][0] == 10 || call_matrix[9-j][0] == 11 ) //DMR Voice Types
    {
      printw ("DCC [%d] ", call_matrix[9-j][4]);
    }
    printw ("%d secs ago\n", time(NULL) - call_matrix[9-j][5]);
   }
    //printw("\n");
  }
  printw ("------------------------------------------------------------------------------\n");
  attroff(COLOR_PAIR(4)); //cyan for history
  //put sync type at very bottom
  //printw ("%s %s\n", state->ftype, state->fsubtype); //some ftype strings have extra spaces in them
  //if (state->lastsynctype != -1) //is there a synctype 0?

  //while((c = getch()) != '~') {
  //  printw ("%c\n",c);}
  //c = getch();
  /*
  if (c = getch() != '~')
  {
    printf ("%c\n",c);
    printf ("closing NCurses");
    ncursesClose();
  }
  */
  //debug sr printw
  if (1==2) //disable this later on
  {

    printw ("sr_0 = %16llX \n", state->sr_0);
    printw ("sr_1 = %16llX \n", state->sr_1);
    printw ("sr_2 = %16llX \n", state->sr_2);
    printw ("sr_3 = %16llX \n", state->sr_3);
    printw ("sr_4 = %16llX \n", state->sr_4);
    printw ("sr_5 = %16llX \n", state->sr_5);
    printw ("sr_6 = %16llX \n", state->sr_6);

  }
  refresh();
}

void ncursesClose ()
{
  endwin();
  printf("Press CTRL+C again to close. Thanks.\n");
  printf("Run 'reset' in your terminal to clean up if necessary.");
}