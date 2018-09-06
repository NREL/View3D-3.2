/*subfile:  types.h  *********************************************************/
/*                                                                           */
/*  This software was developed at the National Institute of Standards       */
/*  and Technology by employees of the Federal Government in the             */
/*  course of their official duties. Pursuant to title 17 Section 105        */
/*  of the United States Code this software is not subject to                */
/*  copyright protection and is in the public domain. These programs         */
/*  are experimental systems. NIST assumes no responsibility                 */
/*  whatsoever for their use by other parties, and makes no                  */
/*  guarantees, expressed or implied, about its quality, reliability,        */
/*  or any other characteristic.  We would appreciate acknowledgment         */
/*  if the software is used. This software can be redistributed and/or       */
/*  modified freely provided that any derivative works bear some             */
/*  notice that they are derived from it, and any modified versions          */
/*  bear some notice that they have been modified.                           */
/*                                                                           */
/*****************************************************************************/

#ifndef STD_TYPES
# define STD_TYPES
typedef char I1;           /* 1 byte signed integer */
typedef short I2;          /* 2 byte signed integer */
typedef long I4;           /* 4 byte signed integer */
typedef int IX;            /* default signed integer */
typedef unsigned char U1;  /* 1 byte unsigned integer */
typedef unsigned short U2; /* 2 byte unsigned integer */
typedef unsigned long U4;  /* 4 byte unsigned integer */
typedef unsigned int UX;   /* default unsigned integer */
typedef float R4;          /* 4 byte real value */
typedef double R8;         /* 8 byte real value */
typedef long double RX;    /* 10 byte real value (extended precision) */
#endif

#define LINELEN 256 /* length of string */

