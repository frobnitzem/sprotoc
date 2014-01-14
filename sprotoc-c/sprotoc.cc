#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include "generate.h"

using namespace std;

int main(int argc, char* argv[]) {
  google::protobuf::compiler::CommandLineInterface cli;

  cli.SetVersionInfo("C Language - v0.1");

  // Support generation of C++ source and headers.
  google::protobuf::compiler::cpp::CppGenerator cpp_generator;
  cli.RegisterGenerator("--cpp_out", &cpp_generator,
    "Generate C++ source and header.");

  // Support generation of Foo code.
  google::protobuf::compiler::ansi_c::CGenerator c_generator;
  cli.RegisterGenerator("--c_out", &c_generator, "Generate C file.");

  return cli.Run(argc, argv);
}

