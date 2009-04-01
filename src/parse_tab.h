/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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
     TOK_BEACON_OFF = 267,
     TOK_ON = 268,
     TOK_ON_RANGED = 269,
     TOK_ON_ALL = 270,
     TOK_OFF = 271,
     TOK_OFF_RANGED = 272,
     TOK_OFF_ALL = 273,
     TOK_CYCLE = 274,
     TOK_CYCLE_RANGED = 275,
     TOK_CYCLE_ALL = 276,
     TOK_RESET = 277,
     TOK_RESET_RANGED = 278,
     TOK_RESET_ALL = 279,
     TOK_PING = 280,
     TOK_SPEC = 281,
     TOK_EXPECT = 282,
     TOK_SETPLUGSTATE = 283,
     TOK_SEND = 284,
     TOK_DELAY = 285,
     TOK_FOREACHPLUG = 286,
     TOK_FOREACHNODE = 287,
     TOK_IFOFF = 288,
     TOK_IFON = 289,
     TOK_OFF_STRING = 290,
     TOK_ON_STRING = 291,
     TOK_MAX_PLUG_COUNT = 292,
     TOK_TIMEOUT = 293,
     TOK_DEV_TIMEOUT = 294,
     TOK_PING_PERIOD = 295,
     TOK_PLUG_NAME = 296,
     TOK_SCRIPT = 297,
     TOK_DEVICE = 298,
     TOK_NODE = 299,
     TOK_ALIAS = 300,
     TOK_TCP_WRAPPERS = 301,
     TOK_LISTEN = 302,
     TOK_MATCHPOS = 303,
     TOK_STRING_VAL = 304,
     TOK_NUMERIC_VAL = 305,
     TOK_YES = 306,
     TOK_NO = 307,
     TOK_BEGIN = 308,
     TOK_END = 309,
     TOK_UNRECOGNIZED = 310,
     TOK_EQUALS = 311
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
#define TOK_BEACON_OFF 267
#define TOK_ON 268
#define TOK_ON_RANGED 269
#define TOK_ON_ALL 270
#define TOK_OFF 271
#define TOK_OFF_RANGED 272
#define TOK_OFF_ALL 273
#define TOK_CYCLE 274
#define TOK_CYCLE_RANGED 275
#define TOK_CYCLE_ALL 276
#define TOK_RESET 277
#define TOK_RESET_RANGED 278
#define TOK_RESET_ALL 279
#define TOK_PING 280
#define TOK_SPEC 281
#define TOK_EXPECT 282
#define TOK_SETPLUGSTATE 283
#define TOK_SEND 284
#define TOK_DELAY 285
#define TOK_FOREACHPLUG 286
#define TOK_FOREACHNODE 287
#define TOK_IFOFF 288
#define TOK_IFON 289
#define TOK_OFF_STRING 290
#define TOK_ON_STRING 291
#define TOK_MAX_PLUG_COUNT 292
#define TOK_TIMEOUT 293
#define TOK_DEV_TIMEOUT 294
#define TOK_PING_PERIOD 295
#define TOK_PLUG_NAME 296
#define TOK_SCRIPT 297
#define TOK_DEVICE 298
#define TOK_NODE 299
#define TOK_ALIAS 300
#define TOK_TCP_WRAPPERS 301
#define TOK_LISTEN 302
#define TOK_MATCHPOS 303
#define TOK_STRING_VAL 304
#define TOK_NUMERIC_VAL 305
#define TOK_YES 306
#define TOK_NO 307
#define TOK_BEGIN 308
#define TOK_END 309
#define TOK_UNRECOGNIZED 310
#define TOK_EQUALS 311




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

