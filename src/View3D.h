/*subfile:  view3d.h *********************************************************/
/*  include file for the VIEW3D program */

#include <string.h> /* prototype: memcpy */

#define MAXNV 4     /* max number of vertices for an initial surface */
#define MAXNV1 5    /* max number of vertices after 1 clip */
#define MAXNV2 9    /* max number of vertices clipped projection */
#define MAXNVT 12   /* maximum number of temporary vertices */
#define NAMELEN 12  /* length of a name */
#ifdef XXX
#define PI       3.141592653589793238
#define PIt2     6.283185307179586477   /* 2 * pi */
#define PIt4    12.566370614359172954   /* 4 * pi */
#define PId2     1.570796326794896619   /* pi / 2 */
#define PId4     0.785398163397448310   /* pi / 4 */
#define PIinv    0.318309886183790672   /* 1 / pi */
#define PIt2inv  0.159154943091895346   /* 1 / (2 * pi) */
#define PIt4inv  0.079577471545947673   /* 1 / (4 * pi) */
#endif
#define RTD 57.2957795  /* convert radians to degrees */
#define EPS 1.0e-6
#define EPS2 1.0e-12

typedef struct v2d       /* structure for 2D vertex */
  {
  R4  x;  /* X-coordinate */
  R4  y;  /* Y-coordinate */
  } VERTEX2D;

typedef struct v3d       /* structure for a 3D vertex or vector */
  {
  R4  x;  /* X-coordinate */
  R4  y;  /* Y-coordinate */
  R4  z;  /* Z-coordinate */
  } V3D;

#define VERTEX3D V3D
#define VECTOR3D V3D

/* vector macros using pointer to 3-element structures */

/*  VECTOR:  define vector C from vertex A to vextex B.  */
#define VECTOR(a,b,c)   \
    c->x = b->x - a->x; \
    c->y = b->y - a->y; \
    c->z = b->z - a->z; 

/*  VCOPY:  copy elements from vertex/vector A to B.  */
#define VCOPY(a,b)  { b->x = a->x; b->y = a->y; b->z = a->z; }

/*  VMID:  define vertex C midway between vertices A and B.  */
#define VMID(a,b,c)   \
    c->x = 0.5f * (a->x + b->x); \
    c->y = 0.5f * (a->y + b->y); \
    c->z = 0.5f * (a->z + b->z); 

/*  VDOT:  compute dot product of vectors A and B.  */
#define VDOT(a,b)   (a->x * b->x + a->y * b->y + a->z * b->z)

/*  VDOTW:  dot product of a vertex, V, and direction cosines, C.  */
#define VDOTW(v,c)  (c->w + VDOT(v,c))

/*  VLEN:  compute length of vector A.  */
#define VLEN(a)     (R4)sqrt( VDOT(a,a) )

/*  VCROSS:  compute vector C as cross product of A and B.  */
#define VCROSS(a,b,c)   \
    c->x = a->y * b->z - a->z * b->y; \
    c->y = a->z * b->x - a->x * b->z; \
    c->z = a->x * b->y - a->y * b->x; 

/*  VSCALE:  vector B = scalar C times vector A.  */
#define VSCALE(c,a,b)  \
    b->x = c * a->x; \
    b->y = c * a->y; \
    b->z = c * a->z; 

typedef struct dircos         /* structure for direction cosines */
  {
  R4  x;  /* X-direction cosine */
  R4  y;  /* Y-direction cosine */
  R4  z;  /* Z-direction cosine */
  R4  w;  /* distance from surface to origin (signed) */
  } DIRCOS;

typedef struct srfdat3d       /* structure for 3D surface data */
  {
  IX nr;              /* surface number */
  IX nv;              /* number of vertices */
  IX shape;           /* 3 = triangle; 4 = parallelogram; 0 = other */
  R4 area;            /* area of surface */
  R4 rc;              /* radius enclosing the surface */
  DIRCOS dc;          /* direction cosines of surface normal */
  VERTEX3D ctd;       /* coordinates of centroid */
  VERTEX3D *v[MAXNV]; /* pointers to coordinates of up to MAXNV vertices */
  IX NrelS;           /* orientation of srf N relative to S:
                         -1: N behind S; +1: N in front of S;
                          0: part of N behind S, part in front */
  IX MrelS;           /* orientation of srf M relative to S */
  IX type;            /* surface type data - defined below */
  } SRFDAT3D;

#define RSRF 0  /* normal surface */
#define SUBS 1  /* subsurface */
#define MASK 2  /* mask surface */
#define NULS 3  /* null surface */
#define OBSO 4  /* obstruction only surface */

typedef struct srfdatnm       /* structure for 3D surface data */
  {
  IX nr;              /* surface number */
  IX nv;              /* number of vertices */
  IX shape;           /* 3 = triangle; 4 = rectangle; 0 = other */
  R4 area;            /* area of surface */
  R4 rc;              /* radius enclosing the surface */
  DIRCOS dc;          /* direction cosines of surface normal */
  VERTEX3D ctd;       /* coordinates of centroid */
  VERTEX3D v[MAXNV1]; /* coordinates of vertices */
  R4 dist[MAXNV1];    /* distances of vertices above plane of other surface */
  } SRFDATNM;

typedef struct srfdat3x       /* structure for 3D surface data */
  {
  IX nr;              /* surface number */
  IX nv;              /* number of vertices */
  IX shape;           /* 3 = triangle; 4 = rectangle; 0 = other */
  R4 area;            /* surface area  */
  R4 ztmax;           /* maximum Z-coordinate of surface */
  DIRCOS dc;          /* direction cosines of surface normal */
  VERTEX3D ctd;       /* coordinates of centroid */
  VERTEX3D v[MAXNV1]; /* coordinates of vertices */
  } SRFDAT3X;

typedef struct edgedcs    /* structure for direction cosines of polygon edge */
  {
  R4  x;  /* X-direction cosine */
  R4  y;  /* Y-direction cosine */
  R4  z;  /* Z-direction cosine */
  R4  s;  /* length of edge */
  } EDGEDCS;

typedef struct edgediv    /* structure for Gaussian division of polygon edge */
  {
  R4  x;  /* X-coordinate of element */
  R4  y;  /* Y-coordinate of element */
  R4  z;  /* Z-coordinate of element */
  R4  s;  /* length of element */
  } EDGEDIV;

typedef struct          /* view factor calculation control values */
  {
  IX nAllSrf;       /* total number of surfaces */
  IX nRadSrf;       /* number of radiating surfaces; 
                         initially includes mask & null surfaces */
  IX nMaskSrf;      /* number of mask & null surfaces */
  IX nObstrSrf;     /* number of obstruction surfaces */
  IX nVertices;     /* number of vertices */
  IX format;        /* geometry format: 3 or 4 */
  IX outFormat;     /* output file format */
  IX row;           /* row to solve; 0 = all rows */
  IX col;           /* column to solve; 0 = all columns */
  IX enclosure;     /* 1 = surfaces form an enclosure */
  IX emittances;    /* 1 = process emittances */
  IX nPossObstr;    /* number of possible view obstructing surfaces */
  IX nProbObstr;    /* number of probable view obstructing surfaces */
  IX prjReverse;    /* projection control; 0 = normal, 1 = reverse */
  R4 epsAdap;       /* convergence for adaptive integration */
  R4 rcRatio;       /* rRatio of surface radii */
  R4 relSep;        /* surface separation / sum of radii */
  IX method;        /* 0 = 2AI, 1 = 1AI, 2 = 2LI, 3 = 1LI, 4 = ALI */
  IX nEdgeDiv;      /* number of edge divisions */
  IX maxRecursALI;  /* max number of ALI recursion levels */
  U4 usedV1LIadapt; /* number of V1LIadapt() calculations used */
  IX failViewALI;   /* 1 = unobstructed view factor did not converge */
  IX maxRecursion;  /* maximum number of recursion levels */
  IX minRecursion;  /* minimum number of recursion levels */
  IX failRecursion; /* 1 = obstructed view factor did not converge */
  R4 epsAF;         /* convergence for current AF calculation */
  U4 wastedVObs;    /* number of ViewObstructed() calculations wasted */
  U4 usedVObs;      /* number of ViewObstructed() calculations used */
  U4 totPoly;       /* total number of polygon view factors */
  U4 totVpt;        /* total number of view points */
  IX failConverge;  /* 1 if any calculation failed to converge */
  SRFDAT3X srf1T;   /* participating surface; transformed coordinates */
  SRFDAT3X srf2T;   /* participating surface; transformed coordinates;
                       view from srf1T toward srf2T. */ 
  SRFDAT3X *srfOT;  /* pointer to array of view obstrucing surfaces;
                       dimensioned from 0 to maxSrfT in View3d();
                       coordinates transformed relative to srf2T. */
  } VFCTRL;

#define UNK -1  /* unknown integration method */
#define DAI 0   /* double area integration */
#define SAI 1   /* single area integration */
#define DLI 2   /* double line integration */
#define SLI 3   /* single line integration */
#define ALI 4   /* adaptive line integration */

typedef struct hcve   /* homogeneous coordinate description of vertex/edge */
  {
  struct hcve *next;  /* pointer to next vertex/edge */
  R4 x, y;    /* X and Y coordinates of the vertex */
  R4 a, b;    /* A, B */
  R4 c;       /*  & C homogeneous coordinates of the edge */
  } HCVE;

typedef struct poly   /* description of a polygon */
  {
  struct poly *next;  /* pointer to next polygon */
  HCVE *firstVE;      /* pointer to first vertex of polygon */
  R4 trns;            /* (0.0 <= transparency <= 1.0) */
  R4 area;            /* area of the polygon */
  } POLY;

/* macros for simple mathematical operations */
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))   /* max of 2 values */
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))   /* min of 2 values */

