/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef  _H_COMPAT_
#define _H_COMPAT_

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include <stdint.h>

// For Microsoft Visual Studio
#ifdef _MSC_VER

#ifndef log2                          // no log2 in xp
#define log2(x) (log(x)/log(2))       // there are fast tricks for finding int(log2(x))
#endif                             

#define msvcextern extern

#define hypotf _hypotf

// long int lround(float x);
// double round(double x);
// float roundf(float x);

//#define isnan _isnan

#else  // NOT MSVC

#define msvcextern 
#ifndef __declspec
#define __declspec(x) 
#endif
#endif

#ifndef INFINITY
  #define INFINITY FLT_MAX
#endif

#ifndef SHARED_EXPORT
#define SHARED_EXPORT __declspec(dllexport)
#endif

#endif //#define _H_COMPAT_
