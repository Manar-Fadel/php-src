/* Generated by re2c 0.5 on Tue Jan  9 17:13:20 2001 */
#line 1 "/home/sas/src/php4/ext/standard/url_scanner_ex.re"
/*
  +----------------------------------------------------------------------+
  | PHP version 4.0                                                      |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997, 1998, 1999, 2000 The PHP Group                   |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Sascha Schumann <sascha@schumann.cx>                        |
  +----------------------------------------------------------------------+
*/

#include "php.h"

#ifdef TRANS_SID

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "php_ini.h"
#include "php_globals.h"
#define STATE_TAG SOME_OTHER_STATE_TAG
#include "basic_functions.h"
#undef STATE_TAG

#define url_adapt_ext url_adapt_ext_ex
#define url_scanner url_scanner_ex

#include "php_smart_str.h"

static PHP_INI_MH(OnUpdateTags)
{
	url_adapt_state_ex_t *ctx;
	char *key;
	char *lasts;
	char *tmp;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);
	
	tmp = estrndup(new_value, new_value_length);
	
	if (ctx->tags)
		zend_hash_destroy(ctx->tags);
	else
		ctx->tags = malloc(sizeof(HashTable));
	
	zend_hash_init(ctx->tags, 0, NULL, NULL, 1);
	
	for (key = php_strtok_r(tmp, ",", &lasts);
			key;
			key = php_strtok_r(NULL, ",", &lasts)) {
		char *val;

		val = strchr(key, '=');
		if (val) {
			char *q;
			int keylen;
			
			*val++ = '\0';
			for (q = key; *q; q++)
				*q = tolower(*q);
			keylen = q - key;
			/* key is stored withOUT NUL
			   val is stored WITH    NUL */
			zend_hash_add(ctx->tags, key, keylen, val, strlen(val)+1, NULL);
		}
	}

	efree(tmp);

	return SUCCESS;
}

PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("url_rewriter.tags", "a=href,area=href,frame=src,form=fakeentry", PHP_INI_ALL, OnUpdateTags, url_adapt_state_ex, php_basic_globals, basic_globals)
PHP_INI_END()

static inline void append_modified_url(smart_str *url, smart_str *dest, smart_str *name, smart_str *val, const char *separator)
{
	register const char *p, *q;
	const char *bash = NULL;
	const char *sep = "?";
	
	q = url->c + url->len;
	
	for (p = url->c; p < q; p++) {
		switch(*p) {
			case ':':
				smart_str_append(dest, url);
				return;
			case '?':
				sep = separator;
				break;
			case '#':
				bash = p;
				break;
		}
	}

	/* Don't modify URLs of the format "#mark" */
	if (bash - url->c == 0) {
		smart_str_append(dest, url);
		return;
	}

	if (bash)
		smart_str_appendl(dest, url->c, bash - url->c);
	else
		smart_str_append(dest, url);

	smart_str_appends(dest, sep);
	smart_str_append(dest, name);
	smart_str_appendc(dest, '=');
	smart_str_append(dest, val);

	if (bash)
		smart_str_appendl(dest, bash, q - bash);
}

static inline void tag_arg(url_adapt_state_ex_t *ctx, char quote PLS_DC)
{
	char f = 0;

	if (strncasecmp(ctx->arg.c, ctx->lookup_data, ctx->arg.len) == 0)
		f = 1;

	smart_str_appendc(&ctx->result, quote);
	if (f) {
		append_modified_url(&ctx->val, &ctx->result, &ctx->q_name, &ctx->q_value, PG(arg_separator));
	} else {
		smart_str_append(&ctx->result, &ctx->val);
	}
	smart_str_appendc(&ctx->result, quote);
}

enum {
	STATE_PLAIN,
	STATE_TAG,
	STATE_NEXT_ARG,
	STATE_ARG,
	STATE_BEFORE_VAL,
	STATE_VAL
};

#define YYFILL(n) goto stop
#define YYCTYPE unsigned char
#define YYCURSOR xp
#define YYLIMIT end
#define YYMARKER q
#define STATE ctx->state

#define STD_PARA url_adapt_state_ex_t *ctx, char *start, char *YYCURSOR PLS_DC
#define STD_ARGS ctx, start, xp PLS_CC

static inline void passthru(STD_PARA) 
{
	smart_str_appendl(&ctx->result, start, YYCURSOR - start);
}

static inline void handle_form(STD_PARA) 
{
	if (ctx->tag.len == 4 && strncasecmp(ctx->tag.c, "form", 4) == 0) {
		smart_str_appends(&ctx->result, "<input type=\"hidden\" name=\""); 
		smart_str_append(&ctx->result, &ctx->q_name);
		smart_str_appends(&ctx->result, "\" value=\"");
		smart_str_append(&ctx->result, &ctx->q_value);
		smart_str_appends(&ctx->result, "\" />");
	}
}

/*
 *  HANDLE_TAG copies the HTML Tag and checks whether we 
 *  have that tag in our table. If we might modify it,
 *  we continue to scan the tag, otherwise we simply copy the complete
 *  HTML stuff to the result buffer.
 */

static inline void handle_tag(STD_PARA) 
{
	int ok = 0;
	int i;

	ctx->tag.len = 0;
	smart_str_appendl(&ctx->tag, start, YYCURSOR - start);
	for (i = 0; i < ctx->tag.len; i++)
		ctx->tag.c[i] = tolower(ctx->tag.c[i]);
	if (zend_hash_find(ctx->tags, ctx->tag.c, ctx->tag.len, (void **) &ctx->lookup_data) == SUCCESS)
		ok = 1;
	STATE = ok ? STATE_NEXT_ARG : STATE_PLAIN;
}

static inline void handle_arg(STD_PARA) 
{
	ctx->arg.len = 0;
	smart_str_appendl(&ctx->arg, start, YYCURSOR - start);
}

static inline void handle_val(STD_PARA, char quotes, char type) 
{
	smart_str_setl(&ctx->val, start + quotes, YYCURSOR - start - quotes * 2);
	tag_arg(ctx, type PLS_CC);
}

#ifdef SCANNER_DEBUG
#define scdebug(x) printf x
#else
#define scdebug(x)
#endif

static inline void mainloop(url_adapt_state_ex_t *ctx, const char *newdata, size_t newlen)
{
	char *end, *q;
	char *xp;
	char *start;
	int rest;
	PLS_FETCH();

	smart_str_appendl(&ctx->buf, newdata, newlen);
	
	YYCURSOR = ctx->buf.c;
	YYLIMIT = ctx->buf.c + ctx->buf.len;

#line 238

	
	while(1) {
		start = YYCURSOR;
		scdebug(("state %d at %s\n", STATE, YYCURSOR));
	switch(STATE) {
		
		case STATE_PLAIN:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if(yych != '<')	goto yy4;
yy2:	yych = *++YYCURSOR;
yy3:
#line 247
	{ passthru(STD_ARGS); STATE = STATE_TAG; continue; }
yy4:	yych = *++YYCURSOR;
yy5:
#line 248
	{ passthru(STD_ARGS); continue; }
}
#line 249

			break;
			
		case STATE_TAG:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy6;
yy7:	++YYCURSOR;
yy6:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych <= '@')	goto yy10;
	if(yych <= 'Z')	goto yy8;
	if(yych <= '`')	goto yy10;
	if(yych >= '{')	goto yy10;
yy8:	yych = *++YYCURSOR;
	goto yy13;
yy9:
#line 254
	{ handle_tag(STD_ARGS); /* Sets STATE */; passthru(STD_ARGS); continue; }
yy10:	yych = *++YYCURSOR;
yy11:
#line 255
	{ passthru(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy12:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy13:	if(yybm[0+yych] & 128)	goto yy12;
	goto yy9;
}
#line 256

  			break;
			
		case STATE_NEXT_ARG:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	goto yy14;
yy15:	++YYCURSOR;
yy14:
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if(yych <= '='){
		if(yych <= '\n'){
			if(yych <= '\t')	goto yy22;
			goto yy18;
		} else {
			if(yych == ' ')	goto yy18;
			goto yy22;
		}
	} else {
		if(yych <= 'Z'){
			if(yych <= '>')	goto yy16;
			if(yych <= '@')	goto yy22;
			goto yy20;
		} else {
			if(yych <= '`')	goto yy22;
			if(yych <= 'z')	goto yy20;
			goto yy22;
		}
	}
yy16:	yych = *++YYCURSOR;
yy17:
#line 261
	{ passthru(STD_ARGS); handle_form(STD_ARGS); STATE = STATE_PLAIN; continue; }
yy18:	yych = *++YYCURSOR;
yy19:
#line 262
	{ passthru(STD_ARGS); continue; }
yy20:	yych = *++YYCURSOR;
yy21:
#line 263
	{ YYCURSOR--; STATE = STATE_ARG; continue; }
yy22:	yych = *++YYCURSOR;
yy23:
#line 264
	{ passthru(STD_ARGS); STATE = STATE_PLAIN; continue; }
}
#line 265

 	 		break;

		case STATE_ARG:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128, 128,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy24;
yy25:	++YYCURSOR;
yy24:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych <= '@')	goto yy28;
	if(yych <= 'Z')	goto yy26;
	if(yych <= '`')	goto yy28;
	if(yych >= '{')	goto yy28;
yy26:	yych = *++YYCURSOR;
	goto yy31;
yy27:
#line 270
	{ passthru(STD_ARGS); handle_arg(STD_ARGS); STATE = STATE_BEFORE_VAL; continue; }
yy28:	yych = *++YYCURSOR;
yy29:
#line 271
	{ passthru(STD_ARGS); STATE = STATE_NEXT_ARG; continue; }
yy30:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy31:	if(yybm[0+yych] & 128)	goto yy30;
	goto yy27;
}
#line 272


		case STATE_BEFORE_VAL:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	128,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy32;
yy33:	++YYCURSOR;
yy32:
	if((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	if(yych == ' ')	goto yy34;
	if(yych == '=')	goto yy36;
	goto yy38;
yy34:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ' ')	goto yy41;
	if(yych == '=')	goto yy39;
yy35:
#line 277
	{ YYCURSOR--; STATE = STATE_NEXT_ARG; continue; }
yy36:	yych = *++YYCURSOR;
	goto yy40;
yy37:
#line 276
	{ passthru(STD_ARGS); STATE = STATE_VAL; continue; }
yy38:	yych = *++YYCURSOR;
	goto yy35;
yy39:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy40:	if(yybm[0+yych] & 128)	goto yy39;
	goto yy37;
yy41:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy42:	if(yych == ' ')	goto yy41;
	if(yych == '=')	goto yy39;
yy43:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy35;
	}
}
#line 278

			break;

		case STATE_VAL:
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 192, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	192, 240,  64, 240, 240, 240, 240, 144, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240,   0, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	240, 240, 240, 240, 240, 240, 240, 240, 
	};
	goto yy44;
yy45:	++YYCURSOR;
yy44:
	if((YYLIMIT - YYCURSOR) < 3) YYFILL(3);
	yych = *YYCURSOR;
	if(yych <= '!'){
		if(yych <= '\n'){
			if(yych <= '\t')	goto yy50;
			goto yy51;
		} else {
			if(yych == ' ')	goto yy51;
			goto yy50;
		}
	} else {
		if(yych <= '\''){
			if(yych <= '"')	goto yy46;
			if(yych <= '&')	goto yy50;
			goto yy48;
		} else {
			if(yych == '>')	goto yy51;
			goto yy50;
		}
	}
yy46:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych != '>')	goto yy63;
yy47:
#line 286
	{ passthru(STD_ARGS); STATE = STATE_NEXT_ARG; continue; }
yy48:	yyaccept = 1;
	yych = *(YYMARKER = ++YYCURSOR);
	goto yy55;
yy49:
#line 285
	{ handle_val(STD_ARGS, 0, '"');  STATE = STATE_NEXT_ARG; continue; }
yy50:	yych = *++YYCURSOR;
	goto yy53;
yy51:	yych = *++YYCURSOR;
	goto yy47;
yy52:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy53:	if(yybm[0+yych] & 16)	goto yy52;
	goto yy49;
yy54:	yyaccept = 1;
	YYMARKER = ++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy55:	if(yybm[0+yych] & 32)	goto yy54;
	if(yych <= '&')	goto yy58;
	if(yych >= '(')	goto yy49;
yy56:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 16)	goto yy52;
yy57:
#line 284
	{ handle_val(STD_ARGS, 1, '\''); STATE = STATE_NEXT_ARG; continue; }
yy58:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy59:	if(yybm[0+yych] & 64)	goto yy58;
	if(yych <= '=')	goto yy61;
yy60:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 1:	goto yy49;
	case 0:	goto yy47;
	}
yy61:	yych = *++YYCURSOR;
	goto yy57;
yy62:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy63:	if(yybm[0+yych] & 128)	goto yy62;
	if(yych >= '>')	goto yy60;
yy64:	yych = *++YYCURSOR;
yy65:
#line 283
	{ handle_val(STD_ARGS, 1, '"');  STATE = STATE_NEXT_ARG; continue; }
}
#line 287

			break;
	}
	}

stop:
	scdebug(("stopped in state %d at pos %d (%d:%c)\n", STATE, YYCURSOR - ctx->buf.c, *YYCURSOR, *YYCURSOR));

	rest = YYLIMIT - start;

	/* XXX: Crash avoidance. Need to work with reporter to figure out what goes wrong */	
	if (rest < 0) rest = 0;
	
	if (rest) memmove(ctx->buf.c, start, rest);
	ctx->buf.len = rest;
}

char *url_adapt_single_url(const char *url, size_t urllen, const char *name, const char *value, size_t *newlen)
{
	smart_str surl = {0};
	smart_str buf = {0};
	smart_str sname = {0};
	smart_str sval = {0};
	PLS_FETCH();

	smart_str_setl(&surl, url, urllen);
	smart_str_sets(&sname, name);
	smart_str_sets(&sval, value);

	append_modified_url(&surl, &buf, &sname, &sval, PG(arg_separator));

	smart_str_0(&buf);
	if (newlen) *newlen = buf.len;
	
	return buf.c;
}

char *url_adapt_ext(const char *src, size_t srclen, const char *name, const char *value, size_t *newlen)
{
	char *ret;
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();

	ctx = &BG(url_adapt_state_ex);

	smart_str_sets(&ctx->q_name, name);
	smart_str_sets(&ctx->q_value, value);
	mainloop(ctx, src, srclen);

	*newlen = ctx->result.len;
	smart_str_0(&ctx->result);
	ctx->result.len = 0;
	return ctx->result.c;
}

PHP_RINIT_FUNCTION(url_scanner)
{
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);

	memset(ctx, 0, ((size_t) &((url_adapt_state_ex_t *)0)->tags));

	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(url_scanner)
{
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);

	smart_str_free(&ctx->result);
	smart_str_free(&ctx->buf);
	smart_str_free(&ctx->tag);
	smart_str_free(&ctx->arg);

	return SUCCESS;
}

PHP_MINIT_FUNCTION(url_scanner)
{
	url_adapt_state_ex_t *ctx;
	BLS_FETCH();
	
	ctx = &BG(url_adapt_state_ex);

	ctx->tags = NULL;
	
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(url_scanner)
{
	BLS_FETCH();

	UNREGISTER_INI_ENTRIES();
	zend_hash_destroy(BG(url_adapt_state_ex).tags);
	free(BG(url_adapt_state_ex).tags);
	return SUCCESS;
}

#endif
