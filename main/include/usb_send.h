#pragma once

#include <stdio.h>
#include <stdint.h>

// int indexOf(char *str, char *pattern);
// char *substring(char *str, uint8_t start, uint8_t end);
#define MESSAGE_BUF_SIZE 256
void send_to_artisan(char *cmd, float ET, float BT);