#include <algorithm>
#include <node.h>
#include "hash.h"
#include "json.h"
#include "semaphore.h"
#include "mempool.h"
#include "crc32.h"
#include "shm.h"
#include "error.h"

using namespace v8;

/* */

void* memalloc (int cnt, size_t size, void *mp)
{
	int allocsize = cnt*size;
	void *p;

	p = mempool_alloc((mp_t*)mp, allocsize);
	if (p) memset(p, 0, allocsize);

	return p;
}

void memfree(void *p, void *mp)
{
	if (p) {
		mempool_free((mp_t*)mp, p);
	}
}

char* strndup(char *src, void *mp)
{
	char *s = NULL;
	int len;

	len = strlen(src) + 1;
	s = (char*) memalloc(1, len, mp);
	if (s) {
		strncpy(s, src, len);
	}
	return s;
}

char* itoindex(unsigned int i, void *mp)
{
	const char *format = "IDX_%d";
	int len = 5;
	int l = i;
	char *s = NULL;

	for (; l; len++) {
		l /= 10;
	}
	
	s = (char*) memalloc(1, len, mp);
	if (s) {
		sprintf(s, format, i);
	}

	return s;
}

/* */
typedef struct jss_header_t {
	unsigned int magic;

	unsigned int name;

	unsigned int lastParsed;
	unsigned int start;

	unsigned char data[];
} jss_header_t;

typedef struct jss_data_t {
	json_type type;
	union {
		json_int_t integer;
		double dbl;
		int boolean;

		struct {
			int length;
			int offset;
		} string;

		int objectoffset;
	} u;
} jss_data_t;

/* */
class Jss : public node::ObjectWrap {
public:
	int AllocStorage(unsigned int key, unsigned int size);
	void FreeStorage();
	int EnterLock();
	int LeaveLock();
	jss_data_t* Parse(json_value *jval);
	void* OffsetToPtr(unsigned long offset);
	unsigned long PtrToOffset(void *ptr);
	void SetData(jss_data_t *jdata);
	void SetLastParsed(unsigned int crc);
	unsigned int GetLastParsed();
	Handle<Value> ShallowClone(jss_data_t *jdata);
	

	static void Init(Handle<Object> target);
	static Handle<Value> NewInstance(int argc, Handle<Value> argv[]);

private:
	 Jss();
	~Jss();
	
	static Handle<Value> New(const Arguments& args);
	static Handle<Value> GetNamedProperty(Local<String> name, const AccessorInfo &info);
	static Handle<Array> EnumerateNamedProperty(const AccessorInfo &info);
	static Handle<Value> GetIndexedProperty(uint32_t index, const AccessorInfo &info);
	static Handle<Array> EnumerateIndexedProperty(const AccessorInfo& info);
	static Persistent<Function> constructor_template_;

	jss_header_t *header_;
	sema_t sema_;
	shm_t *shm_;
	mp_t *mp_;
	jss_data_t *data_;
	int isCloned_;
	hash_userset_t userset_;

	unsigned int size_;
	unsigned int key_;
};

Persistent<Function> Jss::constructor_template_;

Jss::Jss() 
{
	header_ = NULL;
	sema_ = NULL;
	shm_ = NULL;
	mp_ = NULL;
	data_ = NULL;
	isCloned_ = false;

	size_ = 0;
	key_ = 0;

	userset_.memalloc = memalloc;
	userset_.memfree = memfree;
}

Jss::~Jss() 
{
	if (!isCloned_)
		FreeStorage();
}

void Jss::FreeStorage()
{
	if (sema_) sema_del(sema_);
	if (shm_) shm_del(shm_);
	if (mp_) mempool_del(mp_);
}

int Jss::AllocStorage(unsigned int key, unsigned int size)
{
	for (;;) {
		unsigned int realsize = sizeof(jss_header_t) + size;
		jss_header_t *header;
		int chunksize = 32;
		int poolsize = size / (chunksize+sizeof(mp_hdr_t));

		errprint("chunksize(%d), size(%d), realsize(%d), poolsize(%d)\n", chunksize, size, realsize, poolsize);
				
		shm_ = shm_create(key, realsize, SHM_READWRITE);
		if (!shm_) 
			break;
		
		header_ = header = (jss_header_t*) shm_get(shm_);
		errprint("header(0x%x), data(0x%x), size(%d), key(0x%x)\n", header, header->data, size, key);

		if (header->magic =='_JSS') {
			data_ = (jss_data_t *) OffsetToPtr(header->start);
			errprint("AllocStorage header->start=%d", header->start);
			
		} else {
			memset(header, 0, sizeof(jss_header_t));
			header->magic = '_JSS';

			mp_ = mempool_create(header->data, chunksize, poolsize);
			if (!mp_)
				break;
			errprint("mempool_create() ok.\n");

			userset_.userdata = mp_;
		}

		size_ = size;
		key_ = key;
		
		return 1;
	}
	
	FreeStorage();
	return 0;
}

int Jss::EnterLock()
{
	return sema_enter(sema_);
}

int Jss::LeaveLock()
{
	return sema_leave(sema_);
}

void* Jss::OffsetToPtr(unsigned long offset)
{
	return (void *) ((unsigned char*) header_ - offset);
}

unsigned long Jss::PtrToOffset(void *ptr)
{
	return (unsigned char*) header_ - (unsigned char *) ptr;
}

void Jss::SetData(jss_data_t *jdata)
{
	data_ = jdata;
	header_->start = PtrToOffset(jdata);
	errprint("SetData header_->start=%d\n", header_->start);
}

void Jss::SetLastParsed(unsigned int crc)
{
	header_->lastParsed = crc;
}

unsigned int Jss::GetLastParsed()
{
	return header_->lastParsed;
}

Handle<Value> Jss::ShallowClone(jss_data_t *jdata)
{
	HandleScope scope;

	Local<Object> instance = constructor_template_->NewInstance(0, NULL);
	Jss *cloned = Unwrap<Jss>(instance);
	if (!cloned) {
		scope.Close(Undefined());
	}

	cloned->header_ = header_;
	cloned->sema_ = sema_;
	cloned->shm_ = shm_;
	cloned->mp_ = mp_;

	cloned->size_ = size_;
	cloned->key_ = key_;

	cloned->userset_.memalloc = memalloc;
	cloned->userset_.memfree = memfree;

	cloned->data_ = jdata;
	cloned->isCloned_ = true;

	return scope.Close(instance);
}

jss_data_t* Jss::Parse(json_value *jval)
{
	jss_data_t *jdata = NULL, *value = NULL;
	char *str = NULL;
	hash_t *map = NULL;
	int len;

	jdata = (jss_data_t *) memalloc(1, sizeof(jss_data_t), mp_);
	if (!jdata) {
		return NULL;
	}

	jdata->type = jval->type;
	switch(jdata->type) {
	case json_integer:
		jdata->u.integer = jval->u.integer;
		//printf("value %d\n", jval->u.integer);
		break;
	case json_double:
		jdata->u.dbl = jval->u.dbl;
		break;
	case json_boolean:
		jdata->u.boolean = jval->u.boolean;
		break;
	case json_null:
		break;
	case json_none:
		break;
	case json_string:
		str = strndup(jval->u.string.ptr, mp_);
		if (!str) goto error;
		jdata->u.string.length = jval->u.string.length;
		jdata->u.string.offset = PtrToOffset(str);
		//printf("value %s\n", str);
		break;
	case json_object:
		len = jval->u.object.length;
		map = hash_create(len, &userset_);
		if (!map) goto error;
		for (int i=0; i<len; i++) {
			str = strndup(jval->u.object.values[i].name, mp_);
			if (!str) {
				goto error;
			}

	        //printf("key name = %s\n", str);
            //printf("child\n");
			value = Parse(jval->u.object.values[i].value);
			if (!value) {
				memfree(str, mp_);
				goto error;
			}

			if (hash_insert(map, str, value) == HASH_FAIL) {
				//DeleteJdata(jdata);
				goto error;
			}
		}
		jdata->u.objectoffset = PtrToOffset(map);
		break;
	case json_array:
		len = jval->u.array.length;
		map = hash_create(len, &userset_);
		if (!map) goto error;

		for (int i=0; i<len; i++) {
			str = itoindex(i, mp_);
			if (!str) {
				goto error;
			}

			//printf("key name = %s\n", str);

			value = Parse(jval->u.array.values[i]);
			if (!value) {
				memfree(str, mp_);
				goto error;
			}

			if (hash_insert(map, str, value) == HASH_FAIL) {
				//DeleteJdata(jdata);
				goto error;
			}
		}
		jdata->u.objectoffset = PtrToOffset(map);
		break;
	}

	return jdata;

error:
	printf("alloc fail\n");
	/*
	if (jdata)
		DeleteJdata(jdata);
	*/
	return NULL;
}

void Jss::Init(Handle<Object> target)
{
	HandleScope scope;

	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("Jss"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	// Prototype
	tpl->InstanceTemplate()->SetNamedPropertyHandler(GetNamedProperty, 0, 0, 0,EnumerateNamedProperty);
	tpl->InstanceTemplate()->SetIndexedPropertyHandler(GetIndexedProperty, 0, 0, 0, EnumerateIndexedProperty);
	constructor_template_ = Persistent<Function>::New(tpl->GetFunction());
}

Handle<Value> Jss::New(const Arguments& args) 
{
	HandleScope scope;

	Jss* jss = new Jss();
	jss->Wrap(args.This());
	return args.This();
}

Handle<Value> Jss::NewInstance(int argc, Handle<Value> argv[])
{
	HandleScope scope;

	Local<Object> instance = constructor_template_->NewInstance(argc, argv);
	return scope.Close(instance);
}

void hash_enum_callback(const hash_t *tptr, int idx, char *key, void *data, void *userdata) {
	Local<Array> *result = static_cast<Local<Array>*>(userdata);
	if (!strncmp(key, "IDX_", 4)) {
		key += 4;
	}
	(*result)->Set(Integer::New(idx), String::New(key));
}

Handle<Array> Jss::EnumerateNamedProperty(const AccessorInfo& info) 
{
	HandleScope scope;
	Local<Array> result;
	hash_t *object;
	Jss *jss;

	jss = Unwrap<Jss>(info.Holder().As<Object>());
	if (!jss) {
		return scope.Close(Array::New(0));
	}

	object = (hash_t *) jss->OffsetToPtr(jss->data_->u.objectoffset);
	result = Array::New(hash_entries(object));
	hash_enumerator(object, hash_enum_callback, &result);

	return scope.Close(result);
}

 Handle<Array> Jss::EnumerateIndexedProperty(const AccessorInfo &info) 
 {
	 HandleScope scope;
	 return scope.Close(Array::New(0));
}

Handle<Value> Jss::GetNamedProperty(Local<String> name, const AccessorInfo &info)
{
	HandleScope scope;
	jss_data_t *jdata;
	hash_t *object;
	char *str;

	Local<Value> property =  info.This()->GetRealNamedProperty(name);
	if (!property.IsEmpty()) {
		return scope.Close(property);
	}

	String::Utf8Value key(name);
	if (strcmp(*key, "inspect") == 0) {
		return scope.Close(Undefined());
	}

	Jss *jss = Unwrap<Jss>(info.This());
	if (!jss) {
		return scope.Close(Undefined());
	}

	object = (hash_t *) jss->OffsetToPtr(jss->data_->u.objectoffset);
	jdata = (jss_data_t *) hash_lookup(object, *key);
	if (jdata == HASH_FAIL)
		return scope.Close(Undefined());

	//printf("key=%s\n", *key);

	Handle<Value> subobj;

	switch(jdata->type) {
	case json_object:
	case json_array:
		subobj = jss->ShallowClone(jdata);
		return scope.Close(subobj);
	case json_integer:
		return scope.Close(Number::New(jdata->u.integer));
	case json_double:
		return scope.Close(Number::New(jdata->u.dbl));
	case json_string:
		str = (char *) jss->OffsetToPtr(jdata->u.string.offset);
		return scope.Close(String::New(str));
	case json_boolean:
		return scope.Close(Boolean::New(jdata->u.boolean));
	case json_null:
		return scope.Close(Null());
	case json_none:
		break;
	};

	return scope.Close(Undefined());
}

Handle<Value> Jss::GetIndexedProperty(uint32_t index, const AccessorInfo &info)
{
	HandleScope scope;
	const char *format = "IDX_%d";
	char key[128];

	Jss *jss = Unwrap<Jss>(info.This());
	if (!jss) {
		return scope.Close(Undefined());
	}
	
	sprintf(key, "%d", index);

	Handle<Value> instance = GetNamedProperty(String::New(key), info);
	if (instance->IsUndefined() || instance->IsNull()) {
		sprintf(key, format, index);
		instance = GetNamedProperty(String::New(key), info);
	}

	return scope.Close(instance); 
}


/* */

#define REQUIRE_ARGUMENT_STRING(i, var)											\
	if (args.Length() <= (i) || !args[i]->IsString()) {							\
		return ThrowException(Exception::TypeError(								\
			String::New("Argument " #i " must be a string"))					\
		);																		\
	}																			\
	String::Utf8Value var(args[i]->ToString());

Handle<Value> CreateJssObject(const Arguments& args)
{
	HandleScope scope;
	REQUIRE_ARGUMENT_STRING(0, jstr);
	Handle<Value> instance;
	Jss *jss;
	jss_data_t *root;
	json_value *jval;
	unsigned int crc;
	int len, allocsize;
	sema_t sema;

	for (;;) {
		len = strlen(*jstr);
		crc = crc32(0, *jstr, len);
		allocsize = std::max(len*20, 1*1024*1024);
		errprint("jstrlen(%d), crc32(0x%x), allocsize(%d)\n", len, crc, allocsize);

		instance = Jss::NewInstance(0, NULL);
		jss = node::ObjectWrap::Unwrap<Jss>(instance->ToObject());
		errprint("jss(0x%x)\n", jss);

		sema = sema_create(crc);

		try {
			sema_enter(sema);
			if (!jss->AllocStorage(crc, allocsize))
				break;

			if (jss->GetLastParsed() != crc) {
				jval = json_parse(*jstr, len);
				if (!jval)
					break;
				root = jss->Parse(jval);
				json_value_free(jval);
				if (!root) break;
				jss->SetData(root);
				jss->SetLastParsed(crc);
			} else {
				printf("[jss] crc32(0x%x) is already loaded.\n", crc);
			}
			sema_leave(sema);
			sema_del(sema);
			return scope.Close(instance);		
		} catch(...) {
			break;
		}
	}

	sema_leave(sema);
	sema_del(sema);
	jss->FreeStorage();
	ThrowException(Exception::Error(String::New("")));
	return scope.Close(Undefined());
}

void InitAll(Handle<Object> exports)
{
	NODE_SET_METHOD(exports, "createJssObject", CreateJssObject);
	Jss::Init(exports);
}

NODE_MODULE(jss, InitAll)