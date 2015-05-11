#include <string>
#include <stdlib.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/stubs/strutil.h>
#include "lua_code_generator.h"

#include <iostream>
#include <set>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#else
#include <unistd.h>
#endif

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

using namespace std;

namespace google {
namespace protobuf {
namespace compiler {

	class GeneratorResponseContext : public GeneratorContext {
	public:
		GeneratorResponseContext(CodeGeneratorResponse* response,
			const vector<const FileDescriptor*>& parsed_files)
			: response_(response),
			parsed_files_(parsed_files) {}
		virtual ~GeneratorResponseContext() {}

		// implements GeneratorContext --------------------------------------

		virtual io::ZeroCopyOutputStream* Open(const string& filename) {
			CodeGeneratorResponse::File* file = response_->add_file();
			file->set_name(filename);
			return new io::StringOutputStream(file->mutable_content());
		}

		virtual io::ZeroCopyOutputStream* OpenForInsert(
			const string& filename, const string& insertion_point) {
			CodeGeneratorResponse::File* file = response_->add_file();
			file->set_name(filename);
			file->set_insertion_point(insertion_point);
			return new io::StringOutputStream(file->mutable_content());
		}

		void ListParsedFiles(vector<const FileDescriptor*>* output) {
			*output = parsed_files_;
		}

	private:
		CodeGeneratorResponse* response_;
		const vector<const FileDescriptor*>& parsed_files_;
	};
}
}
}

int main(int argc, char* argv[]) {
    static bool wait_for_debugger = false;
    while (wait_for_debugger) {}

#ifdef _MSC_VER
	// Don't print a silly message or stick a modal dialog box in my face,
	// please.
	_set_abort_behavior(0, ~0);
#endif  // !_MSC_VER

	google::protobuf::compiler::LuaCodeGenerator generator;

	if (argc > 1) {
		cerr << argv[0] << ": Unknown option: " << argv[1] << endl;
		return 1;
	}

#ifdef _WIN32
	_setmode(STDIN_FILENO, _O_BINARY);
	_setmode(STDOUT_FILENO, _O_BINARY);
#endif

	google::protobuf::compiler::CodeGeneratorRequest request;
	if (!request.ParseFromFileDescriptor(STDIN_FILENO)) {
		cerr << argv[0] << ": protoc sent unparseable request to plugin." << endl;
		return 1;
	}

	google::protobuf::DescriptorPool pool;
	for (int i = 0; i < request.proto_file_size(); i++) {
		const google::protobuf::FileDescriptor* file = pool.BuildFile(request.proto_file(i));
		if (file == NULL) {
			// BuildFile() already wrote an error message.
			return 1;
		}
	}

	vector<const google::protobuf::FileDescriptor*> parsed_files;
	for (int i = 0; i < request.file_to_generate_size(); i++) {
		parsed_files.push_back(pool.FindFileByName(request.file_to_generate(i)));
		if (parsed_files.back() == NULL) {
			cerr << argv[0] << ": protoc asked plugin to generate a file but "
				"did not provide a descriptor for the file: "
				<< request.file_to_generate(i) << endl;
			return 1;
		}
	}

	google::protobuf::compiler::CodeGeneratorResponse response;
	google::protobuf::compiler::GeneratorResponseContext context(&response, parsed_files);

	{
		string error;
		generator.GenerateCommonFiles(request.parameter(), &context, &error);
	}

	for (int i = 0; i < parsed_files.size(); i++) {
		const google::protobuf::FileDescriptor* file = parsed_files[i];

		string error;
		bool succeeded = generator.Generate(
			file, request.parameter(), &context, &error);

		if (!succeeded && error.empty()) {
			error = "Code generator returned false but provided no error "
				"description.";
		}
		if (!error.empty()) {
			response.set_error(file->name() + ": " + error);
			break;
		}
	}

	if (!response.SerializeToFileDescriptor(STDOUT_FILENO)) {
		cerr << argv[0] << ": Error writing to stdout." << endl;
		return 1;
	}

	return 0;
}