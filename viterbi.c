#include <stdlib.h>
#include <math.h> //for fabs, log, exp
#include <string.h>
#include "utilities.h"
#include "common.h"
#include "assert.h"
         
#include "viterbi.h"

typedef struct _ViterbiNode
{ int                  state;
  struct _ViterbiNode *next;
} ViterbiPath;

typedef struct _ViterbiState
{ real          total;       // Probability of observation given model
  real          prob;        // Probability of most likely state sequence given model and observations
  ViterbiPath  *path;        // Most likely state sequence
} ViterbiState;

/********************************************************************************
 * ViterbiState Functions
 *
 */
void print_viterbi_state (ViterbiState *vs )
{ int i = 0;
  static int max = 10;
  ViterbiPath *node = vs->path;
  printf("%5.5f %5.5f : end",vs->total,vs->prob);
  while(node && (i++ < max))
  { printf(" <- %3d",node->state);
    node = node->next;
  }
  printf("\n");
}

void print_viterbi_state_vec( ViterbiState *vs, int n )
{ int i;
  for(i=0; i<n; i++)
  { printf("State %d: ",i);
    print_viterbi_state(vs+i);
  }
}

/********************************************************************************
 * ViterbiResult Functions
 *
 */
ViterbiResult *Make_Viterbi_Result(int n)
{ ViterbiResult *self = (ViterbiResult*) Guarded_Malloc( sizeof(ViterbiResult), "Make Viterbi Result" );
  self->total = 0;
  self->prob  = 0;
  self->sequence = (int*) Guarded_Malloc( sizeof(int) * n, "Make Viterbi Result" );
  self->n = n;
  return self;
}

void Free_Viterbi_Result( ViterbiResult *self)
{ if( self->sequence ) free(self->sequence);
  self->sequence = NULL;
  free(self);
}

ViterbiResult *Make_Viterbi_Result_From_State( ViterbiState *state )
{ int npath;
  ViterbiResult *out;

  // Compute size of path
  { ViterbiPath *node = state->path;
    npath = 0;
    while(node)
    { npath++;
      node = node->next;
    }
  }

  // Copy into result object
  out = Make_Viterbi_Result( npath );
  out->total = state->total;
  out->prob  = state->prob;
  { ViterbiPath *node = state->path;
    while(node)
    { out->sequence[ --npath ] = node->state;
      //printf("%3d : %3d\n",npath,node->state);
      node = node->next;
    }
  }

  return out;
}

/********************************************************************************
 * Basic forward viterbi
 *
 * This is really setup to be suited for a model with a dense state transition
 * matrix.
 *
 * Transition and emmission probabilities must be precomputed.
 */
ViterbiResult *Forward_Viterbi( int  *sequence,         // size: nobs
                                int   nseq,             // number of observations (size of seqeunce)
                                real *start_prob,       // size: nstates
                                real *transition_prob,  // size: nstates*nstates, destintion state-major order
                                real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs)
                                int nobs,               // the size of the set of observables ( need for stride )
                                int nstates )           // number of states (size of state alphabet)
{ static ViterbiState  *last = NULL;  
  static ViterbiState  *next = NULL;;
  static int maxnext = 0;
  static int maxlast = 0;
  static ViterbiPath  *pool = NULL;
  static int poolsize = 0;
  int npool = 0;
  int isrc,idst,iseq, iobs;

  assert(nseq>=1);

  // prealloc request for minimum required space
  pool = (ViterbiPath*) request_storage( pool, &poolsize, sizeof(ViterbiPath), nstates*(nseq+1), "Forward Vitirbi - init" );
  next = (ViterbiState*) request_storage( next, &maxnext, sizeof(ViterbiState), nstates, "Forward Vitirbi - init" );
  last = (ViterbiState*) request_storage( last, &maxlast, sizeof(ViterbiState), nstates, "Forward Vitirbi - init" );

  // Initialize
  idst = nstates;
  iobs = sequence[0];
  while(idst--)
  { ViterbiPath *this = pool + (npool++); 
    real   p = start_prob[idst] * emmission_prob[idst * nobs + iobs];
    this->state = idst;
    this->next  = NULL;
    last[idst] = (ViterbiState) { p, p, this };
  }

  // Deduction
  for( iseq=1; iseq<nseq; iseq++)
  { iobs = sequence[iseq];
    memset( next, 0, sizeof( ViterbiState ) * nstates );

    //print_viterbi_state_vec( last, nstates );

    for( idst = 0; idst < nstates; idst++ )
    { ViterbiState *argmax = next + idst;
      real valmax = 0.0;
      for( isrc = 0; isrc < nstates; isrc++ )
      { real   p = emmission_prob [idst * nobs    + iobs ]
                 * transition_prob[isrc * nstates + idst ];
        ViterbiState *src    = last + isrc;
        real          prob   = src->total,
                      vprob  = src->prob;
        ViterbiPath  *vpath  = src->path;

        //printf("Last : "); print_viterbi_state( last );
        //printf("Next : "); print_viterbi_state( next );
        //printf("\n");

        prob  *= p;
        vprob *= p;
        argmax->total += prob;
        if( vprob > valmax )        // Look for most likely source path to dst
        { ViterbiPath *this;
          valmax = vprob;
          pool = (ViterbiPath*) request_storage( pool,
                                                &poolsize, 
                                                 sizeof(ViterbiPath), 
                                                 npool+1, 
                                                 "Forward Viterbi - argmax" );
          this = pool + (npool++);  // Append dst to src->path 
          this->state   = idst;     //   src->path points to end of path (the append point)
          this->next    = vpath;
          argmax->path  = this;
          argmax->prob  = vprob;
        }
      }
    }
    // Step in time: switch next and last
    { ViterbiState *temp = last;
      last = next;
      next = temp;
    }
  } // end, for all observations

  //print_viterbi_state_vec( last, nstates );

  // Terminate: Return the best final state
  { ViterbiState *argmax = NULL;
    real valmax = 0.0;
    real total  = 0.0;
    isrc = nstates;
    while(isrc--)
    { ViterbiState *this = last + isrc;
      real p = this->prob;
      total += this->total;
      if( p > valmax )
      { argmax = this;
        valmax = p;
      }
    }
    argmax->total = total;
    return Make_Viterbi_Result_From_State( argmax ); // This copies out the predicted sequence
  }
}

#include <float.h> // for -DBL_MAX
#define LOG2_ADD(x,y) ( (x) + log2( 1.0 + pow(2.0, (y)-(x) ) ) ) // this works b.c. x and y are always same sign (negative)

ViterbiResult *Forward_Viterbi_Log2(   int  *sequence,         // size: nobs
                                       int   nseq,             // number of observations (size of seqeunce)
                                       real *start_prob,       // size: nstates
                                       real *transition_prob,  // size: nstates*nstates, destintion state-major order
                                       real *emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs)
                                       int nobs,               // the size of the set of observables ( need for stride )
                                       int nstates)            // number of states (size of state alphabet)
{ static ViterbiState  *last = NULL;  
  static ViterbiState  *next = NULL;;
  static int maxnext = 0;
  static int maxlast = 0;
  static ViterbiPath  *pool = NULL;
  static int poolsize = 0;
  int npool = 0;
  int isrc,idst,iseq, iobs;

  assert(nseq>=1);

  // prealloc request for minimum required space
  pool = (ViterbiPath*) request_storage( pool, &poolsize, sizeof(ViterbiPath), nstates*(nseq+1), "Forward Vitirbi LogP - init" );
  next = (ViterbiState*) request_storage( next, &maxnext, sizeof(ViterbiState), nstates, "Forward Vitirbi LogP - init" );
  last = (ViterbiState*) request_storage( last, &maxlast, sizeof(ViterbiState), nstates, "Forward Vitirbi LogP - init" );

  // Initialize
  idst = nstates;
  iobs = sequence[0];
  while(idst--)
  { ViterbiPath *this = pool + (npool++); 
    real   p = start_prob[idst] + emmission_prob[idst * nobs + iobs];
    this->state = idst;
    this->next  = NULL;
    last[idst] = (ViterbiState) { p, p, this };
  }

  // Deduction
  for( iseq=1; iseq<nseq; iseq++)
  { iobs = sequence[iseq];
    memset( next, 0, sizeof( ViterbiState ) * nstates );

    //print_viterbi_state_vec( last, nstates );

    for( idst = 0; idst < nstates; idst++ )
    { ViterbiState *argmax = next + idst;
      real valmax = -DBL_MAX;
      // Initialize valmax and argmax from first source
      { real   p = emmission_prob [idst * nobs    + iobs ]
                 + transition_prob[ idst ]; // isrc == 0
        ViterbiState *src    = last;        // isrc == 0 
        real          prob   = src->total,
                      vprob  = src->prob;
        ViterbiPath  *vpath  = src->path;

        //printf("Last : "); print_viterbi_state( last );
        //printf("Next : "); print_viterbi_state( next );
        //printf("\n");

        prob  += p;
        vprob += p;
        argmax->total = prob; 
        { ViterbiPath *this;
          valmax = vprob;
          pool = (ViterbiPath*) request_storage( pool,
                                                &poolsize, 
                                                 sizeof(ViterbiPath), 
                                                 npool+1, 
                                                 "Forward Viterbi - argmax" );
          this = pool + (npool++);  // Append dst to src->path 
          this->state   = idst;     //   src->path points to end of path (the append point)
          this->next    = vpath;
          argmax->path  = this;
          argmax->prob  = vprob;
        }
      }
      // Continue to check other sources
      for( isrc = 1; isrc < nstates; isrc++ )
      { real   p = emmission_prob [idst * nobs    + iobs ]
                 + transition_prob[isrc * nstates + idst ];
        ViterbiState *src    = last + isrc;
        real          prob   = src->total,
                      vprob  = src->prob;
        ViterbiPath  *vpath  = src->path;

        //printf("Last : "); print_viterbi_state( last );
        //printf("Next : "); print_viterbi_state( next );
        //printf("\n");

        prob  += p;
        vprob += p;
        argmax->total = LOG2_ADD( argmax->total, prob );
        //argmax->total += prob; // FIXME: need log(x+y)
        if( vprob > valmax )        // Look for most likely source path to dst
        { ViterbiPath *this;
          valmax = vprob;
          pool = (ViterbiPath*) request_storage( pool,
                                                &poolsize, 
                                                 sizeof(ViterbiPath), 
                                                 npool+1, 
                                                 "Forward Viterbi - argmax" );
          this = pool + (npool++);  // Append dst to src->path 
          this->state   = idst;     //   src->path points to end of path (the append point)
          this->next    = vpath;
          argmax->path  = this;
          argmax->prob  = vprob;
        }
      }
    }
    // Step in time: switch next and last
    { ViterbiState *temp = last;
      last = next;
      next = temp;
    }
  } // end, for all observations

  //print_viterbi_state_vec( last, nstates );

  // Terminate: Return the best final state
  { ViterbiState *argmax = NULL;
    real valmax;
    real total;
    isrc = nstates-1;
    //Init off of first example
    { ViterbiState *this = last + isrc;
      total = this->total;
      valmax = this->prob;
      argmax = this;
    }
    while(isrc--) //...and continue
    { ViterbiState *this = last + isrc;
      real p = this->prob;
      total = LOG2_ADD( total, this->total );
      //total += this->total;  //FIXME: need log(x+y)
      if( p > valmax )
      { argmax = this;
        valmax = p;
      }
    }
    argmax->total = total;
    return Make_Viterbi_Result_From_State( argmax ); // This copies out the predicted sequence
  }
}

#ifdef TEST_VITERBI
#ifndef TEST_VITERBI_PROBLEM_1 || TEST_VITERBI_PROBLEM_2
#define TEST_VITERBI_PROBLEM_2
#endif


#ifdef TEST_VITERBI_PROBLEM_1
/*** Problem 1 ***********************************************************/
/* Source: http://en.wikipedia.org/wiki/Viterbi_algorithm                */
/* At the time of this writing the results posted on that page are wrong */
/*                                                                       */
char *states[] = 
{ "rainy",
  "sunny",
  NULL
};

#define RAINY  0
#define SUNNY  1
#define NSTATE 2

char *observables[] = 
{ "walk",
  "shop",
  "clean",
  NULL
};

#define WALK  0
#define SHOP  1
#define CLEAN 2
#define NOBS  3

double sprob[] = { 0.6, 0.4};
           //dst Rainy Sunny
double tprob[] = { 0.7, 0.3,       // src  Rainy
                   0.4, 0.6 };     // src  Sunny
double eprob[] = { 0.1, 0.4, 0.5,
                   0.6, 0.3, 0.1 };
#define NSEQ 3
int sequence[NSEQ] = { WALK, SHOP, CLEAN };
double expected = 0.01344;
/*** Problem 1  End  *****************************************************/
#endif  // end TEST_VITERBI_PROBLEM_1

#ifdef TEST_VITERBI_PROBLEM_2
/*** Problem 2 ***********************************************************/
/* Source:
 * http://www.comp.leeds.ac.uk/roger/HiddenMarkovModels/html_dev/viterbi_algorithm/s3_pg4.html
 */
char *states[] = 
{ "sunny",
  "cloudy",
  "rainy",
  NULL
};

#define SUNNY  0
#define CLOUDY 1
#define RAINY  2
#define NSTATE 3

char *observables[] = 
{ "dry",
  "dryish",
  "damp",
  "soggy",
  NULL
};

#define DRY    0
#define DRYISH 1
#define DAMP   2
#define SOGGY  3
#define NOBS   4

double sprob[] = { 0.63, 0.17, 0.20};
double tprob[] = { 0.5  , 0.25 , 0.25 , // src = rows, dest = cols
                   0.375, 0.125, 0.375,
                   0.125, 0.675, 0.375 };
double eprob[] = { 0.60, 0.20, 0.15, 0.05, // state = rows, obs = cols
                   0.25, 0.25, 0.25, 0.25,
                   0.05, 0.10, 0.35, 0.50 };
// #define NSEQ 3
// int sequence[NSEQ] = { DRY, DAMP, SOGGY }; // should give sunny -> rainy -> rainy with vprob 0.0062015625 
// double expected = 0.006201562;
#define NSEQ 6
int sequence[NSEQ] = { DRY, DAMP, SOGGY, DRY, DAMP, SOGGY }; // should give sunny -> rainy -> rainy -> cloudy -> rainy -> rainy with vprob 2.5754047e-5
double expected = 2.5754047e-5;
/*** Problem 2  End  *****************************************************/
#endif  // end TEST_VITERBI_PROBLEM_2

/** Test suite ****/
void print_observations( int *sequence, int nseq )
{ int i = 0;
  printf("Observed sequence:\n");
  for( ; i < nseq; i++ )
    printf("\t%3d. %s\n",i, observables[ sequence[i] ] );
}

void print_result( ViterbiResult *res )
{ int i=0;
  printf("Viterbi Result:\n");
  printf("\tTotal: %g\n",res->total);
  printf("\t Prob: %g\n",res->prob);
  for( ; i < res->n; i++ )
    printf("\t%3d. %s\n",i, states[ res->sequence[i] ] );
}

int main( int argc, char* argv[] ) 
{ int retval = 1;
  double tol = 1e-3;
  printf("\nTest: Viterbi \n");
  print_observations( sequence, NSEQ );
  ViterbiResult *res = Forward_Viterbi( sequence, NSEQ,
                                        sprob,  // size: nstates
                                        tprob,  // size: nstates*nstates, destintion state-major order
                                        eprob,  // size: nstates * nobs, state-major order (stride is nobs)
                                        NOBS,   // size of set of observables
                                        NSTATE);// number of states (size of state alphabet)
  print_result( res );
  if( fabs(res->prob - expected) > tol )
  { printf("FAILED: Expected probability of viterbi path to be %g (err: %g)\n",expected,res->prob - expected);
    retval = -1;
  } else
  { printf("PASSED\n");
    retval = 0;
  }
  Free_Viterbi_Result( res );

  printf("\nTest: Viterbi LogP\n");
  { int j,i=0;
    for(;i<NSTATE;i++)
    { sprob[i] = log2(sprob[i]);
      for(j=0;j<NSTATE;j++)
        tprob[i*NSTATE+j] = log2( tprob[i*NSTATE+j] );
      for(j=0;j<NOBS;j++)
        eprob[i*NOBS + j] = log2( eprob[i*NOBS + j] );
    }
  }
  res = Forward_Viterbi_Log2( sequence, NSEQ,
                                        sprob,  // size: nstates
                                        tprob,  // size: nstates*nstates, destintion state-major order
                                        eprob,  // size: nstates * nobs, state-major order (stride is nobs)
                                        NOBS,   // size of set of observables
                                        NSTATE);// number of states (size of state alphabet)
  res->prob  = pow(2.0,res->prob);
  res->total = pow(2.0,res->total);
  print_result( res );
  if( fabs(res->prob - expected) > tol )
  { printf("FAILED: Expected probability of viterbi path to be %g (err: %g)\n",expected,res->prob - expected);
    retval = -1;
  } else
  { printf("PASSED\n");
    retval = 0;
  }
  Free_Viterbi_Result( res );

  printf("\nTest: log2 add\n");
  { double x = 0.1, 
           y = 0.1;
    printf("\tlog2(%3.3g + %3.3g) = %f\n", x  ,   y, LOG2_ADD(log2( x ),log2( y )) );
    printf("\texpected          %f\n", log2(x+y));
  }
  printf("\n");
  { double x = 0.000000000111, 
           y = 0.00000211;
    printf("\tlog2(%3.3g + %3.3g) = %f\n", x  ,   y, LOG2_ADD(log2( x ),log2( y )) );
    printf("\texpected                    %f\n", log2(x+y));
  }

  return retval;
}
#endif //TEST_VITERBI
