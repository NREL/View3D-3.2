/*subfile:  v3main.c  ********************************************************/
/*                                                                           */
/*  View3D, Copyright (c) 2018 Alliance for Sustainable Energy, LLC          */
/*  All rights reserved.                                                     */
/*                                                                           */
/*  Redistribution and use in source and binary forms, with or without       */
/*  modification, are permitted provided that the following conditions are   */
/*  met:                                                                     */
/*                                                                           */
/*  1. Redistributions of source code must retain the above copyright        */
/*     notice, this list of conditions and the following disclaimer.         */
/*                                                                           */
/*  2. Redistributions in binary form must reproduce the above copyright     */
/*     notice, this list of conditions and the following disclaimer in the   */
/*     documentation and/or other materials provided with the distribution.  */
/*                                                                           */
/*  3. The name of the copyright holder(s), any contributors, the United     */
/*     States Government, the United States Department of Energy, or any of  */
/*     their employees may not be used to endorse or promote products        */
/*     derived from this software without specific prior written permission  */
/*     from the respective party.                                            */
/*                                                                           */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND ANY             */
/*  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   */
/*  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND        */
/*  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE   */
/*  COPYRIGHT HOLDER(S), ANY CONTRIBUTORS, THE UNITED STATES GOVERNMENT, OR  */
/*  THE UNITED STATES DEPARTMENT OF ENERGY, NOR ANY OF THEIR EMPLOYEES, BE   */
/*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR      */
/*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF     */
/*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR          */
/*  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,    */
/*  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR  */
/*  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   */
/*  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                               */
/*                                                                           */
/*  This file has been modified from the original public domain version.     */
/*                                                                           */
/*  Original NIST Disclaimer:                                                */
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

/*  Main program for batch processing of 3-D view factors.  */

#include <stdio.h>
#include <string.h> /* prototype: strcpy */
#include <stdlib.h> /* prototype: exit */
#include <math.h>   /* prototype: sqrt */
#include <time.h>   /* prototypes: time, localtime, asctime;
                       define: tm, time_t */
#include "types.h"
#include "view3d.h"
#include "prtyp.h"

FILE *_unxt; /* input file */
FILE *_ulog; /* log file */
IX _echo=0;  /* true = echo input file */
IX _list=0;  /* output control, higher value = more output:
                0 = summary;
                1 = list view factors;
                2 = echo input, note calculations;
                3 = note obstructions. */
I1 _string[LINELEN];  /* buffer for a character string */
I1 *methods[7]={"2AI","1AI","2LI","1LI","ALI","Adapt","Blocked"}; /* abbreviations */

void FindFile( I1 *msg, I1 *name, I1 *type );
void ReadVF( I1 *fileName, I1 *program, I1 *version,
             IX *format, IX *encl, IX *didemit, IX *nSrf,
             R4 *area, R4 *emit, R8 **AF, R4 **F, IX init, IX shape );
void SaveVF( I1 *fileName, I1 *program, I1 *version,
             IX format, IX encl, IX didemit, IX nSrf,
             R4 *area, R4 *emit, R8 **AF, R4 *vtmp );

IX main( IX argc, I1 **argv )
  {
  I1 program[]="View3D";   /* program name */
  I1 version[]="3.2";      /* program version */
  I1 inFile[_MAX_PATH]=""; /* input file name */
  I1 outFile[_MAX_PATH]="";/* output file name */
  /* I1 fileName[_MAX_PATH]; */  /* name of file */
  /* I1 vdrive[_MAX_DRIVE]; */   /* drive letter for program View3D.exe */
  /* I1 vdir[_MAX_DIR]; */       /* directory path for program View3D.exe */
  I1 title[LINELEN];  /* project title */
  I1 **name;       /* surface names [1:nSrf][0:NAMELEN] */
  I1 *types[]={"rsrf","subs","mask","nuls","obso"};
  VERTEX3D *xyz;   /* vector of vertces [1:nVrt] - for ease in 
                        converting V3MAIN to a subroutine */
  SRFDAT3D *srf;   /* vector of surface data structures [1:nSrf] */
  VFCTRL vfCtrl;   /* VF calculation control parameters - avoid globals */
  R8 **AF;         /* triangular array of area*view factor values [1:nSrf][] */
  R4 *area;        /* vector of surface areas [1:nSrf] */
  R4 *emit;        /* vector of surface emittances [1:nSrf] */
  IX *base;        /* vector of base surface numbers [1:nSrf] */
  IX *cmbn;        /* vector of combine surface numbers [1:nSrf] */
  R4 *vtmp;        /* temporary vector [1:nSrf] */
  IX *possibleObstr;  /* list of possible view obstructing surfaces */
  struct tm *curtime; /* time structure */
  time_t bintime;  /* seconds since 00:00:00 GMT, 1/1/70 */
  R4 time0, time1; /* elapsed time values */
  IX nSrf;         /* current number of surfaces */
  IX nSrf0;        /* initial number of surfaces */
  IX encl;         /* 1 = surfaces form enclosure */
  IX n, flag;

#if( DEBUG > 0 && _MSC_VER == 0 )
  errno = 0;
#endif

  if( argc == 1 || argv[1][0] == '?' )
    {
    fputs("\n\
    VIEW3D - compute view factors for a 3D geometry.\n\n\
       VIEW3D  input_file  output_file\n\n\
    You may also enter the file names interactively.\n\n", stderr );
    if( argc > 1 )
      exit( 1 );
    }
                /* open log file */
/* #ifdef ANSI */
  _ulog = fopen( "VIEW3D.LOG", "w" );
  /*
#else
  pathsplit( argv[0], vdrive, vdir, _string, _string );
  pathmerge( fileName, vdrive, vdir, "VIEW3D", ".LOG" );
  _ulog = fopen( fileName, "w" );
#endif
  */
  if( !_ulog )
    error( 3, __FILE__, __LINE__, "Failed to open VIEW3D.LOG", "" );

#if( DEBUG > 0 )
# if( _MSC_VER == 0 )
  NullPointerTest( __FILE__, __LINE__ );
  fprintf( _ulog, "Initial %s\n", MemRem(_string) );
# endif
  MemWalk( );
  _echo = 1;
#endif

  fprintf( _ulog, "Program: %s %s\n", program, version );
  fprintf( _ulog, "Executing: %s\n", argv[0] );

  if( argc > 1 ) {
    if( strlen(argv[1]) >= _MAX_PATH ) {
      error(3, __FILE__, __LINE__, "Input file path is too long", "");
    }
    strncpy( inFile, argv[1], _MAX_PATH );
  }
  FindFile( "Enter name of V/S data file", inFile, "r" );
  fprintf( _ulog, "Data file:  %s\n", inFile );

  if( argc > 2 ) {
    if( strlen(argv[2]) >= _MAX_PATH ) {
      error(3, __FILE__, __LINE__, "Output file path is too long", "");
    }
    strncpy( outFile, argv[2], _MAX_PATH );
   }
  FindFile( "Enter name of VF output file", outFile, "w" );
  fprintf( _ulog, "Output file:  %s\n", outFile );

  time(&bintime);
  curtime = localtime(&bintime);
  fprintf( _ulog, "Time:  %s", asctime(curtime) );
  fputs("\n\
  View3D - calculation of view factors between simple polygons.\n\
    This software was developed at the National Institute of Standards\n\
    and Technology by employees of the Federal Government in the\n\
    course of their official duties. Pursuant to title 17 Section 105\n\
    of the United States Code this software is not subject to\n\
    copyright protection and is in the public domain. These programs\n\
    are experimental systems. NIST assumes no responsibility\n\
    whatsoever for their use by other parties, and makes no\n\
    guarantees, expressed or implied, about its quality, reliability,\n\
    or any other characteristic.  We would appreciate acknowledgment\n\
    if the software is used. This software can be redistributed and/or\n\
    modified freely provided that any derivative works bear some\n\
    notice that they are derived from it, and any modified versions\n\
    bear some notice that they have been modified.\n", stderr );

  time0 = CPUTime( 0.0 );  /* start-of-run time */

                 /* initialize control data */
  memset( &vfCtrl, 0, sizeof(VFCTRL) );
  // non-zero control values:
  vfCtrl.epsAdap = 1.0e-4f; // convergence for adaptive integration
  vfCtrl.maxRecursALI = 12; // maximum number of recursion levels
  vfCtrl.maxRecursion = 8;  // maximum number of recursion levels

                 /* read Vertex/Surface data file */
  NxtOpen( inFile, __FILE__, __LINE__ );
  CountVS3D( title, &vfCtrl );
  fprintf( _ulog, "\nTitle: %s\n", title );
  fprintf( _ulog, "Control values for 3-D view factor calculations:\n" );
  if( vfCtrl.enclosure )
    fprintf( _ulog, "  surfaces form enclosure.\n" );
  if( vfCtrl.emittances )
    fprintf( _ulog, "  will process emittances.\n" );
  fprintf( _ulog, "     adaptive convergence: %g", vfCtrl.epsAdap );
  if( vfCtrl.epsAdap != 1.e-4f )
    fprintf( _ulog, " *" );
  fprintf( _ulog, "\n  unobstructed recursions: %d", vfCtrl.maxRecursALI );
  if( vfCtrl.maxRecursALI != 12 )
    fprintf( _ulog, " *" );
  fprintf( _ulog, "\nmax obstructed recursions: %d", vfCtrl.maxRecursion );
  if( vfCtrl.maxRecursion != 8 )
    fprintf( _ulog, " *" );
  fprintf( _ulog, "\nmin obstructed recursions: %d", vfCtrl.minRecursion );
  if( vfCtrl.minRecursion )
    fprintf( _ulog, " *" );
  fprintf( _ulog, "\n              solving row:" );
  if( vfCtrl.row )
    fprintf( _ulog, " %d *", vfCtrl.row );
  else
    fprintf( _ulog, " all" );
  fprintf( _ulog, "\n           solving column:" );
  if( vfCtrl.col )
    fprintf( _ulog, " %d *", vfCtrl.col );
  else
    fprintf( _ulog, " all" );
  if( vfCtrl.prjReverse )
    fprintf( _ulog, "\n      reverse projections. **" );
  fprintf( _ulog, "\n output control parameter: %d\n", _list );

  fprintf( _ulog, "\n" );
  fprintf( _ulog, " total number of surfaces: %d \n", vfCtrl.nAllSrf );
  fprintf( _ulog, "   heat transfer surfaces: %d \n", vfCtrl.nRadSrf );

  nSrf = nSrf0 = vfCtrl.nRadSrf;
  encl = vfCtrl.enclosure;
  if( vfCtrl.format == 4 )
    vfCtrl.nVertices = 4 * vfCtrl.nAllSrf;
  name = Alc_MC( 1, nSrf0, 0, NAMELEN, sizeof(I1), "name" );
  area = Alc_V( 1, nSrf0, sizeof(R4), "area" );
  emit = Alc_V( 1, nSrf0, sizeof(R4), "emit" );
  vtmp = Alc_V( 1, nSrf0, sizeof(R4), "vtmp" );
  for( n=nSrf; n; n-- )
    vtmp[n] = 1.0;
  base = Alc_V( 1, nSrf0, sizeof(IX), "base" );
  cmbn = Alc_V( 1, nSrf0, sizeof(IX), "cmbn" );
  xyz = Alc_V( 1, vfCtrl.nVertices, sizeof(VERTEX3D), "xyz" );
  srf = Alc_V( 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), "srf" );

               /* read v/s data file */
  if( _list>2 )
    _echo = 1;
  if( vfCtrl.format == 4 )
    GetVS3Da( name, emit, base, cmbn, srf, xyz, &vfCtrl );
  else
    GetVS3D( name, emit, base, cmbn, srf, xyz, &vfCtrl );
  for( n=nSrf; n; n-- )
    area[n] = srf[n].area;
  NxtClose();

  if( encl )    /* determine volume of enclosure */
    {
    R8 volume=0.0;
    for( n=vfCtrl.nAllSrf; n; n-- )
      {
      if( srf[n].type == SUBS ) continue;
      volume += VolPrism( srf[n].v[0], srf[n].v[1], srf[n].v[2] );
      if( srf[n].nv == 4 )
        volume += VolPrism( srf[n].v[2], srf[n].v[3], srf[n].v[0] );
      }
    volume /= -6.0;        /* see VolPrism() */
    fprintf( _ulog, "      volume of enclosure: %.3f\n", volume );
    }

  if( _list>2 )
    {
    fprintf( _ulog, "Surfaces:\n" );
    fprintf( _ulog, "   #        name     area   emit  type bsn csn (dir cos) (centroid)\n" );
    for( n=1; n<=nSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e %5.3f %4s %3d %3d (%g %g %g %g) (%g %g %g)\n",
        n, name[n], area[n], emit[n], types[srf[n].type], base[n], cmbn[n],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );
    for( ; n<=vfCtrl.nAllSrf; n++ )
      fprintf( _ulog, "%4d %12s %9.2e       %4s         (%g %g %g %g) (%g %g %g)\n",
        n, " ", area[n], types[srf[n].type],
        srf[n].dc.x, srf[n].dc.y, srf[n].dc.z, srf[n].dc.w,
        srf[n].ctd.x, srf[n].ctd.y, srf[n].ctd.z );

    fprintf( _ulog, "Vertices:\n" );
    for( n=1; n<=vfCtrl.nAllSrf; n++ )
      {
      IX j;
      fprintf( _ulog, "%4d ", n );
      for( j=0; j<srf[n].nv; j++ )
        fprintf( _ulog, " (%g %g %g)",
          srf[n].v[j]->x, srf[n].v[j]->y, srf[n].v[j]->z );
      fprintf( _ulog, "\n" );
      }
    }

  if( vfCtrl.row )
    AF = Alc_MC( vfCtrl.row, vfCtrl.row, 1, nSrf0, sizeof(R8), "AF" );
  else
    {
    AF = Alc_MSR( 1, nSrf0, sizeof(R8), "AF" );
    fprintf( stderr, "\nComputing view factors for %d surfaces:\n\n",
      vfCtrl.nRadSrf );
    }
  time1 = CPUTime( 0.0 );  /* start-of-VF-calculation time */

  possibleObstr = Alc_V( 1, vfCtrl.nAllSrf, sizeof(IX), "possibleObstr" );
  vfCtrl.nPossObstr = SetPosObstr3D( vfCtrl.nAllSrf, srf, possibleObstr );
  sprintf( _string, "\n %.2f seconds to determine %d possible view obstructing surfaces:",
    CPUTime(time1), vfCtrl.nPossObstr );
  if( vfCtrl.nPossObstr > 0 )
    DumpOS( _string, vfCtrl.nPossObstr, possibleObstr );
  else
    fprintf( _ulog, "    possible obstructions: %3d \n", vfCtrl.nPossObstr );

  View3D( srf, base, possibleObstr, AF, &vfCtrl );

  fprintf( _ulog, "\n%7.2f seconds to compute view factors.\n", CPUTime(time1) );
  if( vfCtrl.row )
    {
    IX n=vfCtrl.row,
       m=vfCtrl.col;
    R8 ai=1.0/area[n];
    fprintf( _ulog, "\n" );
    if( m )
      fprintf( _ulog, "F[%d][%d] = %.8f\n\n", n, m, AF[n][m]*ai );
    else
      {
      R8 F, sum=0.0;
      for( m=1; m<=nSrf; m++ )
        {
        F = AF[n][m]*ai;
        sum += F;
        fprintf( _ulog, "F[%d][%d] = %.8f\n", n, m, F );
        }
      fprintf( _ulog, "    sum = %.8f\n\n", sum );
      }
    fflush( _ulog );
    exit( 0 );
    }
  Fre_V( xyz, 1, vfCtrl.nVertices, sizeof(VERTEX3D), "xyz" );
  FreePolygonMem();
  for( n=nSrf; n; n-- )  /* clear base pointers to OBSO & MASK srfs */
    {
    if( srf[base[n]].type == OBSO )  /* Base is used for several things. */
      base[n] = 0;                   /* It must be progressively cleared */
    if( srf[n].type == MASK )        /* as each use is completed. */
      base[n] = 0;
    }

  if( _list>1 )
    {
    IX *jtmp = Alc_V( 1, nSrf, sizeof(IX), "jtmp" );
    for( n=nSrf; n; n-- )
     if( srf[n].type == NULS )
       jtmp[n] = 0;
     else
       jtmp[n] = base[n];
    ReportAF( nSrf, encl, "Initial view factors:",
      name, area, vtmp, jtmp, AF, 0 );
    Fre_V( jtmp, 1, nSrf, sizeof(IX), "jtmp" );
    }

  fprintf( stderr, "\nAdjusting view factors\n" );
  time1 = CPUTime( 0.0 );

  for( flag=0,n=nSrf; n; n-- )
    if( srf[n].type==NULS ) flag = 1;
  if( flag )                         /* remove null surfaces */
    {
    nSrf = DelNull( nSrf, srf, base, cmbn, emit, area, name, AF );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after removing null surfaces:",
        name, area, vtmp, base, AF, 0 );
    }
  Fre_V( srf, 1, vfCtrl.nAllSrf, sizeof(SRFDAT3D), "srf" );

  for( flag=0,n=nSrf; n; n-- )
    if( base[n]>0 ) flag = 1;
  if( flag )                         /* separate subsurfaces */
    {
    Separate( nSrf, base, area, AF );
    for( n=nSrf; n; n-- )
      base[n] = 0;
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after separating included surfaces:",
        name, area, vtmp, base, AF, 0 );
    }

  for( flag=0,n=nSrf; n; n-- )
    if( cmbn[n]>0 ) flag = 1;
  if( flag )                         /* combine surfaces */
    {
    nSrf = Combine( nSrf, cmbn, area, name, AF );
    if( _list>1 )
      {
      fprintf(_ulog,"Surfaces:\n");
      fprintf(_ulog,"  n   base  cmbn   area\n");
      for( n=1; n<=nSrf; n++ )
        fprintf(_ulog,"%3d%5d%6d%12.4e\n", n, base[n], cmbn[n], area[n] );
      ReportAF( nSrf, encl, "View factors after combining surfaces:",
        name, area, vtmp, base, AF, 0 );
      }
    }

  if( encl || vfCtrl.emittances )    /* intermediate report */
    if( _list < 2 )
      ReportAF( nSrf, encl, title, name, area, vtmp, base, AF, 1 );

  if( encl )                         /* normalize view factors */
    {
    NormAF( nSrf, vtmp, area, AF, 1.0e-7f, 100 );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors after normalization:",
        name, area, vtmp, base, AF, 0 );
    }
  fprintf( _ulog, "%7.2f seconds to adjust view factors.\n", CPUTime(time1) );

  if( vfCtrl.emittances )
    {
    fprintf( stderr, "\nProcessing surface emissivites\n" );
    time1 = CPUTime( 0.0 );
    IntFac( nSrf, emit, area, AF );
    fprintf( _ulog, "%7.2f seconds to include emissivities.\n", CPUTime(time1) );
    if( _list>1 )
      ReportAF( nSrf, encl, "View factors including emissivities:",
        name, area, emit, base, AF, 0 );
    if( encl )
      NormAF( nSrf, emit, area, AF, 1.0e-7f, 30 );   /* fix rounding errors */
    }

  fprintf( _ulog, "\nFinal view factors:" );
  if( vfCtrl.emittances )
    ReportAF( nSrf, encl, title, name, area, emit, base, AF, 0 );
  else
    ReportAF( nSrf, encl, title, name, area, vtmp, base, AF, 0 );

  CPUTime(time1);
  SaveVF( outFile, program, version, vfCtrl.outFormat, vfCtrl.enclosure,
          vfCtrl.emittances, nSrf, area, emit, AF, vtmp );
  fprintf( _ulog, "%7.2f seconds to write view factors.\n", CPUTime(time1) );


  fprintf( _ulog, "\nFinal list of surfaces:\n" );
  fprintf( _ulog, "   #        name     area  emit\n" );
  for( n=1; n<=nSrf; n++ )
    fprintf( _ulog, "%4d %12s %8.3f %5.3f\n", n, name[n], area[n], emit[n] );

  Fre_V( cmbn, 1, nSrf0, sizeof(IX), "cmbn" );
  Fre_V( base, 1, nSrf0, sizeof(IX), "base" );
  Fre_V( vtmp, 1, nSrf0, sizeof(R4), "vtmp" );
  Fre_V( emit, 1, nSrf0, sizeof(R4), "emit" );
  Fre_V( area, 1, nSrf0, sizeof(R4), "area" );
  Fre_MSR( (void **)AF, 1, nSrf0, sizeof(R8), "AF" );
  Fre_MC( (void **)name, 1, nSrf0, 0, NAMELEN, sizeof(I1), "name" );

#if( DEBUG > 0 )
# if( _MSC_VER == 0 )
  fprintf( _ulog, "Final %s", MemRem(_string) );
  NullPointerTest( __FILE__, __LINE__ );
# endif
  MemWalk( );
#endif

  fprintf( _ulog, "\n%7.2f seconds for all calculations.\n", CPUTime(time0) );
  time(&bintime);
  curtime = localtime(&bintime);
  fprintf( _ulog, "Time:  %s", asctime(curtime) );
  fprintf( _ulog, "\n**********\n\n" );

  fclose( _ulog );

  fprintf( stderr, "\nDone!\n" );

  return 0;

  }  /* end of main */

/***  VolPrism.c  ************************************************************/

/*  Compute 6 * volume of a prism defined by vertices a, b, c, and (0,0,0).
 *  Ref: E Kreyszig, _Advanced Engineering Mathematics_, 3rd ed, Wiley, 1972,
 *  pp 214,5.  Volume = A dot (B cross C) / 6; A = vector from 0 to a, ...;
 *  Uses the fact that VECTOR3D A = VERTEX3D a, ...; Sign of result depends
 *  on sequence (clockwise or counter-clockwise) of vertices.   */

R8 VolPrism( VERTEX3D *a, VERTEX3D *b, VERTEX3D *c )
  {
  VECTOR3D bxc;

  VCROSS( b, c, (&bxc) );
  return VDOT( a, (&bxc) );

  }  /* end of VolPrism */

/***  ReportAF.c  ************************************************************/

void ReportAF( const IX nSrf, const IX encl, const I1 *title, const I1 **name,
  const R4 *area, const R4 *emit, const IX *base, const R8 **AF, IX flag )
  {
  IX n;    /* row */
  IX m;    /* column */
  R4 err;  /* error values assuming enclosure */
  R8 F, sumF;  /* view factor, sum of F for row */
  R8 eMax=0.0;     /* maximum row error, if enclosure */
  R8 eRMS=0.0;     /* RMS row error, if enclosure */
#define MAXEL 10
  struct
    {
    R4 err;   /* row sumF error */
    IX n;     /* row number */
    } elist[MAXEL+1];
  IX i;

  fprintf( _ulog, "\n%s\n", title );
  if( encl && _list>0 )
    fprintf( _ulog, "          #        name   SUMj Fij (encl err)\n" );
  memset( elist, 0, sizeof(elist) );

  for( n=1; n<=nSrf; n++ )      /* process AF values for row n */
    {
    for( sumF=0.0,m=1; m<=n; m++ )  /* compute sum of view factors */
      if( base[m] == 0 )
        sumF += AF[n][m];
    for( ; m<=nSrf; m++ )
      if( base[m] == 0 )
        sumF += AF[m][n];
    sumF /= area[n];
    if( _list>0 )
      {
      fprintf( _ulog, " Row:  %4d %12s %9.6f", n, name[n], sumF );
      if( encl )
        fprintf( _ulog, " (%.6f)", fabs( sumF - emit[n] ) );
      fputc( '\n', _ulog );
      }

    if( encl )                /* compute row sumF error value */
      {
      err = (R4)fabs( sumF - emit[n]);
      eRMS += err * err;
      for( i=MAXEL; i>0; i-- )
        {
        if( err<=elist[i-1].err ) break;
        elist[i].err = elist[i-1].err;
        elist[i].n = elist[i-1].n;
        }
      elist[i].err = err;
      elist[i].n = n;
      }

    if( _list>0 )   /* print row n values */
      {
      R8 invArea = 1.0 / area[n];
      for( m=1; m<=nSrf; m++ )
        {
        I1 *s = _string;
        if( m>=n )
          F = AF[m][n] * invArea;
        else
          F = AF[n][m] * invArea;
        sprintf( _string, "%8.6f ", F );
        if( _string[0] == '0' )
          {
          s += 1;
          if( m%10==0 ) _string[8] = '\n';
          }
        else
          {
          sprintf( _string, "%7.5f ", F );  /* handle F = 1.0 */
          if( m%10==0 ) _string[7] = '\n';
          }
        fprintf( _ulog, "%s", s );
        }
      if( m%10!=1 ) fputc( '\n', _ulog );
      }
    }  /* end of row n */

  if( encl )     /* print row sumF error summary */
    {
    fprintf( _ulog, "Summary:\n" );
    eMax = elist[0].err;
    fprintf( _ulog, "Max row sumF error:  %.2e\n", eMax );
    eRMS = sqrt( eRMS/nSrf );
    fprintf( _ulog, "RMS row sumF error:  %.2e\n", eRMS );
    if( flag )
      {
      fprintf( stderr, "\nMax row sumF error:  %.2e\n", eMax );
      fprintf( stderr, "RMS row sumF error:  %.2e\n", eRMS );
      }
    if( elist[0].err>0.5e-6 )
      {
      fprintf( _ulog, "Largest errors [row, error]:\n" );
      for( i=0; i<MAXEL; i++ )
        {
        if( elist[i].err<0.5e-6 ) break;
        fprintf( _ulog, "%8d%10.6f\n", elist[i].n, elist[i].err );
        }
      }
    fprintf( _ulog, "\n" );
    }

  }  /* end of ReportAF */

/***  FindFile.c  ************************************************************/

/*  Find user designated file.
 *  First character of type string must be 'r', 'w', or 'a'.  */

void FindFile( I1 *msg, I1 *fileName, I1 *type )
/*  msg;    message to user
 *  name;   file name (string _MAX_PATH chars long)
 *  type;   type of file, see fopen() */
  {
  FILE  *pfile=NULL;

  while( !pfile )
    {
    if( fileName[0] )   /* try to open file */
      {
      pfile = fopen( fileName, type );
      if( pfile == NULL )
        fprintf( stderr, "Error! Failed to open: %s\nTry again.\n", fileName );
      }
    if( !pfile )        /* ask for file name */
      {
      fileName[_MAX_PATH - 1] = '\0';
      fprintf( stderr, "%s: ", msg );
      scanf( "%260s", fileName ); // _MAX_PATH is 260
      if( fileName[_MAX_PATH - 1] != '\0' ) {
        /* file is too long */
        fprintf(stderr, "Error! File path is too long.\nTry again.\n");
        fileName[0] = '\0';
      }
      }
    }

  fclose( pfile );

  }  /*  end of FindFile  */

