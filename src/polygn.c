/*subfile:  polygn.c  ********************************************************/

/* The functions in this file maintain:
 *   a stack of free vertex/edge structures,
 *   a stack of free polygon structures, and
 *   one or more stacks of defined polygon structures.
 * Only one defined polygon stack may be created at a time.
 * However, multiple stacks may be saved by using external pointers. */

#include <stdio.h>
#include <string.h> /* prototype: memset, memcpy */
#include <math.h>   /* prototype: fabs */
#include "types.h" 
#include "view3d.h"
#include "prtyp.h"
typedef struct memblock   /* block of memory for small structure allocation */
  {
  UX blockSize;   /* number of bytes in block */
  UX dataOffset;  /* offset to free space */
  struct memblock *priorBlock;  /* pointer to previous block */
  struct memblock *nextBlock;   /* pointer to next block */
  }  MEMBLOCK;
IX TransferVrt( VERTEX2D *toVrt, const VERTEX2D *fromVrt, IX nFromVrt );

I1 *_memPoly=NULL; /* memory block for polygon descriptions; must start NULL */
HCVE *_nextFreeVE; /* pointer to next free vertex/edge */
POLY *_nextFreePD; /* pointer to next free polygon descripton */
POLY *_nextUsedPD; /* pointer to top-of-stack used polygon */
R4 _epsDist;   /* minimum distance between vertices */
R4 _epsArea;   /* minimum surface area */

extern FILE *_ulog; /* log file */

/*  Extensive use is made of 'homogeneous coordinates' (HC) which are not 
 *  familiar to most engineers.  The important properties of HC are 
 *  summarized below:
 *  A 2-D point (X,Y) is represented by a 3-element vector (w*X,w*Y,w), 
 *  where w may be any real number except 0.  A line is also represented 
 *  by a 3-element vector (a,b,c).  The directed line from point 
 *  (w*X1,w*Y1,w) to point (v*X2,v*Y2,v) is given by 
 *    (a,b,c) = (w*X1,w*Y1,w) cross (v*X2,v*Y2,v). 
 *  The sequence of the cross product is a convention to determine sign. 
 *  A point lies on a line if (a,b,c) dot (w*X,w*Y,w) = 0. 
 *  'Normalize' the representation of a point by setting w to 1.  Then 
 *  if (a,b,c) dot (X,Y,1) > 0.0, the point is to the left of the line. 
 *  If it is less than zero, the point is to the right of the line. 
 *  The point where two lines intersect is given by
 *    (w*X,w*Y,w) = (a1,b1,c1) cross (a2,b2,c2).
 *  Reference:  W.M. Newman and R.F. Sproull, "Principles of Interactive
 *  Computer Graphics", Appendix II, McGraw-Hill, 1973.  */

/***  PolygonOverlap.c  ******************************************************/

/*  Process two convex polygons.  Vertices are in clockwise sequence!!
 *
 *  There are three processing options -- savePD:
 *  (1)  Determine the portion of polygon P2 which is within polygon P1; 
 *       this may create one convex polygon.
 *  (3)  Determine the portion of polygon P2 which is outside polygon P1; 
 *       this may create one or more convex polygons.
 *  (2)  Do both (1) and (3).
 *
 *  If freeP2 is true, free P2 from its stack before exiting.
 *  Return 0 if P2 outside P1, 1 if P2 inside P1, 2 for partial overlap.
 */

IX PolygonOverlap( const POLY *p1, POLY *p2, const IX savePD, IX freeP2 )
  {
  POLY *pp;     /* pointer to polygon */
  POLY *initUsedPD;  /* initial top-of-stack pointer */
  HCVE *pv1;    /* pointer to edge of P1 */
  IX nLeftVrt;  /* number of vertices to left of edge */
  IX nRightVrt; /* number of vertices to right of edge */
  IX nTempVrt;  /* number of vertices of temporary polygon */
  VERTEX2D leftVrt[MAXNVT];  /* coordinates of vertices to left of edge */
  VERTEX2D rightVrt[MAXNVT]; /* coordinates of vertices to right of edge */
  VERTEX2D tempVrt[MAXNVT];  /* coordinates of temporary polygon */
  IX overlap=0; /* 0: P2 outside P1; 1: P2 inside P1; 2: part overlap */
  IX j, jm1;    /* vertex indices;  jm1 = j - 1 */

#if( DEBUG > 0 )
  NullPointerTest( __FILE__, __LINE__ );
#endif
#if( DEBUG > 1 )
  fprintf( _ulog, "PolygonOverlap:  P1 [%p]  P2 [%p]  flag %d\n",
    p1, p2, savePD );
#endif

  initUsedPD = _nextUsedPD;
  nTempVrt = GetPolygonVrt2D( p2, tempVrt );

#if( DEBUG > 1 )
  DumpP2D( "P2:", nTempVrt, tempVrt );
#endif

  pv1 = p1->firstVE;
  do{  /*  process tempVrt against each edge of P1 (long loop) */
       /*  transfer tempVrt into leftVrt and/or rightVrt  */
    R8 a1, b1, c1; /* HC for current edge of P1 */
    IX u[MAXNVT];  /* +1 = vertex left of edge; -1 = vertex right of edge */
    IX left=1;     /* true if all vertices left of edge */
    IX right=1;    /* true if all vertices right of edge */
#if( DEBUG > 1 )
    fprintf(_ulog, "Test against edge of P1\nU:" );
#endif
 
        /* compute and save u[j] - relations of vertices to edge */
    a1 = pv1->a; b1 = pv1->b; c1 = pv1->c;
    pv1 = pv1->next;
    for( j=0; j<nTempVrt; j++ )
      {
      R8 dot = tempVrt[j].x * a1 + tempVrt[j].y * b1 + c1;
      if( dot > _epsArea )
        { u[j] = 1; right = 0; }
      else if( dot < -_epsArea )
        { u[j] = -1; left = 0; }
      else
        u[j] = 0;
#if( DEBUG > 1 )
      fprintf( _ulog, " %d", u[j] );
#endif
      }
#if( DEBUG > 1 )
    fprintf( _ulog, "\nQuick test:  right %d; left %d;\n", right, left );
#endif
 
        /* use quick tests to skip unnecessary calculations */
    if( right ) continue;
    if( left ) goto p2_outside_p1;

        /* check each vertex of tempVrt against current edge of P1 */
    jm1 = nTempVrt - 1;
    for( nLeftVrt=nRightVrt=j=0; j<nTempVrt; jm1=j++ )    /* short loop */
      {
      if( u[jm1]*u[j] < 0 )  /* vertices j-1 & j on opposite sides of edge */
        {                             /* compute intercept of edges */
        R8 a, b, c, w; /* HC intersection components */
        a = tempVrt[jm1].y - tempVrt[j].y;
        b = tempVrt[j].x - tempVrt[jm1].x;
        c = tempVrt[j].y * tempVrt[jm1].x - tempVrt[jm1].y * tempVrt[j].x;
        w = b * a1 - a * b1;
#if( DEBUG > 1 )
        if( fabs(w) < _epsArea*(a+b+c) )
          {
          error( 1, __FILE__, __LINE__, "small W", "" );
          DumpHC( "P1:", p1, p1 );
          DumpHC( "P2:", p2, p2 );
          fprintf( _ulog, "a, b, c, w: %g %g %g %g\n", a, b, c, w );
          fflush( _ulog );
          fprintf( _ulog, "x, y: %g %g\n", (c*b1-b*c1)/w, (a*c1-c*a1)/w );
          }
#endif
#if( DEBUG > 0 )
        if( w == 0.0 ) error( 3, __FILE__, __LINE__,
          " Would divide by zero (w=0)", "" );
#endif
        rightVrt[nRightVrt].x = leftVrt[nLeftVrt].x = (R4)(( c*b1 - b*c1 ) / w);
        rightVrt[nRightVrt++].y = leftVrt[nLeftVrt++].y = (R4)(( a*c1 - c*a1 ) / w);
        }
      if( u[j] >= 0 )        /* vertex j is on or left of edge */
        {
        leftVrt[nLeftVrt].x = tempVrt[j].x;
        leftVrt[nLeftVrt++].y = tempVrt[j].y;
        }
      if( u[j] <= 0 )        /* vertex j is on or right of edge */
        {
        rightVrt[nRightVrt].x = tempVrt[j].x;
        rightVrt[nRightVrt++].y = tempVrt[j].y;
        }
      }  /* end of short loop */

#if( DEBUG > 1 )
    DumpP2D( "Left polygon:", nLeftVrt, leftVrt );
    DumpP2D( "Right polygon:", nRightVrt, rightVrt );
#endif
    if( nLeftVrt >= MAXNVT || nRightVrt >= MAXNVT )
      error( 3, __FILE__, __LINE__, "Parameter MAXNVT too small", "" );

    if( savePD > 1 )  /* transfer left vertices to outside polygon */
      {
      nTempVrt = TransferVrt( tempVrt, leftVrt, nLeftVrt );
#if( DEBUG > 1 )
      DumpP2D( "Outside polygon:", nTempVrt, tempVrt );
#endif
      if( nTempVrt > 2 )
        {
        SetPolygonHC( nTempVrt, tempVrt, p2->trns );
        overlap = 1;
        }
      }

                      /* transfer right side vertices to tempVrt */
    nTempVrt = TransferVrt( tempVrt, rightVrt, nRightVrt );
#if( DEBUG > 1 )
    DumpP2D( "Inside polygon:", nTempVrt, tempVrt );
#endif
    if( nTempVrt < 2 ) /* 2 instead of 3 allows degenerate P2; espArea = 0 */
      goto p2_outside_p1;

    }  while( pv1 != p1->firstVE );    /* end of P1 long loop */

  /* At this point tempVrt contains the overlap of P1 and P2. */

  if( savePD < 3 )    /* save the overlap polygon */
    {
#if( DEBUG > 1 )
    DumpP2D( "Overlap polygon:", nTempVrt, tempVrt );
#endif
    pp = SetPolygonHC( nTempVrt, tempVrt, p2->trns * p1->trns );
    if( pp==NULL && savePD==2 )   /* overlap area too small */
      goto p2_outside_p1;
    }
  overlap += 1;
  goto finish;

p2_outside_p1:     /* no overlap between P1 and P2 */
  overlap = 0;
#if( DEBUG > 1 )
  fprintf( _ulog, "P2 outside P1\n" );
#endif
  if( savePD > 1 )    /* save outside polygon - P2 */
    {
    if( initUsedPD != _nextUsedPD )  /* remove previous outside polygons */
      FreePolygons( _nextUsedPD, initUsedPD );

    if( freeP2 )         /* transfer P2 to new stack */
      {
      pp = p2;
      freeP2 = 0;               /* P2 already freed */
      }
    else                 /* copy P2 to new stack */
      {
      HCVE *pv, *pv2;
      pp = GetPolygonHC();      /* get cleared polygon data area */
      pp->area = p2->area;      /* copy P2 data */
      pp->trns = p2->trns;
      pv2 = p2->firstVE;
      do{
        if( pp->firstVE )
          pv = pv->next = GetVrtEdgeHC();
        else
          pv = pp->firstVE = GetVrtEdgeHC();
        memcpy( pv, pv2, sizeof(HCVE) );   /* copy vertex/edge data */
        pv2 = pv2->next;
        } while( pv2 != p2->firstVE );
      pv->next = pp->firstVE;   /* complete circular list */
#if( DEBUG > 1 )
    DumpHC( "COPIED SURFACE:", pp, pp );
#endif
      }
    pp->next = initUsedPD;   /* link PP to stack */
    _nextUsedPD = pp;
    }

finish:
#if( DEBUG > 0 )
  NullPointerTest( __FILE__, __LINE__ );
#endif
  if( freeP2 )   /* transfer P2 to free space */
    FreePolygons( p2, p2->next );

  return overlap;

  }  /* end of PolygonOverlap */

/***  TransferVrt.c  *********************************************************/

/*  Transfer vertices from polygon fromVrt to polygon toVrt eliminating nearly
 *  duplicate vertices.  Closeness of vertices determined by _epsDist.  
 *  Return number of vertices in polygon toVrt.  */

IX TransferVrt( VERTEX2D *toVrt, const VERTEX2D *fromVrt, IX nFromVrt )
  {
  IX j,  /* index to vertex in polygon fromVrt */
    jm1, /* = j - 1 */
     n;  /* index to vertex in polygon toVrt */

  jm1 = nFromVrt - 1;
  for( n=j=0; j<nFromVrt; jm1=j++ )
    if( fabs(fromVrt[j].x - fromVrt[jm1].x) > _epsDist ||
        fabs(fromVrt[j].y - fromVrt[jm1].y) > _epsDist )
      {               /* transfer to toVrt */
      toVrt[n].x = fromVrt[j].x;
      toVrt[n++].y = fromVrt[j].y;
      }
    else if( n>0 )    /* close: average with prior toVrt vertex */
      {
      toVrt[n-1].x = 0.5f * (toVrt[n-1].x + fromVrt[j].x);
      toVrt[n-1].y = 0.5f * (toVrt[n-1].y + fromVrt[j].y);
      }
    else              /* (n=0) average with prior fromVrt vertex */
      {
      toVrt[n].x = 0.5f * (fromVrt[jm1].x + fromVrt[j].x);
      toVrt[n++].y = 0.5f * (fromVrt[jm1].y + fromVrt[j].y);
      nFromVrt -= 1;  /* do not examine last vertex again */
      }

  return n;

  }  /* end TransferVrt */

/***  SetPolygonHC.c  ********************************************************/

/*  Set up polygon including homogeneous coordinates of edges.
 *  Return NULL if polygon area too small; otherwise return pointer to polygon.
 */

POLY *SetPolygonHC( const IX nVrt, const VERTEX2D *polyVrt, const R4 trns )
/* nVrt    - number of vertices (vertices in clockwise sequence);
 * polyVrt - X,Y coordinates of vertices (1st vertex not repeated at end),
             index from 0 to nVrt-1. */
  {
  POLY *pp;    /* pointer to polygon */
  HCVE *pv;    /* pointer to HC vertices/edges */
  R8 area=0.0; /* polygon area */
  IX j, jm1;   /* vertex indices;  jm1 = j - 1 */

  pp = GetPolygonHC();      /* get cleared polygon data area */
#if( DEBUG > 1 )
  fprintf( _ulog, " SetPolygonHC:  pp [%p]  nv %d\n", pp, nVrt );
#endif

  jm1 = nVrt - 1;
  for( j=0; j<nVrt; jm1=j++ )  /* loop through vertices */
    {
    if( pp->firstVE )
      pv = pv->next = GetVrtEdgeHC();
    else
      pv = pp->firstVE = GetVrtEdgeHC();
    pv->x = polyVrt[j].x;
    pv->y = polyVrt[j].y;
    pv->a = polyVrt[jm1].y - polyVrt[j].y; /* compute HC values */
    pv->b = polyVrt[j].x - polyVrt[jm1].x;
    pv->c = polyVrt[j].y * polyVrt[jm1].x - polyVrt[j].x * polyVrt[jm1].y;
    area -= pv->c;
    }
  pv->next = pp->firstVE;    /* complete circular list */

  pp->area = (R4)(0.5 * area);
  pp->trns = trns;
#if( DEBUG > 1 )
  fprintf( _ulog, "  areas:  %f  %f,  trns:  %f\n",
             pp->area, _epsArea, pp->trns );
  fflush( _ulog );
#endif

  if( pp->area < _epsArea )  /* polygon too small to save */
    {
    FreePolygons( pp, NULL );
    pp = NULL;
    }
  else
    {
    pp->next = _nextUsedPD;     /* link polygon to current list */
    _nextUsedPD = pp;           /* prepare for next linked polygon */
    }

  return pp;

  }  /*  end of SetPolygonHC  */

/***  GetPolygonHC.c  ********************************************************/

/*  Return pointer to a cleared polygon structure.  
 *  This is taken from the list of unused structures if possible.  
 *  Otherwise, a new structure will be allocated.  */

POLY *GetPolygonHC( void )
  {
  POLY *pp;  /* pointer to polygon structure */

  if( _nextFreePD )
    {
    pp = _nextFreePD;
    _nextFreePD = _nextFreePD->next;
    memset( pp, 0, sizeof(POLY) );  /* clear pointers */
    }
  else
    pp = Alc_EC( &_memPoly, sizeof(POLY), "nextPD" );

  return pp;

  }  /* end GetPolygonHC */

/***  GetVrtEdgeHC.c  ********************************************************/

/*  Return pointer to an uncleared vertex/edge structure.  
 *  This is taken from the list of unused structures if possible.  
 *  Otherwise, a new structure will be allocated.  */

HCVE *GetVrtEdgeHC( void )
  {
  HCVE *pv;  /* pointer to vertex/edge structure */

  if( _nextFreeVE )
    {
    pv = _nextFreeVE;
    _nextFreeVE = _nextFreeVE->next;
    }
  else
    pv = Alc_EC( &_memPoly, sizeof(HCVE), "nextVE" );

  return pv;

  }  /* end GetVrtEdgeHC */

/***  Alc_EC.c  **************************************************************/

/*  Allocate small structures within a larger allocated block.
 *  Can delete entire block but not individual structures.
 *  This saves quite a bit of memory allocation overhead.
 *  Also quite a bit faster than calling alloc() for each small structure.
 *  Based on idea and code by Steve Weller, "The C Users Journal",
 *  April 1990, pp 103 - 107.
 *  Must begin with Alc_ECI for initialization; free with Fre_EC.
 */

void *Alc_EC( I1 **block, UX size, I1 *name )
/*  block;  pointer to current memory block.
 *  size;   size (bytes) of structure being allocated.
 *  name;   name of structure being allocated.  */
  {
  I1 *p;  /* pointer to the structure */
  UX blockSize;
  MEMBLOCK *mb, /* memory block */
           *nb; /* new block */

#ifdef __TURBOC__
  size = (size+3) & 0xFFFC;   /* multiple of 4 */
#else
  size = (size+7) & 0xFFF8;   /* multiple of 8 */
#endif
  mb = (void *)*block;
  blockSize = mb->blockSize;
  if( size > blockSize - sizeof(MEMBLOCK) ) error( 3, __FILE__, __LINE__,
      "Requested size larger than block ", IntStr(size), "" );
  if( mb->dataOffset + size > blockSize )
    {
    if( mb->nextBlock )
      nb = mb->nextBlock;       /* next block already exists */
    else
      {                         /* else create next block */
      nb = (MEMBLOCK *)Alc_E( blockSize, name );
      nb->priorBlock = mb;      /* back linked list */
      mb->nextBlock = nb;       /* forward linked list */
      nb->nextBlock = NULL;
      nb->blockSize = blockSize;
      nb->dataOffset = sizeof(MEMBLOCK);
      }
    mb = nb;
    *block = (I1 *)(void *)nb;
    }
  p = *block + mb->dataOffset;
  mb->dataOffset += size;

  return (void *)p;

  }  /*  end of Alc_EC  */

/***  FreePolygons.c  ********************************************************/

/*  Restore list of polygon descriptions to free space.  */

void FreePolygons( POLY *first, POLY *last )
/* first;  - pointer to first polygon in linked list (not NULL).
 * last;   - pointer to polygon AFTER last one freed (NULL = complete list). */
  {
  POLY *pp; /* pointer to polygon */
  HCVE *pv; /* pointer to edge/vertex */

  for( pp=first; ; pp=pp->next )
    {
#if( DEBUG > 0 )
    if( !pp ) error( 3, __FILE__, __LINE__, "Polygon PP not defined", "" );
    if( !pp->firstVE ) error( 3, __FILE__, __LINE__, "FirstVE not defined", "" );
#endif
    pv = pp->firstVE->next;           /* free vertices (circular list) */
    while( pv->next != pp->firstVE )  /* find "end" of vertex list */
      pv = pv->next;
    pv->next = _nextFreeVE;           /* reset vertex links */
    _nextFreeVE = pp->firstVE;
    if( pp->next == last ) break;
    }
  pp->next = _nextFreePD;       /* reset polygon links */
  _nextFreePD = first;

#if( DEBUG > 0 )
  NullPointerTest( __FILE__, __LINE__ );
#endif

  }  /*  end of FreePolygons  */

/***  NewPolygonStack.c  *****************************************************/

/*  Start a new stack (linked list) of polygons.  */

void NewPolygonStack( void )
  {
  _nextUsedPD = NULL;  /* define bottom of stack */

  }  /* end NewPolygonStack */

/***  TopOfPolygonStack.c  ***************************************************/

/*  Return pointer to top of active polygon stack.  */

POLY *TopOfPolygonStack( void )
  {
  return _nextUsedPD;

  }  /* end TopOfPolygonStack */

/***  GetPolygonVrt2D.c  *****************************************************/

/*  Get polygon vertices.  Return number of vertices.
 *  Be sure polyVrt is sufficiently long.  */

IX GetPolygonVrt2D( const POLY *pp, VERTEX2D *polyVrt )
  {
  HCVE *pv;    /* pointer to HC vertices/edges */
  IX j=0;      /* vertex counter */

  pv = pp->firstVE;
  do{
    polyVrt[j].x = pv->x;
    polyVrt[j++].y = pv->y;
    pv = pv->next;
    } while( pv != pp->firstVE );

  return j;

  }  /*  end of GetPolygonVrt2D  */

/***  GetPolygonVrt3D.c  *****************************************************/

/*  Get polygon vertices.  Return number of vertices.
 *  Be sure polyVrt is sufficiently long.  */

IX GetPolygonVrt3D( const POLY *pp, VERTEX3D *polyVrt )
  {
  HCVE *pv;    /* pointer to HC vertices/edges */
  IX j=0;      /* vertex counter */

  pv = pp->firstVE;
  do{
    polyVrt[j].x = pv->x;
    polyVrt[j].y = pv->y;
    polyVrt[j++].z = 0.0;
    pv = pv->next;
    } while( pv != pp->firstVE );

  return j;

  }  /*  end of GetPolygonVrt3D  */

/***  InitPolygonMem.c  ******************************************************/

/*  Initialize polygon processing memory and globals.  */

void InitPolygonMem( const R4 epsdist, const R4 epsarea )
  {
  if( _memPoly )  /* clear existing polygon structures data */
    _memPoly = Clr_EC( _memPoly );
  else            /* allocate polygon structures heap pointer */
    _memPoly = Alc_ECI( 2000, "memPoly" );

  _epsDist = epsdist;
  _epsArea = epsarea;
  _nextFreeVE = NULL;
  _nextFreePD = NULL;
  _nextUsedPD = NULL;
#if( DEBUG > 1 )
  fprintf( _ulog, "InitPolygonMem: epsDist %g epsArea %g\n",
    _epsDist, _epsArea );
#endif

  }  /* end InitPolygonMem */

/***  FreePolygonMem.c  ******************************************************/

/*  Free polygon processing memory.  */

void FreePolygonMem( void )
  {
  if( _memPoly )
    _memPoly = (I1 *)Fre_EC( _memPoly, "memPoly" );

  }  /* end FreePolygonMem */

/***  Alc_ECI.c  *************************************************************/

/*  Block initialization for Alc_EC.  */

I1 *Alc_ECI( UX size, I1 *name )
/*  size;   size (bytes) of block being allocated.
 *  name;   name of block being allocated.  */
  {
  I1 *p;  /* pointer to the block */
  MEMBLOCK *mb;

  p = (I1 *)Alc_E( size, name );
  mb = (MEMBLOCK *)(void *)p;
  mb->priorBlock = NULL;
  mb->nextBlock = NULL;
  mb->blockSize = size;
  mb->dataOffset = sizeof(MEMBLOCK);

  return p;

  }  /*  end of Alc_ECI  */

/***  Clr_EC.c  **************************************************************/

/*  Clr_EC:  Clear (but do not free) blocks allocated by Alc_EC.  */

I1 *Clr_EC( I1 *block )
/*  block;  pointer to current memory block. */
  {
  IX header;  /* size of header data */
  I1 *p;      /* pointer to the block */
  MEMBLOCK *mb, *nb;

  header = sizeof(MEMBLOCK);
  mb = (MEMBLOCK *)(void *)block;
  while( mb )
    {
    p = (void *)mb;
    nb = mb->priorBlock;
    memset( p + header, 0, mb->blockSize - header );
    mb->dataOffset = header;
    mb = nb;
    }
  return p;

  }  /*  end of Clr_EC  */

/***  Fre_EC.c  **************************************************************/

/*  Free blocks allocated by Alc_EC.  */

void *Fre_EC( I1 *block, I1 *name )
/*  block;  pointer to current memory block.
 *  name;   name of structure being freed.  */
  {
  MEMBLOCK *mb, *nb;

  block = Clr_EC( block );

  mb = (MEMBLOCK *)(void *)block;
  if( mb )
    while( mb->nextBlock )  /* guarantee mb at end of list */
      mb = mb->nextBlock;
  else
    error( 3, __FILE__, __LINE__, "null block pointer, call George", "" );

  while( mb )
    {
    nb = mb->priorBlock;
    Fre_E( mb, mb->blockSize, name );
    mb = nb;
    }
  return (NULL);

  }  /*  end of Fre_EC  */

/***  LimitPolygon.c  ********************************************************/

/*  This function limits the polygon coordinates to a rectangle which encloses
 *  the base surface. Vertices may be in clockwise or counter-clockwise order.
 *  The polygon is checked against each edge of the window, with the portion
 *  inside the edge saved in the tempVrt or polyVrt array in turn.
 *  The code is long, but the loops are short.  
 *  Return the number of number of vertices in the clipped polygon
 *  and the clipped vertices in polyVrt.
 */

IX LimitPolygon( IX nVrt, VERTEX2D polyVrt[],
  const R4 maxX, const R4 minX, const R4 maxY, const R4 minY )
  {
  VERTEX2D tempVrt[MAXNV2];  /* temporary vertices */
  IX n, m;  /* vertex index */

                         /* test vertices against maxX */
  polyVrt[nVrt].x = polyVrt[0].x;
  polyVrt[nVrt].y = polyVrt[0].y; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( polyVrt[n].x < maxX)
      {
      tempVrt[m].x = polyVrt[n].x;
      tempVrt[m++].y = polyVrt[n].y;
      if( polyVrt[n+1].x > maxX)
        {
        tempVrt[m].x = maxX;
        tempVrt[m++].y = polyVrt[n].y + (maxX - polyVrt[n].x)
          * (polyVrt[n+1].y - polyVrt[n].y) / (polyVrt[n+1].x - polyVrt[n].x);
        }
      }
    else if( polyVrt[n].x > maxX )
      {
      if ( polyVrt[n+1].x < maxX )
        {
        tempVrt[m].x = maxX;
        tempVrt[m++].y = polyVrt[n].y + (maxX - polyVrt[n].x)
          * (polyVrt[n+1].y - polyVrt[n].y) / (polyVrt[n+1].x - polyVrt[n].x);
        }
      }
    else
      {
      tempVrt[m].x = polyVrt[n].x;
      tempVrt[m++].y = polyVrt[n].y;
      }
    }  /* end of maxX test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against minX */
  tempVrt[nVrt].x = tempVrt[0].x;
  tempVrt[nVrt].y = tempVrt[0].y; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( tempVrt[n].x > minX)
      {
      polyVrt[m].x = tempVrt[n].x;
      polyVrt[m++].y = tempVrt[n].y;
      if( tempVrt[n+1].x < minX)
        {
        polyVrt[m].x = minX;
        polyVrt[m++].y = tempVrt[n].y + (minX - tempVrt[n].x)
          * (tempVrt[n+1].y - tempVrt[n].y) / (tempVrt[n+1].x - tempVrt[n].x);
        }
      }
    else if( tempVrt[n].x < minX )
      {
      if ( tempVrt[n+1].x > minX )
        {
        polyVrt[m].x = minX;
        polyVrt[m++].y = tempVrt[n].y + (minX - tempVrt[n].x)
          * (tempVrt[n+1].y - tempVrt[n].y) / (tempVrt[n+1].x - tempVrt[n].x);
        }
      }
    else
      {
      polyVrt[m].x = tempVrt[n].x;
      polyVrt[m++].y = tempVrt[n].y;
      }
    }  /* end of minX test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against maxY */
  polyVrt[nVrt].y = polyVrt[0].y;
  polyVrt[nVrt].x = polyVrt[0].x; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( polyVrt[n].y < maxY)
      {
      tempVrt[m].y = polyVrt[n].y;
      tempVrt[m++].x = polyVrt[n].x;
      if( polyVrt[n+1].y > maxY)
        {
        tempVrt[m].y = maxY;
        tempVrt[m++].x = polyVrt[n].x + (maxY - polyVrt[n].y)
          * (polyVrt[n+1].x - polyVrt[n].x) / (polyVrt[n+1].y - polyVrt[n].y);
        }
      }
    else if( polyVrt[n].y > maxY )
      {
      if ( polyVrt[n+1].y < maxY )
        {
        tempVrt[m].y = maxY;
        tempVrt[m++].x = polyVrt[n].x + (maxY - polyVrt[n].y)
          * (polyVrt[n+1].x - polyVrt[n].x) / (polyVrt[n+1].y - polyVrt[n].y);
        }
      }
    else
      {
      tempVrt[m].y = polyVrt[n].y;
      tempVrt[m++].x = polyVrt[n].x;
      }
    }  /* end of maxY test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
                         /* test vertices against minY */
  tempVrt[nVrt].y = tempVrt[0].y;
  tempVrt[nVrt].x = tempVrt[0].x; 
  for( n=m=0; n<nVrt; n++ )
    {
    if( tempVrt[n].y > minY)
      {
      polyVrt[m].y = tempVrt[n].y;
      polyVrt[m++].x = tempVrt[n].x;
      if( tempVrt[n+1].y < minY)
        {
        polyVrt[m].y = minY;
        polyVrt[m++].x = tempVrt[n].x + (minY - tempVrt[n].y)
          * (tempVrt[n+1].x - tempVrt[n].x) / (tempVrt[n+1].y - tempVrt[n].y);
        }
      }
    else if( tempVrt[n].y < minY )
      {
      if ( tempVrt[n+1].y > minY )
        {
        polyVrt[m].y = minY;
        polyVrt[m++].x = tempVrt[n].x + (minY - tempVrt[n].y)
          * (tempVrt[n+1].x - tempVrt[n].x) / (tempVrt[n+1].y - tempVrt[n].y);
        }
      }
    else
      {
      polyVrt[m].y = tempVrt[n].y;
      polyVrt[m++].x = tempVrt[n].x;
      }
    }  /* end of minY test */
  nVrt = m;
  if( nVrt < 3 )
    return 0;
  return nVrt;    /* note: final results are in polyVrt */

  }  /*  end of LimitPolygon  */

#if( DEBUG > 0 )
/***  DumpHC.c  **************************************************************/

/*  Print the descriptions of sequential polygons.  */

void DumpHC( I1 *title, const POLY *pfp, const POLY *plp )
/*  pfp, plp; pointers to first and last (NULL acceptable) polygons  */
  {
  const POLY *pp;
  HCVE *pv;
  IX i, j;

  fprintf( _ulog, "%s\n", title );
  for( i=0,pp=pfp; pp; pp=pp->next )  /* polygon loop */
    {
    fprintf( _ulog, " pd [%p]", pp );
    fprintf( _ulog, "  area %.4g", pp->area );
    fprintf( _ulog, "  trns %.3g", pp->trns );
    fprintf( _ulog, "  next [%p]", pp->next );
    fprintf( _ulog, "  fve [%p]\n", pp->firstVE );
    if( ++i >= 100 ) error( 3, __FILE__, __LINE__, "Too many surfaces", "" );

    j = 0;
    pv = pp->firstVE;
    do{                  /* vertex/edge loop */
      fprintf( _ulog, "  ve [%p] %10.7f %10.7f %10.7f %10.7f %13.8f\n",
               pv, pv->x, pv->y, pv->a, pv->b, pv->c );
      pv = pv->next;
      if( ++j >= MAXNVT ) error( 3, __FILE__, __LINE__, "Too many vertices", "" );
      } while( pv != pp->firstVE );

    if( pp==plp ) break;
    }
  fflush( _ulog );

  }  /* end of DumpHC */

/*** DumpFreePolygons.c  *****************************************************/

void DumpFreePolygons( void )
  {
  POLY *pp;

  fprintf( _ulog, "FREE POLYGONS:" );
  for( pp=_nextFreePD; pp; pp=pp->next )
    fprintf( _ulog, " [%p]", pp );
  fprintf( _ulog, "\n" );

  }  /* end DumpFreePolygons */

/*** DumpFreeVertices.c  *****************************************************/

void DumpFreeVertices( void )
  {
  HCVE *pv;

  fprintf( _ulog, "FREE VERTICES:" );
  for( pv=_nextFreeVE; pv; pv=pv->next )
    fprintf( _ulog, " [%p]", pv );
  fprintf( _ulog, "\n" );

  }  /* end DumpFreeVertices */

/***  DumpP2D.c  *************************************************************/

/*  Dump 2-D polygon vertex data.  */

void DumpP2D( I1 *title, const IX nvs, VERTEX2D *vs )
  {
  IX n;

  fprintf( _ulog, "%s\n", title );
  fprintf( _ulog, " nvs: %d\n", nvs );
  for( n=0; n<nvs; n++)
    fprintf( _ulog, "  vs: %d %12.7f %12.7f\n", n, vs[n].x, vs[n].y );
  fflush( _ulog );

  }  /* end of DumpP2D */

/***  DumpP3D.c  *************************************************************/

/*  Dump 3-D polygon vertex data. */

void DumpP3D( I1 *title, const IX nvs, VERTEX3D *vs )
  {
  IX n;

  fprintf( _ulog, "%s\n", title );
  fprintf( _ulog, " nvs: %d\n", nvs );
  for( n=0; n<nvs; n++)
    fprintf( _ulog, "  vs: %d %12.7f %12.7f %12.7f\n",
             n, vs[n].x, vs[n].y, vs[n].z );
  fflush( _ulog );

  }  /* end of DumpP3D */
#endif  /* end DEBUG */

