#include "usb_send.h"
#include <string.h>

int indexOf(char *str, char *pattern) {
  int i, j;
  int len_str = strlen(str);
  int len_pattern = strlen(pattern);

  for (i = 0; i < len_str - len_pattern + 1; i++) {
    for (j = 0; j < len_pattern; j++) {
      if (str[i + j] != pattern[j]) {
        break;
      }
    }
    if (j == len_pattern) {
      return i;
    }
  }
  return -1;
}

char* substring(char * str, uint8_t start, uint8_t end){
  if (start < 0 || end < 0 || start > end || strlen(str) < end) {
      return NULL;
  }

  char *sub_str = (char *)malloc(end - start + 1);

  if (sub_str == NULL) {
      return NULL;
  }
  for (int i = start; i < end; i++) {
      sub_str[i - start] = str[i];
  }
  sub_str[end - start] = '\0';

  return sub_str;
}


void send_to_artisan(char *cmd, float ET, float BT) {
  if(indexOf(cmd, "CHAN;") == 0){
      printf("#OK");
  } else if (indexOf(cmd, "UNITS;") == 0) {
      
      char *sub_str = substring(cmd, 7, 11);
      if(strcmp(sub_str, "F")){
          printf("#OK Farenheit\n");
      } else if(strcmp(sub_str, "C")) {
          printf("#OK Celsius\n");
      }
      free(sub_str);

  } else if (indexOf(cmd, "READ") == 0) {
                  //ET BT
      printf("0.00,%.2f,%.2f,00,0.00,0.00\n", ET, BT);
  }

  if(indexOf(cmd, "toglesy") == 0) {
      printf("%s", cmd);
      char *sub_str = substring(cmd, 2, 3);
      printf("Substr: %s\n", sub_str);
  }
  memset(cmd, 0, MESSAGE_BUF_SIZE);
}