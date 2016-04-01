//#ifdef USE_SQLEXER

#include "sqpcheader.h"
#include "sqobject.h"
#include "sqcompiler.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqlexer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  /* for malloc */
#include <assert.h>  /* for a few sanity tests */

static const SQChar SQ_LIBNAME[] = _SC("SQLexer");

SQ_OPT_STRING_STRLEN();


struct sq_lexer_st
{
    SQLexer *lex;
    HSQOBJECT source;
    SQStrBufState buf;
    HSQUIRRELVM vm;
};

static const SQChar SQLEXER_Tag[]   = _SC("sq_SQLexer_ctx");
#define GET_SQLexer_INSTANCE() SQ_GET_INSTANCE(v, 1, sq_lexer_st, SQLEXER_Tag) \
	if(self == NULL) return sq_throwerror(v, _SC("SQLexer object already closed"));


static SQRESULT SQLexer_release_hook(SQUserPointer p, SQInteger size, void */*ep*/)
{
	sq_lexer_st *self = (sq_lexer_st*)p;
	if(self && self->lex)
    {
        sq_release(self->vm, &self->source);
        self->lex->~SQLexer();
        sq_free(self->lex, sizeof(SQLexer));
        self->lex = NULL;
        sq_free(self, sizeof(sq_lexer_st));
    }
	return 0;
}

static SQRESULT sq_SQLexer_reset_src(HSQUIRRELVM v, sq_lexer_st *self){
    SQ_FUNC_VARS_NO_TOP(v);
	SQ_GET_STRING(v, 2, src);

    sq_release(v, &self->source);
    sq_resetobject(&self->source);
    SQRESULT rc = sq_getstackobj(v, 2, &self->source);
    if(rc == SQ_OK)
    {
        sq_addref(v, &self->source);
        self->buf.buf = src;
        self->buf.ptr = 0;
        self->buf.size = src_size;
    }
	return 1;
}

static SQRESULT sq_SQLexer_constructor(HSQUIRRELVM v){

	sq_lexer_st *self = (sq_lexer_st*)sq_malloc(sizeof(sq_lexer_st));//sq_newuserdata(v, sizeof(sq_lexer_st));
	memset(self, 0, sizeof(*self));

    sq_SQLexer_reset_src(v, self);
    self->lex = (SQLexer*)sq_malloc(sizeof(SQLexer));
    new (self->lex) SQLexer();
    self->vm = v;

    SQBool want_comments = SQFalse;
    if(sq_gettop(v) > 2)
    {
        //we want comments returned by the lexer
        sq_getbool(v, 3, &want_comments);
    }

    self->lex->Init(v->_sharedstate, sq_strbuf_lexfeed, &self->buf, NULL, NULL, want_comments);

    sq_setinstanceup(v, 1, self);
    sq_setreleasehook(v,1, SQLexer_release_hook);

	return 1;
}

static SQRESULT sq_SQLexer_reset(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

	sq_SQLexer_reset_src(v, self);
    self->lex->ResetReader(sq_strbuf_lexfeed, &self->buf, 1);
	return 1;
}

static SQRESULT sq_SQLexer_lasterror(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushstring(v, self->lex->_lasterror, -1);
	return 1;
}

static SQRESULT sq_SQLexer_longstr(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushstring(v, &self->lex->_longstr[0], self->lex->_longstr.size());
	return 1;
}

static SQRESULT sq_SQLexer_tok2str(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();
	SQ_GET_INTEGER(v, 2, tok);

    const SQChar *tk_name = self->lex->Tok2Str(tok);
    if(tk_name) sq_pushstring(v, tk_name, -1);
    else sq_pushnull(v);
	return 1;
}

static SQRESULT sq_SQLexer_token_name(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();
	SQ_GET_INTEGER(v, 2, tok);

    const SQChar *tk_name = self->lex->GetTokenName(tok);
    if(tk_name) sq_pushstring(v, tk_name, -1);
    else sq_pushnull(v);
	return 1;
}

static SQRESULT sq_SQLexer_svalue(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    if(self->lex->_svalue) sq_pushstring(v, self->lex->_svalue, -1);
    else sq_pushnull(v);
	return 1;
}

static SQRESULT sq_SQLexer_nvalue(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->_nvalue);
	return 1;
}

static SQRESULT sq_SQLexer_fvalue(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushfloat(v, self->lex->_fvalue);
	return 1;
}

static SQRESULT sq_SQLexer_prevtoken(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->_prevtoken);
	return 1;
}

static SQRESULT sq_SQLexer_lasttokenline(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->_lasttokenline);
	return 1;
}

static SQRESULT sq_SQLexer_currentline(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->_currentline);
	return 1;
}

static SQRESULT sq_SQLexer_currentcolumn(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->_currentcolumn);
	return 1;
}

static SQRESULT sq_SQLexer_lex(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, self->lex->Lex());
	return 1;
}

static SQRESULT sq_SQLexer_first_enum_token(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, TK_FIRST_ENUM_TOKEN);
	return 1;
}

static SQRESULT sq_SQLexer_last_enum_token(HSQUIRRELVM v){
	SQ_FUNC_VARS_NO_TOP(v);
	GET_SQLexer_INSTANCE();

    sq_pushinteger(v, TK_LAST_ENUM_TOKEN);
	return 1;
}

#define _DECL_SQLEXER_FUNC(name,nparams,pmask) {_SC(#name),sq_SQLexer_##name,nparams,pmask}
static SQRegFunction SQLexer_obj_funcs[]={

	_DECL_SQLEXER_FUNC(constructor, -2, _SC(".sb")),
	_DECL_SQLEXER_FUNC(reset, 2, _SC(".s")),
	_DECL_SQLEXER_FUNC(tok2str, 2, _SC(".i")),
	_DECL_SQLEXER_FUNC(token_name, 2, _SC(".i")),
	_DECL_SQLEXER_FUNC(lasterror, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(longstr, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(svalue, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(nvalue, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(fvalue, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(currentline, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(currentcolumn, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(prevtoken, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(lasttokenline, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(first_enum_token, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(last_enum_token, 1, _SC(".")),
	_DECL_SQLEXER_FUNC(lex, 1, _SC(".")),
	{0,0}
};
#undef _DECL_SQLEXER_FUNC

typedef struct {
  const SQChar *Str;
  SQInteger Val;
} KeyIntType, * KeyIntPtrType;

static KeyIntType SQLexer_constants[] = {
    #define MK_CONST(c) {_SC(#c), c}
    //MK_CONST(SSL_SESSION_ID_SIZE),

    {0,0}
};

#ifdef __cplusplus
extern "C" {
#endif

/* This defines a function that opens up your library. */
SQRESULT sqext_register_SQLexer (HSQUIRRELVM v) {
	//add a namespace SQLexer
	sq_pushstring(v, SQ_LIBNAME, -1);
    sq_newclass(v,SQFalse);
    sq_settypetag(v,-1,(SQUserPointer)SQLEXER_Tag);
	sq_insert_reg_funcs(v, SQLexer_obj_funcs);

	//add constants
	KeyIntPtrType KeyIntPtr;
	for (KeyIntPtr = SQLexer_constants; KeyIntPtr->Str; KeyIntPtr++) {
		sq_pushstring(v, KeyIntPtr->Str, -1);    //first the key
		sq_pushinteger(v, KeyIntPtr->Val);       //then the value
		sq_newslot(v, -3, SQFalse);              //store then
	}

	sq_newslot(v,-3,SQFalse); //add SQLexer table to the root table

	return SQ_OK;
}

#ifdef __cplusplus
}

//#endif //USE_SQLEXER

#endif

