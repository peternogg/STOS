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

int prints(char* string);
int printi(int value);
int geti();
int gets(char* buff);
int halt();
int exit();
int exec(char* filename);
