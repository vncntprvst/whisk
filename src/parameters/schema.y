/* Grammar for file defining parameters */
/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : June 2010
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/* 
 *  [?] memory leak from string tokens.  "need" to free them when we're done
 * params.y (generated)
 *  [x] check to make sure all the required tokens got hit
 *  [x] API
 *      [x] Load_Params_File(char* filename);
 *      [x] #define * (g_param.*)
 *      [x] make the .h file
 *  [x] More descriptive handling of syntax errors
 *      [ ]  FIXME: enum token checking isn't handled correctly.  Values from
 *                  different enums aren't distinguished.
 *  [x] Set number of errors before panic.
 *  [x] Parse function should return failure if there are any errors
 * For later:
 *  [ ] bounds checking on types
 */

%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#if 0
#define DEBUG_LEXER
#define DEBUG_GRAMMAR
#endif

#define ASRT(e) do {size_t v = (size_t)(e); assert(v); } while(0)

#ifdef DEBUG_LEXER
  #define debug_lexer(...) printf(__VA_ARGS__)
#else
  #define debug_lexer(...)
#endif

#ifdef DEBUG_GRAMMAR
  #define debug_grammar(...) printf(__VA_ARGS__)
#else
  #define debug_grammar(...)
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define CPRN(...) fprintf(cfile,__VA_ARGS__)
#define HPRN(...) fprintf(hfile,__VA_ARGS__)

FILE *cfile = NULL;
FILE *hfile = NULL;
char *g_headername = NULL;
int  yylex  (void);
void yyerror(char const *);

char *default_params[] = {
  "[error]\\n",
  "SHOW_DEBUG_MESSAGES     1\\n",
  "SHOW_PROGRESS_MESSAGES  1\\n",
  "\\n",
  "[reclassify]\\n",
  "HMM_RECLASSIFY_SHP_DISTS_NBINS 16\\n",
  "HMM_RECLASSIFY_VEL_DISTS_NBINS 8096\\n",
  "HMM_RECLASSIFY_BASELINE_LOG2   -500.0\\n",
  "COMPARE_IDENTITIES_DISTS_NBINS 8096\\n",
  "IDENTITY_SOLVER_VELOCITY_NBINS 8096\\n",
  "IDENTITY_SOLVER_SHAPE_NBINS    16\\n",
  "\\n",
  "[trace]\\n",
  "SEED_METHOD                    SEED_ON_GRID // Specify seeding method: may be SEED_ON_MHAT_CONTOURS, SEED_ON_GRID, or SEED_EVERYWHERE\\n",
  "SEED_ON_GRID_LATTICE_SPACING   50           // (pixels)\\n",
  "SEED_SIZE_PX                   4            // Width of the seed detector in pixels.\\n",
  "SEED_ITERATIONS                1            // Maxium number of iterations to re-estimate a seed.\\n",
  "SEED_ITERATION_THRESH          0.0          // (0 to 1) Threshold score determining when a seed should be reestimated.\\n",
  "SEED_ACCUM_THRESH              0.0          // (0 to 1) Threshold score determining when to accumulate statistics\\n",
  "SEED_THRESH                    0.99         // (0 to 1) Threshold score determining when to generate a seed\\n",
  "\\n",
  "HAT_RADIUS                     1.5          // Mexican-hat radius for whisker detection (seeding)\\n",
  "MIN_LEVEL                      1            // Level-set threshold for mexican hat result.  Used for seeding on mexican hat contours.\\n",
  "MIN_SIZE                       20           // Minimum # of pixels in an object considered for mexican-hat based seeding.\\n",
  "\\n",
  "                                            // detector banks parameterization.  If any of these change, the detector banks\\n",
  "                                            // should be deleted.  They will be regenerated on the next run.\\n",
  "                                            //\\n",
  "TLEN                           8            // (px) half the size of the detector support.  If this is changed, the detector banks must be deleted.\\n",
  "OFFSET_STEP                    .1           // pixels\\n",
  "ANGLE_STEP                     18.          // divisions of pi/4\\n",
  "WIDTH_STEP                     .2           // (pixels)\\n",
  "WIDTH_MIN                      0.4          // (pixels) must be a multiple of WIDTH_STEP\\n",
  "WIDTH_MAX                      6.5          // (pixels) must be a multiple of WIDTH_STEP\\n",
  "MIN_SIGNAL                     5.0          // minimum detector response per detector column.  Typically: (2*TLEN+1)*MIN_SIGNAL is the threshold determining when tracing stops.\\n",
  "MAX_DELTA_ANGLE                10.1         // (degrees)  The detector is constrained to turns less than this value at each step.\\n",
  "MAX_DELTA_WIDTH                6.0          // (pixels)   The detector width is constrained to change less than this value at each step.\\n",
  "MAX_DELTA_OFFSET               6.0          // (pixels)   The detector offset is constrained to change less than this value at each step.\\n",
  "HALF_SPACE_ASSYMETRY_THRESH    0.25         // (between 0 and 1)  1 is completely insensitive to asymmetry\\n",
  "HALF_SPACE_TUNNELING_MAX_MOVES 50           // (pixels)  This should be the largest size of an occluding area to cross\\n",
  "\\n",                                                                                                                                                                                                                
  "FRAME_DELTA                    1            // [deprecated?] used in compute_zone to look for moving objects\\n",
  "DUPLICATE_THRESHOLD            5.0          // [deprecated?]\\n",
  "MIN_LENGTH                     20           // [deprecated?]           If span of object is not 20 pixels will not use as a seed\\n",
  "MIN_LENSQR                     100          // [deprecated?]           (MIN_LENGTH/2)^2\\n",
  "MIN_LENPRJ                     14           // [deprecated?] [unused]  floor(MIN_LENGTH/sqrt(2))\\n",
  NULL,
};

//
// Type
//

struct enumlist
{ char *name;
  struct enumlist *last;
};

typedef enum _typeids
{ BOOL=1,
  INT,
  FLOAT,
  ENUM
} etypeid;

typedef struct _type
{ etypeid id;
  struct enumlist *e;
} Type;

struct t_specials
{ etypeid id;
  char *name;
};

struct t_specials specials[] = { {BOOL, "BOOL"},
                                 {INT, "INT"},
                                 {FLOAT, "FLOAT"},
                                 {0, NULL }};
Type *newtype(void)
{ Type *self;
  ASRT(self=malloc(sizeof(Type)));
  memset(self,0,sizeof(Type));
  return self;
}

struct enumlist *newenumnode(void)
{ struct enumlist *self;
  ASRT(self=malloc(sizeof(struct enumlist)));
  memset(self,0,sizeof(struct enumlist));
  return self;
}

Type *gentype(Type *left, char *right)
{ struct t_specials *cur;
  Type *self = newtype();
  for(cur=specials; cur->name; cur++ )
  { if( strcmp(right,cur->name)==0 )
    { self->id = cur->id;
      break;
    }
  }
  if( cur->name==NULL )                // didn't find anything, must be an enum
  {
    self->id = ENUM;
    self->e = newenumnode();
    self->e->name = right;
    if(left)                           // need to append to left (it's an enum list)
    { ASRT(left->id == ENUM );
      self->e->last = left->e;
      free(left);
    } else {                           // this is a new enum list, so make the root node
      self->e->last = NULL;
    }
  }
  return self;
}

//
// KEYVALUE
//
typedef struct keyvalue
{ char *key;
  Type *value;
  struct keyvalue *last;
} tkv;

tkv *g_kv = NULL;

tkv *new_keyvalue(void)
{ tkv *self;
  ASRT(self = malloc(sizeof(tkv)));
  memset(self,0,sizeof(tkv));
  return self;
}

void kvprint_enum_defn(tkv *self)
{ if(self->value && self->value->id==ENUM)
  {
        HPRN("typedef enum _t_enum_%s {",self->key);
        { struct enumlist *cur;
          ASRT(self->value->e);
          for(cur=self->value->e;cur->last;cur=cur->last)
            HPRN("%s, ",cur->name);
          HPRN("%s} enum%s;\n",cur->name, self->key);
        }
  }
}
void kvprint_enum_defns(tkv* self)
{ 
  kvprint_enum_defn(self);
  while(self->last->key)
  { self = self->last;
    kvprint_enum_defn(self);
  }
}

void kvprint_enum_token(tkv *self)
{ if(self->value && self->value->id==ENUM)
  { struct enumlist *cur;
    ASRT(self->value->e);
    for(cur=self->value->e;cur->last;cur=cur->last)
      CPRN("%%token <val%s> TOK_%s \"%s\"\n",self->key,cur->name,cur->name);
    CPRN("%%token <val%s> TOK_%s \"%s\"\n",self->key,cur->name,cur->name);
  }
}
void kvprint_enum_tokens(tkv* self)
{ 
  kvprint_enum_token(self);
  while(self->last->key)
  { self = self->last;
    kvprint_enum_token(self);
  }
}

void kvprint_API_defn_getters(tkv *self)
{ tkv* cur;
  for(cur=self;cur->last;cur=cur->last)
    HPRN("#define %s (Params()->param%s)\n",cur->key,cur->key);
}

void kvprint_params_struct_item(tkv *self)
{ 
  if(self->value)
  { switch( self->value->id )
    { case BOOL:
        HPRN("\tchar\t");
        break;
      case INT:
        HPRN("\tint\t");
        break;
      case FLOAT:
        HPRN("\tfloat\t");
        break;
      case ENUM:
        HPRN("\tenum%s\t",self->key);
        break;
      default:
        ASRT(0);
    }
  }
  if(self->key)
    HPRN("param%s;\n",self->key);
  else
    HPRN("\n");
}

void kvprint_params_struct(tkv* self)
{ HPRN("typedef struct _t_params {\n"); 
  kvprint_params_struct_item(self);
  while(self->last->key)
  { self = self->last;
    kvprint_params_struct_item(self);
  }
  HPRN("} t_params;\n");
  CPRN("t_params g_param;\n");
}

void kvprint_param_string_table(tkv* self)
{ tkv* cur;
  CPRN("char *g_param_string_table[] = {\n");
  for(cur=self;cur->last->last;cur=cur->last)
  { CPRN("\t\"%s\",\n",cur->key);
  }
  CPRN("\t\"%s\"};\n",cur->key); 
}

void kvprint_param_tracking_enum(tkv* self)
{ tkv *cur;
  CPRN("enum eParamTrackingIndexes {\n");
  for(cur=self;cur->last;cur=cur->last)
  { CPRN("\tindex%s,\n",cur->key);
  }
  CPRN("\tindexMAX\n};\n"
         "int g_found_parameters[indexMAX];\n");

}

void kvprint_token(tkv *self)
{ CPRN("%%token %s\t\"%s\"\n",self->key,self->key);
}

void kvprint_tokens(tkv *self)
{ tkv *root = self;
  // union - print the value types
  CPRN("%%union{ int integral;\n"
         "        float decimal;\n");
  for(;self->last->key;self=self->last)
  { if(self->value->id==ENUM)
      CPRN("        enum%s val%s;\n",self->key,self->key);
  }
  CPRN("       }\n");
#if 0
  CPRN("//Parameter name tokens\n");
  self=root;
  kvprint_token(self);
  while(self->last->key)
  { self = self->last;
    kvprint_token(self);
  }
#endif
  CPRN("//Normal tokens\n"
         "%%token            COMMENT\n"
         "%%token <integral> INTEGRAL\n"
         "%%token <decimal>  DECIMAL\n"
         "%%type  <decimal>  decimal\n"
         );
  CPRN("//Enum value tokens\n");
  kvprint_enum_tokens(root);
}                                                  

void kvprint_line_grammer_item_type(tkv *self)
{ 
  if(self->value)
  { 
    switch( self->value->id )
    { case BOOL:
        CPRN("integral");
        break;
      case INT:
        CPRN("integral");
        break;
      case FLOAT:
        CPRN("decimal");
        break;
      case ENUM:
        CPRN("val%s",self->key);
        break;
      default:
        ASRT(0);
    }
  }
}

void kvprint_line_grammer_item(tkv* self)
{ 
  CPRN("      | \"%s\" ",self->key);
  switch( self->value->id )
  { case BOOL:
      CPRN("INTEGRAL");
      break;
    case INT:
      CPRN("INTEGRAL");
      break;
    case FLOAT:
      CPRN("decimal");
      break;
    case ENUM:
      CPRN("value");
      break;
    default:
      ASRT(0);
  }
  CPRN(" comment '\\n'  {g_param.param%s=$<",self->key);
  kvprint_line_grammer_item_type(self);
  CPRN(">2;g_found_parameters[index%s]=1; debug_grammar(\"\\t%s\\n\");}\n",self->key,self->key);
  CPRN("      | \"%s\" error comment '\\n'  {\n"
         "                                      fprintf(stderr,\n"
         "                                             \""
         "Problem encountered loading parameters file at line %%d, columns %%d-%%d\\n\"\n"
         "                                             \"\\tCould not interpret value for %s.\\n\"\n"
         "                                             \"\\tExpected a value of ",self->key,self->key);
  if(self->value->id==ENUM)
  { struct enumlist *cur;
    CPRN(":\\n");
    for(cur=self->value->e;cur->last;cur=cur->last)
    { 
      CPRN("\\t\\t%s\\n",cur->name);
    }
    CPRN("\\t\\t%s\\n\",\n",cur->name);
  } else
  { kvprint_line_grammer_item_type(self);
    CPRN(" type.\\n\",\n");
  }
    CPRN(
         "                                               @2.first_line,@2.first_column+1,\n" 
         "                                               @2.last_column+1);\n"
         "\n"
         "                                       PARSER_INCERR\n"
         "                                       yyerrok;\n" 
         "                                     }\n");
}

void kvprint_line_grammar(tkv* self)
{ CPRN("line:   '\\n'                        { debug_grammar(\"\\tEMPTY LINE\\n\"); }\n");
  kvprint_line_grammer_item(self);
  while(self->last->key)
  { self = self->last;
    kvprint_line_grammer_item(self);
  }
  CPRN(//"      | '[' STRING ']' '\\n'         { debug_grammar(\"\\tSECTION\\n\"); }\n" // These get processed by the (produced) lexer as comments
         "      | comment '\\n'                { debug_grammar(\"\\tCOMMENT LINE\\n\"); }\n"
         "      | error '\\n'                  { fprintf(stderr,\"Problem "
         "encountered loading parameters file at line %%d, columns %%d-%%d\\n\"\n"
         "                                                       "
         "\"\\tCould not interpret parameter name.\\n\",\n"
         "                                               @1.first_line,@1.first_column+1,\n" 
         "                                               @1.last_column+1);\n"
         "                                       PARSER_INCERR\n"
         "                                       yyerrok;\n" 
         "                                     }\n"
         "      ;\n");
}

void kvprint_enum_grammar_value(tkv *self)
{ if(self->value && self->value->id==ENUM)
  { struct enumlist *cur;
    ASRT(self->value->e);
    for(cur=self->value->e;cur->last;cur=cur->last)
      CPRN("       | TOK_%s          {$<val%s>$=%s;}\n",cur->name,self->key,cur->name); 
    CPRN("       | TOK_%s          {$<val%s>$=%s;}\n",cur->name,self->key,cur->name); 
  }
}
void kvprint_enum_grammar_values(tkv* self)
{ 
  kvprint_enum_grammar_value(self);
  while(self->last->key)
  { self = self->last;
    kvprint_enum_grammar_value(self);
  }
}
void kvprint_value_grammar(tkv *self)
{ 
  
  CPRN("decimal: INTEGRAL         {$$=$1;}\n"
         "       | DECIMAL          {$$=$1;}\n");
  CPRN("value:   INTEGRAL         {$<integral>$=$1;}\n"
         "       | DECIMAL          {$<decimal>$=$1;}\n");
  kvprint_enum_grammar_values(self);
  CPRN("       ;\n");
}      

void print_prelude(char *headername)
{ CPRN(
     "/*\n"
     " * Author: Nathan Clack <clackn@janelia.hhmi.org>\n"
     " * Date  : June 2010\n"
     " * \n"
     " * Copyright 2010 Howard Hughes Medical Institute.\n"
     " * All rights reserved.\n"
     " * Use is subject to Janelia Farm Research Campus Software Copyright 1.1\n"
     " * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).\n"
     " */\n"
     "#include <stdio.h>\n"
     "#include <string.h>\n"
     "#include <stdlib.h>\n"
     "#include <ctype.h>\n"
     "#include <assert.h>\n"
     "#include \"%s\"\n"
     "\n"
     "#define ASRT(e) do {size_t v = (size_t)(e); assert(v); } while(0)\n"
     "\n"
     "#define YYPRINT\n"
     "#if 0\n"
     "#define DEBUG_LEXER\n"
     "#define DEBUG_GRAMMAR\n"
     "#endif\n"
     "\n"
     "#ifdef DEBUG_LEXER\n"
     "  #define debug_lexer(...) printf(__VA_ARGS__)\n"
     "#else\n"
     "  #define debug_lexer(...)\n"
     "#endif\n"
     "\n"
     "#ifdef DEBUG_GRAMMAR\n"
     "  #define debug_grammar(...) printf(__VA_ARGS__)\n"
     "#else\n"
     "  #define debug_grammar(...)\n"
     "#endif\n"
     "\n"
     "int  yylex  (void);\n"
     "void yyerror(char const *);\n"
     "\n"
     "int g_nparse_errors = 0;\n"
     "#define MAX_PARSE_ERRORS (10)\n"

     "#define PARSER_INCERR \\\n"
     "if(++g_nparse_errors>MAX_PARSE_ERRORS)\\\n"
     "{ fprintf(stderr,\"Too many problems parsing parameter file. Aborting.\\n\");\\\n"
     "  YYABORT;\\\n"
     "}\n",headername
     );
}

void print_epilogue() 
{
  CPRN(
     "FILE *fp=NULL;                                               // when a file is opened, this points to the file\n"
     "int yylex(void)\n"
     "{ int c;\n"
     "  static char *str = NULL;\n"
     "  static size_t str_max_size = 0;\n"
     "  ASRT(fp);\n"
     "\n"
     "\n"
     "  if(!str)\n"
     "  { str = malloc(1024);\n"
     "    ASRT(str);\n"
     "    str_max_size = 1024;\n"
     "  }\n"
     "\n"
     "  while( (c=getc(fp))==' '||c=='\\t' ) ++yylloc.last_column;  // skip whitespace\n"
     "  if(c==0)\n"
     "  { if(feof(fp))\n"
     "      return 0;\n"
     "    if(ferror(fp))\n"
     "    { fprintf(stderr,\"\\t lex - Got error: %%d\\n\",ferror(fp));\n"
     "    }\n"
     "  }\n"
     "\n"

     "  /* Step */\n"
     "  yylloc.first_line = yylloc.last_line;\n"
     "  yylloc.first_column = yylloc.last_column;\n"
     "  debug_lexer(\"\\t\\t\\t\\t lex - Start: %%d.%%d\\n\",yylloc.first_line,yylloc.first_column);\n"
     "  \n"

     "  if(isalpha(c))                                 // process string tokens\n"
     "  { int i,n=0;\n"
     "    while(!isspace(c))\n"
     "    { \n"
     "      ++yylloc.last_column;\n"
     "      if( n >= str_max_size )                    // resize if necessary\n"
     "      { str_max_size = 1.2*n+50;\n"
     "        str = realloc(str,str_max_size);\n"
     "        ASRT(str);\n"
     "      }\n"
     "      str[n++] = c;\n"
     "      c = getc(fp);\n"
     "    }\n"
     "    ungetc(c,fp);\n"
     "    str[n]='\\0';\n"
     "                                                // search token table\n"
     "    for (i = 0; i < YYNTOKENS; i++)\n"
     "    {\n"
     "      if (yytname[i] != 0\n"
     "          && yytname[i][0] == '\"'\n"
     "          && ! strncmp(yytname[i] + 1, str,strlen(str))\n"
     "          && yytname[i][strlen(str) + 1] == '\"'\n"
     "          && yytname[i][strlen(str) + 2] == 0)\n"
     "        break;\n"
     "    }\n"
     "    if(i<YYNTOKENS)\n"
     "    {\n"
     "      debug_lexer(\"\\tlex - (%%d) %%s\\n\",i,str);\n"
     "      return yytoknum[i];\n"
     "    }\n"
     "    else //nothing was found - put characters back on the stream\n"
     "    { yylloc.last_column-=n;\n"
     "      while(n--)\n"
     "      { ungetc(str[n],fp);\n"
     "      }\n"
     "      c = getc(fp);\n"
     "      ++yylloc.last_column;\n"
     "    }\n"
     "  }\n"
     "\n"

     "  if(c=='.'||isdigit(c)||c=='-')     // process numbers\n"
     "  { int n = 0;\n"
     "    do\n"
     "    { if( n >= str_max_size )        //resize if necessary\n"
     "      { str_max_size = 1.2*n+50;\n"
     "        str = realloc(str,str_max_size);\n"
     "        ASRT(str);\n"
     "      }\n"
     "      str[n++] = c;\n"
     "      c = getc(fp);\n"
     "      ++yylloc.last_column;\n"
     "    }while(c=='.'||isdigit(c));\n"
     "    ungetc(c,fp);\n"
     "    --yylloc.last_column;\n"
     "    str[n]='\\0';\n"
     "    if(strchr(str,'.'))\n"
     "    { yylval.decimal = atof(str);\n"
     "      debug_lexer(\"\\tlex - DECIMAL (%%f)\\n\",yylval.decimal);\n"
     "      return DECIMAL;\n"
     "    } else\n"
     "    { yylval.integral = atoi(str);\n"
     "      debug_lexer(\"\\tlex - INTEGRAL (%%d)\\n\",yylval.integral);\n"
     "      return INTEGRAL;\n"
     "    }\n"
     "  }\n"
     "\n"
     "  if(c=='[')                                                 // process section headers as comments\n"
     "  { while( getc(fp)!='\\n' ) ++yylloc.last_column;           // read the rest of the line\n"
     "    ungetc('\\n',fp);\n"
     "    debug_lexer(\"\\tlex - COMMENT\\n\");\n"
     "    return COMMENT;\n"
     "  }\n"
     "  if(c=='/')                                                 // process c-style end-of-line comments\n"
     "  { int d;\n"
     "    d=getc(fp);\n"
     "    ++yylloc.last_column;\n"
     "    if(d=='/' || d=='*')\n"
     "    { while( getc(fp)!='\\n' ) ++yylloc.last_column;        // read the rest of the line\n"
     "      ungetc('\\n',fp);\n"
     "    }\n"
     "    debug_lexer(\"\\tlex - COMMENT\\n\");\n"
     "    return COMMENT;\n"
     "  }\n"
     "\n"
     "  if(c==EOF)\n"
     "  { debug_lexer(\"\\tlex - EOF\\n\");\n"
     "    fclose(fp);\n"
     "    fp=0;\n"
     "  }\n"
     "  if(c=='\\n')\n"
     "  {\n" 
     "    ++yylloc.last_line;\n" 
     "    yylloc.last_column=0;\n"
     "  }\n" 
     "  return c;\n"
     "}\n"
     "\n"
     "void yyerror(char const *s)\n"
     "{ //fprintf(stderr,\"Parse Error:\\n---\\n\\t%%s\\n---\\n\",s);\n"
     "}\n"
     "\n"
     "SHARED_EXPORT\n"
     "t_params *Params(void) {return &g_param;}\n"
     "\n"
     "SHARED_EXPORT\n"
     "int Load_Params_File(char *filename)\n"
     "{ int sts; //0==success, 1==failure\n"
     "  // FILE *fp is global\n"
     "  g_nparse_errors=0;\n"
     "  memset(g_found_parameters,0,sizeof(g_found_parameters));\n"
     "  fp = fopen(filename,\"r\");\n"
     "  if(!fp)\n"
     "  { fprintf(stderr,\"Could not open parameter file at %%s.\\n\",filename);\n"
     "    return 1;\n"
     "  }\n"
     "  sts = yyparse();\n"
     "  if(fp) fclose(fp);\n"
     "  sts |= (g_nparse_errors>0);\n"
     "  {\n"
     "    int i;\n"
     "    for(i=0;i<indexMAX;++i)\n"
     "      if(g_found_parameters[i]==0)\n"
     "      { sts=1;\n"
     "        fprintf(stderr,\"Failed to load parameter: %%s\\n\",g_param_string_table[i]);\n"
     "      }\n"
     "  }\n"
     "  return sts;\n"
     "}\n"
     "\n"
     "SHARED_EXPORT\n"
     "int Print_Params_File(char *filename)\n"
     "{ int sts=0; //0==success, 1==failure\n"
     "  FILE *fp;\n"
     "  fp = fopen(filename,\"w\");\n"
     "  if(!fp)\n"
     "  { fprintf(stderr,\"Could not open parameter file for writing: %%s\\n\",filename);\n"
     "    return 1;\n"
     "  }\n"
     "  {\n");
    { int iline=0;
      char *line;
      char buf[2048];
      while(line=default_params[iline++])
      { memset(buf,0,sizeof(buf));
        snprintf(buf,2048,"    fprintf(fp,\"%s\");\n",line);
        CPRN("%s",buf);
      }
    }
  CPRN(
     "  }\n"
     "  fclose(fp);\n"
     "  return sts;\n"
     "}\n"
     "\n"
     );
}

void kvprintall(tkv *self)
{ CPRN("%%{\n");
  print_prelude(g_headername);
  HPRN(
       "/*\n"
       " * Author: Nathan Clack <clackn@janelia.hhmi.org>\n"
       " * Date  : June 2010\n"
       " *\n"
       " * Copyright 2010 Howard Hughes Medical Institute.\n"
       " * All rights reserved.\n"
       " * Use is subject to Janelia Farm Research Campus Software Copyright 1.1\n"
       " * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).\n"
       " */\n"
       "#pragma once\n"
       "#ifndef SHARED_EXPORT\n"
       "#ifdef _MSC_VER\n"
       "#define SHARED_EXPORT __declspec(dllexport)\n"
       "#else\n"
       "#define SHARED_EXPORT\n"
       "#endif\n"
       "#endif\n");
  kvprint_enum_defns(self);
  kvprint_API_defn_getters(self);
  kvprint_params_struct(self);
  kvprint_param_tracking_enum(self);
  kvprint_param_string_table(self);
  CPRN("%%}\n");
  CPRN("%%token-table\n");
  kvprint_tokens(self);
  CPRN("%%%%\n");
  CPRN("input:   /*empty*/                   { debug_grammar(\"EMPTY INPUT\\n\"); }\n"  
         "       | input line                  { debug_grammar(\"LINE\\n\"); }\n"
         "       ;\n");
  CPRN("comment: /*empty*/                   { debug_grammar(\"\\t\\tCOMMENT EMPTY\\n\"); }\n"  
         "       | COMMENT                     { debug_grammar(\"\\t\\tCOMMENT TAIL\\n\"); }\n"
         "       ;\n");
  kvprint_line_grammar(self);
  kvprint_value_grammar(self);
  CPRN("%%%%\n");
  HPRN("SHARED_EXPORT t_params* Params();\n");
  HPRN("SHARED_EXPORT int Load_Params_File(char *filename);\n");
  HPRN("SHARED_EXPORT int Print_Params_File(char *filename);\n");
  print_epilogue();
}

tkv *kvpush(tkv *self, char* key, Type *value)
{ tkv *n = new_keyvalue();
  n->key = key;
  n->value = value;
  n->last = self;
  return n;
}
%}

%union { 
  char *string;
  Type *type;
  }

%token <string> STRING
%token <string> COMMENT
%type  <type>   value

%initial-action
{ g_kv = new_keyvalue();
};

%%
input:   /*empty*/                  { debug_grammar("EMPTY INPUT\n"); }  
      | input line                  { debug_grammar("LINE\n"); }   
      ;

line:   '\n'                        { debug_grammar("\tEMPTY LINE\n"); } 
      | STRING value comment '\n'   { debug_grammar("\tKV LINE Pushed\n"); g_kv=kvpush(g_kv,$1,$2);}
      | '[' STRING ']' '\n'         { debug_grammar("\tSECTION\n"); }
      | comment '\n'                { debug_grammar("\tCOMMENT LINE\n"); }
      | error '\n'                  { yyerrok; }
      ;

value:  STRING           {$$=gentype(NULL,$1); debug_grammar("\t\tVALUE Root\n");}
      | value '|' STRING {$$=gentype($1,$3);   debug_grammar("\t\tVALUE Tail\n");}

comment: /*empty*/                  { debug_grammar("\t\tCOMMENT EMPTY\n"); }
      | COMMENT                     { debug_grammar("\t\tCOMMENT TAIL\n"); }

%%

int isnamechar(int c)
{ return isalpha(c) || isdigit(c) || c=='_';
}

FILE *fp=NULL;                                               // when a file is opened, this points to the file
int yylex(void)
{ int c;
  ASRT(fp);
  while( (c=getc(fp))==' '||c=='\t' );                       // skip whitespace
  if(c==0)
  { if(feof(fp))
      return 0;
    if(ferror(fp))
    { fprintf(stderr,"\t lex - Got error: %d\n",ferror(fp));
    }
  }

  if(isnamechar(c))                                          // process strings
  {
    int n,max=1024;
    char *str = malloc(max);
    ASRT(str);
    str[0] = c;
    n = 1;
    while(isnamechar(c=getc(fp)))
    { if(n>=max)
      { str = realloc(str, 1.2*n+50);
        ASRT(str);
      }
      str[n++]=c;
    }
    str[n]='\0';
    ungetc(c,fp);

    yylval.string = str;
    debug_lexer("\tlex - STRING: %s\n",str);
    return STRING;
  }

  if(c=='/')                                                 // process comments
  { int d;
    d=getc(fp);
    if(d=='/' || d=='*')
    { while( getc(fp)!='\n' );                               // read the rest of the line
      ungetc('\n',fp);
    }
    yylval.string = NULL;
    debug_lexer("\tlex - COMMENT\n");
    return COMMENT;
  }

  if(c==EOF)
  { debug_lexer("\tlex - EOF\n");
    kvprintall(g_kv);
    return 0;
  }
  return c;
}

void yyerror(char const *s)
{ fprintf(stderr,"Parse Error:\n---\n\t%s\n---\n",s);
} 

int
main(int argc,char* argv[])
{ int sts;
  //cfile = stdout;
  //hfile = stdout;
  if(argc!=4)
  { printf(
      "Usage:\n"
      "  %s <in parameters.schema> <out param.y> <out param.h>\n",argv[0]);
    return 111;
  }
  cfile = fopen(argv[2],"w");
  hfile = fopen(argv[3],"w");
  ASRT(cfile);
  ASRT(hfile);
  g_headername = argv[3];
  fp = fopen(argv[1],"r");
  ASRT(fp);
  sts = yyparse();
  fclose(fp);
  fclose(hfile);
  fclose(cfile);
  return sts;
}
