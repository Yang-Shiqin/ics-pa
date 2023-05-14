void init_monitor(int, char *[]);
void engine_start();
int is_exit_status_bad();

#define TEST_EXPR 1
#if TEST_EXPR
#include "monitor/debug/expr.h"
// #include <regex.h>
// #include <stdlib.h>
// #include "isa/x86.h"
#endif

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);
#if TEST_EXPR
  /* test expr */
  FILE *fp;
  char str[1024]={0};
  bool c=true;
  bool tmp=true;

  // 打开文件
  fp = fopen("input", "r");
  if (fp == NULL) {
    printf("无法打开文件\n");
    return 1;
  }

  // 读取文件并以空格为分隔符
  while (fscanf(fp, "%s", str) != EOF) {
    c = !c;
    if(c){
      printf("%s\n", str);
      expr(str, &tmp);
    }else{
      printf("ans = %s\n", str);
    }
  }

  // 关闭文件
  fclose(fp);
  // expr
#else 

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
#endif
}
