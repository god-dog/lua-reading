/*
** $Id: llex.h,v 1.58.1.1 2007/12/27 13:02:25 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#ifndef llex_h
#define llex_h

#include "lobject.h"
#include "lzio.h"


/*
** 保留字枚举值第一个枚举值.
** 大于256的目的是避开ASCII值的范围
 */
#define FIRST_RESERVED	257

/* maximum length of a reserved word */
/* 最长保留字长度. `function` 为最长的保留字 */
#define TOKEN_LEN	(sizeof("function")/sizeof(char))


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
/*
** 保留字枚举量, 和`luaX_tokens`对应, 如修改顺序, `luaX_tokens`部分要做相应的顺序调整
 */
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  /* `TK_AND` ~ `TK_WHILE`为表示保留字的终结符 */
  TK_AND = FIRST_RESERVED, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  /* `TK_CONCAT` ~ `TK_EOS`为非保留字的其它终结符(运算符, 字面量和文件结束等) */
  TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_NUMBER,
  TK_NAME, TK_STRING, TK_EOS
};

/* number of reserved words */
/* 保留字个数. `TK_WHILE`为最后一个保留字, 故`TK_WHILE-FIRST_RESERVED+1`为保留字个数 */
#define NUM_RESERVED	(cast(int, TK_WHILE-FIRST_RESERVED+1))


/* array with token `names' */
LUAI_DATA const char *const luaX_tokens [];


/*
** 语义信息
** 如果为TK_NUMBER类型, seminfo.r表示对应的数值;
** 如果为TK_NAME或TK_STRING类型, seminfo.ts就表示对应的字符串
 */
typedef union {
  lua_Number r;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int token;        /* 类型标识 */
  SemInfo seminfo;  /* 语义信息 */
} Token;


/*
** 词法分析状态. 语法分析阶段核心数据结构
 */
typedef struct LexState {
  int current;  /* 当前字符 */ /* current character (charint) */
  int linenumber;  /* 当前行号. 便于错误提示 */ /* input line counter */
  int lastline;  /* 最后一个被消费符号的行号 */ /* line of last token `consumed' */
  Token t;                /* 当前符号信息 */ /* current token */
  Token lookahead;        /* 预读(向前看)符号信息 */ /* look ahead token */
  struct FuncState *fs;   /* fs指向当前正在编译的函数的 `FuncState` */ /* `FuncState' is private to the parser */
  struct lua_State *L;
  ZIO *z;         /* 输入流 */ /* input stream */
  Mbuffer *buff;  /* 临时缓冲区 */ /* buffer for tokens */
  TString *source;  /* 源文件名 */ /* current source name */
  char decpoint;  /* locale decimal point */
} LexState;


LUAI_FUNC void luaX_init (lua_State *L);
LUAI_FUNC void luaX_setinput (lua_State *L, LexState *ls, ZIO *z,
                              TString *source);
LUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC void luaX_next (LexState *ls);
LUAI_FUNC void luaX_lookahead (LexState *ls);
LUAI_FUNC void luaX_lexerror (LexState *ls, const char *msg, int token);
LUAI_FUNC void luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);


#endif
