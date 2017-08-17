/*
** $Id: ldump.c,v 2.8.1.1 2007/12/27 13:02:25 roberto Exp $
** save precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#include <stddef.h>

#define ldump_c
#define LUA_CORE

#include "lua.h"

#include "lobject.h"
#include "lstate.h"
#include "lundump.h"

typedef struct {
 lua_State* L;
 lua_Writer writer;
 void* data;
 int strip;
 int status;
} DumpState;

#define DumpMem(b,n,size,D)	DumpBlock(b,(n)*(size),D)
#define DumpVar(x,D)	 	DumpMem(&x,1,sizeof(x),D)

static void DumpBlock(const void* b, size_t size, DumpState* D)
{
 if (D->status==0)
 {
  lua_unlock(D->L);
  D->status=(*D->writer)(D->L,b,size,D->data);
  lua_lock(D->L);
 }
}

static void DumpChar(int y, DumpState* D)
{
 char x=(char)y;
 DumpVar(x,D);
}

static void DumpInt(int x, DumpState* D)
{
 DumpVar(x,D);
}

static void DumpNumber(lua_Number x, DumpState* D)
{
 DumpVar(x,D);
}

static void DumpVector(const void* b, int n, size_t size, DumpState* D)
{
 DumpInt(n,D);
 DumpMem(b,n,size,D);
}

static void DumpString(const TString* s, DumpState* D)
{
 if (s==NULL || getstr(s)==NULL)
 {
  size_t size=0;
  DumpVar(size,D);
 }
 else
 {
  size_t size=s->tsv.len+1;		/* include trailing '\0' */
  DumpVar(size,D);
  DumpBlock(getstr(s),size,D);
 }
}

#define DumpCode(f,D)	 DumpVector(f->code,f->sizecode,sizeof(Instruction),D)

static void DumpFunction(const Proto* f, const TString* p, DumpState* D);

static void DumpConstants(const Proto* f, DumpState* D)
{
 int i,n=f->sizek;
 DumpInt(n,D);
 for (i=0; i<n; i++)
 {
  const TValue* o=&f->k[i];
  DumpChar(ttype(o),D);
  switch (ttype(o))
  {
   case LUA_TNIL:
	break;
   case LUA_TBOOLEAN:
	DumpChar(bvalue(o),D);
	break;
   case LUA_TNUMBER:
	DumpNumber(nvalue(o),D);
	break;
   case LUA_TSTRING:
	DumpString(rawtsvalue(o),D);
	break;
   default:
	lua_assert(0);			/* cannot happen */
	break;
  }
 }
 n=f->sizep;
 DumpInt(n,D);
 for (i=0; i<n; i++) DumpFunction(f->p[i],f->source,D);
}

/*
** 写入调试信息
 */
static void DumpDebug(const Proto* f, DumpState* D)
{
 int i,n;
 n= (D->strip) ? 0 : f->sizelineinfo;
 DumpVector(f->lineinfo,n,sizeof(int),D);         /* 源码位置 */
 n= (D->strip) ? 0 : f->sizelocvars;
 DumpInt(n,D);                                    /* 局部变量列表 */
 for (i=0; i<n; i++)
 {
  DumpString(f->locvars[i].varname,D);
  DumpInt(f->locvars[i].startpc,D);
  DumpInt(f->locvars[i].endpc,D);
 }
 n= (D->strip) ? 0 : f->sizeupvalues;             /* upvalue列表 */
 DumpInt(n,D);
 for (i=0; i<n; i++) DumpString(f->upvalues[i],D);
}

static void DumpFunction(const Proto* f, const TString* p, DumpState* D)
{
 DumpString((f->source==p || D->strip) ? NULL : f->source,D); /* 源文件名 */
 DumpInt(f->linedefined,D);                                   /* 定义开始行 */
 DumpInt(f->lastlinedefined,D);                               /* 定义结束行 */
 DumpChar(f->nups,D);                                         /* upvalue数量 */
 DumpChar(f->numparams,D);                                    /* 参数数量 */
 DumpChar(f->is_vararg,D);                                    /* 是否可变参数标识 */
 DumpChar(f->maxstacksize,D);                                 /* 最大栈大小 */
 DumpCode(f,D);                                               /* 指令列表 */
 DumpConstants(f,D);                                          /* 常量列表 */
 DumpDebug(f,D);                                              /* 调试信息相关 */
}

/*
** 写入文件头
 */
static void DumpHeader(DumpState* D)
{
 char h[LUAC_HEADERSIZE];
 luaU_header(h);
 DumpBlock(h,LUAC_HEADERSIZE,D);
}

/*
** dump Lua function as precompiled chunk
*/
/*
** 将程序预编译结果写入文件
 */
int luaU_dump (lua_State* L, const Proto* f, lua_Writer w, void* data, int strip)
{
 DumpState D;
 D.L=L;
 D.writer=w;
 D.data=data;
 D.strip=strip;
 D.status=0;
 /* 写入文件头 */
 DumpHeader(&D);
 /* 写入顶层函数 */
 DumpFunction(f,NULL,&D);
 return D.status;
}
