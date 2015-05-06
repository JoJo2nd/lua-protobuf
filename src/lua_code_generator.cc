// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)

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

#define EXPECT_EQ(...)

namespace google {
namespace protobuf {
namespace compiler {

// Returns the list of the names of files in all_files in the form of a
// comma-separated string.
string CommaSeparatedList(const vector<const FileDescriptor*> all_files) {
  vector<string> names;
  for (int i = 0; i < all_files.size(); i++) {
    names.push_back(all_files[i]->name());
  }
  return Join(names, ",");
}

static const char* kFirstInsertionPointName = "first_mock_insertion_point";
static const char* kSecondInsertionPointName = "second_mock_insertion_point";
static const char* kFirstInsertionPoint =
    "# @@protoc_insertion_point(first_mock_insertion_point) is here\n";
static const char* kSecondInsertionPoint =
    "  # @@protoc_insertion_point(second_mock_insertion_point) is here\n";

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
    std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(GetOutputFileName("pb.lua.h", file)));

    io::Printer printer(output.get(), '$');
    printer.PrintRaw("//Output some code header code here\n"/*GetOutputFileContent(name_, parameter,file, context)*/);
    printer.PrintRaw(kFirstInsertionPoint);
    printer.PrintRaw(kSecondInsertionPoint);
    }
    {
    std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(GetOutputFileName("pb.lua.cc", file)));

    io::Printer printer(output.get(), '$');
    printer.PrintRaw("//Output some code cpp code here\n"/*GetOutputFileContent(name_, parameter,file, context)*/);
    printer.PrintRaw(kFirstInsertionPoint);
    printer.PrintRaw(kSecondInsertionPoint);
    }
    return true;
#if 0
  for (int i = 0; i < file->message_type_count(); i++) {
    if (HasPrefixString(file->message_type(i)->name(), "MockCodeGenerator_")) {
      string command = StripPrefixString(file->message_type(i)->name(),
                                         "MockCodeGenerator_");
      if (command == "Error") {
        *error = "Saw message type MockCodeGenerator_Error.";
        return false;
      } else if (command == "Exit") {
        cerr << "Saw message type MockCodeGenerator_Exit." << endl;
        exit(123);
      } else if (command == "Abort") {
        cerr << "Saw message type MockCodeGenerator_Abort." << endl;
        abort();
      } else if (command == "HasSourceCodeInfo") {
        FileDescriptorProto file_descriptor_proto;
        file->CopySourceCodeInfoTo(&file_descriptor_proto);
        bool has_source_code_info =
            file_descriptor_proto.has_source_code_info() &&
            file_descriptor_proto.source_code_info().location_size() > 0;
        cerr << "Saw message type MockCodeGenerator_HasSourceCodeInfo: "
             << has_source_code_info << "." << endl;
        abort();
      } else {
        GOOGLE_LOG(FATAL) << "Unknown MockCodeGenerator command: " << command;
      }
    }
  }

  if (HasPrefixString(parameter, "insert=")) {
    vector<string> insert_into;
    SplitStringUsing(StripPrefixString(parameter, "insert="),
                     ",", &insert_into);

    for (int i = 0; i < insert_into.size(); i++) {
      {
        scoped_ptr<io::ZeroCopyOutputStream> output(context->OpenForInsert(
            GetOutputFileName(insert_into[i], file), kFirstInsertionPointName));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(GetOutputFileContent(name_, "first_insert",
                                              file, context));
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }

      {
        scoped_ptr<io::ZeroCopyOutputStream> output(
            context->OpenForInsert(GetOutputFileName(insert_into[i], file),
                                   kSecondInsertionPointName));
        io::Printer printer(output.get(), '$');
        printer.PrintRaw(GetOutputFileContent(name_, "second_insert",
                                              file, context));
        if (printer.failed()) {
          *error = "MockCodeGenerator detected write error.";
          return false;
        }
      }
    }
  } else {
    scoped_ptr<io::ZeroCopyOutputStream> output(
        context->Open(GetOutputFileName(name_, file)));

    io::Printer printer(output.get(), '$');
    printer.PrintRaw(GetOutputFileContent(name_, parameter,
                                          file, context));
    printer.PrintRaw(kFirstInsertionPoint);
    printer.PrintRaw(kSecondInsertionPoint);

    if (printer.failed()) {
      *error = "MockCodeGenerator detected write error.";
      return false;
    }
  }
#endif // 0
}


}  // namespace compiler
}  // namespace protobuf
}  // namespace google
