/* If SINGLE is defined when triangle.o is compiled, it should also be       */
/*   defined here.  If not, it should not be defined here.                   */

/* #define SINGLE */

#include <float.h>

#ifdef SINGLE
#define TRIREAL float
#else /* not SINGLE */
#define TRIREAL double
#endif /* not SINGLE */

#include "triangle.h"

#ifdef SINGLE
#define MAXPRECISION (-FLT_MIN_10_EXP + FLT_DIG + 1)
#else
#define MAXPRECISION (-DBL_MIN_10_EXP + DBL_DIG + 1)
#endif

/*****************************************************************************/
/*                                                                           */
/*  report()   Print the input or output.                                    */
/*                                                                           */
/*****************************************************************************/

#ifdef ANSI_DECLARATORS
void report(struct triangulateio *io, int markers, int reporttriangles,
            int reportneighbors, int reportsegments,
            int reportedges, int reportnorms)
#else /* not ANSI_DECLARATORS */
void report(io, markers, reporttriangles, reportneighbors, reportsegments,
            reportedges, reportnorms)
struct triangulateio *io;
int markers;
int reporttriangles;
int reportneighbors;
int reportsegments;
int reportedges;
int reportnorms;
#endif /* not ANSI_DECLARATORS */
{
  int i, j;

  for (i = 0; i < io->numberofpoints; i++) {
    printf("Point %4d:", i);
    for (j = 0; j < 2; j++) {
      printf("  %.6g", io->pointlist[i * 2 + j]);
    }
    if (io->numberofpointattributes > 0) {
      printf("   attributes");
    }
    for (j = 0; j < io->numberofpointattributes; j++) {
      printf("  %.6g",
             io->pointattributelist[i * io->numberofpointattributes + j]);
    }
    if (markers) {
      printf("   marker %d\n", io->pointmarkerlist[i]);
    } else {
      printf("\n");
    }
  }
  printf("\n");

  if (reporttriangles || reportneighbors) {
    for (i = 0; i < io->numberoftriangles; i++) {
      if (reporttriangles) {
        printf("Triangle %4d points:", i);
        for (j = 0; j < io->numberofcorners; j++) {
          printf("  %4d", io->trianglelist[i * io->numberofcorners + j]);
        }
        if (io->numberoftriangleattributes > 0) {
          printf("   attributes");
        }
        for (j = 0; j < io->numberoftriangleattributes; j++) {
          printf("  %.6g", io->triangleattributelist[i *
                                         io->numberoftriangleattributes + j]);
        }
        printf("\n");
      }
      if (reportneighbors) {
        printf("Triangle %4d neighbors:", i);
        for (j = 0; j < 3; j++) {
          printf("  %4d", io->neighborlist[i * 3 + j]);
        }
        printf("\n");
      }
    }
    printf("\n");
  }

  if (reportsegments) {
    for (i = 0; i < io->numberofsegments; i++) {
      printf("Segment %4d points:", i);
      for (j = 0; j < 2; j++) {
        printf("  %4d", io->segmentlist[i * 2 + j]);
      }
      if (markers) {
        printf("   marker %d\n", io->segmentmarkerlist[i]);
      } else {
        printf("\n");
      }
    }
    printf("\n");
  }

  if (reportedges) {
    for (i = 0; i < io->numberofedges; i++) {
      printf("Edge %4d points:", i);
      for (j = 0; j < 2; j++) {
        printf("  %4d", io->edgelist[i * 2 + j]);
      }
      if (reportnorms && (io->edgelist[i * 2 + 1] == -1)) {
        for (j = 0; j < 2; j++) {
          printf("  %.6g", io->normlist[i * 2 + j]);
        }
      }
      if (markers) {
        printf("   marker %d\n", io->edgemarkerlist[i]);
      } else {
        printf("\n");
      }
    }
    printf("\n");
  }
}

/*****************************************************************************/
/*                                                                           */
/*  main()   Create and refine a mesh.                                       */
/*                                                                           */
/*****************************************************************************/

SEXP R_triangulate (SEXP P, SEXP PB, SEXP PA, SEXP S, SEXP SB, SEXP(H), SEXP a, SEXP q, SEXP Y, SEXP SS, SEXP j, SEXP D, SEXP V, SEXP Q)
{
  /* Output variables */
  SEXP oP, oPB, oPA, oT, oS, oSB, oE, oEB, oPV, oEV, oNV, oAV, oN;
  SEXP ans;
  double *xoP, *xoPA, *xoPV, *xoNV, *xoAV;
  int *xoT, *xoPB, *xoS, *xoSB, *xoE, *xoEB, *xoEV, *xoN;
  
  /* Convert input point matrix into array */
  PROTECT(P = AS_NUMERIC(P));
  /* Convert input boundary markers into array */
  // PROTECT(B = AS_NUMERIC(B));

  /* Create the triangulateio structures */
  struct triangulateio in, mid, vorout;

  in.numberofpoints = LENGTH(P)/2;
  in.pointlist = REAL(P);

  in.pointmarkerlist = INTEGER(PB);

  in.numberofpointattributes = Rf_ncols(PA);   
  in.pointattributelist = REAL(PA);

  in.numberofsegments = LENGTH(S)/2;
  in.segmentlist = INTEGER(S);
  in.segmentmarkerlist = INTEGER(SB);
  in.numberofholes = LENGTH(H)/2;
  in.holelist = REAL(H);
  in.numberofregions = 0;

  if (isInteger(V)) {
    if (*INTEGER(V) >= 1) {
      printf("Input point set:\n\n");
      report(&in, 1, 0, 0, 1, 0, 0);
    }
  }

  /* Make necessary initializations so that Triangle can return a */
  /*   triangulation in `mid' and a voronoi diagram in `vorout'.  */

  mid.pointlist = (TRIREAL *) NULL;            /* Not needed if -N switch used. */
  /* Not needed if -N switch used or number of point attributes is zero: */
  mid.pointattributelist = (TRIREAL *) NULL;
  mid.pointmarkerlist = (int *) NULL; /* Not needed if -N or -B switch used. */
  mid.trianglelist = (int *) NULL;          /* Not needed if -E switch used. */
  /* Not needed if -E switch used or number of triangle attributes is zero: */
  mid.triangleattributelist = (TRIREAL *) NULL;
  mid.neighborlist = (int *) NULL;         /* Needed only if -n switch used. */
  /* Needed only if segments are output (-p or -c) and -P not used: */
  mid.segmentlist = (int *) NULL;
  /* Needed only if segments are output (-p or -c) and -P and -B not used: */
  mid.segmentmarkerlist = (int *) NULL;
  mid.edgelist = (int *) NULL;             /* Needed only if -e switch used. */
  mid.edgemarkerlist = (int *) NULL;   /* Needed if -e used and -B not used. */

  vorout.pointlist = (TRIREAL *) NULL;        /* Needed only if -v switch used. */
  /* Needed only if -v switch used and number of attributes is not zero: */
  vorout.pointattributelist = (TRIREAL *) NULL;
  vorout.edgelist = (int *) NULL;          /* Needed only if -v switch used. */
  vorout.normlist = (TRIREAL *) NULL;         /* Needed only if -v switch used. */

  /* Triangulate the points.  Switches are chosen to read and write a  */
  /*   PSLG (p), preserve the convex hull (c), assign a regional       */
  /*   attribute to each element (A), and                              */
  /*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
  /*   neighbor list (n).                                              */

  char flags[MAXPRECISION*6 + 20];
  strcpy(flags, "pevn");
  char opts[MAXPRECISION*2 + 1];
  /* If the segment list is empty, enclose the convex hull with */
  /* so that the triangulation is not eaten up. See documentation in */
  /*   triangle.c (-c folag) for more information */
  if (LENGTH(S)==0) {  
    strcat(flags, "c");
  }
  if (isReal(a)) {
    snprintf(opts, MAXPRECISION*2, "a%.*f", MAXPRECISION, *REAL(a));
    strncat(flags, opts, MAXPRECISION*2 + 1);
  }
  if (isReal(q)) {
    snprintf(opts, MAXPRECISION*2, "q%.*f", MAXPRECISION, *REAL(q));
    strncat(flags, opts, MAXPRECISION*2 + 1);
  }
  if (isLogical(Y)) {
    if (*LOGICAL(Y) == TRUE) {
      strcat(flags, "Y");
    }
  }
  if (isInteger(SS)) {
    if (*INTEGER(SS) >= 0) {
      snprintf(opts, MAXPRECISION*2, "S%i", *INTEGER(SS));
      strncat(flags, opts, MAXPRECISION*2 + 1);
    }
  }
  if (isLogical(j)) {
    if (*LOGICAL(j) == TRUE) {
      strncat(flags, "j", 2);
    }
  }
  if (isLogical(D)) {
    if (*LOGICAL(D) == TRUE) {
      strncat(flags, "D", 2);
    }
  }
  if (isInteger(V)) {
    if (*INTEGER(V) == 1) {
      strncat(flags, "V", 2);
    }
    if (*INTEGER(V) == 2) {
      strncat(flags, "VV", 3);
    }
  }
  if (isLogical(Q)) {
    if (*LOGICAL(Q) == TRUE) {
      strncat(flags, "Q", 2);
    }
  }

  triangulate(flags, &in, &mid, &vorout);

  if (isInteger(V)) {
    if (*INTEGER(V) >= 1) {
      printf("Initial triangulation:\n\n");
      report(&mid, 1, 1, 1, 1, 1, 0);
    }
  }
  /* printf("Initial Voronoi diagram:\n\n");
     report(&vorout, 0, 0, 0, 0, 1, 1); */

  /* Attach area constraints to the triangles in preparation for */
  /*   refining the triangulation.                               */

  /* /\* Needed only if -r and -a switches used: *\/ */
  /* mid.trianglearealist = (TRIREAL *) malloc(mid.numberoftriangles * sizeof(TRIREAL)); */
  /* mid.trianglearealist[0] = 3.0; */
  /* mid.trianglearealist[1] = 1.0; */

  /* /\* Make necessary initializations so that Triangle can return a *\/ */
  /* /\*   triangulation in `out'.                                    *\/ */

  /* out.pointlist = (TRIREAL *) NULL;            /\* Not needed if -N switch used. *\/ */
  /* /\* Not needed if -N switch used or number of attributes is zero: *\/ */
  /* out.pointattributelist = (TRIREAL *) NULL; */
  /* out.trianglelist = (int *) NULL;          /\* Not needed if -E switch used. *\/ */
  /* /\* Not needed if -E switch used or number of triangle attributes is zero: *\/ */
  /* out.triangleattributelist = (TRIREAL *) NULL; */

  /* /\* Refine the triangulation according to the attached *\/ */
  /* /\*   triangle area constraints.                       *\/ */

  /* triangulate("praBP", &mid, &out, (struct triangulateio *) NULL); */

  /* printf("Refined triangulation:\n\n"); */
  /* report(&out, 0, 1, 0, 0, 0, 0); */

  /* Make space for answers */
  PROTECT(oP  = allocMatrix(REALSXP,  mid.numberofpoints, 2));
  PROTECT(oPB = allocMatrix(INTSXP,   mid.numberofpoints, 1));
  PROTECT(oPA = allocMatrix(REALSXP,  mid.numberofpoints, mid.numberofpointattributes));
  PROTECT(oT  = allocMatrix(INTSXP,   mid.numberoftriangles, 3));
  PROTECT(oS  = allocMatrix(INTSXP,   mid.numberofsegments, 2));
  PROTECT(oSB = allocMatrix(INTSXP,   mid.numberofsegments, 1));
  PROTECT(oE  = allocMatrix(INTSXP,   mid.numberofedges, 2));
  PROTECT(oEB = allocMatrix(INTSXP,   mid.numberofedges, 1));
  PROTECT(oPV  = allocMatrix(REALSXP, vorout.numberofpoints, 2));
  PROTECT(oEV  = allocMatrix(INTSXP,  vorout.numberofedges, 2));
  PROTECT(oNV  = allocMatrix(REALSXP, vorout.numberofpoints, 2));
  PROTECT(oAV  = allocMatrix(REALSXP, vorout.numberofpoints, mid.numberofpointattributes));
  PROTECT(oN  = allocMatrix(INTSXP,   mid.numberoftriangles, 3));

  xoP = REAL(oP);
  for (int i = 0; i < mid.numberofpoints; i++) {
    for (int j = 0; j < 2; j++) {
      xoP[j * mid.numberofpoints + i] = mid.pointlist[i * 2 + j];
    }
  }

  xoPB = INTEGER(oPB);
  for (int i = 0; i < mid.numberofpoints; i++) {
    xoPB[i] = mid.pointmarkerlist[i];
  }

  xoPA = REAL(oPA);
  for (int i = 0; i < mid.numberofpoints; i++) {
    for (int j = 0; j < mid.numberofpointattributes; j++) {
      xoPA[j * mid.numberofpoints + i] = mid.pointattributelist[i * mid.numberofpointattributes + j];
    }
  }

  xoT = INTEGER(oT);
  for (int i = 0; i < mid.numberoftriangles; i++) {
    for (int j = 0; j < mid.numberofcorners; j++) {
      xoT[j * mid.numberoftriangles + i] = mid.trianglelist[i * mid.numberofcorners + j];
    }
  }
  
  xoN = INTEGER(oN);
  for (int i = 0; i < mid.numberoftriangles; i++) {
    for (int j = 0; j < mid.numberofcorners; j++) {
      xoN[j * mid.numberoftriangles + i] = mid.neighborlist[i * mid.numberofcorners + j];
    }
  }
  
  xoS = INTEGER(oS);
  for (int i = 0; i < mid.numberofsegments; i++) {
    for (int j = 0; j < 2; j++) {
      xoS[j * mid.numberofsegments + i] = mid.segmentlist[i * 2 + j];
    }
  }

  xoSB = INTEGER(oSB);
  for (int i = 0; i < mid.numberofsegments; i++) {
    xoSB[i] = mid.segmentmarkerlist[i];
  }

  xoE = INTEGER(oE);
  for (int i = 0; i < mid.numberofedges; i++) {
    for (int j = 0; j < 2; j++) {
      xoE[j * mid.numberofedges + i] = mid.edgelist[i * 2 + j];
    }
  }

  xoEB = INTEGER(oEB);
  for (int i = 0; i < mid.numberofedges; i++) {
    xoEB[i] = mid.edgemarkerlist[i];
  }

  xoPV = REAL(oPV);
  for (int i = 0; i < vorout.numberofpoints; i++) {
    for (int j = 0; j < 2; j++) {
      xoPV[j * vorout.numberofpoints + i] = vorout.pointlist[i * 2 + j];
    }
  }

  xoEV = INTEGER(oEV);
  for (int i = 0; i < vorout.numberofedges; i++) {
    for (int j = 0; j < 2; j++) {
      xoEV[j * vorout.numberofedges + i] = vorout.edgelist[i * 2 + j];
    }
  }

  xoNV = REAL(oNV);
  for (int i = 0; i < vorout.numberofpoints; i++) {
    for (int j = 0; j < 2; j++) {
      xoNV[j * vorout.numberofpoints + i] = vorout.normlist[i * 2 + j];
    }
  }

  xoAV = REAL(oAV);
  for (int i = 0; i < vorout.numberofpoints; i++) {
    for (int j = 0; j < mid.numberofpointattributes; j++) {
      xoAV[j * vorout.numberofpoints + i] = vorout.pointattributelist[i * mid.numberofpointattributes + j];
    }
  }
  
  PROTECT(ans = allocVector(VECSXP, 13));
  SET_VECTOR_ELT(ans, 0, oP);
  SET_VECTOR_ELT(ans, 1, oPB);
  SET_VECTOR_ELT(ans, 2, oPA);
  SET_VECTOR_ELT(ans, 3, oT);
  SET_VECTOR_ELT(ans, 4, oS);
  SET_VECTOR_ELT(ans, 5, oSB);
  SET_VECTOR_ELT(ans, 6, oE);
  SET_VECTOR_ELT(ans, 7, oEB);
  SET_VECTOR_ELT(ans, 8, oPV);
  SET_VECTOR_ELT(ans, 9, oEV);
  SET_VECTOR_ELT(ans, 10, oNV);
  SET_VECTOR_ELT(ans, 11, oAV);
  SET_VECTOR_ELT(ans, 12, oN);
  UNPROTECT(15);

  /* Free all allocated arrays, including those allocated by Triangle. */
  R_Free(mid.pointlist);
  R_Free(mid.pointattributelist);
  R_Free(mid.pointmarkerlist);
  R_Free(mid.trianglelist);
  R_Free(mid.triangleattributelist);
  /* R_Free(mid.trianglearealist); */
  R_Free(mid.neighborlist);
  R_Free(mid.segmentlist);
  R_Free(mid.segmentmarkerlist);
  R_Free(mid.edgelist);
  R_Free(mid.edgemarkerlist);
  R_Free(vorout.pointlist);
  R_Free(vorout.pointattributelist);
  R_Free(vorout.edgelist);
  R_Free(vorout.normlist);
  /*  R_Free(out.pointlist);
  R_Free(out.pointattributelist);
  R_Free(out.trianglelist);
  R_Free(out.triangleattributelist); */

  return(ans);
} 
