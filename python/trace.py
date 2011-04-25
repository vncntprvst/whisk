""" trace.py

ctypes interface to libwhisk.so
The functionality described here corresponds with that in trace.h.

File I/O
  Load_Whiskers
  Save_Whiskers

Tracing
  Trace_Whisker

Author: Nathan Clack <clackn@janelia.hhmi.org>
Date  : 25 January 2009

Copyright 2010 Howard Hughes Medical Institute.
All rights reserved.
Use is subject to Janelia Farm Research Campus Software Copyright 1.1
license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
"""
import sys,os
from ctypes import *
from ctypes.util import find_library
from numpy import zeros, float32, uint8, array, hypot, arctan2, pi, concatenate, float64, ndarray, int32
from numpy import where, cos, sin, sum
from warnings import warn


import pdb

dllpath = os.path.split(os.path.abspath(__file__))[0]
if sys.platform == 'win32':
  lib = os.path.join(dllpath,'whisk.dll')
else:
  lib = os.path.join(dllpath,'libwhisk.so')
os.environ['PATH']+=os.pathsep + os.pathsep.join(['.','..',dllpath])
name = find_library('whisk')
if not name:
  name=lib
cWhisk = CDLL(name)

_param_file = "default.parameters"
if cWhisk.Load_Params_File(_param_file)==1: #returns 0 on success, 1 on failure
  cWhisk.Print_Params_File(_param_file)
  if cWhisk.Load_Params_File(_param_file)==1: #returns 0 on success, 1 on failure
    raise Exception("Could not load tracing parameters from file: %s"%_param_file)

#
# DATA STRUCTURE TRANSLATIONS
#

class cContour(Structure):                       #typedef struct    
  _fields_ = [( "length"   , c_int            ), #  { int  length;  
              ( "boundary" , c_int            ), #    int  boundary;
              ( "width"    , c_int            ), #    int  width;   
              ( "iscon4"   , c_int            ), #    int  iscon4;  
              ( "tour"     , POINTER( c_int ) )] #    int *tour;    
                                                 #  } Contour;      

  def asarray(self):
    a = zeros( (self.length,2) )
    for i in range(self.length):
      a[i,0] = self.tour[i]%self.width
      a[i,1] = self.tour[i]/self.width
    return a

  def plot(self,*args,**kwargs):
    from pylab import plot
    a = self.asarray()
    plot(a[:,0],a[:,1],*args,**kwargs)

  def draw(self, surface, color, scale, drawfunc):
    a = self.asarray()
    drawfunc( surface, color, 1, a*scale )

class cObject_Map(Structure):
  _fields_ = [( "num_objects",  c_int ),
              ( "objects",      POINTER(POINTER( cContour ))) ]

  def plot(self,*args,**kwargs):
    for i in xrange( self.num_objects ):
      self.objects[i].contents.plot(*args,**kwargs)

  def plot_with_seeds( self, image, *args, **kwargs ):
    from pylab import imshow, cm, axis, subplots_adjust, show
    imshow( image, cmap = cm.gray, hold = 0, interpolation = 'nearest' )
    self.plot( *args, **kwargs )
    for i in xrange(self.num_objects):
      sds = find_seeds( self.objects[i], image )
      if sds:
        sds.plot( linewidths = (1,), 
                  facecolors = ('w',), 
                  edgecolors = ('k',) )
    axis('image')
    axis('off')
    subplots_adjust(0,0,1,1,0,0)
    show()
    return gcf()

  def draw(self, surface, color, scale, drawfunc ):
    for i in xrange( self.num_objects ):
      self.objects[i].contents.draw(surface,color,scale,drawfunc)

class cWhisker_Seg_Old(Structure):                 #typedef struct      
  _fields_ = [( "id"       , c_int   ),            #  { int    id;      
              ( "width"    , c_double),            #    double width;   
              ( "beg"      , c_int   ),            #    int    beg;     
              ( "end"      , c_int   ),            #    int    end;     
              ( "time"     , c_int   ),            #    int    time;    
              ( "track"    , POINTER( c_float ) ), #    float *track;   
              ( "scores"   , POINTER( c_float ) )] #    float *scores;  
                                                  #  } Whisker_Seg_Old;

class cWhisker_Seg(Structure):                      #typedef struct  
  _fields_ = [( "id"       , c_int   ),            #{ int id;       
              ( "time"     , c_int   ),            #  int time;     
              ( "len"      , c_int   ),            #  int len;      
              ( "x"        , POINTER( c_float ) ), #  float *x;     
              ( "y"        , POINTER( c_float ) ), #  float *y;     
              ( "thick"    , POINTER( c_float ) ), #  float *thick; 
              ( "scores"   , POINTER( c_float ) )] #  float *scores;
                                                   #} Whisker_Seg;  
  @staticmethod
  def CastFromWhiskerSeg( w ):
    return cWhisker_Seg( w.id,
                         w.time,
                         len(w.x),
                         w.x.ctypes.data_as( POINTER( c_float ) ),
                         w.y.ctypes.data_as( POINTER( c_float ) ),     
                         w.thick.ctypes.data_as( POINTER( c_float ) ), 
                         w.scores.ctypes.data_as( POINTER( c_float ) ) )
  @staticmethod
  def CastDictToArray( wvd ):
    """
    This function creates a ctypes array of cWhisker_Seg from an input
    whiskers dictionary.  The resulting array can be passed to functions
    in the C library.  Don't use Free_Whisker_Seg_Vec on this array; it 
    doesn't "own" its data.  Instead, it passes most of the data as pointers
    to numpy arrays.
    
    Arguments:
    ---------
    <wvd> is a dictionary.  The keys are "frame ids" (integers indicating
          which time point in the movie).  The value's are another set of
          dictionaries.  The keys of these dictionaries are "whisker ids,"
          and the values are of <class `Whisker_Seg`>.

    Example:
    -------
    >>> import trace
    >>> wvd = trace.Load_Whiskers(r"proc\whisker_data_0140.whiskers")
    >>> wv = trace.cWhisker_Seg.CastDictToArray(wvd)
    >>> len(wv)
    40408
    >>> wv[0]
    <trace.cWhisker_Seg object at 0x03DACC60>
    """
    #first count the number of segments
    nseg = sum( map(len, wvd.values()) )
    
    #create the constructor
    type_wv = cWhisker_Seg * nseg;
    
    #alloc and fill the array
    def itersegs(wvd):
      for v in wvd.itervalues():
        for w in v.itervalues():
          yield w
    wv = type_wv( *map(cWhisker_Seg.CastFromWhiskerSeg,list(itersegs(wvd))) )
    
    return wv

class cWhiskerIndex(Structure):
  _fields_ = [( 'index' ,   POINTER(POINTER( cWhisker_Seg )) ),
              ( 'sz'    ,   POINTER( c_int  ) ),
              ( 'n'     ,   c_int           ) ]

class cSeed(Structure):
  _fields_ = [( 'xpnt',   c_int ),
              ( 'ypnt',   c_int ),
              ( 'xdir',   c_int ),
              ( 'ydir',   c_int )]

  def asarray(self):
    return array([self.xpnt,self.ypnt,self.xdir,self.ydir])

  def __repr__(self):
    return "<%s.%s (%d,%d) slope: %5.3g>"%(
                str(self.__module__),
                "cSeed",
                self.xpnt,
                self.ypnt,
                arctan2(self.ydir, self.xdir)*180/pi)

class cSeedVector(Structure):                  # typedef struct     
  _fields_ = [('nseeds', c_int            ),   #   { int   nseeds;  
              ('seeds' , POINTER( cSeed ) )]   #     Seed *seeds;   
                                               #   } Seed_Vector;   
  def asarray(self):
    a = zeros(( self.nseeds, 4))
    for i in xrange( self.nseeds ):
      a[i] = self.seeds[i].asarray()
    return a

  def plot(self, *args, **kwargs):
    from pylab import quiver
    a = self.asarray()
    if not 'pivot' in kwargs:
      kwargs['pivot'] = 'middle' 
    if not 'scale' in kwargs:
      kwargs['scale'] = 10
    if a.any():
      norm = hypot( a[:,2], a[:,3] )
      a[:,2] /= norm
      a[:,3] /= norm
      quiver( a[:,0], a[:,1], a[:,2], -a[:,3], **kwargs )

class cImage(Structure):
  _fields_ = [( "kind",     c_int   ),
              ( "width",    c_int   ),
              ( "height",   c_int   ),
              ( "text",     POINTER(c_char_p) ),
              ( "array",    POINTER(c_uint8))]
  @staticmethod
  def fromarray(im):
    return cImage(  c_int(  im.dtype.itemsize ),
                    c_int(  im.shape[1] ),                         
                    c_int(  im.shape[0] ),      
                    pointer( c_char_p("") ),
                    im.ctypes.data_as( POINTER( c_uint8 ) ) )

class Whisker_Seg(object):
  def __init__(self, source=None):
    """ 
    If source is:
      None        : initialize an empty segment
      cWhisker_Seg: copy contents of cWhisker_Seg into a Whisker_Seg 
      tuple       : must be (Whisker_Seg, start_index, stop_index)
                    Create a Whisker_Seg referencng the indicated subsegment
      
    """
    if source is None:
      self.id   = None
      self.time = None
      self.x    = array([])
      self.y    = array([])
      self.scores = array([])
      self.thick  = array([])
    elif isinstance( source, cWhisker_Seg ):
      self.id = source.id
      self.time = source.time

      self.x      = zeros( source.len, dtype=float32 )
      self.y      = zeros( source.len, dtype=float32)
      self.scores = zeros( source.len, dtype=float32 )
      self.thick  = zeros( source.len, dtype=float32 )

      for i in xrange( source.len ):
        self.x[i]      = source.x[i]
        self.y[i]      = source.y[i]
        self.thick[i]  = source.thick[i]
        self.scores[i] = source.scores[i]
    elif isinstance( source, tuple ):
      arg,a,b = source
      self.id   = arg.id
      self.time = arg.time
      self.x      = arg.x[a:b]
      self.y      = arg.y[a:b]
      self.thick  = arg.thick[a:b]
      self.scores = arg.scores[a:b]

  def __len__(self):
    return len(self.x)

  def __getitem__(self, idx ):
    return [self.x[idx],self.y[idx],self.thick[idx],self.scores[idx]]

  def asarray( self ):
    return concatenate((self.x,self.y,self.thick,self.scores)).reshape(4,len(self.x)).T

  def split( self, idx ):
    a,b = Whisker_Seg(), Whisker_Seg()
    a.id   = b.id   = self.id
    a.time = b.time = self.time
    a.x      = self.x     [:idx]
    a.y      = self.y     [:idx]
    a.thick  = self.thick [:idx]
    a.scores = self.scores[:idx]
    b.x      = self.x     [idx:]
    b.y      = self.y     [idx:]
    b.thick  = self.thick [idx:]
    b.scores = self.scores[idx:]
    if len(a) == 0: 
      a = None
    if len(b) == 0: 
      b = None
    return a,b

  def join_right( self, w ):
    self.x      = concatenate( (self.x     , w.x     ) )
    self.y      = concatenate( (self.y     , w.y     ) )
    self.thick  = concatenate( (self.thick , w.thick ) )
    self.scores = concatenate( (self.scores, w.scores) )
    return self

  @staticmethod
  def join( left, right ):
    """ Like `join_right` but the joined result is a new whisker. """
    w = left.copy()
    w.join_right( right )
    return w

  def copy(self):
    w = Whisker_Seg()
    w.id = self.id
    w.time = self.time

    w.x      = self.x.copy()
    w.y      = self.y.copy()
    w.scores = self.scores.copy()
    w.thick  = self.thick.copy()
    return w

#
# FILE I/O
#

cWhisk.Load_Whiskers.restype = POINTER( cWhisker_Seg )
cWhisk.Load_Whiskers.argtypes = [
  POINTER( c_char ),
  POINTER( c_char ),
  POINTER( c_int)]  

cWhisk.Save_Whiskers.restype = c_int
cWhisk.Save_Whiskers.argtypes = [
  POINTER( c_char ),
  POINTER( c_char ),
  POINTER( cWhisker_Seg ),
  c_int]

def Load_Whiskers( filename ):
  """ Reads whisker segments from a file.

  Returns a dict of dict with Whisker_Seg elements.  The organization looks
  like:

  >>> from trace import Load_Whiskers
  >>> frameid = 0;
  >>> segmentid = 1;
  >>> wv = Load_Whiskers('test.whiskers')
  >>> w = wv[frameid][segmentid]
  >>> w.id == segmentid
  True
  >>> w.time == frameid
  True

  The file is read using a ctypes call to a libwhisk(trace.h) function.  The
  data is then copied into numpy containers and freed.

  It's not clear to me how this effects the part of the heap hidden from
  python's memmory management.  This is done for convenience, since later no
  one needs to remember to free anything and one gets to use numpy.array's for
  plotting, etc...

  For the kinds of things I imaging doing in python this function will only get
  called once per application instance.
  """
  if not os.path.exists(filename):
    raise IOError, "File not found."
  nwhiskers = c_int(0)
  wv = cWhisk.Load_Whiskers( filename, None, byref(nwhiskers) );
  # organize into dictionary for ui.py {frameid}{segid}
  whiskers = {}
  for idx in xrange( nwhiskers.value ):
    w = wv[idx]
    whiskers[ w.time ] = {}
  for idx in xrange( nwhiskers.value ):
    w = Whisker_Seg(wv[idx])
    whiskers[ w.time ][ w.id ] = w;
  cWhisk.Free_Whisker_Seg_Vec( wv, nwhiskers )
  return whiskers

def Save_Whiskers( filename, whiskers ):
  #count the whiskers
  n = 0
  for v in whiskers.itervalues():
    n += len(v)
  #alloc the c whisker array
  wv = (cWhisker_Seg * n)() 
  #copy into c whisker array
  i = 0
  for fid,v in whiskers.iteritems():
    for wid,t in v.iteritems():
      if not t:
        continue;
      wv[i].id   = wid
      wv[i].time = fid
      wv[i].len  = len(t.x)
      wv[i].x       = t.x.ctypes.data_as( POINTER( c_float ) )
      wv[i].y       = t.y.ctypes.data_as( POINTER( c_float ) )
      wv[i].thick   = t.thick.ctypes.data_as( POINTER( c_float ) )
      wv[i].scores  = t.scores.ctypes.data_as( POINTER( c_float ) )
      i += 1
  #prep for save and save
  #print "Saving %d"%i
  #pdb.set_trace()
  if not cWhisk.Save_Whiskers( filename, None, wv, i ):
    warn("Save Whiskers may have failed.")

#
# WHISKER DISTANCE
#
# cWhisk.whisker_distance.restype = c_float
# cWhisk.whisker_distance.argtypes = [
#   POINTER( cWhisker_Seg ),
#   POINTER( cWhisker_Seg ) ]
#
# cWhisk.whisker_distance_time_independent.restype = c_float
# cWhisk.whisker_distance_time_independent.argtypes = [
#   POINTER( cWhisker_Seg ),
#   POINTER( cWhisker_Seg ) ]
#
# def whisker_distance( a, b ):
#   ca = cWhisker_Seg.CastFromWhiskerSeg( a )
#   cb = cWhisker_Seg.CastFromWhiskerSeg( b )
#   d  = cWhisk.whisker_distance_time_independent( byref(ca), byref(cb) )
#   return d

#
# Bipartite Match
#

try:
  cWhisk.assignmentoptimal.restype = None
  cWhisk.assignmentoptimal.argtypes = [
    POINTER( c_double ),                  # assignment buffer ( sizeof(double)*nOfRows )
    POINTER( c_double ),                  # cost: output by reference
    POINTER( c_double ),                  # distance matrix ( sizeof(double)*nOfRows*nOfCols )
    c_int,                                # nOfRows
    c_int ]                               # nOfCols

  def bipartite_matching( costs ):
    """ Returns mapping of rows to columns and the cost of the matching.
    
    assignment[i] = j ==> row i <--> column j

    """
    from numpy import double
    assignment = zeros( (costs.shape[0],), dtype = double )
    cost = c_double()
    cWhisk.assignmentoptimal( assignment.ctypes.data_as( POINTER(c_double) ),
                              byref(cost),
                              costs.T.ctypes.data_as( POINTER(c_double) ),
                              costs.shape[1],
                              costs.shape[0] )
    print assignment
    map = {}
    for i,j in enumerate(assignment):
      if j != -1:
        map[i]=int(j)
    return map,cost

  def test_bipartite_matching():
    from pylab import plot, rand
    from numpy import zeros, arange
    N = 10
    a = rand(N,2)
    a[:,0] = arange(N)
    a[:,1] = 0
    b = rand(N+2,2)
    b[:,0] = arange(N+2)
    b[:,1] = 1
    d = zeros((a.shape[0],b.shape[0]))
    for i,ai in enumerate(a):
      for j,bi in enumerate(b):
        d[i,j] = ((ai-bi)**2).sum()
    assignment,cost = bipartite_matching( d )
    print "Matching cost: ", cost.value
    plot(a[:,0],a[:,1],'o')
    plot(b[:,0],b[:,1],'s')
    for i,j in assignment.iteritems():
      plot([ a[i,0], b[j,0] ], [ a[i,1], b[j,1] ],'k--')
    return assignment

except AttributeError: #assignmentoptimal not found - usually because match.c wasn't linked in
  pass                 #This isn't typically a big deal because I don't use it anywhere


#
# SEEDING/TRACING
#

cWhisk.get_response_extents.restype = None
cWhisk.get_response_extents.argtypes = [
  POINTER( c_int ),
  POINTER( c_int ),
  POINTER( c_int ) ]

cWhisk.get_response.restype = None
cWhisk.get_response.argtypes = [
  POINTER( c_float ),
  POINTER( cImage ),
  c_int ]

cWhisk.get_response_axes_ticks.restype = None
cWhisk.get_response_axes_ticks.argtypes = [
  POINTER( c_float ),
  POINTER( c_float ),
  POINTER( c_float ) ]

cWhisk.find_seeds.restype = POINTER( cSeedVector )
cWhisk.find_seeds.argtypes = [
  POINTER( cContour ) ,
  POINTER( cImage   ) ]

cWhisk.find_seeds2.restype = POINTER( cSeedVector )
cWhisk.find_seeds2.argtypes = [
  POINTER( cContour ) ,
  POINTER( cImage   ) ]

cWhisk.get_objectmap.restype  = POINTER( cObject_Map )
cWhisk.get_objectmap.argtypes = [ POINTER( cImage ) ] 

cWhisk.compute_seed_from_point.restype = POINTER( cSeed )
cWhisk.compute_seed_from_point.argtypes = [
  POINTER( cImage ),
  c_int,
  c_int ]

cWhisk.compute_seed_from_point_ex.restype = POINTER( cSeed )
cWhisk.compute_seed_from_point_ex.argtypes = [
  POINTER( cImage ),
  c_int,
  c_int,
  POINTER( c_float ),
  POINTER( c_float )]

cWhisk.compute_seed_from_point_histogram.restype = None
cWhisk.compute_seed_from_point_histogram.argtypes = [
  POINTER( cImage ),
  c_int,
  POINTER( cImage )]

cWhisk.compute_seed_from_point_field.restype = None
cWhisk.compute_seed_from_point_field.argtypes = [
  POINTER( cImage ),
  c_int,
  POINTER( cImage ),
  POINTER( cImage ),
  POINTER( cImage )]

cWhisk.compute_seed_from_point_field_windowed.restype = None
cWhisk.compute_seed_from_point_field_windowed.argtypes = [
  POINTER( cImage ),    # image
  c_int,                # maxr
  c_int,                # maxiter
  c_float,              # stat window: low
  c_float,              # stat window: high
  POINTER( cImage ),    # output - histogram
  POINTER( cImage ),    # output - slopes
  POINTER( cImage )]    # output - stats

cWhisk.compute_seed_from_point_field_windowed_on_contour.restype = None
cWhisk.compute_seed_from_point_field_windowed_on_contour.argtypes = [
  POINTER( cImage ),    # image
  POINTER( cContour ),  # trace
  c_int,                # maxr
  c_int,                # maxiter
  c_float,              # stat window: low
  c_float,              # stat window: high
  POINTER( cImage ),    # output - histogram
  POINTER( cImage ),    # output - slopes
  POINTER( cImage )]    # output - stats

cWhisk.compute_seed_from_point_field_on_grid.restype = None
cWhisk.compute_seed_from_point_field_on_grid.argtypes = [
  POINTER( cImage ),    # image
  c_int,                # lattice spacing for grid
  c_int,                # maxr
  c_int,                # maxiter
  c_float,              # stat window: low
  c_float,              # stat window: high
  POINTER( cImage ),    # output - histogram
  POINTER( cImage ),    # output - slopes
  POINTER( cImage )]    # output - stats

def get_response( image, (x,y) ):
  p = c_int( int(x) + int(y)*image.shape[1] )
  nx,ny,nz = c_int(),c_int(),c_int()
  #x is the axis that changes fastest with index, z is the slowest
  #should be x: width, y: angle, z: offset
  cWhisk.get_response_extents( byref(nz), byref(ny), byref(nx) )

  stk = zeros((nz.value,ny.value,nx.value),dtype = float32)
  cim = cImage.fromarray( image )
  cWhisk.get_response( stk.ctypes.data_as( POINTER( c_float ) ), byref(cim), p )

  xticks = zeros(nx.value, dtype=float32)
  yticks = zeros(ny.value, dtype=float32)
  zticks = zeros(nz.value, dtype=float32)
  f = lambda x: x.ctypes.data_as( POINTER( c_float ) )
  cWhisk.get_response_axes_ticks( f(xticks), f(yticks), f(zticks) )
  return stk,(xticks,yticks,zticks)


# FIXME: Memory leak.  Need to free seed vector when done  
def find_seeds( trace, image ):
  """ `trace` : POINTER( cContour )
          A contour returned from `compute object map`
      `image` : 2d numpy.array
          The image of interest
  """
  cim = cImage.fromarray( image )
  res = cWhisk.find_seeds( trace, byref( cim ) ).contents
  return res

# FIXME: Memory leak.  Need to free seed vector when done  
def find_seeds2( trace, image ):
  """ `trace` : POINTER( cContour )
          A contour returned from `compute object map`
      `image` : 2d numpy.array
          The image of interest
  """
  cim = cImage.fromarray( image )
  res = cWhisk.find_seeds2( trace, byref( cim ) )
  if res:
    return res.contents
  return None

# FIXME: Memory leak.  Need to free object map when done   
def compute_object_map( image ):
  cim = cImage.fromarray( image )
  return cWhisk.get_objectmap( byref(cim) ).contents

def compute_seed( image, (x,y), maxr = 4 ):
  cim = cImage.fromarray( image )
  p = c_int( int(x) + int(y)*image.shape[1] )
  pcseed = cWhisk.compute_seed_from_point( cim, p, c_int(maxr) ); 
  if pcseed:
    #print repr(pcseed.contents)
    return pcseed.contents
  return None

def compute_seed_ex( image, (x,y), maxr = 4 ):
  m = c_float()
  stat = c_float()
  cim = cImage.fromarray( image )
  p = c_int( int(x) + int(y)*image.shape[1] )
  pcseed = cWhisk.compute_seed_from_point_ex( cim, p, c_int(maxr), byref(m), byref(stat)); 
  return pcseed.contents, m.value, stat.value

def compute_seed_hist( image, maxr = 4 ):
  cim = cImage.fromarray( image )
  hist = zeros( image.shape, dtype = uint8 )
  chist   = cImage.fromarray(hist)
  cWhisk.compute_seed_from_point_histogram( cim, c_int(maxr), chist )
  return hist

def compute_seed_fields( image, maxr = 4 ):
  cim = cImage.fromarray( image )
  hist = zeros( image.shape, dtype = uint8 )
  slopes = zeros( image.shape, dtype = float32 )
  stats  = zeros( image.shape, dtype = float32 )
  chist   = cImage.fromarray(hist)
  cslopes = cImage.fromarray(slopes)
  cstats  = cImage.fromarray(stats)
  cWhisk.compute_seed_from_point_field( cim, c_int(maxr), chist, cslopes, cstats )
  return hist, slopes, stats

def compute_seed_fields_windowed( image,  maxr = 4, maxiter = 4, window = (0.4,0.4) ):
  cim     = cImage.fromarray( image )
  hist    = zeros( image.shape, dtype = uint8 )
  slopes  = zeros( image.shape, dtype = float32 )
  stats   = zeros( image.shape, dtype = float32 )
  chist   = cImage.fromarray(hist)
  cslopes = cImage.fromarray(slopes)
  cstats  = cImage.fromarray(stats)

  cWhisk.compute_seed_from_point_field_windowed( cim,
                                                 c_int(maxr),
                                                 c_int(maxiter),
                                                 window[0], window[1], 
                                                 chist, cslopes, cstats )

  mask = hist>0
  stats[mask]  = stats [mask]/hist[mask]
  slopes[mask] = slopes[mask]/hist[mask]
  return hist, slopes, stats

def compute_seed_fields_windowed_on_objects( image,  maxr = 4, maxiter=1, window = (0.0,0.0) ):
  cim = cImage.fromarray( image )
  hist = zeros( image.shape, dtype = uint8 )
  slopes = zeros( image.shape, dtype = float32 )
  stats  = zeros( image.shape, dtype = float32 )
  chist   = cImage.fromarray(hist)
  cslopes = cImage.fromarray(slopes)
  cstats  = cImage.fromarray(stats)

  objs = cWhisk.get_objectmap( byref(cim) ).contents
  for i in xrange( objs.num_objects ):
    ptrace = objs.objects[i]
    cWhisk.compute_seed_from_point_field_windowed_on_contour( cim, ptrace, 
                                                              c_int(maxr), c_int(maxiter),
                                                              window[0], window[1], 
                                                              chist, cslopes, cstats )
  mask = hist>0
  stats[mask]  = stats [mask]/hist[mask]
  slopes[mask] = slopes[mask]/hist[mask]
  return hist, slopes, stats

def compute_seed_from_point_field_on_grid(image, spacing=8, maxr=4, maxiter=1, window=(0.0,0.0) ):
  cim = cImage.fromarray( image )
  hist = zeros( image.shape, dtype = uint8 )
  slopes = zeros( image.shape, dtype = float32 )
  stats  = zeros( image.shape, dtype = float32 )
  chist   = cImage.fromarray(hist)
  cslopes = cImage.fromarray(slopes)
  cstats  = cImage.fromarray(stats)
  cWhisk.compute_seed_from_point_field_on_grid( cim, c_int(spacing), c_int(maxr), c_int(maxiter), window[0], window[1],
                                                chist, cslopes, cstats )
  mask = hist>0
  stats[mask]  = stats [mask]/hist[mask]
  slopes[mask] = slopes[mask]/hist[mask]
  return hist, slopes, stats

def plot_seeds_from_seeding_fields( image, fields = compute_seed_fields_windowed_on_objects, color='w' ):
  from pylab import quiver
  h,m,s = fields( image )
  ii,jj = where( s )
  quiver( jj,ii,         cos(m[ii,jj]),     -1.0*sin(m[ii,jj]),
          facecolors = (color,),
          edgecolors = ('k',),
          linewidths = (0.5,),
          pivot = 'middle',
          units = 'x',
          width = 1,
          alpha = 0.7
          )



cWhisk.trace_whisker.restype = POINTER( cWhisker_Seg )
cWhisk.trace_whisker.argtypes = [ 
  POINTER( cSeed ),
  POINTER( cImage ) ]

cWhisk.Cast_As_Image.restype = POINTER( cImage )
cWhisk.Cast_As_Image.argtypes = [
  POINTER( c_uint8 ),
  c_int,
  c_int,
  c_int ]

cWhisk.find_segments.restype = POINTER( cWhisker_Seg )
cWhisk.find_segments.argtypes = [
  c_int,               # the frame id (acts as time stamp)
  POINTER( cImage ),   # the image to analyze
  POINTER( cImage ),   # a background image -- not used
  POINTER( c_int )     # a pointer indicating how many segments are in the returned array
]


def Trace_Whisker( seed, image ):
  cim = cImage.fromarray( image )
  cws = cWhisk.trace_whisker( byref(seed), cim )
  if cws:
    ws = Whisker_Seg( cws.contents )
    cWhisk.Free_Whisker_Seg( cws )
    return ws
  else:
    return None

def find_segments( image, iframe=0):
  cim = cImage.fromarray(image)
  n = c_int()
  cwv = cWhisk.find_segments( iframe, byref(cim), None, byref(n) )
  wv = [ Whisker_Seg( cwv[i] ) for i in xrange( n.value ) ] # copy into friendlier container
  cWhisk.Free_Whisker_Seg_Vec( cwv, n )                     # free
  return wv

#
# HMM's 
#
# It'd probably be best to move this stuff to another module since it has
# little to do with tracing and more with correspondance.  On the other hand,
# it's convenient to have everything in one place...
#

class cViterbiResult(Structure):
  _fields_ = [ ( "total"    ,          c_double  ),   ## Probability of observation given model                                  
               ( "prob"     ,          c_double  ),   ## Probability of most likely state sequence given model and observations  
               ( "n"        ,          c_int     ),   ## Most likely state sequence - size
               ( "sequence" , POINTER( c_int )   )]   ## Most likely state sequence 
  def asarray(self): # this is a copy
    s = zeros( self.n, dtype = int32 )
    for i in xrange( self.n ):
      s[i] = self.sequence[i]
    return s
  def get(self):
    return self.total, self.prob, self.asarray()

cWhisk.Free_Viterbi_Result.restype = None
cWhisk.Free_Viterbi_Result.argtypes = [ POINTER( cViterbiResult ) ]

cWhisk.Forward_Viterbi_Log2.restype = POINTER( cViterbiResult )
cWhisk.Forward_Viterbi_Log2.argtypes = [
  POINTER( c_int    ),          ## sequence,         // size: nseq
           c_int     ,          ## nseq,             // number of observations (size of seqeunce)                           
  POINTER( c_double ),          ## start_prob,       // size: nstates (log probs)                                           
  POINTER( c_double ),          ## transition_prob,  // size: nstates*nstates, destintion state-major order (log probs)     
  POINTER( c_double ),          ## emmission_prob,   // size: nstates * nobs, state-major order (stride is nobs) (log probs)
           c_int     ,          ## nobs,             // the size of the set of observables ( need for stride )            
           c_int     ]          ## nstates           // number of states (size of state alphabet)

def viterbi_log2(sequence, start, transitions, emmissions, do_checks=True):
  if do_checks:
    #check types
    assert( transitions.dtype == float64 )
    assert( transitions.dtype == float64 )

    # try to coerce sequence
    if not isinstance( sequence, ndarray ):
      sequence = array( sequence, dtype = int32 )
    if sequence.dtype != int32:
      sequence = sequence.astpye(int32)

    #check sizes
    nstates = start.shape[0]
    nobs    = emmissions.shape[1]
    assert( transitions.shape[0] == nstates and transitions.shape[1] == nstates )
    assert( emmissions.shape[0] == nstates )

  res = cWhisk.Forward_Viterbi_Log2( sequence.ctypes.data_as( POINTER( c_int32 ) ),
                                     c_int( len(sequence) ),
                                     start.ctypes.data_as(       POINTER( c_double ) ),
                                     transitions.ctypes.data_as( POINTER( c_double ) ),
                                     emmissions.ctypes.data_as(   POINTER( c_double ) ),
                                     c_int( nobs ),
                                     c_int( nstates ) )
  ret = res.contents.get()
  cWhisk.Free_Viterbi_Result(res)
  return ret

