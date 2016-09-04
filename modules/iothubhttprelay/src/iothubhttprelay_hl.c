// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "azure_c_shared_utility/gballoc.h"

#include "iothubhttprelay_hl.h"
#include "iothubhttprelay.h"
#include "azure_c_shared_utility/iot_logging.h"
#include "parson.h"

#define LIGHT light
#define VIBRANT vibrant

#define DEVICECONNECTIONSTRING DeviceConnectionString

static MODULE_HANDLE IoTHubHttpRelay_HL_Create(MESSAGE_BUS_HANDLE busHandle, const void* configuration)
{
    MODULE_HANDLE *result;
    if ((busHandle == NULL) || (configuration == NULL))
    {
        /*Codes_SRS_IOTHUBHTTP_HL_17_001: [If busHandle is NULL then IoTHubHttp_HL_Create shall fail and return NULL.]*/
        /*Codes_SRS_IOTHUBHTTP_HL_17_002: [If configuration is NULL then IoTHubHttp_HL_Create shall fail and return NULL.]*/
        LogError("Invalid NULL parameter, busHandle=[%p] configuration=[%p]", busHandle, configuration);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_IOTHUBHTTP_HL_17_004: [ IoTHubHttp_HL_Create shall parse the configuration as a JSON string. ]*/
        JSON_Value* json = json_parse_string((const char*)configuration);
        if (json == NULL)
        {
            /*Codes_SRS_IOTHUBHTTP_HL_17_003: [ If configuration is not a JSON string, then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
            LogError("Unable to parse json string");
            result = NULL;
        }
        else
        {
            JSON_Object* obj = json_value_get_object(json);
            if (obj == NULL)
            {
                /*Codes_SRS_IOTHUBHTTP_HL_17_005: [ If parsing configuration fails, IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                LogError("Expected a JSON Object in configuration");
                result = NULL;
            }
            else
            {
                const char * DeviceConnectionString;
                if ((DeviceConnectionString = json_object_get_string(obj, DEVICECONNECTIONSTRING)) == NULL)
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_006: [ If the JSON object does not contain a value named "IoTHubName" then IoTHubHttp_HL_Create shall fail and return NULL. ]*/
                    LogError("Did not find expected %s configuration", DEVICECONNECTIONSTRING);
                    result = NULL;
                }
                else
                {
                    /*Codes_SRS_IOTHUBHTTP_HL_17_008: [ IoTHubHttp_HL_Create shall invoke iothubhttp Module's create, using the busHandle, IotHubName, and IoTHubSuffix. ]*/
                    IOTHUBHTTPRELAY_CONFIG llConfiguration;
                    llConfiguration.DeviceConnectionString = DeviceConnectionString;
                    result = MODULE_STATIC_GETAPIS(IOTHUBHTTPRELAY_MODULE)()->Module_Create(busHandle, &llConfiguration);
                }
            }
            json_value_free(json);
        }
    }
    return result;
}

static void IoTHubHttpRelay_HL_Destroy(MODULE_HANDLE moduleHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_012: [ IoTHubHttp_HL_Destroy shall free all used resources. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBHTTPRELAY_MODULE)()->Module_Destroy(moduleHandle);
}


static void IoTHubHttp_HL_Receive(MODULE_HANDLE moduleHandle, MESSAGE_HANDLE messageHandle)
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_011: [ IoTHubHttp_HL_Receive shall pass the received parameters to the underlying IoTHubHttp receive function. ]*/
    MODULE_STATIC_GETAPIS(IOTHUBHTTPRELAY_MODULE)()->Module_Receive(moduleHandle, messageHandle);
}

/*Codes_SRS_IOTHUBHTTP_HL_17_014: [ The MODULE_APIS structure shall have non-NULL Module_Create, Module_Destroy, and Module_Receive fields. ]*/
static const MODULE_APIS IoTHubHttpRelay_HL_GetAPIS_Impl =
{
    IoTHubHttpRelay_HL_Create,
    IoTHubHttpRelay_HL_Destroy,
    IoTHubHttpRelay_HL_Receive
};


#ifdef BUILD_MODULE_TYPE_STATIC
MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(IOTHUBHTTPRELAY_MODULE_HL)(void)
#else
MODULE_EXPORT const MODULE_APIS* Module_GetAPIS(void)
#endif
{
    /*Codes_SRS_IOTHUBHTTP_HL_17_013: [ Module_GetAPIS shall return a non-NULL pointer. ]*/
    return &IoTHubHttpRelay_HL_GetAPIS_Impl;
}