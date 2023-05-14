void init_monitor(int, char *[]);
void engine_start();
int is_exit_status_bad();

#define TEST_EXPR 1
#if TEST_EXPR
#include "monitor/debug/expr.h"
#endif

int main(int argc, char *argv[]) {
#if TEST_EXPR
  /* test expr */
  FILE *fp;
  char str[1024];
  bool c=false;
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
      printf("ans = %s\n", str);
      break;
      expr(str, &tmp);
    }else{
      printf("ans = %s\n", str);
    }
    break;
  }

  // 关闭文件
  fclose(fp);
  // expr
#else 
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
#endif
}
