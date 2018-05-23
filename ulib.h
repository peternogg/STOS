/************************
 * Author: Peter Higginbotham 
 * This file contains the external exports for ulib.c
 */

#pragma once

#include <machine_def.h>
#include <string.h>
#include "syscalls.h"

#define NULL 0
#define ERR -1

int exec(char* filename);
int exit();
int geti();
int gets(char* buff);
int halt();
int printi(int value);
int prints(char* string);
int yield();
