/*
** $Id: lopcodes.h,v 1.125.1.1 2007/12/27 13:02:25 roberto Exp $
** Opcodes for Lua virtual machine
** See Copyright Notice in lua.h
*/

#ifndef lopcodes_h
#define lopcodes_h

#include "llimits.h"


/*===========================================================================
  We assume that instructions are unsigned numbers.
  All instructions have an opcode in the first 6 bits.
  Instructions can have the following fields:
	`A' : 8 bits
	`B' : 9 bits
	`C' : 9 bits
	`Bx' : 18 bits (`B' and `C' together)
	`sBx' : signed Bx

  A signed argument is represented in excess K; that is, the number
  value is the unsigned value minus K. K is exactly the maximum value
  for that argument (so that -max is represented by 0, and +max is
  represented by 2*max), which is half the maximum for the corresponding
  unsigned argument.
===========================================================================*/


enum OpMode {iABC, iABx, iAsBx};  /* basic instruction format */


/*
** size and position of opcode arguments.
*/
#define SIZE_C		9									/* C部分大小 */
#define SIZE_B		9									/* B部分大小 */
#define SIZE_Bx		(SIZE_C + SIZE_B)	/* Bx部分大小 */
#define SIZE_A		8									/* A部分大小 */

#define SIZE_OP		6

#define POS_OP		0
#define POS_A		(POS_OP + SIZE_OP)
#define POS_C		(POS_A + SIZE_A)
#define POS_B		(POS_C + SIZE_C)
#define POS_Bx		POS_C


/*
** limits for opcode arguments.
** we use (signed) int to manipulate most arguments,
** so they must fit in LUAI_BITSINT-1 bits (-1 for sign)
*/
#if SIZE_Bx < LUAI_BITSINT-1
#define MAXARG_Bx        ((1<<SIZE_Bx)-1)
#define MAXARG_sBx        (MAXARG_Bx>>1)         /* `sBx' is signed */
#else
#define MAXARG_Bx        MAX_INT
#define MAXARG_sBx        MAX_INT
#endif


#define MAXARG_A        ((1<<SIZE_A)-1)
#define MAXARG_B        ((1<<SIZE_B)-1)
#define MAXARG_C        ((1<<SIZE_C)-1)


/* creates a mask with `n' 1 bits at position `p' */
#define MASK1(n,p)	((~((~(Instruction)0)<<n))<<p)

/* creates a mask with `n' 0 bits at position `p' */
#define MASK0(n,p)	(~MASK1(n,p))

/*
** the following macros help to manipulate instructions
*/
/*
** 指令操作相关宏
 */

#define GET_OPCODE(i)	(cast(OpCode, ((i)>>POS_OP) & MASK1(SIZE_OP,0)))
#define SET_OPCODE(i,o)	((i) = (((i)&MASK0(SIZE_OP,POS_OP)) | \
		((cast(Instruction, o)<<POS_OP)&MASK1(SIZE_OP,POS_OP))))

#define GETARG_A(i)	(cast(int, ((i)>>POS_A) & MASK1(SIZE_A,0)))
#define SETARG_A(i,u)	((i) = (((i)&MASK0(SIZE_A,POS_A)) | \
		((cast(Instruction, u)<<POS_A)&MASK1(SIZE_A,POS_A))))

#define GETARG_B(i)	(cast(int, ((i)>>POS_B) & MASK1(SIZE_B,0)))
#define SETARG_B(i,b)	((i) = (((i)&MASK0(SIZE_B,POS_B)) | \
		((cast(Instruction, b)<<POS_B)&MASK1(SIZE_B,POS_B))))

#define GETARG_C(i)	(cast(int, ((i)>>POS_C) & MASK1(SIZE_C,0)))
#define SETARG_C(i,b)	((i) = (((i)&MASK0(SIZE_C,POS_C)) | \
		((cast(Instruction, b)<<POS_C)&MASK1(SIZE_C,POS_C))))

#define GETARG_Bx(i)	(cast(int, ((i)>>POS_Bx) & MASK1(SIZE_Bx,0)))
#define SETARG_Bx(i,b)	((i) = (((i)&MASK0(SIZE_Bx,POS_Bx)) | \
		((cast(Instruction, b)<<POS_Bx)&MASK1(SIZE_Bx,POS_Bx))))

#define GETARG_sBx(i)	(GETARG_Bx(i)-MAXARG_sBx)
#define SETARG_sBx(i,b)	SETARG_Bx((i),cast(unsigned int, (b)+MAXARG_sBx))


#define CREATE_ABC(o,a,b,c)	((cast(Instruction, o)<<POS_OP) \
			| (cast(Instruction, a)<<POS_A) \
			| (cast(Instruction, b)<<POS_B) \
			| (cast(Instruction, c)<<POS_C))

#define CREATE_ABx(o,a,bc)	((cast(Instruction, o)<<POS_OP) \
			| (cast(Instruction, a)<<POS_A) \
			| (cast(Instruction, bc)<<POS_Bx))


/*
** Macros to operate RK indices
*/

/* this bit 1 means constant (0 means register) */
#define BITRK		(1 << (SIZE_B - 1))

/* test whether value is a constant */
/* 测试是否常量 */
#define ISK(x)		((x) & BITRK)

/* gets the index of the constant */
#define INDEXK(r)	((int)(r) & ~BITRK)

#define MAXINDEXRK	(BITRK - 1)

/* code a constant index as a RK value */
#define RKASK(x)	((x) | BITRK)


/*
** invalid register that fits in 8 bits
*/
#define NO_REG		MAXARG_A


/*
** R(x) - register
** Kst(x) - constant (in constant table)
** RK(x) == if ISK(x) then Kst(INDEXK(x)) else R(x)
*/
/*
** R(x)表示第X个寄存器;
** K(X)表示第X个常数;
** RK(X)当 X < k(内置参数,一般为250)时, 表示R(X); 否则表示K(X-k)
** G[X]表示全局变量表的X域;
** U[X]表示第X个upvalue
 */

/*
** grep "ORDER OP" if you change these enums
*/
/* Lua虚拟机中的指令将32位分成三或四个域.
** OP域为指令部分, 占6位, 其他域表示操作数;
** A域总是占用8位, B域和C域分别占9位, 合起来可以组成18位的域: Bx(unsigned)和sBx(signed).
** 大部分指令使用三地址格式, 其中A表示目的操作数, 一般为寄存器; B, C分别表示源操作数, 一般为寄存器或常数(形如RK(X)). 寄存器通常是指向当前栈帧中的索引, 0号寄存器是栈底位置.
** 不像C API那样可以支持负索引. 栈顶索引会被编码为特定的操作数(通常是0).
** Lua许多典型操作都可以编码为一条指令.
** 如对一个局部变量进行自增运算, `a = a + 1`可编码成`ADD x x y`, 其中x表示局部变量`a`所在寄存器, `y`表示常数1.
** 当`a`, `b`同为局部变量时,如 赋值语句`a=b.f`可编码为一条指令`GETTABLE x y z`, 其中`x`表示`a`所在寄存器, `y`为`b`所在寄存器, `z`为字符串常量"f"的索引
** (Lua中`b.f`是	b["f"]`的一种变体）
 */
/*
** 指令布局

 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
+-----------+--------------------+-------------------------+------------------------+
|           |                    |                         |                        |
|   OP:6    |        A:8         |           C:9           |          B:9           |
|           |                    |                         |                        |
+-----------+--------------------+-------------------------+------------------------+
|           |                    |                                                  |
|   OP:6    |        A:8         |                      Bx:18                       |
|           |                    |                                                  |
+-----------+--------------------+--------------------------------------------------+
|           |                    |                                                  |
|   OP:6    |        A:8         |                      sBx:18                      |
|           |                    |                                                  |
+-----------+--------------------+--------------------------------------------------+
 */

typedef enum {
/*----------------------------------------------------------------------
name		args	description
------------------------------------------------------------------------*/
OP_MOVE,/*	A B	R(A) := R(B)					*/										/* 将寄存器B赋给寄存器A */
OP_LOADK,/*	A Bx	R(A) := Kst(Bx)					*/								/* 将常量Bx载入寄存器A */
OP_LOADBOOL,/*	A B C	R(A) := (Bool)B; if (C) pc++			*/	/* 将布尔值B载入寄存器A */
OP_LOADNIL,/*	A B	R(A) := ... := R(B) := nil			*/				/* 将nil赋给一系列寄存器 */
OP_GETUPVAL,/*	A B	R(A) := UpValue[B]				*/						/* 读取upvalue b, 写入寄存器A */

OP_GETGLOBAL,/*	A Bx	R(A) := Gbl[Kst(Bx)]				*/				/* 读取全局变量Bx, 写入寄存器A */
OP_GETTABLE,/*	A B C	R(A) := R(B)[RK(C)]				*/					/* 读取表B元素C, 写入寄存器A */

OP_SETGLOBAL,/*	A Bx	Gbl[Kst(Bx)] := R(A)				*/				/* 读取寄存器A, 写入全局变量Bx */
OP_SETUPVAL,/*	A B	UpValue[B] := R(A)				*/						/* 读取寄存器A, 写入upvalue B*/
OP_SETTABLE,/*	A B C	R(A)[RK(B)] := RK(C)				*/				/* 读取寄存器C, 写入表A元素B */

OP_NEWTABLE,/*	A B C	R(A) := {} (size = B,C)				*/			/* 创建表 */

OP_SELF,/*	A B C	R(A+1) := R(B); R(A) := R(B)[RK(C)]		*/	/* 为方法调用准备 */

OP_ADD,/*	A B C	R(A) := RK(B) + RK(C)				*/							/* 加法操作 */
OP_SUB,/*	A B C	R(A) := RK(B) - RK(C)				*/							/* 减法操作 */
OP_MUL,/*	A B C	R(A) := RK(B) * RK(C)				*/							/* 乘法操作 */
OP_DIV,/*	A B C	R(A) := RK(B) / RK(C)				*/							/* 除法操作 */
OP_MOD,/*	A B C	R(A) := RK(B) % RK(C)				*/							/* 取模(余数)操作 */
OP_POW,/*	A B C	R(A) := RK(B) ^ RK(C)				*/							/* 取幂操作 */
OP_UNM,/*	A B	R(A) := -R(B)					*/											/* 取负操作 */
OP_NOT,/*	A B	R(A) := not R(B)				*/										/* 逻辑非操作 */
OP_LEN,/*	A B	R(A) := length of R(B)				*/							/* 取表长操作 */
OP_CONCAT,/*	A B C	R(A) := R(B).. ... ..R(C)			*/				/* 字符串连接操作 */

OP_JMP,/*	sBx	pc+=sBx					*/														/* 无条件跳转 */

OP_EQ,/*	A B C	if ((RK(B) == RK(C)) ~= A) then pc++		*/	/* 相等条件测试 */
OP_LT,/*	A B C	if ((RK(B) <  RK(C)) ~= A) then pc++  		*//* 小于条件测试 */
OP_LE,/*	A B C	if ((RK(B) <= RK(C)) ~= A) then pc++  		*//* 小于等于条件测试 */

OP_TEST,/*	A C	if not (R(A) <=> C) then pc++			*/ 				/* 布尔测试, 带条件跳转 */
OP_TESTSET,/*	A B C	if (R(B) <=> C) then R(A) := R(B) else pc++	*/ /* 布尔测试, 带条件跳转和赋值 */

OP_CALL,/*	A B C	R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) */ /* 闭包(函数)调用 */
OP_TAILCALL,/*	A B C	return R(A)(R(A+1), ... ,R(A+B-1))		*/ /* 尾调用 */
OP_RETURN,/*	A B	return R(A), ... ,R(A+B-2)	(see note)	*/		/* 函数调用返回 */
/*
数字型for循环的初始值, 终止值, 步长分别存放在R(A), R(A + 1), R(A + 2)中, 循环变量i则存放在R(A + 3)中
对于循环语句

for i = 1, 10, 1 do
   print(i)
end

可以展开为
// OP_FORPREP指令部分
	_index = _index - _step;	// R(A)-=R(A+2);
	jmp for_prep;							// pc+=sBx

// 循环体
loop:
	print(i);

// OP_FORLOOP指令部分
for_prep:
  _index = _index + _step;	// R(A)+=R(A+2);
if (_index < _limit) {			// if R(A) less than R(A+1) then {
  jmp loop;									// pc+=sBx;
  i = _index;								// R(A+3)=R(A)
}

 */
OP_FORLOOP,/*	A sBx	R(A)+=R(A+2);
			if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }*/				/* 数字型 for 循环 */
OP_FORPREP,/*	A sBx	R(A)-=R(A+2); pc+=sBx				*/							/* 初始化数字型 for循环 */

OP_TFORLOOP,/*	A C	R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); 
                        if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++	*/ /* 泛型 for 循环 */
OP_SETLIST,/*	A B C	R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B	*/	/* 设置表一系列数组元素 */

OP_CLOSE,/*	A 	close all variables in the stack up to (>=) R(A)*/			/* 关闭用作 upvalue 的一系列局部变量 */
OP_CLOSURE,/*	A Bx	R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))	*/ /* 创建一函数原型的闭包 */

OP_VARARG/*	A B	R(A), R(A+1), ..., R(A+B-1) = vararg		*/				/* 将可变数量参数赋给寄存器 */
} OpCode;


#define NUM_OPCODES	(cast(int, OP_VARARG) + 1)



/*===========================================================================
  Notes:
  (*) In OP_CALL, if (B == 0) then B = top. C is the number of returns - 1,
      and can be 0: OP_CALL then sets `top' to last_result+1, so
      next open instruction (OP_CALL, OP_RETURN, OP_SETLIST) may use `top'.

  (*) In OP_VARARG, if (B == 0) then use actual number of varargs and
      set top (like in OP_CALL with C == 0).

  (*) In OP_RETURN, if (B == 0) then return up to `top'

  (*) In OP_SETLIST, if (B == 0) then B = `top';
      if (C == 0) then next `instruction' is real C

  (*) For comparisons, A specifies what condition the test should accept
      (true or false).

  (*) All `skips' (pc++) assume that next instruction is a jump
===========================================================================*/


/*
** masks for instruction properties. The format is:
** bits 0-1: op mode
** bits 2-3: C arg mode
** bits 4-5: B arg mode
** bit 6: instruction set register A
** bit 7: operator is a test
*/  
/*
** 指令位掩码. 格式为
** 0-1位: op mode
** bits 2-3: C arg mode
** bits 4-5: B arg mode
** bit 6: instruction set register A
** 第7位: 表示是否测试指令
*/

enum OpArgMask {
  OpArgN,  /* argument is not used */
  OpArgU,  /* argument is used */
  OpArgR,  /* argument is a register or a jump offset */
  OpArgK   /* argument is a constant or register/constant */
};

LUAI_DATA const lu_byte luaP_opmodes[NUM_OPCODES];

#define getOpMode(m)	(cast(enum OpMode, luaP_opmodes[m] & 3))
#define getBMode(m)	(cast(enum OpArgMask, (luaP_opmodes[m] >> 4) & 3))
#define getCMode(m)	(cast(enum OpArgMask, (luaP_opmodes[m] >> 2) & 3))
#define testAMode(m)	(luaP_opmodes[m] & (1 << 6))
#define testTMode(m)	(luaP_opmodes[m] & (1 << 7))


LUAI_DATA const char *const luaP_opnames[NUM_OPCODES+1];  /* opcode names */


/* number of list items to accumulate before a SETLIST instruction */
#define LFIELDS_PER_FLUSH	50


#endif
/*
>a=6
; function [0] definition (level 1)
; 0 upvalues, 0 params, 2 stacks
.function  0 0 2 2
.const  "a"  ; 常量索引0
.const  6  ; 常量索引1
[1] loadk      0   1        ; 将数值6(常量索引1)载入0号寄存器
[2] setglobal  0   0        ; 将数值6(0号寄存器)赋给常量"a"(常量索引0)为键的表元素
[3] return     0   1				 ; 系统总是添加多余的return
; end of function
 */
/* 局部变量驻留在栈中,它们在生存期内占用 1 个栈(或寄存器)位置. 作用域通过起始程序计数器( pc)位置和截止程序计数器位置指定 */
/*
>local a="hello"
; function [0] definition (level 1)
; 0 upvalues, 0 params, 2 stacks
.function  0 0 2 2
.local  "a"  ; 局部变量索引0
.const  "hello"  ; 常量索引0
[1] loadk      0   0        ; 将常量"hello"(常量索引0)载入局部变量"a"(局部变量索引0)
[2] return     0   1
; end of function
 */
