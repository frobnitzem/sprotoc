/*********** Individual declaration lines **********/
#include <vector>
#include <utility>
#include <stdio.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor.h>
#include "generate.h"
#include "helpers.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace ansi_c {

void FileGenerator::fwd_declare_struct(io::Printer *printer, const Descriptor *msg) {
    if(written.count(msg->full_name())) {
        return;
    }
    written[msg->full_name()] = 1;

    printer->Print("typedef struct $full_name$ $full_name$;\n",
            "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        switch(field->cpp_type()) {
        default:
            continue;
        case FieldDescriptor::CPPTYPE_MESSAGE:
            fwd_declare_struct(printer, field->message_type());
            continue;
        case FieldDescriptor::CPPTYPE_ENUM:
            declare_enum(printer, field->enum_type());
            continue;
        }
    }
}

void FileGenerator::declare_enum(io::Printer *printer, const EnumDescriptor *ent) {
    const EnumValueDescriptor *val;
    char num[32];
    if(written.count(ent->full_name())) {
        return;
    }
    written[ent->full_name()] = 1;
    printer->Print("enum $full_name$ {\n",
                    "full_name", DotsToUnderscores(ent->full_name()));
    for(int i = 0; i < ent->value_count(); i++) {
        val = ent->value(i);
        snprintf(num, 32, "%d", val->number());
        printer->Print("    $full_name$ = $num$,\n",
                "full_name", DotsToUnderscores(val->full_name()),
                "num", num);
    }
    printer->Print("};\n");
}

void FileGenerator::declare_struct(io::Printer *printer, const Descriptor *msg) {
    if(written[msg->full_name()] > 1) {
        return;
    }
    written[msg->full_name()] = 2;

    // Scan for sub-messages here and write 'em.
    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor* field = msg->field(i);
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            declare_struct(printer, field->message_type());
        }
    }
    // Print message contents only on post-tree traversal.
    // (ensuring un-nesting)
    printer->Print("struct $full_name$ {\n",
            "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        declare_field(printer, msg->field(i));
    }
    printer->Print("};\n");
}

void declare_field(io::Printer *printer, const FieldDescriptor* field) {
  string repstar = "";
  if (field->is_repeated()) {
      printer->Print("    int n_$name$;\n", "name", field->name());
      repstar = "*";
  } else if (field->is_optional()) {
      printer->Print("    unsigned has_$name$;\n", "name", field->name());
  }
  switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        { const Descriptor *submsg = field->message_type();
        printer->Print("    MY_$type$ $rep$*$name$;\n",
                        "rep", repstar,
                        "type", DotsToUnderscores(submsg->full_name()),
                        "name", field->name());
        }
        return;
      case FieldDescriptor::CPPTYPE_STRING:
        switch (field->options().ctype()) {
          default:  // RepeatedStringFieldGenerator handles unknown ctypes.
          case FieldOptions::STRING:
            printer->Print("    int $rep$len_$name$;\n",
                      "rep", repstar,
                      "name", field->name());
            printer->Print("    char $rep$*$name$;\n",
                      "rep", repstar,
                      "name", field->name());
            return;
        }
      case FieldDescriptor::CPPTYPE_ENUM:
        printer->Print("    enum $type$ $rep$$name$;\n",
                      "rep", repstar,
                      "type", DotsToUnderscores(
                                field->enum_type()->full_name()),
                      "name", field->name());
        return;
      default: // primitive field
        printer->Print("    $type$ $rep$$name$;\n",
                      "rep", repstar,
                      "type", CTName(field->type()),
                      "name", field->name());
        return;
  }
}

// use r_ prefix to ensure no collision with
// internal function vars (which won't have that prefix)
void declare_stackspace(io::Printer *printer, const FieldDescriptor *field) {
    if(field->is_repeated()) {
        printer->Print("    $type$ r_$name$[MAX_REPEATED];\n",
                "type", CFieldType(field), "name", field->name());
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING)
            printer->Print("    int r_len_$name$[MAX_REPEATED];\n",
                           "name", field->name());
    }
}

void set_stackspace(io::Printer *printer, const FieldDescriptor *field) {
    if(field->is_repeated()) {
        printer->Print("    r.$name$ = r_$name$;\n",
                        "name", field->name());
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING)
            printer->Print("    r.len_$name$ = r_len_$name$;\n",
                           "name", field->name());
    }
}

}}}}
