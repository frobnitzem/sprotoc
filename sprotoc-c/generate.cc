/*    Copyright (C) David M. Rogers, 2015
 *    
 *    David M. Rogers <predictivestatmech@gmail.com>
 *    Nonequilibrium Stat. Mech. Research Group
 *    Department of Chemistry
 *    University of South Florida
 *
 *    This file is part of sprotoc.
 *
 *    sprotoc is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    sprotoc is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with sprotoc.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <google/protobuf/compiler/code_generator.h>

#include <vector>
#include <utility>
#include <map>
#include <stdio.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include "generate.h"
#include "helpers.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace ansi_c {

CGenerator::CGenerator() {}
CGenerator::~CGenerator() {}


const char kThickSeparator[] =
      "// ===================================================================\n";
const char kThinSeparator[] =
      "// -------------------------------------------------------------------\n";


bool CGenerator::Generate(const FileDescriptor* file,
                            const string& parameter,
                            GeneratorContext* generator_context,
                            string* error) const {
  vector<pair<string, string> > options;
  ParseGeneratorParameter(parameter, &options);
  Options file_options;
  file_options.gen_stubs = false;
  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "stubs") {
      file_options.stub_prefix = options[i].second;
      file_options.gen_stubs = true;
    } else {
      *error = "Unknown generator option: " + options[i].first;
      return false;
    }
  }

  string basename = StripProto(file->name());
  FileGenerator file_generator(file, file_options);

  // Generate header.
  {
    string incname = basename;
    if(file_options.gen_stubs)
        incname = file_options.stub_prefix;
    scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(basename + ".pb-c.h"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateHeader(&printer, incname);
  }

  // Generate cc file.
  {
    scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(basename + ".pb-c.c"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateSource(&printer);
  }

  // Generate stubs
  if(file_options.gen_stubs) {
    {
    scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(file_options.stub_prefix + ".h"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateStubHeader(&printer, basename);
    /**/}{/**/
    scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(file_options.stub_prefix + ".c"));
    io::Printer printer(output.get(), '$');
    file_generator.GenerateStubSource(&printer, basename);
    }
  }

  return true;
}

FileGenerator::FileGenerator(const FileDescriptor* file,
                             const Options& options)
  : file_(file), options_(options) {
  //map<string, int> written;

  SplitStringUsing(file_->package(), ".", &package_parts_);
}

FileGenerator::~FileGenerator() {}

void FileGenerator::GenerateHeader(io::Printer* printer, string incname) {
  string filename_identifier = FilenameIdentifier(file_->name());

  // Generate top of header.
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "// source: $filename$\n\n"
    "#ifndef PROTOBUF_$filename_identifier$__INCLUDED\n"
    "#define PROTOBUF_$filename_identifier$__INCLUDED\n\n",
    "filename", file_->name(),
    "filename_identifier", filename_identifier);

  printer->Print(
    "#include <stdio.h>\n"
    "#include <stdint.h>\n"
    "#include <stdint.h>\n"
    "#include \"sprotoc/sprotoc.h\"\n"
    "#include \"$incname$.h\"\n", 
        "incname", incname);
  for (int i = 0; i < file_->dependency_count(); i++) {
    printer->Print(
      "#include \"$dependency$.pb-c.h\"\n",
      "dependency", StripProto(file_->dependency(i)->name()));
  }

  printer->Print(
    "\n// Maximum number of repetitions of a single field.\n"
    "#define MAX_REPEATED 32\n"
    "// For debugging imperfections in read/written messages.\n"
    "#define DEBUG_MSG2(A,B,...) fprintf(stderr, #A \":\" "
                                            "#B \" \" __VA_ARGS__)\n"
    "#define DEBUG_MSG(...) DEBUG_MSG2(__FILE__, __LINE__, __VA_ARGS__)\n"
    "// @@protoc_insertion_point(includes)\n\n");

  // Generate top-level enum definitions.
  for (int i = 0; i < file_->enum_type_count(); i++) {
    declare_enum(printer, file_->enum_type(i));
  }
  // Generate forward declarations of structs and scoped enums
  for (int i = 0; i < file_->message_type_count(); i++) {
    fwd_declare_struct(printer, file_->message_type(i));
  }
  printer->Print("\n");
  printer->Print(kThinSeparator);
  printer->Print("\n");

  // Generate actual struct declarations
  for (int i = 0; i < file_->message_type_count(); i++) {
    declare_struct(printer, file_->message_type(i));
  }
  printer->Print("\n");
  printer->Print(kThickSeparator);
  printer->Print("\n");

  // Generate API definitions.
  for (int i = 0; i < file_->message_type_count(); i++) {
    if(i > 0) {
        printer->Print("\n");
        printer->Print(kThinSeparator);
        printer->Print("\n");
    }
    declare_top_api(printer, file_->message_type(i));
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    decl_api(printer, file_->message_type(i));
  }

  if (file_->service_count() > 0) {
    printer->Print(
      "#warning Generic Services Not Implemented\n");
  }

  printer->Print(
    "\n#endif  // PROTOBUF_$filename_identifier$__INCLUDED\n",
    "filename_identifier", filename_identifier);
}

void FileGenerator::GenerateSource(io::Printer* printer) {
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "// source: $filename$\n\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "#include \"$basename$.pb-c.h\"\n\n"
    "extern const rbop_t fszops;\n\n",
        "filename", file_->name(),
        "basename", StripProto(file_->name()));

    for (int i = 0; i < file_->message_type_count(); i++) {
        if(i > 0) {
            printer->Print("\n");
            printer->Print(kThinSeparator);
            printer->Print("\n");
        }
        generate_top_api(printer, file_->message_type(i));
    }
    for (int i = 0; i < file_->message_type_count(); i++) {
        gen_api(printer, file_->message_type(i));
    }
    if (file_->service_count() > 0) {
        printer->Print("#warning Generic Services Not Implemented\n");
    }

    /* Define extensions.
    for (int i = 0; i < file_->extension_count(); i++) {
        extension_generators_[i]->GenerateDefinition(printer);
    }*/
}

void FileGenerator::GenerateStubHeader(io::Printer* printer, string basename) {
  printer->Print(
    "// Example header for reading and writing from/to protobufs - do edit!\n"
    "// source: $filename$\n\n"
    "#include <stdint.h>\n"
    "\n"
    "// This is included at the top of $basename$.pb-c.h,\n"
    "//   and can re-define all MY_... symbols.\n"
    "// -- Here we define them to use the auto-generated data structure.\n",
        "filename", file_->name(),
        "basename", basename);
    for(int i = 0; i < file_->message_type_count(); i++) {
        gen_stub_hdr(printer, file_->message_type(i));
    }
}

void FileGenerator::GenerateStubSource(io::Printer* printer, string basename) {
  printer->Print(
    "// Example code for reading and writing from/to protobufs - do edit!\n"
    "// source: $filename$\n\n"
    "#include <stdio.h>\n"
    "#include <string.h>\n"
    "#include <stdlib.h>\n"
    "#include <stdint.h>\n"
    "\n"
    "#include \"$basename$.pb-c.h\"\n\n",
    "filename", file_->name(),
    "basename", basename);

  printer->Print(
    "int main(int argc, char **argv) {\n"
    "    Allocator *l = allocator_ctor();\n"
    "    $full_name$ test = {}; // TODO: initialize something useful\n"
    "    $full_name$ *ret;\n"
    "    size_t len;\n"
    "    uint8_t *buf, *pos;\n"
    "\n"
    "    if( (buf = $full_name$_to_string(&len, &test)) == NULL) {\n"
    "        fprintf(stderr, \"Error writing test data.\\n\");\n"
    "        return 1;\n"
    "    }\n"
    "    pos = buf; // running parser position\n"
    "    ret = read_$full_name$(&pos, len, l);\n"
    "    free(buf);\n"
    "    if(ret == NULL) {\n"
    "        return 1;\n"
    "    }\n"
    "\n"
    "    printf(\"Read test data to %p\\n\", ret); // TODO: show some result fields\n"
    "\n"
    "    allocator_dtor(&l);\n"
    "    return 0;\n"
    "}\n", "full_name",
      DotsToUnderscores(file_->message_type(0)->full_name()));

  for (int i = 0; i < file_->message_type_count(); i++) {
      gen_stub(printer, file_->message_type(i));
  }
  if (file_->service_count() > 0) {
    printer->Print(
      "#warning Generic Services Not Implemented\n");
  }
}

// numbers 1-2 are taken by fwd_declare_struct/declare_enum and declare_struct
void FileGenerator::decl_api(io::Printer *printer, const Descriptor *msg) {
    if(written[msg->full_name()] > 2) {
        return;
    }
    written[msg->full_name()] = 3;

    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            decl_api(printer, field->message_type());
        }
    }

    printer->Print("\n");
    printer->Print(kThickSeparator);
    printer->Print("\n");

    declare_api(printer, msg);
}

void FileGenerator::gen_api(io::Printer *printer, const Descriptor *msg) {
    if(written[msg->full_name()] > 3) {
        //printf("Skipping generation of %s\n", msg->full_name().c_str());
        return;
    }
    written[msg->full_name()] = 4;
    //printf("Generating %s\n", msg->full_name().c_str());

    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            gen_api(printer, field->message_type());
        }
    }

    printer->Print("\n");
    printer->Print(kThickSeparator);
    printer->Print("\n");

    generate_api(printer, msg);
}

void FileGenerator::gen_stub_hdr(io::Printer* printer, const Descriptor *msg) {
    if(written[msg->full_name()] > 4) {
        return;
    }
    written[msg->full_name()] = 5;

    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            gen_stub_hdr(printer, field->message_type());
        }
    }
    printer->Print(
        "#define MY_$full_name$ $full_name$\n",
        "full_name", DotsToUnderscores(msg->full_name()));
}

void FileGenerator::gen_stub(io::Printer *printer, const Descriptor *msg) {
    if(written[msg->full_name()] > 5) {
        return;
    }
    written[msg->full_name()] = 6;

    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            gen_stub(printer, field->message_type());
        }
    }
    printer->Print("\n");
    printer->Print(kThinSeparator);
    printer->Print("\n");

    generate_stub(printer, msg);
}

} // ansi_c
} // compiler
} // protobuf
} // g*gle

