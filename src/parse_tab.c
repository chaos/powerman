/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_LOGIN = 258,
     TOK_LOGOUT = 259,
     TOK_STATUS = 260,
     TOK_STATUS_ALL = 261,
     TOK_STATUS_SOFT = 262,
     TOK_STATUS_SOFT_ALL = 263,
     TOK_STATUS_TEMP = 264,
     TOK_STATUS_TEMP_ALL = 265,
     TOK_STATUS_BEACON = 266,
     TOK_STATUS_BEACON_ALL = 267,
     TOK_BEACON_ON = 268,
     TOK_BEACON_OFF = 269,
     TOK_ON = 270,
     TOK_ON_RANGED = 271,
     TOK_ON_ALL = 272,
     TOK_OFF = 273,
     TOK_OFF_RANGED = 274,
     TOK_OFF_ALL = 275,
     TOK_CYCLE = 276,
     TOK_CYCLE_RANGED = 277,
     TOK_CYCLE_ALL = 278,
     TOK_RESET = 279,
     TOK_RESET_RANGED = 280,
     TOK_RESET_ALL = 281,
     TOK_PING = 282,
     TOK_SPEC = 283,
     TOK_EXPECT = 284,
     TOK_SETPLUGSTATE = 285,
     TOK_SEND = 286,
     TOK_DELAY = 287,
     TOK_FOREACHPLUG = 288,
     TOK_FOREACHNODE = 289,
     TOK_IFOFF = 290,
     TOK_IFON = 291,
     TOK_OFF_STRING = 292,
     TOK_ON_STRING = 293,
     TOK_MAX_PLUG_COUNT = 294,
     TOK_TIMEOUT = 295,
     TOK_DEV_TIMEOUT = 296,
     TOK_PING_PERIOD = 297,
     TOK_PLUG_NAME = 298,
     TOK_SCRIPT = 299,
     TOK_DEVICE = 300,
     TOK_NODE = 301,
     TOK_ALIAS = 302,
     TOK_TCP_WRAPPERS = 303,
     TOK_PORT = 304,
     TOK_MATCHPOS = 305,
     TOK_STRING_VAL = 306,
     TOK_NUMERIC_VAL = 307,
     TOK_YES = 308,
     TOK_NO = 309,
     TOK_BEGIN = 310,
     TOK_END = 311,
     TOK_UNRECOGNIZED = 312,
     TOK_EQUALS = 313
   };
#endif
/* Tokens.  */
#define TOK_LOGIN 258
#define TOK_LOGOUT 259
#define TOK_STATUS 260
#define TOK_STATUS_ALL 261
#define TOK_STATUS_SOFT 262
#define TOK_STATUS_SOFT_ALL 263
#define TOK_STATUS_TEMP 264
#define TOK_STATUS_TEMP_ALL 265
#define TOK_STATUS_BEACON 266
#define TOK_STATUS_BEACON_ALL 267
#define TOK_BEACON_ON 268
#define TOK_BEACON_OFF 269
#define TOK_ON 270
#define TOK_ON_RANGED 271
#define TOK_ON_ALL 272
#define TOK_OFF 273
#define TOK_OFF_RANGED 274
#define TOK_OFF_ALL 275
#define TOK_CYCLE 276
#define TOK_CYCLE_RANGED 277
#define TOK_CYCLE_ALL 278
#define TOK_RESET 279
#define TOK_RESET_RANGED 280
#define TOK_RESET_ALL 281
#define TOK_PING 282
#define TOK_SPEC 283
#define TOK_EXPECT 284
#define TOK_SETPLUGSTATE 285
#define TOK_SEND 286
#define TOK_DELAY 287
#define TOK_FOREACHPLUG 288
#define TOK_FOREACHNODE 289
#define TOK_IFOFF 290
#define TOK_IFON 291
#define TOK_OFF_STRING 292
#define TOK_ON_STRING 293
#define TOK_MAX_PLUG_COUNT 294
#define TOK_TIMEOUT 295
#define TOK_DEV_TIMEOUT 296
#define TOK_PING_PERIOD 297
#define TOK_PLUG_NAME 298
#define TOK_SCRIPT 299
#define TOK_DEVICE 300
#define TOK_NODE 301
#define TOK_ALIAS 302
#define TOK_TCP_WRAPPERS 303
#define TOK_PORT 304
#define TOK_MATCHPOS 305
#define TOK_STRING_VAL 306
#define TOK_NUMERIC_VAL 307
#define TOK_YES 308
#define TOK_NO 309
#define TOK_BEGIN 310
#define TOK_END 311
#define TOK_UNRECOGNIZED 312
#define TOK_EQUALS 313




/* Copy the first part of user declarations.  */
#line 27 "parse_tab.y"

#if HAVE_CONFIG_H
#include "config.h"
#endif
#define YYSTYPE char *  /*  The generic type returned by all parse matches */
#undef YYDEBUG          /* no debug code plese */
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>       /* for HUGE_VAL and trunc */
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>

#include "powerman.h"
#include "list.h"
#include "parse_util.h"
#include "wrappers.h"
#include "pluglist.h"
#include "device.h"
#include "device_serial.h"
#include "device_pipe.h"
#include "device_tcp.h"
#include "error.h"
#include "string.h"

/*
 * A PreScript is a list of PreStmts.
 */
#define PRESTMT_MAGIC 0x89786756
typedef struct {
    int magic;
    StmtType type;              /* delay/expect/send */
    char *str;                  /* expect string, send fmt, setplugstate plug */
    struct timeval tv;          /* delay value */
    int mp1;                    /* setplugstate plug match position */
    int mp2;                    /* setplugstate state match position */
    List prestmts;              /* subblock */
    List interps;               /* interpretations for setplugstate */
} PreStmt;
typedef List PreScript;

/*
 * Unprocessed Protocol (used during parsing).
 * This data will be copied for each instantiation of a device.
 */
typedef struct {
    char *name;                 /* specification name, e.g. "icebox" */
    struct timeval timeout;     /* timeout for this device */
    struct timeval ping_period; /* ping period for this device 0.0 = none */
    List plugs;                 /* list of plug names (e.g. "1" thru "10") */
    PreScript prescripts[NUM_SCRIPTS];  /* array of PreScripts */
} Spec;                                 /*   script may be NULL if undefined */

/* powerman.conf */
static void makeNode(char *nodestr, char *devstr, char *plugstr);
static void makeAlias(char *namestr, char *hostsstr);
static Stmt *makeStmt(PreStmt *p);
static void destroyStmt(Stmt *stmt);
static void makeDevice(char *devstr, char *specstr, char *hoststr, 
                        char *portstr);
static void makeClientPort(char *s2);

/* device config */
static PreStmt *makePreStmt(StmtType type, char *str, char *tvstr, 
                      char *mp1str, char *mp2str, List prestmts, List interps);
static void destroyPreStmt(PreStmt *p);
static Spec *makeSpec(char *name);
static Spec *findSpec(char *name);
static int matchSpec(Spec * spec, void *key);
static void destroySpec(Spec * spec);
static void _clear_current_spec(void);
static void makeScript(int com, List stmts);
static void destroyInterp(Interp *i);
static Interp *makeInterp(InterpState state, char *str);
static List copyInterpList(List ilist);

/* utility functions */
static void _errormsg(char *msg);
static void _warnmsg(char *msg);
static long _strtolong(char *str);
static double _strtodouble(char *str);
static void _doubletotv(struct timeval *tv, double val);

extern int yylex();
void yyerror();


static List device_specs = NULL;      /* list of Spec's */
static Spec current_spec;             /* Holds a Spec as it is built */


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 317 "parse_tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  6
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   141

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  59
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  25
/* YYNRULES -- Number of rules.  */
#define YYNRULES  80
/* YYNRULES -- Number of states.  */
#define YYNSTATES  142

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   313

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     9,    11,    13,    15,    17,    19,
      21,    23,    26,    29,    32,    38,    43,    48,    52,    56,
      59,    61,    67,    70,    72,    74,    76,    78,    80,    83,
      86,    89,    91,    96,    99,   101,   105,   109,   113,   117,
     121,   125,   129,   133,   137,   141,   145,   149,   153,   157,
     161,   165,   169,   173,   177,   181,   185,   189,   193,   197,
     201,   205,   208,   210,   213,   216,   219,   223,   228,   232,
     237,   240,   244,   247,   250,   253,   256,   259,   261,   265,
     269
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      60,     0,    -1,    68,    61,    -1,    61,    62,    -1,    62,
      -1,    64,    -1,    63,    -1,    65,    -1,    66,    -1,    67,
      -1,    48,    -1,    48,    53,    -1,    48,    54,    -1,    49,
      52,    -1,    45,    51,    51,    51,    51,    -1,    45,    51,
      51,    51,    -1,    46,    51,    51,    51,    -1,    46,    51,
      51,    -1,    47,    51,    51,    -1,    68,    69,    -1,    69,
      -1,    28,    51,    55,    70,    56,    -1,    70,    71,    -1,
      71,    -1,    72,    -1,    73,    -1,    75,    -1,    76,    -1,
      41,    52,    -1,    42,    52,    -1,    74,    51,    -1,    51,
      -1,    43,    55,    74,    56,    -1,    76,    77,    -1,    77,
      -1,    44,     3,    78,    -1,    44,     4,    78,    -1,    44,
       5,    78,    -1,    44,     6,    78,    -1,    44,     7,    78,
      -1,    44,     8,    78,    -1,    44,     9,    78,    -1,    44,
      10,    78,    -1,    44,    11,    78,    -1,    44,    12,    78,
      -1,    44,    13,    78,    -1,    44,    14,    78,    -1,    44,
      15,    78,    -1,    44,    16,    78,    -1,    44,    17,    78,
      -1,    44,    18,    78,    -1,    44,    19,    78,    -1,    44,
      20,    78,    -1,    44,    21,    78,    -1,    44,    22,    78,
      -1,    44,    23,    78,    -1,    44,    24,    78,    -1,    44,
      25,    78,    -1,    44,    26,    78,    -1,    44,    27,    78,
      -1,    55,    79,    56,    -1,    79,    80,    -1,    80,    -1,
      29,    51,    -1,    31,    51,    -1,    32,    52,    -1,    30,
      51,    83,    -1,    30,    51,    83,    81,    -1,    30,    83,
      83,    -1,    30,    83,    83,    81,    -1,    30,    83,    -1,
      30,    83,    81,    -1,    34,    78,    -1,    33,    78,    -1,
      35,    78,    -1,    36,    78,    -1,    81,    82,    -1,    82,
      -1,    15,    58,    51,    -1,    18,    58,    51,    -1,    50,
      52,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   149,   149,   155,   156,   158,   159,   160,   161,   162,
     164,   167,   169,   173,   177,   180,   184,   186,   190,   197,
     198,   200,   206,   207,   209,   210,   211,   212,   214,   218,
     222,   225,   230,   236,   237,   239,   241,   243,   245,   247,
     249,   251,   253,   255,   257,   259,   261,   263,   265,   267,
     269,   271,   273,   275,   277,   279,   281,   283,   285,   287,
     291,   295,   298,   303,   305,   307,   309,   311,   314,   316,
     318,   320,   322,   325,   328,   331,   336,   339,   344,   346,
     350
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_LOGIN", "TOK_LOGOUT", "TOK_STATUS",
  "TOK_STATUS_ALL", "TOK_STATUS_SOFT", "TOK_STATUS_SOFT_ALL",
  "TOK_STATUS_TEMP", "TOK_STATUS_TEMP_ALL", "TOK_STATUS_BEACON",
  "TOK_STATUS_BEACON_ALL", "TOK_BEACON_ON", "TOK_BEACON_OFF", "TOK_ON",
  "TOK_ON_RANGED", "TOK_ON_ALL", "TOK_OFF", "TOK_OFF_RANGED",
  "TOK_OFF_ALL", "TOK_CYCLE", "TOK_CYCLE_RANGED", "TOK_CYCLE_ALL",
  "TOK_RESET", "TOK_RESET_RANGED", "TOK_RESET_ALL", "TOK_PING", "TOK_SPEC",
  "TOK_EXPECT", "TOK_SETPLUGSTATE", "TOK_SEND", "TOK_DELAY",
  "TOK_FOREACHPLUG", "TOK_FOREACHNODE", "TOK_IFOFF", "TOK_IFON",
  "TOK_OFF_STRING", "TOK_ON_STRING", "TOK_MAX_PLUG_COUNT", "TOK_TIMEOUT",
  "TOK_DEV_TIMEOUT", "TOK_PING_PERIOD", "TOK_PLUG_NAME", "TOK_SCRIPT",
  "TOK_DEVICE", "TOK_NODE", "TOK_ALIAS", "TOK_TCP_WRAPPERS", "TOK_PORT",
  "TOK_MATCHPOS", "TOK_STRING_VAL", "TOK_NUMERIC_VAL", "TOK_YES", "TOK_NO",
  "TOK_BEGIN", "TOK_END", "TOK_UNRECOGNIZED", "TOK_EQUALS", "$accept",
  "configuration_file", "config_list", "config_item", "TCP_wrappers",
  "client_port", "device", "node", "alias", "spec_list", "spec",
  "spec_item_list", "spec_item", "spec_timeout", "spec_ping_period",
  "string_list", "spec_plug_list", "spec_script_list", "spec_script",
  "stmt_block", "stmt_list", "stmt", "interp_list", "interp", "regmatch", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    59,    60,    61,    61,    62,    62,    62,    62,    62,
      63,    63,    63,    64,    65,    65,    66,    66,    67,    68,
      68,    69,    70,    70,    71,    71,    71,    71,    72,    73,
      74,    74,    75,    76,    76,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      78,    79,    79,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    81,    81,    82,    82,
      83
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     5,     4,     4,     3,     3,     2,
       1,     5,     2,     1,     1,     1,     1,     1,     2,     2,
       2,     1,     4,     2,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     1,     2,     2,     2,     3,     4,     3,     4,
       2,     3,     2,     2,     2,     2,     2,     1,     3,     3,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,    20,     0,     1,     0,     0,     0,
      10,     0,     2,     4,     6,     5,     7,     8,     9,    19,
       0,     0,     0,     0,    11,    12,    13,     3,     0,     0,
       0,     0,     0,    23,    24,    25,    26,    27,    34,     0,
      17,    18,    28,    29,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      21,    22,    33,    15,    16,    31,     0,     0,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    14,    30,    32,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    62,    63,     0,     0,    70,
      64,    65,    73,    72,    74,    75,    60,    61,    80,    66,
       0,     0,    71,    77,    68,    67,     0,     0,    76,    69,
      78,    79
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    12,    13,    14,    15,    16,    17,    18,     3,
       4,    32,    33,    34,    35,    76,    36,    37,    38,    78,
     114,   115,   132,   133,   119
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -86
static const yytype_int8 yypact[] =
{
      -2,     5,    49,    -3,   -86,     3,   -86,    61,    62,    63,
     -13,    66,    56,   -86,   -86,   -86,   -86,   -86,   -86,   -86,
      19,    68,    69,    70,   -86,   -86,   -86,   -86,    71,    72,
      60,    65,    -5,   -86,   -86,   -86,   -86,    73,   -86,    74,
      75,   -86,   -86,   -86,    76,    67,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
      67,    67,    67,    67,    67,    67,    67,    67,    67,    67,
     -86,   -86,   -86,    77,   -86,   -86,     1,    64,   -86,   -86,
     -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,
     -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,
     -86,   -86,   -86,   -86,   -86,   -86,    78,    57,    79,    80,
      67,    67,    67,    67,    -1,   -86,   -86,    81,    84,     9,
     -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,   -86,    91,
      58,    82,    91,   -86,    91,    91,    85,    86,   -86,    91,
     -86,   -86
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -86,   -86,   -86,   119,   -86,   -86,   -86,   -86,   -86,   -86,
     132,   -86,   106,   -86,   -86,   -86,   -86,   -86,   102,   -46,
     -86,    27,   -81,   -85,    -8
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,   100,   101,   102,   130,     1,     1,   131,   106,   107,
     108,   109,   110,   111,   112,   113,    28,    29,    30,    31,
      24,    25,     7,     8,     9,    10,    11,   138,   135,     6,
     138,    70,   104,   139,   138,   126,     5,   105,    20,   117,
      28,    29,    30,    31,   122,   123,   124,   125,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,   106,   107,   108,   109,   110,   111,   112,
     113,     7,     8,     9,    10,    11,   130,   117,   118,   131,
     129,   134,    21,    22,    23,    44,   136,    31,    26,    39,
      40,    41,    77,    42,    43,    73,    74,    75,   103,   116,
     120,    27,   121,   128,   117,    19,   140,   141,    71,    72,
     137,   127
};

static const yytype_uint8 yycheck[] =
{
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    15,    28,    28,    18,    29,    30,
      31,    32,    33,    34,    35,    36,    41,    42,    43,    44,
      53,    54,    45,    46,    47,    48,    49,   132,   129,     0,
     135,    56,    51,   134,   139,    56,    51,    56,    55,    50,
      41,    42,    43,    44,   110,   111,   112,   113,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    29,    30,    31,    32,    33,    34,    35,
      36,    45,    46,    47,    48,    49,    15,    50,    51,    18,
     118,   119,    51,    51,    51,    55,    58,    44,    52,    51,
      51,    51,    55,    52,    52,    51,    51,    51,    51,    51,
      51,    12,    52,    52,    50,     3,    51,    51,    32,    37,
      58,   114
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    28,    60,    68,    69,    51,     0,    45,    46,    47,
      48,    49,    61,    62,    63,    64,    65,    66,    67,    69,
      55,    51,    51,    51,    53,    54,    52,    62,    41,    42,
      43,    44,    70,    71,    72,    73,    75,    76,    77,    51,
      51,    51,    52,    52,    55,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      56,    71,    77,    51,    51,    51,    74,    55,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    51,    51,    56,    29,    30,    31,    32,
      33,    34,    35,    36,    79,    80,    51,    50,    51,    83,
      51,    52,    78,    78,    78,    78,    56,    80,    52,    83,
      15,    18,    81,    82,    83,    81,    58,    58,    82,    81,
      51,    51
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 10:
#line 164 "parse_tab.y"
    { 
    _warnmsg("'tcpwrappers' without yes|no"); 
    conf_set_use_tcp_wrappers(TRUE);
}
    break;

  case 11:
#line 167 "parse_tab.y"
    { 
    conf_set_use_tcp_wrappers(TRUE);
}
    break;

  case 12:
#line 169 "parse_tab.y"
    { 
    conf_set_use_tcp_wrappers(FALSE);
}
    break;

  case 13:
#line 173 "parse_tab.y"
    {
    makeClientPort((yyvsp[(2) - (2)]));
}
    break;

  case 14:
#line 178 "parse_tab.y"
    {
    makeDevice((yyvsp[(2) - (5)]), (yyvsp[(3) - (5)]), (yyvsp[(4) - (5)]), (yyvsp[(5) - (5)]));
}
    break;

  case 15:
#line 180 "parse_tab.y"
    {
    makeDevice((yyvsp[(2) - (4)]), (yyvsp[(3) - (4)]), (yyvsp[(4) - (4)]), NULL);
}
    break;

  case 16:
#line 184 "parse_tab.y"
    {
    makeNode((yyvsp[(2) - (4)]), (yyvsp[(3) - (4)]), (yyvsp[(4) - (4)]));
}
    break;

  case 17:
#line 186 "parse_tab.y"
    {
    makeNode((yyvsp[(2) - (3)]), (yyvsp[(3) - (3)]), NULL);
}
    break;

  case 18:
#line 190 "parse_tab.y"
    {
    makeAlias((yyvsp[(2) - (3)]), (yyvsp[(3) - (3)]));
}
    break;

  case 21:
#line 202 "parse_tab.y"
    {
    makeSpec((yyvsp[(2) - (5)]));
}
    break;

  case 28:
#line 214 "parse_tab.y"
    {
    _doubletotv(&current_spec.timeout, _strtodouble((yyvsp[(2) - (2)])));
}
    break;

  case 29:
#line 218 "parse_tab.y"
    {
    _doubletotv(&current_spec.ping_period, _strtodouble((yyvsp[(2) - (2)])));
}
    break;

  case 30:
#line 222 "parse_tab.y"
    {
    list_append((List)(yyvsp[(1) - (2)]), Strdup((yyvsp[(2) - (2)]))); 
    (yyval) = (yyvsp[(1) - (2)]); 
}
    break;

  case 31:
#line 225 "parse_tab.y"
    {
    (yyval) = (char *)list_create((ListDelF)Free);
    list_append((List)(yyval), Strdup((yyvsp[(1) - (1)]))); 
}
    break;

  case 32:
#line 230 "parse_tab.y"
    {
    if (current_spec.plugs != NULL)
        _errormsg("duplicate plug list");
    current_spec.plugs = (List)(yyvsp[(3) - (4)]);
}
    break;

  case 35:
#line 239 "parse_tab.y"
    {
    makeScript(PM_LOG_IN, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 36:
#line 241 "parse_tab.y"
    {
    makeScript(PM_LOG_OUT, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 37:
#line 243 "parse_tab.y"
    {
    makeScript(PM_STATUS_PLUGS, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 38:
#line 245 "parse_tab.y"
    {
    makeScript(PM_STATUS_PLUGS_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 39:
#line 247 "parse_tab.y"
    {
    makeScript(PM_STATUS_NODES, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 40:
#line 249 "parse_tab.y"
    {
    makeScript(PM_STATUS_NODES_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 41:
#line 251 "parse_tab.y"
    {
    makeScript(PM_STATUS_TEMP, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 42:
#line 253 "parse_tab.y"
    {
    makeScript(PM_STATUS_TEMP_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 43:
#line 255 "parse_tab.y"
    {
    makeScript(PM_STATUS_BEACON, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 44:
#line 257 "parse_tab.y"
    {
    makeScript(PM_STATUS_BEACON_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 45:
#line 259 "parse_tab.y"
    {
    makeScript(PM_BEACON_ON, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 46:
#line 261 "parse_tab.y"
    {
    makeScript(PM_BEACON_OFF, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 47:
#line 263 "parse_tab.y"
    {
    makeScript(PM_POWER_ON, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 48:
#line 265 "parse_tab.y"
    {
    makeScript(PM_POWER_ON_RANGED, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 49:
#line 267 "parse_tab.y"
    {
    makeScript(PM_POWER_ON_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 50:
#line 269 "parse_tab.y"
    {
    makeScript(PM_POWER_OFF, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 51:
#line 271 "parse_tab.y"
    {
    makeScript(PM_POWER_OFF_RANGED, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 52:
#line 273 "parse_tab.y"
    {
    makeScript(PM_POWER_OFF_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 53:
#line 275 "parse_tab.y"
    {
    makeScript(PM_POWER_CYCLE, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 54:
#line 277 "parse_tab.y"
    {
    makeScript(PM_POWER_CYCLE_RANGED, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 55:
#line 279 "parse_tab.y"
    {
    makeScript(PM_POWER_CYCLE_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 56:
#line 281 "parse_tab.y"
    {
    makeScript(PM_RESET, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 57:
#line 283 "parse_tab.y"
    {
    makeScript(PM_RESET_RANGED, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 58:
#line 285 "parse_tab.y"
    {
    makeScript(PM_RESET_ALL, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 59:
#line 287 "parse_tab.y"
    {
    makeScript(PM_PING, (List)(yyvsp[(3) - (3)]));
}
    break;

  case 60:
#line 291 "parse_tab.y"
    {
    (yyval) = (yyvsp[(2) - (3)]);
}
    break;

  case 61:
#line 295 "parse_tab.y"
    { 
    list_append((List)(yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
    (yyval) = (yyvsp[(1) - (2)]); 
}
    break;

  case 62:
#line 298 "parse_tab.y"
    { 
    (yyval) = (char *)list_create((ListDelF)destroyPreStmt);
    list_append((List)(yyval), (yyvsp[(1) - (1)])); 
}
    break;

  case 63:
#line 303 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_EXPECT, (yyvsp[(2) - (2)]), NULL, NULL, NULL, NULL, NULL);
}
    break;

  case 64:
#line 305 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SEND, (yyvsp[(2) - (2)]), NULL, NULL, NULL, NULL, NULL);
}
    break;

  case 65:
#line 307 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_DELAY, NULL, (yyvsp[(2) - (2)]), NULL, NULL, NULL, NULL);
}
    break;

  case 66:
#line 309 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, (yyvsp[(2) - (3)]), NULL, NULL, (yyvsp[(3) - (3)]), NULL, NULL);
}
    break;

  case 67:
#line 311 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, (yyvsp[(2) - (4)]), NULL, NULL, (yyvsp[(3) - (4)]), NULL,
                             (List)(yyvsp[(4) - (4)]));
}
    break;

  case 68:
#line 314 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, (yyvsp[(2) - (3)]), (yyvsp[(3) - (3)]), NULL, NULL);
}
    break;

  case 69:
#line 316 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, (yyvsp[(2) - (4)]), (yyvsp[(3) - (4)]), NULL,(List)(yyvsp[(4) - (4)]));
}
    break;

  case 70:
#line 318 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, NULL, (yyvsp[(2) - (2)]), NULL, NULL);
}
    break;

  case 71:
#line 320 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_SETPLUGSTATE, NULL, NULL, NULL,(yyvsp[(2) - (3)]),NULL,(List)(yyvsp[(3) - (3)]));
}
    break;

  case 72:
#line 322 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_FOREACHNODE, NULL, NULL, NULL, NULL, 
                             (List)(yyvsp[(2) - (2)]), NULL);
}
    break;

  case 73:
#line 325 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_FOREACHPLUG, NULL, NULL, NULL, NULL, 
                             (List)(yyvsp[(2) - (2)]), NULL);
}
    break;

  case 74:
#line 328 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_IFOFF, NULL, NULL, NULL, NULL, 
                             (List)(yyvsp[(2) - (2)]), NULL);
}
    break;

  case 75:
#line 331 "parse_tab.y"
    {
    (yyval) = (char *)makePreStmt(STMT_IFON, NULL, NULL, NULL, NULL, 
                             (List)(yyvsp[(2) - (2)]), NULL);
}
    break;

  case 76:
#line 336 "parse_tab.y"
    { 
    list_append((List)(yyvsp[(1) - (2)]), (yyvsp[(2) - (2)])); 
    (yyval) = (yyvsp[(1) - (2)]); 
}
    break;

  case 77:
#line 339 "parse_tab.y"
    { 
    (yyval) = (char *)list_create((ListDelF)destroyInterp);
    list_append((List)(yyval), (yyvsp[(1) - (1)])); 
}
    break;

  case 78:
#line 344 "parse_tab.y"
    {
    (yyval) = (char *)makeInterp(ST_ON, (yyvsp[(3) - (3)]));
}
    break;

  case 79:
#line 346 "parse_tab.y"
    {
    (yyval) = (char *)makeInterp(ST_OFF, (yyvsp[(3) - (3)]));
}
    break;

  case 80:
#line 350 "parse_tab.y"
    {
    (yyval) = (yyvsp[(2) - (2)]);
}
    break;


/* Line 1267 of yacc.c.  */
#line 2098 "parse_tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 354 "parse_tab.y"


void scanner_init(char *filename);
void scanner_fini(void);
int scanner_line(void);
char *scanner_file(void);

/*
 * Entry point into the yacc/lex parser.
 */
int parse_config_file (char *filename)
{
    extern FILE *yyin; /* part of lexer */

    scanner_init(filename);

    yyin = fopen(filename, "r");
    if (!yyin)
        err_exit(TRUE, "%s", filename);

    device_specs = list_create((ListDelF) destroySpec);
    _clear_current_spec();

    yyparse();
    fclose(yyin);

    scanner_fini();

    list_destroy(device_specs);

    return 0;
} 

/* makePreStmt(type, str, tv, mp1(plug), mp2(stat/node), prestmts, interps */
static PreStmt *makePreStmt(StmtType type, char *str, char *tvstr, 
                      char *mp1str, char *mp2str, List prestmts, 
                      List interps)
{
    PreStmt *new;

    new = (PreStmt *) Malloc(sizeof(PreStmt));

    new->magic = PRESTMT_MAGIC;
    new->type = type;
    new->mp1 = mp1str ? _strtolong(mp1str) : -1;
    new->mp2 = mp2str ? _strtolong(mp2str) : -1;
    if (str)
        new->str = Strdup(str);
    if (tvstr)
        _doubletotv(&new->tv, _strtodouble(tvstr));
    new->prestmts = prestmts;
    new->interps = interps;

    return new;
}

static void destroyPreStmt(PreStmt *p)
{
    assert(p->magic == PRESTMT_MAGIC);
    p->magic = 0;
    if (p->str)
        Free(p->str);
    p->str = NULL;
    if (p->prestmts)
        list_destroy(p->prestmts);
    p->prestmts = NULL;
    if (p->interps)
        list_destroy(p->interps);
    p->interps = NULL;
    Free(p);
}

static void _clear_current_spec(void)
{
    int i;

    current_spec.name = NULL;
    current_spec.plugs = NULL;
    timerclear(&current_spec.timeout);
    timerclear(&current_spec.ping_period);
    for (i = 0; i < NUM_SCRIPTS; i++)
        current_spec.prescripts[i] = NULL;
}

static Spec *_copy_current_spec(void)
{
    Spec *new = (Spec *) Malloc(sizeof(Spec));
    int i;

    *new = current_spec;
    for (i = 0; i < NUM_SCRIPTS; i++)
        new->prescripts[i] = current_spec.prescripts[i];
    _clear_current_spec();
    return new;
}

static Spec *makeSpec(char *name)
{
    Spec *spec;

    current_spec.name = Strdup(name);

    /* FIXME: check for manditory scripts here?  what are they? */

    spec = _copy_current_spec();
    assert(device_specs != NULL);
    list_append(device_specs, spec);

    return spec;
}

static void destroySpec(Spec * spec)
{
    int i;

    if (spec->name)
        Free(spec->name);
    if (spec->plugs)
        list_destroy(spec->plugs);
    for (i = 0; i < NUM_SCRIPTS; i++)
        if (spec->prescripts[i])
            list_destroy(spec->prescripts[i]);
    Free(spec);
}

static int matchSpec(Spec * spec, void *key)
{
    return (strcmp(spec->name, (char *) key) == 0);
}

static Spec *findSpec(char *name)
{
    return list_find_first(device_specs, (ListFindF) matchSpec, name);
}

static void makeScript(int com, List stmts)
{
    if (current_spec.prescripts[com] != NULL)
        _errormsg("duplicate script");
    current_spec.prescripts[com] = stmts;
}

static Interp *makeInterp(InterpState state, char *str)
{
    Interp *new = (Interp *)Malloc(sizeof(Interp));

    new->magic = INTERP_MAGIC; 
    new->str = Strdup(str); 
    new->re = NULL;  /* defer compilation until copyInterpList */
    new->state = state;

    return new;
}

static void destroyInterp(Interp *i)
{
    assert(i->magic == INTERP_MAGIC);
    i->magic = 0;
    Free(i->str);
    if (i->re) {
        regfree(i->re);
        Free(i->re);
    }
    Free(i);
}

static List copyInterpList(List il)
{
    int cflags = REG_EXTENDED | REG_NOSUB;
    ListIterator itr;
    Interp *ip, *icpy;
    List new = list_create((ListDelF) destroyInterp);

    if (il != NULL) {

        itr = list_iterator_create(il);
        while((ip = list_next(itr))) {
            assert(ip->magic == INTERP_MAGIC);
            icpy = makeInterp(ip->state, ip->str);
            icpy->re = (regex_t *)Malloc(sizeof(regex_t));
            Regcomp(icpy->re, icpy->str, cflags);
            assert(icpy->magic == INTERP_MAGIC);
            list_append(new, icpy);
        }
        list_iterator_destroy(itr);
    }

    return new;
}

/**
 ** Powerman.conf stuff.
 **/

static void destroyStmt(Stmt *stmt)
{
    assert(stmt != NULL);

    switch (stmt->type) {
    case STMT_SEND:
        Free(stmt->u.send.fmt);
        break;
    case STMT_EXPECT:
        regfree(&stmt->u.expect.exp);
        break;
    case STMT_DELAY:
        break;
    case STMT_SETPLUGSTATE:
        list_destroy(stmt->u.setplugstate.interps);
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
    case STMT_IFON:
    case STMT_IFOFF:
        list_destroy(stmt->u.foreach.stmts);
        break;
    default:
        break;
    }
    Free(stmt);
}

static Stmt *makeStmt(PreStmt *p)
{
    Stmt *stmt;
    PreStmt *subp;
    int cflags = REG_EXTENDED;
    ListIterator itr;

    assert(p->magic == PRESTMT_MAGIC);
    stmt = (Stmt *) Malloc(sizeof(Stmt));
    stmt->type = p->type;
    switch (p->type) {
    case STMT_SEND:
        stmt->u.send.fmt = Strdup(p->str);
        break;
    case STMT_EXPECT:
        Regcomp(&stmt->u.expect.exp, p->str, cflags);
        break;
    case STMT_SETPLUGSTATE:
        stmt->u.setplugstate.stat_mp = p->mp2;
        if (p->str)
            stmt->u.setplugstate.plug_name = Strdup(p->str);
        else
            stmt->u.setplugstate.plug_mp = p->mp1;
        stmt->u.setplugstate.interps = copyInterpList(p->interps);
        break;
    case STMT_DELAY:
        stmt->u.delay.tv = p->tv;
        break;
    case STMT_FOREACHNODE:
    case STMT_FOREACHPLUG:
        stmt->u.foreach.stmts = list_create((ListDelF) destroyStmt);
        itr = list_iterator_create(p->prestmts);
        while((subp = list_next(itr))) {
            assert(subp->magic == PRESTMT_MAGIC);
            list_append(stmt->u.foreach.stmts, makeStmt(subp));
        }
        list_iterator_destroy(itr);
        break;
    case STMT_IFON:
    case STMT_IFOFF:
        stmt->u.ifonoff.stmts = list_create((ListDelF) destroyStmt);
        itr = list_iterator_create(p->prestmts);
        while((subp = list_next(itr))) {
            assert(subp->magic == PRESTMT_MAGIC);
            list_append(stmt->u.ifonoff.stmts, makeStmt(subp));
        }
        list_iterator_destroy(itr);
        break;
    default:
        break;
    }
    return stmt;
}

static void _parse_hoststr(Device *dev, char *hoststr, char *flagstr)
{
    /* pipe device, e.g. "conman -j baytech0 |&" */
    if (strstr(hoststr, "|&") != NULL) {
        dev->data           = pipe_create(hoststr, flagstr);
        dev->destroy        = pipe_destroy;
        dev->connect        = pipe_connect;
        dev->disconnect     = pipe_disconnect;
        dev->finish_connect = NULL;
        dev->preprocess     = NULL;

    /* serial device, e.g. "/dev/ttyS0" */
    } else if (hoststr[0] == '/') {
        struct stat sb;

        if (stat(hoststr, &sb) == -1 || (!(sb.st_mode & S_IFCHR))) 
            _errormsg("serial device not found or not a char special file");

        dev->data           = serial_create(hoststr, flagstr);
        dev->destroy        = serial_destroy;
        dev->connect        = serial_connect;
        dev->disconnect     = serial_disconnect;
        dev->finish_connect = NULL;
        dev->preprocess     = NULL;

    /* tcp device, e.g. "cyclades0:2001" */
    } else {
        char *port = strchr(hoststr, ':');
        int n;

        if (port) {                 /* host='host:port', flags=NULL */
            *port++ = '\0';
        } else
            _errormsg("hostname is missing :port");

        n = _strtolong(port);       /* verify port number */
        if (n < 1 || n > 65535)
            _errormsg("port number out of range");
        dev->data           = tcp_create(hoststr, port, flagstr);
        dev->destroy        = tcp_destroy;
        dev->connect        = tcp_connect;
        dev->disconnect     = tcp_disconnect;
        dev->finish_connect = tcp_finish_connect;
        dev->preprocess     = tcp_preprocess;
    }
}

static void makeDevice(char *devstr, char *specstr, char *hoststr, 
                        char *flagstr)
{
    ListIterator itr;
    Device *dev;
    Spec *spec;
    int i;

    /* find that spec */
    spec = findSpec(specstr);
    if ( spec == NULL ) 
        _errormsg("device specification not found");

    /* make the Device */
    dev = dev_create(devstr);
    dev->specname = Strdup(specstr);
    dev->timeout = spec->timeout;
    dev->ping_period = spec->ping_period;

    _parse_hoststr(dev, hoststr, flagstr);

    /* create plugs (spec->plugs may be NULL) */
    dev->plugs = pluglist_create(spec->plugs);

    /* transfer remaining info from the spec to the device */
    for (i = 0; i < NUM_SCRIPTS; i++) {
        PreStmt *p;

        if (spec->prescripts[i] == NULL) {
            dev->scripts[i] = NULL;
            continue; /* unimplemented script */
        }

        dev->scripts[i] = list_create((ListDelF) destroyStmt);

        /* copy the list of statements in each script */
        itr = list_iterator_create(spec->prescripts[i]);
        while((p = list_next(itr))) {
            list_append(dev->scripts[i], makeStmt(p));
        }
        list_iterator_destroy(itr);
    }

    dev_add(dev);
}

static void makeAlias(char *namestr, char *hostsstr)
{
    if (!conf_add_alias(namestr, hostsstr))
        _errormsg("bad alias");
}

static void makeNode(char *nodestr, char *devstr, char *plugstr)
{
    Device *dev = dev_findbyname(devstr);

    if (dev == NULL) 
        _errormsg("unknown device");

    /* plugstr can be NULL - see comment in pluglist.h */
    switch (pluglist_map(dev->plugs, nodestr, plugstr)) {
        case EPL_DUPNODE:
            _errormsg("duplicate node");
        case EPL_UNKPLUG:
            _errormsg("unknown plug name");
        case EPL_DUPPLUG:
            _errormsg("plug already assigned");
        case EPL_NOPLUGS:
            _errormsg("more nodes than plugs");
        case EPL_NONODES:
            _errormsg("more plugs than nodes");
        default:
            break;
    }

    if (!conf_addnodes(nodestr))
        _errormsg("duplicate node name");
}

static void makeClientPort(char *s2)
{
    int port = _strtolong(s2);

    if (port < 1024)
        _errormsg("bogus client listener port");
    conf_set_listen_port(port); 
}

/**
 ** Utility functions
 **/

static double _strtodouble(char *str)
{
    char *endptr;
    double val = strtod(str, &endptr);

    if (val == 0.0 && endptr == str)
        _errormsg("error parsing double value");
    if ((val == HUGE_VAL || val == -HUGE_VAL) && errno == ERANGE)
        _errormsg("double value would cause overflow");
    return val;
}

static long _strtolong(char *str)
{
    char *endptr;
    long val = strtol(str, &endptr, 0);

    if (val == 0 && endptr == str)
        _errormsg("error parsing long integer value");
    if ((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)
        _errormsg("long integer value would cause under/overflow");
    return val;
}

static void _doubletotv(struct timeval *tv, double val)
{
    tv->tv_sec = (val * 10.0)/10; /* crude round-down without -lm */
    tv->tv_usec = ((val - tv->tv_sec) * 1000000.0);
}

static void _errormsg(char *msg)
{
    err_exit(FALSE, "%s: %s::%d", msg, scanner_file(), scanner_line());
}

static void _warnmsg(char *msg)
{
    err(FALSE, "warning: %s: %s::%d", msg, scanner_file(), scanner_line());
}

void yyerror()
{
    _errormsg("parse error");
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */

