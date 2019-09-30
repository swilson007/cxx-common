////////////////////////////////////////////////////////////////////////////////
/// Copyright 2019 Steven C. Wilson
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
/// and associated documentation files (the "Software"), to deal in the Software without
/// restriction, including without limitation the rights to use, copy, modify, merge, publish,
/// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
/// Software is furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in all copies or
/// substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
/// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
/// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "assert.h"
#include "defines.h"
#include "strings.h"
#include "types.h"

#include <cctype>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

// Debug
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
/// This contains the PosixPath class -  a path class similar to boost or std::filesystem
/// in concept, but it will treat all paths internally as posix style with utf8 encoding.
/// It will support native translation for Windows only on request. The purpose is that
/// it's easier to use this for cross-platform code than std::fs::path.
////////////////////////////////////////////////////////////////////////////////

namespace sw {

/// Using an #ifdef to determine the type of normalization used by the PosixPath
/// normalize(), normalized(), and absonormed related calls.
/// SCW: Was going to do this via template approach, but it opened up a can of
/// worms that I don't have time for. Most likely, I'll want one form or the other
/// for the application as a whole
#if SW_POSIX_PATH_USE_FULL_NORMALIZATION
static constexpr bool kPosixPathUseFullNormalization = true;
#else
static constexpr bool kPosixPathUseFullNormalization = false;
#endif

/// Private helpers
namespace path_detail {

class PathSegmentIterator;

template <bool kIsConst = false>
class PathIterator;

static constexpr const char kPathSep = '/';
static constexpr const char* kEmptyString = "";
static constexpr const char* kDotString = ".";
static constexpr const char* kDotDotString = "..";
static constexpr const char* kSepString = "/";
static constexpr const char* kDoubleSepString = "//";
static constexpr const auto kNoPos = std::string::npos;
static constexpr const char kSep = '/';
static constexpr const char kWin32Sep = '\\';
static constexpr const char kDot = '.';
static constexpr const char kNullTerm = '\0';

// In case we need this to be something other than :. So... : is a valid posix-path character,
// but the concern is ':' is usually used as a path splitter for POSIX/UNIX systems . Like
// "PATH=/usr/bin:/usr/local/bin/:/sbin" But, I'm also not anticipating a Windows drive-rooted path
// ever escaping and being used in that scenario. But, we could always switch the ':' if it turns
// out to be the wrong choice. Possible good choices: [!#^&%=]  Bad choices would be those used for network
// naming, so [.-]
static constexpr auto kDriveChar = ':';

/// Drive root scheme: //d:/  Thus the root is at 4
static constexpr sizex kDriveRootPos = 4;

bool isDriveRoot(const char* str, sizex endPos);
bool isNetworkRoot(const char* str, sizex endPos);
bool hasRootName(const char* str, sizex endPos);
sizex findExtensionPos(const char* str, sizex endPos);
sizex findNextSep(const char* str, sizex endPos, sizex startPos);
sizex findPrevSep(const char* str, sizex endPos);
sizex findNetworkRootSep(const char* str, sizex endPos);
bool isRootSeparator(const char* str, sizex endPos, sizex pos);
sizex findRootDirPos(const char* str, sizex endPos);
std::tuple<sizex, sizex> findFilenamePos(const char* str, sizex endPos);
}  // namespace path_detail

////////////////////////////////////////////////////////////////////////////////
/// This will be a path class similar to boost or std::filesystem in concept, but
/// it will treat all paths internally as posix style with utf8 encoding. It will
/// support native translation for Windows only on request.
///
/// For windows, drives are represented as rooted letter dirs followed by a colon and
/// using the posix network approach (double slash). So `C:\foo` would be represented
/// as `//c:/foo`.
///
/// This supports the "Root Name" concept of the std::filesystem, which is essentially
/// a windows drive. Will work with paths of the form '/<lower-drive-letter>/<path>'.
/// So, "/c/foo/bar" will have a root name of "/c".  "/foo/bar/baz" will have root name
/// of "".
///
/// Network style directories are also supported. The form is: '//<hostname>/path...'. In
/// this case, the Root Name is '//<hostname>'.
///
/// The API will stay as close to std::filesystem::path as possible.
class PosixPath {
public:
  // Setup the native OS string type
#if SW_POSIX
  using native_string_type = std::string;
#elif SW_WINDOWS
  using native_string_type = std::wstring;
#else
#  error "Impliment for this OS"
#endif

  using string_type = std::string;
  using value_type = char;
  using iterator = path_detail::PathIterator<false>;
  using const_iterator = path_detail::PathIterator<true>;
  static constexpr value_type kSep = '/';
  static constexpr value_type kDot = '.';
  static constexpr auto kNoPos = std::string::npos;

private:
  // clang-format off
  template <typename StringClass> StringClass doFilename() const;
  template <typename StringClass> StringClass doExtension() const;
  template <typename StringClass> StringClass doStem() const;
  template <typename StringClass> StringClass doParentPath() const;
  template <typename StringClass> StringClass doRootName() const;
  template <typename StringClass> StringClass doRootDir() const;
  template <typename StringClass> StringClass doRootPath() const;
  template <typename StringClass> StringClass doRelativePath() const;
  // clang-format on

public:
  ~PosixPath() = default;

  PosixPath() : _normalized(true), _absolute(false) {}
  PosixPath(const StringView& sv) : _pstr(sv.data(), sv.size()) {}
  PosixPath(const StringWrapper& sv) : _pstr(sv.data(), sv.size()) {}
  PosixPath(const std::string& str) : _pstr(str) {}
  PosixPath(std::string&& str) : _pstr(std::move(str)) {}

  template <class Source>
  PosixPath(const Source& source) : _pstr(source) {}

  PosixPath(const PosixPath&) = default;
  PosixPath& operator=(const PosixPath&) = default;

  PosixPath(PosixPath&& that) :
      _pstr(std::move(that._pstr)),
      _normalized(std::exchange(that._normalized, false)),
      _absolute(std::exchange(that._absolute, false)) {}
  PosixPath& operator=(PosixPath&& that) {
    _pstr = std::move(that._pstr);
    _normalized = std::exchange(that._normalized, false);
    _absolute = std::exchange(that._absolute, false);
    return *this;
  }

  iterator begin();
  const_iterator cbegin() const;
  iterator end();
  const_iterator cend() const;

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath& normalize() {
    if (!is_normalized()) {
      auto normed = normalized();
      *this = std::move(normed);
    }
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Make this path weakly-canonical aka absonorm.
  PosixPath& absonormize(const PosixPath& cwd) {
    if (!is_absonorm()) {
      auto cpathd = absonormed(cwd);
      *this = std::move(cpathd);
    }
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the weakly-canonical aka absonormed version of the given path.
  PosixPath absonormed(const PosixPath& cwd) const {
    if (is_absonorm()) {
      return *this;
    }

    return doMakeCanonical(*this, cwd);
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the normalized version of the given path. Normalized has no fluff
  PosixPath normalized() const {
    if (is_normalized()) {
      return *this;
    }

    return doMakeNormalized(*this);
  }

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath& operator+=(const PosixPath& p) { return doConcat(p._pstr.data(), p._pstr.size()); }
  PosixPath& operator+=(const PosixPath::string_type& str) { return doConcat(str.data(), str.size()); }
  PosixPath& operator+=(const StringWrapper& sw) { return doConcat(sw.data(), sw.size()); }
  PosixPath& operator+=(const StringView& sv) { return doConcat(sv.data(), sv.size()); }
  PosixPath& operator+=(const value_type* ptr) {
    _pstr += ptr;
    _normalized = false;
    return *this;
  }
  PosixPath& operator+=(value_type x) {
    _pstr += x;
    _normalized = false;
    return *this;
  }

  template <class Source>
  PosixPath& operator+=(const Source& source) {
    _pstr += source;
    _normalized = false;
    return *this;
  }
  template <class CharT>
  PosixPath& operator+=(CharT x) {
    _pstr += x;
    _normalized = false;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Shortens the path by the given number of characters. This behaves as a dumb
  /// string operation. If the amount is larger then the path length, the path will
  /// end up as "".
  PosixPath& shorten(sizex amount) {
    auto curSize = _pstr.size();
    if (curSize > amount) {
      _pstr.resize(curSize - amount);
    } else {
      _pstr.resize(0);
    }

    // Unfortunately we can't be sure of absolute because of root-names. Not without checking anyway.
    // This shouldn't affect normalized state
    if (_absolute) {
      _absolute = false;
    }

    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Concatenates the path string onto ours. This is a simple string
  /// concatenation - beware.
  template <class Source>
  PosixPath& concat(const Source& source) {
    _pstr.append(source);
    _normalized = false;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Concatenates the path string onto ours. This is a simple string
  /// concatenation - beware.
  template <class InputIt>
  PosixPath& concat(InputIt first, InputIt last) {
    _pstr.append(first, last);
    _normalized = false;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Append the given path to this path using a separator
  PosixPath& operator/=(const PosixPath& p) { return doAppend(p); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Append the given path to this path using a separator
  template <class Source>
  PosixPath& operator/=(const Source& source) {
    return doAppend(source);
  }

  ////////////////////////////////////////////////////////////////////////////////
  bool is_absolute() const { return _absolute || doIsAbsolute(); }

  ////////////////////////////////////////////////////////////////////////////////
  bool is_absolute() {
    if (!_absolute) {
      _absolute = doIsAbsolute();
    }
    return _absolute;
  }

  ////////////////////////////////////////////////////////////////////////////////
  bool is_relative() const { return !_absolute && doIsRelative(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Speculative - may produce false negative
  bool is_normalized() const noexcept { return _normalized; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Call when it's known that the path is normalized.
  PosixPath& forceNormalized() noexcept {
    if (!_normalized)
      _normalized = true;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Call when it's known that the path is absolute.
  PosixPath& forceAbsolute() noexcept {
    if (!_absolute)
      _absolute = true;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Speculative - may produce false negative
  bool is_absonorm() const noexcept { return _normalized && _absolute; }

  ////////////////////////////////////////////////////////////////////////////////
  void clear() noexcept {
    _pstr.clear();
    _normalized = false;
    _absolute = false;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the filename. If the path end with a "/", then the filename
  /// is considered to be ".".
  PosixPath root_name() const { return doRootName<std::string>(); }
  StringView root_name_view() const { return doRootName<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns either a '/' if it's an absolute path, or '' if it's relative.
  PosixPath root_directory() const { return doRootDir<std::string>(); }
  StringView root_directory_view() const { return doRootDir<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns root-name / root-dir, or "/" if no root-name, or "" if it's relative
  PosixPath root_path() const { return doRootPath<std::string>(); }
  StringView root_path_view() const { return doRootPath<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns root-name / root-dir, or "/" if no root-name, or "" if it's relative
  PosixPath relative_path() const { return doRelativePath<std::string>(); }
  StringView relative_path_view() const { return doRelativePath<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the filename. If the path end with a "/", then the filename
  /// is considered to be ".".
  PosixPath filename() const { return doFilename<std::string>(); }
  StringView filename_view() const { return doFilename<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the parent-path.
  PosixPath parent_path() const { return PosixPath{doParentPath<std::string>(), _normalized, _absolute}; }
  StringView parent_path_view() const { return doParentPath<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the extension
  PosixPath extension() const { return doExtension<std::string>(); }
  StringView extension_view() const { return doExtension<StringView>(); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Returns the stem
  PosixPath stem() const { return doStem<std::string>(); }
  StringView stem_view() const { return doStem<StringView>(); }

  bool has_root_path() const { return !root_path_view().empty(); }
  bool has_root_name() const { return !root_name_view().empty(); }
  bool has_root_directory() const { return !root_directory_view().empty(); }
  bool has_relative_path() const { return !relative_path_view().empty(); }
  bool has_parent_path() const { return !parent_path_view().empty(); }
  bool has_filename() const { return !filename_view().empty(); }
  bool has_stem() const { return !stem_view().empty(); }
  bool has_extension() const { return !extension_view().empty(); }
  bool empty() const noexcept { return _pstr.empty(); }

  int compare(const PosixPath& p) const noexcept { return _pstr.compare(p._pstr); }
  int compare(const string_type& str) const { return _pstr.compare(str); }
  int compare(const StringWrapper& str) const { return _pstr.compare(str.c_str()); }
  int compare(const value_type* s) const { return _pstr.compare(s); }

  const char* c_str() const noexcept { return _pstr.c_str(); }
  const std::string& u8string() const { return _pstr; }
  const std::string& u8() const { return _pstr; }
  operator string_type() const { return _pstr; }

  native_string_type native() const;

  ////////////////////////////////////////////////////////////////////////////////
  void swap(PosixPath& that) noexcept {
    _pstr.swap(that._pstr);
    std::swap(_absolute, that._absolute);
    std::swap(_normalized, that._normalized);
  }

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath lexically_normal() const;

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath lexically_full_normal() const;

#if 0
  PosixPath lexically_relative(const PosixPath& base) const {
    return PosixPath(_path.lexically_relative(base._path), false, false);
  }
  PosixPath lexically_proximate(const PosixPath& base) const {
    return PosixPath(_path.lexically_proximate(base._path), false, false);
  }

#endif

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath& remove_filename() {
    const auto fpos = std::get<0>(path_detail::findFilenamePos(_pstr.data(), _pstr.size()));

    // If there's no filename, we do nothing. If there is a filename, then just chop it off
    if (_pstr.data()[fpos] != kSep) {
      _pstr.resize(fpos);
    }

    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath& replace_filename(const PosixPath& replacement) {
    remove_filename();
    return doAppend(replacement);
  }

  ////////////////////////////////////////////////////////////////////////////////
  PosixPath& replace_extension(const PosixPath& replacement = PosixPath()) {
    const auto* str = _pstr.data();
    const auto size = _pstr.size();
    const bool replaceHasDot = !replacement.empty() && (replacement._pstr[0] == kDot);
    const auto extPos = path_detail::findExtensionPos(str, size);
    if (extPos != kNoPos) {
      // We have an extension to replace.
      if (replaceHasDot || replacement.empty()) {
        _pstr.resize(extPos);  // Chop our dot
      } else {
        _pstr.resize(extPos + 1);  // Use our dot
      }
    }

    // And concatenate the replacement
    return doConcat(replacement._pstr.data(), replacement._pstr.size());
  }

private:
  PosixPath doDefaultLexicalNormalization() const {
    if (kPosixPathUseFullNormalization) {
      return lexically_full_normal();
    } else {
      return lexically_normal();
    }
  }

  PosixPath(const StringView& sv, bool norm, bool abs) :
      _pstr(sv.data(), sv.size()),
      _normalized(norm),
      _absolute(abs) {}
  PosixPath(const StringWrapper& sv, bool norm, bool abs) :
      _pstr(sv.data(), sv.size()),
      _normalized(norm),
      _absolute(abs) {}
  PosixPath(const std::string& str, bool norm, bool abs) : _pstr(str), _normalized(norm), _absolute(abs) {}
  PosixPath(std::string&& str, bool norm, bool abs) :
      _pstr(std::move(str)),
      _normalized(norm),
      _absolute(abs) {}

  PosixPath& doConcat(const char* str, sizex size) {
    _pstr.append(str, size);
    _normalized = false;
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Determine if this path is absolute
  bool doIsAbsolute() const {
    // We're absolute if we have a root dir
    const auto& rootDirPos = path_detail::findRootDirPos(_pstr.data(), _pstr.size());
    const auto isAbs = rootDirPos != kNoPos;
    return isAbs;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Determine if this path is absolute
  bool doIsRelative() const { return !doIsAbsolute(); }

  /// Returns the canonical version of the given path. Canonical is absolute and normalized
  static inline PosixPath doMakeCanonical(const PosixPath& p, const PosixPath& cwd);

  /// Returns the normalized version of the given path. Normalized has no fluff
  static inline PosixPath doMakeNormalized(const PosixPath& p);

  ////////////////////////////////////////////////////////////////////////////////
  /// Append the given path onto the end of our path using a separator
  PosixPath& doAppend(const PosixPath& that) {
    // We're always going to append a sep if we don't have one, so figure it out now
    const auto thisSep = endsWith(_pstr, kSep);

    // Quick out for empty path
    if (that.empty()) {
      _pstr += kSep;
      return *this;
    }

    // Like c++17, if the append path is absolute, it replaces us. It also replaces if there is
    // any root-name. Because of these, we can solve all cases by simply looking for a root sep
    if (that._pstr[0] == kSep) {
      // It's absolute or has a root name - it fully replaces us
      *this = that;
      return *this;
    }

    // Go ahead and do the append, adding the separator if needed. Don't add a sep if we're empty
    if (!thisSep && !empty()) {
      _pstr += kSep;
    }
    _pstr.append(that._pstr.data(), that._pstr.size());

    // We can no longer ensure normalization. Absolute stays though
    _normalized = false;

    return *this;
  }

private:
  std::string _pstr;

  // When true, indicates the path is definitely normalized. False only means not sure
  bool _normalized = {false};

  // When true, indicates the path is definitely absolute. False means not sure
  bool _absolute = {false};

  friend PosixPath operator/(const PosixPath& lhs, const PosixPath& rhs) {
    auto lhsCopy = lhs;
    lhsCopy /= rhs;
    return lhsCopy;
  }

  friend PosixPath operator/(PosixPath&& lhs, const PosixPath& rhs) {
    lhs /= rhs;
    return std::move(lhs);
  }

  friend PosixPath operator+(const PosixPath& lhs, const PosixPath& rhs) {
    auto lhsCopy = lhs;
    lhsCopy += rhs;
    return lhsCopy;
  }

  friend PosixPath operator+(PosixPath&& lhs, const PosixPath& rhs) {
    lhs += rhs;
    return std::move(lhs);
  }

  friend std::ostream& operator<<(std::ostream& outs, const PosixPath& p) { return outs << p._pstr; }
  friend class path_detail::PathIterator<true>;
  friend class path_detail::PathIterator<false>;
};

inline bool operator==(const PosixPath& lhs, const PosixPath& rhs) {
  return lhs.compare(rhs) == 0;
}
inline bool operator!=(const PosixPath& lhs, const PosixPath& rhs) {
  return lhs.compare(rhs) != 0;
}
inline bool operator<(const PosixPath& lhs, const PosixPath& rhs) {
  return lhs.compare(rhs) < 0;
}
inline bool operator<=(const PosixPath& lhs, const PosixPath& rhs) {
  return !(rhs < lhs);
}
inline bool operator>(const PosixPath& lhs, const PosixPath& rhs) {
  return rhs < lhs;
}
inline bool operator>=(const PosixPath& lhs, const PosixPath& rhs) {
  return !(lhs < rhs);
}

////////////////////////////////////////////////////////////////////////////////
/// calculates the hash value for a path
inline sizex hash_value(const PosixPath& p) {
  return std::hash<PosixPath::string_type>()(p.u8());
}

////////////////////////////////////////////////////////////////////////////////
struct PosixPathHasher {
  sizex operator()(const PosixPath& p) const { return hash_value(p); }
};

////////////////////////////////////////////////////////////////////////////////
/// Converts a PosixPath to a windows compatible path string.
///
/// Note that this function is available on all platforms. On Posix systems, it will
/// use utf-32 wchars.
inline std::wstring toWin32(const PosixPath& path) {
  const auto& u8 = path.u8();
  const auto str = u8.data();
  const auto len = u8.size();

  // First convert the path string to windows format.
  const bool hasDriveRoot = path_detail::isDriveRoot(str, len);

  // Start the windows path. Chop off the first `//` if it's a drive root. Assumes ':'
  auto winPath = hasDriveRoot ? std::string(str + 2, len - 2) : std::string(u8);
  static_assert(path_detail::kDriveChar == ':', "If drive-root char changes, fix this code");

  // Convert to windows separators
  std::replace(winPath.begin(), winPath.end(), path_detail::kSep, path_detail::kWin32Sep);

  // Finally convert to wstring and be done with this madness
  const auto& wWinPath = sw::widen(winPath);
  return wWinPath;
}

////////////////////////////////////////////////////////////////////////////////
/// Converts a windows form path to PosixPath
///
/// Note that this function is available on all platforms. On Posix systems, it will
/// use utf-32 wchars.
inline PosixPath fromWin32(const std::wstring& wstr) {
  // First convert it to UTF8 so we can do our work
  std::string str = narrow(wstr);

  // Fix any windows separators
  std::replace(str.begin(), str.end(), path_detail::kWin32Sep, path_detail::kSep);

  // See if the path starts with a windows drive letter
  if (str.size() >= 2 && std::isalpha(str[0]) && str[1] == ':') {
    static_assert(path_detail::kDriveChar == ':', "If drive-root char changes, fix this code");

    // Definitely a drive letter. Convert to our //<drive-letter> format. Assumes ':' stays ':'
    str.insert(0, path_detail::kDoubleSepString, 2);
  }

  // And it's done
  return PosixPath(str);
}

////////////////////////////////////////////////////////////////////////////////
/// Calls to convert to OS path type. These have per-OS type signatures
#if SW_POSIX
inline std::string toOsNative(const PosixPath& path) {
  return path.u8();
}
inline PosixPath fromOsNative(const std::string& pathStr) {
  return PosixPath(pathStr);
}
#elif SW_WINDOWS
inline std::wstring toOsNative(const PosixPath& path) {
  return toWin32(path);
}
inline PosixPath fromOsNative(const std::wstring& pathStr) {
  return fromWin32(pathStr);
}
#else
#  error "Impliment for this OS"
#endif

////////////////////////////////////////////////////////////////////////////////
/// Returns the canonical version of the given path. Canonical is absolute and normalized. Note that
/// this is "weakly canonical" since it's only lexically analyzed.
/// @param cwd The current-working-dir which must be absolute. Used for absoluting a relative path.
inline PosixPath PosixPath::doMakeCanonical(const PosixPath& p, const PosixPath& cwd) {
  SW_ASSERT(cwd.is_absolute() == true);

  const auto abs = p.is_absolute() ? p : cwd / p;
  auto canon = abs.normalized();
  canon._absolute = true;
  return canon;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the normalized version of the given path. Normalized has no fluff
inline PosixPath PosixPath::doMakeNormalized(const PosixPath& p) {
  auto norm = p.doDefaultLexicalNormalization();
  norm._normalized = true;
  norm._absolute = p._absolute;
  return norm;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the filename. If the path end with a "/", then the filename
/// is considered to be ".".
template <typename StringClass>
StringClass PosixPath::doFilename() const {
  const auto& size = _pstr.size();
  if (size == 0) {
    return StringClass(path_detail::kEmptyString, 0);  // NOLINT Yes creating an empty
  }

  const auto* str = _pstr.data();
  const auto& fileAndRootPos = path_detail::findFilenamePos(str, size);
  const auto& fpos = std::get<0>(fileAndRootPos);
  if (fpos == kNoPos) {
    // No filename available
    return StringClass(path_detail::kEmptyString, 0);
  }

  // If the result is the root separator, then we return a '/'
  const auto& rootSepPos = std::get<1>(fileAndRootPos);
  if (rootSepPos == fpos) {
    SW_ASSERT(fpos == (size - 1));
    return StringClass(path_detail::kSepString, 1);
  }

  // If the result is the last char of the path and it's a '/', then the filename is "."
  const auto& isLastSlash = (fpos == (size - 1)) && (str[fpos] == kSep);
  const auto& result =
      isLastSlash ? StringClass(path_detail::kDotString, 1) : StringClass(str + fpos, size - fpos);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the parent-path
/// SCW: One of my least favorite functions I've written. A plethora os special cases.
///  And really, this whole class...
template <typename StringClass>
StringClass PosixPath::doParentPath() const {
  const auto& size = _pstr.size();
  const auto* str = _pstr.data();
  if (size == 0) {
    return StringClass(path_detail::kEmptyString, 0);  // NOLINT Yes creating an empty
  }

  // If the path ends with a separator, then we can remove it and call what's left a parent.
  // Except, handle multiple seps by removing all. So: '/x/' -> '/x', '/x///' -> '/x'
  const auto endIdx = size - 1;
  if (str[endIdx] == kSep) {
    auto sepIdx = endIdx;
    while (sepIdx-- > 0) {
      if (str[sepIdx] != kSep) {
        return StringClass(str, sepIdx + 1);
      }
    }
    return StringClass(path_detail::kEmptyString, 0);
  }

  // From here on, we've eliminated all paths that end with a separator.  The end should
  // thus be either a filename or a root-name.

  // Use the filename position and adjust appropriately
  const auto& fileAndRootPos = path_detail::findFilenamePos(str, size);
  const auto& fpos = std::get<0>(fileAndRootPos);
  const auto& rootSepPos = std::get<1>(fileAndRootPos);
  // If it's a file with no parent, return ""
  if (fpos == 0) {
    return StringClass(path_detail::kEmptyString, 0);  // NOLINT Yes creating an empty
  }

  // If there is no file, it must be a root-name in which case we return empty also
  if (fpos == kNoPos) {
    // Root-name means we shouldn't have found a root-separator either
    SW_ASSERT(rootSepPos == kNoPos);
    return StringClass(path_detail::kEmptyString, 0);  // NOLINT Yes creating an empty
  }

  // From here, we have a filename and a position. The position can be a separator before the filename
  if (str[fpos] == kSep) {
    return StringClass(str, fpos);
  }

  // Otherwise we take what's on the left, but not the separator unless it's a root
  if (rootSepPos == (fpos - 1)) {
    return StringClass(str, fpos);
  } else {
    SW_ASSERT(fpos > 1);  // This should be true given the drive lettering scheme. ie, '/x' would
                          // have come back as kNoPos because it's a drive letter.
    return StringClass(str, fpos - 1);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the root name. This will either be an empty string, a two character
/// string as "/<drive-letter>" for windows paths, or "//<hostname" for network
/// paths.
template <typename StringClass>
StringClass PosixPath::doRootName() const {
  const auto size = _pstr.size();
  const auto* str = _pstr.data();
  if (size < 2) {
    return StringClass(path_detail::kEmptyString, 0);
  }

  // Handle drive style
  const bool hasDriveRoot = path_detail::isDriveRoot(str, size);
  if (hasDriveRoot) {
    return StringClass(str, path_detail::kDriveRootPos);
  }

  // Handle network style
  const bool hasNetRoot = path_detail::isNetworkRoot(str, size);
  if (hasNetRoot) {
    const auto netSepPos = path_detail::findNetworkRootSep(str, size);
    const auto rootLen = netSepPos == kNoPos ? size : netSepPos;
    return StringClass(str, rootLen);
  }

  // Otherwise, no root name
  return StringClass(path_detail::kEmptyString, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// Returns either '/' if the path has a root dir, or '' if not.
template <typename StringClass>
StringClass PosixPath::doRootDir() const {
  const auto size = _pstr.size();
  const auto* str = _pstr.data();

  // Look for the root dir position. If we have one, we can return the slash
  const auto& rootDirPos = path_detail::findRootDirPos(str, size);
  const auto& result = rootDirPos == kNoPos ? StringClass(path_detail::kEmptyString, 0) :
                                              StringClass(path_detail::kSepString, 1);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the root path, which is "root_name / root_dir". If root_dir isn't
/// explicity in the path, it will not be in the result. So, "/c" will return
/// "/c", while "/c/" will return "/c/".
template <typename StringClass>
StringClass PosixPath::doRootPath() const {
  const auto size = _pstr.size();
  const auto* str = _pstr.data();

  // Handle drive style
  const bool hasDriveRoot = path_detail::isDriveRoot(str, size);
  if (hasDriveRoot) {
    // Include the root-dir if it's available
    const auto rootLen = std::min(path_detail::kDriveRootPos + 1, size);
    return StringClass(str, rootLen);
  }

  // Handle network style
  const bool hasNetRoot = path_detail::isNetworkRoot(str, size);
  if (hasNetRoot) {
    const auto netSepPos = path_detail::findNetworkRootSep(str, size);
    const auto rootLen = netSepPos == kNoPos ? size : netSepPos + 1;
    return StringClass(str, rootLen);
  }

  // Otherwise, no root name. We just need to check if char 0 is a sep
  bool isAbs = (size > 0 && str[0] == kSep);
  const auto& result =
      isAbs ? StringClass(path_detail::kSepString, 1) : StringClass(path_detail::kEmptyString, 0);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the path relative to the root.
template <typename StringClass>
StringClass PosixPath::doRelativePath() const {
  const auto size = _pstr.size();
  const auto* str = _pstr.data();

  // Handle drive style
  const bool hasDriveRoot = path_detail::isDriveRoot(str, size);
  if (hasDriveRoot) {
    constexpr const sizex kDriveRootSize = path_detail::kDriveRootPos + 1;
    // Include the root-dir if it's available
    return size > kDriveRootSize ? StringClass(str + kDriveRootSize, size - kDriveRootSize) :
                                   StringClass(path_detail::kEmptyString, 0);
  }

  // Handle network style
  const bool hasNetRoot = path_detail::isNetworkRoot(str, size);
  if (hasNetRoot) {
    const auto netSepPos = path_detail::findNetworkRootSep(str, size);
    const auto isEmpty = (netSepPos == kNoPos) || ((netSepPos + 1) >= size);
    const auto& result = isEmpty ? StringClass(path_detail::kEmptyString, 0) :
                                   StringClass(str + netSepPos + 1, size - netSepPos - 1);
    return result;
  }

  // Otherwise, no root name. We just need to deal with char 0 is a sep
  bool isAbs = (size > 0 && str[0] == kSep);
  const auto& result = isAbs && (size > 1) ? StringClass(str + 1, size - 1) : StringClass(str, size);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the extension if there is one
template <typename StringClass>
StringClass PosixPath::doExtension() const {
  const auto* str = _pstr.data();
  const auto size = _pstr.size();
  const auto extPos = path_detail::findExtensionPos(str, size);
  const auto& result = (extPos == kNoPos) ? StringClass(path_detail::kEmptyString, 0) :
                                            StringClass(str + extPos, _pstr.size() - extPos);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the extension if there is one
template <typename StringClass>
StringClass PosixPath::doStem() const {
  const auto size = _pstr.size();
  if (size == 0) {
    // Handling zero size here makes the logic simpler later
    return StringClass(path_detail::kEmptyString, 0);
  }

  // Backup until we find a dot or a /. Deal with it once we've got those
  const auto* str = _pstr.data();
  const auto endPos = size - 1;
  sizex pos = size;
  sizex lastDotPos = kNoPos;
  sizex fileStartPos = kNoPos;
  while (pos-- > 0) {
    if (str[pos] == kSep) {
      fileStartPos = pos + 1;
      break;
    }
    if (lastDotPos == kNoPos && str[pos] == kDot) {
      lastDotPos = pos;
    }
  }

  // Handle a path that ends with a /, so no filename
  if (fileStartPos == size) {
    // No filename
    return StringClass(path_detail::kEmptyString, 0);
  }

  // If we saw no separator, the whole thing is the filename
  if (fileStartPos == kNoPos) {
    fileStartPos = 0;
  }

  // Reconcile other special cases
  if (lastDotPos == kNoPos) {
    // No dot. Just return the filename
    return StringClass(str + fileStartPos, size - fileStartPos);
  } else {
    // Handle special cases. If last dot is first char, then we return the whole file
    if (lastDotPos == fileStartPos) {
      // This covers dot names like ".git" as well as "."
      return StringClass(str + fileStartPos, size - fileStartPos);
    } else if ((lastDotPos == endPos) && ((endPos - fileStartPos) == 1) && (str[fileStartPos] == kDot)) {
      return StringClass(str + fileStartPos, size - fileStartPos);  // Its ".."
    }
  }

  // Otherwise, we can return the file start up to the dot
  const auto finalSize = lastDotPos - fileStartPos;
  return StringClass(str + fileStartPos, finalSize);
}

namespace path_detail {

////////////////////////////////////////////////////////////////////////////////
enum class PathSection : u32 {
  None,
  RootName,
  RootDir,
  Dot,
  DotDot,
  Filename,
  FinalSep,  // Only returned to the user. Not a valid state
  Sep,       // Internal state only, not passed to the user
  End,       // Returned after all components have been processed (ie when done)
};

////////////////////////////////////////////////////////////////////////////////
struct PathSegment {
  StringView str;
  PathSection section = PathSection::End;

  friend bool operator==(const PathSegment& lhs, const PathSegment& rhs) {
    return lhs.section == rhs.section && lhs.str == rhs.str;
  }
  friend bool operator!=(const PathSegment& lhs, const PathSegment& rhs) {
    return lhs.section != rhs.section || lhs.str != rhs.str;
  }
};

////////////////////////////////////////////////////////////////////////////////
static constexpr PathSegment kEndSegment = PathSegment{};

////////////////////////////////////////////////////////////////////////////////
class PathSegmentIterator {
public:
  ~PathSegmentIterator() = default;
  PathSegmentIterator() = default;
  PathSegmentIterator(const std::string& pstr) : _pstr(pstr) {}
  PathSegmentIterator(std::string&& pstr) : _pstr(std::move(pstr)) {}

  PathSegmentIterator(const PathSegmentIterator&) = default;
  PathSegmentIterator& operator=(const PathSegmentIterator&) = default;

  PathSegmentIterator(PathSegmentIterator&& that) noexcept :
      _pstr(std::move(that._pstr)),
      _pos(std::exchange(that._pos, 0)),
      _lastSection(std::exchange(that._lastSection, PathSection::None)),
      _seenFilename(std::exchange(that._seenFilename, false)) {}
  PathSegmentIterator& operator=(PathSegmentIterator&& that) noexcept {
    _pstr = std::move(that._pstr);
    _pos = std::exchange(that._pos, 0);
    _lastSection = std::exchange(that._lastSection, PathSection::None);
    _seenFilename = std::exchange(that._seenFilename, false);
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathSegment begin() {
    // Allow this iterator to be reused
    _pos = 0;
    _lastSection = PathSection::None;
    _seenFilename = false;
    return advance();
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathSegment next() { return advance(); }

  ////////////////////////////////////////////////////////////////////////////////
  PathSegment end() const { return kEndSegment; }

  ////////////////////////////////////////////////////////////////////////////////
  static constexpr PathSegment endSegment() { return kEndSegment; }

private:
  ////////////////////////////////////////////////////////////////////////////////
  /// State machine approach. Moves to the next segment
  PathSegment advance() {
    // Finds the next section to process where _pos is at the start of the section
    while (true) {
      auto section = currentSection();
      _lastSection = section;
      switch (section) {
      case PathSection::None:
        SW_ASSERT(false);
        return kEndSegment;

      case PathSection::RootName:
        return onSectionRootName();
      case PathSection::Filename: {
        _seenFilename = true;
        return onSectionFilename();
      }
      case PathSection::RootDir: {
        _pos += 1;
        return PathSegment{StringView{kSepString, 1}, PathSection::RootDir};
      }
      case PathSection::Dot: {
        _pos += 1;
        _seenFilename = true;
        return PathSegment{StringView{kDotString, 1}, PathSection::Dot};
      }
      case PathSection::DotDot: {
        _pos += 2;
        _seenFilename = true;
        return PathSegment{StringView{kDotDotString, 2}, PathSection::DotDot};
      }
      case PathSection::FinalSep: {
        _pos += 1;
        // We only deliver a FinalSep to the user if we've seen a filename. This gives us the same
        // iteration behavior as std::fs
        return _seenFilename ? PathSegment{StringView{kSepString, 1}, PathSection::FinalSep} : kEndSegment;
      }

      // For separator, we keep going. (These aren't reported to the user)
      case PathSection::Sep: {
        _pos += 1;
        break;
      }

      case PathSection::End:
        return kEndSegment;
      }
    }

    return kEndSegment;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// State machine approach. Moves to the next segment
  ///
  /// Preconditions:
  /// @pre _lastPosition The previous section that had been processed
  /// @pre _pos is currently pointing at the first char of the next section
  PathSection currentSection() {
    switch (_lastSection) {
    case PathSection::End:
      return PathSection::End;
    case PathSection::FinalSep:
      return PathSection::End;

    case PathSection::None: {
      switch (current()) {
      case kNullTerm:
        return PathSection::End;
      case kSep:
        return onInitailSep();
      case kDot:
        return onSectionDot();
      default:
        return PathSection::Filename;
      }
    }

    case PathSection::RootName:
      switch (current()) {
      case kNullTerm:
        return PathSection::End;
      case kSep:
        return PathSection::RootDir;
      case kDot:
        return onSectionDot();
      default:
        return PathSection::Filename;
      }

    case PathSection::RootDir:
    case PathSection::Dot:
    case PathSection::DotDot:
    case PathSection::Filename:
    case PathSection::Sep:
      switch (current()) {
      case kNullTerm:
        return PathSection::End;
      case kSep:
        return (peek() == kNullTerm) ? PathSection::FinalSep : PathSection::Sep;
      case kDot:
        return onSectionDot();
      default:
        return PathSection::Filename;
      }
    }

    return PathSection::End;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Called when we encouncter a '/' as the very first character
  PathSection onInitailSep() const {
    switch (peek()) {
    case kSep: {
      // Two starting separators. We need to determine if this is a root name or not. It's a root name
      // if there's any valid character after the 2nd slash except a '/'
      switch (peekpeek()) {
      case kNullTerm:
      case kSep:
        return PathSection::RootDir;
      default:
        return PathSection::RootName;
      }
    }
    default: {
      return PathSection::RootDir;
    }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Called when we encouncter a '.' during the currentSection() call
  PathSection onSectionDot() const {
    switch (peek()) {
    case kDot:
      return PathSection::DotDot;
    case kSep:
      return PathSection::Dot;
    case kNullTerm:
      return PathSection::Dot;
    default:
      return PathSection::Filename;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// We're starting at a root-name (or root-drive)
  PathSegment onSectionRootName() {
    SW_ASSERT(_pos == 0);
    auto str = _pstr.data();
    auto endPos = _pstr.size();

    // Check for drive root first
    if (isDriveRoot(str, endPos)) {
      _pos += kDriveRootPos;
      return PathSegment{StringView(str, kDriveRootPos), PathSection::RootName};
    }

    // Try network root
    if (isNetworkRoot(str, endPos)) {
      const auto netSepPos = findNetworkRootSep(str, endPos);
      const auto rootNameLen = netSepPos == kNoPos ? endPos : netSepPos;
      _pos += rootNameLen;
      return PathSegment{StringView(str, rootNameLen), PathSection::RootName};
    }

    // Something went horribly wrong
    SW_ASSERT(false);
    return kEndSegment;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// We're starting a filename
  PathSegment onSectionFilename() {
    auto str = _pstr.data() + _pos;
    auto endPos = _pstr.size() - _pos;
    const auto nextSepPos = findNextSep(str, endPos, 3);
    const auto fnameLen = nextSepPos == kNoPos ? endPos : nextSepPos;

    _pos += fnameLen;
    return PathSegment{StringView(str, fnameLen), PathSection::Filename};
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Peeks at the next char. Returns 0 if it's out of size. Also works for initial case
  /// where _pos if essentially -1
  char current() const {
    auto result = _pos >= _pstr.size() ? kNullTerm : _pstr[_pos];
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Peeks at the next char. Returns 0 if it's out of size. Also works for initial case
  /// where _pos if essentially -1
  char peek() const {
    char result = (_pos + 1) >= _pstr.size() ? kNullTerm : _pstr[_pos + 1];
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Peeks at the next-next char. Returns 0 if it's out of size.
  char peekpeek() const {
    char result = (_pos + 2) >= _pstr.size() ? kNullTerm : _pstr[_pos + 2];
    return result;
  }

private:
  std::string _pstr;
  sizex _pos = 0;
  PathSection _lastSection = PathSection::None;
  bool _seenFilename = false;
};  // namespace ts

////////////////////////////////////////////////////////////////////////////////
/// Determine if the string is a drive-letter style root directory
inline bool isDriveRoot(const char* str, sizex endPos) {
  // First check for '//x:'
  const auto isDriveRoot = (endPos >= kDriveRootPos) && str[3] == kDriveChar && (str[0] == kPathSep) &&
                           (str[1] == kPathSep) && sw::isalpha(str[2]);
  return isDriveRoot;
}

////////////////////////////////////////////////////////////////////////////////
/// Determine if the string is a network style root directory
inline bool isNetworkRoot(const char* str, sizex endPos) {
  // Check for '//x', but eliminate drive root `//x:`
  const auto isNetRoot = (endPos >= 3) && (str[0] == kPathSep) && (str[1] == kPathSep) && sw::isalnum(str[2]);
  const auto isNetRootAndNotDriveRoot = isNetRoot && !((endPos >= kDriveRootPos) && (str[3] == kDriveChar));
  return isNetRootAndNotDriveRoot;
}

////////////////////////////////////////////////////////////////////////////////
/// Determine if the path has a root name
inline bool hasRootName(const char* str, sizex endPos) {
  const auto hasRoot = isDriveRoot(str, endPos) || isNetworkRoot(str, endPos);
  return hasRoot;
}

/// Finds the first character which will be '.' of the file's extension.
/// @return First character of the extension, or kNoPos
inline sizex findExtensionPos(const char* str, sizex endPos) {
  if (endPos == 0) {
    kNoPos;
  }

  // Backup until we find a dot or a /. Deal with it once we've got those
  sizex pos = endPos;
  while (pos-- > 0) {
    if (str[pos] == kSep) {
      break;
    }
    if (str[pos] == kDot) {
      // We're at the dot, so we're done. But we need to handle exceptions for path names "." and ".."
      if ((endPos == 1) || (pos == 0) || (str[pos - 1] == kSep)) {
        break;  // It starts with dot so we return ""
      }
      if (pos > 0 && str[pos - 1] == kDot) {
        if (endPos == 2 || (str[pos - 2] == kSep)) {
          break;  // It's '..'
        }
      }
      return pos;
    }
  }

  return kNoPos;
}

/// Find the next separator starting at startPos toward endPos
/// @return the next separator position, or npos if not found
inline sizex findNextSep(const char* str, sizex endPos, sizex startPos) {
  for (; startPos < endPos; ++startPos) {
    if (str[startPos] == kPathSep) {
      return startPos;
    }
  }
  return kNoPos;
}

/// Find the prev separator before endPos
/// @return the prev separator position, or npos if not found
inline sizex findPrevSep(const char* str, sizex endPos) {
  if (endPos > 0) {
    while (endPos--) {
      if (str[endPos] == kPathSep) {
        return endPos;
      }
    }
  }
  return kNoPos;
}

////////////////////////////////////////////////////////////////////////////////
/// This should only be called after it's been verified that the path has a network root
/// @return Position of network separator, or kNoPos if missing
inline sizex findNetworkRootSep(const char* str, sizex endPos) {
  SW_ASSERT(isNetworkRoot(str, endPos));
  // Since we assume that it's '//x', we just look for the first sep after
  const auto pos = findNextSep(str, endPos, 3);
  return pos;
}

////////////////////////////////////////////////////////////////////////////////
/// Determine if the the position is the root separator.  It's a root separator
/// if the path is '/<drive-letter>/blah' where the sep is at pos kDriveRootPos, or
/// if the path is '//<hostname/blash' and it's the sep after the hostname.
/// @pos The position of the separator to check
inline bool isRootSeparator(const char* str, sizex endPos, sizex pos) {
  if ((pos == kDriveRootPos) && isDriveRoot(str, pos)) {
    return true;
  }
  if ((pos >= 3) && isNetworkRoot(str, endPos)) {
    const auto rootSepPos = findNetworkRootSep(str, endPos);
    return rootSepPos == pos;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// Finds the root directory position, or kNoPos if none
inline sizex findRootDirPos(const char* str, sizex endPos) {
  // Do easy eliminations
  if (endPos == 0 || str[0] != kSep) {
    return kNoPos;
  }

  // Check for drive root first
  if (isDriveRoot(str, endPos)) {
    return endPos > kDriveRootPos ? kDriveRootPos : kNoPos;
  }

  // Try network root
  if (isNetworkRoot(str, endPos)) {
    return findNetworkRootSep(str, endPos);
  }

  // Otherwise it must be the 0 sep
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the starting position of the filename. If the path ends as with
/// kSep, we return the final kSep position.
////
/// @param endPos is one past the last char to lookat
/// @return Tuple with starting position of the filename, and position indicating the root sep. (npos if none)
inline std::tuple<sizex, sizex> findFilenamePos(const char* str, sizex endPos) {
  // Find the last separator before the end
  const auto lastPos = endPos - 1;
  const auto lastSep = findPrevSep(str, endPos);

  // If no separator, the whole thing must be a filename
  if (lastSep == kNoPos) {
    return std::make_tuple(0_z, kNoPos);
  }

  sizex rootSepPos = kNoPos;
  if (isDriveRoot(str, endPos)) {
    if (lastSep == 1) {
      return std::make_tuple(kNoPos, kNoPos);  // '//c:' -> ""
    }
    if (lastSep == kDriveRootPos && lastPos == lastSep) {
      return std::make_tuple(lastSep, kDriveRootPos);  // '//c:/'
    }
    rootSepPos = kDriveRootPos;
  } else if (isNetworkRoot(str, endPos)) {
    rootSepPos = findNetworkRootSep(str, endPos);
    if (lastSep == 1) {
      return std::make_tuple(kNoPos, kNoPos);  // '//c'
    }
    if (lastSep == lastPos && lastSep == rootSepPos) {
      return std::make_tuple(lastSep, rootSepPos);  // '//c/'
    }
  } else if (str[0] == kSep) {
    rootSepPos = 0;
  }

  // Otherwise, we can just return the final sep (or one after)
  const auto& resultPos = lastSep == lastPos ? lastSep : lastSep + 1;
  const auto& result = std::make_tuple(resultPos, rootSepPos);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// This will be the public facing iterator
///
/// Supports bi-directional by holding an actual stack of path segments.
/// The stack is lazy eval'd, so calling 'end()' will not incur any
/// allocations or do any work.
///
/// SCW: For now I'm just being lazy because I don't want to write the reverse state
///  machine, so using the PathSegmentIterator to build a stack instead. Don't
///  even know if I'll ever use path iteration, but make it better if needed.
template <bool kIsConst>
class PathIterator {
  static constexpr sizex kNoPos = ~0_z;
  using PathType = std::conditional_t<kIsConst, const PosixPath, PosixPath>;
  using NonConstPathType = PosixPath;
  using PathVec = std::vector<PathType>;

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = PathType;
  using difference_type = ssizex;
  using pointer = PathType*;
  using reference = PathType&;

  ~PathIterator() = default;
  PathIterator(const PosixPath& path, sizex pos) : _path(&path), _current(pos) {}

  PathIterator(const PathIterator& that) : _path(that._path), _current(that._current) {}
  PathIterator operator=(const PathIterator& that) {
    _path = that._path;
    _current = that._current;
    return *this;
  }

  // TODO: write by hand
  //  PathIterator(PathIterator&& that) = default;
  //  PathIterator& operator=(PathIterator&& that) = default;

  /// Gets the path segments. Will create if needed
  PathVec& segments() {
    if (!_segments) {
      auto iter = PathSegmentIterator(_path->u8());
      _segments = std::make_unique<PathVec>();
      for (auto segment = iter.begin(); segment != iter.end(); segment = iter.next()) {
        _segments->emplace_back(segment.str);
      }
    }
    return *_segments;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathIterator& operator++() {
    // To be compatible with our end() iterator which has it's position as kNoPos, we
    // must check that here and set it appropriately
    if (_current == (segments().size() - 1)) {
      _current = kNoPos;
    } else {
      ++_current;
    }
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathIterator operator++(int) & {
    auto tmp = *this;
    this->operator++();
    return tmp;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathIterator& operator--() {
    // Note that 'begin' iter is not decrementable by the standard, so we don't deal
    // with that case. Handle end() case where value is kNoPos
    if (_current == kNoPos) {
      _current = segments().size() - 1;
    } else {
      --_current;
    }
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  PathIterator operator--(int) & {
    auto tmp = *this;
    this->operator--();
    return tmp;
  }

  ////////////////////////////////////////////////////////////////////////////////
  const value_type& operator*() {
    const auto& segs = segments();
    return segs[_current];
  }

  ////////////////////////////////////////////////////////////////////////////////
  friend bool operator==(const PathIterator& lhs, const PathIterator& rhs) {
    /// TODO: For now assumes source paths are the same.
    return (lhs._current == rhs._current);
  }

  friend bool operator!=(const PathIterator& lhs, const PathIterator& rhs) { return !(lhs == rhs); }

private:
  // Going to use an SP to a vector so that default construction avoids
  // any allocation. Think 'end()' call. Cheaper copies, etc.
  std::unique_ptr<std::vector<NonConstPathType>> _segments;
  const PosixPath* _path = nullptr;
  sizex _current = ~0_z;
};

}  // namespace path_detail

////////////////////////////////////////////////////////////////////////////////
/// Computes the normalized form of this path. See std::filesystem::path::lexically_normal.
///
/// A path can be normalized by following this algorithm:
/// 1 If the path is empty, stop (normal form of an empty path is an empty path)
/// 2 Replace each directory-separator (which may consist of multiple slashes) with a single
///   path::preferred_separator.
/// 3 Replace each slash character in the root-name with path::preferred_separator.
/// 4 Remove each dot and any immediately following directory-separator.
/// 5 Remove each non-dot-dot filename immediately followed by a directory-separator and a dot-dot, along with
///   any immediately following directory-separator.
/// 6 If there is root-directory, remove all dot-dots and any directory-separators immediately following them.
/// 7 If the last filename is dot-dot, remove any trailing directory-separator.
/// 8 If the path is empty, add a dot (normal form of ./ is .)
inline PosixPath PosixPath::lexically_normal() const {
  namespace pd = path_detail;

  // Handle #1
  if (empty()) {
    return *this;
  }

  // This will essentially be a stack form of the path. We'll use the iterator to build it
  std::vector<pd::PathSegment> segments;
  segments.reserve(32);

  /// #2 and #3 happens automatically from iterator
  /// Gets the previous segment. Returns End if there is no previous segment
  const auto& prevSegment = [&]() {
    const auto& result = segments.empty() ? pd::PathSegmentIterator::endSegment() :
                                            (segments.size() > 1 ? segments[segments.size() - 2] :
                                                                   pd::PathSegmentIterator::endSegment());
    return result;
  };

  /// Iterate and process
  pd::PathSegmentIterator iter(*this);
  pd::PathSection lastSection = pd::PathSection::None;
  bool concatFinalSep = false;
  for (auto segment = iter.begin(); segment != iter.end(); segment = iter.next()) {
    switch (segment.section) {
    case pd::PathSection::Dot:
      break;  // #4. Just ignore any dot we find

    // Dotdot has a various implications.
    case pd::PathSection::DotDot: {
      // #5. If we have a preceeding filename, we can get rid of it. Otherwise we must add the ".."
      // #6 If there is root-directory, remove all dot-dots ... immediately following them.
      switch (prevSegment().section) {
      case pd::PathSection::Filename:
        segments.pop_back();
        break;
      case pd::PathSection::RootDir:
        break;  // Don't add the ".." if it's preceeded by the root dir
      default:
        segments.push_back(segment);
      }
      break;
    }

    case pd::PathSection::RootDir:
    case pd::PathSection::RootName:
    case pd::PathSection::Filename: {
      segments.push_back(segment);
      break;
    }

    case pd::PathSection::FinalSep: {
      // #7 If the last filename is dot-dot, remove any trailing directory-separator.
      // Also, #8, "./" should be "."
      // Instead of adding the final-sep segment, we just flag it. The reason here is so we can
      // reconstruct the final path entirely using "/=". If it we added the "/", it would be
      // treated as absolute and mess it all up
      if (prevSegment().section != pd::PathSection::DotDot && lastSection != pd::PathSection::Dot) {
        concatFinalSep = true;
      }
      break;
    }

    // None of these should occur
    case pd::PathSection::Sep:
    case pd::PathSection::None:
    case pd::PathSection::End: {
      SW_ASSERT(false);
      break;
    }
    }

    lastSection = segment.section;
  }

  // #8 If the path is empty, add a dot (normal form of ./ is .)
  if (segments.empty()) {
    return pd::kDotString;
  }

  // If the last thing we saw in the path was '.', then we need to check for the form
  // of "/x/." which should norm to "/x/". Without this check we get "/x".
  if (lastSection == pd::PathSection::Dot && segments.back().section != pd::PathSection::RootDir) {
    concatFinalSep = true;
  }

  // Guess the size to limit the result allocations. Should always be >= actual. Include space
  // for separators by adding segments.size()
  const auto sizeGuess =
      segments.size() + (concatFinalSep ? 1 : 0) +
      std::accumulate(segments.begin(), segments.end(), 0_z,
                      [](sizex total, const auto& segment) { return total + segment.str.size(); });

  // Now construct the result via the final stack
  PosixPath result;
  result._pstr.reserve(sizeGuess);
  for (const auto& segment : segments) {
    if (segment.section == pd::PathSection::RootDir) {
      // Can't use /= for root-dir separator - it will wipe out a dir-name if we had one since it's abs
      result += segment.str;
    } else {
      result /= segment.str;
    }
  }

  // And the final sep as needed
  if (concatFinalSep) {
    result += kSep;
  }

  // This is not a critical issue - just adjust the sizeGuess if this is ever triggered
  SW_ASSERT(sizeGuess >= result._pstr.size());

  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// Based on the std::filesystem::path::lexically_normal algorithm, with the following
/// modifications:
///
/// * All ending separators will be removed. This should more consistency in the output in that
///   the different versions of the same path will always normalize to the same thing. Thus,
///   the following normalizations will occur:
///    '/x/y/.' -> '/x/y'
///    '/x/y/' -> '/x/y'
///    '/x/y' -> '/x/y'
///
/// * If a path is empty after being normalized, it will be returned as ""
///    '.' -> ''
///    './' -> ''
///    './///' -> ''
/// @return
inline PosixPath PosixPath::lexically_full_normal() const {
  namespace pd = path_detail;

  // Handle #1
  if (empty()) {
    return *this;
  }

  // This will essentially be a stack form of the path. We'll use the iterator to build it
  std::vector<pd::PathSegment> segments;
  segments.reserve(32);

  /// #2 and #3 happens automatically from iterator
  /// Gets the previous segment. Returns End if there is no previous segment
  const auto& prevSegment = [&]() {
    const auto& result = segments.empty() ? pd::PathSegmentIterator::endSegment() :
                                            (segments.size() > 1 ? segments[segments.size() - 2] :
                                                                   pd::PathSegmentIterator::endSegment());
    return result;
  };

  /// Iterate and process
  pd::PathSegmentIterator iter(*this);
  pd::PathSection lastSection = pd::PathSection::None;
  for (auto segment = iter.begin(); segment != iter.end(); segment = iter.next()) {
    switch (segment.section) {
    case pd::PathSection::Dot:
      break;  // #4. Just ignore any dot we find

      // Dotdot has a various implications.
    case pd::PathSection::DotDot: {
      // #5. If we have a preceeding filename, we can get rid of it. Otherwise we must add the ".."
      // #6 If there is root-directory, remove all dot-dots ... immediately following them.
      switch (prevSegment().section) {
      case pd::PathSection::Filename:
        segments.pop_back();
        break;
      case pd::PathSection::RootDir:
        break;  // Don't add the ".." if it's preceeded by the root dir
      default:
        segments.push_back(segment);
      }
      break;
    }

    case pd::PathSection::RootDir:
    case pd::PathSection::RootName:
    case pd::PathSection::Filename: {
      segments.push_back(segment);
      break;
    }

    case pd::PathSection::FinalSep: {
      break;
    }

      // None of these should occur
    case pd::PathSection::Sep:
    case pd::PathSection::None:
    case pd::PathSection::End: {
      SW_ASSERT(false);
      break;
    }
    }

    lastSection = segment.section;
  }

  // Don't turn "" into a dot
  if (segments.empty()) {
    return PosixPath{};
  };

  // Guess the size to limit the result allocations. Should always be >= actual. Include space
  // for separators by adding segments.size()
  const auto sizeGuess =
      segments.size() +
      std::accumulate(segments.begin(), segments.end(), 0_z,
                      [](sizex total, const auto& segment) { return total + segment.str.size(); });

  // Now construct the result via the final stack
  PosixPath result;
  result._pstr.reserve(sizeGuess);
  for (const auto& segment : segments) {
    if (segment.section == pd::PathSection::RootDir) {
      // Can't use /= for root-dir separator - it will wipe out a dir-name if we had one since it's abs
      result += segment.str;
    } else {
      result /= segment.str;
    }
  }

  // This is not a critical issue - just adjust the sizeGuess if this is ever triggered
  SW_ASSERT(sizeGuess >= result._pstr.size());

  return result;
}
////////////////////////////////////////////////////////////////////////////////
inline PosixPath::iterator PosixPath::begin() {
  return iterator(*this, empty() ? kNoPos : 0);
}

////////////////////////////////////////////////////////////////////////////////
inline PosixPath::const_iterator PosixPath::cbegin() const {
  return const_iterator(*this, empty() ? kNoPos : 0);
}

////////////////////////////////////////////////////////////////////////////////
inline PosixPath::iterator PosixPath::end() {
  return iterator(*this, kNoPos);
}

////////////////////////////////////////////////////////////////////////////////
inline PosixPath::const_iterator PosixPath::cend() const {
  return const_iterator(*this, kNoPos);
}

////////////////////////////////////////////////////////////////////////////////
inline PosixPath::native_string_type PosixPath::native() const {
  return toOsNative(*this);
}

}  // namespace sw
