/*subfile:  misc.c  **********************************************************/
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

#include <stdio.h>
#include <stdarg.h> /* variable argument list macro definitions */
#include <stdlib.h> /* prototypes: exit, strtod, strtol */
#include <time.h>   /* prototype: clock;  define: CLOCKS_PER_SEC */
#include <limits.h> /* define: SHRT_MAX, SHRT_MIN */
#include <float.h>  /* define: FLT_MAX */
#ifdef __TURBOC__ 
#include <dir.h>    /* prototypes: fnmerge, fnsplit */
#endif
#include "types.h" 
#include "view3d.h"
#include "prtyp.h"

extern FILE *_ulog; /* log file */
extern FILE *_unxt; /* input file */
extern IX _echo;    /* true = echo input file */
extern I1 _string[LINELEN];  /* buffer for a character string */
IX _emode = 1;

/*** #pragma optimize("", off) /*** bug in errorb() ***/

/***  CPUTime.c  *************************************************************/

/*  Determine elapsed time.  Call once to determine t1;
    call later to get elapsed time. */

R4 CPUTime( R4 t1 )
  {
  R4 t2;

  t2 = (R4)clock() / (R4)CLOCKS_PER_SEC;
  return (t2-t1);

  }  /* end CPUTime */

/***  pathmerge.c  ***********************************************************/

void pathmerge( I1 *fullpath, I1 *drv, I1 *path, I1 *file, I1 *ext )
  {
#if( _MSC_VER )
  _makepath( fullpath, drv, path, file, ext );
#elif( __WATCOMC__ )
  _makepath( fullpath, drv, path, file, ext );
#elif( __TURBOC__ )
  fnmerge( fullpath, drv, path, file, ext );
#else
  strcpy( fullpath, drv );
  strcat( fullpath, path );
  strcat( fullpath, file );
  strcat( fullpath, ext );
#endif
  }  /* end pathmerge */

/***  pathsplit.c  ***********************************************************/

void pathsplit( I1 *fullpath, I1 *drv, I1 *path, I1 *file, I1 *ext )
  {
#if( _MSC_VER )
  _splitpath( fullpath, drv, path, file, ext );
#elif( __WATCOMC__ )
  _splitpath( fullpath, drv, path, file, ext );
#elif( __TURBOC__ )
  fnsplit( fullpath, drv, path, file, ext );
#else
  I1 *c=file,
     *p=fullpath;

  while( *p!='.' )
    {
    *c = *p++;
    if( *c == '\\' )
      c = file;
    c++;
    }
  *c = '\0';
  c = ext;
  *c++ = '.';
  while( *p )
    *c++ = *p++;
  *c = '\0';
  drv[0] = '\0';
  path[0] = '\0';
#endif
  }  /* end pathsplit */

/***  finish.c  **************************************************************/

/*  VIEW3D termination.
 *  Called from error() with flag=2 for fatal error.  */

void finish( IX flag )
  {
  if( _ulog )
    fclose( _ulog );

  exit( flag );   /* exit value determines message in ContamW */

  }  /* end finish */

/***  errora.c  **************************************************************/

/*  Minimal error message - written to .LOG file.  */

void errora( const I1 *head, I1 *message, I1 *source )
  {
  if( _ulog )
    {
    fputs( head, _ulog );
    fputs( source, _ulog );
    fputs( message, _ulog );
    fflush( _ulog );
    }

  }  /*  end of errora  */

/***  errorb.c  **************************************************************/

/*  Standard error message for console version.  */

void errorb( const I1 *head, I1 *message, I1 *source )
  {
  fputs( head, stderr );
  fputs( message, stderr );
  errora( head, message, source );

  }  /*  end of errorb  */

/***  error.c  ***************************************************************/

/*  Print/dispaly error messages.  */

IX error( IX severity, I1 *file, IX line, ... )
/* severity;  values 0 - 3 defined below
 * file;      file name: __FILE__
 * line;      line number: __LINE__
 * ...;       string variables (up to 80 char total) */
  {
  I1 message[256];
  I1 source[64];
  va_list argp;        /* variable argument list */
  I1 start[]=" ";      /* leading blank in message */
  I1 *msg, *s;
  static IX count=0;   /* count of severe errors */
  static const I1 *head[4] = { "NOTE", "WARNING", "ERROR", "FATAL" };
  I1 name[16];
  IX n=1;

  if( severity >= 0 )
    {
    if( severity>3 ) severity = 3;
    if( severity==2 ) count += 1;
    msg = start;   /* merge message strings */
    s = message;
    va_start( argp, line );
    while( *msg && n<LINELEN )
      {
      while( *msg && n++ < LINELEN )
        *s++ = *msg++;
      msg = (I1 *)va_arg( argp, I1 * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    pathsplit( file, source, source, name, source );
    sprintf( source, "  (file %s, line %d)\n", name, line );

    if( _emode == 1 )
      errorb( head[severity], message, source );
    else
      errora( head[severity], message, source );
    if( severity>2 )
      exit( 2 );
    }
  else if( severity < -1 )   /* clear error count */
    count = 0;

  return count;

  }  /*  end error  */

/***  StrCpyS.c  *************************************************************/

/*  Copy characters from strings to S up to index mx.
 *  This can prevent copying past the end of S.
 *  Variable argument list terminated by a null string ("").  */

I1 *StrCpyS( I1 *s, const IX mx, ...  )
/* s;   destination string S
 * mx;  dimension of S ( char s[mx]; )
 * ...  strings to be appended to S  */
  {
  va_list argp;         /* variable argument list */
  I1 *start;
  I1 *str;
  IX n=1;

  start = s;
  va_start( argp, mx );
  str = (I1 *)va_arg( argp, I1 * );
  while( *str && n<mx )
    {
    while( *str && n++<mx )
      *s++ = *str++;
    str = (I1 *)va_arg( argp, I1 * );
    }
  *s = '\0';
  va_end( argp );

  return start;

  }  /*  end of StrCpyS  */

/***  StrEql.c  **************************************************************/

/*  Test for equality of two strings; return 1 if equal, 0 if not */

IX StrEql( I1 *s1, I1 *s2 )
  {
  IX i;

  for( i=0; s1[i]; i++ )
    if( s1[i] != s2[i] ) break;

  if( s1[i] == s2[i] )
    return 1;
  else
    return 0;

  }  /* end of StrEql */

/***  NxtOpen.c  *************************************************************/

void NxtOpen( I1 *file_name, I1 *file, IX line )
/* file;  file name: __FILE__
 * line;  line number: __LINE__
 */
  {
  if( _unxt ) error( 3, file, line, "_UNXT already open", "" );
  _unxt = fopen( file_name, "r" );  /* = NULL if no file */
  if( _unxt )
    NxtWord( "", -1, 0 );  /* initialize nxtwrd() */
  else
    error( 3, file, line, " Could not open file: ", file_name, "" );

  }  /* end NxtOpen */

/***  NxtClose.c  ************************************************************/

/*  Close _unxt.  */

void NxtClose( void )
  {
  if( fclose( _unxt ) )
    error( 3, __FILE__, __LINE__, "Problem while closing _UNXT", "" );
  _unxt = NULL;

  }  /* end NxtClose */

/***  NxtLine.c  *************************************************************/

/*  Get line of characters; used by NxtWord().  */

I1 *NxtLine( I1 *str, IX maxlen )
  {
  IX c=0;       /* character read from _unxt */
  IX i=0;       /* current position in str */

  while( c!='\n' )
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL;   // @@@
    if( c==EOF ) error( 3, __FILE__, __LINE__,
      "Attempt to read past end-of-file", "" );
    if( _echo ) putc( c, _ulog );
    str[i++] = c;
    if( i == maxlen )
      {
      str[i-1] = '\0';
      error( 3, __FILE__, __LINE__, "Buffer overflow: ", str, "" );
      }
    }
  str[i-1] = '\0';

  return str;

  }  /* end NxtLine */

/***  NxtWord.c  *************************************************************/

/*  Get the next word from file _unxt.  Fatal error at end-of-file.
 *  Assuming standard word separators (blank, comma, tab),
 *  comment identifiers (/ or ! to end-of-line, and 
 *  end of data (* or end-of-file).   */

I1 *NxtWord( I1 *str, IX flag, IX maxlen )
/* str;   string where word is stored; return pointer.
/* flag:  -1:  initialize function;
          0:  get next word from current position in _unxt;
          1:  get 1st word from next line of _unxt;
          2:  get remainder of current line from _unxt (\n --> \0);
          3:  get next line from _unxt (\n --> \0). */
  {
  static IX newl;      /* true if last call to NxtWord() ended at \n */
         IX c=0;       /* character read from _unxt */
         IX i=0;       /* current position in str */
         IX done=0;

  if( flag==-1 )     /* initialization */
    { newl = 1; return NULL; }

  str[0] = '\0';
  if( flag && !newl )
    { NxtLine( str, maxlen ); newl = 1; }

  if( flag==2 ) return str;

  if( flag==3 )
    { NxtLine( str, maxlen ); newl = 1; return str; }

  while( !done )   /* search for start of next word */
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL; // @@@
    if( c==EOF ) error( 3, __FILE__, __LINE__,
      "Attempt to read past end-of-file", "" );
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case ' ':          /* skip word separators */
      case ',':
      case '\n':
      case '\r':
      case '\t':
      case '\0':
        break;
      case '/':          /* comment; read next line */
      case '!':
        NxtLine( str, maxlen );
        break;
      case '*':          /* end-of-file indicator */
        return NULL;
      default:           /* first character of word found */
        str[i++] = c;
        done = 1;
        break;
      }
    }  /* end start-of-word search */

  done = newl = 0;
  while( !done )   /* search for end of word */
    {
    c = getc( _unxt );
    if( c==EOF ) return NULL;  // @@@
    if( c==EOF ) error( 3, __FILE__, __LINE__,
      "Attempt to read past end-of-file", "" );
    if( _echo ) putc( c, _ulog );
    switch( c )
      {
      case '\n':
        newl = 1;
      case ' ':
      case ',':
      case '\t':
        str[i] = '\0';
        done = 1;
        break;
      case '\0':
        break;
      default:
        str[i++] = c;
        if( i == maxlen )
          {
          str[i-1] = '\0';
          error( 3, __FILE__, __LINE__, "Buffer overflow: ", str, "" );
          }
        break;
      }
    }  /* end end-of-word search */

  return str;

  }  /* end NxtWord */

/***  FltStr.c  **************************************************************/

/*  Convert a float number to a string of characters;
 *  n significant digits; uses ANSI sprintf().
 *  Static string required to retain results in calling function.  */

I1 *FltStr( R8 f, IX n )
  {
  static I1 string[32];  /* string long enough for any practical value */
  I1 format[8];

  sprintf( format, "%%%dg", n );
  sprintf( string, format, f );

  return ( string );

  }  /* end of FltStr */

/***  IntStr.c  **************************************************************/

/*  Convert an integer to a string of characters.
 *  Can handle short or long integers by conversion to long.
 *  Static variables required to retain results in calling function.
 *  NMAX allows up to 2 calls to instr() in one statement.  */

I1 *IntStr( I4 i )
  {
#define NMAX 4
  static I1 string[NMAX][12];  /* string long enough for longest integer */
  static index=0;

  if( ++index == NMAX )
    index = 0;

  sprintf( string[index], "%ld", i );

  return string[index];
#undef NMAX

  }  /* end of IntStr */

/***  IntCon.c  **************************************************************/

/*  Convert a string of characters to a short integer
 *  with a flag set to 0 to indicate successful conversion.
 *  No imbedded blanks or stray characters permitted.
 *  Blanks before the number okay; \0 terminated string.
 *  The integer is assumed to be 2 bytes long:
 *  integers are in the range -32767 to +32767 (0x7FFF).
 *  Used in place of ATOI because of error processing.
 */

IX IntCon( I1 *s, I2 *i )
  {
  I1 *endptr;
  I4 longval;

  longval = strtol( s, &endptr, 10 );

  if( *endptr ) goto badval;
  if( endptr == s ) goto badval;
  if( longval > SHRT_MAX ) goto badval;
  if( longval < SHRT_MIN ) goto badval;

  *i = (I2)longval;
  return 0;

badval:
  *i = 0;
  return 1;
  
  }  /* end of IntCon */

/***  ReadI2.c  **************************************************************/

I2 ReadI2( IX flag )
  {
  I2 value;

  NxtWord( _string, flag, sizeof(_string) );
  if( IntCon( _string, &value ) )
    error( 2, __FILE__, __LINE__, "Bad integer: ", _string, "" );

  return value;

  }  /* end of ReadI2 */

/***  FltCon.c  **************************************************************/

/*  Function to convert a string of characters to a float value
 *  with a flag set to 0 to indicate successful conversion.
 *  No imbedded blanks or stray characters permitted.
 *  Blanks before or after the number okay.
 *  Floats are in the range -3.4e38 to +3.4e38.
 */

IX FltCon( I1 *s, R4 *f )
  {
  I1 *endptr;
  R8 value;    /* compute result in high presicion, then round */

  value = strtod( s, &endptr );

  if( *endptr ) goto badval;
  if( endptr == s ) goto badval;
  if( value > FLT_MAX ) goto badval;
  if( value < -FLT_MAX ) goto badval;

  *f = (R4)value;
  return 0;

badval:
  *f = 0.0;
  return 1;
  
  }  /* end of FltCon */

/***  ReadR4.c  **************************************************************/

R4 ReadR4( IX flag )
  {
  R4 value;

  NxtWord( _string, flag, sizeof(_string) );
  if( FltCon( _string, &value ) )
    error( 2, __FILE__, __LINE__, "Bad float value: ", _string, "" );

  return value;

  }  /* end ReadR4 */

