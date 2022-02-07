/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        DAP.c CMSIS-DAP Commands
 *
 *---------------------------------------------------------------------------*/

#include "DAP.h"

// Get DAP Information
//   id:      info identifier
//   info:    pointer to info datas
//   return:  number of bytes in info datas
static UINT8I DAP_Info(UINT8 id, UINT8 *info)
{
    UINT8I length = 0U;

    switch (id)
    {
    case DAP_ID_VENDOR:
        length = 0;
        break;
    case DAP_ID_PRODUCT:
        length = 0;
        break;
    case DAP_ID_SER_NUM:
        length = 0;
        break;
    case DAP_ID_FW_VER:
        length = (UINT8)sizeof(DAP_FW_VER);
        memcpy(info, DAP_FW_VER, length);
        break;
    case DAP_ID_DEVICE_VENDOR:

        break;
    case DAP_ID_DEVICE_NAME:

        break;
    case DAP_ID_CAPABILITIES:
        info[0] = DAP_PORT_SWD;
        length = 1U;
        break;
    case DAP_ID_TIMESTAMP_CLOCK:

        break;
    case DAP_ID_SWO_BUFFER_SIZE:

        break;
    case DAP_ID_PACKET_SIZE:
        info[0] = (UINT8)(DAP_PACKET_SIZE >> 0);
        info[1] = (UINT8)(DAP_PACKET_SIZE >> 8);
        length = 2U;
        break;
    case DAP_ID_PACKET_COUNT:
        info[0] = DAP_PACKET_COUNT;
        length = 1U;
        break;
    default:
        break;
    }

    return (length);
}

// Process Delay command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_Delay(const UINT8 *req, UINT8 *res)
{
    UINT16I delay;

    delay = (UINT16)(*(req + 0)) |
            (UINT16)(*(req + 1) << 8);

    while (--delay)
    {
    };

    *res = DAP_OK;
    return 1;
}

// Process Host Status command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_HostStatus(const UINT8 *req, UINT8 *res)
{

    switch (*req)
    {
    case DAP_DEBUGGER_CONNECTED:
        DAP_LED_BUSY = ((*(req + 1) & 1U));
        break;
    case DAP_TARGET_RUNNING:
        DAP_LED_BUSY = ((*(req + 1) & 1U));
        break;
    default:
        *res = DAP_ERROR;
        return 1;
    }

    *res = DAP_OK;
    return 1;
}

// Process Connect command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
UINT8I debug_port;
static UINT8I DAP_Connect(const UINT8 *req, UINT8 *res)
{
    UINT8I port;

    if (*req == DAP_PORT_AUTODETECT)
    {
        port = DAP_DEFAULT_PORT;
    }
    else
    {
        port = *req;
    }

    switch (port)
    {
    case DAP_PORT_SWD:
        debug_port = DAP_PORT_SWD;
        PORT_SWD_SETUP();
        break;
    default:
        port = DAP_PORT_DISABLED;
        break;
    }

    *res = (UINT8)port;
    return 1;
}

// Process Disconnect command and prepare response
//   response: pointer to response datas
//   return:   number of bytes in response
static UINT8I DAP_Disconnect(UINT8 *res)
{
    debug_port = DAP_PORT_DISABLED;
    PORT_SWD_OFF();
    *res = DAP_OK;
    return (1U);
}

// Process Reset Target command and prepare response
//   response: pointer to response datas
//   return:   number of bytes in response
static UINT8I DAP_ResetTarget(UINT8 *res)
{
		SWD_ResetTarget_Soft();
    *(res + 1) = 0; 
    *(res + 0) = DAP_OK;
    return 2;
}

// Process SWJ Pins command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_SWJ_Pins(const UINT8 *req, UINT8 *res)
{
    UINT8I value;
    UINT8I select;
    UINT16I wait;

    value = (UINT8I) * (req + 0);
    select = (UINT8I) * (req + 1);
    wait = (UINT16I)(*(req + 2) << 0) | (UINT16I)(*(req + 3) << 8);
    if ((UINT8I)(*(req + 4)) || (UINT8I)(*(req + 5)))
        wait |= 0x8000;

    if ((select & DAP_SWJ_SWCLK_TCK_BIT) != 0U)
    {
        if ((value & DAP_SWJ_SWCLK_TCK_BIT) != 0U)
        {
            SWK = 1;
        }
        else
        {
            SWK = 0;
        }
    }
    if ((select & DAP_SWJ_SWDIO_TMS_BIT) != 0U)
    {
        if ((value & DAP_SWJ_SWDIO_TMS_BIT) != 0U)
        {
            SWD = 1;
        }
        else
        {
            SWD = 0;
        }
    }
    if ((select & DAP_SWJ_nRESET_BIT) != 0U)
    {
        if ((value & DAP_SWJ_SWDIO_TMS_BIT) != 0U)
        {
            RST = 0;
        }
        else
        {
						SWD_ResetTarget_Soft();
            RST = 1;
        }			
//        RST = value >> DAP_SWJ_nRESET;
    }

//    if (wait != 0U)
//    {

//        do
//        {
//            if ((select & DAP_SWJ_SWCLK_TCK_BIT) != 0U)
//            {
//                if ((value >> DAP_SWJ_SWCLK_TCK) ^ SWK)
//                {
//                    continue;
//                }
//            }
//            if ((select & DAP_SWJ_SWDIO_TMS_BIT) != 0U)
//            {
//                if ((value >> DAP_SWJ_SWDIO_TMS) ^ SWD)
//                {
//                    continue;
//                }
//            }
//            if ((select & DAP_SWJ_nRESET_BIT) != 0U)
//            {
//									//continue;
////                if ((value >> DAP_SWJ_nRESET) ^ RST)
////                {
////                    continue;
////                }
//            }
//            break;
//        }
//        while (wait--);
//    }

//    value = ((UINT8I)SWK << DAP_SWJ_SWCLK_TCK) |
//            ((UINT8I)SWD << DAP_SWJ_SWDIO_TMS) |
//            (0 << DAP_SWJ_TDI) |
//            (0 << DAP_SWJ_TDO) |
//            (0 << DAP_SWJ_nTRST) |
//            ((UINT8I)RST << DAP_SWJ_nRESET);
    value = ((UINT8I)SWK << DAP_SWJ_SWCLK_TCK) |
            ((UINT8I)SWD << DAP_SWJ_SWDIO_TMS) |
            (0 << DAP_SWJ_TDI) |
            (0 << DAP_SWJ_TDO) |
            (0 << DAP_SWJ_nTRST) |
            (0 << DAP_SWJ_nRESET);	
		if(RST == 0)	value|=(1 << DAP_SWJ_nRESET);

    *res = (UINT8)value;

    return 1;
}

// Process SWJ Clock command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
UINT8I fast_clock;
UINT8I clock_delay;
static UINT8I DAP_SWJ_Clock(const UINT8 *req, UINT8 *res)
{
    /**/
    fast_clock = *req;
    fast_clock = 0;
    clock_delay = 0;

    *res = DAP_OK;
    return 1;
}
// Process SWJ Sequence command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_SWJ_Sequence(const UINT8 *req, UINT8 *res)
{
    UINT8I count;

    count = *req++;
    if (count == 0U)
    {
        count = 255U;
    }
    SWJ_Sequence(count, req);
    *res = DAP_OK;
		

    return 1;
}

// Process SWD Configure command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
UINT8I turnaround;
UINT8I data_phase;
static UINT8I DAP_SWD_Configure(const UINT8 *req, UINT8 *res)
{
    UINT8I value;

    value = *req;
    turnaround = (value & 0x03U) + 1U;
    data_phase = (value & 0x04U) ? 1U : 0U;
		
    *res = DAP_OK;
    return 1;
}

// Process SWD Sequence command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)

/*
static UINT8I DAP_SWD_Sequence(const UINT8 *req, UINT8 *res)
{
    UINT8I sequence_info;
    UINT8I sequence_count;
    UINT8I request_count;
    UINT8I response_count;
    UINT8I count;

    *res++ = DAP_OK;

    request_count = 1U;
    response_count = 1U;

    sequence_count = *req++;
    while (sequence_count--)
    {
        sequence_info = *req++;
        count = sequence_info & SWD_SEQUENCE_CLK;
        if (count == 0U)
        {
            count = 64U;
        }
        count = (count + 7U) / 8U;
        if ((sequence_info & SWD_SEQUENCE_DIN) != 0U)
        {
            SWD = 1;//PIN_SWDIO_OUT_DISABLE();
        }
        else
        {
            SWD = 1;//PIN_SWDIO_OUT_ENABLE();
        }
        SWD_Sequence(sequence_info, req, res);
        if (sequence_count == 0U)
        {
            SWD = 1;//PIN_SWDIO_OUT_ENABLE();
        }
        if ((sequence_info & SWD_SEQUENCE_DIN) != 0U)
        {
            request_count++;
            res += count;
            response_count += count;
        }
        else
        {
            req += count;
            request_count += count + 1U;
        }
    }

    return response_count;
}
*/
static UINT8I DAP_SWD_Sequence(const UINT8 *req, UINT8 *res)
{
    UINT8I sequence_info;
    UINT8I sequence_count;
    //UINT8I request_count;
    UINT8I response_count;
    UINT8I count;

    *res++ = DAP_OK;

    //request_count = 1U;
    response_count = 1U;
    sequence_count = *req++;
    while (sequence_count--)
    {
        sequence_info = *req++;
        count = sequence_info & SWD_SEQUENCE_CLK;
        if (count == 0U)
        {
            count = 64U;
        }
				if ((sequence_info & SWD_SEQUENCE_DIN) != 0U)
				{//input
						SWD_IN_Sequence(count,res);
						count = (count + 7U) / 8U;
            //request_count++;
            res += count;
            response_count += count;
				}
				else
				{//output
						SWJ_Sequence(count,req);
						count = (count + 7U) / 8U;
						req += count;
						//request_count += count + 1U;
				}
    }
    return response_count;
}

// Process Transfer Configure command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
UINT8I idle_cycles;
UINT16I retry_count;
UINT16I match_retry;
static UINT8I DAP_TransferConfigure(const UINT8 *req, UINT8 *res)
{

    idle_cycles = *(req + 0);
    retry_count = (UINT16I) * (req + 1) |
                  (UINT16I)(*(req + 2) << 8);
    match_retry = (UINT16I) * (req + 3) |
                  (UINT16I)(*(req + 4) << 8);

    *res = DAP_OK;
    return 1;
}

// Process SWD Transfer command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)

UINT8I datas[4];
UINT8I match_mask[4];
UINT8I match_value[4];
UINT8I DAP_TransferAbort = 0U;
UINT8I request_count;
UINT8I request_value;
UINT8I response_count;
UINT8I response_value;
UINT16I retry;
static UINT8I DAP_SWD_Transfer(const UINT8 *req, UINT8 *res)
{
    const UINT8 *request_head;
    UINT8 *response_head;

    UINT8I post_read;
    UINT8I check_write;
    request_head = req;

    response_count = 0U;
    response_value = 0U;
    response_head = res;
    res += 2;

    DAP_TransferAbort = 0U;

    post_read = 0U;
    check_write = 0U;

    req++; // Ignore DAP index

    request_count = *req++;
    for (; request_count != 0U; request_count--)
    {
        request_value = *req++;
        if ((request_value & DAP_TRANSFER_RnW) != 0U)
        {
            // Read registers
            if (post_read)
            {
                // Read was posted before
                retry = retry_count;
                if ((request_value & (DAP_TRANSFER_APnDP | DAP_TRANSFER_MATCH_VALUE)) == DAP_TRANSFER_APnDP)
                {
                    // Read previous AP datas and post next AP read
                    do
                    {
                        response_value = SWD_Transfer(request_value, &datas);
                    }
                    while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                }
                else
                {
                    // Read previous AP datas
                    do
                    {
                        response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &datas);
                    }
                    while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                    post_read = 0U;
                }
                if (response_value != DAP_TRANSFER_OK)
                {
                    break;
                }
                // Store previous AP datas
                *res++ = (UINT8)datas[0];
                *res++ = (UINT8)datas[1];
                *res++ = (UINT8)datas[2];
                *res++ = (UINT8)datas[3];
            }
            if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U)
            {
                // Read with value match
                match_value[0] = (UINT8)(*(req + 0));
                match_value[1] = (UINT8)(*(req + 1));
                match_value[2] = (UINT8)(*(req + 2));
                match_value[3] = (UINT8)(*(req + 3));
                req += 4;
                match_retry = match_retry;
                if ((request_value & DAP_TRANSFER_APnDP) != 0U)
                {
                    // Post AP read
                    retry = retry_count;
                    do
                    {
                        response_value = SWD_Transfer(request_value, NULL);
                    }
                    while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                    if (response_value != DAP_TRANSFER_OK)
                    {
                        break;
                    }
                }
                do
                {
                    // Read registers until its value matches or retry counter expires
                    retry = retry_count;
                    do
                    {
                        response_value = SWD_Transfer(request_value, &datas);
                    }
                    while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                    if (response_value != DAP_TRANSFER_OK)
                    {
                        break;
                    }
                }
                while (((datas[0] & match_mask[0]) != match_value[0] ||
                        (datas[1] & match_mask[1]) != match_value[1] ||
                        (datas[2] & match_mask[2]) != match_value[2] ||
                        (datas[3] & match_mask[3]) != match_value[3]) &&
                        match_retry-- && !DAP_TransferAbort);
                if ((datas[0] & match_mask[0]) != match_value[0] ||
                        (datas[1] & match_mask[1]) != match_value[1] ||
                        (datas[2] & match_mask[2]) != match_value[2] ||
                        (datas[3] & match_mask[3]) != match_value[3])
                {
                    response_value |= DAP_TRANSFER_MISMATCH;
                }
                if (response_value != DAP_TRANSFER_OK)
                {
                    break;
                }
            }
            else
            {
                // Normal read
                retry = retry_count;
                if ((request_value & DAP_TRANSFER_APnDP) != 0U)
                {
                    // Read AP registers
                    if (post_read == 0U)
                    {
                        // Post AP read
                        do
                        {
                            response_value = SWD_Transfer(request_value, NULL);
                        }
                        while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                        if (response_value != DAP_TRANSFER_OK)
                        {
                            break;
                        }
                        post_read = 1U;
                    }
                }
                else
                {
                    // Read DP register
                    do
                    {
                        response_value = SWD_Transfer(request_value, &datas);
                    }
                    while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                    if (response_value != DAP_TRANSFER_OK)
                    {
                        break;
                    }

                    // Store datas
                    *res++ = datas[0];
                    *res++ = datas[1];
                    *res++ = datas[2];
                    *res++ = datas[3];
                }
            }
            check_write = 0U;
        }
        else
        {
            // Write register
            if (post_read)
            {
                // Read previous datas
                retry = retry_count;
                do
                {
                    response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &datas);
                }
                while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                if (response_value != DAP_TRANSFER_OK)
                {
                    break;
                }
                // Store previous datas
                *res++ = datas[0];
                *res++ = datas[1];
                *res++ = datas[2];
                *res++ = datas[3];
                post_read = 0U;
            }
            // Load datas
            datas[0] = (UINT8)(*(req + 0));
            datas[1] = (UINT8)(*(req + 1));
            datas[2] = (UINT8)(*(req + 2));
            datas[3] = (UINT8)(*(req + 3));
            req += 4;
            if ((request_value & DAP_TRANSFER_MATCH_MASK) != 0U)
            {
                // Write match mask
                match_mask[0] = datas[0];
                match_mask[1] = datas[1];
                match_mask[2] = datas[2];
                match_mask[3] = datas[3];
                response_value = DAP_TRANSFER_OK;
            }
            else
            {
                // Write DP/AP register
                retry = retry_count;
                do
                {
                    response_value = SWD_Transfer(request_value, &datas);
                }
                while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                if (response_value != DAP_TRANSFER_OK)
                {
                    break;
                }

                check_write = 1U;
            }
        }
        response_count++;
        if (DAP_TransferAbort)
        {
            break;
        }
    }

    for (; request_count != 0U; request_count--)
    {
        // Process canceled requests
        request_value = *req++;
        if ((request_value & DAP_TRANSFER_RnW) != 0U)
        {
            // Read register
            if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U)
            {
                // Read with value match
                req += 4;
            }
        }
        else
        {
            // Write register
            req += 4;
        }
    }

    if (response_value == DAP_TRANSFER_OK)
    {
        if (post_read)
        {
            // Read previous datas
            retry = retry_count;
            do
            {
                response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &datas);
            }
            while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK)
            {
                goto end;
            }
            // Store previous datas
            *res++ = (UINT8)datas[0];
            *res++ = (UINT8)datas[1];
            *res++ = (UINT8)datas[2];
            *res++ = (UINT8)datas[3];
        }
        else if (check_write)
        {
            // Check last write
            retry = retry_count;
            do
            {
                response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
            }
            while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
        }
    }

end:
    *(response_head + 0) = (UINT8)response_count;
    *(response_head + 1) = (UINT8)response_value;

    return ((UINT8I)(res - response_head));
}

// Process Dummy Transfer command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
//static UINT8 DAP_Dummy_Transfer(const UINT8 *req, UINT8 *res)
//{
//  UINT8 *request_head;
//  UINT8 request_count;
//  UINT8 request_value;

//  request_head = req;

//  req++; // Ignore DAP index

//  request_count = *req++;

//  for (; request_count != 0U; request_count--)
//  {
//    // Process dummy requests
//    request_value = *req++;
//    if ((request_value & DAP_TRANSFER_RnW) != 0U)
//    {
//      // Read registers
//      if ((request_value & DAP_TRANSFER_MATCH_VALUE) != 0U)
//      {
//        // Read with value match
//        req += 4;
//      }
//    }
//    else
//    {
//      // Write registers
//      req += 4;
//    }
//  }

//  *(res + 0) = 0U; // res count
//  *(res + 1) = 0U; // res value

//  return 2;
//}

// Process Transfer command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_Transfer(const UINT8 *req, UINT8 *res)
{
    UINT8I num = 0;

//  switch (debug_port)
//  {
//  case DAP_PORT_SWD:
    num = DAP_SWD_Transfer(req, res);
//    break;
//  default:
//    num = DAP_Dummy_Transfer(req, res);
//    break;
//  }

    return (num);
}

// Process SWD Transfer Block command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response
static UINT8I DAP_SWD_TransferBlock(const UINT8 *req, UINT8 *res)
{

    UINT8 *response_head;
	
    response_count = 0U;
    response_value = 0U;
    response_head = res;
    res += 3;

    DAP_TransferAbort = 0U;

    req++; // Ignore DAP index

    request_count = (UINT16)(*(req + 0) << 0) |
                    (UINT16)(*(req + 1) << 8);
    req += 2;
    if (request_count == 0U)
    {
        goto end;
    }

    request_value = *req++;
    if ((request_value & DAP_TRANSFER_RnW) != 0U)
    {
        // Read register block
        if ((request_value & DAP_TRANSFER_APnDP) != 0U)
        {
            // Post AP read
            retry = retry_count;
            do
            {
                response_value = SWD_Transfer(request_value, NULL);
            }
            while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK)
            {
                goto end;
            }
        }
        while (request_count--)
        {
            // Read DP/AP register
            if ((request_count == 0U) && ((request_value & DAP_TRANSFER_APnDP) != 0U))
            {
                // Last AP read
                request_value = DP_RDBUFF | DAP_TRANSFER_RnW;
            }
            retry = retry_count;
            do
            {
                response_value = SWD_Transfer(request_value, &datas);
            }
            while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK)
            {
                goto end;
            }
            // Store datas
            *res++ = (UINT8)datas[0];
            *res++ = (UINT8)datas[1];
            *res++ = (UINT8)datas[2];
            *res++ = (UINT8)datas[3];
            response_count++;
        }
    }
    else
    {
        // Write register block
        while (request_count--)
        {
            // Load datas
            datas[0] = (UINT8)(*(req + 0));
            datas[1] = (UINT8)(*(req + 1));
            datas[2] = (UINT8)(*(req + 2));
            datas[3] = (UINT8)(*(req + 3));

            req += 4;
            // Write DP/AP register
            retry = retry_count;
            do
            {
                response_value = SWD_Transfer(request_value, &datas);
            }
            while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK)
            {
                goto end;
            }
            response_count++;
        }
        // Check last write
        retry = retry_count;
        do
        {
            response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, NULL);
        }
        while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
    }

end:
    *(response_head + 0) = (UINT8)(response_count >> 0);
    *(response_head + 1) = (UINT8)(response_count >> 8);
    *(response_head + 2) = (UINT8)response_value;

    return ((UINT8I)(res - response_head));
}

// Process Transfer Block command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_TransferBlock(const UINT8 *req, UINT8 *res)
{
    UINT8I num;

    switch (debug_port)
    {
    case DAP_PORT_SWD:
        num = DAP_SWD_TransferBlock(req, res);
        break;

    default:
        *(res + 0) = 0U; // res count [7:0]
        *(res + 1) = 0U; // res count[15:8]
        *(res + 2) = 0U; // res value
        num = 3U;
        break;
    }

    if ((*(req + 3) & DAP_TRANSFER_RnW) != 0U)
    {
        // Read registers block
        num |= 4U << 16;
    }
    else
    {
        // Write registers block
        num |= (4U + (((UINT8I)(*(req + 1)) | (UINT8I)(*(req + 2) << 8)) * 4)) << 16;
    }

    return (num);
}

// Process SWD Write ABORT command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response
static UINT8I DAP_SWD_WriteAbort(const UINT8 *req, UINT8 *res)
{
    // Load datas (Ignore DAP index)
    datas[0] = (UINT8I)(*(req + 1));
    datas[1] = (UINT8I)(*(req + 2));
    datas[2] = (UINT8I)(*(req + 3));
    datas[3] = (UINT8I)(*(req + 4));

    // Write Abort register
    SWD_Transfer(DP_ABORT, &datas);

    *res = DAP_OK;
    return (1U);
}

// Process Write ABORT command and prepare response
//   request:  pointer to request datas
//   response: pointer to response datas
//   return:   number of bytes in response (lower 16 bits)
//             number of bytes in request (upper 16 bits)
static UINT8I DAP_WriteAbort(const UINT8 *req, UINT8 *res)
{
    UINT8I num;

    switch (debug_port)
    {
    case DAP_PORT_SWD:
        num = DAP_SWD_WriteAbort(req, res);
        break;

    default:
        *res = DAP_ERROR;
        num = 1U;
        break;
    }
    return num;
}

// DAP Thread.
void DAP_Thread(void)
{
    UINT8I num;

    if (Ep2Oi != Ep2Oo)
    {
        PUINT8 req = &Ep2BufferO[Ep2Oo];
        PUINT8 res = &Ep3BufferI[Ep3Ii];
        Ep2Oo += 64;


        *res++ = *req;
        switch (*req++)
        {
        case ID_DAP_Transfer:
            num = DAP_Transfer(req, res);
            break;

        case ID_DAP_TransferBlock:
            num = DAP_TransferBlock(req, res);
            break;

        case ID_DAP_Info:
            num = DAP_Info(*req, res + 1);
            *res = (UINT8)num;
            num++;
            break;

        case ID_DAP_HostStatus:
            num = DAP_HostStatus(req, res);
            break;

        case ID_DAP_Connect:
            num = DAP_Connect(req, res);
            break;

        case ID_DAP_Disconnect:
            num = DAP_Disconnect(res);
            break;

        case ID_DAP_Delay:
            num = DAP_Delay(req, res);
            break;

        case ID_DAP_ResetTarget:
            num = DAP_ResetTarget(res);
            break;

        case ID_DAP_SWJ_Pins:
            num = DAP_SWJ_Pins(req, res);
            break;

        case ID_DAP_SWJ_Clock:
            num = DAP_SWJ_Clock(req, res);
            break;

        case ID_DAP_SWJ_Sequence:
            num = DAP_SWJ_Sequence(req, res);
            break;

        case ID_DAP_SWD_Configure:
            num = DAP_SWD_Configure(req, res);
            break;

        case ID_DAP_SWD_Sequence:
            num = DAP_SWD_Sequence(req, res);
            break;

        case ID_DAP_TransferConfigure:
            num = DAP_TransferConfigure(req, res);
            break;

        case ID_DAP_WriteABORT:
            num = DAP_WriteAbort(req, res);
            break;
        
        case ID_DAP_ExecuteCommands:
        case ID_DAP_QueueCommands:
        default:
            *(res - 1) = ID_DAP_Invalid;
            num = 1;
            break;
        }

        Ep3Is[0]/*(Ep3Ii>>6)]*/ = num + 1;
        Ep3Ii += 64;
    }
}
