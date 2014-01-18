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
