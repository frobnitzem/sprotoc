#include <stdlib.h>
#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include "helpers.h"

namespace google {
namespace protobuf {

// Suggested buffer size for FastToBuffer functions.  Also works with
// DoubleToBuffer() and FloatToBuffer().
static const int kFastToBufferSize = 32;
char *FastHexToBuffer(int i, char* buffer) {
  GOOGLE_CHECK(i >= 0) << "FastHexToBuffer() wants non-negative integers, not " << i;

  static const char *hexdigits = "0123456789abcdef";
  char *p = buffer + 21;
  *p-- = '\0';
  do {
    *p-- = hexdigits[i & 15];   // mod by 16
    i >>= 4;                    // divide by 16
  } while (i > 0);
  return p + 1;
}


// ----------------------------------------------------------------------
// ascii_isalnum()
//    Check if an ASCII character is alphanumeric.  We can't use ctype's
//    isalnum() because it is affected by locale.  This function is applied
//    to identifiers in the protocol buffer language, not to natural-language
//    strings, so locale should not be taken into account.
// ascii_isdigit()
//    Like above, but only accepts digits.
// ----------------------------------------------------------------------

inline bool ascii_isalnum(char c) {
  return ('a' <= c && c <= 'z') ||
         ('A' <= c && c <= 'Z') ||
         ('0' <= c && c <= '9');
}

inline bool ascii_isdigit(char c) {
  return ('0' <= c && c <= '9');
}


// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename) {
  string result;
  for (int i = 0; i < filename.size(); i++) {
    if (ascii_isalnum(filename[i])) {
      result.push_back(filename[i]);
    } else {
      // Not alphanumeric.  To avoid any possibility of name conflicts we
      // use the hex code for the character.
      result.push_back('_');
      char buffer[kFastToBufferSize];
      result.append(FastHexToBuffer(static_cast<uint8>(filename[i]), buffer));
    }
  }
  return result;
}

// ----------------------------------------------------------------------
// HasSuffixString()
//    Return true if str ends in suffix.
// StripSuffixString()
//    Given a string and a putative suffix, returns the string minus the
//    suffix string if the suffix matches, otherwise the original
//    string.
// ----------------------------------------------------------------------
inline bool HasSuffixString(const string& str,
                            const string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline string StripSuffixString(const string& str, const string& suffix) {
  if (HasSuffixString(str, suffix)) {
    return str.substr(0, str.size() - suffix.size());
  } else {
    return str;
  }
}

string DotsToUnderscores(const string& name) {
  return StringReplace(name, ".", "_", true);
}

string StripProto(const string& filename) {
  if (HasSuffixString(filename, ".protodevel")) {
    return StripSuffixString(filename, ".protodevel");
  } else {
    return StripSuffixString(filename, ".proto");
  }
}

inline void LowerString(string * s) {
  string::iterator end = s->end();
  for (string::iterator i = s->begin(); i != end; ++i) {
    // tolower() changes based on locale.  We don't want this!
    if ('A' <= *i && *i <= 'Z') *i += 'a' - 'A';
  }
}

inline void UpperString(string * s) {
  string::iterator end = s->end();
  for (string::iterator i = s->begin(); i != end; ++i) {
    // toupper() changes based on locale.  We don't want this!
    if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
  }
}

// ----------------------------------------------------------------------
// StringReplace()
//    Replace the "old" pattern with the "new" pattern in a string,
//    and append the result to "res".  If replace_all is false,
//    it only replaces the first instance of "old."
// ----------------------------------------------------------------------

void StringReplace(const string& s, const string& oldsub,
                   const string& newsub, bool replace_all,
                   string* res) {
  if (oldsub.empty()) {
    res->append(s);  // if empty, append the given string.
    return;
  }

  string::size_type start_pos = 0;
  string::size_type pos;
  do {
    pos = s.find(oldsub, start_pos);
    if (pos == string::npos) {
      break;
    }
    res->append(s, start_pos, pos - start_pos);
    res->append(newsub);
    start_pos = pos + oldsub.size();  // start searching again after the "old"
  } while (replace_all);
  res->append(s, start_pos, s.length() - start_pos);
}

// ----------------------------------------------------------------------
// StringReplace()
//    Give me a string and two patterns "old" and "new", and I replace
//    the first instance of "old" in the string with "new", if it
//    exists.  If "global" is true; call this repeatedly until it
//    fails.  RETURN a new string, regardless of whether the replacement
//    happened or not.
// ----------------------------------------------------------------------

string StringReplace(const string& s, const string& oldsub,
                     const string& newsub, bool replace_all) {
  string ret;
  StringReplace(s, oldsub, newsub, replace_all, &ret);
  return ret;
}

// ----------------------------------------------------------------------
// SplitStringUsing()
//    Split a string using a character delimiter. Append the components
//    to 'result'.
//
// Note: For multi-character delimiters, this routine will split on *ANY* of
// the characters in the string, not the entire string as a single delimiter.
// ----------------------------------------------------------------------
template <typename ITR>
static inline
void SplitStringToIteratorUsing(const string& full,
                                const char* delim,
                                ITR& result) {
  // Optimize the common case where delim is a single character.
  if (delim[0] != '\0' && delim[1] == '\0') {
    char c = delim[0];
    const char* p = full.data();
    const char* end = p + full.size();
    while (p != end) {
      if (*p == c) {
        ++p;
      } else {
        const char* start = p;
        while (++p != end && *p != c);
        *result++ = string(start, p - start);
      }
    }
    return;
  }

  string::size_type begin_index, end_index;
  begin_index = full.find_first_not_of(delim);
  while (begin_index != string::npos) {
    end_index = full.find_first_of(delim, begin_index);
    if (end_index == string::npos) {
      *result++ = full.substr(begin_index);
      return;
    }
    *result++ = full.substr(begin_index, (end_index - begin_index));
    begin_index = full.find_first_not_of(delim, end_index);
  }
}

void SplitStringUsing(const string& full,
                      const char* delim,
                      vector<string>* result) {
  back_insert_iterator< vector<string> > it(*result);
  SplitStringToIteratorUsing(full, delim, it);
}

// Split a string using a character delimiter. Append the components
// to 'result'.  If there are consecutive delimiters, this function
// will return corresponding empty strings. The string is split into
// at most the specified number of pieces greedily. This means that the
// last piece may possibly be split further. To split into as many pieces
// as possible, specify 0 as the number of pieces.
//
// If "full" is the empty string, yields an empty string as the only value.
//
// If "pieces" is negative for some reason, it returns the whole string
// ----------------------------------------------------------------------
template <typename StringType, typename ITR>
static inline
void SplitStringToIteratorAllowEmpty(const StringType& full,
                                     const char* delim,
                                     int pieces,
                                     ITR& result) {
  string::size_type begin_index, end_index;
  begin_index = 0;

  for (int i = 0; (i < pieces-1) || (pieces == 0); i++) {
    end_index = full.find_first_of(delim, begin_index);
    if (end_index == string::npos) {
      *result++ = full.substr(begin_index);
      return;
    }
    *result++ = full.substr(begin_index, (end_index - begin_index));
    begin_index = end_index + 1;
  }
  *result++ = full.substr(begin_index);
}

void SplitStringAllowEmpty(const string& full, const char* delim,
                           vector<string>* result) {
  back_insert_iterator<vector<string> > it(*result);
  SplitStringToIteratorAllowEmpty(full, delim, 0, it);
}

// ----------------------------------------------------------------------
// JoinStrings()
//    This merges a vector of string components with delim inserted
//    as separaters between components.
//
// ----------------------------------------------------------------------
template <class ITERATOR>
static void JoinStringsIterator(const ITERATOR& start,
                                const ITERATOR& end,
                                const char* delim,
                                string* result) {
  GOOGLE_CHECK(result != NULL);
  result->clear();
  int delim_length = strlen(delim);

  // Precompute resulting length so we can reserve() memory in one shot.
  int length = 0;
  for (ITERATOR iter = start; iter != end; ++iter) {
    if (iter != start) {
      length += delim_length;
    }
    length += iter->size();
  }
  result->reserve(length);

  // Now combine everything.
  for (ITERATOR iter = start; iter != end; ++iter) {
    if (iter != start) {
      result->append(delim, delim_length);
    }
    result->append(iter->data(), iter->size());
  }
}

void JoinStrings(const vector<string>& components,
                 const char* delim,
                 string * result) {
  JoinStringsIterator(components.begin(), components.end(), delim, result);
}

int fixed_size(FieldDescriptor::Type t) {
    switch(t) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_FIXED64:
        return 8;
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FIXED32:
        return 4;
    case FieldDescriptor::TYPE_BOOL:
        return 1;
    }
    return 0;
}

string CTName(FieldDescriptor::Type t) {
    switch(t) {
    case FieldDescriptor::TYPE_DOUBLE:
        return "double";
    case FieldDescriptor::TYPE_FLOAT:
        return "float";
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64:
        return "int64_t";
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FIXED64:
        return "uint64_t";
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32:
        return "int32_t";
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_FIXED32:
        return "uint32_t";
    case FieldDescriptor::TYPE_BOOL:
        return "bool";
    case FieldDescriptor::TYPE_STRING:
        return "string";
    case FieldDescriptor::TYPE_BYTES:
        return "bytes";
    case FieldDescriptor::TYPE_ENUM:
        return "enum";
    case FieldDescriptor::TYPE_GROUP:
        return "#error Group not supported";
    case FieldDescriptor::TYPE_MESSAGE:
        return "#error YDNBH";
    }
    return "#error unknown field type";
}

string GTName(FieldDescriptor::Type t) {
    switch(t) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_FIXED64:
        return "fixed64";
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FIXED32:
        return "fixed32";
    case FieldDescriptor::TYPE_INT64:
        return "int64";
    case FieldDescriptor::TYPE_SINT64:
        return "sint64";
    case FieldDescriptor::TYPE_UINT64:
        return "uint64_t";
    case FieldDescriptor::TYPE_INT32:
        return "int32";
    case FieldDescriptor::TYPE_SINT32:
        return "sint32";
    case FieldDescriptor::TYPE_UINT32:
        return "uint32";
    case FieldDescriptor::TYPE_BOOL:
        return "bool";
    case FieldDescriptor::TYPE_STRING:
        return "string";
    case FieldDescriptor::TYPE_BYTES:
        return "bytes";
    case FieldDescriptor::TYPE_ENUM:
        return "uint32";
    case FieldDescriptor::TYPE_GROUP:
        return "#error Group not supported";
    case FieldDescriptor::TYPE_MESSAGE:
        return "#error YDNBH";
    }
    return "#error unknown field type";
}

int varint_sz(uint64 i) {
    if(!(i>>7)) return 1;
    if(!(i>>14)) return 2;
    if(!(i>>21)) return 3;
    if(!(i>>28)) return 4;
    if(!(i>>35)) return 5;
    if(!(i>>42)) return 6;
    if(!(i>>49)) return 7;
    if(!(i>>56)) return 8;
    if(!(i>>63)) return 9;
    return 10;
}

}}
