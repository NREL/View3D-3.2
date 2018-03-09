/*subfile:  view3d.c  ********************************************************/

#include <stdio.h>
#include <stdarg.h> /* variable argument list macro definitions */
#include <stdlib.h> /* prototype: exit */
#include <string.h> /* prototype: memcpy */
#include <math.h>   /* prototypes: fabs, sqrt */
#include <float.h>  /* define: FLT_EPSILON */
#include "types.h"
#include "view3d.h"
#include "prtyp.h"
void ViewMethod( SRFDATNM *srfN, SRFDATNM *srfM, R4 distNM, VFCTRL *vfCtrl );
void InitViewMethod( VFCTRL *vfCtrl );

extern IX _list;    /* output control, higher value = more output */
extern FILE *_ulog; /* log file */
extern I1 _string[]; /* buffer for a character string */
extern I1 *methods[]; /* method abbreviations */

IX _row=0;  /* row number; save for errorf() */
IX _col=0;  /* column number; " */
R4 _sli4;   /* use SLI if rcRatio > 4 and relSep > _sli4 */
R4 _sai4;   /* use SAI if rcRatio > 4 and relSep > _sai4 */
R4 _sai10;  /* use SAI if rcRatio > 10 and relSep > _sai10 */
R4 _dai1;   /* use DAI if relSep > _dai1 */
R4 _sli1;   /* use SLI if relSep > _sli1 */

/***  View3D.c  **************************************************************/

/*  Driver function to compute view factors for 3-dimensional surfaces.  */

/*  The AF array is stored in triangular form:
 *  +--------+--------+--------+--------+----
 *  | [1][1] |   -    |   -    |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [2][1] | [2][2] |   -    |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [3][1] | [3][2] | [3][3] |   -    |  - 
 *  +--------+--------+--------+--------+----
 *  | [4][1] | [4][2] | [4][3] | [4][4] |  - 
 *  +--------+--------+--------+--------+----
 *  |  ...   |  ...   |  ...   |  ...   | ...
 */

void View3D( SRFDAT3D *srf, const IX *base, IX *possibleObstr,
             R8 **AF, VFCTRL *vfCtrl )
/* srf    - surface / vertex data for all surfaces
 * base   - base surface numbers
 * possibleObstr - list of possible view obstructing surfaces
 * AF     - array of Area * F values
 * vfCtrl - control values consolitated into structure
 */
  {
  IX n;  /* row */
  IX m;  /* column */
  IX n1=1, nn;     /* first and last rows */
  IX m1=1, mm;     /* first and last columns */
  IX *maskSrf;     /* list of mask and null surfaces */
  IX *possibleObstrN;  /* list of possible obstructions rel. to N */
  IX *probableObstr;   /* list of probable obstructions */
  IX nPossN;       /* number of possible obstructions rel. to N */
  IX nProb;        /* number of probable obstructions */
  IX mayView;      /* true if surfaces may view each other */
  SRFDATNM srfN,   /* row N surface */
           srfM,   /* column M surface */
          *srf1,   /* view from srf1 to srf2 -- */
          *srf2;   /*   one is srfN, the other is srfM. */
  VECTOR3D vNM;    /* vector between centroids of srfN and srfM */
  R4 distNM;       /* distance between centroids of srfN and srfM */
  R4 minArea;      /* area of smaller surface */
  UX nAF0=0,       /* number of AF which must equal 0 */
     nAFnO=0,      /* number of AF without obstructing surfaces */
     nAFwO=0,      /* number of AF with obstructing surfaces */
     nObstr=0;     /* total number of obstructions considered */
  UX **bins;       /* for statistical summary */
  R4 nAFtot=1;     /* total number of view factors to compute */
  IX maxSrfT=4;    /* max number of participating (transformed) surfaces */

#if( DEBUG > 0 && _MSC_VER == 0 )
  fprintf( _ulog, "At start of View3D - %s", MemRem( _string ) );
#endif

  nn = vfCtrl->nRadSrf;
  if( nn>1 )
    nAFtot = (R4)((nn-1)*nn);
  if( vfCtrl->row > 0 )
    {
    n1 = nn = vfCtrl->row;   /* can process a single row of view factors, */
    if( vfCtrl->col > 0 )
      m1 = vfCtrl->col;      /* or a single view factor, */
    }

  ViewsInit( 4, 1 );  /* initialize Gaussian integration coefficients */ 
  InitViewMethod( vfCtrl );

  possibleObstrN = Alc_V( 1, vfCtrl->nAllSrf, sizeof(IX), "possibleObstrN" );
  probableObstr = Alc_V( 1, vfCtrl->nAllSrf, sizeof(IX), "probableObstr" );

  vfCtrl->srfOT = Alc_V( 0, maxSrfT, sizeof(SRFDAT3X), "srfOT" );
  bins = Alc_MC( 0, 4, 1, 5, sizeof(UX), "bins" );
  vfCtrl->failConverge = 0;
  
  if( vfCtrl->nMaskSrf ) /* pre-process view masking surfaces */
    {
    maskSrf = Alc_V( 1, vfCtrl->nMaskSrf, sizeof(IX), "mask" );
    for( m=1,n=vfCtrl->nRadSrf; n; n-- )   /* set mask list */
      if( srf[n].type == MASK || srf[n].type == NULS )
        maskSrf[m++] = n;
    DumpOS( "Mask and Null surfaces:", vfCtrl->nMaskSrf, maskSrf );

    for( n=n1; n<=nn; n++ )
      {
      if( vfCtrl->col )
        mm = m1 + 1;
      else
        mm = n;
      for( m=m1; m<mm; m++ )   /* set all AF involving mask/null */
        if( srf[n].type == MASK || srf[n].type == NULS )
          if( base[n] == m )
            AF[n][m] = srf[n].area;
          else
            AF[n][m] = 0.0;
        else if( srf[m].type == MASK || srf[m].type == NULS )
          AF[n][m] = 0.0;
        else
          AF[n][m] = -1.0;     /* set AF flag values */
      }
    }

#if( DEBUG > 0 && _MSC_VER == 0 )
  fprintf( _ulog, "After View3D allocations - %s", MemRem( _string ) );
  MemWalk();
#endif

  for( n=n1; n<=nn; n++ )  /* process AF values for row N */
    {
    _row = n;
    if( nn == vfCtrl->nRadSrf )  /* progress display */
      {
      R4 pctDone = 100 * (R4)((n-1)*n) / nAFtot;
      fprintf( stderr, "\rSurface: %d; ~ %.1f %% complete", n, pctDone );
      }
    AF[n][n] = 0.0;
    nPossN = vfCtrl->nPossObstr;  /* remove obstructions behind N */
    memcpy( possibleObstrN+1, possibleObstr+1, nPossN*sizeof(IX) );
    nPossN = OrientationTestN( srf, n, vfCtrl, possibleObstrN, nPossN );
    if( vfCtrl->col )  /* set column limits */
      mm = m1 + 1;
    else if( vfCtrl->row > 0 )
      mm = vfCtrl->nRadSrf + 1;
    else
      mm = n;

    for( m=m1; m<mm; m++ )   /* compute view factor: row N, columns M */
      {
      if( vfCtrl->nMaskSrf && AF[n][m] >= 0.0 ) continue;
      _col = m;
      if( _list>2 )
        fprintf( _ulog, "*ROW %d, COL %d\n", _row, _col );

      mayView = SelfObstructionTest3D( srf+n, srf+m, &srfM );
      if( mayView )
        mayView = SelfObstructionTest3D( srf+m, srf+n, &srfN );
      if( mayView )
        {
        if( srfN.area * srfM.area == 0.0 )  /* must clip one or both surfces */
          {
          if( srfN.area + srfM.area == 0.0 )
            {
            IntersectionTest( &srfN, &srfM );  /* check invalid geometry */
            SelfObstructionClip( &srfN );
            SelfObstructionClip( &srfM );
            }
          else if( srfN.area == 0.0 )
            SelfObstructionClip( &srfN );
          else if( srfM.area == 0.0 )
            SelfObstructionClip( &srfM );
          }
        if( vfCtrl->col && _list>3 )
          {
          IX j;
          fprintf( _ulog, "Surface centroid, radius, area:\n" );
          fprintf( _ulog, "    ctd:  x       y       z     radius    area\n" );
          fprintf( _ulog, "N:%4d %7.4f %7.4f %7.4f %7.4f %10.3e\n", srfN.nr,
                   srfN.ctd.x, srfN.ctd.y, srfN.ctd.z, srfN.rc, srfN.area );
          fprintf( _ulog, "M:%4d %7.4f %7.4f %7.4f %7.4f %10.3e\n", srfM.nr,
                   srfM.ctd.x, srfM.ctd.y, srfM.ctd.z, srfM.rc, srfM.area );
          fprintf( _ulog, "    v: x       y       z\n" );
          for( j=0; j<srfN.nv; j++ )
            fprintf( _ulog, "N%d: %7.4f %7.4f %7.4f\n",
                     j, srfN.v[j].x, srfN.v[j].y, srfN.v[j].z );
          for( j=0; j<srfM.nv; j++ )
            fprintf( _ulog, "M%d: %7.4f %7.4f %7.4f\n",
                     j, srfM.v[j].x, srfM.v[j].y, srfM.v[j].z );
          fflush( _ulog );
          }
        VECTOR( (&srfN.ctd), (&srfM.ctd), (&vNM) );
        distNM = VLEN( (&vNM) );
        if( distNM < 1.0e-5 * (srfN.rc + srfM.rc) )
          errorf( 3, __FILE__, __LINE__, "Surfaces have same centroids", "" );

        nProb = nPossN;
        memcpy( probableObstr+1, possibleObstrN+1, nProb*sizeof(IX) );
        if( nProb )
          nProb = ConeRadiusTest( srf, &srfN, &srfM,
            vfCtrl, probableObstr, nProb, distNM );

        if( nProb )
          nProb = BoxTest( srf, &srfN, &srfM, vfCtrl, probableObstr, nProb );

        if( nProb )   /* test/set obstruction orientations */
          nProb = OrientationTest( srf, &srfN, &srfM,
            vfCtrl, probableObstr, nProb );

        if( vfCtrl->nMaskSrf ) /* add masking surfaces */
          nProb = AddMaskSrf( srf, &srfN, &srfM, maskSrf, base,
            vfCtrl, probableObstr, nProb );
        vfCtrl->nProbObstr = nProb;

        if( vfCtrl->nProbObstr )    /*** obstructed view factors ***/
          {
          SRFDAT3X subs[5];    /* subsurfaces of surface 1  */
          IX j, nSubSrf;       /* count / number of subsurfaces */
          R8 calcAF = 0.0;
                               /* set direction of projection */
          if( ProjectionDirection( srf, &srfN, &srfM,
              probableObstr, vfCtrl ) > 0 )
            { srf1 = &srfN; srf2 = &srfM; }
          else
            { srf1 = &srfM; srf2 = &srfN; }
          if( vfCtrl->nProbObstr && _list>2 )
            {
            if( vfCtrl->col && _list>3 )
              fprintf( _ulog, " Project rays from srf %d to srf %d\n",
                srf1->nr, srf2->nr );
            DumpOS( " Final LOS:", vfCtrl->nProbObstr, probableObstr );
            }
          if( vfCtrl->nProbObstr > maxSrfT ) /* expand srfOT array */
            {
            Fre_V( vfCtrl->srfOT, 0, maxSrfT, sizeof(SRFDAT3X), "srfOT" );
            maxSrfT = vfCtrl->nProbObstr + 4;
            vfCtrl->srfOT = Alc_V( 0, maxSrfT, sizeof(SRFDAT3X), "srfOT" );
            }
          CoordTrans3D( srf, srf1, srf2, probableObstr, vfCtrl );

          nSubSrf = Subsurface( &vfCtrl->srf1T, subs );
          for( vfCtrl->failRecursion=j=0; j<nSubSrf; j++ )
            {
            minArea = MIN( subs[j].area, vfCtrl->srf2T.area );
            vfCtrl->epsAF = minArea * vfCtrl->epsAdap;
            if( subs[j].nv == 3 )
              calcAF += ViewTP( subs[j].v, subs[j].area, 0, vfCtrl );
            else 
              calcAF += ViewRP( subs[j].v, subs[j].area, 0, vfCtrl );
            }
          AF[n][m] = calcAF * srf2->rc * srf2->rc;   /* area scaling factor */
          if( vfCtrl->failRecursion )
            {
            fprintf( _ulog, " row %d, col %d,  recursion did not converge, AF %g\n",
              _row, _col, AF[n][m] );
            vfCtrl->failConverge = 1;
            }
          nObstr += vfCtrl->nProbObstr;
          nAFwO += 1;
          vfCtrl->method = 5;
          }

        else                      /*** unobstructed view factors ***/
          {
          SRFDATNM *srf1;  /* pointer to surface 1 (smaller surface) */
          SRFDATNM *srf2;  /* pointer to surface 2 */
          vfCtrl->method = 5;
          vfCtrl->failViewALI = 0;
          if( srfN.rc >= srfM.rc )
            { srf1 = &srfM; srf2 = &srfN; }
          else
            { srf1 = &srfN; srf2 = &srfM; }
          ViewMethod( &srfN, &srfM, distNM, vfCtrl );
          minArea = MIN( srfN.area, srfM.area );
          vfCtrl->epsAF = minArea * vfCtrl->epsAdap;
          AF[n][m] = ViewUnobstructed( vfCtrl, _row, _col );
          if( vfCtrl->failViewALI )
            {
            fprintf( _ulog, " row %d, col %d,  line integral did not converge, AF %g\n",
              _row, _col, AF[n][m] );
            vfCtrl->failConverge = 1;
            }
          if( vfCtrl->method<5 ) // ???
            bins[vfCtrl->method][vfCtrl->nEdgeDiv] += 1;   /* count edge divisions */
          nAFnO += 1;
          }
        }
      else
        {                         /* view not possible */
        AF[n][m] = 0.0;
        nAF0 += 1;
        vfCtrl->method = 6;
        }

      if( srf[n].area > srf[m].area )  /* remove very small values */
        {
        if( AF[n][m] < 1.0e-8 * srf[n].area )
          AF[n][m] = 0.0;
        }
      else
        if( AF[n][m] < 1.0e-8 * srf[m].area )
          AF[n][m] = 0.0;

      if( _list>2 )
        {
        fprintf( _ulog, " AF(%d,%d): %.7e %.7e %.7e %s\n", _row, _col,
          AF[n][m], AF[n][m] / srf[n].area, AF[n][m] / srf[m].area,
          methods[vfCtrl->method] );
        fflush( _ulog );
        }

      }  /* end of element M of row N */

    }  /* end of row N */
  fputc( '\n', stderr );

  fprintf( _ulog, "\nSurface pairs where F(i,j) must be zero: %8u\n", nAF0 );
  fprintf( _ulog, "\nSurface pairs without obstructed views:  %8u\n", nAFnO );
  bins[4][5] = bins[0][5] + bins[1][5] + bins[2][5] + bins[3][5];
  fprintf( _ulog, "   nd %7s %7s %7s %7s %7s\n",
    methods[0], methods[1], methods[2], methods[3], methods[4] );
  fprintf( _ulog, "    2 %7u %7u %7u %7u %7u direct\n",
     bins[0][2], bins[1][2], bins[2][2], bins[3][2], bins[4][2] );
  fprintf( _ulog, "    3 %7u %7u %7u %7u\n",
     bins[0][3], bins[1][3], bins[2][3], bins[3][3] );
  fprintf( _ulog, "    4 %7u %7u %7u %7u\n",
     bins[0][4], bins[1][4], bins[2][4], bins[3][4] );
  fprintf( _ulog, "  fix %7u %7u %7u %7u %7u fixes\n",
     bins[0][5], bins[1][5], bins[2][5], bins[3][5], bins[4][5] );
  ViewsInit( 4, 0 );
  fprintf( _ulog, "Adaptive line integral evaluations used: %8lu\n",
    vfCtrl->usedV1LIadapt );
  fprintf( _ulog, "\nSurface pairs with obstructed views:   %10u\n", nAFwO );
  if( nAFwO>0 )
    {
    fprintf( _ulog, "Average number of obstructions per pair:   %6.2f\n",
      (R8)nObstr / (R8)nAFwO );
    fprintf( _ulog, "Adaptive viewpoint evaluations used:   %10u\n",
      vfCtrl->usedVObs );
    fprintf( _ulog, "Adaptive viewpoint evaluations lost:   %10u\n",
      vfCtrl->wastedVObs );
    fprintf( _ulog, "Non-zero viewpoint evaluations:        %10u\n",
      vfCtrl->totVpt );
/***fprintf( _ulog, "Number of 1AI point-polygon evaluations: %8u\n",
      vfCtrl->totPoly );***/
    fprintf( _ulog, "Average number of polygons per viewpoint:  %6.2f\n\n",
      (R8)vfCtrl->totPoly / (R8)vfCtrl->totVpt );
    }

  if( vfCtrl->failConverge ) error( 1, __FILE__, __LINE__,
    "Some calculations did not converge, see VIEW3D.LOG", "" );

#if( DEBUG > 0 && _MSC_VER == 0 )
  MemWalk();
  fprintf( _ulog, "Minimum %s", MemRem( _string ) );
#endif
  if( vfCtrl->nMaskSrf ) /* pre-process view masking surfaces */
    Fre_V( maskSrf, 1, vfCtrl->nMaskSrf, sizeof(IX), "mask" );
  Fre_MC( bins, 0, 4, 1, 5, sizeof(IX), "bins" );
  Fre_V( vfCtrl->srfOT, 0, maxSrfT, sizeof(SRFDAT3X), "srft" );
  Fre_V( probableObstr, 1, vfCtrl->nAllSrf, sizeof(IX), "probableObstr" );
  Fre_V( possibleObstrN, 1, vfCtrl->nAllSrf, sizeof(IX), "possibleObstrN" );
  Fre_V( possibleObstr, 1, vfCtrl->nAllSrf, sizeof(IX), "possibleObstr" );
#if( DEBUG > 0 && _MSC_VER == 0 )
  fprintf( _ulog, "At end of View3D - %s", MemRem( _string ) );
#endif

  }  /* end of View3D */

/***  ProjectionDirection.c  *************************************************/

/*  Set direction of projection of obstruction shadows.
 *  Direction will be from surface 1 to surface 2.  
 *  Want surface 2 large and distance from surface 2
 *  to obstructions to be small. 
 *  Return direction indicator: 1 = N to M, -1 = M to N.
 */

IX ProjectionDirection( SRFDAT3D *srf, SRFDATNM *srfN, SRFDATNM *srfM, 
  IX *probableObstr, VFCTRL *vfCtrl )
/* srf  - data for all surfaces.
 * srfN - data for surface N.
 * srfM - data for surface M.
 * probableObstr  - list of probable obstructing surfaces (input and revised for output).
 * vfCtrl - computation controls.
 */
  {
  IX j, k;   /* surface number */
  VECTOR3D v;
  R4 sdtoN, sdtoM; /* minimum distances from obstruction to N and M */
  IX direction=0;  /* 1 = N is surface 1; -1 = M is surface 1 */

#if( DEBUG > 1 )
  fprintf( _ulog, "ProjectionDirection:\n");
#endif

#ifdef XXX
  if( fabs(srfN->rc - srfM->rc) > 0.002*(srfN->rc + srfM->rc) )
    if( srfN->rc <= srfM->rc )    old code
      direction = 1;
    else
      direction = -1;
               this is worse for OVERHANG.VS3
  if( srfM->rc > 10.0 * srfN->rc )
    direction = 1;
  if( srfN->rc > 10.0 * srfM->rc )
    direction = -1;
#endif
  if( direction == 0 )
    {
      /* determine distances from centroid of K to centroids of N & M */
    sdtoM = sdtoN = 1.0e9;
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      {
      R4 dist;
      k = probableObstr[j];
      if( srf[k].NrelS >= 0 )
        {
        VECTOR( (&srfN->ctd), (&srf[k].ctd), (&v) );
        dist = VDOT( (&v), (&v) );
        if( dist<sdtoN ) sdtoN = dist;
        }
      if( srf[k].MrelS >= 0 )
        {
        VECTOR( (&srfM->ctd), (&srf[k].ctd), (&v) );
        dist = VDOT( (&v), (&v) );
        if( dist<sdtoM ) sdtoM = dist;
        }
      }
    sdtoN = (R4)sqrt( sdtoN );
    sdtoM = (R4)sqrt( sdtoM );
#if( DEBUG > 1 )
    fprintf( _ulog, " min dist to srf %d: %e\n", srfN->nr, sdtoN );
    fprintf( _ulog, " min dist to srf %d: %e\n", srfM->nr, sdtoM );
#endif

      /* direction based on distances and areas of N & M */
    if( fabs(sdtoN - sdtoM) > 0.002*(sdtoN + sdtoM) )
      {
      if( sdtoN < sdtoM ) direction = -1;
      if( sdtoN > sdtoM ) direction = 1;
      }

#if( DEBUG > 1 )
    fprintf( _ulog, " sdtoN %e, sdtoM %e, dir %d\n",
      sdtoN, sdtoM, direction );
#endif
    }

  if( !direction )   /* direction based on number of obstructions */
    {
    IX nosN=0, nosM=0;   /* number of surfaces facing N or M */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      {
      k = probableObstr[j];
      if( srf[k].NrelS >= 0 ) nosN++;
      if( srf[k].MrelS >= 0 ) nosM++;
      }
    if( nosN > nosM )
      direction = +1;
    else
      direction = -1;
    }

#if( DEBUG > 1 )
  fprintf( _ulog, " rcN %e, rcM %e, dir %d, pdir %d\n",
    srfN->rc, srfM->rc, direction, vfCtrl->prjReverse );
#endif

  if( vfCtrl->prjReverse )       /* direction reversal parameter */
    direction *= -1;

  k = 0;         /* eliminate probableObstr surfaces facing wrong direction */
  if( direction > 0 )  /* projections from N toward M */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      if( srf[probableObstr[j]].MrelS >= 0 )
        probableObstr[++k] = probableObstr[j];
  if( direction < 0 )  /* projections from M toward N */
    for( j=1; j<=vfCtrl->nProbObstr; j++ )
      if( srf[probableObstr[j]].NrelS >= 0 )
        probableObstr[++k] = probableObstr[j];
  vfCtrl->nProbObstr = k;

  return direction;

  }  /*  end of ProjectionDirection  */

/***  ViewMethod.c  **********************************************************/

/*  Determine method to compute unobstructed view factor.  */

void ViewMethod( SRFDATNM *srfN, SRFDATNM *srfM, R4 distNM, VFCTRL *vfCtrl )
  {
  SRFDATNM *srf1;  /* pointer to surface 1 (smaller surface) */
  SRFDATNM *srf2;  /* pointer to surface 2 */

  if( srfN->rc >= srfM->rc )
    { srf1 = srfM; srf2 = srfN; }
  else
    { srf1 = srfN; srf2 = srfM; }
  memcpy( &vfCtrl->srf1T, srf1, sizeof(SRFDAT3X) );
  memcpy( &vfCtrl->srf2T, srf2, sizeof(SRFDAT3X) );

  vfCtrl->rcRatio = srf2->rc / srf1->rc;
  vfCtrl->relSep = distNM / (srf1->rc + srf2->rc);

  vfCtrl->method = UNK;
  if( vfCtrl->rcRatio > 4.0f )
    {
    if( srf1->shape )
      {
      R4 relDot = VDOTW( (&srf1->ctd), (&srf2->dc) );
      if( vfCtrl->rcRatio > 10.0f &&
        (vfCtrl->relSep > _sai10 || relDot > 2.0*srf1->rc ) )
        vfCtrl->method = SAI;
      else if( vfCtrl->relSep > _sai4 || relDot > 2.0*srf1->rc )
        vfCtrl->method = SAI;
      }
    if( vfCtrl->method==UNK && vfCtrl->relSep > _sli4 )
      vfCtrl->method = SLI;
    }
  if( vfCtrl->method==UNK && vfCtrl->relSep > _dai1
      && srf1->shape && srf2->shape )
    vfCtrl->method = DAI;
  if( vfCtrl->method==UNK && vfCtrl->relSep > _sli1 )
    vfCtrl->method = SLI;
  if( vfCtrl->method==SLI && vfCtrl->epsAdap < 0.5e-6 )
    vfCtrl->method = ALI;
  if( vfCtrl->method==UNK )
    vfCtrl->method = ALI;

#if( DEBUG > 1 )
  fprintf( _ulog, "  rcRatio %.3f,  relSep %.2f, shapes %d %d, method %d\n",
    vfCtrl->rcRatio, vfCtrl->relSep, srf1->shape, srf2->shape, vfCtrl->method );
#endif

  }  /* end ViewMethod */

/***  InitViewMethod.c  ******************************************************/

/*  Initialize ViewMethod() coefficients.  */

void InitViewMethod( VFCTRL *vfCtrl )
  {
  if( vfCtrl->epsAdap < 0.99e-7 )
    {
    _sli4 = 0.7f;
    _sai4 = 1.5f;
    _sai10 = 1.8f;
    _dai1 = 3.0f;
    _sli1 = 3.0f;
    }
  else if( vfCtrl->epsAdap < 0.99e-6 )
    {
    _sli4 = 0.5f;
    _sai4 = 1.2f;
    _sai10 = 1.2f;
    _dai1 = 2.3f;
    _sli1 = 2.2f;
    }
  else if( vfCtrl->epsAdap < 0.99e-5 )
    {
    _sli4 = 0.45f;
    _sai4 = 1.1f;
    _sai10 = 1.0f;
    _dai1 = 1.7f;
    _sli1 = 1.5f;
    }
  else if( vfCtrl->epsAdap < 0.99e-4 )
    {
    _sli4 = 0.4f;
    _sai4 = 1.0f;
    _sai10 = 0.8f;
    _dai1 = 1.3f;
    _sli1 = 0.9f;
    }
  else
    {
    _sli4 = 0.3f;
    _sai4 = 0.9f;
    _sai10 = 0.6f;
    _dai1 = 1.0f;
    _sli1 = 0.6f;
    }
  
  }  /* end InitViewMethod */

/***  errorf.c  **************************************************************/

/*  error messages for view factor calculations  */

IX errorf( IX severity, I1 *file, IX line, ... )
/* severity;  values 0 - 3 defined below
 * file;      file name: __FILE__
 * line;      line number: __LINE__
 * ...;       string variables (up to 80 char total) */
  {
  va_list argp;     /* variable argument list */
  I1 start[]=" ";
  I1 *msg, *s;
  static I1 *head[4] = { "  *** note *** ",
                         "*** warning ***",
                         " *** error *** ",
                         " *** fatal *** " };
  IX n=1;

  if( severity >= 0 )
    {
    if( severity>3 ) severity = 3;
    StrCpyS( _string, LINELEN, head[severity], "      file/function: ",
      file, ",    line: ", IntStr( line ), "\n", "" );
    fputs( _string, stderr );
    if( _ulog != NULL && _ulog != stderr )
      {
      fputs( _string, _ulog );
      fflush( _ulog );
      }

    msg = start;   /* merge message strings */
    sprintf( _string, "row %d, col %d; ", _row, _col );
    s = _string;
    while( *s )
      s++;
    va_start( argp, line );
    while( *msg && n<80 )
      {
      while( *msg && n++<80 )
        *s++ = *msg++;
      msg = (I1 *)va_arg( argp, I1 * );
      }
    *s++ = '\n';
    *s = '\0';
    va_end( argp );

    fputs( _string, stderr );
    if( _ulog != NULL && _ulog != stderr )
      fputs( _string, _ulog );
    }

  if( severity>2 ) exit( 1 );

  return 0;

  }  /*  end of errorf  */


