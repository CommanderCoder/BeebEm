/*******************************************************************************
*                                                                              *
*   Define size independent data types and operations.                         *
*                                                                              *
*   The following types must be supported by all platforms:                    *
*                                                                              *
*   UINT8  - Unsigned 8-bit Integer     INT8  - Signed 8-bit integer           *
*   UINT16 - Unsigned 16-bit Integer    INT16 - Signed 16-bit integer          *
*   UINT32 - Unsigned 32-bit Integer    INT32 - Signed 32-bit integer          *
*   UINT64 - Unsigned 64-bit Integer    INT64 - Signed 64-bit integer          *
*                                                                              *
*                                                                              *
*   The macro names for the artithmatic operations are composed as follows:    *
*                                                                              *
*   XXX_R_A_B, where XXX - 3 letter operation code (ADD, SUB, etc.)            *
*                    R   - The type of the result                              *
*                    A   - The type of operand 1                               *
*                    B   - The type of operand 2 (if binary operation)         *
*                                                                              *
*                    Each type is one of: U8,8,U16,16,U32,32,U64,64            *
*                                                                              *
*******************************************************************************/


#ifndef OSD_CPU_H
#define OSD_CPU_H

#define INLINE	inline


/* The Win32 port requires this constant for variable arg routines. */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

typedef unsigned char						UINT8;
typedef signed char 						INT8;
typedef unsigned short						UINT16;
typedef signed short						INT16;

#ifndef _WINDOWS_H
typedef unsigned int						UINT32;
typedef signed int							INT32;
#ifdef _MSC_VER
typedef signed __int64						INT64;
typedef unsigned __int64					UINT64;
#else
__extension__ typedef unsigned long long	UINT64;
__extension__ typedef signed long long		INT64;
#endif
#endif

typedef UINT32			offs_t;

typedef unsigned short WORD;
typedef unsigned char  BYTE;

/* Combine two 32-bit integers into a 64-bit integer */
#define COMBINE_64_32_32(A,B)     ((((UINT64)(A))<<32) | (UINT32)(B))
#define COMBINE_U64_U32_U32(A,B)  COMBINE_64_32_32(A,B)

/* Return upper 32 bits of a 64-bit integer */
#define HI32_32_64(A)		  (((UINT64)(A)) >> 32)
#define HI32_U32_U64(A)		  HI32_32_64(A)

/* Return lower 32 bits of a 64-bit integer */
#define LO32_32_64(A)		  ((A) & 0xffffffff)
#define LO32_U32_U64(A)		  LO32_32_64(A)

#define DIV_64_64_32(A,B)	  ((A)/(B))
#define DIV_U64_U64_U32(A,B)  ((A)/(UINT32)(B))

#define MOD_32_64_32(A,B)	  ((A)%(B))
#define MOD_U32_U64_U32(A,B)  ((A)%(UINT32)(B))

#define MUL_64_32_32(A,B)	  ((A)*(INT64)(B))
#define MUL_U64_U32_U32(A,B)  ((A)*(UINT64)(UINT32)(B))

#endif	/* defined OSD_CPU_H */
