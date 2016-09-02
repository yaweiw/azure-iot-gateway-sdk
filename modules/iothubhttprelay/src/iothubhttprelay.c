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
#include "schemaserializer.h"
#include "messageproperties.h"
#include "message_bus.h"

#define LIGHT light
#define VIBRANT vibrant

typedef struct IOTHUBHTTPRELAY_HANDLE_DATA_TAG
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    MESSAGE_BUS_HANDLE busHandle;
}IOTHUBHTTPRELAY_HANDLE_DATA;

static MODULE_HANDLE IoTHubHttpRelay_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    LogInfo("Enter IoTHubHttpRelay_Create\r\n");
    IOTHUBHTTPRELAY_HANDLE_DATA *result;
    /*Codes_SRS_IOTHUBHTTP_02_001: [If busHandle is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_002: [If configuration is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_003: [If configuration->IoTHubName is NULL then IoTHubHttp_Create shall and return NULL.]*/
    /*Codes_SRS_IOTHUBHTTP_02_004: [If configuration->IoTHubSuffix is NULL then IoTHubHttp_Create shall fail and return NULL.]*/
    if (
        (busHandle == NULL) ||
        (configuration == NULL) ||
        (((const IOTHUBHTTPRELAY_CONFIG*)configuration)->DeviceConnectionString == NULL)
        )
    {
        LogError("invalid arg busHandle=%p, configuration=%p, IoTHubName=%s IoTHubSuffix=%s\r\n", busHandle, configuration, (configuration!=NULL)?((const IOTHUBHTTP_CONFIG*)configuration)->DeviceConnectionString:"undefined behavior");
        result = NULL;
    }
    else
    {
        const char* devConStr = ((const IOTHUBHTTPRELAY_CONFIG*)configuration)->DeviceConnectionString;
        if(serializer_init(NULL) != SERIALIZER_OK)
        {
            LogInfo("Failed on serializer_init\r\n");
        }
        else
        {
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
    }
    return result;
}

static void IoTHubHttp_Destroy(MODULE_HANDLE moduleHandle)
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
            /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
            LogError("unable to get properties of the GW message\r\n");
            IoTHubMessage_Destroy(result);
            result = NULL;
        }
        else
        {
            size_t i;
            for (i = 0; i < nProperties; i++)
            {
                /*add all the properties of the GW message to the IOTHUB message*/ /*with the exception*/
                /*Codes_SRS_IOTHUBHTTP_02_018: [IoTHubHttp_Receive shall create a new IOTHUB_MESSAGE_HANDLE having the same content as the messageHandle and same properties with the exception of deviceName and deviceKey properties.]*/
                if (
                    (strcmp(keys[i], "deviceName") != 0) &&
                    (strcmp(keys[i], "deviceKey") != 0)
                    )
                {
                   
                    if (Map_AddOrUpdate(iothubMessageProperties, keys[i], values[i]) != MAP_OK)
                    {
                        /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
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
                /*Codes_SRS_IOTHUBHTTP_02_019: [If creating the IOTHUB_MESSAGE_HANDLE fails, then IoTHubHttp_Receive shall return.]*/
                IoTHubMessage_Destroy(result);
                result = NULL;
            }
        }
        ConstMap_Destroy(gwMessageProperties);
    }
    return result;
}

static void IoTHubHttp_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBHTTP_02_009: [If moduleHandle or messageHandle is NULL then IoTHubHttp_Receive shall do nothing.]*/
    if (
        (moduleHandle == NULL) ||
        (messageHandle == NULL)
        )
    {
        LogError("invalid arg moduleHandle=%p, messageHandle=%p\r\n", moduleHandle, messageHandle);
        /*do nothing*/
    }
    else
    {
        CONSTMAP_HANDLE properties = Message_GetProperties(messageHandle);
        const char* source = ConstMap_GetValue(properties, SOURCE); /*properties is !=NULL by contract of Message*/

        /*Codes_SRS_IOTHUBHTTP_02_010: [If message properties do not contain a property called "source" having the value set to "mapping" then IoTHubHttp_Receive shall do nothing.]*/
        if (
            (source == NULL) ||
            (strcmp(source, MAPPING)!=0)
            )
        {
            /*do nothing, the properties do not contain either "source" or "source":"mapping"*/
            LogError("properties do not contain source or source:mapping\r\n");
        }
        else
        {
            /*Codes_SRS_IOTHUBHTTP_02_011: [If message properties do not contain a property called "deviceName" having a non-NULL value then IoTHubHttp_Receive shall do nothing.]*/
            const char* light = ConstMap_GetValue(properties, LIGHT);
            if (light == NULL)
            {
                /*do nothing, not a message for this module*/
            }
            else
            {
                /*Codes_SRS_IOTHUBHTTP_02_012: [If message properties do not contain a property called "deviceKey" having a non-NULL value then IoTHubHttp_Receive shall do nothing.]*/
                const char* vibrant = ConstMap_GetValue(properties, VIBRANT);
                if (vibrant == NULL)
                {
                    /*do nothing, incomplete properties*/
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
                        IoTHubClient_LL_DoWork(iotHubClientHandle);
                        ThreadAPI_Sleep(1000);
                        IoTHubMessage_Destroy(iotHubMessage);
                    }
                }
            }
        }
    }
    ConstMap_Destroy(properties);
    /*Codes_SRS_IOTHUBHTTP_02_022: [IoTHubHttp_Receive shall return.]*/
}

static const MODULE_APIS Module_GetAPIS_Impl = 
{
    /*Codes_SRS_IOTHUBHTTP_02_026: [The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields.]*/
    IoTHubHttp_Create,
    IoTHubHttp_Destroy,
    IoTHubHttp_Receive
};

/*Codes_SRS_IOTHUBHTTP_02_025: [Module_GetAPIS shall return a non-NULL pointer.]*/
#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTP_MODULE)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    return &Module_GetAPIS_Impl;
}