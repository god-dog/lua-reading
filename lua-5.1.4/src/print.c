/*
** $Id: print.c,v 1.55a 2006/05/31 13:30:05 lhf Exp $
** print bytecodes
** See Copyright Notice in lua.h
*/

/*
** 打印字节码
 */
#include <ctype.h>
#include <stdio.h>

#define luac_c
#define LUA_CORE

#include "ldebug.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lundump.h"

#define PrintFunction	luaU_print

#define Sizeof(x)	((int)sizeof(x))
#define VOID(p)		((const void*)(p))

/*
** 打印字符串. 输出Lua TString类型
 */
static void PrintString(const TString* ts)
{
 const char* s=getstr(ts);
 size_t i,n=ts->tsv.len;
 putchar('"');
 for (i=0; i<n; i++)
 {
  int c=s[i];
  switch (c)
  {
   case '"': printf("\\\""); break;
   case '\\': printf("\\\\"); break;
   case '\a': printf("\\a"); break;
   case '\b': printf("\\b"); break;
   case '\f': printf("\\f"); break;
   case '\n': printf("\\n"); break;
   case '\r': printf("\\r"); break;
   case '\t': printf("\\t"); break;
   case '\v': printf("\\v"); break;
   default:	if (isprint((unsigned char)c))
   			putchar(c);
		else
			printf("\\%03u",(unsigned char)c);
  }
 }
 putchar('"');
}

/*
** 打印常量值
 */
static void PrintConstant(const Proto* f, int i)
{
 const TValue* o=&f->k[i];
 switch (ttype(o))
 {
  case LUA_TNIL:
	printf("nil");
	break;
  case LUA_TBOOLEAN:
	printf(bvalue(o) ? "true" : "false");
	break;
  case LUA_TNUMBER:
	printf(LUA_NUMBER_FMT,nvalue(o));
	break;
  case LUA_TSTRING:
	PrintString(rawtsvalue(o));
	break;
  default:				/* cannot happen */
	printf("? type=%d",ttype(o));
	break;
 }
}

/*
** 打印指令列表
 */
static void PrintCode(const Proto* f)
{
 const Instruction* code=f->code;
 /* `pc`: programming counter. 程序计数器, 指令(字节码)行号 */
 int pc,n=f->sizecode;
 /* 遍历指令列表 */
 for (pc=0; pc<n; pc++)
 {
  /* 指令二进制值 */
  Instruction i=code[pc];
  /* `o` 为指令名称索引  */
  /* OpCode o=(((OpCode)(((i)>>0) & ((~((~(Instruction)0)<<6))<<0)))); */
  OpCode o=GET_OPCODE(i);
  /* int a=(((int)(((i)>>(0 + 6)) & ((~((~(Instruction)0)<<8))<<0)))); */
  int a=GETARG_A(i);
  /* int b=(((int)(((i)>>(((0 + 6) + 8) + 9)) & ((~((~(Instruction)0)<<9))<<0)))); */
  int b=GETARG_B(i);
  /* int c=(((int)(((i)>>((0 + 6) + 8)) & ((~((~(Instruction)0)<<9))<<0)))); */
  int c=GETARG_C(i);
  /* int bx=(((int)(((i)>>((0 + 6) + 8)) & ((~((~(Instruction)0)<<(9 + 9)))<<0)))); */
  int bx=GETARG_Bx(i);
  /* int sbx=((((int)(((i)>>((0 + 6) + 8)) & ((~((~(Instruction)0)<<(9 + 9)))<<0))))-(((1<<(9 + 9))-1)>>1)); */
  int sbx=GETARG_sBx(i);
  /* `line`为当前指令对应lua源文件行号 */
  int line=getline(f,pc);
  /* 打印指令行号. 指令行号从1开始 */
  printf("\t%d\t",pc+1);
  /* 打印lua源文件行号. 形式为[line] */
  if (line>0) printf("[%d]\t",line); else printf("[-]\t");
  /* 打印操作码(指令名称) */
  printf("%-9s\t",luaP_opnames[o]);
  /* 打印操作数 */
  switch (getOpMode(o))
  {
   case iABC:
    printf("%d",a);
    if (getBMode(o)!=OpArgN) printf(" %d",ISK(b) ? (-1-INDEXK(b)) : b);
    if (getCMode(o)!=OpArgN) printf(" %d",ISK(c) ? (-1-INDEXK(c)) : c);
    break;
   case iABx:
    if (getBMode(o)==OpArgK) printf("%d %d",a,-1-bx); else printf("%d %d",a,bx);
    break;
   case iAsBx:
    if (o==OP_JMP) printf("%d",sbx); else printf("%d %d",a,sbx);
    break;
  }
  /* 打印分号之后的部分, lua源码对应的常量名, 变量名, 函数名, 包名等 */
  switch (o)
  {
   case OP_LOADK:
    printf("\t; "); PrintConstant(f,bx);
    break;
   case OP_GETUPVAL:
   case OP_SETUPVAL:
    printf("\t; %s", (f->sizeupvalues>0) ? getstr(f->upvalues[b]) : "-");
    break;
   case OP_GETGLOBAL:
   case OP_SETGLOBAL:
    printf("\t; %s",svalue(&f->k[bx]));
    break;
   case OP_GETTABLE:
   case OP_SELF:
    if (ISK(c)) { printf("\t; "); PrintConstant(f,INDEXK(c)); }
    break;
   case OP_SETTABLE:
   case OP_ADD:
   case OP_SUB:
   case OP_MUL:
   case OP_DIV:
   case OP_POW:
   case OP_EQ:
   case OP_LT:
   case OP_LE:
    if (ISK(b) || ISK(c))
    {
     printf("\t; ");
     if (ISK(b)) PrintConstant(f,INDEXK(b)); else printf("-");
     printf(" ");
     if (ISK(c)) PrintConstant(f,INDEXK(c)); else printf("-");
    }
    break;
   case OP_JMP:
   case OP_FORLOOP:
   case OP_FORPREP:
    printf("\t; to %d",sbx+pc+2);
    break;
   case OP_CLOSURE:
    printf("\t; %p",VOID(f->p[bx]));
    break;
   case OP_SETLIST:
    if (c==0) printf("\t; %d",(int)code[++pc]);
    else printf("\t; %d",c);
    break;
   default:
    break;
  }
  printf("\n");
 }
}

#define SS(x)	(x==1)?"":"s"
#define S(x)	x,SS(x)

/*
** 打印基本头信息, 如文件名称, 指令个数, 字节码大小, 常量个数, 局部变量个数, 函数个数等
 */
static void PrintHeader(const Proto* f)
{
 const char* s=getstr(f->source);
 if (*s=='@' || *s=='=')
  s++;
 else if (*s==LUA_SIGNATURE[0])
  s="(bstring)";
 else
  s="(string)";
 printf("\n%s <%s:%d,%d> (%d instruction%s, %d bytes at %p)\n",
 	(f->linedefined==0)?"main":"function",s,
	f->linedefined,f->lastlinedefined,
	S(f->sizecode),f->sizecode*Sizeof(Instruction),VOID(f));
 printf("%d%s param%s, %d slot%s, %d upvalue%s, ",
	f->numparams,f->is_vararg?"+":"",SS(f->numparams),
	S(f->maxstacksize),S(f->nups));
 printf("%d local%s, %d constant%s, %d function%s\n",
	S(f->sizelocvars),S(f->sizek),S(f->sizep));
}

/*
** 打印常量信息
 */
static void PrintConstants(const Proto* f)
{
 int i,n=f->sizek;
 printf("constants (%d) for %p:\n",n,VOID(f));
 for (i=0; i<n; i++)
 {
  printf("\t%d\t",i+1);
  PrintConstant(f,i);
  printf("\n");
 }
}

/*
** 打印局部变量信息
 */
static void PrintLocals(const Proto* f)
{
 int i,n=f->sizelocvars;
 printf("locals (%d) for %p:\n",n,VOID(f));
 for (i=0; i<n; i++)
 {
  printf("\t%d\t%s\t%d\t%d\n",
  i,getstr(f->locvars[i].varname),f->locvars[i].startpc+1,f->locvars[i].endpc+1);
 }
}

/*
** 打印UpValue信息
 */
static void PrintUpvalues(const Proto* f)
{
 int i,n=f->sizeupvalues;
 printf("upvalues (%d) for %p:\n",n,VOID(f));
 if (f->upvalues==NULL) return;
 for (i=0; i<n; i++)
 {
  printf("\t%d\t%s\n",i,getstr(f->upvalues[i]));
 }
}

/*
** 打印主函数信息. 顶层为`main`
 */
void PrintFunction(const Proto* f, int full)
{
 int i,n=f->sizep;
 PrintHeader(f);
 PrintCode(f);
 /* 参数为"-l -l", 完整打印, 包括常量区, 局部变量区, UpValue区信息 */
 if (full)
 {
  PrintConstants(f);
  PrintLocals(f);
  PrintUpvalues(f);
 }
 /* 递归打印内部定义函数 */
 for (i=0; i<n; i++) PrintFunction(f->p[i],full);
}
