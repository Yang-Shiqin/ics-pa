#include <isa.h>
#include "expr.h"
#include "watchpoint.h"
#include "memory/vaddr.h"   // 这样不好，但我调用vaddr_read

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

// 通过ui_mainloop可知args就是除去第一个子字符串剩下的字符串
static int cmd_help(char *args);
static int cmd_si(char *args);    // [x] 2023/5/13/14:20
static int cmd_info(char *args);  // [x] 2023/5/15/20:53
static int cmd_x(char *args);
static int cmd_p(char *args);     // [x] 2023/5/14/17:24(暂时还没怎么测试,没有识别变量名)
static int cmd_w(char *args);     // [x] 2023/5/16/09:36(功能限制于expr)
static int cmd_d(char *args);     // [x] 2023/5/16/09:47

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "step one instruction exactly", cmd_si },
  { "info", "generic cmd for showing things about the program being debugged: info r/info w", cmd_info },
  { "x", "examine memory: x N Addr", cmd_x },
  { "p", "Print value of expression EXP.", cmd_p },
  { "w", "Set a watchpoint for EXPRESSION.", cmd_w },
  { "d", "Delete all or some watchpoints.", cmd_d },

  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args){
  // if (nemu_state.state!=NEMU_RUNNING || nemu_state.state!=NEMU_STOP){   /*程序不在跑*/
  //   printf("The program is not being run.\n");
  //   return 0;
  // }
  if(args == NULL){                    /*没参数，默认N=1*/
    cpu_exec(1);
    return 0;
  }
  char* args_end = args+strlen(args);
  char *arg = strtok(NULL, " ");
  char* second_args = arg+strlen(arg)+1;
  if(second_args < args_end){         /*参数多于1个*/
    printf("A syntax error in expression, near `%s'\n", second_args);
  }else if(((arg[0]!='-')&&(arg[0]<'0'||arg[0]>'9')) || ((arg[0]=='-')&&(arg[1]<'0'||arg[1]>'9'))){ /*非数字符号开头*/ 
    printf("No symbol \"%s\" in current context.\n", arg);
  }else{
    char *str_end=NULL;
    int step = strtol(arg, &str_end, 0);
    if(str_end!=arg+strlen(arg)){     /*数字开头的非数*/
      printf("Invalid number \"%s\".\n", arg);
    }else{                            /*正常*/
      cpu_exec(step>0?step:0);        // 执行max(0, step)步
    }
  }
  return 0;
}

static int cmd_info(char *args){
  if(args == NULL){                    /*没参数，打印info用法*/
    cmd_help("info");
    return 0;
  }
  if (strcmp(args, "r") == 0){
    isa_reg_display();
  }else if (strcmp(args, "w") == 0){  // 目前只能打印全部
    wp_display();
  }else
    printf("Undefined info command: \"%s\".  Try \"help info\"\n", args);
  return 0;
}

static int cmd_x(char *args){
  if(args == NULL){                    /*没参数，打印x用法*/
    cmd_help("x");
    return 0;
  }
  char *arg = strtok(NULL, " ");
  char* second_args = arg+strlen(arg)+1;
  bool success=true;
  uint32_t len = expr(arg, &success);  // 长度
  uint32_t val = expr(second_args, &success);  // 地址
  if (true==success){
    uint32_t i;
    for (i=0; i<len; i++)
      printf("0x%x\n", vaddr_read(val+i, 4));
  }
  else
    printf("bad expr\n");
  return 0;
}

static int cmd_p(char *args){
  if(args == NULL){                    /*没参数，打印p用法*/
    cmd_help("p");
    return 0;
  }
  static uint32_t No=0;
  bool success=true;
  int val = (int)expr(args, &success);
  if (true==success)
    printf("$%u=%d\n", ++No, val);
  else
    printf("bad expr\n");
  return 0;
}

static int cmd_w(char *args){
  if(args == NULL){                    /*没参数，打印w用法*/
    cmd_help("w");
    return 0;
  }
  bool success=true;
  uint32_t val = expr(args, &success);  // 地址
  if (true==success){
    WP* wp = new_wp();
    wp->addr = val;
    wp->last_val = vaddr_read(val, 4);
  }else
    printf("bad expr\n");
  return 0;
}

static int cmd_d(char *args){
  if(args == NULL){                    /*没参数，打印d用法*/
    cmd_help("d");
    return 0;
  }
  bool success=true;
  uint32_t val = expr(args, &success);  // 序号
  if (true==success)
    free_wp(val);
  else
    printf("bad expr\n");
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) { // 读取输入命令
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");   // 获取以空格为分隔符的第一个子字符(一般是命令名)(剩下的字符串会留在strtok里)
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1; // 命令除去命令名子串的字符串(一般是参数)
    if (args >= str_end) { // 不带参数
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
