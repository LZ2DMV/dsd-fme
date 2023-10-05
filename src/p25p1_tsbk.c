/*-------------------------------------------------------------------------------
 * p25p1_tsbk.c
 * P25p1 Trunking Signal Block Handler
 *
 * LWVMOBILE
 * 2022-10 DSD-FME Florida Man Edition
 *-----------------------------------------------------------------------------*/

#include "dsd.h"

void processTSBK(dsd_opts * opts, dsd_state * state)
{

  //reset some strings when returning from a call in case they didn't get zipped already
  sprintf (state->call_string[0], "%s", "                     "); //21 spaces
  sprintf (state->call_string[1], "%s", "                     "); //21 spaces

  //clear stale Active Channel messages here
  if ( (time(NULL) - state->last_active_time) > 3 )
  {
    memset (state->active_channel, 0, sizeof(state->active_channel));
  }

  int tsbkbit[196]; //tsbk bit array, 196 trellis encoded bits
  uint8_t tsbk_dibit[98];

  memset (tsbkbit, 0, sizeof(tsbkbit));
  memset (tsbk_dibit, 0, sizeof(tsbk_dibit));

  int dibit = 0;

  uint8_t tsbk_byte[12]; //12 byte return from p25_12
  memset (tsbk_byte, 0, sizeof(tsbk_byte));

  unsigned long long int PDU[24]; //24 byte PDU to send to the tsbk_byte vPDU handler, should be same formats (mostly)
  memset (PDU, 0, sizeof(PDU));

  int tsbk_decoded_bits[190]; //decoded bits from tsbk_bytes for sending to crc16_lb_bridge
  memset (tsbk_decoded_bits, 0, sizeof(tsbk_decoded_bits));

  int i, j, k, x;
  int ec = -2; //error value returned from p25_12
  int err = -2; //error value returned from crc16_lb_bridge
  int skipdibit = 14; //initial status dibit will occur at 14, then add 36 each time it occurs
  int protectbit = 0;
  int MFID = 0xFF; //Manufacturer ID - Might be beneficial to NOT send anything but standard 0x00 or 0x01 messages
  int lb = 0; //last block

  //TODO: Don't Print TSBKs/vPDUs unless verbosity levels set higher

  //collect three reps of 101 dibits (98 valid dibits with status dibits interlaced)
  for (j = 0; j < 3; j++)
  {
    k = 0;
    for (i = 0; i < 101; i++)
    {

      dibit = getDibit(opts, state);

      if (i+(j*101) != skipdibit) //
      {
        tsbk_dibit[k] =   dibit;
        tsbkbit[k*2]   = (dibit >> 1) & 1;
        tsbkbit[k*2+1] = (dibit & 1);
        k++;
      }

      if (i+(j*101) == skipdibit) 
      {
        skipdibit += 36;
      }

    }

    //send to p25_12
    ec = p25_12 (tsbk_dibit, tsbk_byte);

    //debug err tally from 1/2 decoder
    // if (ec) fprintf (stderr, " #%d ERR = %d;", j+1, ec);

    //too many bit manipulations!
    k = 0;
    for (i = 0; i < 12; i++)
    {
      for (x = 0; x < 8; x++)
      {
        tsbk_decoded_bits[k] = ((tsbk_byte[i] << x) & 0x80) >> 7;
        k++;
      }
    }

    err = crc16_lb_bridge(tsbk_decoded_bits, 80);

    //shuffle corrected bits back into tsbk_byte
    k = 0;
    for (i = 0; i < 12; i++)
    {
      int byte = 0;
      for (x = 0; x < 8; x++)
      {
        byte = byte << 1;
        byte = byte | tsbk_decoded_bits[k];
        k++;
      }
      tsbk_byte[i] = byte;
    }

    //convert tsbk_byte to vPDU and send to vPDU handler
    //...may or may not be entirely compatible,
    MFID   = tsbk_byte[1];
    PDU[0] = 0x07; //P25p1 TSBK Duid 0x07
    PDU[1] = tsbk_byte[0] & 0x3F;
    PDU[2] = tsbk_byte[2];
    PDU[3] = tsbk_byte[3];
    PDU[4] = tsbk_byte[4];
    PDU[5] = tsbk_byte[5];
    PDU[6] = tsbk_byte[6];
    PDU[7] = tsbk_byte[7];
    PDU[8] = tsbk_byte[8];
    PDU[9] = tsbk_byte[9];
    //remove CRC to prevent false positive when vPDU goes to look for additional message in block
    PDU[10] = 0; //tsbk_byte[10]; 
    PDU[11] = 0; //tsbk_byte[11];
    PDU[1] = PDU[1] ^ 0x40; //flip bit to make it compatible with MAC_PDUs, i.e. 3D to 7D

    //check the protect bit, don't run if protected
    protectbit = (tsbk_byte[0] >> 6) & 0x1;
    lb         = (tsbk_byte[0] >> 7) & 0x1;

    //Don't run NET_STS out of this, or will set wrong NAC/CC //&& PDU[1] != 0x49 && PDU[1] != 0x45
    //0x49 is telephone grant, 0x46 Unit to Unit Channel Answer Request (seems bogus) (mfid 90)
    // if (MFID < 0x2 && protectbit == 0 && err == 0 && ec == 0 && PDU[1] != 0x7B )
    //running with A4 and 90 causing some false positives when sending to vPDU, issuing bad call grants to nowhere!
    // if ( (MFID < 0x2 || MFID == 0xA4 || MFID == 0x90) && protectbit == 0 && err == 0 && ec == 0 && PDU[1] != 0x7B )  
    if (MFID < 0x2 && protectbit == 0 && err == 0 && PDU[1] != 0x7B ) 
    {
      fprintf (stderr, "%s",KYEL);
      process_MAC_VPDU(opts, state, 0, PDU);
      fprintf (stderr, "%s",KNRM);
    }

    //look at Harris Opcodes and payload portion of TSBK
    else if (MFID == 0xA4 && protectbit == 0 && err == 0)
    {
      //TODO: Add Known Opcodes from Manual (all one of them)
      fprintf (stderr, "%s",KCYN);
      fprintf (stderr, "\n MFID A4 (Harris); Opcode: %02X; ", tsbk_byte[0] & 0x3F);
      for (i = 2; i < 10; i++)
        fprintf (stderr, "%02X", tsbk_byte[i]);
      fprintf (stderr, " %s",KNRM);

      //This seems to follow the same structure as the MFID 90 Group Regroup Add Command

      //MFID A4 Opcode 1 on TSBK / Opcode 0x41 on vPDU
      // P25 PDU Payload #1 [81][A4] [11][0D] [83][4E] [84][2A] [84][EE] -------[39][30]
      // MFID A4 Protected: 0 Last Block: 1

      //these values do seem to change, but not often, but seem to pair up into 16-bit values
      // P25 PDU Payload #1 [81][A4] [11][0D] [84][2A] [84][EE] [FF][FF] -------[4D][2F]
      // MFID A4 Protected: 0 Last Block: 1

    }

    //look at Motorola Opcodes and payload portion of TSBK
    else if (MFID == 0x90 && protectbit == 0 && err == 0)
    {
      //group list mode so we can look and see if we need to block tuning any groups, etc
      char mode[8]; //allow, block, digital, enc, etc
      sprintf (mode, "%s", "");

      //MFID 90 Group Regroup Channel Grant (MOT_GRG_CN_GRANT) TIA-102.AABH
      if ( (tsbk_byte[0] & 0x3F) == 0x02 )
      {
        int svc = tsbk_byte[2]; //Just the Res, P-bit, and more res bits
        int channel  = (tsbk_byte[3] << 8) | tsbk_byte[4];
        long int source = (tsbk_byte[7] << 16) |(tsbk_byte[8] << 8) | tsbk_byte[9];
        int group = (tsbk_byte[5] << 8) | tsbk_byte[6];
        long int freq1 = 0;
        UNUSED(source);
        fprintf (stderr, "%s\n ",KYEL);

        //unsure if this follows for GRG
        // if (svc & 0x80) fprintf (stderr, " Emergency"); 
        
        if (svc & 0x40) fprintf (stderr, " Encrypted"); //P-bit

        //unsure if this follows for GRG
        // if (opts->payload == 1) //hide behind payload due to len
        // {
        //   if (svc & 0x20) fprintf (stderr, " Duplex");
        //   if (svc & 0x10) fprintf (stderr, " Packet");
        //   else fprintf (stderr, " Circuit");
        //   if (svc & 0x8) fprintf (stderr, " R"); //reserved bit is on
        //   fprintf (stderr, " Priority %d", svc & 0x7); //call priority
        // }

        fprintf (stderr, " MFID90 Group Regroup Channel Grant");
        fprintf (stderr, "\n  SVC [%02X] CHAN [%04X] SG [%d][%04X]", svc, channel, group, group);
        freq1 = process_channel_to_freq (opts, state, channel);

        //add active channel to string for ncurses display
        sprintf (state->active_channel[0], "MFID90 Ch: %04X SG: %d ", channel, group);
        state->last_active_time = time(NULL);

        if (opts->trunk_use_allow_list == 1) sprintf (mode, "%s", "B");

        for (int i = 0; i < state->group_tally; i++)
        {
          if (state->group_array[i].groupNumber == group)
          {
            fprintf (stderr, " [%s]", state->group_array[i].groupName);
            strcpy (mode, state->group_array[i].groupMode);
            break;
          }
        }

        //TG hold on MFID90 GRG -- block non-matching target, allow matching group
        if (state->tg_hold != 0 && state->tg_hold != group) sprintf (mode, "%s", "B");
        if (state->tg_hold != 0 && state->tg_hold == group) sprintf (mode, "%s", "A");

        //Skip tuning group calls if group calls are disabled
        if (opts->trunk_tune_group_calls == 0) goto SKIPCALL;

        //Skip tuning encrypted calls if enc calls are disabled
        if ( (svc & 0x40) && opts->trunk_tune_enc_calls == 0) goto SKIPCALL;

        //tune if tuning available
        if (opts->p25_trunk == 1 && (strcmp(mode, "DE") != 0) && (strcmp(mode, "B") != 0))
        {
          //reworked to set freq once on any call to process_channel_to_freq, and tune on that, independent of slot
          if (state->p25_cc_freq != 0 && opts->p25_is_tuned == 0 && freq1 != 0) //if we aren't already on a VC and have a valid frequency
          {

            //changed to allow symbol rate change on C4FM Phase 2 systems as well as QPSK
            if (1 == 1)
            {
              if (state->p25_chan_tdma[channel >> 12] == 1)
              {
                state->samplesPerSymbol = 8;
                state->symbolCenter = 3;
              }
            }
            //rigctl
            if (opts->use_rigctl == 1)
            {
              if (opts->setmod_bw != 0 ) SetModulation(opts->rigctl_sockfd, opts->setmod_bw);
              SetFreq(opts->rigctl_sockfd, freq1);
              state->p25_vc_freq[0] = state->p25_vc_freq[1] = freq1;
              opts->p25_is_tuned = 1; //set to 1 to set as currently tuned so we don't keep tuning nonstop 
              state->last_vc_sync_time = time(NULL);
            }
            //rtl
            else if (opts->audio_in_type == 3)
            {
              #ifdef USE_RTLSDR
              rtl_dev_tune (opts, freq1);
              state->p25_vc_freq[0] = state->p25_vc_freq[1] = freq1;
              opts->p25_is_tuned = 1;
              state->last_vc_sync_time = time(NULL);
              #endif
            }
          }    
        }
      }

      else if ( (tsbk_byte[0] & 0x3F) == 0x03 )
      {
        //MFID 90 Group Regroup Channel Update (MOT_GRG_CN_GRANT_UPDT) TIA-102.AABH
        int channel1  = (tsbk_byte[2] << 8) | tsbk_byte[3];
        int group1 = (tsbk_byte[4] << 8) | tsbk_byte[5];
        int channel2  = (tsbk_byte[6] << 8) | tsbk_byte[7];
        int group2 = (tsbk_byte[8] << 8) | tsbk_byte[9];

        long int freq1 = 0;
        long int freq2 = 0;

        int tempg = 0; //temp group
        int tempc = 0; //temp chan
        long int tempf = 0; //temp freq

        fprintf (stderr, "%s\n ",KYEL);
        fprintf (stderr, " MFID90 Group Regroup Channel Grant Update");
        fprintf (stderr, "\n  CHAN1 [%04X] SG [%d][%04X] CHAN2 [%04X] SG [%d][%04X]", channel1, group1, group1, channel2, group2, group2);
        freq1 = process_channel_to_freq (opts, state, channel1);
        freq2 = process_channel_to_freq (opts, state, channel2);

        //add active channel to string for ncurses display
        sprintf (state->active_channel[0], "MFID90 Ch: %04X SG: %d; Ch: %04X SG: %d ", channel1, group1, channel2, group2);
        state->last_active_time = time(NULL);

        tempf = freq1;
        tempc = channel1;
        tempg = group1;

        for (int j = 0; j < 2; j++)
        {
          if (j == 1)
          {
            tempf = freq2;
            tempc = channel2;
            tempg = group2;
          }

          if (opts->trunk_use_allow_list == 1) sprintf (mode, "%s", "B");

          for (int i = 0; i < state->group_tally; i++)
          {
            if (state->group_array[i].groupNumber == tempg)
            {
              fprintf (stderr, " [%s]", state->group_array[i].groupName);
              strcpy (mode, state->group_array[i].groupMode);
              break;
            }
          }

          //TG hold on MFID90 GRG -- block non-matching target, allow matching group
          if (state->tg_hold != 0 && state->tg_hold != tempg) sprintf (mode, "%s", "B");
          if (state->tg_hold != 0 && state->tg_hold == tempg) sprintf (mode, "%s", "A");

          //Skip tuning group calls if group calls are disabled
          if (opts->trunk_tune_group_calls == 0) goto SKIPCALL;

          //Skip tuning encrypted calls if enc calls are disabled
          if (opts->trunk_tune_enc_calls == 0) goto SKIPCALL;

          //tune if tuning available
          if (opts->p25_trunk == 1 && (strcmp(mode, "DE") != 0) && (strcmp(mode, "B") != 0))
          {
            //reworked to set freq once on any call to process_channel_to_freq, and tune on that, independent of slot
            if (state->p25_cc_freq != 0 && opts->p25_is_tuned == 0 && tempf != 0) //if we aren't already on a VC and have a valid frequency
            {

              //changed to allow symbol rate change on C4FM Phase 2 systems as well as QPSK
              if (1 == 1)
              {
                if (state->p25_chan_tdma[tempc >> 12] == 1)
                {
                  state->samplesPerSymbol = 8;
                  state->symbolCenter = 3;
                }
              }
              //rigctl
              if (opts->use_rigctl == 1)
              {
                if (opts->setmod_bw != 0 ) SetModulation(opts->rigctl_sockfd, opts->setmod_bw);
                SetFreq(opts->rigctl_sockfd, tempf);
                state->p25_vc_freq[0] = state->p25_vc_freq[1] = tempf;
                opts->p25_is_tuned = 1; //set to 1 to set as currently tuned so we don't keep tuning nonstop 
                state->last_vc_sync_time = time(NULL);
              }
              //rtl
              else if (opts->audio_in_type == 3)
              {
                #ifdef USE_RTLSDR
                rtl_dev_tune (opts, tempf);
                state->p25_vc_freq[0] = state->p25_vc_freq[1] = tempf;
                opts->p25_is_tuned = 1;
                state->last_vc_sync_time = time(NULL);
                #endif
              }
            }    
          }
        }

      }
      else
      {
        fprintf (stderr, "%s",KCYN);
        fprintf (stderr, "\n MFID 90 (Moto); Opcode: %02X; ", tsbk_byte[0] & 0x3F);
        for (i = 2; i < 10; i++)
          fprintf (stderr, "%02X", tsbk_byte[i]);
        fprintf (stderr, " %s",KNRM);
      }

      SKIPCALL: ; //do nothing

    }

    //set our WACN and SYSID here now that we have valid ec and crc/checksum
    else if (protectbit == 0 && err == 0 && (tsbk_byte[0] & 0x3F) == 0x3B) 
    {
      long int wacn = (tsbk_byte[3] << 12) | (tsbk_byte[4] << 4) | (tsbk_byte[5] >> 4);
      int sysid = ((tsbk_byte[5] & 0xF) << 8) | tsbk_byte[6];
      int channel = (tsbk_byte[7] << 8) | tsbk_byte[8];
      fprintf (stderr, "%s",KYEL);
      fprintf (stderr, "\n Network Status Broadcast TSBK - Abbreviated \n");
      fprintf (stderr, "  WACN [%05lX] SYSID [%03X] NAC [%03llX]", wacn, sysid, state->p2_cc);
      state->p25_cc_freq = process_channel_to_freq(opts, state, channel);
      state->p25_cc_is_tdma = 0; //flag off for CC tuning purposes when system is qpsk

      //place the cc freq into the list at index 0 if 0 is empty, or not the same, 
      //so we can hunt for rotating CCs without user LCN list
      if (state->trunk_lcn_freq[0] == 0 || state->trunk_lcn_freq[0] != state->p25_cc_freq)
      {
        state->trunk_lcn_freq[0] = state->p25_cc_freq; 
      } 

      //only set IF these values aren't already hard set by the user
      if (state->p2_hardset == 0)
      {
        state->p2_wacn = wacn;
        state->p2_sysid = sysid;
      }  
        
      if (opts->payload == 1)
      {
        fprintf (stderr, "%s",KCYN);
        fprintf (stderr, "\n P25 PDU Payload #%d ", j);
        for (i = 0; i < 12; i++)
        {
          fprintf (stderr, "[%02X]", tsbk_byte[i]);
        }
      }

      fprintf (stderr, "%s ", KNRM);

    }

    else
    {
      if (opts->payload == 1)
      {
        fprintf (stderr, "%s",KCYN);
        fprintf (stderr, "\n P25 PDU Payload #%d ", j+1);
        for (i = 0; i < 12; i++)
        {
          fprintf (stderr, "[%02X]", tsbk_byte[i]);
        }
        fprintf (stderr, "\n MFID %02X Protected: %d Last Block: %d", MFID, protectbit, lb);
        
        if (ec != 0) 
        {
          fprintf (stderr, "%s",KRED);
          fprintf (stderr, " ERR = %d", ec);
        }
        if (err != 0) 
        {
          fprintf (stderr, "%s",KRED);
          fprintf (stderr, " (CRC ERR)");
        }
        fprintf (stderr, "%s ", KNRM);
      }
    } 

    //reset for next rep
    ec = -2;
    err = -2;
    protectbit = 0;
    MFID = 0xFF;

    //check for last block bit
    if (lb) break;
  }

  fprintf (stderr, "%s ", KNRM);
  fprintf (stderr, "\n"); 
}
