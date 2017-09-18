SQUIRREL_API_FUNC(SQRESULT, preload_modules, (HSQUIRRELVM v, const sq_modules_preload_st *modules))
SQUIRREL_API_FUNC(SQFUNCTION, get_preload_module_func, (HSQUIRRELVM v, const SQChar *module_name))

/*vm*/
SQUIRREL_API_FUNC(HSQUIRRELVM, open, (SQInteger initialstacksize))
SQUIRREL_API_FUNC(HSQUIRRELVM, newthread, (HSQUIRRELVM friendvm, SQInteger initialstacksize))
SQUIRREL_API_FUNC(void, seterrorhandler, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, geterrorhandler, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setatexithandler, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, getatexithandler, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, close, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setforeignptr, (HSQUIRRELVM v,SQUserPointer p))
SQUIRREL_API_FUNC(SQUserPointer, getforeignptr, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setsharedforeignptr, (HSQUIRRELVM v,SQUserPointer p))
SQUIRREL_API_FUNC(SQUserPointer, getsharedforeignptr, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setvmreleasehook, (HSQUIRRELVM v,SQRELEASEHOOK hook))
SQUIRREL_API_FUNC(SQRELEASEHOOK, getvmreleasehook, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setsharedreleasehook, (HSQUIRRELVM v,SQRELEASEHOOK hook))
SQUIRREL_API_FUNC(SQRELEASEHOOK, getsharedreleasehook, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setprintfunc, (HSQUIRRELVM v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc))
SQUIRREL_API_FUNC(SQPRINTFUNCTION, getprintfunc, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQPRINTFUNCTION, geterrorfunc, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, suspendvm, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, wakeupvm, (HSQUIRRELVM v,SQBool resumedret,SQBool retval,SQBool raiseerror,SQBool throwerror))
SQUIRREL_API_FUNC(SQInteger, getvmstate, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQInteger, getversion, ())
SQUIRREL_API_FUNC(void, set_include_path, (HSQUIRRELVM v, const SQChar *include_path))
SQUIRREL_API_FUNC(const SQChar*, get_include_path, (HSQUIRRELVM v))

/*compiler*/
#ifndef SQ_MAX_INCLUDE_FILES
#define SQ_MAX_INCLUDE_FILES 10
#endif
SQUIRREL_API_FUNC(SQRESULT, compile, (HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,
                                 SQBool raiseerror, SQBool show_warnings, SQInteger max_nested_includes))
SQUIRREL_API_FUNC(SQRESULT, compilebuffer, (HSQUIRRELVM v,const SQChar *s,SQInteger size,const SQChar *sourcename,
                                       SQBool raiseerror, SQBool show_warnings, SQInteger max_nested_includes))
SQUIRREL_API_FUNC(void, enabledebuginfo, (HSQUIRRELVM v, SQBool enable))
SQUIRREL_API_FUNC(void, notifyallexceptions, (HSQUIRRELVM v, SQBool enable))
SQUIRREL_API_FUNC(void, setcompilererrorhandler, (HSQUIRRELVM v,SQCOMPILERERROR f))

/*stack operations*/
SQUIRREL_API_FUNC(void, push, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(void, pop, (HSQUIRRELVM v,SQInteger nelemstopop))
SQUIRREL_API_FUNC(void, poptop, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, remove, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(void, insert, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(void, replace, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQInteger, gettop, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, settop, (HSQUIRRELVM v,SQInteger newtop))
SQUIRREL_API_FUNC(SQRESULT, reservestack, (HSQUIRRELVM v,SQInteger nsize))
SQUIRREL_API_FUNC(SQInteger, compare, (HSQUIRRELVM v, SQInteger idx1, SQInteger idx2))
SQUIRREL_API_FUNC(SQInteger, cmp, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, move, (HSQUIRRELVM dest,HSQUIRRELVM src,SQInteger idx))

/*object creation handling*/
SQUIRREL_API_FUNC(SQUserPointer, newuserdata, (HSQUIRRELVM v,SQUnsignedInteger size))
SQUIRREL_API_FUNC(void, newtable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, newtableex, (HSQUIRRELVM v,SQInteger initialcapacity))
SQUIRREL_API_FUNC(void, newarray, (HSQUIRRELVM v,SQInteger size))
SQUIRREL_API_FUNC(void, newclosure, (HSQUIRRELVM v,SQFUNCTION func,SQUnsignedInteger nfreevars))
SQUIRREL_API_FUNC(SQRESULT, setparamscheck, (HSQUIRRELVM v,SQInteger nparamscheck,const SQChar *typemask))
SQUIRREL_API_FUNC(SQRESULT, setfenv, (HSQUIRRELVM v,SQInteger idx, SQBool cloning))
SQUIRREL_API_FUNC(SQRESULT, getfenv, (HSQUIRRELVM v,SQInteger idx, SQBool roottable_when_null))
SQUIRREL_API_FUNC(SQRESULT, bindenv, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, setclosureroot, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getclosureroot, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(void, pushstring, (HSQUIRRELVM v,const SQChar *s,SQInteger len))
SQUIRREL_API_FUNC(void, pushfstring, (HSQUIRRELVM v,const SQChar *fmt, ...))
SQUIRREL_API_FUNC(void, pushfloat, (HSQUIRRELVM v,SQFloat f))
SQUIRREL_API_FUNC(void, pushinteger, (HSQUIRRELVM v,SQInteger n))
SQUIRREL_API_FUNC(void, pushbool, (HSQUIRRELVM v,SQBool b))
SQUIRREL_API_FUNC(void, pushuserpointer, (HSQUIRRELVM v,SQUserPointer p))
SQUIRREL_API_FUNC(void, pushnull, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, pushthread, (HSQUIRRELVM v, HSQUIRRELVM thread))
SQUIRREL_API_FUNC(SQRESULT, checkoption, (HSQUIRRELVM v, SQInteger narg, const SQChar *def,
                                 const SQChar *const lst[]))
SQUIRREL_API_FUNC(SQObjectType, gettype, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(const SQChar*, gettypename, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, typeof, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQInteger, getsize, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQHash, gethash, (HSQUIRRELVM v, SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getbase, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQBool, instanceof, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, tostring, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, tobool, (HSQUIRRELVM v, SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, tointeger, (HSQUIRRELVM v, SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, tofloat, (HSQUIRRELVM v, SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getstring, (HSQUIRRELVM v,SQInteger idx,const SQChar **c))
SQUIRREL_API_FUNC(SQRESULT, getstr_and_size, (HSQUIRRELVM v,SQInteger idx,const SQChar **c, SQInteger *size))
SQUIRREL_API_FUNC(SQRESULT, getinteger, (HSQUIRRELVM v,SQInteger idx,SQInteger *i))
SQUIRREL_API_FUNC(SQRESULT, getinteger_ptr, (HSQUIRRELVM v,SQInteger idx,SQInteger **i))
SQUIRREL_API_FUNC(SQRESULT, getfloat, (HSQUIRRELVM v,SQInteger idx,SQFloat *f))
SQUIRREL_API_FUNC(SQRESULT, getfloat_ptr, (HSQUIRRELVM v,SQInteger idx,SQFloat **f))
SQUIRREL_API_FUNC(SQRESULT, getbool, (HSQUIRRELVM v,SQInteger idx,SQBool *b))
SQUIRREL_API_FUNC(SQRESULT, getthread, (HSQUIRRELVM v,SQInteger idx,HSQUIRRELVM *thread))
SQUIRREL_API_FUNC(SQRESULT, getuserpointer, (HSQUIRRELVM v,SQInteger idx,SQUserPointer *p))
SQUIRREL_API_FUNC(SQUserPointer, get_as_userpointer, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getuserdata, (HSQUIRRELVM v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag))
SQUIRREL_API_FUNC(SQRESULT, settypetag, (HSQUIRRELVM v,SQInteger idx,SQUserPointer typetag))
SQUIRREL_API_FUNC(SQRESULT, gettypetag, (HSQUIRRELVM v,SQInteger idx,SQUserPointer *typetag))
SQUIRREL_API_FUNC(void, setreleasehook, (HSQUIRRELVM v,SQInteger idx,SQRELEASEHOOK hook))
SQUIRREL_API_FUNC(SQRELEASEHOOK, getreleasehook, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQChar*, getscratchpad, (HSQUIRRELVM v,SQInteger minsize))
SQUIRREL_API_FUNC(SQRESULT, getfunctioninfo, (HSQUIRRELVM v,SQInteger level,SQFunctionInfo *fi))
SQUIRREL_API_FUNC(SQRESULT, getclosureinfo, (HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger *nparams,SQUnsignedInteger *nfreevars))
SQUIRREL_API_FUNC(SQRESULT, getclosurename, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, setnativeclosurename, (HSQUIRRELVM v,SQInteger idx,const SQChar *name))
SQUIRREL_API_FUNC(SQRESULT, setinstanceup, (HSQUIRRELVM v, SQInteger idx, SQUserPointer p))
SQUIRREL_API_FUNC(SQRESULT, getinstanceup, (HSQUIRRELVM v, SQInteger idx, SQUserPointer *p,SQUserPointer typetag))
SQUIRREL_API_FUNC(SQRESULT, setclassudsize, (HSQUIRRELVM v, SQInteger idx, SQInteger udsize))
SQUIRREL_API_FUNC(SQRESULT, newclass, (HSQUIRRELVM v,SQBool hasbase))
SQUIRREL_API_FUNC(SQRESULT, pushnewclass, (HSQUIRRELVM v, const SQChar *className,
                          const SQChar *parentName,
                          void *classTag, SQRegFunction *methods, SQBool leaveOnTop))
SQUIRREL_API_FUNC(SQRESULT, createinstance, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, setattributes, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getattributes, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getclass, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(void, weakref, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getdefaultdelegate, (HSQUIRRELVM v,SQObjectType t))
SQUIRREL_API_FUNC(SQRESULT, getmemberhandle, (HSQUIRRELVM v,SQInteger idx,HSQMEMBERHANDLE *handle))
SQUIRREL_API_FUNC(SQRESULT, getbyhandle, (HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle))
SQUIRREL_API_FUNC(SQRESULT, setbyhandle, (HSQUIRRELVM v,SQInteger idx,const HSQMEMBERHANDLE *handle))
SQUIRREL_API_FUNC(void, insertfunc, (HSQUIRRELVM sqvm, const SQChar *fname, SQFUNCTION func,
                        SQInteger nparamscheck, const SQChar *typemask, SQBool isStatic))
SQUIRREL_API_FUNC(void, insert_reg_funcs, (HSQUIRRELVM sqvm, SQRegFunction *obj_funcs))
//SQUIRREL_API_FUNC(const SQChar*, optstring, (HSQUIRRELVM sqvm, SQInteger idx, const SQChar *dflt, SQInteger *size))
SQUIRREL_API_FUNC(SQRESULT, optinteger, (HSQUIRRELVM sqvm, SQInteger idx, SQInteger *value, SQInteger default_value))

/*object manipulation*/
SQUIRREL_API_FUNC(void, pushroottable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, getonroottable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, setonroottable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, pushregistrytable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, getonregistrytable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, setonregistrytable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, delete_on_registry_table, (HSQUIRRELVM v, SQUserPointer uptr))
SQUIRREL_API_FUNC(void, pushconsttable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, setroottable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, setconsttable, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, newslot, (HSQUIRRELVM v, SQInteger idx, SQBool bstatic))
SQUIRREL_API_FUNC(SQRESULT, deleteslot, (HSQUIRRELVM v,SQInteger idx,SQBool pushval))
SQUIRREL_API_FUNC(SQRESULT, set, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, get, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getbyname, (HSQUIRRELVM v,SQInteger idx, const SQChar *key, SQInteger key_len))
SQUIRREL_API_FUNC(SQRESULT, rawget, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQBool, rawexists, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, rawset, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, rawdeleteslot, (HSQUIRRELVM v,SQInteger idx,SQBool pushval))
SQUIRREL_API_FUNC(SQRESULT, newmember, (HSQUIRRELVM v,SQInteger idx,SQBool bstatic))
SQUIRREL_API_FUNC(SQRESULT, rawnewmember, (HSQUIRRELVM v,SQInteger idx,SQBool bstatic))
SQUIRREL_API_FUNC(SQRESULT, arrayappend, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, arraypop, (HSQUIRRELVM v,SQInteger idx,SQBool pushval))
SQUIRREL_API_FUNC(SQRESULT, arrayresize, (HSQUIRRELVM v,SQInteger idx,SQInteger newsize))
SQUIRREL_API_FUNC(SQRESULT, arrayminsize, (HSQUIRRELVM v,SQInteger idx,SQInteger minsize))
SQUIRREL_API_FUNC(SQRESULT, arrayreverse, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, arrayremove, (HSQUIRRELVM v,SQInteger idx,SQInteger itemidx))
SQUIRREL_API_FUNC(SQRESULT, arrayinsert, (HSQUIRRELVM v,SQInteger idx,SQInteger destpos))
SQUIRREL_API_FUNC(SQRESULT, arrayget, (HSQUIRRELVM v,SQInteger idx,SQInteger pos))
SQUIRREL_API_FUNC(SQRESULT, arrayset, (HSQUIRRELVM v,SQInteger idx,SQInteger destpos))
SQUIRREL_API_FUNC(SQRESULT, setdelegate, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getdelegate, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, clone, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, setfreevariable, (HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval))
SQUIRREL_API_FUNC(SQRESULT, next, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getweakrefval, (HSQUIRRELVM v,SQInteger idx))
SQUIRREL_API_FUNC(SQRESULT, clear, (HSQUIRRELVM v,SQInteger idx))

/*calls*/
SQUIRREL_API_FUNC(SQRESULT, call, (HSQUIRRELVM v,SQInteger params,SQBool retval,SQBool raiseerror))
SQUIRREL_API_FUNC(SQRESULT, call_va_vl, (HSQUIRRELVM v, SQBool reset_stack, SQInteger idx, const SQChar *func,
                                    SQInteger idx_this, const SQChar *sig, va_list vl))
SQUIRREL_API_FUNC(SQRESULT, call_va, (HSQUIRRELVM v, SQBool reset_stack, SQInteger idx, const SQChar *func,
                                 SQInteger idx_this, const SQChar *sig, ...))
SQUIRREL_API_FUNC(SQRESULT, resume, (HSQUIRRELVM v,SQBool retval,SQBool raiseerror))
SQUIRREL_API_FUNC(const SQChar*, getlocal, (HSQUIRRELVM v,SQUnsignedInteger level,SQUnsignedInteger idx))
SQUIRREL_API_FUNC(SQRESULT, getcallee, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(const SQChar*, getfreevariable, (HSQUIRRELVM v,SQInteger idx,SQUnsignedInteger nval))
SQUIRREL_API_FUNC(SQRESULT, throwerror, (HSQUIRRELVM v,const SQChar *fmt, ...))
SQUIRREL_API_FUNC(SQRESULT, throwobject, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, reseterror, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, getlasterror, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(const SQChar*, getlasterror_str, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, getlasterror_line_col, (HSQUIRRELVM v, SQInteger *line, SQInteger *column))

/*raw object handling*/
SQUIRREL_API_FUNC(SQRESULT, getstackobj, (HSQUIRRELVM v,SQInteger idx,HSQOBJECT *po))
SQUIRREL_API_FUNC(void, pushobject, (HSQUIRRELVM v,HSQOBJECT obj))
SQUIRREL_API_FUNC(void, addref, (HSQUIRRELVM v,HSQOBJECT *po))
SQUIRREL_API_FUNC(SQBool, release, (HSQUIRRELVM v,HSQOBJECT *po))
SQUIRREL_API_FUNC(SQUnsignedInteger, getrefcount, (HSQUIRRELVM v,HSQOBJECT *po))
SQUIRREL_API_FUNC(void, resetobject, (HSQOBJECT *po))
SQUIRREL_API_FUNC(const SQChar*,objtostring, (const HSQOBJECT *o))
SQUIRREL_API_FUNC(SQBool, objtobool, (const HSQOBJECT *o))
SQUIRREL_API_FUNC(SQInteger, objtointeger, (const HSQOBJECT *o))
SQUIRREL_API_FUNC(SQFloat, objtofloat, (const HSQOBJECT *o))
SQUIRREL_API_FUNC(SQUserPointer, objtouserpointer, (const HSQOBJECT *o))
SQUIRREL_API_FUNC(SQRESULT, getobjtypetag, (const HSQOBJECT *o,SQUserPointer * typetag))
SQUIRREL_API_FUNC(SQUnsignedInteger, getvmrefcount, (HSQUIRRELVM v, const HSQOBJECT *po))


/*GC*/
SQUIRREL_API_FUNC(SQInteger, collectgarbage, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(SQRESULT, resurrectunreachable, (HSQUIRRELVM v))

/*serialization*/
SQUIRREL_API_FUNC(SQRESULT, writeclosure, (HSQUIRRELVM vm,SQWRITEFUNC writef,SQUserPointer up))
SQUIRREL_API_FUNC(SQRESULT, writeclosure_as_source, (HSQUIRRELVM vm,SQWRITEFUNC writef,SQUserPointer up))
SQUIRREL_API_FUNC(SQRESULT, readclosure, (HSQUIRRELVM vm,SQREADFUNC readf,SQUserPointer up))

/*mem allocation*/
SQUIRREL_API_FUNC(void*, malloc, (SQUnsignedInteger size))
SQUIRREL_API_FUNC(void*, realloc, (void* p,SQUnsignedInteger oldsize,SQUnsignedInteger newsize))
SQUIRREL_API_FUNC(void, free, (void *p,SQUnsignedInteger size))

/*debug*/
SQUIRREL_API_FUNC(SQRESULT, stackinfos, (HSQUIRRELVM v,SQInteger level,SQStackInfos *si))
SQUIRREL_API_FUNC(void, setdebughook, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, setnativedebughook, (HSQUIRRELVM v,SQDEBUGHOOK hook))
SQUIRREL_API_FUNC(SQInteger, getfulltop, (HSQUIRRELVM v))
SQUIRREL_API_FUNC(void, getlaststackinfo, (HSQUIRRELVM v))
