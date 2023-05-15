#include "watchpoint.h"
#include "expr.h"

#define NR_WP 32    // 多于32会assert(0)

static WP wp_pool[NR_WP] = {};  // 池的形式保存
static WP *head = NULL, *free_ = NULL;  // 两个链表：head组织使用中的监视点结构；free_组织空闲的监视点结构.head倒着，free_正着

void init_wp_pool() { // 对head和free_初始化
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    // TODO: init new member
    wp_pool[i].hit_time = 0;
    wp_pool[i].what[0] = 0;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(){
  WP *wp = NULL;
  if (free_==NULL){  // wp_pool满了
    printf("overflow wp_pool\n");
    assert(0);
  }
  wp = free_;
  free_ = wp->next;
  // 插开头，到时候打印递归打
  wp->next = head;
  head = wp;
  return wp;
}

void free_wp(WP *wp){
  if (NULL==wp) {
    printf("NULL cannot be freed\n");
    return;
  }
  WP *prev = head;
  // 把wp移出head
  if (wp==head) {
    head = head->next;
  }else{
    while(prev && prev->next){
      if (prev->next==wp) break;
    }
    if (NULL==prev || NULL==prev->next){
      printf("%s is already freed\n", wp->what);
      return;
    }
    prev->next = wp->next;
  }
  // 把wp移入free_
  wp->hit_time = 0;
  wp->next = NULL;
  wp->what[0] = 0;
  if (NULL == free_){
    free_ = wp;
    return;
  }
  prev = free_;
  while (prev && prev->next) prev=prev->next;
  prev->next = wp;
}

void wp_recu_display(WP* wp){
  if (NULL==wp) return;
  wp_recu_display(wp->next);
  printf("%-8d%s\n", wp->NO, wp->what);
  if (wp->hit_time)
    printf("watchpoint already hit %u time\n", wp->hit_time);
}

void wp_display(){
  if (NULL == head){
    printf("No watchpoints.\n");
    return;
  }
  printf("Num     What\n");
  wp_recu_display(head);
}