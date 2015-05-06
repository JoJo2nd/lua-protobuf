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

string LuaCodeGenerator::GetOutputFileName(const string& generator_name, const FileDescriptor* file) {
    return GetOutputFileName(generator_name, file->name());
}

string LuaCodeGenerator::GetOutputFileName(const string& generator_name, const string& file) {
    return file + generator_name;
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
