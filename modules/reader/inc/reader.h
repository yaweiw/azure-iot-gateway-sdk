// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef READER_H
#define READER_H

#include "module.h"
#ifdef linux
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C"
{
#endif

MODULE_EXPORT const MODULE_APIS* MODULE_STATIC_GETAPIS(READER_MODULE)(void);

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


#endif /*READER_H*/
