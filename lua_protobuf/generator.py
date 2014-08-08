#  Copyright 2011 Gregory Szorc
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

# Modified by James Moran 2014
# Updated to work with Lua 5.2. Fixing windows platform issues.
# Added wrapper classes for CodedInput/OutputStream(s)

from google.protobuf.descriptor import FieldDescriptor
import re

RE_BARE_BEGIN_BRACKET = re.compile(r'^\s*{\s*$')
RE_BEGIN_BRACKET = re.compile(r'{\s*$')
RE_END_BRACKET = re.compile(r'^\s*};?\s*$')

FIELD_LABEL_MAP = {
    FieldDescriptor.LABEL_OPTIONAL: 'optional',
    FieldDescriptor.LABEL_REQUIRED: 'required',
    FieldDescriptor.LABEL_REPEATED: 'repeated'
}

FIELD_TYPE_MAP = {
    FieldDescriptor.TYPE_DOUBLE: 'double',
    FieldDescriptor.TYPE_FLOAT: 'float',
    FieldDescriptor.TYPE_INT64: 'int64',
    FieldDescriptor.TYPE_UINT64: 'uint64',
    FieldDescriptor.TYPE_INT32: 'int32',
    FieldDescriptor.TYPE_FIXED64: 'fixed64',
    FieldDescriptor.TYPE_FIXED32: 'fixed32',
    FieldDescriptor.TYPE_BOOL: 'bool',
    FieldDescriptor.TYPE_STRING: 'string',
    FieldDescriptor.TYPE_GROUP: 'group',
    FieldDescriptor.TYPE_MESSAGE: 'message',
    FieldDescriptor.TYPE_BYTES: 'bytes',
    FieldDescriptor.TYPE_UINT32: 'uint32',
    FieldDescriptor.TYPE_ENUM: 'enum',
    FieldDescriptor.TYPE_SFIXED32: 'sfixed32',
    FieldDescriptor.TYPE_SFIXED64: 'sfixed64',
    FieldDescriptor.TYPE_SINT32: 'sint32',
    FieldDescriptor.TYPE_SINT64: 'sint64',
}

def lua_protobuf_header():
    '''Returns common header included by all produced files'''
    return '''
#ifndef LUA_PROTOBUF_H
#define LUA_PROTOBUF_H

#include <google/protobuf/message.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>

#ifdef WINDOWS
#define LUA_PROTOBUF_EXPORT __declspec(dllexport)
#else
#define LUA_PROTOBUF_EXPORT
#endif

// type for callback function that is executed before Lua performs garbage
// collection on a message instance.
// if called function returns 1, Lua will free the memory backing the object
// if returns 0, Lua will not free the memory
typedef int (*lua_protobuf_gc_callback)(::google::protobuf::MessageLite *msg, void *userdata);

// __index and __newindex functions for enum tables
LUA_PROTOBUF_EXPORT int lua_protobuf_enum_index(lua_State *L);
LUA_PROTOBUF_EXPORT int lua_protobuf_enum_newindex(lua_State *L);

// GC callback function that always returns true
LUA_PROTOBUF_EXPORT int lua_protobuf_gc_always_free(::google::protobuf::MessageLite *msg, void *userdata);

// A minimal Lua interface for coded input/output protobuf streams
int lua_protobuf_coded_streams_open(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif

'''

def lua_protobuf_source():
    '''Returns source for common code'''
    return '''

#include "lua-protobuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <lauxlib.h>

#ifdef __cplusplus
}
#endif

int lua_protobuf_enum_index(lua_State *L)
{
    return luaL_error(L, "attempting to access undefined enumeration value: %s", lua_tostring(L, 2));
}

int lua_protobuf_enum_newindex(lua_State *L)
{
    return luaL_error(L, "cannot modify enumeration tables");
}

int lua_protobuf_gc_always_free(::google::protobuf::MessageLite *msg, void *ud)
{
    return 1;
}

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include <fcntl.h>
#include <sys/stat.h>

#if defined (_MSC_VER)
#   include <io.h> // for open
#else
#   include <sys/types.h>
#   define O_BINARY (0)
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int lua_protobuf_coded_input_stream_new(lua_State* L) {
    const char* filepath = luaL_checkstring(L, 1);
    int fd = open(filepath, O_RDONLY | O_BINARY, S_IREAD);
    if (fd == -1) {
        return luaL_error(L, "Failed to open file %s", filepath);
    }
    char* udataptr = (char*)lua_newuserdata(L, sizeof(::google::protobuf::io::CodedInputStream)+sizeof(::google::protobuf::io::FileInputStream));
    auto instream = new (udataptr+sizeof(::google::protobuf::io::FileInputStream)) ::google::protobuf::io::FileInputStream(fd);
    instream->SetCloseOnDelete(true);
    auto codestream = new (udataptr) ::google::protobuf::io::CodedInputStream(instream);
    luaL_setmetatable(L, "protobuf_.CodedInputStream");
    return 1;
}

int lua_protobuf_coded_input_stream_gc(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    ::google::protobuf::io::FileInputStream* filestream = (::google::protobuf::io::FileInputStream*)(codestream+1);
    codestream->~CodedInputStream();
    filestream->~FileInputStream();
    return 0;
}

int lua_protobuf_coded_input_stream_skip(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    int count = luaL_checkint(L, 2);
    codestream->Skip(count);
    return 0;
}

int lua_protobuf_coded_input_stream_push_limit(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    int limit = luaL_checkint(L, 2);
    limit = codestream->PushLimit(limit);
    lua_pushinteger(L, limit);
    return 1;
}

int lua_protobuf_coded_input_stream_pop_limit(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    int limit = luaL_checkint(L, 2);
    codestream->PopLimit(limit);
    return 0;
}

int lua_protobuf_coded_input_stream_current_position(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    lua_pushinteger(L, codestream->CurrentPosition());
    return 1;
}

int lua_protobuf_coded_input_stream_read_raw(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    int count = luaL_checkint(L, 2);
    char* buf = new char[count];
    bool success = codestream->ReadRaw(buf, count);
    if (success) {
        lua_pushlstring(L, buf, count);
    } else {
        lua_pushnil(L);
    }
    delete buf;
    return 1;
}

int lua_protobuf_coded_input_stream_read_varint_32(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    ::google::protobuf::uint32 val;
    bool success = codestream->ReadVarint32(&val);
    lua_pushboolean(L, success);
    if (success) {
        lua_pushinteger(L, val);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int lua_protobuf_coded_input_stream_read_varint_64(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    ::google::protobuf::uint64 val;
    bool success = codestream->ReadVarint64(&val);
    lua_pushboolean(L, success);
    if (success) {
        lua_pushinteger(L, val);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int lua_protobuf_coded_input_stream_read_little_endian_32(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    ::google::protobuf::uint32 val;
    bool success = codestream->ReadLittleEndian32(&val);
    lua_pushboolean(L, success);
    if (success) {
        lua_pushinteger(L, val);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int lua_protobuf_coded_input_stream_read_little_endian_64(lua_State* L) {
    ::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, "protobuf_.CodedInputStream");
    ::google::protobuf::uint64 val;
    bool success = codestream->ReadLittleEndian64(&val);
    lua_pushboolean(L, success);
    if (success) {
        lua_pushinteger(L, val);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static const struct luaL_Reg CodedInputStream_functions [] = {
    {"new", lua_protobuf_coded_input_stream_new},
    {NULL, NULL}
};

static const struct luaL_Reg CodedInputStream_methods [] = {
    {"__gc", lua_protobuf_coded_input_stream_gc},
    {"Skip", lua_protobuf_coded_input_stream_skip},
    {"PushLimit", lua_protobuf_coded_input_stream_push_limit},
    {"PopLimit", lua_protobuf_coded_input_stream_pop_limit},
    {"CurrentPosition", lua_protobuf_coded_input_stream_current_position},
    {"ReadRaw", lua_protobuf_coded_input_stream_read_raw},
    {"ReadVarint32", lua_protobuf_coded_input_stream_read_varint_32},
    {"ReadVarint64", lua_protobuf_coded_input_stream_read_varint_64},
    {"ReadLittleEndian32", lua_protobuf_coded_input_stream_read_little_endian_32},
    {"ReadLittleEndian64", lua_protobuf_coded_input_stream_read_little_endian_64},
    {NULL, NULL},
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int lua_protobuf_coded_output_stream_new(lua_State* L) {
    const char* filepath = luaL_checkstring(L, 1);
    int fd = open(filepath, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);
    if (fd == -1) {
        return luaL_error(L, "Failed to open file %s", filepath);
    }
    char* udataptr = (char*)lua_newuserdata(L, sizeof(::google::protobuf::io::CodedOutputStream)+sizeof(::google::protobuf::io::FileOutputStream));
    auto outstream = new(udataptr+sizeof(::google::protobuf::io::CodedOutputStream)) ::google::protobuf::io::FileOutputStream(fd);
    outstream->SetCloseOnDelete(true);
    auto codestream = new (udataptr) ::google::protobuf::io::CodedOutputStream(outstream);
    luaL_setmetatable(L, "protobuf_.CodedOutputStream");
    return 1;
}

int lua_protobuf_coded_output_stream_gc(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    ::google::protobuf::io::FileOutputStream* filestream = (::google::protobuf::io::FileOutputStream*)(codestream+1);
    codestream->~CodedOutputStream();
    filestream->~FileOutputStream();
    return 0;
}

int lua_protobuf_coded_output_stream_skip(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    int count = luaL_checkint(L, 2);
    codestream->Skip(count);
    return 0;
}

int lua_protobuf_coded_output_stream_byte_count(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    lua_pushinteger(L, codestream->ByteCount());
    return 1;
}

int lua_protobuf_coded_output_stream_write_raw(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    size_t count;
    const char* buf = luaL_checklstring(L, 2, &count);
    codestream->WriteRaw(buf, (int)count);
    return 0;
}

int lua_protobuf_coded_output_stream_write_varint_32(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    ::google::protobuf::uint32 val = luaL_checkunsigned(L, 2);
    codestream->WriteVarint32(val);
    return 0;
}

int lua_protobuf_coded_output_stream_write_varint_64(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    ::google::protobuf::uint64 val = luaL_checkunsigned(L, 2);
    codestream->WriteVarint64(val);
    return 0;
}

int lua_protobuf_coded_output_stream_write_little_endian_32(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    ::google::protobuf::uint32 val = luaL_checkunsigned(L, 2);
    codestream->WriteLittleEndian32(val);
    return 0;
}

int lua_protobuf_coded_output_stream_write_little_endian_64(lua_State* L) {
    ::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, "protobuf_.CodedOutputStream");
    ::google::protobuf::uint64 val = luaL_checkunsigned(L, 2);
    codestream->WriteLittleEndian64(val);
    return 0;
}

static const struct luaL_Reg CodedOutputStream_functions [] = {
    {"new", lua_protobuf_coded_output_stream_new},
    {NULL, NULL}
};

static const struct luaL_Reg CodedOutputStream_methods [] = {
    {"__gc", lua_protobuf_coded_output_stream_gc},
    {"Skip", lua_protobuf_coded_output_stream_skip},
    {"ByteCount", lua_protobuf_coded_output_stream_byte_count},
    {"WriteRaw", lua_protobuf_coded_output_stream_write_raw},
    {"WriteVarint32", lua_protobuf_coded_output_stream_write_varint_32},
    {"WriteVarint64", lua_protobuf_coded_output_stream_write_varint_64},
    {"WriteLittleEndian32", lua_protobuf_coded_output_stream_write_little_endian_32},
    {"WriteLittleEndian64", lua_protobuf_coded_output_stream_write_little_endian_64},
    {NULL, NULL},
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const struct luaL_Reg CodedInputStream_lib_functions [] = {
    {NULL, NULL}
};

int lua_protobuf_coded_streams_open(lua_State* L) {
    luaL_checktype(L, -1, LUA_TTABLE);

    luaL_newmetatable(L, "protobuf_.CodedInputStream");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, CodedInputStream_methods, 0);
    lua_pop(L, 1);//pop the metatable

    luaL_newmetatable(L, "protobuf_.CodedOutputStream");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, CodedOutputStream_methods, 0);
    lua_pop(L, 1);//pop the metatable

    // add create funcs and tables
    luaL_newlib(L, CodedInputStream_functions);
    lua_setfield(L, -2, "CodedInputStream");
    luaL_newlib(L, CodedOutputStream_functions);
    lua_setfield(L, -2, "CodedOutputStream");
    return 0;
}
#ifdef __cplusplus
extern "C" {
#endif

const char *luaEXT_findtable (lua_State *L, const char *fname, int idx, int szhint) {
  const char *e;
  if (idx) lua_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    lua_pushlstring(L, fname, e - fname);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {  /* no such field? */
      lua_pop(L, 1);  /* remove this nil */
      lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      lua_pushlstring(L, fname, e - fname);
      lua_pushvalue(L, -2);
      lua_settable(L, -4);  /* set new table into field */
    }
    else if (!lua_istable(L, -1)) {  /* field has a non-table value? */
      lua_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    lua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}

#ifdef __cplusplus
}
#endif

'''

def c_header_header(filename, package):
    return [
        '// Generated by the lua-protobuf compiler.',
        '// You shouldn\'t be editing this file manually',
        '//',
        '// source proto file: %s' % filename,
        '',
        '#ifndef LUA_PROTOBUF_%s_%s_H' % (package.replace('.', '_'), filename.replace('.proto', '')),
        '#define LUA_PROTOBUF_%s_%s_H' % (package.replace('.', '_'), filename.replace('.proto', '')),
        '',
        '#include "lua-protobuf.h"',
        '#include <%s.pb.h>' % filename.replace('.proto', ''),#package.replace('.', '_'),
        '',
        '#ifdef __cplusplus',
        'extern "C" {',
        '#endif',
        '',
        '#include <lua.h>',
        '',
        'const char* luaEXT_findtable (lua_State*, const char*, int, int);',
        '',
        ## We do this function based on file name to avoid name collisions
        '// register all messages in this package to a Lua state',
        'LUA_PROTOBUF_EXPORT int %sopen(lua_State *L);' % proto_function_open_name(filename),
        '',
    ]

def source_header(filename, package, file_descriptor):
    '''Returns lines that begin a source file'''
    lines = []

    lines.extend( [
        '// Generated by the lua-protobuf compiler',
        '// You shouldn\'t edit this file manually',
        '//',
        '// source proto file: %s' % filename,
        '',
        ])
    lines.append('#include "%s.pb.lua.h"' % filename.replace('.proto', ''))
    for type in file_descriptor.dependency:
        lines.append('#include "%s.pb.lua.h"' % type.replace('.proto', ''))
    lines.extend( ['',
        '#ifdef __cplusplus',
        'extern "C" { // make sure functions treated with C naming',
        '#endif',
        '',
        '#include <lauxlib.h>',
        '',
        '#ifdef __cplusplus',
        '}',
        '#endif',
        '',
        '#include <string>',
        '',
        '// this represents Lua udata for a protocol buffer message',
        '// we record where a message came from so we can GC it properly',
        'typedef struct msg_udata { // confuse over-simplified pretty-printer',
        '    ::google::protobuf::MessageLite * msg;',
        '    bool lua_owns;',
        '    lua_protobuf_gc_callback gc_callback;',
        '    void * callback_data;',
        '} msg_udata;',
        '',])
    return lines

def proto_function_open_name(filename):
    return 'lua_protobuf_%s_' % filename.replace('.proto', '')
    
def package_function_prefix(package):
    return 'lua_protobuf_%s_' % package.replace('.', '_')

def message_function_prefix(package, message):
    return '%s%s_' % (package_function_prefix(package), message)

def message_open_function_name(package, message):
    '''Returns function name that registers the Lua library for a message type'''

    return '%sopen' % message_function_prefix(package, message)

def cpp_class(package, message = None):
    '''Returns the fully qualified class name for a message type'''

    if not message:
        return package.replace('.', '::')

    return '::%s::%s' % ( package.replace('.', '::'), message )

def field_function_name(package, message, prefix, field):
    '''Obtain the function name of a field accessor/mutator function'''

    return '%s%s_%s' % ( message_function_prefix(package, message), prefix, field )

def field_function_start(package, message, prefix, field):
    '''Obtain the start of function for a field accessor function'''
    return [
        'int %s(lua_State *L)' % field_function_name(package, message, prefix, field.lower()),
        '{',
    ]

def lua_libname(package, message):
    '''Returns the Lua library name for a specific message'''

    return 'protobuf.%s.%s' % (package, message)

def metatable(package, message):
    '''Returns Lua metatable for protocol buffer message type'''
    return 'protobuf_.%s.%s' % (package, message)

def obtain_message_from_udata(package, message=None, index=1, varname='m'):
    '''Statement that obtains a message from userdata'''

    c = cpp_class(package, message)
    return [
        'msg_udata * %sud = (msg_udata *)%s;' % ( varname, check_udata(package, message, index) ),
        '%s *%s = (%s *)%sud->msg;' % ( c, varname, c, varname ),
    ]

def check_udata(package, message, index=1):
    '''Validates a udata is instance of protocol buffer message

    By default, it validates udata at top of the stack
    '''

    return 'luaL_checkudata(L, %d, "%s")' % ( index, metatable(package, message) )

def has_body(package, message, field):
    '''Returns the function body for a has_<field> function'''

    lines = []
    lines.extend(obtain_message_from_udata(package, message))
    lines.append('lua_pushboolean(L, m->has_%s());' % field.lower())
    lines.append('return 1;')

    return lines

def clear_body(package, message, field):
    '''Returns the function body for a clear_<field> function'''
    lines = []
    lines.extend(obtain_message_from_udata(package, message))
    lines.append('m->clear_%s();' % field.lower())
    lines.append('return 0;')

    return lines

def size_body(package, message, field):
    '''Returns the function body for a size_<field> function'''
    lines = []
    lines.extend(obtain_message_from_udata(package, message))
    lines.append('int size = m->%s_size();' % field.lower())
    lines.append('lua_pushinteger(L, size);')
    lines.append('return 1;')

    return lines

def add_body(package, message, field, type_name):
    '''Returns the function body for the add_<field> function for repeated embedded messages'''
    lines = []
    lines.extend(obtain_message_from_udata(package, message))
    lines.extend([
        '%s *msg_new = m->add_%s();' % ( cpp_class(type_name), field.lower() ),

        # since the message is allocated out of the containing message, Lua
        # does not need to do GC
        'lua_protobuf%s_pushreference(L, msg_new, NULL, NULL);' % type_name.replace('.', '_'),
        'return 1;',
    ])

    return lines

def field_get(package, message, field_descriptor):
    '''Returns function definition for a get_<field> function'''

    name = field_descriptor.name
    type = field_descriptor.type
    type_name = field_descriptor.type_name
    label = field_descriptor.label
    repeated = label == FieldDescriptor.LABEL_REPEATED

    lines = []
    lines.extend(field_function_start(package, message, 'get', name))
    lines.extend(obtain_message_from_udata(package, message))

    # the logic is significantly different depending on if the field is
    # singular or repeated.
    # for repeated, we have an argument which points to the numeric index to
    # retrieve. in true Lua convention, we index starting from 1, which is
    # different from protocol buffers, which indexes from 0

    if repeated:
        lines.extend([
            'if (lua_gettop(L) != 2) {',
                'return luaL_error(L, "missing required numeric argument");',
            '}',
            'lua_Integer index = luaL_checkinteger(L, 2);',
            'if (index < 1 || index > m->%s_size()) {' % name.lower(),
                # TODO is returning nil the more Lua way?
                'return luaL_error(L, "index must be between 1 and current size: %%d", m->%s_size());' % name.lower(),
            '}',
        ])

    # TODO float and double types are not equivalent. don't treat them as such
    # TODO figure out how to support 64 bit integers properly

    if repeated:
        if type in [ FieldDescriptor.TYPE_STRING, FieldDescriptor.TYPE_BYTES ]:
            lines.extend([
                'string s = m->%s(index - 1);' % name.lower(),
                'lua_pushlstring(L, s.c_str(), s.size());',
            ])
        elif type == FieldDescriptor.TYPE_BOOL:
            lines.append('lua_pushboolean(L, m->%s(index-1));' % name.lower())

        elif type in [FieldDescriptor.TYPE_INT32, FieldDescriptor.TYPE_UINT32,
            FieldDescriptor.TYPE_FIXED32, FieldDescriptor.TYPE_SFIXED32, FieldDescriptor.TYPE_SINT32]:

            lines.append('lua_pushinteger(L, m->%s(index-1));' % name.lower())

        elif type in [ FieldDescriptor.TYPE_INT64, FieldDescriptor.TYPE_UINT64,
            FieldDescriptor.TYPE_FIXED64, FieldDescriptor.TYPE_SFIXED64, FieldDescriptor.TYPE_SINT64]:
            lines.append('lua_pushinteger(L, m->%s(index-1));' % name.lower())

        elif type == FieldDescriptor.TYPE_FLOAT or type == FieldDescriptor.TYPE_DOUBLE:
            lines.append('lua_pushnumber(L, m->%s(index-1));' % name.lower())

        elif type == FieldDescriptor.TYPE_ENUM:
            lines.append('lua_pushnumber(L, m->%s(index-1));' % name.lower())

        elif type == FieldDescriptor.TYPE_MESSAGE:
            lines.extend([
                '%s * got_msg = m->mutable_%s(index-1);' % ( type_name.replace('.', '::'), name.lower() ),
                'lua_protobuf%s_pushreference(L, got_msg, NULL, NULL);' % type_name.replace('.', '_'),
            ])

        else:
            lines.append('return luaL_error(L, "lua-protobuf does not support this field type");')
    else:
        # for scalar fields, we push nil if the value is not defined
        # this is the Lua way
        if type == FieldDescriptor.TYPE_STRING or type == FieldDescriptor.TYPE_BYTES:
            lines.append('string s = m->%s();' % name.lower())
            lines.append('if (m->has_%s()) lua_pushlstring(L, s.c_str(), s.size()); else lua_pushnil(L);' % name.lower())

        elif type == FieldDescriptor.TYPE_BOOL:
            lines.append('if (m->has_%s()) lua_pushboolean(L, m->%s()); else lua_pushnil(L);' % ( name.lower(), name.lower() ))

        elif type in [FieldDescriptor.TYPE_INT32, FieldDescriptor.TYPE_UINT32,
            FieldDescriptor.TYPE_FIXED32, FieldDescriptor.TYPE_SFIXED32, FieldDescriptor.TYPE_SINT32]:
            lines.append('if (m->has_%s()) lua_pushinteger(L, m->%s()); else lua_pushnil(L);' % ( name.lower(), name.lower() ))

        elif type in [ FieldDescriptor.TYPE_INT64, FieldDescriptor.TYPE_UINT64,
            FieldDescriptor.TYPE_FIXED64, FieldDescriptor.TYPE_SFIXED64, FieldDescriptor.TYPE_SINT64]:
            lines.append('if (m->has_%s()) lua_pushinteger(L, m->%s()); else lua_pushnil(L);' % ( name.lower(), name.lower() ))

        elif type == FieldDescriptor.TYPE_FLOAT or type == FieldDescriptor.TYPE_DOUBLE:
            lines.append('if (m->has_%s()) lua_pushnumber(L, m->%s()); else lua_pushnil(L);' % ( name.lower(), name.lower() ))

        elif type == FieldDescriptor.TYPE_ENUM:
            lines.append('if (m->has_%s()) lua_pushinteger(L, m->%s()); else lua_pushnil(L);' % ( name.lower(), name.lower() ))

        elif type == FieldDescriptor.TYPE_MESSAGE:
            lines.extend([
                'if (!m->has_%s()) {' % name.lower(),
                    'lua_pushnil(L);',
                '}',

                # we push the message as userdata
                # since the message is allocated out of the parent message, we
                # don't need to do garbage collection
                '%s * got_msg = m->mutable_%s();' % ( type_name.replace('.', '::'), name.lower() ),
                'lua_protobuf%s_pushreference(L, got_msg, NULL, NULL);' % type_name.replace('.', '_'),
            ])

        else:
            # not supported yet :(
            lines.append('return luaL_error(L, "lua-protobuf does not support this field type");')

    lines.append('return 1;')
    lines.append('}\n')

    return lines

def field_set_assignment(field, args):
    return [
        'if (index == current_size + 1) {',
            'm->add_%s(%s);' % ( field.lower(), args ),
        '}',
        'else {',
            'm->set_%s(index-1, %s);' % ( field.lower(), args ),
        '}',
    ]

def field_set(package, message, field_descriptor):
    '''Returns function definition for a set_<field> function'''

    name = field_descriptor.name
    type = field_descriptor.type
    type_name = field_descriptor.type_name
    label = field_descriptor.label
    repeated = label == FieldDescriptor.LABEL_REPEATED

    lines = []
    lines.extend(field_function_start(package, message, 'set', name.lower()))
    lines.extend(obtain_message_from_udata(package, message, 1))

    # we do things differently depending on if this is a singular or repeated field
    # for singular fields, the new value is the first argument
    # for repeated fields, the index is arg1 and the value is arg2
    if repeated:
        lines.extend([
            'if (lua_gettop(L) != 3) {',
            '    return luaL_error(L, "required 2 arguments not passed to function");',
            '}',
            'lua_Integer index = luaL_checkinteger(L, 2);',
            'int current_size = m->%s_size();' % name.lower(),
            'if (index < 1 || index > current_size + 1) {',
                'return luaL_error(L, "index must be between 1 and %d", current_size + 1);',
            '}',

            # we don't support the automagic nil clears value... yet
            'if (lua_isnil(L, 3)) {',
                'return luaL_error(L, "cannot assign nil to repeated fields (yet)");',
            '}',
        ])

    # TODO proper 64 bit handling

    # now move on to the assignment
    if repeated:
        if type in [ FieldDescriptor.TYPE_STRING, FieldDescriptor.TYPE_BYTES ]:
            lines.extend([
                'size_t length = 0;',
                'const char *s = luaL_checklstring(L, 3, &length);',
            ])
            lines.extend(field_set_assignment(name, 's, length'))

        elif type == FieldDescriptor.TYPE_BOOL:
            lines.append('bool b = !!lua_toboolean(L, 3);')
            lines.extend(field_set_assignment(name, 'b'))

        elif type in [ FieldDescriptor.TYPE_DOUBLE, FieldDescriptor.TYPE_FLOAT ]:
            lines.append('double d = lua_tonumber(L, 3);')
            lines.extend(field_set_assignment(name, 'd'))

        elif type in [ FieldDescriptor.TYPE_INT32, FieldDescriptor.TYPE_FIXED32,
            FieldDescriptor.TYPE_UINT32, FieldDescriptor.TYPE_SFIXED32, FieldDescriptor.TYPE_SINT32 ]:

            lines.append('lua_Integer i = lua_tointeger(L, 3);')
            lines.extend(field_set_assignment(name, 'i'))

        elif type in [ FieldDescriptor.TYPE_INT64, FieldDescriptor.TYPE_UINT64,
            FieldDescriptor.TYPE_FIXED64, FieldDescriptor.TYPE_SFIXED64, FieldDescriptor.TYPE_SINT64]:

            lines.append('lua_Integer i = lua_tointeger(L, 3);')
            lines.extend(field_set_assignment(name, 'i'))

        elif type == FieldDescriptor.TYPE_ENUM:
            lines.append('lua_Integer i = lua_tointeger(L, 3);')
            lines.extend(field_set_assignment(name, '(%s)i' % type_name.replace('.', '::')))

        elif type == FieldDescriptor.TYPE_MESSAGE:
            lines.append('return luaL_error(L, "to manipulate embedded messages, fetch the embedded message and modify it");')

        else:
            lines.append('return luaL_error(L, "field type not yet supported");')

        lines.append('return 0;')
    else:
        # if they call set() with nil, we interpret as a clear
        # this is the Lua way, after all
        lines.extend([
            'if (lua_isnil(L, 2)) {',
                'm->clear_%s();' % name.lower(),
                'return 0;',
            '}',
            '',
        ])

        if type in [ FieldDescriptor.TYPE_STRING, FieldDescriptor.TYPE_BYTES ]:
            lines.extend([
                'if (!lua_isstring(L, 2)) return luaL_error(L, "passed value is not a string");',
                'size_t len;',
                'const char *s = lua_tolstring(L, 2, &len);',
                'if (!s) {',
                    'luaL_error(L, "could not obtain string on stack. weird");',
                '}',
                'm->set_%s(s, len);' % name.lower(),
                'return 0;',
            ])

        elif type in [ FieldDescriptor.TYPE_DOUBLE, FieldDescriptor.TYPE_FLOAT ]:
            lines.extend([
                'if (!lua_isnumber(L, 2)) return luaL_error(L, "passed value cannot be converted to a number");',
                'lua_Number n = lua_tonumber(L, 2);',
                'm->set_%s(n);' % name.lower(),
                'return 0;',
            ])

        elif type in [ FieldDescriptor.TYPE_INT32, FieldDescriptor.TYPE_FIXED32,
            FieldDescriptor.TYPE_UINT32, FieldDescriptor.TYPE_SFIXED32, FieldDescriptor.TYPE_SINT32 ]:

            lines.extend([
                'lua_Integer v = luaL_checkinteger(L, 2);',
                'm->set_%s(v);' % name.lower(),
                'return 0;',
            ])

        elif type in [ FieldDescriptor.TYPE_INT64, FieldDescriptor.TYPE_UINT64,
            FieldDescriptor.TYPE_FIXED64, FieldDescriptor.TYPE_SFIXED64, FieldDescriptor.TYPE_SINT64]:

            lines.extend([
                'lua_Integer i = luaL_checkinteger(L, 2);',
                'm->set_%s(i);' % name.lower(),
                'return 0;',
            ])

        elif type == FieldDescriptor.TYPE_BOOL:
            lines.extend([
                'bool b = !!lua_toboolean(L, 2);',
                'm->set_%s(b);' % name.lower(),
                'return 0;',
            ])

        elif type == FieldDescriptor.TYPE_ENUM:
            lines.extend([
                'lua_Integer i = luaL_checkinteger(L, 2);',
                'm->set_%s((%s)i);' % ( name.lower(), type_name.replace('.', '::') ),
                'return 0;',
            ])

        elif type == FieldDescriptor.TYPE_MESSAGE:
            lines.append('return luaL_error(L, "to manipulate embedded messages, obtain the embedded message and manipulate it");')

        else:
            lines.append('return luaL_error(L, "field type is not yet supported");')

    lines.append('}\n')

    return lines

def new_message(package, message):
    '''Returns function definition for creating a new protocol buffer message'''

    lines = []

    lines.append('int %snew(lua_State *L)' % message_function_prefix(package, message))
    lines.append('{')

    c = cpp_class(package, message)
    lines.append('msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));')

    lines.append('ud->lua_owns = true;')
    lines.append('ud->msg = new %s();' % c)
    lines.append('ud->gc_callback = NULL;')
    lines.append('ud->callback_data = NULL;')

    lines.append('luaL_getmetatable(L, "%s");' % metatable(package, message))
    lines.append('lua_setmetatable(L, -2);')
    lines.append('return 1;')

    lines.append('}\n')

    return lines

def message_pushcopy_function(package, message):
    '''Returns function definition for pushing a copy of a message to the stack'''

    return [
        'bool %spushcopy(lua_State *L, const %s &from)' % ( message_function_prefix(package, message), cpp_class(package, message) ),
        '{',
        'msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));',
        'ud->lua_owns = true;',
        'ud->msg = new %s(from);' % cpp_class(package, message),
        'ud->gc_callback = NULL;',
        'ud->callback_data = NULL;',
        'luaL_getmetatable(L, "%s");' % metatable(package, message),
        'lua_setmetatable(L, -2);',
        'return true;',
        '}',
    ]

def message_pushreference_function(package, message):
    '''Returns function definition for pushing a reference of a message on the stack'''

    return [
        'bool %spushreference(lua_State *L, %s *msg, lua_protobuf_gc_callback f, void *data)' % ( message_function_prefix(package, message), cpp_class(package, message) ),
        '{',
        'msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));',
        'ud->lua_owns = false;',
        'ud->msg = msg;',
        'ud->gc_callback = f;',
        'ud->callback_data = data;',
        'luaL_getmetatable(L, "%s");' % metatable(package, message),
        'lua_setmetatable(L, -2);',
        'return true;',
        '}',
    ]

def parsefromstring_message_function(package, message):
    '''Returns function definition for parsing a message from a serialized string'''

    lines = []

    lines.append('int %sparsefromstring(lua_State *L)' % message_function_prefix(package, message))
    c = cpp_class(package, message)

    lines.extend([
        '{',
        'if (lua_gettop(L) != 1) {',
            'return luaL_error(L, "parsefromstring() requires a string argument. none given");',
        '}',

        'size_t len;',
        'const char *s = luaL_checklstring(L, -1, &len);',
        '%s * msg = new %s();' % ( c, c ),
        'if (!msg->ParseFromArray((const void *)s, len)) {',
            'return luaL_error(L, "error deserializing message");',
        '}',

        'msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));',
        'ud->lua_owns = true;',
        'ud->msg = msg;',
        'ud->gc_callback = NULL;',
        'ud->callback_data = NULL;',
        'luaL_getmetatable(L, "%s");' % metatable(package, message),
        'lua_setmetatable(L, -2);',

        'return 1;',
        '}',
    ])

    return lines
    
def label_to_string(label_value):
    if label_value == FieldDescriptor.LABEL_OPTIONAL:
        return "optional"
    if label_value == FieldDescriptor.LABEL_REPEATED:
        return "repeated"
    if label_value == FieldDescriptor.LABEL_REQUIRED:
        return "required"
        
def type_to_string(type_value):
    if type_value == FieldDescriptor.TYPE_BOOL:# = 8
        return "bool"
    if type_value == FieldDescriptor.TYPE_BYTES:# = 12
        return "bytes"
    if type_value == FieldDescriptor.TYPE_DOUBLE:# = 1
        return "double"
    if type_value == FieldDescriptor.TYPE_ENUM:# = 14
        return "enum"
    if type_value == FieldDescriptor.TYPE_FIXED32:# = 7
        return "fixed32"
    if type_value == FieldDescriptor.TYPE_FIXED64:# = 6
        return "fixed64"
    if type_value == FieldDescriptor.TYPE_FLOAT:# = 2
        return "float"
    if type_value == FieldDescriptor.TYPE_GROUP:# = 10
        return "group"
    if type_value == FieldDescriptor.TYPE_INT32:# = 5
        return "int32"
    if type_value == FieldDescriptor.TYPE_INT64:# = 3
        return "int64"
    if type_value == FieldDescriptor.TYPE_MESSAGE:# = 11
        return "message"
    if type_value == FieldDescriptor.TYPE_SFIXED32:# = 15
        return "sfixed32"
    if type_value == FieldDescriptor.TYPE_SFIXED64:# = 16
        return "sfixed64"
    if type_value == FieldDescriptor.TYPE_SINT32:# = 17
        return "sint32"
    if type_value == FieldDescriptor.TYPE_SINT64:# = 18
        return "sint64"
    if type_value == FieldDescriptor.TYPE_STRING:# = 9
        return "string"
    if type_value == FieldDescriptor.TYPE_UINT32:# = 13
        return "uint32"
    if type_value == FieldDescriptor.TYPE_UINT64:# = 4
        return "uint64"
    
def descriptor_message_function(package, message, descriptor):
    ''' Return a function that builds a table that describes message. Returns table to Lua for inspection'''
    lines = []
    
    lines.extend([
        'int %sdescriptor(lua_State* L)' % message_function_prefix(package, message),
        '{',
        '   lua_newtable(L);',
        '   ',
    ]);        
    for fields_descriptor in descriptor.field:
        lines.extend([
            '   // Field: default_value = %s' % fields_descriptor.default_value,
            '   lua_newtable(L);',
            '   lua_pushstring(L, "%s");' % fields_descriptor.name,
            '   lua_setfield(L, -2, "name");',
            '   lua_pushstring(L, "%s");' % label_to_string(fields_descriptor.label),
            '   lua_setfield(L, -2, "label");',
            '   lua_pushnumber(L, %s);' % fields_descriptor.number,
            '   lua_setfield(L, -2, "number");',
            '   lua_pushstring(L, "%s");' % type_to_string(fields_descriptor.type),
            '   lua_setfield(L, -2, "type");',
            '   lua_pushstring(L, "%s");' % (fields_descriptor.type_name) if fields_descriptor.type_name else '',
            '   lua_setfield(L, -2, "type_name");' if fields_descriptor.type_name else '',
            '   lua_setfield(L, -2, "%s");' % fields_descriptor.name,
        ]);
    
    lines.extend([
        '',
        '   return 1;',
        '}',
    ])
    return lines

def gc_message_function(package, message):
    '''Returns function definition for garbage collecting a message'''

    lines = [
        'int %sgc(lua_State *L)' % message_function_prefix(package, message),
        '{',
    ]
    lines.extend(obtain_message_from_udata(package, message, 1))
    # if Lua "owns" the message, we delete it
    # else, we delete only if a callback exists and it says it is OK
    lines.extend([
        'if (mud->lua_owns) {',
        'delete mud->msg;',
        'mud->msg = NULL;',
        'return 0;',
        '}',
        'if (mud->gc_callback && mud->gc_callback(m, mud->callback_data)) {',
        'delete mud->msg;',
        'mud->msg = NULL;',
        'return 0;',
        '}',
        'return 0;',
        '}',
    ])

    return lines

def clear_message_function(package, message):
    '''Returns the function definition for clearing a message'''

    lines = [
        'int %sclear(lua_State *L)' % message_function_prefix(package, message),
        '{'
    ]
    lines.extend(obtain_message_from_udata(package, message, 1))
    lines.extend([
        'm->Clear();',
        'return 0;',
        '}',
    ])

    return lines

def serialized_message_function(package, message):
    '''Returns the function definition for serializing a message and its length'''

    lines = [
        'int %sserialized(lua_State *L)' % message_function_prefix(package, message),
        '{'
    ]
    lines.extend(obtain_message_from_udata(package, message, 1))
    lines.extend([
        'string s;',
        'if (!m->SerializeToString(&s)) {',
        'return luaL_error(L, "error serializing message");',
        '}',
        'lua_pushlstring(L, s.c_str(), s.length());',
        'lua_pushnumber(L, s.length());',
        'return 2;',
        '}',
    ])

    return lines

def message_function_array(package, message):
    '''Defines functions for Lua object type

    These are defined on the Lua metatable for the message type.
    These are basically constructors and static methods in Lua land.
    '''
    return [
        'static const struct luaL_Reg %s_functions [] = {' % message,
        '{"new", %snew},' % message_function_prefix(package, message),
        '{"parsefromstring", %sparsefromstring},' % message_function_prefix(package, message),
        '{"descriptor", %sdescriptor},' % message_function_prefix(package, message),
        '{NULL, NULL}',
        '};\n',
    ]

def message_method_array(package, descriptor):
    '''Defines functions for Lua object instances

    These are functions available to each instance of a message.
    They take the object userdata as the first parameter.
    '''

    message = descriptor.name
    fp = message_function_prefix(package, message)

    lines = []
    lines.append('static const struct luaL_Reg %s_methods [] = {' % message)
    lines.append('{"serialized", %sserialized},' % fp)
    lines.append('{"clear", %sclear},' % fp)
    lines.append('{"__gc", %sgc},' % message_function_prefix(package, message))

    for fd in descriptor.field:
        name = fd.name
        label = fd.label
        type = fd.type

        lines.append('{"clear_%s", %s},' % ( name.lower(), field_function_name(package, message, 'clear', name.lower()) ))
        lines.append('{"get_%s", %s},' % ( name.lower(), field_function_name(package, message, 'get', name.lower()) ))
        lines.append('{"set_%s", %s},' % ( name.lower(), field_function_name(package, message, 'set', name.lower()) ))

        if label in [ FieldDescriptor.LABEL_REQUIRED, FieldDescriptor.LABEL_OPTIONAL ]:
            lines.append('{"has_%s", %s},' % ( name.lower(), field_function_name(package, message, 'has', name.lower()) ))

        if label == FieldDescriptor.LABEL_REPEATED:
            lines.append('{"size_%s", %s},' % ( name.lower(), field_function_name(package, message, 'size', name.lower()) ))

            if type == FieldDescriptor.TYPE_MESSAGE:
                lines.append('{"add_%s", %s},' % ( name.lower(), field_function_name(package, message, 'add', name.lower()) ))

    lines.append('{NULL, NULL},')
    lines.append('};\n')

    return lines

def message_open_function(package, descriptor):
    '''Function definition for opening/registering a message type'''

    message = descriptor.name

    lines = [
        'int %s(lua_State *L)' % message_open_function_name(package, message),
        '{',
        'luaL_checktype(L, -1, LUA_TTABLE);', #
        'luaL_newmetatable(L, "%s");' % metatable(package, message),
        'lua_pushvalue(L, -1);',
        'lua_setfield(L, -2, "__index");',
        'luaL_setfuncs(L, %s_methods, 0);' % message, ##'luaL_register(L, NULL, %s_methods);' % message,
        'lua_pop(L, 1); // remove the metatable', #
        'if (luaEXT_findtable(L, "%s", -1, 1)) { ' % package, #
        '   return luaL_error(L, "Error finding correct table");',
        '}',
        'luaL_newlib(L, %s_functions);' % message, ##'luaL_register(L, "%s", %s_functions);' % (lua_libname(package, message), message),
        'lua_setfield(L, -2, "%s");' % message, #
        'lua_pop(L, 1); //remove the returned table from findtable' #
    ]

    for enum_descriptor in descriptor.enum_type:
        lines.extend(enum_source(enum_descriptor))

    lines.extend([
        # this is wrong if we are calling through normal Lua module load means
        #'lua_pop(L, 1);',
        'return 0;',#'return 1;',
        '}',
        '\n',
    ])

    return lines

def message_header(package, message_descriptor):
    '''Returns the lines for a header definition of a message'''

    message_name = message_descriptor.name

    lines = []
    lines.append('// Message %s' % message_name)

    function_prefix = 'lua_protobuf_' + package.replace('.', '_') + '_'
    c = cpp_class(package, message_name)

    lines.extend([
        '// registers the message type with Lua',
        'LUA_PROTOBUF_EXPORT int %s(lua_State *L);\n' % message_open_function_name(package, message_name),
        '',
        '// push a copy of the message to the Lua stack',
        '// caller is free to use original message however she wants, but changes will not',
        '// be reflected in Lua and vice-verse',
        'LUA_PROTOBUF_EXPORT bool %s%s_pushcopy(lua_State *L, const %s &msg);' % ( function_prefix, message_name, c),
        '',
        '// push a reference of the message to the Lua stack',
        '// the 3rd and 4th arguments define a callback that can be invoked just before Lua',
        '// garbage collects the message. If the 3rd argument is NULL, Lua will *NOT* free',
        '// memory. If the second argument points to a function, that function is called when',
        '// Lua garbage collects the object. The function is sent a pointer to the message being',
        '// collected and the 4th argument to this function. If the function returns true,',
        '// Lua will free the memory. If false (0), Lua will not free the memory.',
        'LUA_PROTOBUF_EXPORT bool %s%s_pushreference(lua_State *L, %s *msg, lua_protobuf_gc_callback callback, void *data);' % ( function_prefix, message_name, c ),
        '',
        '',
        '// The following functions are called by Lua. Many people will not need them,',
        '// but they are exported for those that do.',
        '',
        '',
        '// constructor called from Lua',
        'LUA_PROTOBUF_EXPORT int %s%s_new(lua_State *L);' % ( function_prefix, message_name ),
        '',
        '// obtain instance from a serialized string',
        'LUA_PROTOBUF_EXPORT int %s%s_parsefromstring(lua_State *L);' % ( function_prefix, message_name ),
        '',
        '// obtain table of fields in this message',
        'LUA_PROTOBUF_EXPORT int %s%s_descriptor(lua_State* L);' % ( function_prefix, message_name),
        '',
        '// garbage collects message instance in Lua',
        'LUA_PROTOBUF_EXPORT int %s%s_gc(lua_State *L);' % ( function_prefix, message_name ),
        '',
        '// obtain serialized representation of instance',
        'LUA_PROTOBUF_EXPORT int %s%s_serialized(lua_State *L);' % ( function_prefix, message_name ),
        '',
        '// clear all fields in the message',
        'LUA_PROTOBUF_EXPORT int %s%s_clear(lua_State *L);' % ( function_prefix, message_name ),
        '',
    ])

    # each field defined in the message
    for field_descriptor in message_descriptor.field:
        field_name = field_descriptor.name
        field_number = field_descriptor.number
        field_label = field_descriptor.label
        field_type = field_descriptor.type
        field_default = field_descriptor.default_value

        if field_label not in FIELD_LABEL_MAP.keys():
            raise Exception('unknown field label constant: %s' % field_label)

        field_label_s = FIELD_LABEL_MAP[field_label]

        if field_type not in FIELD_TYPE_MAP.keys():
            raise Exception('unknown field type: %s' % field_type)

        field_type_s = FIELD_TYPE_MAP[field_type]

        lines.append('// %s %s %s = %d' % (field_label_s, field_type_s, field_name, field_number))
        lines.append('LUA_PROTOBUF_EXPORT int %s%s_clear_%s(lua_State *L);' % (function_prefix, message_name, field_name.lower()))
        lines.append('LUA_PROTOBUF_EXPORT int %s%s_get_%s(lua_State *L);' % (function_prefix, message_name, field_name.lower()))

        # TODO I think we can get rid of this for message types
        lines.append('LUA_PROTOBUF_EXPORT int %s%s_set_%s(lua_State *L);' % (function_prefix, message_name, field_name.lower()))

        if field_label in [ FieldDescriptor.LABEL_REQUIRED, FieldDescriptor.LABEL_OPTIONAL ]:
            lines.append('LUA_PROTOBUF_EXPORT int %s%s_has_%s(lua_State *L);' % (function_prefix, message_name, field_name.lower()))

        if field_label == FieldDescriptor.LABEL_REPEATED:
            lines.append('LUA_PROTOBUF_EXPORT int %s%s_size_%s(lua_State *L);' % (function_prefix, message_name, field_name.lower()))

            if field_type == FieldDescriptor.TYPE_MESSAGE:
                lines.append('LUA_PROTOBUF_EXPORT int %s%s_add_%s(lua_State *L);' % ( function_prefix, message_name, field_name.lower()))

        lines.append('')

    lines.append('// end of message %s\n' % message_name)

    return lines


def message_source(package, message_descriptor):
    '''Returns lines of source code for an individual message type'''
    lines = []

    message = message_descriptor.name

    lines.extend(message_function_array(package, message))
    lines.extend(message_method_array(package, message_descriptor))
    lines.extend(message_open_function(package, message_descriptor))
    lines.extend(message_pushcopy_function(package, message))
    lines.extend(message_pushreference_function(package, message))
    lines.extend(new_message(package, message))
    lines.extend(parsefromstring_message_function(package, message))
    lines.extend(descriptor_message_function(package, message, message_descriptor))
    lines.extend(gc_message_function(package, message))
    lines.extend(clear_message_function(package, message))
    lines.extend(serialized_message_function(package, message))

    for descriptor in message_descriptor.field:
        name = descriptor.name

        # clear() is in all label types
        lines.extend(field_function_start(package, message, 'clear', name))
        lines.extend(clear_body(package, message, name))
        lines.append('}\n')

        lines.extend(field_get(package, message, descriptor))
        lines.extend(field_set(package, message, descriptor))

        if descriptor.label in [FieldDescriptor.LABEL_OPTIONAL, FieldDescriptor.LABEL_REQUIRED]:
            # has_<field>()
            lines.extend(field_function_start(package, message, 'has', name))
            lines.extend(has_body(package, message, name))
            lines.append('}\n')

        if descriptor.label == FieldDescriptor.LABEL_REPEATED:
            # size_<field>()
            lines.extend(field_function_start(package, message, 'size', name))
            lines.extend(size_body(package, message, name))
            lines.append('}\n')

            if descriptor.type == FieldDescriptor.TYPE_MESSAGE:
                lines.extend(field_function_start(package, message, 'add', name))
                lines.extend(add_body(package, message, name, descriptor.type_name))
                lines.append('}\n')


    return lines

def enum_source(descriptor):
    '''Returns source code defining an enumeration type'''

    # this function assumes the module/table the enum should be assigned to
    # is at the top of the stack when it is called

    name = descriptor.name

    # enums are a little funky
    # at the core, there is a table whose keys are the enum string names and
    # values corresponding to the respective integer values. this table also
    # has a metatable with __index to throw errors when unknown enumerations
    # are accessed
    #
    # this table is then wrapped in a proxy table. the proxy table is empty
    # but has a metatable with __index and __newindex set. __index is the
    # table that actually contains the values. __newindex is a function that
    # always throws an error.
    #
    # we need the proxy table so we can intercept all requests for writes.
    # __newindex is only called for new keys, so we need an empty table so
    # all writes are sent to __newindex
    lines = [
        '// %s enum' % name,
        'lua_newtable(L); // proxy table',
        'lua_newtable(L); // main table',
    ]

    # assign enumerations to the table
    for value in descriptor.value:
        k = value.name
        v = value.number
        lines.extend([
            'lua_pushnumber(L, %d);' % v,
            'lua_setfield(L, -2, "%s");' % k
        ])

    # assign the metatable
    lines.extend([
        '// define metatable on main table',
        'lua_newtable(L);',
        'lua_pushcfunction(L, lua_protobuf_enum_index);',
        'lua_setfield(L, -2, "__index");',
        'lua_setmetatable(L, -2);',
        '',

        '// define metatable on proxy table',
        'lua_newtable(L);',
        # proxy meta: -1; main: -2; proxy: -3
        'lua_pushvalue(L, -2);',
        'lua_setfield(L, -2, "__index");',
        'lua_pushcfunction(L, lua_protobuf_enum_newindex);',
        'lua_setfield(L, -2, "__newindex");',
        'lua_remove(L, -2);',
        'lua_setmetatable(L, -2);',

        # proxy at top of stack now
        # assign to appropriate module
        'lua_setfield(L, -2, "%s");' % name,
        '// end %s enum' % name
    ])

    return lines

def file_header(file_descriptor):

    filename = file_descriptor.name
    package = file_descriptor.package

    lines = []

    lines.extend(c_header_header(filename, package))

    for descriptor in file_descriptor.message_type:
        lines.extend(message_header(package, descriptor))

    lines.append('#ifdef __cplusplus')
    lines.append('}')
    lines.append('#endif')
    lines.append('')
    lines.append('#endif')

    return '\n'.join(lines)

def file_source(file_descriptor):
    '''Obtains the source code for a FileDescriptor instance'''

    filename = file_descriptor.name
    package = file_descriptor.package

    lines = []
    lines.extend(source_header(filename, package, file_descriptor))
    lines.append('using ::std::string;\n')

    lines.extend([
        'int %sopen(lua_State *L)' % proto_function_open_name(filename),
        '{',
    ])

    # we populate enumerations as tables inside the protobuf global
    # variable/module
    # this is a little tricky, because we need to ensure all the parent tables
    # are present
    # i.e. protobuf.package.foo.enum => protobuf['package']['foo']['enum']
    # we interate over all the tables and create missing ones, as necessary

    # we cheat here and use the undocumented/internal luaL_findtable function
    # we probably shouldn't rely on an "internal" API, so
    # TODO don't use internal API call
    lines.extend([
        'luaL_checktype(L, -1, LUA_TTABLE);',
        'const char *table = luaEXT_findtable(L, "%s", -1, 1);' % package,
        'if (table) {',
            'return luaL_error(L, "could not create parent Lua tables");',
        '}',
        'if (!lua_istable(L, -1)) {',
            'return luaL_error(L, "could not create parent Lua tables");',
        '}',
    ])

    for descriptor in file_descriptor.enum_type:
        lines.extend(enum_source(descriptor))

    lines.extend([
        # don't need main table on stack any more
        'lua_pop(L, 1);',

        # and we register this package as a module, complete with enumerations
        #'luaL_Reg funcs [] = { { NULL, NULL } };',
        #'luaL_register(L, "protobuf.%s", funcs);' % package,
    ])

    for descriptor in file_descriptor.message_type:
        lines.append('%s(L);' % message_open_function_name(package, descriptor.name))

    lines.append('return 0;')
    lines.append('}')
    lines.append('\n')

    for descriptor in file_descriptor.message_type:
        lines.extend(message_source(package, descriptor))

    # perform some hacky pretty-printing
    formatted = []
    indent = 0
    for line in lines:
        if RE_BARE_BEGIN_BRACKET.search(line):
            formatted.append((' ' * indent) + line)
            indent += 4
        elif RE_BEGIN_BRACKET.search(line):
            formatted.append((' ' * indent) + line)
            indent += 4
        elif RE_END_BRACKET.search(line):
            if indent >= 4:
                indent -= 4
            formatted.append((' ' * indent) + line)
        else:
            formatted.append((' ' * indent) + line)

    return '\n'.join(formatted)

