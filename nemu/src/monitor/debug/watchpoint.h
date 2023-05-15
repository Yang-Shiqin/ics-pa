#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  uint32_t hit_time;
  char what[64];
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

WP* new_wp();
void free_wp(WP *wp);
void wp_display();

#endif
