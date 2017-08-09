/*
** $Id: lstate.h,v 2.24.1.2 2008/01/03 15:20:39 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/
/*
** 全局状态
 */

#ifndef lstate_h
#define lstate_h

#include "lua.h"

#include "lobject.h"
#include "ltm.h"
#include "lzio.h"



struct lua_longjmp;  /* defined in ldo.c */


/* table of globals */
/* 全局变量表 */
#define gt(L)	(&L->l_gt)

/* registry */
/* 注册表. C代码可以自由使用, 但Lua代码不能访问 */
#define registry(L)	(&G(L)->l_registry)


/* extra stack space to handle TM calls and some other extras */
#define EXTRA_STACK   5


#define BASIC_CI_SIZE           8

#define BASIC_STACK_SIZE        (2*LUA_MINSTACK)



typedef struct stringtable {
  GCObject **hash;
  lu_int32 nuse;  /* 元素个数 */  /* number of elements */
  int size;       /* 桶大小 */
} stringtable;


/*
** informations about a call
*/
/*
** 函数调用信息
 */
typedef struct CallInfo {
  StkId base;  /* base for this function */
  StkId func;  /* function index in the stack */
  StkId	top;  /* top for this function */
  const Instruction *savedpc; /* 调用中断时, 用于记录程序计数器(pc)位置信息 */
  int nresults;  /* 返回值个数. -1 表示返回值个数不限 */ /* expected number of results from this function */
  int tailcalls;  /* 尾递归调用次数. 调试之用 */ /* number of tail calls lost under this entry */
} CallInfo;



#define curr_func(L)	(clvalue(L->ci->func))
#define ci_func(ci)	(clvalue((ci)->func))
#define f_isLua(ci)	(!ci_func(ci)->c.isC)
#define isLua(ci)	(ttisfunction((ci)->func) && f_isLua(ci))


/*
** `global state', shared by all threads of this state
*/
/*
** 全局状态, 所有执行线程共享该状态
 */
typedef struct global_State {
  stringtable strt;  /* 全局字符串表 */ /* hash table for strings */
  lua_Alloc frealloc;  /* function to reallocate memory */
  void *ud;         /* auxiliary data to `frealloc' */
  lu_byte currentwhite;
  lu_byte gcstate;  /* state of garbage collector */
  int sweepstrgc;  /* position of sweep in `strt' */
  GCObject *rootgc;  /* list of all collectable objects */
  GCObject **sweepgc;  /* position of sweep in `rootgc' */
  GCObject *gray;  /* list of gray objects */
  GCObject *grayagain;  /* list of objects to be traversed atomically */
  GCObject *weak;  /* list of weak tables (to be cleared) */
  GCObject *tmudata;  /* last element of list of userdata to be GC */
  Mbuffer buff;  /* temporary buffer for string concatentation */
  lu_mem GCthreshold;
  lu_mem totalbytes;  /* number of bytes currently allocated */
  lu_mem estimate;  /* an estimate of number of bytes actually in use */
  lu_mem gcdept;  /* how much GC is `behind schedule' */
  int gcpause;  /* size of pause between successive GCs */
  int gcstepmul;  /* GC `granularity' */
  lua_CFunction panic;  /* to be called in unprotected errors */
  TValue l_registry;  /* `LUA_REGISTRYINDEX` 对应的全局表 */
  struct lua_State *mainthread;
  UpVal uvhead;  /* head of double-linked list of all open upvalues */
  struct Table *mt[NUM_TAGS];  /* 基本类型的元表 */ /* metatables for basic types */
  TString *tmname[TM_N];  /* 元方法名数组 */ /* array with tag-method names */
} global_State;


/*
** `per thread' state
*/
/*
** 每个线程(协程)对应一个状态上下文. 每个线程拥有独立的数据栈, 函数调用链, 调试钩子和错误处理方法.
 * `lua_State`不是一个静态数据结构, 而是lua程序的执行状态机.
 * 几乎所有的API操作都是围绕`lua_State`进行, 因此API的第一个参数类型总是`lua_State*`.
 * lua_State中包括两个重要的数据结构, 一个是数组表示的数据栈, 一个是链表表示的函数调用栈
 * lua虚拟机模拟CPU, lua栈模拟内存
 */
struct lua_State {
  CommonHeader;
  lu_byte status; /* 线程状态. @see LUA_YIELD: 1, LUA_ERRRUN: 2, LUA_ERRSYNTAX: 3, LUA_ERRMEM: 4, LUA_ERRERR: 5  */
  StkId top;  /* 数据栈栈顶 */ /* first free slot in the stack */
  StkId base; /* 当前函数运行时基址位置 */  /* base of current function */
  global_State *l_G;  /* 全局状态指针 */
  CallInfo *ci;  /* 当前函数调用信息 */ /* call info for current function */
  const Instruction *savedpc; /* 当前函数程序计数器 */ /* `savedpc' of current function */
  StkId stack_last;  /* 数据栈最后一个实际空闲位置 */ /* last free slot in the stack */
  StkId stack;  /* 数据栈的基址 */ /* stack base */
  CallInfo *end_ci;  /* 函数调用栈栈顶 */ /* points after end of ci array*/
  CallInfo *base_ci;  /* 函数调用栈栈底 */ /* array of CallInfo's */
  int stacksize;  /* 数据栈大小 */
  int size_ci;  /* 函数调用栈大小 */ /* size of array `base_ci' */
  unsigned short nCcalls;  /* C函数调用深度 */ /* number of nested C calls */
  unsigned short baseCcalls;  /* nested C calls when resuming coroutine */
  lu_byte hookmask; /* hook掩码. @see LUA_MASKCALL, LUA_MASKRET, LUA_MASKLINE, LUA_MASKCOUNT */
  lu_byte allowhook; /* 是否允许hook */
  int basehookcount; /* 掩码设置为`LUA_MASKCOUNT`时, 执行`basehookcount`条指令触发hook */
  int hookcount;  /* 掩码设置为`LUA_MASKCOUNT`时, 运行了`hookcount`条指令 */
  lua_Hook hook; /* 用户注册的hook回调函数. 函数指针 */
  TValue l_gt;  /* 当前线程的全局环境表 */ /* table of globals */
  TValue env;  /* 当前环境表 */ /* temporary place for environments */
  GCObject *openupval;  /* list of open upvalues in this stack */
  GCObject *gclist;
  struct lua_longjmp *errorJmp;  /* 记录当函数发生错误时长跳转位置 */ /* current error recover point */
  ptrdiff_t errfunc;  /* 错误处理回调函数 */ /* current error handling function (stack index) */
};

/* 全局状态快速访问宏 */
#define G(L)	(L->l_G)


/*
** Union of all collectable objects
*/
/*
** >=4 的数据类型会被垃圾回收. @see `iscollectable`
 */
union GCObject {
  GCheader gch;
  union TString ts;     /* 字符串对象 */
  union Udata u;        /* userdata对象 */
  union Closure cl;     /* 闭包对象 */
  struct Table h;       /* 表对象 */
  struct Proto p;       /* 原型对象 */
  struct UpVal uv;      /* UpVal对象 */
  struct lua_State th;  /* 线程对象 */  /* thread */
};


/* macros to convert a GCObject into a specific value */
#define rawgco2ts(o)	check_exp((o)->gch.tt == LUA_TSTRING, &((o)->ts))
#define gco2ts(o)	(&rawgco2ts(o)->tsv)
#define rawgco2u(o)	check_exp((o)->gch.tt == LUA_TUSERDATA, &((o)->u))
#define gco2u(o)	(&rawgco2u(o)->uv)
#define gco2cl(o)	check_exp((o)->gch.tt == LUA_TFUNCTION, &((o)->cl))
#define gco2h(o)	check_exp((o)->gch.tt == LUA_TTABLE, &((o)->h))
#define gco2p(o)	check_exp((o)->gch.tt == LUA_TPROTO, &((o)->p))
#define gco2uv(o)	check_exp((o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define ngcotouv(o) \
	check_exp((o) == NULL || (o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define gco2th(o)	check_exp((o)->gch.tt == LUA_TTHREAD, &((o)->th))

/* macro to convert any Lua object into a GCObject */
/* lua对象转换为gc对象宏 */
#define obj2gco(v)	(cast(GCObject *, (v)))


LUAI_FUNC lua_State *luaE_newthread (lua_State *L);
LUAI_FUNC void luaE_freethread (lua_State *L, lua_State *L1);

#endif

