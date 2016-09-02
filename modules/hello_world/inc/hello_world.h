// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HELLO_WORLD_H
#define HELLO_WORLD_H

#include "module.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
//#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include <cstdio>
#include <pthread.h>

#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(HELLOWORLD_MODULE)(void);

#ifdef __cplusplus
}
#endif

static const char* mmapFilePath = "/tmp/arduino";


typedef struct mmapData_TAG {
  int light;   // led on IO8
  int vibrant;  // built-in led
  pthread_mutex_t mutex;
  pthread_cond_t cond;
}MMAPDATA;

typedef MMAPDATA* MMAPDATA_HANDLE;


#endif /*HELLO_WORLD_H*/
