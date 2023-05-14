#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_XNUM,
  TK_ONUM,
  TK_DNUM,
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
      if (i==q) flag=false; // 第一个不是(
      continue;
    }
  }
  return flag;
}


// ysq  // 2023/5/14/10:47
// 解析表达式（暂时只考虑数字、括号、+-*/
// p: 表达式开头，tokens下标
// q: 表达式结尾，包括q的token
// err: 是否发生错误，1为错误（传入的时候就要为0
int eval(int p, int q, bool *err) {
  if (*err==true) {
    return 0;
  }
  if (p > q) {
    /* Bad expression */ // 如()
    printf("Bad expression: p=%d, q=%d\n", p, q);
    *err = true;
    return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if ((tokens[p].type!=TK_XNUM)&&(tokens[p].type!=TK_ONUM)&&(tokens[p].type!=TK_DNUM)){ // 非数
      printf("Single token but not num\n");
      *err = true;
      return 0;
    }
    // str2num
    return strtol(tokens[p].str, NULL, 0);
  }
  else if (check_parentheses(p, q, err) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    printf("(%d)\n", eval(p + 1, q - 1, err));
    return eval(p + 1, q - 1, err);
  }
  else {
    if (*err==true) {
      return 0;
    }
    int op = -1;
    int top = -1; // 在括号里
    int i;
    int quit=0;
    for (i=q; i>=p && !quit; i--) {
      switch (tokens[i].type)
      {
      case '(':
        top--;  // 经过check_parentheses默认括号是配对的
        break;
      case ')':
        top++;
        break;
      case '+':
      case '-':
        if (top == -1){ // 括号外
          op = i;
          quit=1;
        }
        break;
      case '*':
      case '/':
        if (top==-1 && op==-1){ // 括号外且是右边第一个
          op = i;
        }
        break;
      default:
        continue;
      }
    }
    int val1 = eval(p, op - 1, err);
    int val2 = eval(op + 1, q, err);
    printf("val1 = %d, val2 = %d\n", val1, val2);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
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
            nr_token++;
            break;
          case TK_XNUM:
          case TK_ONUM:
          case TK_DNUM:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
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
  static uint32_t No=0;
  bool err=false;
  int val = eval(0, nr_token-1, &err);
  if(err==false)
    printf("$%u=%d\n", ++No, val);
  return 0;
}
