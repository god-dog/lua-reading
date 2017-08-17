/*
** $Id: lundump.c,v 2.7.1.4 2008/04/04 19:51:41 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#include <string.h>

#define lundump_c
#define LUA_CORE

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstring.h"
#include "lundump.h"
#include "lzio.h"

typedef struct {
 lua_State* L;
 ZIO* Z;
 Mbuffer* b;
 const char* name;
} LoadState;

#ifdef LUAC_TRUST_BINARIES
#define IF(c,s)
#define error(S,s)
#else
#define IF(c,s)		if (c) error(S,s)

/*
** 报告错误.
 * 抛出LUA_ERRSYNTAX异常
 */
static void error(LoadState* S, const char* why)
{
 luaO_pushfstring(S->L,"%s: %s in precompiled chunk",S->name,why);
 luaD_throw(S->L,LUA_ERRSYNTAX);
}
#endif

#define LoadMem(S,b,n,size)	LoadBlock(S,b,(n)*(size))
#define	LoadByte(S)		(lu_byte)LoadChar(S)
#define LoadVar(S,x)		LoadMem(S,&x,1,sizeof(x))
#define LoadVector(S,b,n,size)	LoadMem(S,b,n,size)

/*
** 读取一个`size`大小的数据块, 保存在`b`中
 */
static void LoadBlock(LoadState* S, void* b, size_t size)
{
 size_t r=luaZ_read(S->Z,b,size);
 IF (r!=0, "unexpected end");
}

/*
** 从`S`中读取一个char变量. 实际返回`int`类型
 */
static int LoadChar(LoadState* S)
{
 char x;
 LoadVar(S,x);
 return x;
}

/*
** 读取一个int变量
 */
static int LoadInt(LoadState* S)
{
 int x;
 LoadVar(S,x);
 IF (x<0, "bad integer");
 return x;
}

/*
** 读取一个`lua_Number`变量
 */
static lua_Number LoadNumber(LoadState* S)
{
 lua_Number x;
 LoadVar(S,x);
 return x;
}

/*
** 读取一个`string`变量
 */
static TString* LoadString(LoadState* S)
{
 size_t size;
 LoadVar(S,size);
 if (size==0)
  return NULL;
 else
 {
  char* s=luaZ_openspace(S->L,S->b,size);
  LoadBlock(S,s,size);
  return luaS_newlstr(S->L,s,size-1);		/* 删除末尾'\0' */ /* remove trailing '\0' */
 }
}

/*
** 加载指令列表, 填充原型`proto`
 */
static void LoadCode(LoadState* S, Proto* f)
{
 int n=LoadInt(S);
 f->code=luaM_newvector(S->L,n,Instruction);
 f->sizecode=n;
 LoadVector(S,f->code,n,sizeof(Instruction));
}

static Proto* LoadFunction(LoadState* S, TString* p);

/*
** 加载常量列表, 填充原型`proto`. 与`LoadFunction`函数相互递归调用
** `lundump`将嵌套子函数当作常量表
 */
static void LoadConstants(LoadState* S, Proto* f)
{
 int i,n;
 n=LoadInt(S); /* 常量个数 */
 f->k=luaM_newvector(S->L,n,TValue);       /* 创建常量数组 */
 f->sizek=n;
 for (i=0; i<n; i++) setnilvalue(&f->k[i]);/* 清空数组 */
 /* 读入实际值 */
 for (i=0; i<n; i++)
 {
  TValue* o=&f->k[i];
  int t=LoadChar(S);
  switch (t)
  {
   case LUA_TNIL:  /* nil值 */
   	setnilvalue(o);
	break;
   case LUA_TBOOLEAN: /* 布尔值 */
   	setbvalue(o,LoadChar(S)!=0);
	break;
   case LUA_TNUMBER:  /* 数值 */
	setnvalue(o,LoadNumber(S));
	break;
   case LUA_TSTRING:  /* 字符串 */
	setsvalue2n(S->L,o,LoadString(S));
	break;
   default:
	error(S,"bad constant");
	break;
  }
 }
 n=LoadInt(S); /* 嵌套函数个数 */
 f->p=luaM_newvector(S->L,n,Proto*);     /* 创建函数原型数组 */
 f->sizep=n;
 for (i=0; i<n; i++) f->p[i]=NULL;
 for (i=0; i<n; i++) f->p[i]=LoadFunction(S,f->source); /* 加载函数. 缺省源文件名为父函数的源文件名 */
}

/*
** 加载调试信息, 填充原型`f`
 */
static void LoadDebug(LoadState* S, Proto* f)
{
 int i,n;
 n=LoadInt(S);
 f->lineinfo=luaM_newvector(S->L,n,int);        /* 创建行信息数组 */
 f->sizelineinfo=n;
 LoadVector(S,f->lineinfo,n,sizeof(int));
 n=LoadInt(S);
 f->locvars=luaM_newvector(S->L,n,LocVar);      /* 创建局部变量数组 */
 f->sizelocvars=n;
 for (i=0; i<n; i++) f->locvars[i].varname=NULL;/* 清空局部变量数组 */
 /* 读入实际值 */
 for (i=0; i<n; i++)
 {
  f->locvars[i].varname=LoadString(S);
  f->locvars[i].startpc=LoadInt(S);
  f->locvars[i].endpc=LoadInt(S);
 }
 n=LoadInt(S);
 f->upvalues=luaM_newvector(S->L,n,TString*);  /* 创建上值数组 */
 f->sizeupvalues=n;
 for (i=0; i<n; i++) f->upvalues[i]=NULL;      /* 清空上值数组 */
 /* 读入实际值 */
 for (i=0; i<n; i++) f->upvalues[i]=LoadString(S);
}

/*
** 加载函数. 与`LoadConstants`函数相互递归调用
 */
static Proto* LoadFunction(LoadState* S, TString* p)
{
 Proto* f;
 /* 检查函数嵌套深度 */
 if (++S->L->nCcalls > LUAI_MAXCCALLS) error(S,"code too deep");
 f=luaF_newproto(S->L);
 setptvalue2s(S->L,S->L->top,f); incr_top(S->L);  /* f入栈 */
 f->source=LoadString(S); if (f->source==NULL) f->source=p; /* 源文件为空, 则取父函数源文件 */
 f->linedefined=LoadInt(S);     /* 读取函数定义起始行号 */
 f->lastlinedefined=LoadInt(S); /* 读取函数定义终止行号 */
 f->nups=LoadByte(S);           /* 读取上值个数 */
 f->numparams=LoadByte(S);      /* 读取参数个数 */
 f->is_vararg=LoadByte(S);      /* 读取是否可变参数标识 */
 f->maxstacksize=LoadByte(S);
 LoadCode(S,f);                 /* 加载指令列表 */
 LoadConstants(S,f);            /* 加载常量列表 */
 LoadDebug(S,f);                /* 加载调试信息 */
 IF (!luaG_checkcode(f), "bad code"); /* 模拟代码运行过程, 执行检查代码 */
 /* 还原栈顶和嵌套层数 */
 S->L->top--;
 S->L->nCcalls--;
 return f;
}

/*
** 加载文件头
 */
static void LoadHeader(LoadState* S)
{
 char h[LUAC_HEADERSIZE];
 char s[LUAC_HEADERSIZE];
 luaU_header(h);
 LoadBlock(S,s,LUAC_HEADERSIZE);
 IF (memcmp(h,s,LUAC_HEADERSIZE)!=0, "bad header");
}

/*
** load precompiled chunk
*/
/*
** 加载lua预编译程序块. 函数原型同`luaY_parser`
 */
Proto* luaU_undump (lua_State* L, ZIO* Z, Mbuffer* buff, const char* name)
{
 LoadState S;
 if (*name=='@' || *name=='=')
  S.name=name+1;
 else if (*name==LUA_SIGNATURE[0])
  S.name="binary string";
 else
  S.name=name;
 S.L=L;
 S.Z=Z;
 S.b=buff;
 LoadHeader(&S);
 return LoadFunction(&S,luaS_newliteral(L,"=?")); /* 将整个程序文件看成一个函数, 缺省源文件名为=? */
}

/*
* make header
*/
/*
** 生成文件头
 */
void luaU_header (char* h)
{
 int x=1;
 memcpy(h,LUA_SIGNATURE,sizeof(LUA_SIGNATURE)-1); /* 头部签名 `<esc>Lua`或`0x1B4C7561`, 通过检查该签名确定文件是二进制还是文本 */
 h+=sizeof(LUA_SIGNATURE)-1;
 *h++=(char)LUAC_VERSION;                         /* 版本号, lua5.1为`0x51` */
 *h++=(char)LUAC_FORMAT;                          /* 格式版本, 官方文本为0 */
 *h++=(char)*(char*)&x;				                    /* 字节序标识, 默认为 1. 0: 大端模式; 1: 小端模式 *//* endianness */
 *h++=(char)sizeof(int);                          /* integer数据类型大小 */
 *h++=(char)sizeof(size_t);                       /* size_t数据类型大小 */
 *h++=(char)sizeof(Instruction);                  /* opcode大小 */
 *h++=(char)sizeof(lua_Number);                   /* lua_Number数据类型大小 */
 *h++=(char)(((lua_Number)0.5)==0);		            /* 标识lua_Number类型用整数表示还是浮点数表示 */ /* is lua_Number integral? */
}
