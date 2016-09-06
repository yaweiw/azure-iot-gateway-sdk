// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothub_client_ll.h"
#include "iothubhttprelay.h"
#include "iothub_client.h"
#include "iothubtransport.h"
#include "iothubtransporthttp.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/vector.h"
#include "azure_c_shared_utility/iot_logging.h"
#include "azure_c_shared_utility/strings.h"
#include "messageproperties.h"
#include "message_bus.h"

#ifndef _LIGHT
#define LIGHT "LIGHT"
#endif

#ifndef _VIBRANT
#define VIBRANT "VIBRAT"
#endif

typedef struct IOTHUBHTTPRELAY_HANDLE_DATA_TAG
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    MESSAGE_BUS_HANDLE busHandle;
}IOTHUBHTTPRELAY_HANDLE_DATA;

static MODULE_HANDLE IoTHubHttpRelay_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    IOTHUBHTTPRELAY_HANDLE_DATA *result;
    if (
        (busHandle == NULL) ||
        (configuration == NULL) ||
        (((const IOTHUBHTTPRELAY_CONFIG*)configuration)->DeviceConnectionString == "NULL") // NULL
        )
    {
        LogError("invalid arg busHandle=%p, configuration=%p, IoTHubName=%s IoTHubSuffix=%s\r\n", busHandle, configuration, (configuration!=NULL)?((const IOTHUBHTTPRELAY_CONFIG*)configuration)->DeviceConnectionString:"undefined behavior");
        result = NULL;
    }
    else
    {
        const char* devConStr = ((const IOTHUBHTTPRELAY_CONFIG*)configuration)->DeviceConnectionString;
        result = malloc(sizeof(IOTHUBHTTPRELAY_HANDLE_DATA));
        if (result == NULL)
        {
            LogError("malloc returned NULL\r\n");
            /*return as is*/
        }
        else
        {
            result->iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(devConStr, HTTP_Protocol);
            if (result->iotHubClientHandle == NULL)
            {
                free(result);
                result = NULL;
                LogError("iotHubClientHandle returned NULL\r\n");
            }
            else
            {
                result->busHandle = busHandle;
            }
        }
    }
    return result;
}

static void IoTHubHttpRelay_Destroy(MODULE_HANDLE moduleHandle)
{
    if (moduleHandle == NULL)
    {
        LogError("moduleHandle parameter was NULL\r\n");
    }
    else
    {
        IOTHUBHTTPRELAY_HANDLE_DATA * handleData = moduleHandle;
        if(handleData->iotHubClientHandle != NULL)
        {
            IoTHubClient_LL_Destroy(handleData->iotHubClientHandle);
        }
        free(handleData);
    }
}

static IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromGWMessage(MESSAGE_HANDLE message)
{
    IOTHUB_MESSAGE_HANDLE result;
    const CONSTBUFFER* content = Message_GetContent(message);

    result = IoTHubMessage_CreateFromByteArray(content->buffer, content->size);
    if (result == NULL)
    {
        LogError("IoTHubMessage_CreateFromByteArray failed\r\n");
        /*return as is*/
    }
    else
    {
        MAP_HANDLE iothubMessageProperties = IoTHubMessage_Properties(result);
        CONSTMAP_HANDLE gwMessageProperties = Message_GetProperties(message);
        const char* const* keys;
        const char* const* values;
        size_t nProperties;
        if (ConstMap_GetInternals(gwMessageProperties, &keys, &values, &nProperties) != CONSTMAP_OK)
        {
            LogError("unable to get properties of the GW message\r\n");
            IoTHubMessage_Destroy(result);
            result = NULL;
        }
        else
        {
            size_t i;
            for (i = 0; i < nProperties; i++)
            {
                if (
                    (strcmp(keys[i], "deviceName") != 0) &&
                    (strcmp(keys[i], "deviceKey") != 0)
                    )
                {
                   
                    if (Map_AddOrUpdate(iothubMessageProperties, keys[i], values[i]) != MAP_OK)
                    {
                        LogError("unable to Map_AddOrUpdate\r\n");
                        break;
                    }
                }
            }

            if (i == nProperties)
            {
                /*all is fine, return as is*/
            }
            else
            {
                IoTHubMessage_Destroy(result);
                result = NULL;
            }
        }
        ConstMap_Destroy(gwMessageProperties);
    }
    return result;
}

static void IoTHubHttpRelay_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    if (
        (moduleHandle == NULL) ||
        (messageHandle == NULL)
        )
    {
        LogError("invalid arg moduleHandle=%p, messageHandle=%p\r\n", moduleHandle, messageHandle);
    }
    else
    {
        CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
        const char* light = ConstMap_GetValue(properties, LIGHT);
        if (light == NULL)
        {
            /*do nothing, not a message for this module*/
            printf("LIGHT is NULL\r\n");
        }
        else
        {
            const char* vibrant = ConstMap_GetValue(properties, VIBRANT);
            if (vibrant == NULL)
            {
                /*do nothing, incomplete properties*/
                printf("VIBRANT is NULL\r\n");
            }
            else
            {
                IOTHUBHTTPRELAY_HANDLE_DATA* moduleHandleData = moduleHandle;
                IOTHUB_MESSAGE_HANDLE iotHubMessage = IoTHubMessage_CreateFromGWMessage(messageHandle);
                if(iotHubMessage == NULL)
                {
                    LogError("unable to IoTHubMessage_CreateFromGWMessage (internal)\r\n");
                }
                else
                {
                    if (IoTHubClient_LL_SendEventAsync(moduleHandleData->iotHubClientHandle, iotHubMessage, NULL, NULL) != IOTHUB_CLIENT_OK)
                    {
                        LogError("unable to IoTHubClient_SendEventAsync\r\n");
                    }
                    else
                    {
                        LogInfo("all is fine, message has been accepted for delivery\r\n");
                    }
                    IoTHubClient_LL_DoWork(moduleHandleData->iotHubClientHandle);
                    ThreadAPI_Sleep(1000);
                    IoTHubMessage_Destroy(iotHubMessage);
                }
            }
        }
        ConstMap_Destroy(properties);
    }
}

static const MODULE_APIS Module_GetAPIS_Impl = 
{
    /*Codes_SRS_IOTHUBHTTP_02_026: [The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields.]*/
    IoTHubHttpRelay_Create,
    IoTHubHttpRelay_Destroy,
    IoTHubHttpRelay_Receive
};

/*Codes_SRS_IOTHUBHTTP_02_025: [Module_GetAPIS shall return a non-NULL pointer.]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTPRELAY_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    return &Module_GetAPIS_Impl;
}