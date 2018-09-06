/*subfile:  heap.c  **********************************************************/
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

/*   functions for heap (memory) processing  */

#define MEMTEST 0   /* 2 = list allocations; 1 = check guard bytes */

#include <stdio.h>
#include <stdlib.h> /* prototype: malloc, free */
#include <string.h> /* prototype: memset */
#include "types.h" 
#include "view3d.h"
#include "prtyp.h" 

#ifdef __TURBOC__              /* TURBO C compiler */
# include <alloc.h> /* prototype: heapcheck, heapwalk */
#endif

#ifdef __WATCOMC__      /* WATCOM C compiler */
# include <malloc.h>/* prototype: _heapchk, _heapwalk */
# include "port.h"
#endif

#ifdef _MSC_VER         /* VISUAL C++ compiler */
# include <malloc.h>/* prototype: _heapchk, _heapwalk */
#endif

extern FILE *_ulog; /* identifier of output file */
extern I1 _string[LINELEN];  /* buffer for a character string */

#define MCHECK 0x5A5A5A5AL

/***  Alc_E.c  ***************************************************************/

/*  Allocate memory for a single element, i.e. contiguous portion of
 *  the heap.  This may be a single structure or a vector.  */

/* NOTES:
 *   All memory allocations and de-allocations should go through Alc_E and 
 * Fre_E to allow some useful heap checking options.
 *   MEMTEST must be defined in Alc_E and Fre_E to activate the following print 
 * and test operations.  Otherwise, they are not available, but the size of
 * the executable program is reduced and its speed increased.  The test flags
 * _dmem1 and _dmem2 are global variables from vglob.h.
 *   When _dmem1 is 1, 4 guard bytes are added before and after the normal 
 * heap memory allocation.  These guard bytes help to test for accessing 
 * beyond the ends of the allocated vector -- especially for off-by-one 
 * indexing.  Based on idea and code by Paul Anderson, 
 * "Dr. Dobb's C Sourcebook", Winter 1989/90, pp 62 - 66, 94.
 *   When _dmem2 is 1, every call to Alc_E and Fre_E is noted in the 
 * output file _ulog, which is also a vglob.h global variable.
 *   Turbo C++ has some very useful functions to directly test the heap
 * integrity.  The check for an individual heap entry has been added to Chk_E. 
 * The complete heap check and listing has been placed in MemWalk.
 */

void *Alc_E( UX length, I1 *name )
/*  length; length of element (bytes) (Turbo C limit: 65535 = 8192*8).
 *  name;   name of variable being allocated.  */
  {
  U1 *p;     /* pointer to allocated memory */
#if( MEMTEST > 0 )
  U4 *pt;    /* pointer to heap guard bytes */
#endif

#ifdef __TURBOC__
  if( length > 65512L ) error( 3, __FILE__, __LINE__,
    name, " too large to allocate", "" );
#endif
  if( length <= 0L ) error( 3, __FILE__, __LINE__,
    name, " too small to allocate", "" );

#if( MEMTEST > 0 )
  p = (U1 *)malloc( length+8 );
#else
  p = (U1 *)malloc( length );
#endif

#if( MEMTEST > 1 )
  fprintf( _ulog, "alc_e %5u bytes at [%p] for: %s\n", length, p, name );
  fflush( _ulog );
#endif

  if( p == NULL )
    {
    fprintf( _ulog, "%s", MemRem( _string) );
    fprintf( _ulog, "Attempting to allocate %u bytes\n", length );
    error( 3, __FILE__, __LINE__, "Insufficient memory to allocate: ", name, "" );
    }

#if( MEMTEST > 0 )
  pt = (U4 *)p;
  *pt = MCHECK;      /* set guard bytes */
  p+= 4;
  pt = (U4 *)(p+length);
  *pt = MCHECK;
#endif

  memset( p, 0, length );    /* zero the vector */

  return (void *)p;

  }  /*  end of Alc_E  */

/***  Chk_E.c  ***************************************************************/

/*  Check pointer to allocated memory. 
//  Return non-zero if heap is in error.  */

IX Chk_E( void *pm, UX length, I1 *name )
/*  *pm;    pointer to allocated memory.
 *  length; length of element (bytes) (Turbo C limit: 65535).
 *  name;   name of variable being checked.  */
  {
  IX status=0;
#if( MEMTEST > 0 )
  U1 *p;     /* pointer to allocated memory */
  U4 *pt;    /* pointer to guard bytes */

  p = (U1 *)pm + length;
  pt = (U4 *)p;
  if( *pt != MCHECK )
    {
    error( 2, __FILE__, __LINE__, "Overrun at end of: ", name, "" );
    status = 1;
    }
  p = (U1 *)pm - 4;
  pt = (U4 *)p;
  if( *pt != MCHECK )
    {
    error( 2, __FILE__, __LINE__, "Overrun at start of: ", name, "" );
    status = 1;
    }
  pm = (void *)p;
#endif

#if(__TURBOC__ >= 0x295)      /* requires Turbo C++ compiler */
  switch( heapchecknode( pm ) )
    {
    case _HEAPEMPTY:
      error( 2, __FILE__, __LINE__, "The heap is empty: ", name, "" );
      status = 1;
      break;
    case _HEAPCORRUPT:
      error( 2, __FILE__, __LINE__, "The heap is corrupted: ", name, "" );
      status = 1;
      break;
    case _BADNODE:
      error( 2, __FILE__, __LINE__, "Bad heap node entry: ", name, "" );
      status = 1;
      break;
    case _FREEENTRY:
      error( 2, __FILE__, __LINE__, "Free heap node entry: ", name, "" );
      status = 1;
      break;
    case _USEDENTRY:
      break;
    default:
      error( 2, __FILE__, __LINE__, "Unknown heap node status: ", name, "" );
      break;
    }  /* end switch */
#endif

  return status;

  }  /*  end of Chk_E  */

/***  Fre_E.c  ***************************************************************/

/*  Free pointer to previously allocated memory -- see Alc_E.
 *  Includes a memory check.  */

void *Fre_E( void *pm, UX length, I1 *name )
/*  *pm;    pointer to allocated memory.
 *  length; length of element (bytes) (16-bit limit: 65535).
 *  name;   name of variable being freed.  */
  {
#if( MEMTEST > 0 )
  IX test;
  U4 *pt;          /* pointer to guard bytes */

  test = Chk_E( (void *)pm, length, name );

  pt = (U4 *)pm;
  pt -= 1;
  pm = (void *)pt;
# if( MEMTEST > 1 )
  fprintf( _ulog, "fre_e %5u bytes at [%p] for: %s\n", length, pm, name );
  fflush( _ulog );
# endif

  if( !test )
#endif
    free( pm );

  return (NULL);

  }  /*  end of Fre_E  */

/***  Alc_MC.c  **************************************************************/

/*  Allocate (contiguously) a matrix.
 *  This matrix is accessed (and stored) in a rectangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][j  ] | [i  ][j+1] | [i  ][j+2] | [i  ][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+1][j  ] | [i+1][j+1] | [i+1][j+2] | [i+1][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+2][j  ] | [i+2][j+1] | [i+2][j+2] | [i+2][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+3][j  ] | [i+3][j+1] | [i+3][j+2] | [i+3][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *  where i is the minimum row index and
 *  j is the minimum column index (usually 0 or 1).
 */

void *Alc_MC( IX min_row_index, IX max_row_index, IX min_col_index,
              IX max_col_index, IX size, I1 *name )
/*  min_row_index;  minimum row index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
  {
  I1 **p;  /* pointer to the array of row pointers */
  IX i;     /* row number */
  IX max_index;  /* max index when treating matrix as a vector */
  IX row_size;   /* number of bytes in a row */

  p = (I1 **)Alc_V( min_row_index, max_row_index, sizeof(I1 *), name );
  max_index = min_col_index + (max_row_index - min_row_index + 1) *
              (max_col_index - min_col_index + 1) - 1;
  p[min_row_index] = (I1 *)Alc_V( min_col_index, max_index, size, name );
  row_size = (max_col_index - min_col_index + 1) * size;
  for( i=min_row_index; i<max_row_index; i++ )
    p[i+1] = p[i] + row_size;

  return ((void *)p);

  }  /*  end of Alc_MC  */

/***  Fre_MC.c  **************************************************************/

/*  Free a matrix allocated by Alc_MC.  */

void *Fre_MC( void *v, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name )
/*  min_row_index;  minimum row index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being freed.
 */
  {
  I1 **p;  /* pointer to the array of row pointers */
  IX max_index;  /* max index when treating matrix as a vector */

  p = (I1 **)v;
  max_index = min_col_index + (max_row_index - min_row_index + 1) *
              (max_col_index - min_col_index + 1) - 1;
  Fre_V( p[min_row_index], min_col_index, max_index, size, name );
  Fre_V( p, min_row_index, max_row_index, sizeof(I1 *), name );

  return (NULL);

  }  /*  end of Fre_MC  */

/***  Alc_MR.c  **************************************************************/

/*  Allocate (by rows) a matrix.
 *  This matrix is accessed (and stored) in a rectangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][j  ] | [i  ][j+1] | [i  ][j+2] | [i  ][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+1][j  ] | [i+1][j+1] | [i+1][j+2] | [i+1][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+2][j  ] | [i+2][j+1] | [i+2][j+2] | [i+2][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  | [i+3][j  ] | [i+3][j+1] | [i+3][j+2] | [i+3][j+3] |   ...
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *  where i is the minimum row index and
 *  j is the minimum column index (usually 0 or 1).
 */

void *Alc_MR( IX min_row_index, IX max_row_index, IX min_col_index,
              IX max_col_index, IX size, I1 *name )
/*  min_row_index;  minimum row index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.
 */
  {
  I1 **p;  /* pointer to the array of row pointers */
  IX j;     /* row number */

  p = (I1 **)Alc_V( min_row_index, max_row_index, sizeof(I1 *), name );
  for( j=min_row_index; j<=max_row_index; j++ )
    p[j] = (I1 *)Alc_V( min_col_index, max_col_index, size, name );

  return ((void *)p);

  }  /*  end of Alc_MR  */

/***  Fre_MR.c  **************************************************************/

/*  Free a matrix allocated by Alc_MR.  */

void *Fre_MR( void *v, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name )
/*  min_row_index;  minimum row index.
 *  max_row_index;  maximum row index.
 *  min_col_index;  minimum column index.
 *  max_col_index;  maximum column index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being freed.
 */
  {
  I1 **p;  /* pointer to the array of row pointers */
  IX j;     /* row number */

  p = (I1 **)v;
  for( j=min_row_index; j<=max_row_index; j++ )
    Fre_V( p[j], min_col_index, max_col_index, size, name );
  Fre_V( p, min_row_index, max_row_index, sizeof(I1 *), name );

  return (NULL);

  }  /*  end of Fre_MR  */

/***  Alc_MSR.c  *************************************************************/

/*  Allocate (by rows) a symmertic matrix.
 *  This matrix is accessed (and stored) in a triangular form:
 *  +------------+------------+------------+------------+-------
 *  | [i  ][i  ] |     -      |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+1][i  ] | [i+1][i+1] |     -      |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+2][i  ] | [i+2][i+1] | [i+2][i+2] |     -      |    - 
 *  +------------+------------+------------+------------+-------
 *  | [i+3][i  ] | [i+3][i+1] | [i+3][i+2] | [i+3][i+3] |    - 
 *  +------------+------------+------------+------------+-------
 *  |    ...     |    ...     |    ...     |    ...     |   ...
 *  where i is the minimum array index (usually 0 or 1).
 */

void *Alc_MSR( IX minIndex, IX maxIndex, IX size, I1 *name )
/*  minIndex;  minimum vector index:  matrix[minIndex][minIndex] valid.
 *  maxIndex;  maximum vector index:  matrix[maxIndex][maxIndex] valid.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.  */
  {
  I1 **p;  /*  pointer to the array of row pointers  */
  IX i;     /* row number */
  IX j;     /* column number */

  /* Allocate vector of row pointers; then allocate individual rows. */ 

  p = (I1 **)Alc_V( minIndex, maxIndex, sizeof(I1 *), name );
  for( j=i=maxIndex; i>=minIndex; i--,j-- )
    p[i] = (I1 *)Alc_V( minIndex, j, size, name );

  /* Vectors allocated from largest to smallest to allow
     maximum filling of holes in the heap. */

  return ((void *)p);

  }  /*  end of Alc_MSR  */

/***  Fre_MSR.c  *************************************************************/

/*  Free a symmertic matrix allocated by Alc_MSR.  */

void *Fre_MSR( void *v, IX minIndex, IX maxIndex, IX size, I1 *name )
/*  v;      pointer to allocated vector.
 *  minIndex;  minimum vector index:  matrix[minIndex][minIndex] valid.
 *  maxIndex;  maximum vector index:  matrix[maxIndex][maxIndex] valid.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being freed.  */
  {
  I1 **p;  /*  pointer to the array of row pointers  */
  IX i;     /* row number */
  IX j;     /* column number */

  p = (I1 **)v;
  for( j=i=minIndex; i<=maxIndex; i++,j++ )
    Fre_V( p[i], minIndex, j, size, name );
  Fre_V( p, minIndex, maxIndex, sizeof(I1 *), name );

  return (NULL);

  }  /*  end of Fre_MSR  */

/***  Alc_V.c  ***************************************************************/

/*  Allocate pointer for a vector with optional debugging data.
 *  This vector is accessed and stored as:
 *     v[i] v[i+1] v[i+2] ... v[N-1] v[N]
 *  where i is the minimum vector index and N is the maximum index --
 *  i need not equal zero; i may be less than zero.  
 */

void *Alc_V( IX minIndex, IX maxIndex, IX size, I1 *name )
/*  minIndex;  minimum vector index:  vector[minIndex] valid.
 *  maxIndex;  maximum vector index:  vector[maxIndex] valid.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being allocated.  */
  {
  I1 *p;      /* pointer to the vector */
  UX length;  /* length of vector (bytes) (16-bit limit: 65535) */

  if( maxIndex < minIndex )
    {
    sprintf( _string, "Max index(%d) < min index(%d): %s",
      maxIndex, minIndex, name );
    error( 3, __FILE__, __LINE__, _string, "" );
    }

  length = (UX)(maxIndex - minIndex + 1) * (UX)size;
  p = (I1 *)Alc_E( length, name );
  p -= minIndex * size;

  return ((void *)p);

  }  /*  end of Alc_V  */

/***  Fre_V.c  ***************************************************************/

/*  Free pointer to a vector allocated by Alc_V.  */

void *Fre_V( void *v, IX minIndex, IX maxIndex, IX size, I1 *name )
/*  v;      pointer to allocated vector.
 *  minIndex;  minimum vector index.
 *  maxIndex;  maximum vector index.
 *  size;   size (bytes) of one data element.
 *  name;   name of variable being freed.  */
  {
  I1 *p;       /* pointer to the vector */
  UX  length;  /* number of bytes in vector elements */

  p = (I1 *)v + minIndex * size;
  length = (UX)(maxIndex - minIndex + 1) * (UX)size;
  Fre_E( (void *)p, length, name );

  return (NULL);

  }  /*  end of Fre_V  */

#if( __TURBOC__ >= 0x295 )      /* requires Turbo C++ compiler */
/***  MemWalk.c  *************************************************************/

/*  Check the heap using non-ANSI functions.  */

IX MemWalk( void )
  {
  IX status = 0;
  struct heapinfo hp;   /* heap information */

  switch( heapcheck( ) )
    {
    case _HEAPEMPTY:
      fprintf( _ulog, "The heap is empty.\n" );
      break;
    case _HEAPOK:
      fprintf( _ulog, "The heap is O.K.\n" );
      break;
    case _HEAPCORRUPT:
      fprintf( _ulog, "The heap is corrupted.\n" );
      status = 1;
      break;
    }  /* end switch */

  fprintf( _ulog, "Heap: loc, size, used?\n" );
  hp.ptr = NULL;
  while( heapwalk( &hp ) == _HEAPOK )
    {
    fprintf( _ulog, "[%p]%8lu %s\n",
             hp.ptr, hp.size, hp.in_use ? "used" : "free" );
    }

  return status;

  }  /*  end of memwalk  */

#elif( _MSC_VER || __WATCOMC__ )

/***  MemWalk.c  *************************************************************/

/*  Check the heap using non-ANSI functions.  */

IX MemWalk( void )
  {
  IX status = 0;
  struct _heapinfo hp;   /* heap information */

  switch( _heapchk( ) )
    {
    case _HEAPOK:
      fprintf( _ulog, "The heap is O.K.\n" );
      break;
    case _HEAPEMPTY:
      fprintf( _ulog, "The heap is empty.\n" );
      break;
    case _HEAPBADBEGIN:
      fprintf( _ulog, "The heap is corrupted.\n" );
      status = 1;
      break;
    case _HEAPBADNODE:
      fprintf( _ulog, "The heap has a bad node.\n" );
      status = 1;
      break;
    }  /* end switch */

  fprintf( _ulog, "Heap: loc, size, used?\n" );
  hp._pentry = NULL;
  while( _heapwalk( &hp ) == _HEAPOK )
    fprintf( _ulog, "[%Fp] %5u %s\n", hp._pentry, hp._size,
             (hp._useflag == _USEDENTRY ? "used" : "free") );

  return status;

  }  /*  end of MemWalk  */

#else   /* ANSI - no action */

/***  MemWalk.c  *************************************************************/

IX MemWalk( void )
  {
  return 0;

  }  /*  end of MemWalk  */
#endif

#if( __TURBOC__ )
/***  MemRem.c  **************************************************************/

/*  Estimate unallocated memory -- Turbo C function call.
 *  Writes to string for subsequent processing by caller.
 *  CORELEFT reports the the amount of memory between the
 *  highest allocated block and the top of the heap.
 *  Freed lower blocks are not counted as available memory.  */

I1 *MemRem( I1 *string )
  {
  U4 bytes;
  bytes = coreleft();
  StrCpyS( string, LINELEN, "unallocated memory:  ",
           IntStr( bytes ), " bytes\n", "" );

  return(string);

  }  /* end of MemRem */

#elif( __WATCOMC__ )

/***  MemRem.c  **************************************************************/

I1 *MemRem( I1 *string )
  {
  struct meminfo {
    U4 LargestBlockAvail;
    U4 MaxUnlockedPage;
    U4 LargestLockablePage;
    U4 LinAddrSpace;
    U4 NumFreePagesAvail;
    U4 NumPhysicalPagesFree;
    U4 TotalPhysicalPages;
    U4 FreeLinAddrSpace;
    U4 SizeOfPageFile;
    U4 Reserved[3];
    } MemInfo;

# define DPMI_INT 0x31

  union REGS regs;
  struct SREGS sregs;

  regs.x.eax = 0x00000500;
  memset( &sregs, 0, sizeof(sregs) );
  sregs.es = FP_SEG( &MemInfo );
  regs.x.edi = FP_OFF( &MemInfo );

  int386x( DPMI_INT, &regs, &regs, &sregs );
  sprintf( string, "Largest unallocated block:  %ld bytes\nUnallocated stack:  %ld bytes\n",
    MemInfo.LargestBlockAvail, stackavail() );

  return(string);

  }  /* end of MemRem */

#else   /* ANSI - no action */

/***  MemRem.c  **************************************************************/

I1 *MemRem( I1 *string )
  {
  string[0] = '\0';

  return(string);

  }  /* end of MemRem */
#endif

#if( __TURBOC__ || __WATCOMC__ )
/***  NullPointerTest.c  *****************************************************/

/*  Test for a value being assigned to location NULL.
 *  Return 1 if value stored at NULL has changed.
 *  ( That condition will give "Null pounter assignment"
 *  at the end of a run. )
 *  Can use NullPointerTest to find the source of this error:
 *  Call NullPointerTest at start of program to set OV;
 *  Add NullPointerTest before and after possible sources of error;
 *  Use the return flag to print pointer addresses.
 *  Heap and stack checks added Apr 95.
 */

IX NullPointerTest( I1 *file, IX line )
/* file;  file name: __FILE__;
 * line;  line number: __LINE__;
 */
  {
  static U4 *_nullptr=NULL;  /* pointer to NULL */
  static U4 ov=1234567L;     /* original value at NULL */
  extern IX errno;  /* Turbo C global */
  extern I1 *sys_errlist[];  /* Turbo C global */
  IX nperror=0;

  if( errno )
    {
    fprintf( _ulog, "Error: %d - %s: file %s, line %d\n",
       errno, sys_errlist[errno], file, line );
    errno = 0;
    }

  if( ov==1234567L )    /* initialization */
    {
    fprintf( _ulog, "Initial value stored at NULL: [%lX]\n", *_nullptr );
    ov = *_nullptr;
    }

  if( *_nullptr != ov )  /* test */
    {
    error( 2, file, line, "Value stored to NULL", "" );
    ov = *_nullptr;     /* reset for next error */
    nperror = 1;
    }

  fflush( _ulog );

  return nperror;

  }  /* end NullPointerTest */

#else   /* ANSI - no action */

/***  NullPointerTest.c  **************************************************************/

IX NullPointerTest( I1 *file, IX line )
  {
  return 0;

  }  /* end NullPointerTest */
#endif

