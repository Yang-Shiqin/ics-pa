#include <isa.h>
#include "expr.h"
#include "watchpoint.h"
#include <monitor/monitor.h>

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
static int cmd_si(char *args);
// static int cmd_info(char *args);
// static int cmd_x(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "step one instruction exactly", cmd_si },
//   { "info", "generic cmd for showing things about the program being debugged", cmd_info },
//   { "x", "examine memory", cmd_x },

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
  char* args_end = args+strlen(args);
  // if (nemu_state.state!=NEMU_RUNNING || nemu_state.state!=NEMU_STOP){   /*程序不在跑*/
  //   printf("The program is not being run.\n");
  //   return 0;
  // }
  cpu_exec(1);
  char *arg = strtok(NULL, " ");
  if(arg == NULL){                    /*没参数，默认N=1*/
    return 1;
  }
  char* second_args = arg+strlen(arg)+1;
  if(second_args < args_end){         /*参数多于1个*/
    printf("A syntax error in expression, near `%s'\n", second_args);
    return 0;
  }
  if(arg[0]<'0' || arg[0]>'9'){       /*非数字符号开头*/ 
    printf("No symbol \"%s\" in current context.", arg);
    return 0;
  }
  char *str_end=NULL;
  int step = strtol(arg, &str_end, 0);
  if(str_end!=arg+strlen(arg)){       /*数字开头的非数*/
    printf("Invalid number \"%s\".", arg);
    return 0;
  }else{                              /*正常*/
    return step>0?step:0;             // 执行max(0, step)步
  }
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
