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
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include "generate.h"

using namespace std;

int main(int argc, char* argv[]) {
  google::protobuf::compiler::CommandLineInterface cli;

  cli.SetVersionInfo("C Language - v2.0");

  // Support generation of C++ source and headers.
  google::protobuf::compiler::cpp::CppGenerator cpp_generator;
  cli.RegisterGenerator("--cpp_out", &cpp_generator,
    "Generate C++ source and header.");

  // Support generation of Foo code.
  google::protobuf::compiler::ansi_c::CGenerator c_generator;
  cli.RegisterGenerator("--c_out", &c_generator, "Generate C file.");

  return cli.Run(argc, argv);
}

