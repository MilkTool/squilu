/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqclass.h"
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

SQ_OPT_STRING_STRLEN();

static bool str2num(const SQChar *s,SQObjectPtr &res, SQInteger base=10)
{
	SQChar *end;
	const SQChar *e = s;
	SQBool isfloat = SQFalse;
	SQBool isEIntBase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
	SQChar c;
	while((c = *e) != _SC('\0'))
	{
		if(c == _SC('.') || (!isEIntBase && (c == _SC('E')|| c == _SC('e')))) { //e and E is for scientific notation
			isfloat = SQTrue;
			break;
		}
		e++;
	}
	if(isfloat){
		SQFloat r = SQFloat(scstrtod(s,&end));
		if(s == end) return false;
		res = r;
	}
	else{
		SQInteger r = SQInteger(scstrtol(s,&end,(int)base));
		if(s == end) return false;
		res = r;
	}
	return true;
}

static SQRESULT base_dummy(HSQUIRRELVM SQ_UNUSED_ARG(v))
{
	return 0;
}

#ifndef NO_GARBAGE_COLLECTOR
static SQRESULT base_collectgarbage(HSQUIRRELVM v)
{
	sq_pushinteger(v, sq_collectgarbage(v));
	return 1;
}
static SQRESULT base_resurectureachable(HSQUIRRELVM v)
{
	sq_resurrectunreachable(v);
	return 1;
}
static SQRESULT base_getrefcount(HSQUIRRELVM v)
{
    SQObjectPtr &o=stack_get(v,2);
    if(!ISREFCOUNTED(sq_type(o))) sq_pushinteger(v,0);
    else sq_pushinteger(v, o._unVal.pRefCounted->_uiRef - 1);
	return 1;
}
#ifdef SQ_WITH_DELAYED_RELEASE_HOOKS
static SQRESULT base_check_delayed_release_hooks(HSQUIRRELVM v)
{
	SQ_FUNC_VARS(v);
	if(_top_ > 1){
		SQ_GET_BOOL(v, 2, bval);
		v->_check_delayed_relase_hooks = bval;
		return 0;
	}
    sq_pushbool(v, v->_check_delayed_relase_hooks);
	return 1;
}
static SQRESULT base_call_delayed_release_hooks(HSQUIRRELVM v)
{
    v->_sharedstate->CallDelayedReleaseHooks(v);
	return 0;
}
#endif // SQ_WITH_DELAYED_RELEASE_HOOKS
#endif

static SQRESULT base_getroottable(HSQUIRRELVM v)
{
	v->Push(v->_roottable);
	return 1;
}

static SQRESULT base_getconsttable(HSQUIRRELVM v)
{
	v->Push(_ss(v)->_consts);
	return 1;
}


static SQRESULT base_setroottable(HSQUIRRELVM v)
{
	SQObjectPtr o = v->_roottable;
	if(SQ_FAILED(sq_setroottable(v))) return SQ_ERROR;
	v->Push(o);
	return 1;
}

static SQRESULT base_setconsttable(HSQUIRRELVM v)
{
	SQObjectPtr o = _ss(v)->_consts;
	if(SQ_FAILED(sq_setconsttable(v))) return SQ_ERROR;
	v->Push(o);
	return 1;
}

static SQRESULT base_seterrorhandler(HSQUIRRELVM v)
{
	sq_seterrorhandler(v);
	return 0;
}

static SQRESULT base_geterrorhandler(HSQUIRRELVM v)
{
	return sq_geterrorhandler(v);
}

static SQRESULT base_setatexithandler(HSQUIRRELVM v)
{
	sq_setatexithandler(v);
	return 0;
}

static SQRESULT base_getatexithandler(HSQUIRRELVM v)
{
	return sq_getatexithandler(v);
}

static SQRESULT base_setdebughook(HSQUIRRELVM v)
{
	sq_setdebughook(v);
	return 0;
}

static SQRESULT base_enabledebuginfo(HSQUIRRELVM v)
{
	SQObjectPtr &o=stack_get(v,2);

	sq_enabledebuginfo(v,SQVM::IsFalse(o)?SQFalse:SQTrue);
	return 0;
}

static SQRESULT __getcallstackinfos(HSQUIRRELVM v,SQInteger level)
{
	SQStackInfos si;
	SQInteger seq = 0;
	const SQChar *name = NULL;

	if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
	{
		const SQChar *fn = _SC("unknown");
		const SQChar *src = _SC("unknown");
		if(si.funcname)fn = si.funcname;
		if(si.source)src = si.source;
		sq_newtable(v);
		sq_pushstring(v, _SC("func"), -1);
		sq_pushstring(v, fn, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("src"), -1);
		sq_pushstring(v, src, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("line"), -1);
		sq_pushinteger(v, si.line);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("locals"), -1);
		sq_newtable(v);
		seq=0;
		while ((name = sq_getlocal(v, level, seq))) {
			sq_pushstring(v, name, -1);
			sq_push(v, -2);
			sq_newslot(v, -4, SQFalse);
			sq_pop(v, 1);
			seq++;
		}
		sq_newslot(v, -3, SQFalse);
		return 1;
	}

	return 0;
}
static SQRESULT base_getstackinfos(HSQUIRRELVM v)
{
	SQInteger level;
	sq_getinteger(v, -1, &level);
	return __getcallstackinfos(v,level);
}

static SQRESULT base_getstacktop(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_getfulltop(v));
	return 1;
}

static SQRESULT base_gettypetag(HSQUIRRELVM v)
{
    SQUserPointer bklass = 0;
    if(sq_gettypetag(v, 2, &bklass) == SQ_OK && bklass){
        //lets see if it can be a string
        const SQChar *tag = (const SQChar*)bklass;
        int i=0;
        for(; i<32; ++i){
            if(!tag[i]) break;
        }
        if(i > 0 && i < 32) sq_pushstring(v, tag, i);
        else sq_pushuserpointer(v, bklass);
    }
    else sq_pushnull(v);
	return 1;
}

static SQRESULT base_getdefaultdelegate(HSQUIRRELVM v)
{
    if(sq_getdefaultdelegate(v, sq_gettype(v, 2)) != SQ_OK)
    {
        sq_pushnull(v);
    }
	return 1;
}

static SQRESULT base_assert(HSQUIRRELVM v)
{
    if(SQVM::IsFalse(stack_get(v,2))){
        SQInteger top = sq_gettop(v);
        if (top>2 && SQ_SUCCEEDED(sq_tostring(v,3))) {
            const SQChar *str = 0;
            if (SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
                return sq_throwerror(v,_SC("%s"), str);
            }
        }
        return sq_throwerror(v, _SC("assertion failed"));
    }
    return 0;
}

static SQRESULT get_slice_params(HSQUIRRELVM v,SQInteger &sidx,SQInteger &eidx,SQObjectPtr &o)
{
	SQInteger top = sq_gettop(v);
	sidx=0;
	eidx=0;
	o=stack_get(v,1);
	if(top>1){
		SQObjectPtr &start=stack_get(v,2);
		if(sq_type(start)!=OT_NULL && sq_isnumeric(start)){
			sidx=tointeger(start);
		}
	}
	if(top>2){
		SQObjectPtr &end=stack_get(v,3);
		if(sq_isnumeric(end)){
			eidx=tointeger(end);
		}
	}
	else {
		eidx = sq_getsize(v,1);
	}
	return 1;
}

#define MAX_PRINT_STRSZ 1024*1024*1024
static void base_print_huge_str(HSQUIRRELVM v, const SQChar *str, SQInteger str_size)
{
    SQPRINTFUNCTION sqprint = _ss(v)->_printfunc;
    if(str_size > MAX_PRINT_STRSZ)
    {
        /*Print huge strings (> 2GB) is problematic*/
        int print_size = MAX_PRINT_STRSZ;
        while(str_size > 0)
        {
            sqprint(v,_SC("%.*s"), print_size, str);
            str += print_size;
            str_size -= print_size;
            if(str_size < print_size) print_size = str_size;
        }
    }
    else sqprint(v,_SC("%s"),str);
}

static SQInteger base_print1(HSQUIRRELVM v)
{
    const SQChar *str;
    SQInteger str_size;
    if(SQ_SUCCEEDED(sq_tostring(v,2)))
    {
        if(SQ_SUCCEEDED(sq_getstr_and_size(v,-1,&str, &str_size))) {
            if(_ss(v)->_printfunc) base_print_huge_str(v, str, str_size);
            return 0;
        }
    }
    return SQ_ERROR;
}

static SQRESULT base_print(HSQUIRRELVM v)
{
    if(_ss(v)->_printfunc){
        SQPRINTFUNCTION sqprint = _ss(v)->_printfunc;
        const SQChar *str;
        SQInteger str_size, nargs=sq_gettop(v);
        for(int i=2; i<=nargs; ++i){
            if(i>2) sqprint(v,_SC("\t"));
            if(SQ_SUCCEEDED(sq_tostring(v,i))) {
                sq_getstr_and_size(v,-1,&str, &str_size);
                base_print_huge_str(v, str, str_size);
                sq_poptop(v); //remove converted string
            } else {
                return SQ_ERROR;
            }
        }
        sqprint(v,_SC("\n"));
    }
	return 0;
}

static SQRESULT base_error(HSQUIRRELVM v)
{
	const SQChar *str;
    if(SQ_SUCCEEDED(sq_tostring(v,2)))
    {
        if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
            if(_ss(v)->_errorfunc) _ss(v)->_errorfunc(v,_SC("%s"),str);
            return 0;
        }
    }
    return SQ_ERROR;
}

static SQRESULT base_get_last_error(HSQUIRRELVM v)
{
    sq_getlasterror(v);
	return 1;
}

static SQRESULT base_get_last_stackinfo(HSQUIRRELVM v)
{
    sq_getlaststackinfo(v);
	return 1;
}

static SQRESULT base_compilestring(HSQUIRRELVM v)
{
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 2, src);
    SQ_OPT_STRING(v, 3, name, _SC("unnamedbuffer"));
    SQ_OPT_BOOL(v, 4, show_wanings, SQTrue);
    //if we want to have includes we should call loadstring from sqstdio
    if(SQ_SUCCEEDED(sq_compilebuffer(v,src,src_size,name,SQFalse, show_wanings, 0)))
        return 1;
    else
        return SQ_ERROR;
}

static SQRESULT base_newthread(HSQUIRRELVM v)
{
    if(sq_gettype(v, 2) != OT_CLOSURE) return sq_throwerror(v, _SC("invalid type function expected"));
	SQObjectPtr &func = stack_get(v,2);
	SQInteger stksize = (_closure(func)->_function->_stacksize << 1) +2;
	HSQUIRRELVM newv = sq_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
	sq_move(newv,v,-2);
	return 1;
}

static SQRESULT base_suspend(HSQUIRRELVM v)
{
	return sq_suspendvm(v);
}

template <typename T>
static SQRESULT base_array_base(HSQUIRRELVM v)
{
	T *a;
	SQObject &size = stack_get(v,2);
	if(sq_gettop(v) > 2) {
		a = T::Create(_ss(v),0);
		a->Resize(tointeger(size),stack_get(v,3));
	}
	else {
		a = T::Create(_ss(v),tointeger(size));
	}
	v->Push(a);
	return 1;
}

static SQRESULT base_array(HSQUIRRELVM v)
{
    return base_array_base<SQArray>(v);
}

static SQRESULT base_array_float64(HSQUIRRELVM v)
{
    return base_array_base<SQFloat64Array>(v);
}

static SQRESULT base_array_float32(HSQUIRRELVM v)
{
    return base_array_base<SQFloat32Array>(v);
}

static SQRESULT base_array_int64(HSQUIRRELVM v)
{
    return base_array_base<SQInt64Array>(v);
}

static SQRESULT base_array_int32(HSQUIRRELVM v)
{
    return base_array_base<SQInt32Array>(v);
}

static SQRESULT base_array_int16(HSQUIRRELVM v)
{
    return base_array_base<SQInt16Array>(v);
}

static SQRESULT base_array_int8(HSQUIRRELVM v)
{
    return base_array_base<SQInt8Array>(v);
}

static SQRESULT base_type(HSQUIRRELVM v)
{
	SQObjectPtr &o = stack_get(v,2);
	v->Push(SQString::Create(_ss(v),GetTypeName(o),-1));
	return 1;
}

static SQRESULT base_try_tostring(HSQUIRRELVM v)
{
    SQRESULT rc = SQ_SUCCEEDED(sq_tostring(v,2))?1:SQ_ERROR;
    if( (rc == SQ_ERROR) && (sq_gettop(v) > 2) )
    {
        sq_push(v, 3); //copy default to top
        return 1;
    }
	return rc;
}

static SQRESULT base_callee(HSQUIRRELVM v)
{
	if(v->_callsstacksize > 1)
	{
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return 1;
	}
	return sq_throwerror(v,_SC("no closure in the calls stack"));
}

static SQRESULT base_str_from_chars (HSQUIRRELVM v) {
  SQ_FUNC_VARS_NO_TOP(v);
  SQInteger n = sq_gettop(v);  /* number of arguments */
  SQInteger i;
  SQChar *data = sq_getscratchpad(v, n);
  for (i=2; i<=n; ++i) {
    SQ_GET_INTEGER(v, i, c);
    if(SQUChar(c) != c){
        return sq_throwerror(v, _SC("invalid value for parameter " _PRINT_INT_FMT), i);
    }
    data[i-2] = SQUChar(c);
  }
  sq_pushstring(v, data, n-1);
  return 1;
}

static SQRESULT base_getincludepath(HSQUIRRELVM v)
{
    const SQChar *include_path = v->GetIncludePath();
    if(include_path) sq_pushstring(v, include_path, -1);
    else sq_pushnull(v);
	return 1;
}

/////////////////////////////////////////////////////////////////
//TABLE BASE FUNCTIONS

static SQRESULT bf_table_rawdelete(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_rawdeleteslot(v,2,SQTrue)))
		return SQ_ERROR;
	return 1;
}


static SQRESULT bf_table_rawexists(HSQUIRRELVM v)
{
	sq_pushbool(v, sq_rawexists(v,-2));
	return 1;
}

static SQRESULT bf_table_set(HSQUIRRELVM v)
{
	return sq_set(v,-3);
}

static SQRESULT bf_table_rawset(HSQUIRRELVM v)
{
	return sq_rawset(v,-3);
}

static SQRESULT bf_table_get(HSQUIRRELVM v)
{
    switch(sq_gettop(v)){
        case 3: return SQ_SUCCEEDED(sq_get(v,-2))?1:SQ_ERROR;break;
        case 4: {
            sq_push(v, 3); //copy key to top
            sq_get(v,-4); //if it fail pop the key and default value is on top
            return 1;
        }
        break;
    }
    return sq_throwerror(v, _SC("invalid number of parameters"));
}

static SQRESULT bf_table_rawget(HSQUIRRELVM v)
{
    switch(sq_gettop(v)){
        case 3: return SQ_SUCCEEDED(sq_rawget(v,-2))?1:SQ_ERROR;break;
        case 4: {
            sq_push(v, 3); //copy key to top
            sq_rawget(v,-4); //if it fail pop the key and default value is on top
            return 1;
        }
        break;
    }
    return sq_throwerror(v, _SC("invalid number of parameters"));
}

static SQRESULT bf_table_clear(HSQUIRRELVM v)
{
	return sq_clear(v,-1);
}

static SQRESULT bf_table_incnum(HSQUIRRELVM v)
{
	SQObjectPtr &self = stack_get(v, 2);
	SQObjectPtr &key = stack_get(v, 3);
	SQBool addMissing = SQFalse;
	if(sq_gettop(v) > 4) sq_getbool(v, 5, &addMissing);
	bool rc = _table(self)->IncNum(key, stack_get(v, 4), addMissing);
	v->Pop(2);
	sq_pushbool(v, rc);
	return 1;
}

static SQRESULT bf_table_toarray(HSQUIRRELVM v)
{
	SQObjectPtr &self = stack_get(v, 2);
	SQInteger tsz = sq_getsize(v, 2);
	sq_newarray(v, tsz);
	SQInteger idx = 0;
    sq_pushnull(v);
    while(sq_next(v, 2) == SQ_OK)
    {
        sq_newarray(v, 2);
        sq_push(v, -3);
        sq_arrayset(v, -2, 0); //table key
        sq_push(v, -2);
        sq_arrayset(v, -2, 1); //table value
        sq_arrayset(v, 3, idx++); //set the new array into ary
        sq_pop(v, 2);
    }
    sq_pop(v, 1); //the null before while
	return 1;
}

static SQRESULT bf_obj_clone(HSQUIRRELVM v)
{
    SQRESULT rc = sq_clone(v,-1);
	 if(rc != SQ_OK) return rc;
    return 1;
}

static SQRESULT bf_table_len(HSQUIRRELVM v)
{
	v->Push(SQInteger(sq_getsize(v,2)));
	return 1;
}

static SQRESULT bf_table_weakref(HSQUIRRELVM v)
{
	sq_weakref(v,2);
	return 1;
}

static SQRESULT bf_table_tostring(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_tostring(v,2)))
		return SQ_ERROR;
	return 1;
}

static SQRESULT bf_table_setdelegate(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_setdelegate(v,-2)))
		return SQ_ERROR;
	sq_push(v,-1); // -1 because sq_setdelegate pops 1
	return 1;
}

static SQRESULT bf_table_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdelegate(v,2))?1:SQ_ERROR;
}

static SQRESULT bf_table_getdelegate_squirrel(HSQUIRRELVM v)
{
    SQSharedState *ss = _ss(v);
    v->Push(ss->_table_default_delegate_squirrel);
	return 1;
}

static SQRESULT bf_table_create(HSQUIRRELVM v)
{
    if(sq_gettop(v) > 1)
    {
        SQInteger capacity;
        sq_getinteger(v, 2, &capacity);
        sq_newtableex(v, capacity);
    }
    else
    {
        sq_newtable(v);
    }
	return 1;
}

#ifdef SQ_DEBUG_MEMORY
static SQRESULT base_getdebugmemory(HSQUIRRELVM v)
{
    sq_pushfstring(v, "tm=%d, tr=%d, tf=%d", _sq_total_malloc, _sq_total_realloc, _sq_total_free);
	return 1;
}
#endif

static SQRegFunction base_funcs[]={
	//generic
#ifdef SQ_DEBUG_MEMORY
	{_SC("getdebugmemory"),base_getdebugmemory,1, _SC("."), false},
#endif
	{_SC("setatexithandler"),base_setatexithandler,2, _SC(".c"), false},
	{_SC("getatexithandler"),base_getatexithandler,1, NULL, false},
	{_SC("seterrorhandler"),base_seterrorhandler,2, _SC(".c"), false},
	{_SC("geterrorhandler"),base_geterrorhandler,1, NULL, false},
	{_SC("setdebughook"),base_setdebughook,2, NULL, false},
	{_SC("enabledebuginfo"),base_enabledebuginfo,2, NULL, false},
	{_SC("getstackinfos"),base_getstackinfos,2, _SC(".n"), false},
	{_SC("getstacktop"),base_getstacktop,1, _SC("."), false},
	{_SC("gettypetag"),base_gettypetag,2, _SC(".."), false},
	{_SC("getdefaultdelegate"),base_getdefaultdelegate,2, _SC(".."), false},
	{_SC("getroottable"),base_getroottable,1, NULL, false},
	{_SC("setroottable"),base_setroottable,2, NULL, false},
	{_SC("getconsttable"),base_getconsttable,1, NULL, false},
	{_SC("setconsttable"),base_setconsttable,2, NULL, false},
	{_SC("assert"),base_assert,-2, ".bs", false},
	{_SC("print1"),base_print1,2, NULL, false},
	{_SC("print"),base_print,-2, NULL, false},
	{_SC("error"),base_error,2, NULL, false},
	{_SC("get_last_error"),base_get_last_error,1, NULL, false},
	{_SC("get_last_stackinfo"),base_get_last_stackinfo,1, NULL, false},
	{_SC("compilestring"),base_compilestring,-2, _SC(".ssb"), false},
	{_SC("newthread"),base_newthread,2, _SC(".c"), false},
	{_SC("suspend"),base_suspend,-1, NULL, false},
	{_SC("array"),base_array,-2, _SC(".n."), false},
	{_SC("array_float64"),base_array_float64,-2, _SC(".nf"), false},
	{_SC("array_float32"),base_array_float32,-2, _SC(".nf"), false},
	{_SC("array_int64"),base_array_int64,-2, _SC(".ni"), false},
	{_SC("array_int32"),base_array_int32,-2, _SC(".ni"), false},
	{_SC("array_int16"),base_array_int16,-2, _SC(".ni"), false},
	{_SC("array_int8"),base_array_int8,-2, _SC(".ni"), false},
	{_SC("type"),base_type,2, NULL, false},
	{_SC("callee"),base_callee,0,NULL, false},
	{_SC("dummy"),base_dummy,0,NULL, false},
#ifndef NO_GARBAGE_COLLECTOR
	{_SC("collectgarbage"),base_collectgarbage,0, NULL, false},
	{_SC("resurrectunreachable"),base_resurectureachable,0, NULL, false},
	{_SC("getrefcount"),base_getrefcount,2, _SC(".."), false},
#ifdef SQ_WITH_DELAYED_RELEASE_HOOKS
	{_SC("check_delayed_release_hooks"),base_check_delayed_release_hooks,-1, _SC(".b"), false},
	{_SC("call_delayed_release_hooks"),base_call_delayed_release_hooks,1, NULL, false},
#endif // SQ_WITH_DELAYED_RELEASE_HOOKS
#endif
	{_SC("str_from_chars"),base_str_from_chars,-1, _SC(".i"), false},
	{_SC("try_tostring"),base_try_tostring,-2, _SC("..s"), false},
	{_SC("getincludepath"),base_getincludepath,1, _SC("."), false},
	{_SC("table_create"),bf_table_create,-1, _SC(".i"), false},
	{_SC("table_new"),bf_table_create,-1, _SC(".i"), false},
	{_SC("table_len"),bf_table_len,2, _SC(".t"), false},
	{_SC("table_size"),bf_table_len,2, _SC(".t"), false},
	{_SC("table_get"),bf_table_get,-3, _SC(".t."), false},
	{_SC("table_rawget"),bf_table_rawget,-3, _SC(".t."), false},
	{_SC("table_set"),bf_table_set,4, _SC(".t.."), false},
	{_SC("table_rawset"),bf_table_rawset,4, _SC(".t.."), false},
	{_SC("table_rawdelete"),bf_table_rawdelete,3, _SC(".t."), false},
	{_SC("table_rawin"),bf_table_rawexists,3, _SC(".t."), false},
	{_SC("table_weakref"),bf_table_weakref,2, _SC(".t"), false},
	{_SC("table_tostring"),bf_table_tostring,2, _SC(".t"), false},
	{_SC("table_clear"),bf_table_clear,2, _SC(".t"), false},
	{_SC("table_setdelegate"),bf_table_setdelegate,3, _SC(".t t|o"), false},
	{_SC("table_getdelegate"),bf_table_getdelegate,2, _SC(".t"), false},
	{_SC("table_getdelegate_squirrel"),bf_table_getdelegate_squirrel,1, _SC("."), false},
	{_SC("table_incnum"),bf_table_incnum,-4, _SC(".tsnb"), false},
	{_SC("table_toarray"),bf_table_toarray,2, _SC(".t"), false},
	{_SC("obj_clone"),bf_obj_clone,2, _SC(". t|a|x|i|f|s"), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

void sq_base_register(HSQUIRRELVM v)
{
	SQInteger i=0;
	sq_pushroottable(v);
	while(base_funcs[i].name!=0) {
		sq_pushstring(v,base_funcs[i].name,-1);
		sq_newclosure(v,base_funcs[i].f,0);
		sq_setnativeclosurename(v,-1,base_funcs[i].name);
		sq_setparamscheck(v,base_funcs[i].nparamscheck,base_funcs[i].typemask);
		sq_newslot(v,-3, SQFalse);
		i++;
	}

	sq_pushstring(v,_SC("_versionnumber_"),-1);
	sq_pushinteger(v,SQUIRREL_VERSION_NUMBER);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_version_"),-1);
	sq_pushstring(v,SQUIRREL_VERSION,-1);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_charsize_"),-1);
	sq_pushinteger(v,sizeof(SQChar));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_intsize_"),-1);
	sq_pushinteger(v,sizeof(SQInteger));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_floatsize_"),-1);
	sq_pushinteger(v,sizeof(SQFloat));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_ptrsize_"),-1);
	sq_pushinteger(v,sizeof(void*));
	sq_newslot(v,-3, SQFalse);
	sq_pop(v,1);
}

static SQRESULT default_delegate_len(HSQUIRRELVM v)
{
	v->Push(SQInteger(sq_getsize(v,1)));
	return 1;
}

static SQRESULT default_delegate_tofloat(HSQUIRRELVM v)
{
	SQObjectPtr &o=stack_get(v,1);
	switch(sq_type(o)){
	case OT_STRING:{
		SQObjectPtr res;
		if(str2num(_stringval(o),res)){
			v->Push(SQObjectPtr(tofloat(res)));
			break;
		}}
		return sq_throwerror(v, _SC("cannot convert the string"));
	case OT_INTEGER:case OT_FLOAT:
		v->Push(SQObjectPtr(tofloat(o)));
		break;
	case OT_BOOL:
		v->Push(SQObjectPtr((SQFloat)(_integer(o)?1:0)));
		break;
	default:
		v->PushNull();
		break;
	}
	return 1;
}

static SQRESULT default_delegate_tointeger(HSQUIRRELVM v)
{
	SQObjectPtr &o=stack_get(v,1);
	switch(sq_type(o)){
	case OT_STRING:{
		SQObjectPtr res;
		SQInteger base;
		if(sq_gettop(v) > 1){
		    if(sq_getinteger(v, 2, &base) < 0) return sq_throwerror(v, _SC("parameter integer expected (2-36)"));
		    if(base < 2 || base > 36) return sq_throwerror(v, _SC("invalid base \"" _PRINT_INT_FMT "\" to tointeger (2-36)"), base);
		}
		else base = 10;
		if(str2num(_stringval(o),res, base)){
			v->Push(SQObjectPtr(tointeger(res)));
			break;
		}}
		return sq_throwerror(v, _SC("cannot convert the string"));
	case OT_INTEGER:case OT_FLOAT:
		v->Push(SQObjectPtr(tointeger(o)));
		break;
	case OT_BOOL:
		v->Push(SQObjectPtr(_integer(o)?(SQInteger)1:(SQInteger)0));
		break;
	default:
		v->PushNull();
		break;
	}
	return 1;
}

static SQRESULT default_delegate_tostring(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_tostring(v,1)))
		return SQ_ERROR;
	return 1;
}

static SQRESULT obj_delegate_weakref(HSQUIRRELVM v)
{
	sq_weakref(v,1);
	return 1;
}

static SQRESULT obj_clear(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_clear(v,-1)) ? 1 : SQ_ERROR;
}

static SQRESULT number_delegate_tochar(HSQUIRRELVM v)
{
	SQObject &o=stack_get(v,1);
	SQChar c = (SQChar)tointeger(o);
	v->Push(SQString::Create(_ss(v),(const SQChar *)&c,1));
	return 1;
}



/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static SQRESULT table_rawdelete(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_rawdeleteslot(v,1,SQTrue)))
		return SQ_ERROR;
	return 1;
}


static SQRESULT container_rawexists(HSQUIRRELVM v)
{
	if(SQ_SUCCEEDED(sq_rawget(v,-2))) {
		sq_pushbool(v,SQTrue);
		return 1;
	}
	sq_pushbool(v,SQFalse);
	return 1;
}

static SQRESULT container_rawset(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_rawset(v,-3)) ? 1 : SQ_ERROR;
}

static SQRESULT container_rawget(HSQUIRRELVM v)
{
    switch(sq_gettop(v)){
        case 2: return SQ_SUCCEEDED(sq_rawget(v,-2))?1:SQ_ERROR;break;
        case 3: {
            sq_push(v, 2); //copy key to top
            sq_rawget(v,-4); //if it fail pop the key and default value is on top
            return 1;
        }
        break;
    }
    return sq_throwerror(v, _SC("invalid number of parameters"));
}

static SQRESULT container_get(HSQUIRRELVM v)
{
    switch(sq_gettop(v)){
        case 2: return SQ_SUCCEEDED(sq_get(v,-2))?1:SQ_ERROR;
        case 3: {
            sq_push(v, 2); //copy key to top
            sq_get(v,-4); //if it fail pop the key and default value is on top
            return 1;
        }
        break;
    }
    return sq_throwerror(v, _SC("invalid number of parameters"));
}

static SQRESULT table_setdelegate(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_setdelegate(v,-2)))
		return SQ_ERROR;
	sq_push(v,-1); // -1 because sq_setdelegate pops 1
	return 1;
}

static SQRESULT table_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdelegate(v,-1))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_table_default_delegate_squirrel_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("t"), false},
	{_SC("size"),default_delegate_len,1, _SC("t"), false},
	{_SC("get"),container_get,-2, _SC("t"), false},
	{_SC("rawget"),container_rawget,-2, _SC("t"), false},
	{_SC("rawset"),container_rawset,3, _SC("t"), false},
	{_SC("rawdelete"),table_rawdelete,2, _SC("t"), false},
	{_SC("rawin"),container_rawexists,2, _SC("t"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("clear"),obj_clear,1, _SC("."), false},
	{_SC("setdelegate"),table_setdelegate,2, _SC(".t|o"), false},
	{_SC("getdelegate"),table_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

SQRegFunction SQSharedState::_table_default_delegate_funcz[]={
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static SQRESULT array_append(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_arrayappend(v,-2)) ? 1 : SQ_ERROR;
}

static SQRESULT array_extend(HSQUIRRELVM v)
{
	_array(stack_get(v,1))->Extend(_array(stack_get(v,2)));
	sq_pop(v,1);
	return 1;
}

static SQRESULT array_reverse(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_arrayreverse(v,-1)) ? 1 : SQ_ERROR;
}

static SQRESULT array_pop(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_arraypop(v,1,SQTrue))?1:SQ_ERROR;
}

static SQRESULT array_top(HSQUIRRELVM v)
{
	SQObject &o=stack_get(v,1);
	if(_array(o)->Size()>0){
        v->PushNull();
        SQObjectPtr &ot = stack_get(v, -1);
        _array(o)->Top(ot);
		//v->Push(_array(o)->Top());
		return 1;
	}
	else return sq_throwerror(v,_SC("top() on a empty array"));
}

static SQRESULT array_insert(HSQUIRRELVM v)
{
	SQObject &o=stack_get(v,1);
	SQObject &idx=stack_get(v,2);
	SQObject &val=stack_get(v,3);
	if(!_array(o)->Insert(tointeger(idx),val))
		return sq_throwerror(v,_SC("index out of range"));
	sq_pop(v,2);
	return 1;
}

static SQRESULT array_set(HSQUIRRELVM v)
{
	SQObject &o=stack_get(v,1);
	SQObject &idx=stack_get(v,2);
	SQObject &val=stack_get(v,3);
	if(!_array(o)->Set(tointeger(idx),val))
		return sq_throwerror(v,_SC("index out of range"));
	return 0;
}

static SQRESULT array_remove(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v, 1);
	SQObject &idx = stack_get(v, 2);
	if(!sq_isnumeric(idx)) return sq_throwerror(v, _SC("wrong type"));
	SQObjectPtr val;
	if(_array(o)->Get(tointeger(idx), val)) {
		_array(o)->Remove(tointeger(idx));
		v->Push(val);
		return 1;
	}
	return sq_throwerror(v, _SC("idx out of range"));
}

enum e_array_op_type {e_resize, e_minsize, e_reserve, e_capacity};
static inline SQRESULT array_resize_base(HSQUIRRELVM v, e_array_op_type opType)
{
	SQObject &o = stack_get(v, 1);
	if(opType == e_capacity)
	{
		sq_pushinteger(v, _array(o)->Capacity());
		return 1;
	}
	SQObject &nsize = stack_get(v, 2);
	SQObjectPtr fill;
	if(sq_isnumeric(nsize)) {
		SQInteger sz = tointeger(nsize);
		if (sz<0)
		  return sq_throwerror(v, _SC("resizing to negative length"));
		SQUnsignedInteger usize = sz;
		switch(opType)
		{
		case e_reserve:
		    _array(o)->Reserve(usize);
		    break;
		case e_minsize:
		    if(_array(o)->Size() >= usize) break;
		    //falthrough
		default:
		    if(sq_gettop(v) > 2)
			fill = stack_get(v, 3);
		    _array(o)->Resize(usize,fill);
		    sq_settop(v, 1);
		    return 1;
		}
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("size must be a number"));
}

static SQRESULT array_resize(HSQUIRRELVM v)
{
    return array_resize_base(v, e_resize);
}

static SQRESULT array_minsize(HSQUIRRELVM v)
{
    return array_resize_base(v, e_minsize);
}

static SQRESULT array_reserve(HSQUIRRELVM v)
{
    return array_resize_base(v, e_reserve);
}

static SQRESULT array_capacity(HSQUIRRELVM v)
{
    return array_resize_base(v, e_capacity);
}

static SQRESULT __map_array(SQArrayBase *dest,SQArrayBase *src,HSQUIRRELVM v) {
	SQObjectPtr temp;
	SQInteger size = src->Size();
	for(SQInteger n = 0; n < size; n++) {
		src->Get(n,temp);
		v->Push(src);
		v->Push(temp);
		if(SQ_FAILED(sq_call(v,2,SQTrue,SQFalse))) {
			return SQ_ERROR;
		}
		dest->Set(n,v->GetUp(-1));
		v->Pop();
	}
	return 0;
}

static SQRESULT array_map(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	SQObjectPtr ret = _array(o)->Clone();
	if(SQ_FAILED(__map_array(_array(ret),_array(o),v)))
		return SQ_ERROR;
	v->Push(ret);
	return 1;
}

static SQRESULT array_apply(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	if(SQ_FAILED(__map_array(_array(o),_array(o),v)))
		return SQ_ERROR;
	sq_pop(v,1);
	return 1;
}

static SQRESULT array_reduce(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	SQArrayBase *a = _array(o);
	SQInteger size = a->Size();
	if(size == 0) {
		return 0;
	}
	SQObjectPtr res;
	a->Get(0,res);
	if(size > 1) {
		SQObjectPtr other;
		for(SQInteger n = 1; n < size; n++) {
			a->Get(n,other);
			v->Push(o);
			v->Push(res);
			v->Push(other);
			if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {
				return SQ_ERROR;
			}
			res = v->GetUp(-1);
			v->Pop();
		}
	}
	v->Push(res);
	return 1;
}

static SQRESULT array_filter(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	SQArrayBase *a = _array(o);
	SQObjectPtr ret = a->Clone(false);
	SQInteger size = a->Size();
	SQObjectPtr val;
	for(SQInteger n = 0; n < size; n++) {
		a->Get(n,val);
		v->Push(o);
		v->Push(n);
		v->Push(val);
		if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {
			return SQ_ERROR;
		}
		if(!SQVM::IsFalse(v->GetUp(-1))) {
			_array(ret)->Append(val);
		}
		v->Pop();
	}
	v->Push(ret);
	return 1;
}

static SQRESULT array_find(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	SQObjectPtr &val = stack_get(v,2);
	SQArrayBase *a = _array(o);
	SQInteger size = a->Size();
	SQObjectPtr temp;
	for(SQInteger n = 0; n < size; n++) {
		a->Get(n,temp);
		if(v->IsEqual(temp,val)) {
			v->Push(n);
			return 1;
		}
	}
	return 0;
}


static SQRESULT array_bsearch(HSQUIRRELVM v)
{
	SQObject &o = stack_get(v,1);
	SQObjectPtr &val = stack_get(v,2);
	SQArrayBase *a = _array(o);
	SQObjectPtr temp;
	SQInteger imid = 0, imin = 0, imax = a->Size()-1;
	while(imax >= imin) {
	    /* calculate the midpoint for roughly equal partition */
        imid = (imin + imax) / 2;

        // determine which subarray to search
		SQInteger res = 0;
		a->Get(imid,temp);

		if(v->ObjCmp(temp,val,res)) {
            if(res <  0)
                // change min index to search upper subarray
                imin = imid + 1;
            else if(res > 0 )
                // change max index to search lower subarray
                imax = imid - 1;
            else{
                // key found at index imid
                sq_pushinteger(v, imid);
                return 1;
            }
		}
		else break;
	}
	sq_pushinteger(v, imid > 0 ? -imid : -1);
	return 1;
}


static bool _sort_compare(HSQUIRRELVM v, SQArrayBase *arr,const SQObjectPtr &a,const SQObjectPtr &b,SQInteger func,SQInteger &ret)
{
	if(func < 0) {
		if(!v->ObjCmp(a,b,ret)) return false;
	}
	else {
		SQInteger top = sq_gettop(v);
		sq_push(v, func);
		sq_pushroottable(v);
		v->Push(a);
		v->Push(b);
		void *valptr = arr->RawData();
		SQUnsignedInteger precallsize = arr->Size();
		if(SQ_FAILED(sq_call(v, 3, SQTrue, SQFalse))) {
			if(!sq_isstring( v->_lasterror))
				v->Raise_Error(_SC("compare func failed"));
			return false;
		}
		if(SQ_FAILED(sq_getinteger(v, -1, &ret))) {
			v->Raise_Error(_SC("numeric value expected as return value of the compare function"));
			return false;
		}
		if (precallsize != arr->Size() || valptr != arr->RawData()) {
			v->Raise_Error(_SC("array resized during sort operation"));
			return false;
		}
		sq_settop(v, top);
		return true;
	}
	return true;
}

/*
** The lua_auxsort code is adapted from from lua 5.1.5
** {======================================================
** Quicksort
** (based on 'Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
** =======================================================
*/

static bool lua_auxsort (HSQUIRRELVM v, SQArrayBase *arr, SQInteger l, SQInteger u,
                                   SQInteger func, SQArrayBase *arrMirror) {
  #define ARR_SWAP(a,b) {arr->_swap(a, b);if(arrMirror) arrMirror->_swap(a, b);}
  while (l < u) {  /* for tail recursion */
    SQInteger i, j, ret;
    bool rc;
    SQObject o1, o2;
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    arr->_get2(u, o1);
    arr->_get2(l, o2);
    if(!_sort_compare(v,arr,o1,o2,func,ret))
        return false;
    if (ret < 0)  /* a[u] < a[l]? */
      ARR_SWAP(l, u)  /* swap a[l] - a[u] */
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    arr->_get2(i, o1);
    arr->_get2(l, o2);
    if(!_sort_compare(v,arr,o1,o2,func,ret))
        return false;
    if (ret < 0)  /* a[i]<a[l]? */
      ARR_SWAP(i, l)
    else {
      arr->_get2(u, o1);
      arr->_get2(i, o2);
      if(!_sort_compare(v,arr,o1,o2,func,ret))
        return false;
      if (ret < 0)  /* a[u]<a[i]? */
        ARR_SWAP(i, u)
    }
    if (u-l == 2) break;  /* only 3 elements */
    SQObject P;
    arr->_get2(i, P);  /* Pivot */
    ARR_SWAP(i, u-1)
    /* a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (arr->_get2(++i, o1), (rc = _sort_compare(v,arr,o1,P,func,ret)) && (ret < 0)) {
        if (i>u)
        {
            sq_throwerror(v, _SC("invalid order function for sorting"));
            return false;
        }
      }
      if(!rc) return false;
      /* repeat --j until a[j] <= P */
      while (arr->_get2(--j, o2), (rc = _sort_compare(v,arr,P, o2,func,ret)) && (ret < 0)) {
        if (j<l)
        {
            sq_throwerror(v, _SC("invalid order function for sorting"));
            return false;
        }
      }
      if(!rc) return false;
      if (j<i) {
        break;
      }
      ARR_SWAP(i, j)
    }
    ARR_SWAP(u-1, i)  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    if(!lua_auxsort(v, arr, j, i, func, arrMirror))  /* call recursively for upper interval */
        return false;
  }  /* repeat the routine for the larger one */
  return true;
  #undef ARR_SWAP
}

static SQRESULT array_sort(HSQUIRRELVM v) {
	SQInteger func = -1;
	SQObjectPtr &o = stack_get(v,1);
	SQArrayBase *arr = _array(o);
	SQArrayBase *arrMirror = NULL;
	if(arr->Size() > 1) {
		if(sq_gettop(v) > 1){
            if(sq_gettype(v, 2) == OT_CLOSURE) func = 2;
		}
		if(sq_gettop(v) > 2){
            SQObjectPtr &om = stack_get(v,3);
            arrMirror = _array(om);
            if(arr->Size() != arrMirror->Size()) return sq_throwerror(v, _SC("arrays size mismatch"));
		}
		if(!lua_auxsort(v, arr, 0, arr->Size()-1, func, arrMirror))
			return SQ_ERROR;

	}
	sq_settop(v,1);
	return 1;
}

/* }====================================================== */

static SQRESULT array_slice(HSQUIRRELVM v)
{
	SQInteger sidx,eidx;
	SQObjectPtr o;
	if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
	SQInteger alen = _array(o)->Size();
	if(sidx < 0)sidx = alen + sidx;
	if(eidx < 0)eidx = alen + eidx;
	if(eidx < sidx)return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > alen || sidx < 0)return sq_throwerror(v, _SC("slice out of range"));
	SQArrayBase *arr=_array(o)->Clone(false);
	arr->Resize(eidx-sidx);
	SQObjectPtr t;
	SQInteger count=0;
	for(SQInteger i=sidx;i<eidx;i++){
		_array(o)->Get(i,t);
		arr->Set(count++,t);
	}
	v->Push(arr);
	return 1;

}

//DAD start
#include <sqstdio.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

static SQRESULT array_concat0 (HSQUIRRELVM v, int allowAll) {
    SQ_FUNC_VARS(v);
    SQObjectPtr &arobj = stack_get(v,1);
    SQArrayBase *arr = _array(arobj);

    SQInteger last = arr->Size()-1;
    if(last == -1){
        sq_pushstring(v, _SC(""), 0);
        return 1;
    }
    SQ_OPT_STRING(v, 2, sep, _SC(""));
    SQ_OPT_INTEGER(v, 3, opt_first, 0);
    SQ_OPT_INTEGER(v, 4, opt_last, last);

  opt_last = opt_last < last ? opt_last : last;

  if(opt_first > opt_last)
  {
      sq_pushstring(v, "", 0);
      return 1;
  }

  SQBlob blob(0, 8192);

  for (SQInteger i=opt_first; i <= opt_last; ++i) {
      SQObject o;
      SQObjectPtr str;
      arr->_get2(i, o);
      switch(sq_type(o)){
          case OT_STRING:
              break;
          case OT_INTEGER:
          case OT_FLOAT:
          case OT_NULL:
              if(!v->ToString(o,str)) return SQ_ERROR;
              break;
          case OT_USERDATA:
            if(allowAll){
              if(!v->ToString(o,str)) return SQ_ERROR;
              break;
            }
          default:
              return sq_throwerror(v, _SC("Invalid type \"%s\" at position " _PRINT_INT_FMT " for array concat !"),
								GetTypeName(o), i);
      }

      const SQChar *value;
      SQInteger value_size;
      if(sq_type(o) == OT_STRING) {
		value = _stringval(o);
		value_size = _string(o)->_len;
      }
      else
      {
		value = _stringval(str);
		value_size = _string(str)->_len;
      }
      if(i > opt_first && sep_size) blob.Write((void*)sep, sep_size);
      blob.Write((void*)value, value_size);
  }
  sq_pushstring(v, (SQChar*)blob.GetBuf(), blob.Len());
  return 1;
}

static SQRESULT array_concat (HSQUIRRELVM v) {
  return array_concat0(v, 0);
}

static SQRESULT array_concat2 (HSQUIRRELVM v) {
  return array_concat0(v, 1);
}

static SQRESULT array_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_ARRAY))?1:SQ_ERROR;
}

static SQRESULT array_empty(HSQUIRRELVM v)
{
	sq_pushbool(v,sq_getsize(v,1) == 0);
	return 1;
}

static SQRESULT array_sizeofelm(HSQUIRRELVM v)
{
	sq_arraygetsizeof(v,1);
	return 1;
}

//DAD end

SQRegFunction SQSharedState::_array_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("a"), false},
	{_SC("size"),default_delegate_len,1, _SC("a"), false},
	{_SC("append"),array_append,2, _SC("a"), false},
	{_SC("push"),array_append,2, _SC("a"), false},
	{_SC("push_back"),array_append,2, _SC("a"), false},
	{_SC("extend"),array_extend,2, _SC("aa"), false},
	{_SC("pop"),array_pop,1, _SC("a"), false},
	{_SC("top"),array_top,1, _SC("a"), false},
	{_SC("insert"),array_insert,3, _SC("an"), false},
	{_SC("remove"),array_remove,2, _SC("an"), false},
	{_SC("resize"),array_resize,-2, _SC("an"), false},
	{_SC("minsize"),array_minsize,-2, _SC("an"), false},
	{_SC("reserve"),array_reserve,-2, _SC("an"), false},
	{_SC("capacity"),array_capacity,1, _SC("a"), false},
	{_SC("reverse"),array_reverse,1, _SC("a"), false},
	{_SC("sort"),array_sort,-1, _SC("a c|o a"), false},
	{_SC("slice"),array_slice,-1, _SC("ann"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("clear"),obj_clear,1, _SC("."), false},
	{_SC("map"),array_map,2, _SC("ac"), false},
	{_SC("apply"),array_apply,2, _SC("ac"), false},
	{_SC("reduce"),array_reduce,2, _SC("ac"), false},
	{_SC("filter"),array_filter,2, _SC("ac"), false},
	{_SC("find"),array_find,2, _SC("a."), false},
	{_SC("indexOf"),array_find,2, _SC("a."), false},
	{_SC("bsearch"),array_bsearch,2, _SC("a."), false},
	{_SC("concat"),array_concat,-1, _SC("as"), false},
	{_SC("join"),array_concat,-1, _SC("as"), false},
	{_SC("concat2"),array_concat2,-1, _SC("as"), false},
	{_SC("getdelegate"),array_getdelegate,1, _SC("."), false},
	{_SC("get"),container_rawget, -2, _SC("ai."), false},
	{_SC("set"),array_set, 3, _SC("ai."), false},
	{_SC("isempty"),array_empty, 1, _SC("a"), false},
	{_SC("sizeofelm"),array_sizeofelm, 1, _SC("a"), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//STRING DEFAULT DELEGATE//////////////////////////
static SQRESULT string_hash(HSQUIRRELVM v)
{
	SQObjectPtr &o = stack_get(v,1);
    sq_pushinteger(v,_string(o)->_hash);
	return 1;
}

static SQRESULT string_slice(HSQUIRRELVM v)
{
	SQInteger sidx,eidx;
	SQObjectPtr o;
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
	SQInteger slen = _string(o)->_len;
	if(sidx < 0)sidx = slen + sidx;
	if(eidx < 0)eidx = slen + eidx;
	if(eidx < sidx)	return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > slen || sidx < 0)	return sq_throwerror(v, _SC("slice out of range"));
	v->Push(SQString::Create(_ss(v),&_stringval(o)[sidx],eidx-sidx));
	return 1;
}

static SQRESULT string_substr(HSQUIRRELVM v)
{
    SQ_FUNC_VARS(v);
    SQ_GET_INTEGER(v, 2, start);
	SQObjectPtr &o = stack_get(v,1);
    SQInteger str_size = sq_getsize(v, 1);
    SQ_OPT_INTEGER(v, 3, len, str_size - start);
	if(start < 0)	return sq_throwerror(v,_SC("invalid start index " _PRINT_INT_FMT), start);
	if(len > (str_size - start))	return sq_throwerror(v,_SC("lenght out of range"));
	v->Push(SQString::Create(_ss(v),&_stringval(o)[start], len));
	return 1;
}

static SQRESULT string_find(HSQUIRRELVM v)
{
	SQInteger top,start_idx=0;
	const SQChar *str,*substr,*ret;
	if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
		if(top>2)sq_getinteger(v,3,&start_idx);
		if(sq_getsize(v,1)>start_idx)
		{
			if(start_idx>=0)
			{
				ret=scstrstr(&str[start_idx],substr);
				if(ret){
					sq_pushinteger(v,(SQInteger)(ret-str));
					return 1;
				}
			}
		}
		sq_pushinteger(v,-1);
		return 1;
	}
	return sq_throwerror(v,_SC("invalid param"));
}

#define STRING_TOFUNCZ(func) static SQInteger string_##func(HSQUIRRELVM v) \
{\
	SQInteger sidx,eidx; \
	SQObjectPtr str; \
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
	SQInteger slen = _string(str)->_len; \
	if(sidx < 0)sidx = slen + sidx; \
	if(eidx < 0)eidx = slen + eidx; \
	if(eidx < sidx)	return sq_throwerror(v,_SC("wrong indexes")); \
	if(eidx > slen || sidx < 0)	return sq_throwerror(v,_SC("slice out of range")); \
	SQInteger len=_string(str)->_len; \
	const SQChar *sthis=_stringval(str); \
	SQChar *snew=(_ss(v)->GetScratchPad(sq_rsl(len))); \
	memcpy(snew,sthis,sq_rsl(len));\
	for(SQInteger i=sidx;i<eidx;i++) snew[i] = func(sthis[i]); \
	v->Push(SQString::Create(_ss(v),snew,len)); \
	return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

//DAD start
#include "lua-regex.h"

static SQInteger calc_new_size_by_max_len(SQInteger start_pos, SQInteger max_len, SQInteger curr_size)
{
    SQInteger new_size;
    if(start_pos < 0)
    {
        new_size = curr_size + start_pos;
        start_pos = new_size < 0 ? 0 : new_size;
    }
    if(max_len > 0) new_size = start_pos + max_len;
    else new_size = curr_size + max_len;
    if( (new_size < curr_size) && (new_size > start_pos) )
    {
        return new_size;
    }
    return curr_size;
}

static inline void push_match_capture(HSQUIRRELVM v, int i, LuaMatchState *ms)
{
    ptrdiff_t len = ms->capture[i].len;
    if(len == CAP_POSITION) sq_pushinteger(v, ms->capture[i].init - ms->src_init);
    else sq_pushstring(v, ms->capture[i].init, ms->capture[i].len);
}


//on 64 bits there is an error SQRESULT/int
static int process_string_gsub(LuaMatchState *ms, void *udata, lua_char_buffer_st **b) {
    const SQChar *str;
    SQInteger str_size;
    HSQUIRRELVM v = (HSQUIRRELVM)udata;
    SQObjectType rtype = sq_gettype(v, 3);
    SQInteger top = sq_gettop(v);
    SQInteger result = 1;
    int rc;
    switch(rtype){
        case OT_NATIVECLOSURE:
        case OT_CLOSURE:{
            sq_push(v, 3); //push the function
            sq_pushroottable(v); //this
            int i=0;
            for(; i < ms->level; ++i){
                push_match_capture(v, i, ms);
            }
            if(i==0) //no captures push whole match
            {
                sq_pushstring(v, ms->src_init + ms->start_pos, ms->end_pos-ms->start_pos);
                ++i;
            }
            rc = sq_call(v, i+1, SQTrue, SQTrue);
            if(rc < 0) {
                ms->error = sq_getlasterror_str(v);
                return 0;
            }

            if(SQ_SUCCEEDED(sq_getstr_and_size(v, -1, &str, &str_size))){
                if(!char_buffer_add_str(ms, b, str, str_size)) {
                    result = 0;
                    break;
                }
            }
        }
        break;
        case OT_ARRAY:
        case OT_TABLE:{
            bool isArray = rtype == OT_ARRAY;
            for(int i=0; (i < ms->level) || (!ms->level && !i); ++i){
                sq_settop(v, top);
                if(ms->level == 0)//no captures push whole match
                {
                    sq_pushstring(v, ms->src_init + ms->start_pos, ms->end_pos-ms->start_pos);
                }
                else push_match_capture(v, i, ms);

                rc = !isArray || (sq_gettype(v, -1) == OT_INTEGER);
                if(rc && (sq_get(v, 3) == SQ_OK))
                {
                    if(isArray) rc = sq_tostring(v, -1);
                    else rc = SQ_OK;

                    if(rc == SQ_OK)
                    {
                        rc = sq_getstr_and_size(v, -1, &str, &str_size);
                        if(rc == SQ_OK)
                        {
                            if(!char_buffer_add_str(ms, b, str, str_size)) {
                                result = 0;
                                break;
                            }
                        }
                    }
                }
                else //not found in table push the original value
                {
                    if(!char_buffer_add_str(ms, b, ms->src_init+ms->start_pos, ms->end_pos-ms->start_pos)) {
                        result = 0;
                        break;
                    }
                }
            }
        }
        break;
	default:
		return sq_throwerror(v, _SC("unexpected type"));
    }
    sq_settop(v, top); //restore the stack to it's original state
    return result; //returning non zero means continue
}

static SQRESULT string_gsub(HSQUIRRELVM v)
{
    const char *error_ptr = NULL;
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, src);
    SQ_GET_STRING(v, 2, pattern);
    SQ_OPT_INTEGER(v, 4, max_sub, 0);
    SQObjectType rtype = sq_gettype(v, 3);
    if(rtype == OT_STRING){
        SQ_GET_STRING(v, 3, replacement);
        lua_char_buffer_st *buf = lua_str_gsub (src, src_size, pattern, pattern_size,
                              replacement, replacement_size, max_sub, &error_ptr, 0, 0);
        if(buf){
            sq_pushstring(v, buf->buf, buf->used);
            free(buf);
            return 1;
        }
        return sq_throwerror(v,_SC("%s"),error_ptr);
    }
    else
    {
        switch(rtype){
            case OT_CLOSURE:
            case OT_NATIVECLOSURE:
            case OT_ARRAY:
            case OT_TABLE:{
                lua_char_buffer_st *buf = lua_str_gsub (src, src_size, pattern, pattern_size,
                              0, 0, max_sub, &error_ptr, process_string_gsub, v);
                if(buf){
                    if(buf->used) sq_pushstring(v, buf->buf, buf->used);
                    else sq_push(v, 1); //nothing matches so return the original
                    free(buf);
                    if(!error_ptr) return 1;
                }
                return sq_throwerror(v,_SC("%s"),error_ptr);
            }
   	   default:
		return sq_throwerror(v, _SC("unexpected type"));
        }
    }
	return sq_throwerror(v,_SC("invalid type for parameter 3 function/table/array/string expected"));
}

static SQRESULT process_string_gmatch_find(LuaMatchState *ms, void *udata, lua_char_buffer_st **b, bool isFind) {
    HSQUIRRELVM v = (HSQUIRRELVM)udata;
    SQInteger top = sq_gettop(v);
    SQInteger result = 1;
    int i=0;
    sq_push(v, 3); //push the function
    sq_pushroottable(v); //this en, function already on top of stack
    if(isFind){
        sq_pushinteger(v, ms->start_pos);
        sq_pushinteger(v, ms->end_pos);
    }
    for(; i < ms->level; ++i){
        push_match_capture(v, i, ms);
    }
    if(!isFind && i == 0){
        sq_pushstring(v, ms->src_init + ms->start_pos, ms->end_pos-ms->start_pos);
        i=1;
    }
    int rc = sq_call(v, i+1 + (isFind ? 2 : 0), SQTrue, SQTrue);
    if(rc < 0) {
        ms->error = sq_getlasterror_str(v);
        return 0;
    }
    SQObjectType rtype = sq_gettype(v, -1);
    if(rtype == OT_BOOL) {
        SQBool bv;
        sq_getbool(v, -1, &bv);
        result = bv == SQTrue;
    }
    else result = rtype != OT_NULL;

    sq_settop(v, top); //restore the stack to it's original state
    return result; //returning non zero means continue
}

//on 64 bits there is an error SQRESULT/int
static int process_string_gmatch(LuaMatchState *ms, void *udata, lua_char_buffer_st **b) {
    return process_string_gmatch_find(ms, udata, b, false);
}

//used by sqstdblob
SQRESULT string_gmatch_base(HSQUIRRELVM v, int isGmatch, const SQChar *src, SQInteger src_size)
{
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 2, pattern);
    LuaMatchState ms;
    memset(&ms, 0, sizeof(ms));

    if(isGmatch){
        SQ_OPT_INTEGER(v, 4, start_pos, 0);
        SQ_OPT_INTEGER(v, 5, max_len, 0);
        SQInteger rtype = sq_gettype(v, 3);
        if(max_len)
        {
            src_size = calc_new_size_by_max_len(start_pos, max_len, src_size);
        }
        //if (start_pos < 0) start_pos = 0;
        if((rtype == OT_CLOSURE) || (rtype == OT_NATIVECLOSURE)){
            _rc_ = lua_str_match(&ms, src, max_len ? start_pos + max_len : src_size,
                    pattern, pattern_size, start_pos, 0, process_string_gmatch, v);
            if(ms.error) return sq_throwerror(v,_SC("%s"), ms.error);
            sq_pushinteger(v, _rc_);
            return 1;
        }
        return sq_throwerror(v,_SC("invalid type for parameter 3 function expected"));
    }
    SQ_OPT_INTEGER(v, 3, start_pos, 0);
    SQ_OPT_INTEGER(v, 4, max_len, 0);
    if(max_len)
    {
        src_size = calc_new_size_by_max_len(start_pos, max_len, src_size);
    }
    _rc_ = lua_str_match(&ms, src, max_len ? start_pos + max_len : src_size,
                        pattern, pattern_size, start_pos, 0, 0, 0);
    if(ms.error) return sq_throwerror(v,_SC("%s"), ms.error);
    if(_rc_ < 0) sq_pushnull(v);
    else if(ms.level){
        if(ms.level == 1)
        {
            if(ms.capture[0].len == CAP_POSITION) sq_pushinteger(v, ms.capture[0].init - ms.src_init);
            else sq_pushstring(v, ms.capture[0].init, ms.capture[0].len);
        }
        else {
            sq_newarray(v, ms.level);
            for(int i=0; i < ms.level; ++i){
                sq_pushinteger(v, i);
                push_match_capture(v, i, &ms);
                sq_rawset(v, -3);
            }
        }
    } else {
        sq_pushstring(v, src + ms.start_pos, ms.end_pos-ms.start_pos);
    }
    return 1;
}

static SQRESULT string_gmatch(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);
    return string_gmatch_base(v, 1, src, src_size);
}

static SQRESULT string_match(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);
    return string_gmatch_base(v, 0, src, src_size);
}

static int process_string_find_lua(LuaMatchState *ms, void *udata, lua_char_buffer_st **b) {
    return process_string_gmatch_find(ms, udata, b, true);
}

//used by sqstdblob
SQRESULT string_find_lua(HSQUIRRELVM v, const SQChar *src, SQInteger src_size)
{
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 2, pattern);
    SQ_OPT_INTEGER(v, 4, start_pos, 0);
    SQ_OPT_BOOL(v, 5, raw, SQFalse);
    SQ_OPT_INTEGER(v, 6, max_len, 0);
    SQInteger rtype = sq_gettype(v, 3);
    if(max_len)
    {
        src_size = calc_new_size_by_max_len(start_pos, max_len, src_size);
    }

    if(_top_ == 2){
        //only want to know if it exists
        LuaMatchState ms;
        memset(&ms, 0, sizeof(ms));
        int rc = lua_str_find(&ms, src, src_size, pattern, pattern_size,
                start_pos, raw == SQTrue, 0, 0);
        if(ms.error) return sq_throwerror(v,_SC("%s"), ms.error);
        sq_pushinteger(v, rc);
        return 1;
    }
    if((rtype == OT_CLOSURE) || (rtype == OT_NATIVECLOSURE)){
        LuaMatchState ms;
        memset(&ms, 0, sizeof(ms));
        int rc = lua_str_find(&ms, src, src_size, pattern, pattern_size,
                start_pos, raw == SQTrue, process_string_find_lua, v);
        if(ms.error) return sq_throwerror(v,_SC("%s"), ms.error);
        sq_pushinteger(v, rc);
        return 1;
    }
    else if(rtype == OT_TABLE || rtype == OT_ARRAY){
        LuaMatchState ms;
        memset(&ms, 0, sizeof(ms));
        int rc = lua_str_find(&ms, src, src_size, pattern, pattern_size,
                start_pos, raw == SQTrue, 0, 0);
        if(ms.error) return sq_throwerror(v,_SC("%s"), ms.error);
        if(rtype == OT_TABLE){
            sq_pushstring(v, _SC("start_pos"), -1);
            sq_pushinteger(v, ms.start_pos);
            sq_rawset(v, 3);
            sq_pushstring(v, _SC("end_pos"), -1);
            sq_pushinteger(v, ms.end_pos);
            sq_rawset(v, 3);
        }
        else if(rtype == OT_ARRAY)
        {
            SQObjectPtr &arr = stack_get(v,3);
            _array(arr)->Minsize(2 + (ms.level*2));
            sq_pushinteger(v, 0);
            sq_pushinteger(v, ms.start_pos);
            sq_rawset(v, 3);
            sq_pushinteger(v, 1);
            sq_pushinteger(v, ms.end_pos);
            sq_rawset(v, 3);
            SQInteger idx = 2;
            for(int i=0; i < ms.level; ++i){
                sq_pushinteger(v, idx++);
                sq_pushinteger(v, ms.capture[i].init - ms.src_init);
                sq_rawset(v, 3);
                sq_pushinteger(v, idx++);
                sq_pushinteger(v, ms.capture[i].len);
                sq_rawset(v, 3);
            }
        }
        sq_pushinteger(v, rc);
        return 1;
    }
	return sq_throwerror(v,_SC("invalid type for parameter 3 function expected"));
}

static SQRESULT string_find_lua(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);
    return string_find_lua(v, src, src_size);
}

static const SQChar *lmemfind (const SQChar *s1, size_t l1,
                               const SQChar *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const SQChar *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const SQChar *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}

/*DAD */
static SQRESULT string_replace(HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, src);
    SQ_GET_STRING(v, 2, p);
    if(p_size == 0) //empty str to search
    {
        sq_push(v, 1);
        return 1;
    }
    SQ_GET_STRING(v, 3, p2);
    SQ_OPT_INTEGER(v, 4, count, 0);
    const SQChar *s2;
    SQInteger n = 0;
    SQInteger init = 0;

    SQBlob b(0, 8192);

    while (1) {
        s2 = lmemfind(src+init, src_size-init, p, p_size);
        if (s2) {
            b.Write(src+init, s2-(src+init));
            b.Write(p2, p2_size);
            init = init + (s2-(src+init)) + p_size;
            n++;
            if(count && (n >= count)) {
                b.Write(src+init, src_size-init);
                break;
            }
        } else {
            b.Write(src+init, src_size-init);
            break;
        }
    }
    sq_pushstring(v, (const SQChar*)b.GetBuf(), b.Len());
    return 1;
}

static SQRESULT string_endswith(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_STRING(v, 2, token);

    SQInteger ti = token_size, si = str_size;
    SQBool end = SQTrue;
    if(token_size <= str_size){
        while(ti > 0) {
            if(str[--si] != token[--ti]){
                end = SQFalse;
                break;

            }
        }
    }
    else {
        end = SQFalse;
    }
    sq_pushbool(v, end);
    return 1;
}

static SQRESULT string_startswith(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_STRING(v, 2, token);

    SQInteger i;
    SQBool start = SQTrue;
    // please make this less ugly...
    if(token_size <= str_size){
    	for(i = 0; i < token_size; ++i) {
            if(str[i] != token[i]){
                start = SQFalse;
                break;
            }
        }
    }
    else {
        start = SQFalse;
    }
    sq_pushbool(v, start);
    return 1;
}

static SQRESULT string_find_close_quote(HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, src);
    SQ_OPT_INTEGER(v, 2, init, 0);
    SQ_OPT_INTEGER(v, 3, quote, '"');
    if(init >= src_size) return sq_throwerror(v, _SC("invalid start position"));

    for(; init < src_size; ++init) {
        if(src[init] == quote){
             if(src[init+1] == quote) ++init; //skip quoted quote
             else break;
        }
    }
    if(src[init] != quote) init = -1;
    sq_pushinteger(v, init);
    return 1;
}

static SQRESULT string_strchr(HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, src);
    SQ_GET_INTEGER(v, 2, delimiter);
    SQ_OPT_INTEGER(v, 3, offset, 0);
    if(offset > src_size) return sq_throwerror(v, _SC("offset bigger than string size"));
    const SQChar *token = scstrchr(src+offset, delimiter);
    sq_pushinteger(v, (token ? (SQInteger)(token-src) : -1));
    return 1;
}

static SQRESULT string_strncmp(HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, str1);
    SQ_GET_INTEGER(v, 2, offset);
    SQ_GET_STRING(v, 3, str2);
    SQ_OPT_INTEGER(v, 4, n, str2_size);
    if(offset > str1_size) return sq_throwerror(v, _SC("offset bigger than string size"));
    sq_pushinteger(v, scstrncmp(str1+offset, str2, str2_size));
    return 1;
}

static SQRESULT string_countchr(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);
    SQ_GET_INTEGER(v, 2, ch);

    SQInteger count = 0;
    for(SQInteger i=0; i < src_size; ++i) {
        if(src[i] == ch) ++count;
    }
    sq_pushinteger(v, count);
    return 1;
}

static SQRESULT string_find_delimiter(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);
    SQ_GET_INTEGER(v, 2, delimiter);
    SQ_GET_INTEGER(v, 3, escape_char);
    SQ_GET_INTEGER(v, 4, init);
    if(init >= src_size) return sq_throwerror(v, _SC("invalid start position"));

    for(; init < src_size; ++init) {
        if(src[init] == delimiter){
            int i = 1;
            if(src[init-i] != escape_char) break;
            while(src[init- ++i] == escape_char);
            if(((i-1) % 2) == 0) break; //non escaped escaped_char
        }
    }
    if((init >= src_size) || (src[init] != delimiter)) init = -1;
    sq_pushinteger(v, init);
    return 1;
}

static SQRESULT string_reverse (HSQUIRRELVM v) {
  SQInteger i;
  SQ_FUNC_VARS_NO_TOP(v);
  SQ_GET_STRING(v, 1, s)
  SQChar *data = sq_getscratchpad(v,s_size);
  --s_size;
  for(i=0; i<=s_size ; ++i){
      data[i] = s[s_size-i];
  }
  sq_pushstring(v, data, s_size+1);
  return 1;
}


static SQRESULT string_rep (HSQUIRRELVM v) {
  SQInteger i;
  SQ_FUNC_VARS_NO_TOP(v);
  SQ_GET_STRING(v, 1, s)
  SQ_GET_INTEGER(v, 2, n);
  //FIXME should check all number parameters that need be positive|negative only
  if(n < 0)  return sq_throwerror(v, _SC("only positive number allowed"));
  SQInteger nsize = n*s_size;
  SQChar *data = sq_getscratchpad(v, nsize);
  //FIXME all calls to sq_getscratchpad should check for NULL pointer
  if(!data) return sq_throwerror(v, _SC("not enough memory"));
  for(i=0; i<n ; ++i){
      memcpy(data+(i*s_size), s, s_size);
  }
  sq_pushstring(v, data, nsize);
  return 1;
}

static SQRESULT string_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_STRING))?1:SQ_ERROR;
}

// Based on utf8_check.c by Markus Kuhn, 2005
// https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
// Optimized for predominantly 7-bit content by Alex Hultman, 2016
// Licensed as Zlib, like the rest of this project
static bool isValidUtf8(const unsigned char *s, size_t length)
{
    for (const unsigned char *e = s + length; s != e; ) {
        if (s + 4 <= e && ((*(const SQUnsignedInteger32 *) s) & 0x80808080) == 0) {
            s += 4;
        } else {
            while (!(*s & 0x80)) {
                if (++s == e) {
                    return true;
                }
            }

            if ((s[0] & 0x60) == 0x40) {
                if (s + 1 >= e || (s[1] & 0xc0) != 0x80 || (s[0] & 0xfe) == 0xc0) {
                    return false;
                }
                s += 2;
            } else if ((s[0] & 0xf0) == 0xe0) {
                if (s + 2 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
                        (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) || (s[0] == 0xed && (s[1] & 0xe0) == 0xa0)) {
                    return false;
                }
                s += 3;
            } else if ((s[0] & 0xf8) == 0xf0) {
                if (s + 3 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80 ||
                        (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) || (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) {
                    return false;
                }
                s += 4;
            } else {
                return false;
            }
        }
    }
    return true;
}

static SQRESULT string_isvalidutf8(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, src);

    sq_pushbool(v, isValidUtf8((const unsigned char*)src, src_size));
    return 1;
}

//adapted from https://www.techiedelight.com/longest-common-substring-problem/
static const SQChar * cstrLCS(const SQChar *X, const SQChar *Y, SQInteger X_sz, SQInteger Y_sz, SQInteger *result_sz)
{
	SQInteger maxlen = 0;			// stores the max length of LCS
	SQInteger endingIndex = 0;	// stores the ending index of LCS in X
	SQInteger m = X_sz;
	SQInteger n = Y_sz;

	// lookup[i][j] stores the length of LCS of substring
	// X[0..i-1], Y[0..j-1]
	int lookup[m + 1][n + 1];

	// initialize all cells of lookup table to 0
	memset(lookup, 0, sizeof(lookup));

	// fill the lookup table in bottom-up manner
	for (SQInteger i = 1; i <= m; ++i)
	{
		for (SQInteger j = 1; j <= n; ++j)
		{
			// if current character of X and Y matches
			if (X[i - 1] == Y[j - 1])
			{
				lookup[i][j] = lookup[i - 1][j - 1] + 1;

				// update the maximum length and ending index
				if (lookup[i][j] > maxlen)
				{
					maxlen = lookup[i][j];
					endingIndex = i;
				}
			}
		}
	}

	// return Longest common substring having length maxlen
	*result_sz = maxlen;
	return X+(endingIndex - maxlen);
}

static SQRESULT string_longestcommonsubstr(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, strX);
    SQ_GET_STRING(v, 2, strY);
    SQInteger result_sz;
    const SQChar *result = cstrLCS(strX, strY, strX_size, strY_size, &result_sz);
    sq_pushstring(v, result, result_sz);
    return 1;
}

//DAD end

static void __strip_l(const SQChar *str,const SQChar **start)
{
	const SQChar *t = str;
	while(((*t) != '\0') && scisspace(*t)){ t++; }
	*start = t;
}

static void __strip_r(const SQChar *str,SQInteger len,const SQChar **end)
{
	if(len == 0) {
		*end = str;
		return;
	}
	const SQChar *t = &str[len-1];
	while(t >= str && scisspace(*t)) { t--; }
	*end = t + 1;
}

static SQRESULT string_strip(HSQUIRRELVM v)
{
	const SQChar *str,*start,*end;
	sq_getstring(v,1,&str);
	SQInteger len = sq_getsize(v,1);
	__strip_l(str,&start);
	__strip_r(str,len,&end);
	sq_pushstring(v,start,end - start);
	return 1;
}

static SQRESULT string_lstrip(HSQUIRRELVM v)
{
	const SQChar *str,*start;
	sq_getstring(v,1,&str);
	__strip_l(str,&start);
	sq_pushstring(v,start,-1);
	return 1;
}

static SQRESULT string_rstrip(HSQUIRRELVM v)
{
	const SQChar *str,*end;
	sq_getstring(v,1,&str);
	SQInteger len = sq_getsize(v,1);
	__strip_r(str,len,&end);
	sq_pushstring(v,str,end - str);
	return 1;
}

static SQRESULT string_split_csv(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_INTEGER(v, 2, sep);
    if((sep > 0xFF) || (sep < 0)) return sq_throwerror(v,_SC("character separator out of range 0..255"));
    const SQChar *token;
    sq_newarray(v,0);
    while ((token = scstrchr(str, sep)) != NULL) {
        sq_pushstring(v, str, token - str);
        sq_arrayappend(v, -2);
        str = token + 1;
    }
    if(*str){ //there is anything left ?
        sq_pushstring(v, str, -1);
        sq_arrayappend(v, -2);
    } else if( str_size && (*(str-1) == sep) ){ //last empty column ?
        sq_pushstring(v, _SC(""), 0);
        sq_arrayappend(v, -2);
    }
    return 1;
}

static SQRESULT string_split(HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQObjectType rtype = sq_gettype(v, 2);
    if(rtype == OT_STRING)
    {
        const SQChar *str,*seps;
        SQChar *stemp,*tok;
        sq_getstring(v,1,&str);
        sq_getstring(v,2,&seps);
        if(sq_getsize(v,2) == 0) return sq_throwerror(v,_SC("empty separators string"));
        SQInteger memsize = (sq_getsize(v,1)+1)*sizeof(SQChar);
        stemp = sq_getscratchpad(v,memsize);
        memcpy(stemp,str,memsize);
        tok = scstrtok(stemp,seps);
        sq_newarray(v,0);
        while( tok != NULL ) {
            sq_pushstring(v,tok,-1);
            sq_arrayappend(v,-2);
            tok = scstrtok( NULL, seps );
        }
    }
    else if(rtype == OT_INTEGER)
    {
        const SQChar *token;
        SQ_GET_STRING(v, 1, str);
        SQ_GET_INTEGER(v, 2, sep);
        if((sep > 0xFF) || (sep < 0)) return sq_throwerror(v,_SC("character separator out of range 0..255"));
        sq_newarray(v,0);
        while ((token = scstrchr(str, sep)) != NULL) {
            SQInteger sz = token - str;
            if(sz > 0)
            {
                sq_pushstring(v, str, token - str);
                sq_arrayappend(v, -2);
            }
            str = token + 1;
        }
        if(*str){ //there is anything left ?
            sq_pushstring(v, str, -1);
            sq_arrayappend(v, -2);
        }
    }
    return 1;
}

static SQRESULT string_isempty(HSQUIRRELVM v)
{
	sq_pushbool(v,sq_getsize(v,1) == 0);
	return 1;
}

#define string_char_is(name) \
static SQRESULT string_##name(HSQUIRRELVM v)\
{\
    SQ_FUNC_VARS_NO_TOP(v);\
    SQ_GET_STRING(v, 1, str);\
    SQ_GET_INTEGER(v, 2, idx);\
    if(idx >= str_size) {\
        return sq_throwerror(v, _SC("index " _PRINT_INT_FMT " out of range"), idx);\
    }\
	sq_pushbool(v, sc##name(str[idx]));\
	return 1;\
}

string_char_is(isspace);
string_char_is(isprint);
string_char_is(isalpha);
string_char_is(isalnum);
string_char_is(isdigit);
string_char_is(isxdigit);
string_char_is(iscntrl);
string_char_is(islower);
string_char_is(isupper);

static SQRESULT string_count_char(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_INTEGER(v, 2, char_to_count);
    SQInteger i, count = 0;
    for(i=0; i < str_size; ++i) {
        if(str[i] == char_to_count)
        {
            ++count;
        }
    }
	sq_pushinteger(v, count);
	return 1;
}

static SQRESULT string_uchar(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_INTEGER(v, 2, char_idx);
    if((char_idx >= str_size) || (char_idx < 0))
    {
        return sq_throwerror(v, _SC("index out of range"));
    }
	sq_pushinteger(v, (SQUChar)(str[char_idx]));
	return 1;
}

static SQRESULT string_ushort(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQ_GET_INTEGER(v, 2, char_idx);
    if(((char_idx*((SQInteger)sizeof(SQUnsignedInt16))) >= str_size) || (char_idx < 0))
    {
        return sq_throwerror(v, _SC("index out of range"));
    }
	sq_pushinteger(v, (((const SQUnsignedInt16*)str)[char_idx]));
	return 1;
}


#define MMIN(a,b) (((a)<(b))?(a):(b))
static SQRESULT string_edit_distance (HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, s1);
    SQ_GET_STRING(v, 2, s2);
    SQ_OPT_INTEGER(v, 3, max_size, 1024);

	SQInteger k, i, j, cost, array_size, *d, result = -1;

	if ( s1_size < max_size && s2_size < max_size )
    {
        if( s1_size != 0 && s2_size != 0){
            array_size = (sizeof(*d))*(++s2_size)*(++s1_size);
            d=(SQInteger*)sq_getscratchpad(v, array_size);
            for(k=0;k<s1_size;++k){
                d[k]=k;
            }
            for(k=0;k<s2_size;++k){
                d[k*s1_size]=k;
            }
            for(i=1;i<s1_size;++i){
                for(j=1;j<s2_size;++j){
                    if(s1[i-1]==s2[j-1])
                        cost=0;
                    else
                        cost=1;
                    d[j*s1_size+i]=MMIN(MMIN( d[(j-1)*s1_size+i]+1, d[j*s1_size+i-1]+1 ), d[(j-1)*s1_size+i-1]+cost );
                }
            }
            result=d[s1_size*s2_size-1];
        }
        else {
            result = (s1_size>s2_size)?s1_size:s2_size;
        }
	}

	sq_pushinteger(v, result);
    return 1;
}

#ifdef SQ_SUBLATIN
#include "sublatin.h"

static SQRESULT string_sl_len (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    sq_pushinteger(v, strLenSubSetLatinUtf8(str));
    return 1;
}

static SQRESULT string_sl_lower (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQInteger size = str_size+sizeof(SQChar); //'\0' terminator
    SQChar *s = sq_getscratchpad(v, size);
    memcpy(s, str, size);
    toLowerSubSetLatinUtf8(s);
    sq_pushstring(v, s, -1);
    return 1;
}


static SQRESULT string_sl_upper (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQInteger size = str_size+sizeof(SQChar); //'\0' terminator
    SQChar *s = sq_getscratchpad(v, size);
    memcpy(s, str, size);
    toUpperSubSetLatinUtf8(s);
    sq_pushstring(v, s, -1);
    return 1;
}

static SQRESULT string_sl_deaccent (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQInteger size = str_size+sizeof(SQChar); //'\0' terminator
    SQChar *s = sq_getscratchpad(v, size);
    memcpy(s, str, size);
    deAccentSubSetLatinUtf8(s);
    sq_pushstring(v, s, -1);
    return 1;
}

static SQRESULT string_sl_lower_deaccent (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQInteger size = str_size+sizeof(SQChar); //'\0' terminator
    SQChar *s = sq_getscratchpad(v, size);
    memcpy(s, str, size);
    toLowerDeaccentSubSetLatinUtf8(s);
    sq_pushstring(v, s, -1);
    return 1;
}

static SQRESULT string_sl_icmp (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, sl);
    SQ_GET_STRING(v, 2, sr);
    sq_pushinteger(v, strICmpSubSetLatinUtf8(sl, sr));
    return 1;
}

static SQRESULT string_sl_icmp_noaccents (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, sl);
    SQ_GET_STRING(v, 2, sr);
    sq_pushinteger(v, strICmpSubSetLatinUtf8NoAccents(sl, sr));
    return 1;
}

static SQRESULT string_sl_cmp_noaccents (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, sl);
    SQ_GET_STRING(v, 2, sr);
    sq_pushinteger(v, strCmpSubSetLatinUtf8NoAccents(sl, sr));
    return 1;
}

static SQRESULT string_sl_like_cmp (HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, sl);
    SQ_GET_STRING(v, 2, sr);
    SQ_OPT_INTEGER(v, 3, s_esc, _SC('%'));
    sq_pushbool(v, subLatinLikeCompare(sl, sr, s_esc) == 1);
    return 1;
}

static SQRESULT string_sl_like_cmp_noaccents (HSQUIRRELVM v) {
    SQ_FUNC_VARS(v);
    SQ_GET_STRING(v, 1, sl);
    SQ_GET_STRING(v, 2, sr);
    SQ_OPT_INTEGER(v, 3, s_esc, _SC('%'));
    sq_pushbool(v, subLatinLikeCompareNoAccents(sl, sr, s_esc) == 1);
    return 1;
}

#endif

static int mod_97_10(const char *snum)
{
    char s9[12];
    int result = 0;
    int step = 9;
    int n97 = 97;
    int slen = strlen(snum);

    strncpy(s9, snum, step);
    int i9 = atoi(s9);
    result = i9 % n97;
    slen -= step;
    snum += step;

    step = 7;
    while(slen > 0)
    {
        //snprintf(s9, sizeof(s9), "%.2d", result);
        //strncpy(s9+2, snum, 7);
        snprintf(s9, sizeof(s9), "%.2d%.7s", result, snum);
        i9 = atoi(s9);
        result = i9 % n97;
        slen -= step;
        snum += step;
    }
    return 98 - result;
}

static SQRESULT string_mod_97_10 (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    sq_pushinteger(v, mod_97_10(str));
    return 1;
}

static SQRESULT string_iso88959_to_utf8 (HSQUIRRELVM v) {
    SQ_FUNC_VARS_NO_TOP(v);
    SQ_GET_STRING(v, 1, str);
    SQInteger size = (str_size)+sizeof(SQChar); //'\0' terminator
    SQChar *buf = sq_getscratchpad(v, size*2);
    SQUChar *c = (SQUChar*)buf;
    const SQUChar *s = (const SQUChar*)str;
    for (; *s; ++s)
    {
        if (*s < 0x80)
        {
            *c++ = *s;
        }
        else
        {
            *c++ = (0xc0 | (0x03 & (*s >> 6)));
            *c++ = (0x80 | (*s & 0x3f));
        }
    }
    *c = '\0';
    sq_pushstring(v, buf, c - (SQUChar*)buf);
    return 1;
}


SQRegFunction SQSharedState::_string_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("s"), false},
	{_SC("size"),default_delegate_len,1, _SC("s"), false},
	{_SC("tointeger"),default_delegate_tointeger,-1, _SC("sn"), false},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("s"), false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("hash"),string_hash,1, _SC("s"), false},
	{_SC("slice"),string_slice,-1, _SC(" s n  n"), false},
	{_SC("substr"),string_substr,-2, _SC(" s n  n"), false},
	{_SC("replace"),string_replace,-3, _SC("sssi"), false},
	{_SC("find"),string_find,-2, _SC("s s n "), false},
	{_SC("indexOf"),string_find,-2, _SC("s s n "), false},
	{_SC("find_lua"),string_find_lua,-2, _SC("ss a|t|c n b n"), false},
	{_SC("find_close_quote"),string_find_close_quote,-1, _SC("sni"), false},
	{_SC("find_delimiter"),string_find_delimiter,4, _SC("siin"), false},
	{_SC("strchr"),string_strchr,-2, _SC("sii"), false},
	{_SC("strncmp"),string_strncmp,-3, _SC("sisi"), false},
	{_SC("countchr"),string_countchr,2, _SC("si"), false},
	{_SC("gsub"),string_gsub,-3, _SC("s s s|a|t|c n"), false},
	{_SC("gmatch"),string_gmatch, -3, _SC("s s c n n"), false},
	{_SC("match"), string_match, -2, _SC("s s n n"), false},
	{_SC("startswith"),string_startswith, 2, _SC("ss"), false},
	{_SC("endswith"),string_endswith, 2, _SC("ss"), false},
	{_SC("reverse"),string_reverse, 1, _SC("s"), false},
	{_SC("rep"),string_rep, 2, _SC("si"), false},
	{_SC("tolower"),string_tolower,1, _SC("s"), false},
	{_SC("toupper"),string_toupper,1, _SC("s"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("getdelegate"),string_getdelegate,1, _SC("."), false},
	{_SC("strip"),string_strip,1, _SC("s"), false},
	{_SC("trim"),string_strip,1, _SC("s"), false},
	{_SC("lstrip"),string_lstrip,1, _SC("s"), false},
	{_SC("ltrim"),string_lstrip,1, _SC("s"), false},
	{_SC("rstrip"),string_rstrip,1, _SC("s"), false},
	{_SC("rtrim"),string_rstrip,1, _SC("s"), false},
	{_SC("split"),string_split,2, _SC("s i|s"), false},
	{_SC("split_csv"),string_split_csv,2, _SC("si"), false},
	{_SC("isempty"),string_isempty,1, _SC("s"), false},
	{_SC("isspace"),string_isspace,2, _SC("si"), false},
	{_SC("isprint"),string_isprint,2, _SC("si"), false},
	{_SC("iscntrl"),string_iscntrl,2, _SC("si"), false},
	{_SC("isalpha"),string_isalpha,2, _SC("si"), false},
	{_SC("isalnum"),string_isalnum,2, _SC("si"), false},
	{_SC("isdigit"),string_isdigit,2, _SC("si"), false},
	{_SC("isxdigit"),string_isxdigit,2, _SC("si"), false},
	{_SC("islower"),string_islower,2, _SC("si"), false},
	{_SC("isupper"),string_isupper,2, _SC("si"), false},
	{_SC("count_char"),string_count_char,2, _SC("si"), false},
	{_SC("uchar"),string_uchar,2, _SC("si"), false},
	{_SC("ushort"),string_ushort,2, _SC("si"), false},
	{_SC("edit_distance"),string_edit_distance,-2, _SC("ssi"), false},
	{_SC("mod_97_10"),string_mod_97_10,1, _SC("s"), false},
	{_SC("iso88959_to_utf8"),string_iso88959_to_utf8,1, _SC("s"), false},
	{_SC("isvalidutf8"),string_isvalidutf8,1, _SC("s"), false},
	{_SC("longestcommonsubstr"),string_longestcommonsubstr,2, _SC("ss"), false},

#ifdef SQ_SUBLATIN
	{_SC("sl_len"),string_sl_len,1, _SC("s"), false},
	{_SC("sl_lower"),string_sl_lower,1, _SC("s"), false},
	{_SC("sl_upper"),string_sl_upper,1, _SC("s"), false},
	{_SC("sl_deaccent"),string_sl_deaccent,1, _SC("s"), false},
	{_SC("sl_lower_deaccent"),string_sl_lower_deaccent,1, _SC("s"), false},
	{_SC("sl_icmp"),string_sl_icmp,2, _SC("ss"), false},
	{_SC("sl_icmp_noaccents"),string_sl_icmp_noaccents,2, _SC("ss"), false},
	{_SC("sl_cmp_noaccents"),string_sl_cmp_noaccents, 2, _SC("ss"), false},
	{_SC("sl_like_cmp"),string_sl_like_cmp, -2, _SC("ssi"), false},
	{_SC("sl_like_cmp_noaccents"),string_sl_like_cmp_noaccents, -2, _SC("ssi"), false},
#endif
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//NUMBER DEFAULT DELEGATE//////////////////////////
static SQRESULT number_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_INTEGER))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_number_default_delegate_funcz[]={
	{_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b"), false},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b"), false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("tochar"),number_delegate_tochar,1, _SC("n|b"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("getdelegate"),number_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
/*
//pcall removed because it doesn't work as documented use try/catch instead
static SQRESULT closure_pcall(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_call(v,sq_gettop(v)-1,SQTrue,SQFalse))?1:SQ_ERROR;
}
*/

static SQRESULT closure_call(HSQUIRRELVM v)
{
	SQObjectPtr &c = stack_get(v, -1);
	if (sq_type(c) == OT_CLOSURE && (_closure(c)->_function->_bgenerator == false))
	{
		return sq_tailcall(v, sq_gettop(v) - 1);
	}
	return SQ_SUCCEEDED(sq_call(v, sq_gettop(v) - 1, SQTrue, SQTrue)) ? 1 : SQ_ERROR;
}

static SQRESULT _closure_acall(HSQUIRRELVM v,SQBool raiseerror, SQBool v2)
{
	SQArrayBase *aparams=_array(stack_get(v, v2 ? 3 : 2));
	SQInteger nparams=aparams->Size();
	v->Push(stack_get(v,1));
	if(v2) v->Push(stack_get(v,2));
	for(SQInteger i=0;i<nparams;i++) v->Push((*aparams)[i]);
	return SQ_SUCCEEDED(sq_call(v,nparams + (v2 ? 1 : 0),SQTrue,raiseerror))?1:SQ_ERROR;
}

static SQRESULT closure_acall(HSQUIRRELVM v)
{
	return _closure_acall(v,SQTrue, SQFalse);
}

static SQRESULT closure_acall2(HSQUIRRELVM v)
{
	return _closure_acall(v,SQTrue, SQTrue);
}

static SQRESULT closure_pacall(HSQUIRRELVM v)
{
	return _closure_acall(v,SQFalse, SQFalse);
}

static SQRESULT closure_bindenv(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_bindenv(v,1)))
		return SQ_ERROR;
	return 1;
}

static SQRESULT closure_setenv(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_setfenv(v,1, SQFalse)))
		return SQ_ERROR;
	return 0;
}
static SQRESULT closure_getenv(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_getfenv(v,1, SQFalse)))
		return SQ_ERROR;
	return 1;
}

static SQRESULT closure_getinfos(HSQUIRRELVM v) {
	SQObject o = stack_get(v,1);
	SQTable *res = SQTable::Create(_ss(v),4);
	if(sq_type(o) == OT_CLOSURE) {
		SQFunctionProto *f = _closure(o)->_function;
		SQInteger nparams = f->_nparameters + (f->_varparams?1:0);
		SQObjectPtr params = SQArray::Create(_ss(v),nparams);
		SQObjectPtr params_type = SQArray::Create(_ss(v),nparams);
        SQObjectPtr defparams = SQArray::Create(_ss(v),f->_ndefaultparams);
		for(SQInteger n = 0; n<f->_nparameters; n++) {
			_array(params)->Set((SQInteger)n,f->_parameters[n]);
			_array(params_type)->Set((SQInteger)n,f->_parameters_type[n]);
		}
    for(SQInteger j = 0; j<f->_ndefaultparams; j++) {
			_array(defparams)->Set((SQInteger)j,_closure(o)->_defaultparams[j]);
		}
		if(f->_varparams) {
			_array(params)->Set(nparams-1,SQString::Create(_ss(v),_SC("..."),-1));
		}
		res->NewSlot(SQString::Create(_ss(v),_SC("native"),-1),false);
		res->NewSlot(SQString::Create(_ss(v),_SC("name"),-1),f->_name);
		res->NewSlot(SQString::Create(_ss(v),_SC("return_type"),-1),f->_return_type);
		res->NewSlot(SQString::Create(_ss(v),_SC("src"),-1),f->_sourcename);
		res->NewSlot(SQString::Create(_ss(v),_SC("parameters"),-1),params);
		res->NewSlot(SQString::Create(_ss(v),_SC("parameters_type"),-1),params_type);
		res->NewSlot(SQString::Create(_ss(v),_SC("varargs"),-1),f->_varparams);
    res->NewSlot(SQString::Create(_ss(v),_SC("defparams"),-1),defparams);
	}
	else { //OT_NATIVECLOSURE
		SQNativeClosure *nc = _nativeclosure(o);
		res->NewSlot(SQString::Create(_ss(v),_SC("native"),-1),true);
		res->NewSlot(SQString::Create(_ss(v),_SC("name"),-1),nc->_name);
		res->NewSlot(SQString::Create(_ss(v),_SC("paramscheck"),-1),(SQInteger)nc->_nparamscheck);
		SQObjectPtr typecheck;
		if(nc->_typecheck.size() > 0) {
			typecheck =
				SQArray::Create(_ss(v), nc->_typecheck.size());
			for(SQUnsignedInteger n = 0; n<nc->_typecheck.size(); n++) {
					_array(typecheck)->Set((SQInteger)n,nc->_typecheck[n]);
			}
		}
		res->NewSlot(SQString::Create(_ss(v),_SC("typecheck"),-1),typecheck);
	}
	v->Push(res);
	return 1;
}

//CLOSURE DEFAULT DELEGATE//////////////////////////
static SQRESULT closure_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_CLOSURE))?1:SQ_ERROR;
}


SQRegFunction SQSharedState::_closure_default_delegate_funcz[]={
	{_SC("call"),closure_call,-1, _SC("c"), false},
	//pcall removed because it doesn't work as documented use try/catch instead
	//{_SC("pcall"),closure_pcall,-1, _SC("c"), false},
	{_SC("acall"),closure_acall,2, _SC("ca"), false},
	{_SC("acall2"),closure_acall2,3, _SC("c.a"), false},
	{_SC("pacall"),closure_pacall,2, _SC("ca"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("bindenv"),closure_bindenv,2, _SC("c x|y|t|a"), false},
	{_SC("setenv"),closure_setenv,2, _SC("c x|y|t"), false},
	{_SC("getenv"),closure_getenv,1, _SC("c"), false},
	{_SC("getinfos"),closure_getinfos,1, _SC("c"), false},
	{_SC("getdelegate"),closure_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//GENERATOR DEFAULT DELEGATE
static SQRESULT generator_getstatus(HSQUIRRELVM v)
{
	SQObject &o=stack_get(v,1);
	switch(_generator(o)->_state){
		case SQGenerator::eSuspended:v->Push(SQString::Create(_ss(v),_SC("suspended")));break;
		case SQGenerator::eRunning:v->Push(SQString::Create(_ss(v),_SC("running")));break;
		case SQGenerator::eDead:v->Push(SQString::Create(_ss(v),_SC("dead")));break;
	}
	return 1;
}

//GENERATOR DEFAULT DELEGATE//////////////////////////
static SQRESULT generator_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_GENERATOR))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_generator_default_delegate_funcz[]={
	{_SC("getstatus"),generator_getstatus,1, _SC("g"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("getdelegate"),generator_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

//THREAD DEFAULT DELEGATE
static SQRESULT thread_call(HSQUIRRELVM v)
{
	SQObjectPtr o = stack_get(v,1);
	if(sq_type(o) == OT_THREAD) {
		SQInteger nparams = sq_gettop(v);
		_thread(o)->Push(_thread(o)->_roottable);
		for(SQInteger i = 2; i<(nparams+1); i++)
			sq_move(_thread(o),v,i);
		if(SQ_SUCCEEDED(sq_call(_thread(o),nparams,SQTrue,SQTrue))) {
			sq_move(v,_thread(o),-1);
			sq_pop(_thread(o),1);
			return 1;
		}
		v->_lasterror = _thread(o)->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

static SQRESULT thread_wakeup(HSQUIRRELVM v)
{
	SQObjectPtr o = stack_get(v,1);
	if(sq_type(o) == OT_THREAD) {
		SQVM *thread = _thread(o);
		SQInteger state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,_SC("cannot wakeup a running thread"));
			}
		}

		SQInteger wakeupret = sq_gettop(v)>1?1:0;
		if(wakeupret) {
			sq_move(thread,v,2);
		}
		if(SQ_SUCCEEDED(sq_wakeupvm(thread,wakeupret,SQTrue,SQTrue,SQFalse))) {
			sq_move(v,thread,-1);
			sq_pop(thread,1); //pop retval
			if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
				sq_settop(thread,1); //pop roottable
			}
			return 1;
		}
		sq_settop(thread,1);
		v->_lasterror = thread->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

static SQRESULT thread_getstatus(HSQUIRRELVM v)
{
	SQObjectPtr &o = stack_get(v,1);
	switch(sq_getvmstate(_thread(o))) {
		case SQ_VMSTATE_IDLE:
			sq_pushstring(v,_SC("idle"),-1);
		break;
		case SQ_VMSTATE_RUNNING:
			sq_pushstring(v,_SC("running"),-1);
		break;
		case SQ_VMSTATE_SUSPENDED:
			sq_pushstring(v,_SC("suspended"),-1);
		break;
		default:
			return sq_throwerror(v,_SC("internal VM error"));
	}
	return 1;
}

static SQRESULT thread_getstackinfos(HSQUIRRELVM v)
{
	SQObjectPtr o = stack_get(v,1);
	if(sq_type(o) == OT_THREAD) {
		SQVM *thread = _thread(o);
		SQInteger threadtop = sq_gettop(thread);
		SQInteger level;
		sq_getinteger(v,-1,&level);
		SQRESULT res = __getcallstackinfos(thread,level);
		if(SQ_FAILED(res))
		{
			sq_settop(thread,threadtop);
			if(sq_type(thread->_lasterror) == OT_STRING) {
				sq_throwerror(v,_SC("%s"),_stringval(thread->_lasterror));
			}
			else {
				sq_throwerror(v,_SC("unknown error"));
			}
		}
		if(res > 0) {
			//some result
			sq_move(v,thread,-1);
			sq_settop(thread,threadtop);
			return 1;
		}
		//no result
		sq_settop(thread,threadtop);
		return 0;

	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

//THREAD DEFAULT DELEGATE//////////////////////////
static SQRESULT thread_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_THREAD))?1:SQ_ERROR;
}


SQRegFunction SQSharedState::_thread_default_delegate_funcz[] = {
	{_SC("call"), thread_call, -1, _SC("v"), false},
	{_SC("wakeup"), thread_wakeup, -1, _SC("v"), false},
	{_SC("getstatus"), thread_getstatus, 1, _SC("v"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("getstackinfos"),thread_getstackinfos,2, _SC("vn"), false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("getdelegate"),thread_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL,false},
};

static SQRESULT class_getattributes(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getattributes(v,-2))?1:SQ_ERROR;
}

static SQRESULT class_setattributes(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_setattributes(v,-3))?1:SQ_ERROR;
}

static SQRESULT class_instance(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_createinstance(v,-1))?1:SQ_ERROR;
}

static SQRESULT class_getbase(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getbase(v,-1))?1:SQ_ERROR;
}

static SQRESULT class_newmember(HSQUIRRELVM v)
{
	SQInteger top = sq_gettop(v);
	SQBool bstatic = SQFalse;
	if(top == 5)
	{
		sq_getbool(v,-1,&bstatic);
		sq_pop(v,1);
	}

	if(top < 4) {
		sq_pushnull(v);
	}
	return SQ_SUCCEEDED(sq_newmember(v,-4,bstatic))?1:SQ_ERROR;
}

static SQRESULT class_rawnewmember(HSQUIRRELVM v)
{
	SQInteger top = sq_gettop(v);
	SQBool bstatic = SQFalse;
	if(top == 5)
	{
		sq_getbool(v,-1,&bstatic);
		sq_pop(v,1);
	}

	if(top < 4) {
		sq_pushnull(v);
	}
	return SQ_SUCCEEDED(sq_rawnewmember(v,-4,bstatic))?1:SQ_ERROR;
}

static SQRESULT class_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_CLASS))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_class_default_delegate_funcz[] = {
	{_SC("getattributes"), class_getattributes, 2, _SC("y."), false},
	{_SC("setattributes"), class_setattributes, 3, _SC("y.."), false},
	{_SC("get"),container_get,-2, _SC("y"), false},
	{_SC("rawget"),container_rawget,-2, _SC("y"), false},
	{_SC("rawset"),container_rawset,3, _SC("y"), false},
	{_SC("rawin"),container_rawexists,2, _SC("y"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("instance"),class_instance,1, _SC("y"), false},
	{_SC("getbase"),class_getbase,1, _SC("y"), false},
	{_SC("newmember"),class_newmember,-3, _SC("y"), false},
	{_SC("rawnewmember"),class_rawnewmember,-3, _SC("y"), false},
	{_SC("getdelegate"),class_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};


static SQRESULT instance_getclass(HSQUIRRELVM v)
{
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

static SQRESULT instance_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_INSTANCE))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_instance_default_delegate_funcz[] = {
	{_SC("getclass"), instance_getclass, 1, _SC("x"), false},
	{_SC("get"),container_get,-2, _SC("x"), false},
	{_SC("rawget"),container_rawget,-2, _SC("x"), false},
	{_SC("rawset"),container_rawset,3, _SC("x"), false},
	{_SC("rawin"),container_rawexists,2, _SC("x"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("getdelegate"),instance_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL, false}
};

static SQRESULT weakref_ref(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_getweakrefval(v,1)))
		return SQ_ERROR;
	return 1;
}

static SQRESULT weakref_getdelegate(HSQUIRRELVM v)
{
	return SQ_SUCCEEDED(sq_getdefaultdelegate(v,OT_WEAKREF))?1:SQ_ERROR;
}

SQRegFunction SQSharedState::_weakref_default_delegate_funcz[] = {
	{_SC("ref"),weakref_ref,1, _SC("r"), false},
	{_SC("weakref"),obj_delegate_weakref,1, NULL, false},
	{_SC("tostring"),default_delegate_tostring,1, _SC("."), false},
	{_SC("getdelegate"),weakref_getdelegate,1, _SC("."), false},
	{NULL,(SQFUNCTION)0,0,NULL,false}
};


