/*subfile:  prtyp.h  *********************************************************/
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

/*  function prototypes.  */

     /* input / output */
void CountVS3D( I1 *title, VFCTRL *vfCtrl );
void GetVS3D( I1 **name, R4 *emit, IX *base, IX *cmbn,
  SRFDAT3D *srf, VERTEX3D *xyz, VFCTRL *vfCtrl );
void GetVS3Da( I1 **name, R4 *emit, IX *base, IX *cmbn,
  SRFDAT3D *srf, VERTEX3D *xyz, VFCTRL *vfCtrl );
R8 VolPrism( VERTEX3D *a, VERTEX3D *b, VERTEX3D *c );
void SetPlane( SRFDAT3D *srf );
void ReportAF( const IX nSrf, const IX encl, const I1 *title, const I1 **name, 
  const R4 *area, const R4 *emit, const IX *base, const R8 **AF, IX flag );

     /* 3-D view factor functions */
void View3D( SRFDAT3D *srf, const IX *base, IX *possibleObstr,
  R8 **AF, VFCTRL *vfCtrl );
IX ProjectionDirection( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm,
  IX *los, VFCTRL *vfCtrl );
IX errorf( IX severity, I1 *file, IX line, ... );

R8 ViewUnobstructed( VFCTRL *vfCtrl, IX row, IX col );
R8 View2AI( const IX nss1, const DIRCOS *dc1, const VERTEX3D *pt1, const R4 *area1,
            const IX nss2, const DIRCOS *dc2, const VERTEX3D *pt2, const R4 *area2 );
R8 View2LI( const IX nd1, const IX nv1, const EDGEDCS *rc1, const EDGEDIV **dv1,
  const IX nd2, const IX nv2, const EDGEDCS *rc2, const EDGEDIV **dv2 );
R8 View1LI( const IX nd1, const IX nv1, const EDGEDCS *rc1,
  const EDGEDIV **dv1, const VERTEX3D *v1, const IX nv2, const VERTEX3D *v2 );
R8 V1LIpart( const VERTEX3D *pp, const VERTEX3D *b0, const VERTEX3D *b1,
  const VECTOR3D *B, const R8 b2, IX *flag );
R8 V1LIxact( const VERTEX3D *a0, const VERTEX3D *a1, const R8 a, 
  const VERTEX3D *b0, const VERTEX3D *b1, const R8 b );
R8 V1LIadapt( VERTEX3D Pold[3], R8 dFold[3], R8 h, const VERTEX3D *b0,
  const VERTEX3D *b1, const VECTOR3D *B, const R8 b2, IX level, VFCTRL *vfCtrl );
R8 ViewALI( const IX nv1, const VERTEX3D *v1,
  const IX nv2, const VERTEX3D *v2, VFCTRL *vfCtrl );
void ViewsInit( IX maxDiv, IX init );
IX DivideEdges( IX nd, IX nv, VERTEX3D *vs, EDGEDCS *rc, EDGEDIV **dv );
IX GQParallelogram( const IX nDiv, const VERTEX3D *vp, VERTEX3D *p, R4 *w );
IX GQTriangle( const IX nDiv, const VERTEX3D *vt, VERTEX3D *p, R4 *w );
IX SubSrf( const IX nDiv, const IX nv, const VERTEX3D *v, const R4 area,
  VERTEX3D *pt, R4 *wt );

R8 ViewObstructed( VFCTRL *vfCtrl, IX nv1, VERTEX3D v1[], R4 area, IX nDiv );
R8 View1AI( IX nss, VERTEX3D *p1, R4 *area1, DIRCOS *dc1, SRFDAT3X *srf2 );
R8 V1AIpart( const IX nv, const VERTEX3D p2[],
            const VERTEX3D *p1, const DIRCOS *u1 );
IX Subsurface( SRFDAT3X *srf, SRFDAT3X sub[] );
R4 SetCentroid( const IX nv, VERTEX3D *vs, VERTEX3D *ctd );
R4 Triangle( VERTEX3D *p1, VERTEX3D *p2, VERTEX3D *p3, void *dc, IX dcflag );
void substs( IX n, VERTEX3D v[], VERTEX3D s[] );
R8 ViewTP( VERTEX3D v1[], R4 area, IX level, VFCTRL *vfCtrl );
R8 ViewRP( VERTEX3D v1[], R4 area, IX level, VFCTRL *vfCtrl );

     /* 3-D view test functions */
IX AddMaskSrf( SRFDAT3D *srf, const SRFDATNM *srfN, const SRFDATNM *srfM,
  const IX *maskSrf, const IX *baseSrf, VFCTRL *vfCtrl, IX *los, IX nPoss );
IX BoxTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm, VFCTRL *vfCtrl,
  IX *los, IX nProb );
IX ClipPolygon( const R4 flag, const IX nv, VERTEX3D *v,
  R4 *dot, VERTEX3D *vc );
IX ConeRadiusTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm,
  VFCTRL *vfCtrl, IX *los, IX nProb, R4 distNM );
IX CylinderRadiusTest( SRFDAT3D *srf, SRFDATNM *srfN, SRFDATNM *srfM,
  IX *los, R4 distNM, IX nProb );
IX OrientationTest( SRFDAT3D *srf, SRFDATNM *srfn, SRFDATNM *srfm, 
  VFCTRL *vfCtrl, IX *los, IX nProb );
IX OrientationTestN( SRFDAT3D *srf, IX N, VFCTRL *vfCtrl,
  IX *possibleObstr, IX nPossObstr );
void SelfObstructionClip( SRFDATNM *srfn );
IX SetShape( const IX nv, VERTEX3D *v, R4 *area );
IX SelfObstructionTest3D( SRFDAT3D *srf1, SRFDAT3D *srf2, SRFDATNM *srfn );
void IntersectionTest( SRFDATNM *srfn, SRFDATNM *srfm );
void DumpOS( I1 *title, const IX nos, IX *los );
IX SetPosObstr3D( IX nSrf, SRFDAT3D *srf, IX *lpos );

     /* polygon processing */
IX PolygonOverlap( const POLY *p1, POLY *p2, const IX flagOP, IX freeP2 );
void FreePolygons( POLY *first, POLY *last );
POLY *SetPolygonHC( const IX nVrt, const VERTEX2D *polyVrt, const R4 trns );
IX GetPolygonVrt2D( const POLY *pp, VERTEX2D *polyVrt );
IX GetPolygonVrt3D( const POLY *pp, VERTEX3D *srfVrt );
POLY *GetPolygonHC( void );
HCVE *GetVrtEdgeHC( void );
void NewPolygonStack( void );
POLY *TopOfPolygonStack( void );
void InitPolygonMem( const R4 epsDist, const R4 epsArea );
void FreePolygonMem( void );
IX LimitPolygon( IX nVrt, VERTEX2D polyVrt[],
  const R4 maxX, const R4 minX, const R4 maxY, const R4 minY );
void DumpHC( I1 *title, const POLY *pfp, const POLY *plp );
void DumpFreePolygons( void );
void DumpFreeVertices( void );
void DumpP2D( I1 *title, const IX nvs, VERTEX2D *vs );
void DumpP3D( I1 *title, const IX nvs, VERTEX3D *vs );

     /* vector functions */
void CoordTrans3D( SRFDAT3D *srfAll, SRFDATNM *srf1, SRFDATNM *srf2,
  IX *probableObstr, VFCTRL *vfCtrl );
void Dump3X( I1 *tittle, SRFDAT3X *srfT );
void DumpVA( I1 *title, const IX rows, const IX cols, R4 *a );

     /* post processing */
IX DelNull( const IX nSrf, SRFDAT3D *srf, IX *base, IX *cmbn,
  R4 *emit, R4 *area, I1 **name, R8 **AF );
void NormAF( const nSrf, const R4 *emit, const R4 *area, R8 **AF,
  const R4 eMax, const IX itMax );
IX Combine( const IX nSrf, const IX *cmbn, R4 *area, I1 **name, R8 **AF );
void Separate( const IX nSrf, const IX *base, R4 *area, R8 **AF );
void IntFac( const IX nSrf, const R4 *emit, const R4 *area, R8 **AF );
void LUFactorSymm( const IX neq, R8 **a );
void LUSolveSymm( const IX neq, const R8 **a, R8 *b );
void DAXpY( const IX n, const R8 a, const R8 *x, R8 *y );
R8 DotProd( const IX n, const R8 *x, const R8 *y );

     /* miscellaneous functions */
R4 CPUTime( R4 t1 );
void pathmerge( I1 *fullpath, I1 *drv, I1 *path, I1 *file, I1 *ext );
void pathsplit( I1 *fullpath, I1 *drv, I1 *path, I1 *file, I1 *ext );
void finish( IX flag );
void errora( const I1 *head, I1 *message, I1 *source );
void errorb( const I1 *head, I1 *message, I1 *source );
IX error( IX severity, I1 *file, IX line, ... );
I1 *FltStr( R8 f, IX n );
I1 *IntStr( I4 i );
I1 *StrCpyS( I1 *s, const IX mx, ...  );
IX StrEql( I1 *s1, I1 *s2 );
void NxtOpen( I1 *file_name, I1 *file, IX line );
void NxtClose( void );
I1 *NxtLine( I1 *str, IX maxlen );
I1 *NxtWord( I1 *str, IX flag, IX maxlen );
IX IntCon( I1 *s, I2 *i );
I2 ReadI2( IX flag );
IX FltCon( I1 *s, R4 *f );
R4 ReadR4( IX flag );

     /* heap processing */
void *Alc_E( UX length, I1 *name );
IX Chk_E( void *pm, UX length, I1 *name );
void *Fre_E( void *pm, UX length, I1 *name );
void *Alc_EC( I1 **block, UX size, I1 *name );
I1 *Alc_ECI( UX size, I1 *name );
I1 *Clr_EC( I1 *block );
void *Fre_EC( I1 *block, I1 *name );
void *Alc_MC( IX min_row_index, IX max_row_index, IX min_col_index,
              IX max_col_index, IX size, I1 *name );
void *Fre_MC( void *v, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name );
void *Alc_MR( IX min_row_index, IX max_row_index, IX min_col_index,
              IX max_col_index, IX size, I1 *name );
void *Fre_MR( void *v, IX min_row_index, IX max_row_index, IX min_col_index,
             IX max_col_index, IX size, I1 *name );
void *Alc_MSR( IX minIndex, IX maxIndex, IX size, I1 *name );
void *Fre_MSR( void *v, IX minIndex, IX maxIndex, IX size, I1 *name );
void *Alc_V( IX minIndex, IX maxIndex, IX size, I1 *name );
void *Fre_V( void *v, IX minIndex, IX maxIndex, IX size, I1 *name );
IX MemWalk( void );
I1 *MemRem( I1 *string );
IX NullPointerTest( I1 *file, IX line );

