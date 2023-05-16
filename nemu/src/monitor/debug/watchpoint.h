#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  uint32_t hit_time;
  uint32_t addr;
  uint32_t last_val;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

WP* new_wp();
void free_wp(uint32_t NO);
void wp_display();
bool check_wp();

#endif
