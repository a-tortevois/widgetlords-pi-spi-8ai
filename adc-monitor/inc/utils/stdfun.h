//
// Created by Alexandre Tortevois

#ifndef H_STDFUN_H
#define H_STDFUN_H

#include <stdbool.h>
#include <stdio.h>

int dir_exists(char *path);
int fifo_push(int arr[], size_t size, int *pos, int value);

// libjson -->
int strtoint(const char *str, int *value);
#ifndef isxdigit
int isxdigit(int c);
#endif
int atoh(char c, int *i);
int uc_to_utf8(int uc, unsigned char *dest);
// <-- libjson

#endif //H_STDFUN_H
