#include <isa.h>
#include "memory/vaddr.h"
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, 
  TK_XNUM,
  TK_ONUM,
  TK_DNUM,
  TK_EQ,
  TK_NE,
  TK_AND,
  TK_REG,
  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces  匹配一个或多个(+)空格
  {"\\+", '+'},         // plus    匹配加号
  {"\\-", '-'},         // sub
  {"\\*", '*'},         // mul
  {"/", '/'},           // div
  {"0x[0-9a-fA-F]+", TK_XNUM},      // 十六进制数
  {"0[0-7]+", TK_ONUM}, // 八进制数
  {"[0-9]+", TK_DNUM},  // 十进制数
  {"\\(", '('},         // 左括号
  {"\\)", ')'},         // 右括号
  {"==", TK_EQ},        // equal
  {"!=", TK_NE},        // not equal
  {"&&", TK_AND},       // 逻辑与
  {"\\$[a-z]+", TK_REG},      // 寄存器值
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

// ysq
// 检查括号配对，若括号不配对，*err=true；若配对且最外面的能去掉，返回true，否则返回false
// p: 表达式开头，tokens下标
// q: 表达式结尾，包括q的token
// err: 是否发生错误，1为错误
bool check_parentheses(int p, int q, bool *err){
  int top=-1;
  int i;
  bool flag=true;
  for(i=p; i<=q; i++){
    switch (tokens[i].type)
    {
    case '(':
      top++;
      break;
    case ')':
      if (top==-1) {
        *err = true;
        return false;
      }
      top--;
      if (top==-1 && i!=q) {   // 非最后一个 把第一个括号匹配掉了
        flag = false;
      }
      break;
    default:
      if (i==p) flag=false; // 第一个不是(
      continue;
    }
  }
  return flag;
}

// ysq  // 2023/5/14/17:24
// 解析表达式（暂时只考虑数字、括号、+-*/、正负、解引用、&&、!=、==、寄存器变量
// p: 表达式开头，tokens下标
// q: 表达式结尾，包括q的token
// err: 是否发生错误，1为错误（传入的时候就要为0
int eval(int p, int q, bool *err) {
  if (*err==true) {
    return 0;
  }
  if (p > q) {
    /* Bad expression */
    printf("Bad expression: p=%d, q=%d\n", p, q);
    *err = true;
    return 0;
  }
  else if (p == q) {  // num或寄存器变量
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if ((tokens[p].type!=TK_XNUM)&&(tokens[p].type!=TK_ONUM)&&(tokens[p].type!=TK_DNUM)&&(tokens[p].type!=TK_REG)){ // 非数
      printf("Single token but not num or reg\n");
      *err = true;
      return 0;
    }
    // str2num
    if (tokens[p].type==TK_REG){
      bool success=true;
      int ans = isa_reg_str2val(tokens[p].str, &success);
      if (success==true) {
        return ans;
      }
      *err = true;
      printf("No reg named %s\n", tokens[p].str);
      return 0;
    }else{
      return strtol(tokens[p].str, NULL, 0);
    }
  }
  else if (check_parentheses(p, q, err) == true) {  // (expr)
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, err);
  }
  else {  // expr
    if (*err==true) {
      return 0;
    }
    int op = -1;  // 主符号下标
    int op_pr = -1;  // 当前所选主符号优先级
    int top = -1; // 在括号里
    int i;
    for (i=p; i<=q; i++) {
      switch (tokens[i].type)
      {
      case '(':   // 优先级1
        top--;  // 经过check_parentheses默认括号是配对的
        break;
      case ')':
        top++;
        break;
      case '+':   // 优先级5
      case '-':
        if (top == -1 && op_pr<=5){ // 括号外且优先级低于等于当前主操作符
          op = i;
          op_pr=5;
        }
        break;
      case '*':   // 优先级4
      case '/':
        if (top==-1 && op_pr<=4){ // 括号外
          op = i;
          op_pr = 4;
        }
        break;
      case TK_EQ: // 优先级8
      case TK_NE:
        if (top==-1 && op_pr<=8){ // 括号外
          op = i;
          op_pr = 8;
        }
        break;
      case TK_AND:  // 优先级12
        if (top==-1 && op_pr<=12){ // 括号外
          op = i;
          op_pr = 8;
        }
        break;
      default:
        continue;
      }
      while(i<=q && (tokens[i].type=='+'||      // 连续操作符只有最左边才能是主操作符
                     tokens[i].type=='-'||
                     tokens[i].type=='*'||
                     tokens[i].type=='/'||
                     tokens[i].type==TK_EQ||
                     tokens[i].type==TK_NE||
                     tokens[i].type==TK_AND)) i++;
    }
    int val1 = 0;
    if (op!=p || (tokens[op].type!='+'&&tokens[op].type!='-'&&tokens[op].type!='*')) val1 = eval(p, op - 1, err);
    int val2 = eval(op + 1, q, err);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': 
        if (op==p){
          return vaddr_read(val2, 4);
        }else{
          return val1 * val2;
        }
      case '/': return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NE: return val1 != val2;
      case TK_AND: return val1 && val2;
      default: 
        *err=true;
        printf("Fault op: %d(%c)\n", tokens[op].type, (char)tokens[op].type);
        return 0;
    }
  }
}

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) { // 在开头匹配到re[i]
        char *substr_start = e + position;  // 子串开始
        int substr_len = pmatch.rm_eo;      // 子串长度

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case TK_EQ:
          case TK_NE:
          case TK_AND:
            nr_token++;
            break;
          case TK_XNUM:
          case TK_ONUM:
          case TK_DNUM:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token++].str[substr_len] = '\0';
            break;
          case TK_REG:
            strncpy(tokens[nr_token].str, substr_start+1, substr_len);
            tokens[nr_token++].str[substr_len] = '\0';
            break;
          case TK_NOTYPE:
          default: break;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  bool err=false;
  int val = eval(0, nr_token-1, &err);
  if(err==true)
    *success = false;
  return val;
}
