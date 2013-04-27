#include "StringTests.h"
#include "Data/String.h"
#include "Memory/Memory.h"
#include "Memory/RootSource.h"

namespace magpie
{
  void StringTests::runTests()
  {
    create();
    concat();
    subscript();
    equals();
    substring();
  }

  void StringTests::create()
  {
    gc<String> s1 = String::create("some text");
    gc<String> s2 = String::create("more");

    EXPECT_EQUAL(9, s1->length());
    EXPECT_EQUAL("some text", *s1);

    EXPECT_EQUAL(4, s2->length());
    EXPECT_EQUAL("more", *s2);
  }

  void StringTests::concat()
  {
    gc<String> s1 = String::create("first");
    gc<String> s2 = String::create("second");
    gc<String> result = String::concat(s1, s2);

    EXPECT_EQUAL("firstsecond", *result);
  }

  void StringTests::subscript()
  {
    gc<String> s = String::create("abcd");

    EXPECT_EQUAL('a', (*s)[0]);
    EXPECT_EQUAL('b', (*s)[1]);
    EXPECT_EQUAL('c', (*s)[2]);
    EXPECT_EQUAL('d', (*s)[3]);
  }

  void StringTests::equals()
  {
    gc<String> s = String::create("something");
    gc<String> same = String::create("something");
    gc<String> different = String::create("different");

    // String to C-string.
    EXPECT(*s == "something");
    EXPECT_FALSE(*s != "something");
    EXPECT_FALSE(*s == "else");
    EXPECT(*s != "else");

    // String to String.
    EXPECT(*s == *same);
    EXPECT_FALSE(*s != *same);
    EXPECT_FALSE(*s == *different);
    EXPECT(*s != *different);

    // C-string to String.
    EXPECT("something" == *s);
    EXPECT_FALSE("something" != *s);
    EXPECT_FALSE("else" == *s);
    EXPECT("else" != *s);

    // Make sure substrings don't appear equal.
    gc<String> s2 = String::create("abc");

    EXPECT_FALSE("abcd" == *s2);
    EXPECT_FALSE(*s2 == "abcd");
    EXPECT_FALSE("ab" == *s2);
    EXPECT_FALSE(*s2 == "ab");
  }

  void StringTests::substring()
  {
    gc<String> s = String::create("abcdef");

    // Zero-length.
    gc<String> sub = s->substring(3, 3);
    EXPECT_EQUAL("", *sub);

    // From beginning.
    sub = s->substring(0, 2);
    EXPECT_EQUAL("ab", *sub);

    // In middle.
    sub = s->substring(2, 5);
    EXPECT_EQUAL("cde", *sub);

    // To end.
    sub = s->substring(4, 6);
    EXPECT_EQUAL("ef", *sub);
  }
}

