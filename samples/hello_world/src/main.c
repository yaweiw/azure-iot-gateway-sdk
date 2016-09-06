// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#include "gateway.h"

#include "mmap.h"

int main(int argc, char** argv)
{
    MMAPDATA_HANDLE p_mmapData; // here our mmapped data will be accessed
    int fd_mmapFile; // file descriptor for memory mapped file

    /* Create shared memory object and set its size */
    fd_mmapFile = open(mmapFilePath, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_mmapFile == -1) printf("fd error; check errno for details\n");
    
    /* Map shared memory object read-writable */
    p_mmapData = (MMAPDATA_HANDLE)(mmap(NULL, sizeof(MMAPDATA), PROT_READ | PROT_WRITE, MAP_SHARED, fd_mmapFile, 0));
    if (p_mmapData == MAP_FAILED) printf("mmap error\n");

    GATEWAY_HANDLE gateway;
    if (argc != 2)
    {
        printf("usage: hello_world_sample configFile\n");
        printf("where configFile is the name of the file that contains the Gateway configuration\n");
    }
    else
    {
        if (argc !=2 )//((gateway = Gateway_Create_From_JSON(argv[1])) == NULL)
        {
            printf("failed to create the gateway from JSON\n");
        }
        else
        {
            printf("gateway successfully created from JSON\n");
            printf("gateway shall run until ENTER is pressed\n");
            //(void)getchar();

            while(getchar() != 'e')
            {
                if (pthread_mutex_lock(&(p_mmapData->mutex)) != 0) printf("pthread_mutex_lock err\n");
                if (pthread_cond_wait(&(p_mmapData->cond), &(p_mmapData->mutex)) != 0) printf("pthread_cond_wait err\n");
                // signal to waiting thread
                printf("p_mmapData->light = %d\r\n", p_mmapData->light);
                printf("p_mmapData->vibrant = %d\r\n", p_mmapData->vibrant);
                if (pthread_mutex_unlock(&(p_mmapData->mutex)) != 0) printf("pthread_mutex_unlock err\n");
            }

            Gateway_LL_Destroy(gateway);
        }
    }
	return 0;
}
