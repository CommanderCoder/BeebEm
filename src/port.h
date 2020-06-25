/****************************************************************************/
/*              Beebem - (c) David Alan Gilbert 1994                        */
/*              ------------------------------------                        */
/* This program may be distributed freely within the following restrictions:*/
/*                                                                          */
/* 1) You may not charge for this program or for any part of it.            */
/* 2) This copyright message must be distributed with all copies.           */
/* 3) This program must be distributed complete with source code.  Binary   */
/*    only distribution is not permitted.                                   */
/* 4) The author offers no warrenties, or guarentees etc. - you use it at   */
/*    your own risk.  If it messes something up or destroys your computer   */
/*    thats YOUR problem.                                                   */
/* 5) You may use small sections of code from this program in your own      */
/*    applications - but you must acknowledge its use.  If you plan to use  */
/*    large sections then please ask the author.                            */
/*                                                                          */
/* If you do not agree with any of the above then please do not use this    */
/* program.                                                                 */
/* Please report any problems to the author at beebem@treblig.org           */
/****************************************************************************/

#ifndef PORT_HEADER
#define PORT_HEADER

#include <limits.h>

/* Used for accelerating copies */
#ifdef WIN32
typedef __int64 EightByteType;	// $NRM for MSVC. Will it work though?
#else
typedef long long EightByteType;
#endif

/* Used to keep a count of total number of cycles executed */
typedef int CycleCountT;

#define CycleCountTMax INT_MAX
#define CycleCountWrap (INT_MAX / 2)

#define DEFAULTSAMPLERATE 40000

#endif

