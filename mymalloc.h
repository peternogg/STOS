#pragma once

void my_mem_init(void *ptr, int size);
void *my_malloc(int size);
void my_free(void *ptr);
void *my_get_largest(int *size);
void *my_set_limit(void *base, void *limit);
int my_validate();
void my_print_mem();

