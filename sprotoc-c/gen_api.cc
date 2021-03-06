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

/* The source of the source. */
#include <vector>
#include <utility>
#include <map>
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

// API helper functions (hide details of getting sizes before writes)
void declare_top_api(io::Printer *printer, const Descriptor *msg) {
    printer->Print(
        "int $full_name$_to_file(int fd, MY_$full_name$ *a, void *info);\n"
        "uint8_t *$full_name$_to_string(size_t *len, MY_$full_name$ *a, void *info);\n",
        "full_name", DotsToUnderscores(msg->full_name()));
}

void generate_top_api(io::Printer *printer, const Descriptor *msg) {
    printer->Print(
    "int $full_name$_to_file(int fd, MY_$full_name$ *a, void *user_info) {\n"
    "    Allocator *l = allocator_ctor();\n"
    "    struct _fszmap *f;\n"
    "    SWriter s = { .write = _swrite_to_file, .stream = &fd };\n"
    "    int ret;\n"
    "\n"
    "    f = size_$full_name$(l, a, user_info);\n"
    "    ret = write_$full_name$(&s, f, a, user_info);\n"
    "\n"
    "    allocator_dtor(&l);\n"
    "    return ret;\n"
    "}\n\n"
    "uint8_t *$full_name$_to_string(size_t *len, MY_$full_name$ *a, void *user_info) {\n"
    "    Allocator *l = allocator_ctor();\n"
    "    uint8_t *buf, *pos;\n"
    "    struct _fszmap *f;\n"
    "    SWriter s = { .write = _swrite_to_string, .stream = &pos };\n"
    "\n"
    "    f = size_$full_name$(l, a, user_info);\n"
    "    *len = f->len;\n"
    "    if( (buf = malloc(*len)) == NULL) {\n"
    "        DEBUG_MSG(\"Memory allocation error.\\n\");\n"
    "        allocator_dtor(&l);\n"
    "        return NULL;\n"
    "    }\n"
    "    pos = buf;\n"
    "    if(write_$full_name$(&s, f, a, user_info)) {\n"
    "        free(buf);\n"
    "        buf = NULL;\n"
    "    }\n"
    "\n"
    "    allocator_dtor(&l);\n"
    "    return buf;\n"
    "}\n", "full_name", DotsToUnderscores(msg->full_name()));
}

void declare_api(io::Printer *printer, const Descriptor *msg) {
    printer->Print(
      "void protowr_$full_name$($full_name$ *out,\n"
      "                         MY_$full_name$ *a, void *info); // user \n"
      "MY_$full_name$ *protord_$full_name$(void *info,\n"
      "                         $full_name$ *r); // user \n"
      "MY_$full_name$ *read_$full_name$(uint8_t **buf,\n"
      "                         ssize_t sz, void *info);\n\n"
      //"static inline void init_$full_name$($full_name$ *r);\n"
      "struct _fszmap *size_$full_name$(Allocator *l,\n"
      "                         MY_$full_name$ *a, void *info);\n"
      "int write_$full_name$(SWriter *s,\n"
      "                         struct _fszmap *f,\n"
      "                         MY_$full_name$ *a, void *info);\n",
      "full_name", DotsToUnderscores(msg->full_name()));
}

void generate_api(io::Printer *printer, const Descriptor *msg) {
    printer->Print(
    "static inline void init_$full_name$($full_name$ *r) {\n"
    "    // @generated\n",
        "full_name", DotsToUnderscores(msg->full_name()));
    // list static default string values
    for(int i = 0; i < msg->field_count(); i++) {
      const FieldDescriptor* field = msg->field(i);
      if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING
            && field->has_default_value() && !field->is_optional()) {
        printer->Print(
        "    static char def_$name$[] = \"$def$\";\n",
            "name", field->name(),
            "def", field->default_value_string());
      }
    }
    for(int i = 0; i < msg->field_count(); i++) {
        generate_init_call(printer, msg->field(i));
    }
    //"    r.n_child = 0;\n"
    //"    r.has_i = 0; //(etc.)\n"
    printer->Print("}\n");

    printer->Print(
    "// Create a message size tree to associate\n"
    "// with recursively included messages.\n"
    "struct _fszmap *size_$full_name$(Allocator *l, MY_$full_name$ *a, void *user_info) {\n"
    "    $full_name$ r;\n"
    "    struct _fszmap *f = allocate(l, sizeof(struct _fszmap));\n"
    "    struct _fszmap *c;\n"
    "    size_t sz = 0;\n"
    "    // @generated\n", "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        declare_stackspace(printer, msg->field(i));
    }
    for(int i = 0; i < msg->field_count(); i++) {
        set_stackspace(printer, msg->field(i));
    }
    //"   enum $full_name$ *$name$[MAX_REPEATED];\n"
    printer->Print(
    "    f->sub = NULL; // tree for adding sub-messages\n"
    "\n"
    "    init_$full_name$(&r);\n"
    "    protowr_$full_name$(&r, a, user_info); // user's function call\n"
    "\n"
    "    // recursively find sizes of enclosed objects\n"
    "    // @generated\n", "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_size_call(printer, msg->field(i));
    }
    //"    SIZE_PRIM(uint32_t, type, 1);\n"
    //"    SIZE_MSG(Sil_Ast, reqd_ast, 22, 1);\n"
    //"    if(r.has_t2) SIZE_PRIM(uint32_t, t2, 1);\n"
    //"    if(r.has_dir) SIZE_MSG(Sil_ADir, dir, 7, 1);\n"
    //"    SIZE_REP_MSG(Sil_Ast, child, 9, 1);\n"
    printer->Print("\n"
    "    // ONLY SAVE ENCAPSULATED LEN\n"
    "    f->len = sz;\n"
    "    return f;\n"
    "}\n\n");

    printer->Print(
    "int write_$full_name$(SWriter *s, struct _fszmap *f, MY_$full_name$ *a,\n"
    "                           void *user_info) {\n"
    "    $full_name$ r;\n"
    "    struct _fszmap *c;\n"
    "    // @generated\n", "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        declare_stackspace(printer, msg->field(i));
    }
    for(int i = 0; i < msg->field_count(); i++) {
        set_stackspace(printer, msg->field(i));
    }
    //"   enum $full_name$ *$name$[MAX_REPEATED];\n"
    printer->Print("\n"
    "    init_$full_name$(&r);\n"
    "    protowr_$full_name$(&r, a, user_info); // user's function call\n"
    "\n"
    "    // recursively write enclosed objects\n"
    "    // @generated\n", "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_write_call(printer, msg->field(i));
    }
    //"    WRITE_PRIM(enum, , type, 1);\n"
    //"    WRITE_PRIM(float, *(uint32_t *)&, type, 1);\n"
    //"    if(r.has_t2) WRITE_PRIM(sint32_t, , t2, 2);\n"
    //"    if(r.has_name) WRITE_STRING(name, 4);\n"
    //"    if(r.has_dir) WRITE_MSG(Sil_ADir, dir, 7);\n"
    //"    WRITE_REP_MSG(Sil_Ast, child, 10);\n"
    printer->Print("\n"
    "    return 0;\n"
    "err:\n"
    "    DEBUG_MSG(\"Encapsulated message size lookup failed!\\n\");\n"
    "    return 1;\n"
    "}\n");

    printer->Print(
    "MY_$full_name$ *read_$full_name$(uint8_t **buf, ssize_t sz, void *info) {\n"
    "    $full_name$ r; // use the stack to store incoming object\n"
    "    MY_$full_name$ *a = NULL;\n"
    "    unsigned k;\n"
    "    uint32_t tag;\n"
    "    uint64_t n;\n"
    "    // @generated\n",
                    "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        declare_stackspace(printer, msg->field(i));
    }
    for(int i = 0; i < msg->field_count(); i++) {
        set_stackspace(printer, msg->field(i));
    }
    printer->Print(
    "\n"
    "    init_$full_name$(&r);\n"
    "    while(sz > 0) {\n"
    "        k = read_uint32(&tag, *buf, sz);\n"
    "        *buf += k; sz -= k;\n"
    "        switch(tag >> 3) { // Handle known message read types\n"
    "        // @generated\n",
            "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_read_case(printer, msg->field(i));
        printer->Print(
        "            continue;\n");
    }
    printer->Print(
    "        }\n"
    "skip:\n"
    "        k = skip_len(tag, *buf, sz);\n"
    "        *buf += k; sz -= k;\n"
    "    }\n"
    "    if(sz == 0) {\n"
    "        a = protord_$full_name$(info, &r); // user's function call\n"
    "    } else {\n"
    "err:\n"
    "        DEBUG_MSG(\"Read of $full_name$ failed.\\n\");\n"
    "    } // @generated\n",
        "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        const FieldDescriptor *field = msg->field(i);
        if(!field->is_repeated()) continue;
        if(field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
            printer->Print("    if(r.$name$ != r_$name$) { free(r.$name$); "
                                                     "free(r.len_$name$); }\n",
                           "name", field->name());
        } else {
            printer->Print("    if(r.$name$ != r_$name$) free(r.$name$);\n", 
                           "name", field->name());
        }
    }
    printer->Print(
    "    return a;\n"
    "}\n");
}

// stub implementation
void generate_stub(io::Printer *printer, const Descriptor *msg) {
    printer->Print(
        "void protowr_$full_name$($full_name$ *out, MY_$full_name$ *a, void *info) {\n"
        "    int i;\n"
          , "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_copy_in(printer, msg->field(i));
    }
    printer->Print(
        "}\n"
        "MY_$full_name$ *protord_$full_name$(void *info, $full_name$ *r) {\n"
        "    Allocator *l = info;\n"
        "    MY_$full_name$ *out;\n"
        "    size_t len = sizeof(MY_$full_name$);\n"
        "    int i;\n"
        "\n    //count sizes\n",
            "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_struct_len(printer, msg->field(i));
    }
    printer->Print(
        "    if( (out = allocate(l, len)) == NULL) return NULL;\n"
        "\n    // copy, keeping a running tab of used space\n"
        "    len = sizeof(MY_$full_name$);\n",
            "full_name", DotsToUnderscores(msg->full_name()));
    for(int i = 0; i < msg->field_count(); i++) {
        generate_copy_out(printer, msg->field(i));
    }
    printer->Print(
        "\n    return out;\n"
        "}\n");
}

}}}}
