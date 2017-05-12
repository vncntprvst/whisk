/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_POLY
// TODO: It would be much nicer to structure this for 
//       stack based operations.  e.g
//
//       poly_push(p,degree);
//       poly_der(2)
//       poly_push(q,degree2);
//       poly_mul();
//       degree3 = poly_pop(r);
//
#include "compat.h"

 double polyval             ( double *p, int degree, double x);
 int    polymul_nelem_dest  ( int na, int nb);
 void   polymul             ( double *a, int na, double *b, int nb, double *dest );
 void   polyadd_ip_left     ( double *a, int na, double *b, int nb );
 void   polysub_ip_left     ( double *a, int na, double *b, int nb );
 void   polyadd             ( double *a, int na, double *b, int nb, double *dest );
 void   polysub             ( double *a, int na, double *b, int nb, double *dest );
 void   polyder_ip          ( double *a, int na, int times);

void    Vandermonde_Build             ( double *x, int n, int degree, double *V );
void    Vandermonde_Inverse           ( double *x, int n, double *invV);
double  Vandermonde_Determinant       ( double *x, int n );
double  Vandermonde_Determinant_Log2  ( double *x, int n );
 int polyfit_size_workspace     ( int n, int degree );
double *polyfit_alloc_workspace       ( int n, int degree );
void    polyfit_realloc_workspace     ( int n, int degree, double **workspace );
void    polyfit_free_workspace        ( double *workspace );
void    polyfit_reuse                 ( double *y, int n, int degree, double *coeefs, double *workspace );
void    polyfit                       ( double *x, double *y, int n, int degree, double *coeffs, double *workspace );
#endif
