#include "watchpoint.h"
#include "expr.h"
#include "memory/vaddr.h"   // 这样不好，但我调用vaddr_read

#define NR_WP 32    // 多于32会assert(0)

static WP wp_pool[NR_WP] = {};  // 池的形式保存
static WP *head = NULL, *free_ = NULL;  // 两个链表：head组织使用中的监视点结构；free_组织空闲的监视点结构.head倒着，free_正着

void init_wp_pool() { // 对head和free_初始化
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    // TODO: init new member
    wp_pool[i].hit_time = 0;
    wp_pool[i].addr = 0;
    wp_pool[i].last_val = 0;
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

void free_wp(uint32_t NO){
  if (NO > NR_WP) {
    printf("no watchpoint number %u\n", NO);
    return;
  }
  WP *wp = &wp_pool[NO];
  WP *prev = head;
  // 把wp移出head
  if (wp==head) {
    head = head->next;
  }else{
    while(prev){
      if (prev->next==wp) break;
      prev = prev->next;
    }
    if (NULL==prev){
      printf("No watchpoint number %u\n", wp->NO);
      return;
    }
    prev->next = wp->next;
  }
  // 把wp移入free_
  wp->hit_time = 0;
  wp->next = NULL;
  wp->addr = 0;
  wp->last_val = 0;
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
  printf("%-8d0x%x\n", wp->NO, wp->addr);
  if (wp->hit_time)
    printf("watchpoint already hit %u time\n", wp->hit_time);
}

void wp_display(){
  if (NULL == head){
    printf("No watchpoints.\n");
    return;
  }
  printf("Num     Address\n");
  wp_recu_display(head);
}

void check_recu_wp(WP* wp, bool* stop){
  if (NULL == wp) return;
  check_recu_wp(wp->next, stop);
  uint32_t new_v = vaddr_read(wp->addr, 4);
  if (wp->last_val!=new_v) {
    printf("watchpoint %u: 0x%x\n\n", wp->NO, wp->addr);
    printf("Old value = %u\n", wp->last_val);   // 统一用uint32_t，只有p是用int
    printf("New value = %u\n", new_v);          // 统一用uint32_t，只有p是用int
    wp->last_val = new_v;
    *stop = true;
    // todo: 应当打印中断的代码但我不知道咋打（暂时没打
  }
}

bool check_wp(){  // 全打印，stop为是否改变而暂停  // 2023/5/16/12:49
  bool stop=false;
  check_recu_wp(head, &stop);
  return stop;
}