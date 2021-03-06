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
        switch(field->type()) { //(field->options().ctype()) {
          case FieldDescriptor::TYPE_BYTES:
            printer->Print(
                    "    int $rep$len_$name$;\n"
                    "    void $rep$*$name$;\n"
                    "    void (*write_$name$)(SWriter *print, const void *buf, size_t len);\n"
                    , "rep", repstar, "name", field->name());
            return;
          default:  // RepeatedStringFieldGenerator handles unknown ctypes.
          //case FieldOptions::STRING:
          case FieldDescriptor::TYPE_STRING:
            printer->Print(
                    "    int $rep$len_$name$;\n"
                    "    char $rep$*$name$;\n"
                    , "rep", repstar, "name", field->name());
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
        printer->Print("    r.$name$ = r_$name$;\n", "name", field->name());
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING)
            printer->Print("    r.len_$name$ = r_len_$name$;\n",
                           "name", field->name());
    }
}

}}}}
