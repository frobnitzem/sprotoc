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
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
    int varint_sz(uint64 i);
    inline bool ascii_isalnum(char c);
    inline bool ascii_isdigit(char c);
    char* FastHexToBuffer(int i, char* buffer);
    string FilenameIdentifier(const string& filename);
    inline bool HasSuffixString(const string& str, const string& suffix);
    inline string StripSuffixString(const string& str, const string& suffix);
    string StripProto(const string& filename);
    string DotsToUnderscores(const string& name);
    inline void LowerString(string * s);
    inline void UpperString(string * s);
    string StringReplace(const string& s, const string& oldsub,
                         const string& newsub, bool replace_all);
    void StringReplace(const string& s, const string& oldsub,
                       const string& newsub, bool replace_all,
                       string* res);
    void SplitStringUsing(const string& full,
                          const char* delim,
                          vector<string>* result);
    void SplitStringAllowEmpty(const string& full, const char* delim,
                               vector<string>* result);
    void JoinStrings(const vector<string>& components,
                     const char* delim,
                     string * result);
    int fixed_size(const FieldDescriptor::Type t);
    string CFieldType(const FieldDescriptor *field);
    string CTName(const FieldDescriptor::Type t);
    string GTName(const FieldDescriptor::Type t);
}}
