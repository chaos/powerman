
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     TOK_STATUS_TEMP = 262,
     TOK_STATUS_TEMP_ALL = 263,
     TOK_STATUS_BEACON = 264,
     TOK_STATUS_BEACON_ALL = 265,
     TOK_BEACON_ON = 266,
     TOK_BEACON_ON_RANGED = 267,
     TOK_BEACON_OFF = 268,
     TOK_BEACON_OFF_RANGED = 269,
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
     TOK_LISTEN = 304,
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
#define TOK_STATUS_TEMP 262
#define TOK_STATUS_TEMP_ALL 263
#define TOK_STATUS_BEACON 264
#define TOK_STATUS_BEACON_ALL 265
#define TOK_BEACON_ON 266
#define TOK_BEACON_ON_RANGED 267
#define TOK_BEACON_OFF 268
#define TOK_BEACON_OFF_RANGED 269
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
#define TOK_LISTEN 304
#define TOK_MATCHPOS 305
#define TOK_STRING_VAL 306
#define TOK_NUMERIC_VAL 307
#define TOK_YES 308
#define TOK_NO 309
#define TOK_BEGIN 310
#define TOK_END 311
#define TOK_UNRECOGNIZED 312
#define TOK_EQUALS 313




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


