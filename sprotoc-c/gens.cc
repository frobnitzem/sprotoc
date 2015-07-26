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

/*********** Individual generation lines **********/
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

void generate_write_call(io::Printer *printer, const FieldDescriptor* field) {
  char line[320];
  string r_type, cast="";

  switch(field->type()) {
  case FieldDescriptor::TYPE_FIXED64:
  case FieldDescriptor::TYPE_SFIXED64:
  case FieldDescriptor::TYPE_DOUBLE:
      cast = "*(uint64_t *)&"; // magic cast
      break;
  case FieldDescriptor::TYPE_FIXED32:
  case FieldDescriptor::TYPE_SFIXED32:
  case FieldDescriptor::TYPE_FLOAT:
      cast = "*(uint32_t *)&";
      break;
  default:
      break;
  }

  printer->Print("    ");
  if(field->is_repeated()) {
      if(field->is_packable() && field->options().packed()) {
          snprintf(line, sizeof(line), "%d", field->number());
          printer->Print("WRITE_REP_PACKED($type$, $cast$,",
                        "type", GTName(field->type()),
                        "cast", cast);
                  
          printer->Print("$name$, $num$);\n",
                        "name", field->name(),
                        "num", line);
          return;
      } else {
          r_type = "WRITE_REP";
      }
  } else {
      if(field->is_optional())
          printer->Print("if(r.has_$name$) ", "name", field->name());

      r_type = "WRITE";
  }

  switch (field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_MESSAGE:
        { const Descriptor *submsg = field->message_type();
        snprintf(line, sizeof(line), "%s_MSG(%s, %s, %d);\n",
                    r_type.c_str(),
                    DotsToUnderscores(submsg->full_name()).c_str(),
                    field->name().c_str(), field->number());
        }
        break;
  case FieldDescriptor::CPPTYPE_STRING:
        if(field->type() == FieldDescriptor::TYPE_BYTES) {
          snprintf(line, sizeof(line), "%s_BYTES(%s, %d);\n",
                    r_type.c_str(),
                    field->name().c_str(), field->number());
        } else {
          snprintf(line, sizeof(line), "%s_STRING(%s, %d);\n",
                    r_type.c_str(),
                    field->name().c_str(), field->number());
        }
        break;
  default: // primitive field
        snprintf(line, sizeof(line), "%s_PRIM(%s, %s, %s, %d);\n",
                    r_type.c_str(),
                    GTName(field->type()).c_str(), cast.c_str(),
                    field->name().c_str(), field->number());
        break;
  }
  printer->Print(line);

  return;
}

//"    r.n_child = 0;\n"
//"    r.has_i = 0; //(etc.)\n"
void generate_init_call(io::Printer *printer, const FieldDescriptor* field) {
  char def[32];

  if(field->is_optional())
      printer->Print("    r->has_$name$ = 0;\n", "name", field->name());
  if(field->is_repeated())
      printer->Print("    r->n_$name$ = 0;\n", "name", field->name());
  if(field->type() == FieldDescriptor::TYPE_BYTES)
      printer->Print("    r->write_$name$ = NULL;\n", "name", field->name());

  if(! field->has_default_value() || field->is_optional()) {
      return;
  }

  switch(field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_DOUBLE:
      snprintf(def, sizeof(def), "%f", field->default_value_double());
      break;
  case FieldDescriptor::CPPTYPE_FLOAT:
      snprintf(def, sizeof(def), "%f", field->default_value_float());
      break;
  case FieldDescriptor::CPPTYPE_INT64:
      snprintf(def, sizeof(def), "%lld", field->default_value_int64());
      break;
  case FieldDescriptor::CPPTYPE_UINT64:
      snprintf(def, sizeof(def), "%llu", field->default_value_uint64());
      break;
  case FieldDescriptor::CPPTYPE_INT32:
      snprintf(def, sizeof(def), "%d", field->default_value_int32());
      break;
  case FieldDescriptor::CPPTYPE_UINT32:
      snprintf(def, sizeof(def), "%u", field->default_value_uint32());
      break;
  case FieldDescriptor::CPPTYPE_BOOL:
      snprintf(def, sizeof(def), "%lld", field->default_value_int64());
      break;
  case FieldDescriptor::CPPTYPE_STRING:
      snprintf(def, sizeof(def), "%lu", field->default_value_string().length());
      printer->Print("    r->$name$ = def_$name$;\n"
                     "    r->len_$name$ = $num$;\n",
                             "name", field->name(), "num", def);
      return;
  case FieldDescriptor::CPPTYPE_ENUM:
      printer->Print("    r->$name$ = $def$;\n", "name", field->name(), "def",
          DotsToUnderscores(field->default_value_enum()->full_name()));
      return;
  case FieldDescriptor::CPPTYPE_MESSAGE:
      printer->Print("#error Cannot initialize default for a message.\n");
      return;
  }
  printer->Print("    r->$name$ = $def$;\n", "name", field->name(), "def", def);

  return;
}

//"        case 1:\n"
//"            if((tag & 7) != 0) goto skip; // check wire type for errors\n"
//"            READ_PRIM(uint32_t, i);\n"
//"            has_i = 1;\n" // track required fields
void generate_read_case(io::Printer *printer, const FieldDescriptor* field) {
  char num[32];
  string read_type;
  snprintf(num, sizeof(num), "%d", field->number());
  printer->Print("        case $num$:\n", "num", num);

  if(field->is_repeated()) {
      if(field->is_packable() && field->options().packed()) {
          printer->Print("            if((tag & 7) != 2) goto skip;\n");
          printer->Print("            READ_REP_PACKED($type$, $name$);\n",
                        "type", GTName(field->type()),
                        "name", field->name());
          goto skip;
      } else {
          read_type = "READ_REP";
      }
  } else {
          read_type = "READ";
  }

  num[1] = 0;
  switch(field->type()) {
  case FieldDescriptor::TYPE_DOUBLE:
  case FieldDescriptor::TYPE_FIXED64:
  case FieldDescriptor::TYPE_SFIXED64:
      num[0] = '1'; break;
  case FieldDescriptor::TYPE_FLOAT:
  case FieldDescriptor::TYPE_FIXED32:
  case FieldDescriptor::TYPE_SFIXED32:
      num[0] = '5'; break;
  case FieldDescriptor::TYPE_INT64:
  case FieldDescriptor::TYPE_UINT64:
  case FieldDescriptor::TYPE_INT32:
  case FieldDescriptor::TYPE_BOOL:
  case FieldDescriptor::TYPE_UINT32:
  case FieldDescriptor::TYPE_ENUM:
  case FieldDescriptor::TYPE_SINT32:
  case FieldDescriptor::TYPE_SINT64:
      num[0] = '0'; break;
  case FieldDescriptor::TYPE_STRING:
  case FieldDescriptor::TYPE_MESSAGE:
  case FieldDescriptor::TYPE_BYTES:
      num[0] = '2'; break;
  case FieldDescriptor::TYPE_GROUP: // deprecated.
      num[0] = '3'; break;
  }
  printer->Print("            if((tag & 7) != $num$) goto skip;\n", "num", num);
  
  switch (field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_MESSAGE:
        { const Descriptor *submsg = field->message_type();
        printer->Print("            $rt$_MSG($type$, $name$);\n",
                        "rt", read_type,
                        "type", DotsToUnderscores(submsg->full_name()),
                        "name", field->name());
        }
        break;
  case FieldDescriptor::CPPTYPE_STRING:
        printer->Print("            $rt$_STRING($name$);\n",
                      "rt", read_type,
                      "name", field->name());
        break;
  //case FieldDescriptor::CPPTYPE_ENUM: // Note: enum range not validated.
  default: // primitive field
        printer->Print("            $rt$_PRIM($type$, $name$);\n",
                      "rt", read_type,
                      "type", GTName(field->type()),
                      "name", field->name());
        break;
  }
  if (field->is_optional()) {
      printer->Print("            r.has_$name$ = 1;\n", "name", field->name());
  } // TODO: add has_... to all!

skip:
  return;
  /*if(field->is_required()) {
      printer->Print("            nreqd++;\n");
  }*/
}

void generate_size_call(io::Printer *printer, const FieldDescriptor* field) {
  //char sz[32];
  char line[320];
  string r_type;

  printer->Print("    ");
  if(field->is_repeated()) {
      if(field->is_packable() && field->options().packed()) {
          snprintf(line, sizeof(line), "%d", varint_sz(field->number() << 3));
          printer->Print("SIZE_REP_PACKED($type$, $name$, $wsz$);\n",
                        "type", GTName(field->type()),
                        "name", field->name(),
                        "wsz", line);
          return;
      } else {
          r_type = "SIZE_REP";
      }
  } else {
      if(field->is_optional())
          printer->Print("if(r.has_$name$) ", "name", field->name());

      r_type = "SIZE";
  }

  switch (field->cpp_type()) {
  case FieldDescriptor::CPPTYPE_MESSAGE:
        { const Descriptor *submsg = field->message_type();
        snprintf(line, sizeof(line), "%s_MSG(%s, %s, %d, %d);\n",
                r_type.c_str(), DotsToUnderscores(submsg->full_name()).c_str(),
                field->name().c_str(),
                field->number(), varint_sz(field->number() << 3));
        printer->Print(line);
        }
        break;
  case FieldDescriptor::CPPTYPE_STRING:
        snprintf(line, sizeof(line), "%d",  varint_sz(field->number() << 3));
        printer->Print("$rt$_STRING($name$, $sz$);\n",
                        "rt", r_type,
                        "name", field->name(),
                        "sz", line);
        break;
  //case FieldDescriptor::CPPTYPE_ENUM: // not validated.
  default: // primitive field
        snprintf(line, sizeof(line), "%s_PRIM(%s, %s, %d);\n",
           r_type.c_str(), GTName(field->type()).c_str(), field->name().c_str(),
           varint_sz(field->number() << 3));
        printer->Print(line);
        break;
  }
  return;
}

// For message copying stubs
//    out->name = a->name;
//    out->len_name = strlen(a->name);
//    out->id = a->id;
//    if(a->email != NULL) {
//        out->email = a->email;
//        out->len_email = strlen(a->email);
//        out->has_email = 1;
//    }
//    out->n_phone = a->phones;
void generate_copy_in(io::Printer *printer, const FieldDescriptor* field) {
  int opt = 0;

  if(field->is_repeated()) { // handle repeated messages here
      printer->Print("    if(a->$name$ != NULL) {\n"
                     "      out->n_$name$ = a->n_$name$;\n"
                     "      out->$name$ = a->$name$;\n"
              , "name", field->name());
      if(field->type() == FieldDescriptor::TYPE_STRING) {
          printer->Print("      out->len_$name$ = a->len_$name$;"
                      " // must have space for storing lengths\n"
                         "      for(i=0; i<a->n_$name$; i++)\n"
                         "        out->len_$name$[i] = strlen(a->$name$[i]);\n"
              , "name", field->name());
      } else if(field->type() == FieldDescriptor::TYPE_BYTES) {
          printer->Print("      out->len_$name$ = a->len_$name$;\n"
                         "      out->write_$name$ = "
                     "(void (*)(SWriter *, void *, size_t))& simple_writer;\n"
              , "name", field->name());
      }
      printer->Print(
                     "    } else {\n"
                     "      out->n_$name$ = 0;\n"
                     "    }\n", "name", field->name());
      return;
  }
  if(field->is_optional()) { // handle specially tested messages
      printer->Print("    out->has_$name$ = a->has_$name$;\n"
                     "    if(a->has_$name$) {\n"
              , "name", field->name());
      printer->Indent(); printer->Indent();
      opt = 1;
  } else if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      printer->Print("    if(a->$name$ != NULL) {\n"
              , "name", field->name());
      printer->Indent(); printer->Indent();
      opt = 2;
  }

  if(field->type() == FieldDescriptor::TYPE_STRING) {
      printer->Print("    out->len_$name$ = strlen(a->$name$);\n"
                      , "name", field->name());
  } else if(field->type() == FieldDescriptor::TYPE_BYTES) {
      printer->Print("    out->len_$name$ = a->len_$name$;\n"
                     "    out->write_$name$ = "
                 "(void (*)(SWriter *, void *, size_t))& simple_writer;\n"
              , "name", field->name());
  }
  printer->Print("    out->$name$ = a->$name$;\n", "name", field->name());

  if(opt) {
      if(opt == 2) {
          printer->Print("} else {\n"
                         "    out->len_$name$ = 0;\n",
                            "name", field->name());
      }
      printer->Outdent(); printer->Outdent();
      printer->Print("    }\n");
  }
}

//len += r->len_name + 1;
//if(r->has_email) len += r->len_email + 1;
void generate_struct_len(io::Printer *printer, const FieldDescriptor* field) {
  int opt = 0;

  if(field->is_repeated()) { // handle repeated messages here
      printer->Print("    len += r->n_$name$*sizeof($type$);\n"
              , "type", CFieldType(field), "name", field->name());
      if(field->type() == FieldDescriptor::TYPE_STRING) {
          printer->Print("    for(i=0; i<r->n_$name$; i++)\n"
                         "        len += r->len_$name$[i]+1;\n"
              , "name", field->name());
      } else {
          printer->Print("    // bytes aren't null-terminated, save sizes!\n"
                         "    len += r->n_$name$*sizeof(size_t);\n"
                         "    for(i=0; i<r->n_$name$; i++)\n"
                         "        len += len_$name$[i];\n"
              , "name", field->name());
      }
      return;
  }
  if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
      printer->Print("    ");
      if(field->is_optional()) { // handle specially tested messages
          printer->Print("if(r->has_$name$) ", "name", field->name());
      }
      if(field->type() == FieldDescriptor::TYPE_STRING) {
          printer->Print("len += r->len_$name$+1;\n", "name", field->name());
      } else {
          printer->Print("len += len_$name$;\n" , "name", field->name());
      }
  }
}

//a->name = (void *)a + len; len += r->len_name+1;
//memcpy(a->name, r->name, r->len_name);
//a->name[r->len_name] = 0;
//if(r->has_email) {
//    a->email = (void *)a + len; len += r->len_email+1;
//    memcpy(a->email, r->email, r->len_email);
//    a->email[r->len_email] = 0;
//} else {
//    a->email = NULL;
//}
void generate_copy_out(io::Printer *printer, const FieldDescriptor* field) {
  int opt = 0;

  if(field->is_repeated()) { // handle repeated messages here
      printer->Print(
         "    out->n_$name$ = r->n_$name$;\n"
         "    out->$name$ = (void *)out + len; len += r->n_$name$*sizeof($type$);\n"
             , "type", CFieldType(field), "name", field->name());
      if(field->type() == FieldDescriptor::TYPE_STRING) {
          printer->Print(
         "    for(i=0; i<r->n_$name$; i++) {\n"
         "        out->$name$[i] = (void *)out + len; len += r->len_$name$[i]+1;\n"
         "        memcpy(out->$name$[i], r->$name$[i], r->len_$name$[i]);\n"
         "        out->$name$[i][r->len_$name$[i]] = 0;\n"
         "    }\n"
             , "name", field->name());
      }
      if(field->type() == FieldDescriptor::TYPE_BYTES) {
          printer->Print(
         "    out->len_$name$ = (void *)out + len; len += r->n_$name$*sizeof(size_t);\n"
         "    for(i=0; i<r->n_$name$; i++) {\n"
         "        out->$name$[i] = (void *)out + len; len += r->len_$name$[i];\n"
         "        out->len_$name$[i] = r->len_$name$[i];\n"
         "        memcpy(out->$name$[i], r->$name$[i], r->len_$name$[i]);\n"
         "    }\n"
             , "name", field->name());
      } else {
          printer->Print(
         "    memcpy(out->$name$, r->$name$, r->n_$name$*sizeof($type$));\n"
             , "type", CFieldType(field), "name", field->name());
      }
      return;
  }
  if(field->is_optional()) { // handle specially tested messages
      printer->Print("    out->has_$name$ = r->has_$name$;\n"
                     "    if(r->has_$name$) {\n"
              , "name", field->name());
      printer->Indent(); printer->Indent();
      opt = 1;
  }

  if(field->type() == FieldDescriptor::TYPE_STRING) {
      printer->Print("    out->$name$ = (void *)out + len; len += r->len_$name$+1;\n"
                     "    memcpy(out->$name$, r->$name$, r->len_$name$);\n"
                     "    out->$name$[r->len_$name$] = 0;\n"
                      , "name", field->name());
  } else if(field->type() == FieldDescriptor::TYPE_BYTES) {
      printer->Print("    out->len_$name$ = r->len_$name$;\n"
                     "    out->$name$ = (void *)out + len; len += r->len_$name$;\n"
                     "    memcpy(out->$name$, r->$name$, r->len_$name$);\n"
                      , "name", field->name());
  } else {
      printer->Print("    out->$name$ = r->$name$;\n", "name", field->name());
  }

  if(opt) {
      printer->Outdent(); printer->Outdent();
      printer->Print("    }\n");
  }
}

}}}}
