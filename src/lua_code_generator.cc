

#include "lua_code_generator.h"

#include <memory>

#include <google/protobuf/testing/file.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
//#include <gtest/gtest.h>
#include <google/protobuf/stubs/stl_util.h>
#include <iostream>
#include <algorithm>
#include <sstream>


namespace google {
namespace protobuf {
namespace compiler {

static const char* getLabelString(FieldDescriptor::Label l) {
    if (l == FieldDescriptor::LABEL_OPTIONAL) return "optional";
    else if (l == FieldDescriptor::LABEL_REPEATED) return "repeated";
    else if (l == FieldDescriptor::LABEL_REQUIRED) return "required";
    return "";
}

LuaCodeGenerator::LuaCodeGenerator()
{}

LuaCodeGenerator::~LuaCodeGenerator() {}


bool LuaCodeGenerator::GenerateCommonFiles(const string& parameter, GeneratorContext* context, string* error) {
	//Output Common Code
	{
		std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open("proto_lua.h"));

		io::Printer printer(output.get(), '$');
static const char* header_str = 
"\
#ifndef LUA_PROTOBUF_H\n\
#define LUA_PROTOBUF_H\n\
\n\
#include <google/protobuf/message.h>\n\
\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
#include <lua.h>\n\
\n\
#ifdef WINDOWS\n\
#define LUA_PROTOBUF_EXPORT __declspec(dllexport)\n\
#else\n\
#define LUA_PROTOBUF_EXPORT\n\
#endif\n\
\n\
	// type for callback function that is executed before Lua performs garbage\n\
	// collection on a message instance.\n\
	// if called function returns 1, Lua will free the memory backing the object\n\
	// if returns 0, Lua will not free the memory\n\
	typedef int(*lua_protobuf_gc_callback)(::google::protobuf::MessageLite *msg, void *userdata);\n\
\n\
	// __index and __newindex functions for enum tables\n\
	LUA_PROTOBUF_EXPORT int lua_protobuf_enum_index(lua_State *L);\n\
	LUA_PROTOBUF_EXPORT int lua_protobuf_enum_newindex(lua_State *L);\n\
\n\
	// GC callback function that always returns true\n\
	LUA_PROTOBUF_EXPORT int lua_protobuf_gc_always_free(::google::protobuf::MessageLite *msg, void *userdata);\n\
\n\
	// A minimal Lua interface for coded input/output protobuf streams\n\
	int lua_protobuf_coded_streams_open(lua_State* L);\n\
\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
#endif\n\
";
		printer.PrintRaw(header_str);
	}
	{
		std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open("proto_lua.cpp"));

		io::Printer printer(output.get(), '$');
static const char* cpp_str =
"\
\n\
#include \"lua-protobuf.h\"\n\
\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
#include <lauxlib.h>\n\
\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
int lua_protobuf_enum_index(lua_State *L)\n\
{\n\
	return luaL_error(L, \"attempting to access undefined enumeration value: %s\", lua_tostring(L, 2));\n\
}\n\
\n\
int lua_protobuf_enum_newindex(lua_State *L)\n\
{\n\
	return luaL_error(L, \"cannot modify enumeration tables\");\n\
}\n\
\n\
int lua_protobuf_gc_always_free(::google::protobuf::MessageLite *msg, void *ud)\n\
{\n\
	return 1;\n\
}\n\
\n\
#include \"google/protobuf/io/coded_stream.h\"\n\
#include \"google/protobuf/io/zero_copy_stream_impl.h\"\n\
#include \"google/protobuf/io/zero_copy_stream_impl_lite.h\"\n\
#include <fcntl.h>\n\
#include <sys/stat.h>\n\
\n\
#if defined (_MSC_VER)\n\
#   include <io.h> // for open\n\
#else\n\
#   include <sys/types.h>\n\
#   define O_BINARY (0)\n\
#endif\n\
\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
\n\
int lua_protobuf_coded_input_stream_new(lua_State* L) {\n\
	const char* filepath = luaL_checkstring(L, 1);\n\
	int fd = open(filepath, O_RDONLY | O_BINARY, S_IREAD);\n\
	if (fd == -1) {\n\
		return luaL_error(L, \"Failed to open file %s\", filepath);\n\
	}\n\
	char* udataptr = (char*)lua_newuserdata(L, sizeof(::google::protobuf::io::CodedInputStream) + sizeof(::google::protobuf::io::FileInputStream));\n\
	auto instream = new (udataptr + sizeof(::google::protobuf::io::FileInputStream)) ::google::protobuf::io::FileInputStream(fd);\n\
	instream->SetCloseOnDelete(true);\n\
	auto codestream = new (udataptr) ::google::protobuf::io::CodedInputStream(instream);\n\
	luaL_setmetatable(L, \"protobuf_.CodedInputStream\");\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_gc(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	::google::protobuf::io::FileInputStream* filestream = (::google::protobuf::io::FileInputStream*)(codestream + 1);\n\
	codestream->~CodedInputStream();\n\
	filestream->~FileInputStream();\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_skip(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	int count = luaL_checkint(L, 2);\n\
	codestream->Skip(count);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_push_limit(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	int limit = luaL_checkint(L, 2);\n\
	limit = codestream->PushLimit(limit);\n\
	lua_pushinteger(L, limit);\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_pop_limit(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	int limit = luaL_checkint(L, 2);\n\
	codestream->PopLimit(limit);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_current_position(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	lua_pushinteger(L, codestream->CurrentPosition());\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_read_raw(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	int count = luaL_checkint(L, 2);\n\
	char* buf = new char[count];\n\
	bool success = codestream->ReadRaw(buf, count);\n\
	if (success) {\n\
		lua_pushlstring(L, buf, count);\n\
	}\n\
	else {\n\
		lua_pushnil(L);\n\
	}\n\
	delete buf;\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_read_varint_32(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	::google::protobuf::uint32 val;\n\
	bool success = codestream->ReadVarint32(&val);\n\
	lua_pushboolean(L, success);\n\
	if (success) {\n\
		lua_pushinteger(L, val);\n\
	}\n\
	else {\n\
		lua_pushnil(L);\n\
	}\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_read_varint_64(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	::google::protobuf::uint64 val;\n\
	bool success = codestream->ReadVarint64(&val);\n\
	lua_pushboolean(L, success);\n\
	if (success) {\n\
		lua_pushinteger(L, val);\n\
	}\n\
	else {\n\
		lua_pushnil(L);\n\
	}\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_read_little_endian_32(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	::google::protobuf::uint32 val;\n\
	bool success = codestream->ReadLittleEndian32(&val);\n\
	lua_pushboolean(L, success);\n\
	if (success) {\n\
		lua_pushinteger(L, val);\n\
	}\n\
	else {\n\
		lua_pushnil(L);\n\
	}\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_input_stream_read_little_endian_64(lua_State* L) {\n\
	::google::protobuf::io::CodedInputStream* codestream = (::google::protobuf::io::CodedInputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedInputStream\");\n\
	::google::protobuf::uint64 val;\n\
	bool success = codestream->ReadLittleEndian64(&val);\n\
	lua_pushboolean(L, success);\n\
	if (success) {\n\
		lua_pushinteger(L, val);\n\
	}\n\
	else {\n\
		lua_pushnil(L);\n\
	}\n\
	return 1;\n\
}\n\
\n\
static const struct luaL_Reg CodedInputStream_functions[] = {\n\
		{ \"new\", lua_protobuf_coded_input_stream_new },\n\
		{ NULL, NULL }\n\
};\n\
\n\
static const struct luaL_Reg CodedInputStream_methods[] = {\n\
		{ \"__gc\", lua_protobuf_coded_input_stream_gc },\n\
		{ \"Skip\", lua_protobuf_coded_input_stream_skip },\n\
		{ \"PushLimit\", lua_protobuf_coded_input_stream_push_limit },\n\
		{ \"PopLimit\", lua_protobuf_coded_input_stream_pop_limit },\n\
		{ \"CurrentPosition\", lua_protobuf_coded_input_stream_current_position },\n\
		{ \"ReadRaw\", lua_protobuf_coded_input_stream_read_raw },\n\
		{ \"ReadVarint32\", lua_protobuf_coded_input_stream_read_varint_32 },\n\
		{ \"ReadVarint64\", lua_protobuf_coded_input_stream_read_varint_64 },\n\
		{ \"ReadLittleEndian32\", lua_protobuf_coded_input_stream_read_little_endian_32 },\n\
		{ \"ReadLittleEndian64\", lua_protobuf_coded_input_stream_read_little_endian_64 },\n\
		{ NULL, NULL },\n\
};\n\
\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
\n\
int lua_protobuf_coded_output_stream_new(lua_State* L) {\n\
	const char* filepath = luaL_checkstring(L, 1);\n\
	int fd = open(filepath, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, S_IREAD | S_IWRITE);\n\
	if (fd == -1) {\n\
		return luaL_error(L, \"Failed to open file %s\", filepath);\n\
	}\n\
	char* udataptr = (char*)lua_newuserdata(L, sizeof(::google::protobuf::io::CodedOutputStream) + sizeof(::google::protobuf::io::FileOutputStream));\n\
	auto outstream = new(udataptr + sizeof(::google::protobuf::io::CodedOutputStream)) ::google::protobuf::io::FileOutputStream(fd);\n\
	outstream->SetCloseOnDelete(true);\n\
	auto codestream = new (udataptr) ::google::protobuf::io::CodedOutputStream(outstream);\n\
	luaL_setmetatable(L, \"protobuf_.CodedOutputStream\");\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_gc(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	::google::protobuf::io::FileOutputStream* filestream = (::google::protobuf::io::FileOutputStream*)(codestream + 1);\n\
	codestream->~CodedOutputStream();\n\
	filestream->~FileOutputStream();\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_skip(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	int count = luaL_checkint(L, 2);\n\
	codestream->Skip(count);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_byte_count(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	lua_pushinteger(L, codestream->ByteCount());\n\
	return 1;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_write_raw(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	size_t count;\n\
	const char* buf = luaL_checklstring(L, 2, &count);\n\
	codestream->WriteRaw(buf, (int)count);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_write_varint_32(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	::google::protobuf::uint32 val = luaL_checkunsigned(L, 2);\n\
	codestream->WriteVarint32(val);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_write_varint_64(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	::google::protobuf::uint64 val = luaL_checkunsigned(L, 2);\n\
	codestream->WriteVarint64(val);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_write_little_endian_32(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	::google::protobuf::uint32 val = luaL_checkunsigned(L, 2);\n\
	codestream->WriteLittleEndian32(val);\n\
	return 0;\n\
}\n\
\n\
int lua_protobuf_coded_output_stream_write_little_endian_64(lua_State* L) {\n\
	::google::protobuf::io::CodedOutputStream* codestream = (::google::protobuf::io::CodedOutputStream*)luaL_checkudata(L, 1, \"protobuf_.CodedOutputStream\");\n\
	::google::protobuf::uint64 val = luaL_checkunsigned(L, 2);\n\
	codestream->WriteLittleEndian64(val);\n\
	return 0;\n\
}\n\
\n\
static const struct luaL_Reg CodedOutputStream_functions[] = {\n\
		{ \"new\", lua_protobuf_coded_output_stream_new },\n\
		{ NULL, NULL }\n\
};\n\
\n\
static const struct luaL_Reg CodedOutputStream_methods[] = {\n\
		{ \"__gc\", lua_protobuf_coded_output_stream_gc },\n\
		{ \"Skip\", lua_protobuf_coded_output_stream_skip },\n\
		{ \"ByteCount\", lua_protobuf_coded_output_stream_byte_count },\n\
		{ \"WriteRaw\", lua_protobuf_coded_output_stream_write_raw },\n\
		{ \"WriteVarint32\", lua_protobuf_coded_output_stream_write_varint_32 },\n\
		{ \"WriteVarint64\", lua_protobuf_coded_output_stream_write_varint_64 },\n\
		{ \"WriteLittleEndian32\", lua_protobuf_coded_output_stream_write_little_endian_32 },\n\
		{ \"WriteLittleEndian64\", lua_protobuf_coded_output_stream_write_little_endian_64 },\n\
		{ NULL, NULL },\n\
};\n\
\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
//////////////////////////////////////////////////////////////////////////\n\
\n\
static const struct luaL_Reg CodedInputStream_lib_functions[] = {\n\
		{ NULL, NULL }\n\
};\n\
\n\
int lua_protobuf_coded_streams_open(lua_State* L) {\n\
	luaL_checktype(L, -1, LUA_TTABLE);\n\
\n\
	luaL_newmetatable(L, \"protobuf_.CodedInputStream\");\n\
	lua_pushvalue(L, -1);\n\
	lua_setfield(L, -2, \"__index\");\n\
	luaL_setfuncs(L, CodedInputStream_methods, 0);\n\
	lua_pop(L, 1);//pop the metatable\n\
\n\
	luaL_newmetatable(L, \"protobuf_.CodedOutputStream\");\n\
	lua_pushvalue(L, -1);\n\
	lua_setfield(L, -2, \"__index\");\n\
	luaL_setfuncs(L, CodedOutputStream_methods, 0);\n\
	lua_pop(L, 1);//pop the metatable\n\
\n\
	// add create funcs and tables\n\
	luaL_newlib(L, CodedInputStream_functions);\n\
	lua_setfield(L, -2, \"CodedInputStream\");\n\
	luaL_newlib(L, CodedOutputStream_functions);\n\
	lua_setfield(L, -2, \"CodedOutputStream\");\n\
	return 0;\n\
}\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
	const char *luaEXT_findtable(lua_State *L, const char *fname, int idx, int szhint) {\n\
		const char *e;\n\
		if (idx) lua_pushvalue(L, idx);\n\
		do {\n\
			e = strchr(fname, '.');\n\
			if (e == NULL) e = fname + strlen(fname);\n\
			lua_pushlstring(L, fname, e - fname);\n\
			lua_rawget(L, -2);\n\
			if (lua_isnil(L, -1)) {  /* no such field? */\n\
				lua_pop(L, 1);  /* remove this nil */\n\
				lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */\n\
				lua_pushlstring(L, fname, e - fname);\n\
				lua_pushvalue(L, -2);\n\
				lua_settable(L, -4);  /* set new table into field */\n\
			}\n\
			else if (!lua_istable(L, -1)) {  /* field has a non-table value? */\n\
				lua_pop(L, 2);  /* remove table and value */\n\
				return fname;  /* return problematic part of the name */\n\
			}\n\
			lua_remove(L, -2);  /* remove previous table */\n\
			fname = e + 1;\n\
		} while (*e == '.');\n\
		return NULL;\n\
	}\n\
\n\
#ifdef __cplusplus\n\
}\n\
#endif\n\
";
		printer.PrintRaw(cpp_str);
	}

	return true;
}

string LuaCodeGenerator::GetOutputFileName(const string& generator_name, const FileDescriptor* file) {
    return GetOutputFileName(generator_name, file->name());
}

string LuaCodeGenerator::GetOutputFileName(const string& generator_name, const string& file) {
	auto newfilename = file;
	newfilename.replace(newfilename.find(".proto"), string::npos, generator_name);
    return newfilename;
}

bool LuaCodeGenerator::Generate(
    const FileDescriptor* file,
    const string& parameter,
    GeneratorContext* context,
    string* error) const {

    {
    std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(GetOutputFileName(".pb.lua.h", file)));
    string package = file->package();
    std::replace(package.begin(), package.end(), '.', '_');
    map<string, string> parameters;
    parameters["src_filename"] = file->name();
    parameters["filename_base"] = GetOutputFileName("", file);
    parameters["package"] = package;
    parameters["function_prefix"] = string("lua_protobuf_") + package + string("_");
    parameters["todo"] = "!?!fix_me!?!";
    io::Printer printer(output.get(), '$');
    printer.Print(parameters,
"\
// Generated by the lua-protobuf compiler.\n\
// You shouldn\'t be editing this file manually\n\
//\n\
// source proto file: $src_filename$\n\
\n\
#ifndef LUA_PROTOBUF_$package$_$filename_base$_H\n\
#define LUA_PROTOBUF_$package$_$filename_base$_H\n\
\n\
#include \"lua-protobuf.h\"\n\
#include <$filename_base$.pb.h>\n\
\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
#include <lua.h>\n\
\n\
const char* luaEXT_findtable (lua_State*, const char*, int, int);\n\
\n\
// register all messages in this package to a Lua state\n\
LUA_PROTOBUF_EXPORT int lua_protobuf_$filename_base$_open(lua_State *L);\n\
\n\
"
        );
	for (auto i=0, n=file->message_type_count(); i < n; ++i) {
		auto* message_desc = file->message_type(i);
		string cpp_classname = file->package() + "::" + message_desc->name();
        size_t loc = 0;
		while (loc = cpp_classname.find(".", loc)) {
            if(loc == string::npos) {
                break;
            }
            cpp_classname.replace(loc, 1, "::");
        }
		parameters["message_name"] = message_desc->name();
		parameters["cpp_class"] = cpp_classname;
		parameters["todo"] = "!?!fix_me!?!";
        printer.Print(parameters,
"\n\
// registers the message type with Lua\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_open(lua_State *L);\n\
\n\
// push a copy of the message to the Lua stack\n\
// caller is free to use original message however she wants, but changes will not\n\
// be reflected in Lua and vice-verse\n\
LUA_PROTOBUF_EXPORT bool $function_prefix$$message_name$_pushcopy(lua_State *L, const $cpp_class$ &msg);\n\
\n\
// push a reference of the message to the Lua stack\n\
// the 3rd and 4th arguments define a callback that can be invoked just before Lua\n\
// garbage collects the message. If the 3rd argument is NULL, Lua will *NOT* free\n\
// memory. If the second argument points to a function, that function is called when\n\
// Lua garbage collects the object. The function is sent a pointer to the message being\n\
// collected and the 4th argument to this function. If the function returns true,\n\
// Lua will free the memory. If false (0), Lua will not free the memory.\n\
LUA_PROTOBUF_EXPORT bool $function_prefix$$message_name$_pushreference(lua_State *L, $cpp_class$ *msg, lua_protobuf_gc_callback callback, void *data);\n\
\n\
// get a copy of the message from the Lua stack\n\
// caller is free to use the new message however she wants, but changes will not\n\
// be reflected in Lua and vice-verse\n\
LUA_PROTOBUF_EXPORT bool $function_prefix$$message_name$_getcopy(lua_State *L, int index, $cpp_class$ &msg);\n\
// constructor called from Lua\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_new(lua_State *L);\n\
// obtain instance from a serialized string\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_parsefromstring(lua_State *L);\n\
// obtain table of fields in this message\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_descriptor(lua_State* L);\n\
// garbage collects message instance in Lua\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_gc(lua_State *L);\n\
// obtain serialized representation of instance\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_serialized(lua_State *L);\n\
// clear all fields in the message\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_clear(lua_State *L);\n\
"
        );

        // each field defined in the message
        for (auto fi=0, fn=message_desc->field_count(); fi < fn; ++fi) {
            auto* field = message_desc->field(fi);
            parameters["field_name"] = field->name();
            parameters["field_name_lower"] = field->name();
            parameters["field_label"] = field->label();
            parameters["field_type"] = field->cpp_type_name();
            std::transform(parameters["field_name_lower"].begin(), parameters["field_name_lower"].end(), parameters["field_name_lower"].begin(), tolower);
            printer.Print(parameters,
"\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_clear_$field_name_lower$(lua_State *L);\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_get_$field_name_lower$(lua_State *L);\n\
LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_set_$field_name_lower$(lua_State *L);\n\
"
);
            if (field->label() == FieldDescriptor::LABEL_OPTIONAL || field->label() == FieldDescriptor::LABEL_REQUIRED) {
                printer.Print(parameters, "LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_has_$field_name_lower$(lua_State *L);\n");
            }
            if (field->label() == FieldDescriptor::LABEL_REPEATED) {
                printer.Print(parameters, "LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_size_$field_name_lower$(lua_State *L);\n");
                if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
                    printer.Print(parameters, "LUA_PROTOBUF_EXPORT int $function_prefix$$message_name$_add_$field_name_lower$(lua_State *L);\n");
                }
            }
        }
	}
    printer.PrintRaw(
"\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
#endif\n\
\n");
    }
    {
    std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(GetOutputFileName(".pb.lua.cc", file)));
    io::Printer printer(output.get(), '$');
    string package = file->package();
    std::replace(package.begin(), package.end(), '.', '_');
    map<string, string> parameters;
    parameters["src_filename"] = file->name();
    parameters["filename_base"] = GetOutputFileName("", file);
    parameters["header_filename"] = GetOutputFileName(".pb.lua.h", file);
    parameters["package"] = package;
    parameters["dotted_package"] = file->package();
    parameters["function_prefix"] = string("lua_protobuf_") + package + string("_");
    parameters["open_function_prefix"] = string("lua_protobuf_") + GetOutputFileName("", file) + string("_");
    parameters["todo"] = "!?!fix_me!?!";
    printer.Print(parameters,
        "// Generated by the lua-protobuf compiler\n"
        "// You shouldn\'t edit this file manually\n"
        "//\n"
        "// source proto file: $src_filename$\n"
        "#include \"$header_filename$\"\n");
    for (auto i=0, n=file->dependency_count(); i<n; ++i) {
        auto* dep_desc = file->dependency(i);
        parameters["dependency_header_filename"] = GetOutputFileName(".pb.lua.h", dep_desc->name());
        printer.Print(parameters, "#include \"$dependency_header_filename$\"\n");
    }
    printer.Print(parameters,
		"#ifdef __cplusplus\n"
		"extern \"C\" { // make sure functions treated with C naming\n"
		"#endif\n"
		"\n"
		"#include <lauxlib.h>\n"
		"\n"
		"#ifdef __cplusplus\n"
		"}\n"
		"#endif\n"
		"\n"
		"#include <string>\n"
		"\n"
		"// this represents Lua udata for a protocol buffer message\n"
		"// we record where a message came from so we can GC it properly\n"
		"typedef struct msg_udata { // confuse over-simplified pretty-printer\n"
		"  ::google::protobuf::MessageLite * msg;\n"
		"  bool lua_owns;\n"
		"  lua_protobuf_gc_callback gc_callback;\n"
		"  void * callback_data;\n"
		"} msg_udata;\n"
		"using std::string;\n");
    printer.Print(parameters, "int $open_function_prefix$open(lua_State *L)\n{\n");
    printer.Indent();
    // we populate enumerations as tables inside the protobuf global
    // variable / module
    // this is a little tricky, because we need to ensure all the parent tables
    // are present
    // i.e.protobuf.package.foo.enum = > protobuf['package']['foo']['enum']
    // we interate over all the tables and create missing ones, as necessary
    // 
    // we cheat here and use the undocumented / internal luaL_findtable function
    // we probably shouldn't rely on an "internal" API, so
    // TODO don't use internal API call
    printer.Print(parameters,
		"luaL_checktype(L, -1, LUA_TTABLE);\n"
		"const char *table = luaEXT_findtable(L, \"$dotted_package$\", -1, 1);\n"
		"if (table) {\n"
		"  return luaL_error(L, \"could not create parent Lua tables\");\n"
		"}\n"
		"if (!lua_istable(L, -1)) {\n"
		"  return luaL_error(L, \"could not create parent Lua tables\");\n"
		"}\n");
    for (auto i=0, n=file->enum_type_count(); i<n; ++i) {
        // Create a read only table in Lua for enum values.
        auto* enum_desc = file->enum_type(i);
        generateEnumTable(parameters, enum_desc, printer);

    }
    printer.Print(parameters,
        "lua_pop(L, 1);\n");

    for (auto i = 0, n = file->message_type_count(); i < n; ++i) {
        auto* message_desc = file->message_type(i);
        parameters["message_name"] = message_desc->name();
        printer.Print(parameters,
            "$function_prefix$$message_name$_open(L);\n");
    }

    printer.Print(parameters, "return 0;\n");
    printer.Outdent();
    printer.Print(parameters, "}\n");

    for (auto i = 0, n = file->message_type_count(); i < n; ++i) {
        auto* message_desc = file->message_type(i);
        string cpp_classname = file->package() + "::" + message_desc->name();
        size_t loc = 0;
        while (loc = cpp_classname.find(".", loc)) {
            if (loc == string::npos) {
                break;
            }
            cpp_classname.replace(loc, 1, "::");
        }
        parameters["message_name"] = message_desc->name();
        parameters["cpp_class"] = cpp_classname;
        printer.Print(parameters, 
            "static const struct luaL_Reg $message_name$_functions [] = {\n"
            "  {\"new\", $function_prefix$$message_name$_new},\n"
            "  {\"parsefromstring\", $function_prefix$$message_name$_parsefromstring},\n"
            "  {\"descriptor\", $function_prefix$$message_name$_descriptor},\n"
            "  {NULL, NULL}\n"
            "};\n");
        printer.Print(parameters, "static const struct luaL_Reg $message_name$_methods [] = {\n");
        printer.Indent();
        printer.Print(parameters, 
            "{\"serialized\", $function_prefix$$message_name$_serialized},\n"
            "{\"clear\", $function_prefix$$message_name$_clear},\n"
            "{\"__gc\", $function_prefix$$message_name$_gc},\n"
            );
        // each field defined in the message
        for (auto fi = 0, fn = message_desc->field_count(); fi < fn; ++fi) {
            auto* field = message_desc->field(fi);
            parameters["field_name"] = field->name();
            parameters["field_name_lower"] = field->name();
            parameters["field_label"] = getLabelString(field->label());
            parameters["field_type"] = field->cpp_type_name();
            std::transform(parameters["field_name_lower"].begin(), parameters["field_name_lower"].end(), parameters["field_name_lower"].begin(), tolower);
            printer.Print(parameters,
                "{\"clear_$field_name_lower$\", $function_prefix$$message_name$_clear_$field_name_lower$},\n"
                "{\"get_$field_name_lower$\", $function_prefix$$message_name$_get_$field_name_lower$},\n"
                "{\"set_$field_name_lower$\", $function_prefix$$message_name$_set_$field_name_lower$},\n"
                );
            if (field->label() == FieldDescriptor::LABEL_OPTIONAL || field->label() == FieldDescriptor::LABEL_REQUIRED) {
                printer.Print(parameters, "{\"has_$field_name_lower$\", $function_prefix$$message_name$_has_$field_name_lower$},\n");
            }
            if (field->label() == FieldDescriptor::LABEL_REPEATED) {
                printer.Print(parameters, "{\"size_$field_name_lower$\", $function_prefix$$message_name$_size_$field_name_lower$},\n");
                if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
                    printer.Print(parameters, "{\"add_$field_name_lower$\", $function_prefix$$message_name$_add_$field_name_lower$},\n");
                }
            }
        }
        printer.Outdent();
        printer.Print(parameters, "{NULL, NULL},\n");
        printer.Print(parameters, "};\n");

        //open function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_open(lua_State *L)\n"
            "{\n");
        printer.Indent();
        printer.Print(parameters,
            "luaL_checktype(L, -1, LUA_TTABLE);\n"
            "luaL_newmetatable(L, \"protobuf_.$dotted_package$.$message_name$\");\n"
            "lua_pushvalue(L, -1);\n"
            "lua_setfield(L, -2, \"__index\");\n"
            "luaL_setfuncs(L, $message_name$_methods, 0);\n"
            "lua_pop(L, 1); // remove the metatable\n"
            "if (luaEXT_findtable(L, \"$dotted_package$\", -1, 1)) { \n"
            "  return luaL_error(L, \"Error finding correct table\");\n"
            "}\n"
            "luaL_newlib(L, $message_name$_functions);\n"
            "lua_setfield(L, -2, \"$message_name$\");\n"
            "lua_pop(L, 1); //remove the returned table from findtable\n" );

        for (auto ei=0, en=message_desc->enum_type_count(); ei<en; ++ei) {
            auto* enum_desc = message_desc->enum_type(ei);
            generateEnumTable(parameters, enum_desc, printer);
        }
        printer.PrintRaw("return 0;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //pushcopy function
        printer.Print(parameters, 
            "bool $function_prefix$$message_name$_pushcopy(lua_State *L, const $cpp_class$ &from)\n"
            "{\n");
        printer.Indent();
        printer.Print(parameters, 
            "msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));\n"
            "ud->lua_owns = true;\n"
            "ud->msg = new $cpp_class$(from);\n"
            "ud->gc_callback = NULL;\n"
            "ud->callback_data = NULL;\n"
            "luaL_getmetatable(L, \"protobuf_.$dotted_package$.$message_name$\");\n"
            "lua_setmetatable(L, -2);\n"
            "return true;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //pushreference function
        printer.Print(parameters,
            "bool $function_prefix$$message_name$_pushreference(lua_State *L, $cpp_class$ *msg, lua_protobuf_gc_callback f, void *data)\n"
            "{\n");
        printer.Indent();
        printer.Print(parameters,
            "msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));\n"
            "ud->lua_owns = false;\n"
            "ud->msg = msg;\n"
            "ud->gc_callback = f;\n"
            "ud->callback_data = data;\n"
            "luaL_getmetatable(L, \"protobuf_.$dotted_package$.$message_name$\");\n"
            "lua_setmetatable(L, -2);\n"
            "return true;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //new message function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_new(lua_State *L)\n"
            "{\n");
        printer.Indent();
        printer.Print(parameters,
            "msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));\n"
            "ud->lua_owns = true;\n"
            "ud->msg = new $cpp_class$();\n"
            "ud->gc_callback = NULL;\n"
            "ud->callback_data = NULL;\n"
            "luaL_getmetatable(L, \"protobuf_.$dotted_package$.$message_name$\");\n"
            "lua_setmetatable(L, -2);\n"
            "return 1;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //parsefromstring message function
        printer.Print(parameters, 
            "int $function_prefix$$message_name$_parsefromstring(lua_State* L)\n"
            "{\n");
        printer.Indent();
        printer.Print(parameters, 
            "if (lua_gettop(L) != 1) {\n"
            "  return luaL_error(L, \"parsefromstring() requires a string argument. none given\");\n"
            "}\n"
            "size_t len;\n"
            "const char *s = luaL_checklstring(L, -1, &len);\n"
            "$cpp_class$* msg = new $cpp_class$();\n"
            "if (!msg->ParseFromArray((const void *)s, len)) {\n"
            "  delete msg\n"
            "  return luaL_error(L, \"error deserializing message\");\n"
            "}\n"
            "msg_udata * ud = (msg_udata *)lua_newuserdata(L, sizeof(msg_udata));\n"
            "ud->lua_owns = true;\n"
            "ud->msg = msg;\n"
            "ud->gc_callback = NULL;\n"
            "ud->callback_data = NULL;\n"
            "luaL_getmetatable(L, \"protobuf_.$dotted_package$.$message_name$\");\n"
            "lua_setmetatable(L, -2);\n"
            "return 1;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        // descriptor_message_function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_descriptor(lua_State* L)\n"
            "{\n");
        printer.Indent();
        printer.PrintRaw("lua_newtable(L);\n");
        for (auto fi = 0, fn = message_desc->field_count(); fi < fn; ++fi) {
            auto* field = message_desc->field(fi);
            stringstream ss;
            ss << field->number();
            parameters["field_name"] = field->name();
            parameters["field_name_lower"] = field->name();
            parameters["field_label"] = getLabelString(field->label());
            parameters["field_type"] = field->cpp_type_name();
            parameters["field_number"] = ss.str();
            std::transform(parameters["field_name_lower"].begin(), parameters["field_name_lower"].end(), parameters["field_name_lower"].begin(), tolower);
            printer.Print(parameters,
                "// Field: default_value = $todo$\n"
                "lua_newtable(L);\n"
                "lua_pushstring(L, \"$field_name$\");\n"
                "lua_setfield(L, -2, \"name\");\n"
                "lua_pushstring(L, \"$field_label$\");\n"
                "lua_setfield(L, -2, \"label\");\n"
                "lua_pushnumber(L, $field_number$);\n"
                "lua_setfield(L, -2, \"number\");\n"
                "lua_pushstring(L, \"$field_type$\");\n"
                "lua_setfield(L, -2, \"type\");\n"
                "lua_setfield(L, -2, \"$field_name$\");\n");
        }
        printer.PrintRaw("return 1;");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //gc_message_function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_gc(lua_State* L)\n"
            "{\n");
        printer.Indent();
        getMessageFromUData(parameters, printer, "1", "m");
        printer.Print(parameters,
            "if (mud->lua_owns) {\n"
            "  delete mud->msg;\n"
            "  mud->msg = NULL;\n"
            "  return 0;\n"
            "}\n"
            "if (mud->gc_callback && mud->gc_callback(m, mud->callback_data)) {\n"
            "  delete mud->msg;\n"
            "  mud->msg = NULL;\n"
            "  return 0;\n"
            "}\n"
            "return 0;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        //clear_message_function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_clear(lua_State* L)\n"
            "{\n");
        printer.Indent();
        getMessageFromUData(parameters, printer, "1", "m");
        printer.PrintRaw(
            "m->Clear()\n"
            "return 0;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");

        // serialized_message_function
        printer.Print(parameters,
            "int $function_prefix$$message_name$_serialized(lua_State* L)\n"
            "{\n");
        printer.Indent();
        getMessageFromUData(parameters, printer, "1", "m");
        printer.Print(parameters,
            "string s;\n"
            "if (!m->SerializeToString(&s)) {\n"
            "return luaL_error(L, \"error serializing message\");\n"
            "}\n"
            "lua_pushlstring(L, s.c_str(), s.length());\n"
            "lua_pushnumber(L, s.length());\n"
            "return 2;\n");
        printer.Outdent();
        printer.PrintRaw("}\n");
    }

    }
    return true;
}


void LuaCodeGenerator::getMessageFromUData(map<string, string>& parameters, io::Printer& printer, const char* index, const char* varname) const {
    parameters["index"] = index;
    parameters["tmp_var"] = varname;
    printer.Print(parameters,
        "msg_udata * $tmp_var$ud = (msg_udata*)luaL_checkudata(L, $index$, \"protobuf_.$dotted_package$.$message_name$\");\n"
        "$cpp_class$ *$tmp_var$ = ($cpp_class$*)$tmp_var$ud->msg;\n");
}

void LuaCodeGenerator::generateEnumTable(map<string, string>& parameters, const EnumDescriptor* enum_desc, io::Printer& printer) const {
    parameters["enum_name"] = enum_desc->name();
    printer.Print(parameters,
        "// $enum_name$ enum\n"
        "lua_newtable(L); // proxy table\n"
        "lua_newtable(L); // main table\n");
    for (auto ei = 0, en = enum_desc->value_count(); ei < en; ++ei) {
        stringstream ss;
        ss << enum_desc->value(ei)->number();
        parameters["enum_value_name"] = enum_desc->value(ei)->name();
        parameters["enum_value"] = ss.str();;
        printer.Print(parameters,
            "lua_pushnumber(L, $enum_value$);\n"
            "lua_setfield(L, -2, \"$enum_value_name$\");\n");
    }

    printer.Print(parameters,
        "// define metatable on main table\n"
        "lua_newtable(L);\n"
        "lua_pushcfunction(L, lua_protobuf_enum_index);\n"
        "lua_setfield(L, -2, \"__index\");\n"
        "lua_setmetatable(L, -2);\n"
        "// define metatable on proxy table\n"
        "lua_newtable(L);\n"
        // proxy meta : -1; main: -2; proxy: -3
        "lua_pushvalue(L, -2);\n"
        "lua_setfield(L, -2, \"__index\");\n"
        "lua_pushcfunction(L, lua_protobuf_enum_newindex);\n"
        "lua_setfield(L, -2, \"__newindex\");\n"
        "lua_remove(L, -2);\n"
        "lua_setmetatable(L, -2);\n"
        // proxy at top of stack now
        // assign to appropriate module
        "lua_setfield(L, -2, \"$enum_name$\");\n"
        "// end $enum_name$ enum\n");
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
