/* see copyright notice in squirrel.h */
#include <new>
#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

//#define SQSTD_BLOB_TYPE_TAG (SQSTD_STREAM_TYPE_TAG | 0x00000002)
static const SQChar  SQSTD_BLOB_TYPE_TAG[] = _SC("std_stream_blob");


//Blob


SQUserPointer SQBlob::SQBlob_TAG = (SQUserPointer)SQSTD_BLOB_TYPE_TAG;

void SQBlob::init(SQInteger size, SQInteger allocated) {
    _size = size;
    _allocated = allocated > size ? allocated : size;
    _buf = (unsigned char *)sq_malloc(_allocated);
    memset(_buf, 0, _allocated);
    _ptr = 0;
    _owns = true;
}
SQBlob::~SQBlob() {
    if(_buf) sq_free(_buf, _allocated);
}
SQInteger SQBlob::Write(const void *buffer, SQInteger size) {
    if(!CanAdvance(size)) {
        GrowBufOf(_ptr + size - _size);
    }
    memcpy(&_buf[_ptr], buffer, size);
    _ptr += size;
    return size;
}
SQInteger SQBlob::WriteZstr(const char *zStr) {
    if(zStr) //do not try to strlen(NULL)
    {
        SQInteger size = strlen(zStr);
        return Write(zStr, size);
    }
    return 0;
}
SQInteger SQBlob::WriteChar(const char c) {
    return Write(&c, 1);
}
SQInteger SQBlob::Read(void *buffer,SQInteger size) {
    SQInteger n = size;
    if(!CanAdvance(size)) {
        if((_size - _ptr) > 0)
            n = _size - _ptr;
        else return 0;
    }
    memcpy(buffer, &_buf[_ptr], n);
    _ptr += n;
    return n;
}
SQInteger SQBlob::Gets(char *buffer,SQInteger size) {
    SQInteger n = size;
    if(!CanAdvance(size)) {
        if((_size - _ptr) > 0)
            n = _size - _ptr;
        else return 0;
    }
    //because we are zero based wee need to use/return i+1 when i > 0
    SQInteger i=0;
    for(; i < n; ++i)
    {
        char c = _buf[_ptr+i];
        buffer[i] = c;
        if(c == '\n')
        {
            ++i;
            _ptr += i;
            return i;
        }
    }
    if(i)
    {
        ++i;
        _ptr += i;
    }
    return i;
}
bool SQBlob::Resize(SQInteger n) {
    if(!_owns) return false;
    if(n != _allocated) {
        unsigned char *newbuf = (unsigned char *)sq_malloc(n);
        memset(newbuf,0,n);
        if(_size > n)
            memcpy(newbuf,_buf,n);
        else
            memcpy(newbuf,_buf,_size);
        sq_free(_buf,_allocated);
        _buf=newbuf;
        _allocated = n;
        if(_size > _allocated)
            _size = _allocated;
        if(_ptr > _size)
            _ptr = _size;
    }
    return true;
}
bool SQBlob::Reserve(SQInteger n)
{
    bool ret = true;
    if(_size + n > _allocated) {
        if(_size + n > _size * 2)
            ret = Resize(_size + n);
        else
            ret = Resize(_size * 2);
    }
    return ret;
}
bool SQBlob::GrowBufOf(SQInteger n)
{
    bool ret = Reserve(n);
    if(ret) _size = _size + n;
    return ret;
}
SQInteger SQBlob::Seek(SQInteger offset, SQInteger origin) {
    switch(origin) {
        case SQ_SEEK_SET:
            if(offset > _size || offset < 0) return -1;
            _ptr = offset;
            break;
        case SQ_SEEK_CUR:
            if(_ptr + offset > _size || _ptr + offset < 0) return -1;
            _ptr += offset;
            break;
        case SQ_SEEK_END:
            if(_size + offset > _size || _size + offset < 0) return -1;
            _ptr = _size + offset;
            break;
        default: return -1;
    }
    return 0;
}

bool SQBlob::SetLen(SQInteger len){
    if(len <= _allocated || Resize(len)){
        _size = len;
        if(_ptr > _size)
            _ptr = _size;
        return true;
    }
    return false;
}

#define SETUP_BLOB(v) \
	SQBlob *self = NULL; \
	{ if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) \
		return sq_throwerror(v,_SC("invalid type tag"));  } \
	if(!self || !self->IsValid())  \
		return sq_throwerror(v,_SC("the blob is invalid"));


static SQRESULT _blob_resize(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger size;
	sq_getinteger(v,2,&size);
	if(!self->Resize(size))
		return sq_throwerror(v,_SC("resize failed"));
	return 0;
}

static SQRESULT _blob_grow(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger size;
	sq_getinteger(v,2,&size);
	if(!self->GrowBufOf(size))
		return sq_throwerror(v,_SC("reserve failed"));
	return 0;
}

static SQRESULT _blob_reserve(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger size;
	sq_getinteger(v,2,&size);
	if(!self->Reserve(size))
		return sq_throwerror(v,_SC("reserve failed"));
	return 0;
}

static void __swap_dword(unsigned int *n)
{
	*n=(unsigned int)(((*n&0xFF000000)>>24)  |
			((*n&0x00FF0000)>>8)  |
			((*n&0x0000FF00)<<8)  |
			((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
	*n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static SQRESULT _blob_swap4(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger num=(self->Len()-(self->Len()%4))>>2;
	unsigned int *t=(unsigned int *)self->GetBuf();
	for(SQInteger i = 0; i < num; i++) {
		__swap_dword(&t[i]);
	}
	return 0;
}

static SQRESULT _blob_swap2(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger num=(self->Len()-(self->Len()%2))>>1;
	unsigned short *t = (unsigned short *)self->GetBuf();
	for(SQInteger i = 0; i < num; i++) {
		__swap_word(&t[i]);
	}
	return 0;
}

static SQRESULT _blob_memset(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
	SETUP_BLOB(v);
	SQ_GET_INTEGER(v, 2, idx);
	SQ_GET_INTEGER(v, 3, val);
	SQ_GET_INTEGER(v, 4, size);
	if(idx < 0 || idx >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));
	if(idx+size < 0 || idx+size >= self->Len())
		return sq_throwerror(v,_SC("index+size out of range"));
    memset(((unsigned char*)self->GetBuf())+idx, val, size);
	return 0;
}

static SQRESULT _blob__set(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger idx,val;
	sq_getinteger(v,2,&idx);
	sq_getinteger(v,3,&val);
	if(idx < 0 || idx >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));
	((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
	sq_push(v,3);
	return 1;
}

static SQRESULT _blob__get(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger idx;

	if ((sq_gettype(v, 2) & SQOBJECT_NUMERIC) == 0)
	{
		sq_pushnull(v);
		return sq_throwobject(v);
	}
	SQObjectType ptype = sq_gettype(v, 2);
	if( !(ptype & SQOBJECT_NUMERIC) ){
	    //allow call metatable methods indexed by strings
	    sq_reseterror(v);
	    return SQ_ERROR;
	}
	sq_getinteger(v,2,&idx);
	if(idx < 0 || idx >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));
	sq_pushinteger(v,((unsigned char *)self->GetBuf())[idx]);
	return 1;
}

static SQRESULT _blob__nexti(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	if(sq_gettype(v,2) == OT_NULL) {
		sq_pushinteger(v, 0);
		return 1;
	}
	SQInteger idx;
	if(SQ_SUCCEEDED(sq_getinteger(v, 2, &idx))) {
		if(idx+1 < self->Len()) {
			sq_pushinteger(v, idx+1);
			return 1;
		}
		sq_pushnull(v);
		return 1;
	}
	return sq_throwerror(v,_SC("internal error (_nexti) wrong argument type"));
}

static SQRESULT _blob__typeof(HSQUIRRELVM v)
{
	sq_pushstring(v,_SC("blob"),-1);
	return 1;
}

static SQRESULT _blob_releasehook(SQUserPointer p, SQInteger /*size*/, void */*ep*/)
{
	SQBlob *self = (SQBlob*)p;
	self->~SQBlob();
	sq_free(self,sizeof(SQBlob));
	return 1;
}

static SQRESULT _blob_constructor(HSQUIRRELVM v)
{
	SQInteger nparam = sq_gettop(v);
	SQInteger size = 0, allocate = 0;
	if(nparam >= 2) {
		sq_getinteger(v, 2, &size);
	}
	if(nparam >= 3) {
		sq_getinteger(v, 2, &allocate);
	}
	if(size < 0) return sq_throwerror(v, _SC("cannot create blob with negative size"));
	if(allocate < 0) return sq_throwerror(v, _SC("cannot create blob with negative allocate"));
	//SQBlob *b = new SQBlob(size);

	SQBlob *b = new (sq_malloc(sizeof(SQBlob)))SQBlob(size, allocate);
	if(SQ_FAILED(sq_setinstanceup(v,1,b))) {
		b->~SQBlob();
		sq_free(b,sizeof(SQBlob));
		return sq_throwerror(v, _SC("cannot create blob"));
	}
	sq_setreleasehook(v,1,_blob_releasehook);
	return 0;
}

static SQRESULT _blob__cloned(HSQUIRRELVM v)
{
	SQBlob *other = NULL;
	{
		if(SQ_FAILED(sq_getinstanceup(v,2,(SQUserPointer*)&other,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
			return SQ_ERROR;
	}
	//SQBlob *thisone = new SQBlob(other->Len());
	SQBlob *thisone = new (sq_malloc(sizeof(SQBlob)))SQBlob(other->Len());
	memcpy(thisone->GetBuf(),other->GetBuf(),thisone->Len());
	if(SQ_FAILED(sq_setinstanceup(v,1,thisone))) {
		thisone->~SQBlob();
		sq_free(thisone,sizeof(SQBlob));
		return sq_throwerror(v, _SC("cannot clone blob"));
	}
	sq_setreleasehook(v,1,_blob_releasehook);
	return 0;
}

static SQRESULT _blob__tostring(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
	sq_pushstring(v, (const SQChar*)self->GetBuf(), self->Len());
	return 1;
}

static SQRESULT _blob_tostring(HSQUIRRELVM v)
{
    return _blob__tostring(v);
}

static SQRESULT _blob_setLen(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SETUP_BLOB(v);
    SQ_GET_INTEGER(v, 2, newLen);
    self->SetLen(newLen);
    return 0;
}

static SQRESULT _blob_clear(HSQUIRRELVM v)
{
    SETUP_BLOB(v);
    self->SetLen(0);
    return 0;
}

#if 0
static SQRESULT _blob_writefile(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SETUP_BLOB(v);
    SQ_GET_STRING(v, fname);
	sq_pushstring(v, (const SQChar*)self->GetBuf(), self->Len());
	return 1;
}

static SQRESULT _blob_readfile(HSQUIRRELVM v)
{
    SQ_FUNC_VARS_NO_TOP(v);
    SETUP_BLOB(v);
    SQ_GET_STRING(v, fname);
	sq_pushstring(v, (const SQChar*)self->GetBuf(), self->Len());
	return 1;
}
#endif

//bit operations
#define BLOB_BIT_GET 1
#define BLOB_BIT_SET 2
#define BLOB_BIT_CLEAR 3
#define BLOB_BIT_TOGLE 4

static SQRESULT _blob_bit_ops(HSQUIRRELVM v, int op)
{
	SETUP_BLOB(v);
	SQInteger bpos;
	sq_getinteger(v,2,&bpos);
	SQInteger cpos = bpos/8;
	SQInteger b8 = 1 << (bpos%8);

	if(cpos < 0 || cpos >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));

	#define BLOB_CH_POS(i) ((unsigned char *)self->GetBuf())[i]

    switch(op)
    {
    case BLOB_BIT_GET:
        sq_pushinteger(v, (BLOB_CH_POS(cpos) & b8));
        return 1;

    case BLOB_BIT_SET:
        BLOB_CH_POS(cpos) |= b8;
        break;

    case BLOB_BIT_CLEAR:
        BLOB_CH_POS(cpos) &= (~b8);
        break;

    case BLOB_BIT_TOGLE:
        BLOB_CH_POS(cpos) ^= b8;
        break;

    }
	return 0;
}

static SQRESULT _blob_bitGet(HSQUIRRELVM v)
{
    return _blob_bit_ops(v, BLOB_BIT_GET);
}

static SQRESULT _blob_bitSet(HSQUIRRELVM v)
{
    return _blob_bit_ops(v, BLOB_BIT_SET);
}

static SQRESULT _blob_bitClear(HSQUIRRELVM v)
{
    return _blob_bit_ops(v, BLOB_BIT_CLEAR);
}

static SQRESULT _blob_bitTogle(HSQUIRRELVM v)
{
    return _blob_bit_ops(v, BLOB_BIT_TOGLE);
}

extern SQRESULT string_gmatch_base(HSQUIRRELVM v, int isGmatch, const SQChar *src, SQInteger src_size);
extern SQRESULT string_find_lua(HSQUIRRELVM v, const SQChar *src, SQInteger src_size);

static SQRESULT _blob_find_lua(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
    return string_find_lua(v, (const SQChar*)self->GetBuf(), self->Len());
}

static SQRESULT _blob_gmatch(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
    return string_gmatch_base(v, 1, (const SQChar*)self->GetBuf(), self->Len());
}

static SQRESULT _blob_match(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
    return string_gmatch_base(v, 0, (const SQChar*)self->GetBuf(), self->Len());
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_SC(#name),_blob_##name,nparams,typecheck,false}
static SQRegFunction _blob_methods[] = {
	_DECL_BLOB_FUNC(constructor,-1,_SC("xnn")),
	_DECL_BLOB_FUNC(resize,2,_SC("xn")),
	_DECL_BLOB_FUNC(grow,2,_SC("xn")),
	_DECL_BLOB_FUNC(reserve,2,_SC("xn")),
	_DECL_BLOB_FUNC(swap2,1,_SC("x")),
	_DECL_BLOB_FUNC(swap4,1,_SC("x")),
	_DECL_BLOB_FUNC(memset,4,_SC("xiii")),
	_DECL_BLOB_FUNC(_set,3,_SC("xnn")),
	_DECL_BLOB_FUNC(_get,2,_SC("x.")), //allow call metatable methods indexed by strings
	_DECL_BLOB_FUNC(_typeof,1,_SC("x")),
	_DECL_BLOB_FUNC(_nexti,2,_SC("x")),
	_DECL_BLOB_FUNC(_cloned,2,_SC("xx")),
	//_DECL_BLOB_FUNC(_tostring,1,_SC("x")),
	_DECL_BLOB_FUNC(tostring,1,_SC("x")),
	_DECL_BLOB_FUNC(setLen,2,_SC("xn")),
	_DECL_BLOB_FUNC(clear,1,_SC("x")),
	_DECL_BLOB_FUNC(bitGet,2,_SC("xn")),
	_DECL_BLOB_FUNC(bitSet,2,_SC("xn")),
	_DECL_BLOB_FUNC(bitClear,2,_SC("xn")),
	_DECL_BLOB_FUNC(bitTogle,2,_SC("xn")),
	_DECL_BLOB_FUNC(find_lua,-2,_SC("xs a|t|c n b n")),
	_DECL_BLOB_FUNC(gmatch,-3,_SC("x s c n n")),
	_DECL_BLOB_FUNC(match,-2,_SC("x s n n")),
	{NULL,(SQFUNCTION)0,0,NULL, false}
};



//GLOBAL FUNCTIONS

static SQRESULT _g_blob_casti2f(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	sq_pushfloat(v,*((SQFloat *)&i));
	return 1;
}

static SQRESULT _g_blob_castf2i(HSQUIRRELVM v)
{
	SQFloat f;
	sq_getfloat(v,2,&f);
	sq_pushinteger(v,*((SQInteger *)&f));
	return 1;
}

static SQRESULT _g_blob_swap2(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	unsigned short s = (unsigned short)i;
	sq_pushinteger(v, ((s << 8) | ((s >> 8) & 0x00FFu)) & 0xFFFFu);
	return 1;
}

static SQRESULT _g_blob_swap4(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	unsigned int t4 = (unsigned int)i;
	__swap_dword(&t4);
	sq_pushinteger(v,(SQInteger)t4);
	return 1;
}

static SQRESULT _g_blob_swapfloat(HSQUIRRELVM v)
{
	SQFloat f;
	sq_getfloat(v,2,&f);
	__swap_dword((unsigned int *)&f);
	sq_pushfloat(v,f);
	return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_SC(#name),_g_blob_##name,nparams,typecheck,false}
static SQRegFunction bloblib_funcs[]={
	_DECL_GLOBALBLOB_FUNC(casti2f,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(castf2i,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap2,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap4,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swapfloat,2,_SC(".n")),
	{NULL,(SQFUNCTION)0,0,NULL,false}
};

SQRESULT sqstd_getblob(HSQUIRRELVM v,SQInteger idx,SQUserPointer *ptr)
{
	SQBlob *blob;
	if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
		return -1;
	*ptr = blob->GetBuf();
	return SQ_OK;
}

SQInteger sqstd_getblobsize(HSQUIRRELVM v,SQInteger idx)
{
	SQBlob *blob;
	if(SQ_FAILED(sq_getinstanceup(v,idx,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG)))
		return -1;
	return blob->Len();
}

//used by sqstdio
SQInteger blob_read(SQUserPointer file,SQUserPointer buf,SQInteger size)
{
	SQInteger ret;
	SQBlob *blob = (SQBlob *)file;
	if( ( ret = blob->Read(buf, size)) !=0 ) return ret;
	return -1;
}

//used by sqstdio
SQInteger blob_write(SQUserPointer file,SQUserPointer p,SQInteger size)
{
    SQBlob *blob = (SQBlob *)file;
	return blob->Write(p,size);
}


SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size)
{
	SQInteger top = sq_gettop(v);
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_blob"),-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) {
		sq_remove(v,-2); //removes the registry
		sq_push(v,1); // push the this
		sq_pushinteger(v,size); //size
		SQBlob *blob = NULL;
		if(SQ_SUCCEEDED(sq_call(v,2,SQTrue,SQFalse))
			&& SQ_SUCCEEDED(sq_getinstanceup(v,-1,(SQUserPointer *)&blob,(SQUserPointer)SQSTD_BLOB_TYPE_TAG))) {
			sq_remove(v,-2);
			return blob->GetBuf();
		}
	}
	sq_settop(v,top);
	return NULL;
}

SQRESULT sqstd_register_bloblib(HSQUIRRELVM v)
{
	return declare_stream(v,_SC("blob"),(SQUserPointer)SQSTD_BLOB_TYPE_TAG,_SC("std_blob"),_blob_methods,bloblib_funcs);
}

