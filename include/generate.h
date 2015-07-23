// Protocol Buffers - Google's data interchange format
// Copyright 2014 David M. Rogers
// Copyright 2008 Google Inc.  All rights reserved.
//   modified form of: http://code.google.com/p/protobuf/
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

// Author: David M. Rogers (predictivestatmech@gmail.com)
//  Based on C++-generator by
//  Kenton Varda, kenton@google.com
//    Based on original Protocol Buffers design by
//    Sanjay Ghemawat, Jeff Dean, and others.
//
// Generates Stack-based C code for a given .proto file.


#ifndef SPROTOC_C_GEN_API_H__
#define SPROTOC_C_GEN_API_H__

#include <string>
#include <vector>
#include <map>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>

namespace google {
namespace protobuf {
  class FileDescriptor;        // descriptor.h
  /*namespace io {
    class Printer;             // printer.h
  }*/

namespace compiler {
namespace ansi_c {

class LIBPROTOC_EXPORT CGenerator : public CodeGenerator {
 public:
  CGenerator();
  ~CGenerator();

  // implements CodeGenerator ----------------------------------------
  bool Generate(const FileDescriptor* file,
                const string& parameter,
                GeneratorContext* generator_context,
                string* error) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CGenerator);
};

typedef struct {
    bool gen_stubs;
    string stub_prefix;
} Options;

class FileGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit FileGenerator(const FileDescriptor* file,
                         const Options& options);
  ~FileGenerator();

  void GenerateHeader(io::Printer* printer, string incname);
  void GenerateSource(io::Printer* printer);
  void GenerateStubHeader(io::Printer* printer, string basename);
  void GenerateStubSource(io::Printer* printer, string basename);

  void decl_api(io::Printer *printer, const Descriptor *msg);
  void gen_api(io::Printer *printer, const Descriptor *msg);
  void gen_stub_hdr(io::Printer *printer, const Descriptor *msg);
  void gen_stub(io::Printer *printer, const Descriptor *msg);

  /* declares.cc */
  void fwd_declare_struct(io::Printer *printer, const Descriptor *msg);
  void declare_enum(io::Printer *printer, const EnumDescriptor *ent);
  void declare_struct(io::Printer *printer, const Descriptor *msg);

 private:
  const FileDescriptor* file_;

  // E.g. if the package is foo.bar, package_parts_ is {"foo", "bar"}.
  vector<string> package_parts_;

  const Options options_;
  map<string,int> written;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileGenerator);
};

/* gen_api.cc */
void declare_top_api(io::Printer *printer, const Descriptor *msg);
void generate_top_api(io::Printer *printer, const Descriptor *msg);

void declare_api(io::Printer *printer, const Descriptor *msg);
void generate_api(io::Printer *printer, const Descriptor *msg);
void generate_stub(io::Printer *printer, const Descriptor *msg);

/* declares.cc */
void declare_field(io::Printer *printer, const FieldDescriptor* field);

void declare_stackspace(io::Printer *printer, const FieldDescriptor *field);
void set_stackspace(io::Printer *printer, const FieldDescriptor *field);

/* gens.cc */
void generate_init_call(io::Printer *printer, const FieldDescriptor* field);
void generate_write_call(io::Printer *printer, const FieldDescriptor* field);
void generate_read_case(io::Printer *printer, const FieldDescriptor* field);
void generate_size_call(io::Printer *printer, const FieldDescriptor* field);
void generate_copy_in(io::Printer *printer, const FieldDescriptor* field);
void generate_struct_len(io::Printer *printer, const FieldDescriptor* field);
void generate_copy_out(io::Printer *printer, const FieldDescriptor* field);

}  // namespace ansi_c
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_C_GENERATOR_H__


