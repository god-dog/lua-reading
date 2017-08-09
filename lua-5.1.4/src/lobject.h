/*
** $Id: lobject.h,v 2.20.1.2 2008/08/06 13:29:48 roberto Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/


#ifndef lobject_h
#define lobject_h


#include <stdarg.h>


#include "llimits.h"
#include "lua.h"


/* tags for values visible from Lua */
#define LAST_TAG	LUA_TTHREAD

#define NUM_TAGS	(LAST_TAG+1)


/*
** Extra tags for non-values
*/
/*
** 其他类型(非普通值类型)
 */
#define LUA_TPROTO	(LAST_TAG+1)
#define LUA_TUPVAL	(LAST_TAG+2)
#define LUA_TDEADKEY	(LAST_TAG+3)


/*
** Union of all collectable objects
*/
typedef union GCObject GCObject;


/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
/*
** 所有可回收对象的公共头
** `next` 相邻可回收对象, 串联成链表
** `tt` 数据类型
** `marked` 标识可回收对象的颜色值
 * */
#define CommonHeader	GCObject *next; lu_byte tt; lu_byte marked


/*
** Common header in struct form
*/
typedef struct GCheader {
  CommonHeader;
} GCheader;




/*
** Union of all Lua values
*/
typedef union {
  GCObject *gc; /* 可被gc的对象 */
  void *p;      /* light userdata类型 */
  lua_Number n; /* 数值类型 */
  int b;        /* 布尔类型 */
} Value;


/*
** Tagged Values
*/

/*
**  `value`: 值对象
**  `tt`: tagged type. 类型标记, 含义见LUA_T{type}
 */
#define TValuefields	Value value; int tt

/*
** 值对象基类结构体. 弱数据类型实现
** 考虑结构体对齐, lua中任何一种数据类型至少占用16字节, 即使nil类型也会占用8个字节
 */
/*
typedef struct lua_TValue {
  union {
    union GCObject {
      struct GCheader {
        GCObject *next;
        lu_byte tt;
        lu_byte marked
      } gch;
      union TString ts;
      union Udata u;
      union Closure cl;
      struct Table h;
      struct Proto p;
      struct UpVal uv;
      struct lua_State th;
    } *gc;
    void *p;
    lua_Number n;
    int b;
  } value;

  int tt
} TValue;
*/

typedef struct lua_TValue {
  TValuefields;
} TValue;


/* Macros to test type */
/* 类型检测宏 */
#define ttisnil(o)	(ttype(o) == LUA_TNIL)
#define ttisnumber(o)	(ttype(o) == LUA_TNUMBER)
#define ttisstring(o)	(ttype(o) == LUA_TSTRING)
#define ttistable(o)	(ttype(o) == LUA_TTABLE)
#define ttisfunction(o)	(ttype(o) == LUA_TFUNCTION)
#define ttisboolean(o)	(ttype(o) == LUA_TBOOLEAN)
#define ttisuserdata(o)	(ttype(o) == LUA_TUSERDATA)
#define ttisthread(o)	(ttype(o) == LUA_TTHREAD)
#define ttislightuserdata(o)	(ttype(o) == LUA_TLIGHTUSERDATA)

/* Macros to access values */
/* 取值相关宏 */
#define ttype(o)	((o)->tt)
#define gcvalue(o)	check_exp(iscollectable(o), (o)->value.gc)
#define pvalue(o)	check_exp(ttislightuserdata(o), (o)->value.p)
#define nvalue(o)	check_exp(ttisnumber(o), (o)->value.n)
#define rawtsvalue(o)	check_exp(ttisstring(o), &(o)->value.gc->ts)
#define tsvalue(o)	(&rawtsvalue(o)->tsv)
#define rawuvalue(o)	check_exp(ttisuserdata(o), &(o)->value.gc->u)
#define uvalue(o)	(&rawuvalue(o)->uv)
#define clvalue(o)	check_exp(ttisfunction(o), &(o)->value.gc->cl)
#define hvalue(o)	check_exp(ttistable(o), &(o)->value.gc->h)
#define bvalue(o)	check_exp(ttisboolean(o), (o)->value.b)
#define thvalue(o)	check_exp(ttisthread(o), &(o)->value.gc->th)

#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

/*
** for internal debug only
*/
#define checkconsistency(obj) \
  lua_assert(!iscollectable(obj) || (ttype(obj) == (obj)->value.gc->gch.tt))

#define checkliveness(g,obj) \
  lua_assert(!iscollectable(obj) || \
  ((ttype(obj) == (obj)->value.gc->gch.tt) && !isdead(g, (obj)->value.gc)))


/* Macros to set values */
/* 赋值相关宏. GC部分 */
#define setnilvalue(obj) ((obj)->tt=LUA_TNIL)

#define setnvalue(obj,x) \
  { TValue *i_o=(obj); i_o->value.n=(x); i_o->tt=LUA_TNUMBER; }

#define setpvalue(obj,x) \
  { TValue *i_o=(obj); i_o->value.p=(x); i_o->tt=LUA_TLIGHTUSERDATA; }

#define setbvalue(obj,x) \
  { TValue *i_o=(obj); i_o->value.b=(x); i_o->tt=LUA_TBOOLEAN; }

/* 赋值相关宏. GC部分 */
#define setsvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TSTRING; \
    checkliveness(G(L),i_o); }

#define setuvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TUSERDATA; \
    checkliveness(G(L),i_o); }

#define setthvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TTHREAD; \
    checkliveness(G(L),i_o); }

#define setclvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TFUNCTION; \
    checkliveness(G(L),i_o); }

#define sethvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TTABLE; \
    checkliveness(G(L),i_o); }

#define setptvalue(L,obj,x) \
  { TValue *i_o=(obj); \
    i_o->value.gc=cast(GCObject *, (x)); i_o->tt=LUA_TPROTO; \
    checkliveness(G(L),i_o); }



/* 对象赋值 & 浅拷贝 */
#define setobj(L,obj1,obj2) \
  { const TValue *o2=(obj2); TValue *o1=(obj1); \
    o1->value = o2->value; o1->tt=o2->tt; \
    checkliveness(G(L),o1); }


/*
** different types of sets, according to destination
*/

/* from stack to (same) stack */
#define setobjs2s	setobj
/* to stack (not from same stack) */
#define setobj2s	setobj
#define setsvalue2s	setsvalue
#define sethvalue2s	sethvalue
#define setptvalue2s	setptvalue
/* from table to same table */
#define setobjt2t	setobj
/* to table */
#define setobj2t	setobj
/* to new object */
#define setobj2n	setobj
#define setsvalue2n	setsvalue

#define setttype(obj, tt) (ttype(obj) = (tt))


/* >= 4 的数据类型会被垃圾回收*/
#define iscollectable(o)	(ttype(o) >= LUA_TSTRING)



typedef TValue *StkId;  /* 栈元素引用 *//* index to stack elements */


/*
** String headers for string table
*/
typedef union TString {
  L_Umaxalign dummy;  /* ensures maximum alignment for strings */
  struct {
    CommonHeader;
    lu_byte reserved; /* 是否保留字. 词法分析时便于快速判断字符串是否为保留字 @see `enum RESERVED` 和 `luaX_tokens` */
    unsigned int hash;/* 字符串的散列值. lua 5.2 以后区分长字符串和短字符串, 为避免Hash Dos, 长字符串不再内部化 */
    size_t len;       /* 字符串长度 */
  } tsv;
} TString;


/* 获取C字符串的原始指针 */
#define getstr(ts)	cast(const char *, (ts) + 1)
#define svalue(o)       getstr(rawtsvalue(o))



/*
 * user data 储存形式上和字符串近似, 可看成是有独立元表, 不需要内部化处理, 不以'\0'结尾的字符串
 */
typedef union Udata {
  L_Umaxalign dummy;  /* ensures maximum alignment for `local' udata */
  struct {
    CommonHeader;
    struct Table *metatable;
    struct Table *env;
    size_t len;       /* 字节大小 */
  } uv;
} Udata;




/*
** Function Prototypes
*/
typedef struct Proto {
  CommonHeader;
  TValue *k;  /* 常量表: 函数运行期需要的所有常量, 使用常量表id进行索引 */ /* constants used by the function */
  Instruction *code; /* 指令列表: 包含函数编译后生成的虚拟机指令 */
  struct Proto **p;  /* Proto表：嵌套于该函数的所有子函数的Proto列表, OP_CLOSURE指令中的proto id通过proto表进行索引 */ /* functions defined inside the function */
  int *lineinfo;  /* 指令行号信息, 调试之用 */ /* map from opcodes to source lines */
  struct LocVar *locvars;  /* 局部变量信息: 函数中的所有局部变量名称及其生命周期. 由于所有局部变量在运行期都转化成了寄存器id, 这些信息仅供debug使用 */ /* information about local variables */
  TString **upvalues;  /* upvalue names */
  TString  *source;     /* 函数信息, 调试之用 */
  int sizeupvalues; /* `upvalues` 大小 */
  int sizek;  /* 常量表 `k` 大小 */ /* size of `k' */
  int sizecode;
  int sizelineinfo;
  int sizep;  /* Proto表 `p` 大小 */ /* size of `p' */
  int sizelocvars; /* `localvars` 数组长度*/
  int linedefined;      /* 函数定义起始行号, 即 `function` 关键字所在的行号 */
  int lastlinedefined;  /* 函数定义结束行号, 即 `end` 关键字所在的行号 */
  GCObject *gclist;
  lu_byte nups;  /* number of upvalues */
  lu_byte numparams;  /* 参数个数 */
  lu_byte is_vararg;  /* 参数是否为可变参数 "..." */
  lu_byte maxstacksize;
} Proto;


/* masks for new-style vararg */
#define VARARG_HASARG		1
#define VARARG_ISVARARG		2
#define VARARG_NEEDSARG		4


typedef struct LocVar {
  TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} LocVar;



/*
** Upvalues
*/

typedef struct UpVal {
  CommonHeader;
  TValue *v;  /* points to stack or to its own value */
  union {
    TValue value;  /* the value (when closed) */
    struct {  /* 双链表指针. UpValue开放时生效, 关闭时失效 */ /* double linked list (when open) */
      struct UpVal *prev;
      struct UpVal *next;
    } l;
  } u;
} UpVal;


/*
** Closures
*/
/*
** isC 是否C函数. 0 - lua函数, 1 - c函数
** nupvalues upvalue或upvals个数
 * env 上下文环境
 */
#define ClosureHeader \
	CommonHeader; lu_byte isC; lu_byte nupvalues; GCObject *gclist; \
	struct Table *env

/*
** CClosure: 与外部C函数交互的闭包
 */
typedef struct CClosure {
  ClosureHeader;
  lua_CFunction f;      /* C函数原型, 实际为一个函数指针 */
  TValue upvalue[1];    /* 函数参数 */
} CClosure;

/*
** LClosure: lua内部函数的闭包, 由lua虚拟机管理
 * Closure对象是lua运行期的一个函数实例对象, Proto则是编译期Closure的原型对象
 * 拥有upvalue的函数即为闭包, 没有upvalue的闭包是函数
 */
typedef struct LClosure {
  ClosureHeader;
  struct Proto *p;
  UpVal *upvals[1];
} LClosure;


typedef union Closure {
  CClosure c;
  LClosure l;
} Closure;


/* 检查是否 C API */
#define iscfunction(o)	(ttype(o) == LUA_TFUNCTION && clvalue(o)->c.isC)
/* 检查是否 Lua API */
#define isLfunction(o)	(ttype(o) == LUA_TFUNCTION && !clvalue(o)->c.isC)


/*
** Tables
*/

typedef union TKey {
  struct {
    TValuefields;
    struct Node *next;  /* for chaining */
  } nk;
  TValue tvk;
} TKey;


typedef struct Node {
  TValue i_val;
  TKey i_key;
} Node;

/*
** table是一个数组和散列表的混合数据结构. key为整数时数据保存在数组中; 其他类型保存在散列表中
 */
typedef struct Table {
  CommonHeader;
  lu_byte flags;  /* 元方法标识位. @see `TMS`. */ /* 1<<p means tagmethod(p) is not present */
  lu_byte lsizenode;  /* 桶大小的log_2值, 即桶基于原来大小的2倍进行扩容, 其大小为2次幂. */ /* log2 of size of `node' array */
  struct Table *metatable; /* 元表 */
  TValue *array;  /* 数组部分首指针 */ /* array part */
  Node *node;   /* Hash桶部分首指针 */
  Node *lastfree;  /* Hash桶部分尾指针, 最后一个空闲位置 */ /* any free position is before this position */
  GCObject *gclist;
  int sizearray;  /* 数组部分大小 */ /* size of `array' array */
} Table;



/*
** `module' operation for hashing (size is always a power of 2)
*/
#define lmod(s,size) \
	(check_exp((size&(size-1))==0, (cast(int, (s) & ((size)-1)))))


#define twoto(x)	(1<<(x))
#define sizenode(t)	(twoto((t)->lsizenode))


#define luaO_nilobject		(&luaO_nilobject_)

LUAI_DATA const TValue luaO_nilobject_;

#define ceillog2(x)	(luaO_log2((x)-1) + 1)

LUAI_FUNC int luaO_log2 (unsigned int x);
LUAI_FUNC int luaO_int2fb (unsigned int x);
LUAI_FUNC int luaO_fb2int (int x);
LUAI_FUNC int luaO_rawequalObj (const TValue *t1, const TValue *t2);
LUAI_FUNC int luaO_str2d (const char *s, lua_Number *result);
LUAI_FUNC const char *luaO_pushvfstring (lua_State *L, const char *fmt,
                                                       va_list argp);
LUAI_FUNC const char *luaO_pushfstring (lua_State *L, const char *fmt, ...);
LUAI_FUNC void luaO_chunkid (char *out, const char *source, size_t len);


#endif

