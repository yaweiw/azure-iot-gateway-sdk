// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "azure_c_shared_utility/threadapi.h"
#include "gateway.h"

int main(int argc, char** argv)
{
    GATEWAY_HANDLE gateway;
    if (argc != 2)
    {
        printf("usage: device_ping configFile\n");
        printf("where configFile is the name of the file that contains the Gateway configuration\n");
    }
    else
    {
        if ((gateway = Gateway_Create_From_JSON(argv[1])) == NULL) //(argc !=2 )//
        {
            printf("failed to create the gateway from JSON\n");
        }
        else
        {
            printf("gateway successfully created from JSON\n");
            //ThreadAPI_Sleep(4000);
            (void)getchar();
            Gateway_LL_Destroy(gateway);
        }
    }
	return 0;
}