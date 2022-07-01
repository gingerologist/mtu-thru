/******************************************************************************

 @file  simple_gatt_profile.c

 @brief This file contains the Simple GATT profile sample GATT service profile
        for use with the BLE sample application.

 Group: WCS, BTS
 Target Device: cc2640r2

 ******************************************************************************
 
 Copyright (c) 2010-2021, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <icall.h>
#include "util.h"
/* This Header file contains all BLE API and icall structure definition */
#include "icall_ble_api.h"

#include "simple_gatt_profile.h"

/*********************************************************************
 * MACROS
 */
#define DEFINE_SERV_UUID(n) const uint8_t simpleProfileServ## n ##UUID[ATT_UUID_SIZE] = \
{ SIMPLEPROFILE_BASE_UUID_128(SIMPLEPROFILE_SERV## n ##_UUID) }

#define DEFINE_CHAR_UUID(n) const uint8_t simpleProfileChar## n ##UUID[ATT_UUID_SIZE] = \
{ SIMPLEPROFILE_BASE_UUID_128(SIMPLEPROFILE_CHAR## n ##_UUID) }

#define DEFINE_CHAR_ATTRS(n, props, value, desc) \
  static uint8_t simpleProfileChar## n ##Props= props; \
  static uint8_t simpleProfileChar## n [SIMPLEPROFILE_CHAR## n ##_LEN] = value; \
  static uint8_t simpleProfileChar## n ##UserDesp[] = desc;

#define DEFINE_CHAR_ATTRS_CFG(n, props, value, desc) \
  static uint8_t simpleProfileChar## n ##Props= props; \
  static uint8_t simpleProfileChar## n [SIMPLEPROFILE_CHAR## n ##_LEN] = value; \
  static gattCharCfg_t* simpleProfileChar## n ##Config = NULL; \
  static uint8_t simpleProfileChar## n ##UserDesp[] = desc;


#define INIT_CHAR_ATTRS(n, perm)                        \
    {                                                   \
      { ATT_BT_UUID_SIZE, characterUUID },              \
      GATT_PERMIT_READ,                                 \
      0,                                                \
      &simpleProfileChar## n ##Props                    \
    },                                                  \
    {                                                   \
      { ATT_UUID_SIZE, simpleProfileChar## n ##UUID },  \
      perm,                                             \
      0,                                                \
      simpleProfileChar##n                              \
    },                                                  \
    {                                                   \
      { ATT_BT_UUID_SIZE, charUserDescUUID },           \
      GATT_PERMIT_READ,                                 \
      0,                                                \
      simpleProfileChar## n ##UserDesp                  \
    }

#define INIT_CHAR_ATTRS_CFG(n, perm)                    \
    {                                                   \
      { ATT_BT_UUID_SIZE, characterUUID },              \
      GATT_PERMIT_READ,                                 \
      0,                                                \
      &simpleProfileChar## n ##Props                    \
    },                                                  \
    {                                                   \
      { ATT_UUID_SIZE, simpleProfileChar## n ##UUID },  \
      perm,                                             \
      0,                                                \
      simpleProfileChar##n                              \
    },                                                  \
    {                                                   \
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },          \
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,             \
      0,                                                \
      (uint8 *)&simpleProfileChar## n ##Config          \
    },                                                  \
    {                                                   \
      { ATT_BT_UUID_SIZE, charUserDescUUID },           \
      GATT_PERMIT_READ,                                 \
      0,                                                \
      simpleProfileChar## n ##UserDesp                  \
    }

#define SET_PARAM_CHAR(i)                               \
  if (len == SIMPLEPROFILE_CHAR## i ##_LEN) {           \
    VOID memcpy(&simpleProfileChar##i, value,           \
        SIMPLEPROFILE_CHAR##i##_LEN);                   \
  }                                                     \
  else                                                  \
  {                                                     \
    ret = bleInvalidRange;                              \
  }                                                     \

#define SET_PARAM_CHAR_CFG(i)                           \
  if (len == SIMPLEPROFILE_CHAR## i ##_LEN) {           \
    VOID memcpy(&simpleProfileChar##i,                  \
      value, SIMPLEPROFILE_CHAR## i ##_LEN);            \
    GATTServApp_ProcessCharCfg(                         \
            simpleProfileChar## i ##Config,             \
            (uint8_t*)&simpleProfileChar##i, FALSE,     \
            simpleProfileAttrTbl,                       \
            GATT_NUM_ATTRS( simpleProfileAttrTbl ),     \
            selfEntity, simpleProfile_ReadAttrCB );     \
  }                                                     \
  else                                                  \
  {                                                     \
    ret = bleInvalidRange;                              \
  }

#define GET_PARAM_CHAR(i)                               \
  VOID memcpy(value, simpleProfileChar5,                \
              SIMPLEPROFILE_CHAR5_LEN)


#define READ_ATTR(i)                                    \
    *pLen = SIMPLEPROFILE_CHAR## i ##_LEN;              \
    VOID memcpy(pValue, pAttr->pValue,                  \
                SIMPLEPROFILE_CHAR## i ## _LEN)

#define READ_ATTR_LONG(i)                               \
    if (offset + maxLen <=                              \
      sizeof(simpleProfileChar##i))                     \
    {                                                   \
      *pLen = maxLen;                                   \
    }                                                   \
    else                                                \
    {                                                   \
      *pLen = sizeof(simpleProfileChar##i) - offset;    \
    }                                                   \
    VOID memcpy(pValue, &((uint8_t *)(pAttr->pValue))[offset], *pLen)

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
DEFINE_SERV_UUID();
DEFINE_CHAR_UUID(1);
DEFINE_CHAR_UUID(2);
DEFINE_CHAR_UUID(3);
DEFINE_CHAR_UUID(4);
DEFINE_CHAR_UUID(5);

/*********************************************************************
 * EXTERNAL VARIABLES
 */

extern ICall_EntityID selfEntity;


/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static CONST gattAttrType_t simpleProfileService = { ATT_BT_UUID_SIZE, simpleProfileServUUID };

DEFINE_CHAR_ATTRS(1, GATT_PROP_READ | GATT_PROP_WRITE, {}, "Characteristic 1");
DEFINE_CHAR_ATTRS(2, GATT_PROP_READ, {}, "Characteristic 2");
DEFINE_CHAR_ATTRS(3, GATT_PROP_WRITE, {}, "Characteristic 3");
DEFINE_CHAR_ATTRS_CFG(4, 0, {}, "Characteristic 4");
DEFINE_CHAR_ATTRS(5, GATT_PROP_READ, {}, "Characteristic 5");

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[] =
{
  // Simple Profile Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&simpleProfileService            /* pValue */
  },
  INIT_CHAR_ATTRS(1, GATT_PERMIT_READ | GATT_PERMIT_WRITE),
  INIT_CHAR_ATTRS(2, GATT_PERMIT_READ),
  INIT_CHAR_ATTRS(3, GATT_PERMIT_WRITE),
  INIT_CHAR_ATTRS_CFG(4, 0),
  INIT_CHAR_ATTRS(5, GATT_PERMIT_AUTHEN_READ),
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method);
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Simple Profile Service Callbacks
// Note: When an operation on a characteristic requires authorization and
// pfnAuthorizeAttrCB is not defined for that characteristic's service, the
// Stack will report a status of ATT_ERR_UNLIKELY to the client.  When an
// operation on a characteristic requires authorization the Stack will call
// pfnAuthorizeAttrCB to check a client's authorization prior to calling
// pfnReadAttrCB or pfnWriteAttrCB, so no checks for authorization need to be
// made within these functions.
CONST gattServiceCBs_t simpleProfileCBs =
{
  simpleProfile_ReadAttrCB,  // Read callback function pointer
  simpleProfile_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService( uint32 services )
{
  uint8 status;

  // Allocate Client Characteristic Configuration table
  simpleProfileChar4Config = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
                                                            MAX_NUM_BLE_CONNS );
  if ( simpleProfileChar4Config == NULL )
  {
    return ( bleMemAllocError );
  }

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( CONNHANDLE_INVALID, simpleProfileChar4Config );

  if ( services & SIMPLEPROFILE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( simpleProfileAttrTbl,
                                          GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &simpleProfileCBs );
  }
  else
  {
    status = SUCCESS;
  }

  return ( status );
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    simpleProfile_AppCBs = appCallbacks;

    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch (param)
  {
    case SIMPLEPROFILE_CHAR1:
      SET_PARAM_CHAR(1);
      break;

    case SIMPLEPROFILE_CHAR2:
      SET_PARAM_CHAR(2);
      break;

    case SIMPLEPROFILE_CHAR3:
      SET_PARAM_CHAR(3);
      break;

    case SIMPLEPROFILE_CHAR4:
      SET_PARAM_CHAR_CFG(4);
      break;

    case SIMPLEPROFILE_CHAR5:
      SET_PARAM_CHAR(5);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
    }

  return ( ret );
}

/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR1:
      GET_PARAM_CHAR(1);
      break;

    case SIMPLEPROFILE_CHAR2:
      GET_PARAM_CHAR(2);
      break;

    case SIMPLEPROFILE_CHAR3:
      GET_PARAM_CHAR(3);
      break;

    case SIMPLEPROFILE_CHAR4:
      GET_PARAM_CHAR(4);
      break;

    case SIMPLEPROFILE_CHAR5:
      GET_PARAM_CHAR(5);
      break;

    default:
      ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @internal
 * @fn          simpleProfile_findCharParamId
 *
 * @brief       Find the logical param id of an attribute in the service's attr table.
 *
 *              Works only for Characteristic Value attributes and
 *              Client Characteristic Configuration Descriptor attributes.
 *
 * @param       pAttr - pointer to attribute
 *
 * @return      uint8_t paramID (ref button_service.h) or 0xFF if not found.
 */
static uint8_t simpleProfile_findCharParamId(gattAttribute_t *pAttr)
{
  // Is this a Client Characteristic Configuration Descriptor?
  if (ATT_BT_UUID_SIZE == pAttr->type.len &&
      GATT_CLIENT_CHAR_CFG_UUID == *(uint16_t*) pAttr->type.uuid)
  {
    // Assume the value attribute precedes CCCD and recurse
    return (simpleProfile_findCharParamId(pAttr - 1));
  }
  else if (ATT_UUID_SIZE == pAttr->type.len)
  {
    if (!memcmp(pAttr->type.uuid, simpleProfileChar1UUID, pAttr->type.len))
    {
      return SIMPLEPROFILE_CHAR1;
    }
    else if (!memcmp(pAttr->type.uuid, simpleProfileChar2UUID, pAttr->type.len))
    {
      return SIMPLEPROFILE_CHAR2;
    }
    else if (!memcmp(pAttr->type.uuid, simpleProfileChar3UUID, pAttr->type.len))
    {
      return SIMPLEPROFILE_CHAR3;
    }
    else if (!memcmp(pAttr->type.uuid, simpleProfileChar4UUID, pAttr->type.len))
    {
      return SIMPLEPROFILE_CHAR4;
    }
    else if (!memcmp(pAttr->type.uuid, simpleProfileChar5UUID, pAttr->type.len))
    {
      return SIMPLEPROFILE_CHAR5;
    }
  }

  return (0xFF); // Not found. Return invalid.
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method)
{
  bStatus_t status = SUCCESS;

  if ( pAttr->type.len == ATT_UUID_SIZE )
  {
    // Get 16-bit UUID from 128-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[12], pAttr->type.uuid[13]);
    switch ( uuid )
    {
      case SIMPLEPROFILE_CHAR1_UUID:
        READ_ATTR(1);
        break;

      case SIMPLEPROFILE_CHAR2_UUID:
        READ_ATTR(2);
        break;

      case SIMPLEPROFILE_CHAR4_UUID:
        READ_ATTR(4);
        break;

      case SIMPLEPROFILE_CHAR5_UUID:
        READ_ATTR(5);
        break;

      default:
        // Should never get here! (characteristics 3 and 4 do not have read permissions)
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 16-bit UUID, or invalid UUID length
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method)
{
  bStatus_t status = SUCCESS;
  bool notifyApp = false;

  if ( pAttr->type.len == ATT_BT_UUID_SIZE)
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch (uuid)
    {
      case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                 offset, GATT_CLIENT_CFG_NOTIFY | GATT_CLIENT_CFG_INDICATE);

        if (SUCCESS == status && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnCfgChangeCb)
        {
          simpleProfile_AppCBs->pfnCfgChangeCb(
            connHandle, simpleProfile_findCharParamId(pAttr), len, pValue);
        }
        break;

      default:
        // Should never get here!
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[12], pAttr->type.uuid[13]);
    switch ( uuid )
    {
      case SIMPLEPROFILE_CHAR1_UUID:
      case SIMPLEPROFILE_CHAR3_UUID:

        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len != 1 )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }

        //Write the value
        if ( status == SUCCESS )
        {
          uint8 *pCurValue = (uint8 *)pAttr->pValue;
          *pCurValue = pValue[0];
          notifyApp = true;
        }

        break;

      case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                 offset, GATT_CLIENT_CFG_NOTIFY );
        break;

      default:
        // Should never get here! (characteristics 2 and 4 do not have write permissions)
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }

  // If a characteristic value changed then callback function to notify application of change
  if ( notifyApp && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnValChangeCb )
  {
    simpleProfile_AppCBs->pfnValChangeCb(connHandle, simpleProfile_findCharParamId(pAttr), len, pValue);
  }

  return ( status );
}

/*********************************************************************
*********************************************************************/
