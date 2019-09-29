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
#include <sw/posix_path.h>

#include <gtest/gtest.h>

#include <iostream>
#include <unordered_map>

namespace sw {

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, rootChecks) {
  ASSERT_TRUE(path_detail::isDriveRoot("//C:", 4));
  ASSERT_TRUE(path_detail::isDriveRoot("//c:", 4));
  ASSERT_TRUE(path_detail::isDriveRoot("//c:/", 5));
  ASSERT_TRUE(path_detail::isDriveRoot("//c:/foobar", 11));
  ASSERT_FALSE(path_detail::isDriveRoot("", 0));
  ASSERT_FALSE(path_detail::isDriveRoot("/", 1));
  ASSERT_FALSE(path_detail::isDriveRoot("/cx/foobar", 10));
  ASSERT_FALSE(path_detail::isDriveRoot("c/", 2));
  ASSERT_FALSE(path_detail::isDriveRoot("//host/foobar", 13));

  ASSERT_TRUE(path_detail::isNetworkRoot("//foo", 5));
  ASSERT_TRUE(path_detail::isNetworkRoot("//foo/", 6));
  ASSERT_TRUE(path_detail::isNetworkRoot("//fo/f", 6));
  ASSERT_FALSE(path_detail::isNetworkRoot("", 0));
  ASSERT_FALSE(path_detail::isNetworkRoot("/", 1));
  ASSERT_FALSE(path_detail::isNetworkRoot("//c:/foo", 8));
  ASSERT_FALSE(path_detail::isNetworkRoot("c:/", 2));

  ASSERT_TRUE(path_detail::hasRootName("//foo", 5));
  ASSERT_TRUE(path_detail::hasRootName("//foo/", 6));
  ASSERT_TRUE(path_detail::hasRootName("//fo/f", 6));
  ASSERT_TRUE(path_detail::hasRootName("//c:", 4));
  ASSERT_TRUE(path_detail::hasRootName("//c:/", 5));
  ASSERT_TRUE(path_detail::hasRootName("//c:/foobar", 11));
  ASSERT_FALSE(path_detail::hasRootName("", 0));
  ASSERT_FALSE(path_detail::hasRootName("/", 1));
  ASSERT_FALSE(path_detail::hasRootName("/cx/foo", 6));
  ASSERT_FALSE(path_detail::hasRootName("c:/", 2));

  ASSERT_TRUE(path_detail::isRootSeparator("//f/foo/bar", 11, 3));
  ASSERT_FALSE(path_detail::isRootSeparator("//f/foo/bar", 11, 0));
  ASSERT_FALSE(path_detail::isRootSeparator("//f/foo/bar", 11, 1));
  ASSERT_FALSE(path_detail::isRootSeparator("//f/foo/bar", 11, 2));
  ASSERT_FALSE(path_detail::isRootSeparator("//f/foo/bar", 11, 4));
  ASSERT_FALSE(path_detail::isRootSeparator("//f:/foo/bar", 12, 0));
  ASSERT_FALSE(path_detail::isRootSeparator("//f:/foo/bar", 12, 1));
  ASSERT_FALSE(path_detail::isRootSeparator("//f:/foo/bar", 12, 2));
  ASSERT_FALSE(path_detail::isRootSeparator("//f:/foo/bar", 12, 3));
  ASSERT_TRUE(path_detail::isRootSeparator("//f:/foo/bar", 12, 4));
  ASSERT_FALSE(path_detail::isRootSeparator("//f:/foo/bar", 12, 5));
  ASSERT_FALSE(path_detail::isRootSeparator("/foo/bar", 10, 4));
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, separatorChecks) {
  ASSERT_EQ(1, path_detail::findNextSep("f/foo/bar/baz", 13, 0));
  ASSERT_EQ(0, path_detail::findNextSep("/foo/bar/baz", 12, 0));
  ASSERT_EQ(4, path_detail::findNextSep("/foo/bar/baz", 12, 1));
  ASSERT_EQ(std::string::npos, path_detail::findNextSep("/foo/bar/baz", 12, 10));
  ASSERT_EQ(8, path_detail::findPrevSep("/foo/bar/baz", 12));
  ASSERT_EQ(4, path_detail::findPrevSep("/foo/bar/baz", 8));
  ASSERT_EQ(0, path_detail::findPrevSep("/foo/bar/baz", 3));
  ASSERT_EQ(std::string::npos, path_detail::findPrevSep("_foo_bar/baz", 8));

  ASSERT_EQ(3, path_detail::findNetworkRootSep("//f/foo/bar", 11));
  ASSERT_EQ(5, path_detail::findNetworkRootSep("//foo/", 6));
  ASSERT_EQ(std::string::npos, path_detail::findNetworkRootSep("//foo", 5));

  constexpr auto npos = std::string::npos;
  ASSERT_EQ(std::make_tuple(2_z, npos), path_detail::findFilenamePos("f/baz", 5));
  ASSERT_EQ(std::make_tuple(5_z, 4), path_detail::findFilenamePos("//f:/baz", 8));
  ASSERT_EQ(std::make_tuple(4_z, 4), path_detail::findFilenamePos("//f:/", 5));
  ASSERT_EQ(std::make_tuple(npos, npos), path_detail::findFilenamePos("//f:", 4));
  ASSERT_EQ(std::make_tuple(1_z, 0), path_detail::findFilenamePos("/fo", 3));
  ASSERT_EQ(std::make_tuple(0_z, npos), path_detail::findFilenamePos("foo", 0));
  ASSERT_EQ(std::make_tuple(3_z, 3), path_detail::findFilenamePos("//f/", 4));
  ASSERT_EQ(std::make_tuple(4_z, 3), path_detail::findFilenamePos("//f/foo", 7));
  ASSERT_EQ(std::make_tuple(npos, npos), path_detail::findFilenamePos("//f", 3));
  ASSERT_EQ(std::make_tuple(0_z, 0), path_detail::findFilenamePos("/", 1));
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, filename) {
  ASSERT_EQ("foobar.txt", PosixPath("/foo/foobar.txt").filename());
  ASSERT_EQ("foobar.txt", PosixPath("/foobar.txt").filename());
  ASSERT_EQ(".", PosixPath("/foo/").filename());
  ASSERT_EQ(".", PosixPath("/foo/.").filename());
  ASSERT_EQ("/", PosixPath("/").filename());
  // Drive roots
  ASSERT_EQ("/", PosixPath("//f:/").filename());
  ASSERT_EQ("bar", PosixPath("//f:/bar").filename());
  ASSERT_EQ("", PosixPath("//f:").filename());
  // Net roots
  ASSERT_EQ("", PosixPath("//blah").filename());
  ASSERT_EQ("/", PosixPath("//blah/").filename());
  ASSERT_EQ("foo", PosixPath("//blah/foo").filename());

  ASSERT_TRUE(PosixPath("/foo/foobar.txt").has_filename());
  ASSERT_TRUE(PosixPath("/foo/").has_filename());
  ASSERT_TRUE(PosixPath("/foo").has_filename());
  ASSERT_TRUE(PosixPath("//c:/").has_filename());
  ASSERT_TRUE(PosixPath("/").has_filename());
  ASSERT_FALSE(PosixPath("//c:").has_filename());
  ASSERT_FALSE(PosixPath("//blash").has_filename());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, extension) {
  ASSERT_EQ("", PosixPath("/foo/").extension());
  ASSERT_EQ(".txt", PosixPath("/foo/foobar.txt").extension());
  ASSERT_EQ("", PosixPath("/foo/.txt").extension());
  ASSERT_EQ("", PosixPath("/foo/.").extension());
  ASSERT_EQ("", PosixPath("/foo/..").extension());
  ASSERT_EQ(".bat", PosixPath("/foo/bar.bat").extension());
  ASSERT_EQ(".txt", PosixPath("foobar.txt").extension());
  ASSERT_EQ("", PosixPath(".txt").extension());
  ASSERT_EQ("", PosixPath(".").extension());
  ASSERT_EQ("", PosixPath("..").extension());
  ASSERT_EQ(".bat", PosixPath("bar.bat").extension());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, stem) {
  ASSERT_EQ("", PosixPath("/foo/").stem());
  ASSERT_EQ("foobar", PosixPath("/foo/foobar.txt").stem());
  ASSERT_EQ(".txt", PosixPath("/foo/.txt").stem());
  ASSERT_EQ(".", PosixPath("/foo/.").stem());
  ASSERT_EQ("..", PosixPath("/foo/..").stem());
  ASSERT_EQ("bar", PosixPath("/foo/bar.bat").stem());
  ASSERT_EQ("foobar", PosixPath("foobar.txt").stem());
  ASSERT_EQ(".txt", PosixPath(".txt").stem());
  ASSERT_EQ(".", PosixPath(".").stem());
  ASSERT_EQ("..", PosixPath("..").stem());
  ASSERT_EQ("bar", PosixPath("bar.bat").stem());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, parent) {
  ASSERT_EQ("/foo", PosixPath("/foo/foobar.txt").parent_path());
  ASSERT_EQ("/", PosixPath("/foobar.txt").parent_path());
  ASSERT_EQ("/foo", PosixPath("/foo/").parent_path());
  ASSERT_EQ("/foo", PosixPath("/foo/.").parent_path());
  ASSERT_EQ("", PosixPath("/").parent_path());
  ASSERT_EQ("/foo", PosixPath("/foo/").parent_path());
  ASSERT_EQ("/foo", PosixPath("/foo////").parent_path());
  // Drive roots
  ASSERT_EQ("//f:/bar", PosixPath("//f:/bar/").parent_path());
  ASSERT_EQ("//f:/", PosixPath("//f:/bar").parent_path());
  ASSERT_EQ("//f:", PosixPath("//f:/").parent_path());
  ASSERT_EQ("", PosixPath("//f:").parent_path());
  ASSERT_EQ("", PosixPath("").parent_path());
  // Net roots
  ASSERT_EQ("//blah/", PosixPath("//blah/foo").parent_path());
  ASSERT_EQ("//blah", PosixPath("//blah/").parent_path());
  ASSERT_EQ("", PosixPath("//blah").parent_path());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, rootCalls) {
  ASSERT_EQ("", PosixPath("/foo/foobar.txt").root_name());
  ASSERT_EQ("", PosixPath("/fo/foobar.txt").root_name());
  ASSERT_EQ("//f:", PosixPath("//f:/foobar.txt").root_name());
  ASSERT_EQ("//f:", PosixPath("//f:").root_name());
  ASSERT_EQ("//f:", PosixPath("//f:/").root_name());
  ASSERT_EQ("//foo", PosixPath("//foo/bar").root_name());
  ASSERT_EQ("//foo", PosixPath("//foo/").root_name());
  ASSERT_EQ("//foo", PosixPath("//foo").root_name());

  ASSERT_EQ("/", PosixPath("/foo/foobar.txt").root_directory());
  ASSERT_EQ("/", PosixPath("/fo/foobar.txt").root_directory());
  ASSERT_EQ("/", PosixPath("//f:/foobar.txt").root_directory());
  ASSERT_EQ("", PosixPath("//f:").root_directory());
  ASSERT_EQ("/", PosixPath("//f:/").root_directory());
  ASSERT_EQ("", PosixPath("//foo").root_directory());
  ASSERT_EQ("/", PosixPath("//foo/").root_directory());
  ASSERT_EQ("/", PosixPath("//foo/foo").root_directory());
  ASSERT_EQ("", PosixPath("f/foo").root_directory());
  ASSERT_EQ("", PosixPath("x/foo").root_directory());

  ASSERT_EQ("/", PosixPath("/foo/foobar.txt").root_path());
  ASSERT_EQ("/", PosixPath("/fo/foobar.txt").root_path());
  ASSERT_EQ("//f:/", PosixPath("//f:/foobar.txt").root_path());
  ASSERT_EQ("//f:", PosixPath("//f:").root_path());
  ASSERT_EQ("//f:/", PosixPath("//f:/").root_path());
  ASSERT_EQ("//foo", PosixPath("//foo").root_path());
  ASSERT_EQ("//foo/", PosixPath("//foo/foo").root_path());
  ASSERT_EQ("", PosixPath("f/foo").root_path());
  ASSERT_EQ("", PosixPath("x/foo").root_path());

  ASSERT_EQ("foo/foobar.txt", PosixPath("/foo/foobar.txt").relative_path());
  ASSERT_EQ("fo/foobar.txt", PosixPath("/fo/foobar.txt").relative_path());
  ASSERT_EQ("foobar.txt", PosixPath("//f:/foobar.txt").relative_path());
  ASSERT_EQ("", PosixPath("//f:").relative_path());
  ASSERT_EQ("", PosixPath("//f:/").relative_path());
  ASSERT_EQ("", PosixPath("//foo").relative_path());
  ASSERT_EQ("", PosixPath("//foo/").relative_path());
  ASSERT_EQ("f", PosixPath("//foo/f").relative_path());
  ASSERT_EQ("foo", PosixPath("//foo/foo").relative_path());
  ASSERT_EQ("f/foo", PosixPath("f/foo").relative_path());
  ASSERT_EQ("x/foo", PosixPath("x/foo").relative_path());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, append) {
  auto pres = PosixPath("/foo/bar");
  auto pfoo = PosixPath("/foo");
  auto pfoo2 = PosixPath("/foo/");
  auto pbar = PosixPath("bar");

  ASSERT_EQ(pres, pfoo / pbar);
  ASSERT_EQ(pres, pfoo2 / pbar);
  ASSERT_EQ(pres, "/foo" / pbar);
  ASSERT_EQ(pres, "/foo/" / pbar);
  ASSERT_EQ("/foo/", PosixPath("/foo") / "");

  ASSERT_EQ("/x/y/", PosixPath("/x/y") / "");
  ASSERT_EQ("/x/y/.", PosixPath("/x/y") / ".");
  ASSERT_EQ("/x/y/.", PosixPath("/x/y/") / ".");

  ASSERT_EQ(PosixPath("f"), PosixPath("") / "f");
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, concatAndShorten) {
  ASSERT_EQ("/foo", PosixPath("/foo") + "");
  ASSERT_EQ("/foobar", PosixPath("/foo") + "bar");

  ASSERT_EQ("/foo", PosixPath("/foo/.").shorten(2));
  ASSERT_EQ("/", PosixPath("/foo").shorten(3));
  ASSERT_EQ("", PosixPath("/foo").shorten(4));
  ASSERT_EQ("", PosixPath("/foo").shorten(10));
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, filenameOps) {
  ASSERT_EQ("/foo/", PosixPath("/foo/foo.txt").remove_filename() + "");
  ASSERT_EQ("/foo/", PosixPath("/foo/").remove_filename() + "");
  ASSERT_EQ("foo/", PosixPath("foo/foo.txt").remove_filename() + "");
  ASSERT_EQ("", PosixPath("foo").remove_filename() + "");

  ASSERT_EQ("/foo/bar.txt", PosixPath("/foo/foo.txt").replace_filename("bar.txt"));
  ASSERT_EQ("/foo/bar.txt", PosixPath("/foo/").replace_filename("bar.txt"));
  ASSERT_EQ("/foo/x/y", PosixPath("/foo/").replace_filename("x/y"));

  ASSERT_EQ("/foo/foo.cxx", PosixPath("/foo/foo.cpp").replace_extension("cxx"));
  ASSERT_EQ("/foo/foo.cxx", PosixPath("/foo/foo.cpp").replace_extension(".cxx"));
  ASSERT_EQ("/foo/.cpp.cxx", PosixPath("/foo/.cpp").replace_extension(".cxx"));
  ASSERT_EQ("/foo/foo.cpp", PosixPath("/foo/foo.cpp.cxx").replace_extension(""));
  ASSERT_EQ("/foo/foo", PosixPath("/foo/foo.cpp").replace_extension(""));
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, pathSegmentIterator) {
  namespace pd = path_detail;
  {
    pd::PathSegmentIterator iter("");
    ASSERT_EQ(iter.end(), iter.next());
  }  // namespace pd=path_detail;
  {
    pd::PathSegmentIterator iter("//foo/foo/.././bar/foobar.txt");
    ASSERT_EQ((pd::PathSegment{StringView{"//foo"}, pd::PathSection::RootName}), iter.begin());
    ASSERT_EQ((pd::PathSegment{StringView{"/"}, pd::PathSection::RootDir}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{"foo"}, pd::PathSection::Filename}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{"."}, pd::PathSection::Dot}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{"bar"}, pd::PathSection::Filename}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{"foobar.txt"}, pd::PathSection::Filename}), iter.next());
    ASSERT_EQ(iter.end(), iter.next());
    // Ensure we can call again with the same answer
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("//");
    ASSERT_EQ((pd::PathSegment{StringView{"/"}, pd::PathSection::RootDir}), iter.begin());
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("/foo/");
    ASSERT_EQ((pd::PathSegment{StringView{"/"}, pd::PathSection::RootDir}), iter.begin());
    ASSERT_EQ((pd::PathSegment{StringView{"foo"}, pd::PathSection::Filename}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{"/"}, pd::PathSection::FinalSep}), iter.next());
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("foo");
    ASSERT_EQ((pd::PathSegment{StringView{"foo"}, pd::PathSection::Filename}), iter.begin());
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("foo/");
    ASSERT_EQ((pd::PathSegment{StringView{"foo"}, pd::PathSection::Filename}), iter.begin());
    ASSERT_EQ((pd::PathSegment{StringView{"/"}, pd::PathSection::FinalSep}), iter.next());
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("../../..");
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.begin());
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.next());
    ASSERT_EQ(iter.end(), iter.next());
  }
  {
    pd::PathSegmentIterator iter("././../..");
    ASSERT_EQ((pd::PathSegment{StringView{"."}, pd::PathSection::Dot}), iter.begin());
    ASSERT_EQ((pd::PathSegment{StringView{"."}, pd::PathSection::Dot}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.next());
    ASSERT_EQ((pd::PathSegment{StringView{".."}, pd::PathSection::DotDot}), iter.next());
    ASSERT_EQ(iter.end(), iter.next());
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, lexicallyNormal) {
  ASSERT_EQ("foo", PosixPath("./foo").lexically_normal());
  ASSERT_EQ("foo", PosixPath("././foo").lexically_normal());
  ASSERT_EQ("foo/", PosixPath("foo/").lexically_normal());
  ASSERT_EQ("foo/", PosixPath("foo//").lexically_normal());
  ASSERT_EQ(".", PosixPath("./.").lexically_normal());
  ASSERT_EQ(".", PosixPath("././").lexically_normal());
  ASSERT_EQ("", PosixPath("").lexically_normal());
  ASSERT_EQ(".", PosixPath(".").lexically_normal());
  ASSERT_EQ(".", PosixPath("./").lexically_normal());
  ASSERT_EQ("/", PosixPath("/").lexically_normal());
  ASSERT_EQ("/", PosixPath("/.").lexically_normal());
  ASSERT_EQ("/foo/bar", PosixPath("/foo/bar").lexically_normal());
  ASSERT_EQ("/foo/bar/", PosixPath("/foo/bar/").lexically_normal());
  ASSERT_EQ("/foo/bar/", PosixPath("/foo/bar/.").lexically_normal());
  ASSERT_EQ("/foo/foo", PosixPath("/foo/bar/../bar/.././foo").lexically_normal());
  ASSERT_EQ("/foo/bar/foo", PosixPath("/foo/bar/../bar/../bar/foo").lexically_normal());

  ASSERT_EQ("//C:/bar/foo", PosixPath("//C:/bar/foo").lexically_normal());
  ASSERT_EQ("//hello/bar/foo", PosixPath("//hello/bar/foo").lexically_normal());
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, iterator) {
  auto path = PosixPath("/foo/bar/foobar");
  {
    auto iter = path.begin();
    ASSERT_NE(path.end(), iter);
    ASSERT_EQ("/", *iter);
    ASSERT_EQ("/", *iter++);
    ASSERT_EQ("foo", *iter++);
    --iter;
    ++iter;
    ASSERT_EQ("bar", *iter++);
    ASSERT_EQ("foobar", *iter++);
    ASSERT_EQ(path.end(), iter);
    --iter;
    ASSERT_NE(path.end(), iter);
    ASSERT_EQ("foobar", *iter--);
    ASSERT_EQ("bar", *iter--);
    ASSERT_EQ("foo", *iter--);
    ASSERT_EQ("/", *iter);
    ASSERT_EQ(path.begin(), iter);
  }
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, windowsConversion) {
  ASSERT_EQ(L"foobar", toWin32(PosixPath("foobar")));
  ASSERT_EQ(L"foo\\bar", toWin32(PosixPath("foo/bar")));
  ASSERT_EQ(L"c:\\foo\\bar", toWin32(PosixPath("//c:/foo/bar")));
  ASSERT_EQ(L"c:\\", toWin32(PosixPath("//c:/")));
  ASSERT_EQ(L"c:", toWin32(PosixPath("//c:")));
  ASSERT_EQ(L"\\\\net.name.lan\\foo\\bar", toWin32(PosixPath("//net.name.lan/foo/bar")));

  ASSERT_EQ("foobar", fromWin32(L"foobar"));
  ASSERT_EQ("foo/bar", fromWin32(L"foo\\bar"));
  ASSERT_EQ("//c:/foo/bar", fromWin32(L"c:\\foo\\bar"));
  ASSERT_EQ("//c:/", fromWin32(L"c:\\"));
  ASSERT_EQ("//c:", fromWin32(L"c:"));
  ASSERT_EQ("//net.name.lan/foo/bar", fromWin32(L"\\\\net.name.lan\\foo\\bar"));
}

////////////////////////////////////////////////////////////////////////////////
TEST(PosixPathTest, osConvert) {
#if SW_POSIX
  ASSERT_EQ(PosixPath("/foo/bar"), fromOsNative("/foo/bar"));
  ASSERT_EQ("/foo/bar", PosixPath("/foo/bar").native());
#elif SW_WINDOWS
  ASSERT_EQ(PosixPath("/foo/bar"), fromOsNative(L"\\foo\\bar"));
  ASSERT_EQ(L"/foo/bar", PosixPath("/foo/bar").native());
#endif
}

}  // namespace sw
