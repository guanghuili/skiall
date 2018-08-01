/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkRandom.h"
#include "SkRefCnt.h"
#include "SkTSearch.h"
#include "SkTSort.h"
#include "SkUtils.h"
#include "Test.h"

class RefClass : public SkRefCnt {
public:


    RefClass(int n) : fN(n) {}
    int get() const { return fN; }

private:
    int fN;

    typedef SkRefCnt INHERITED;
};

static void test_autounref(skiatest::Reporter* reporter) {
    RefClass obj(0);
    REPORTER_ASSERT(reporter, obj.unique());

    sk_sp<RefClass> tmp(&obj);
    REPORTER_ASSERT(reporter, &obj == tmp.get());
    REPORTER_ASSERT(reporter, obj.unique());

    REPORTER_ASSERT(reporter, &obj == tmp.release());
    REPORTER_ASSERT(reporter, obj.unique());
    REPORTER_ASSERT(reporter, nullptr == tmp.release());
    REPORTER_ASSERT(reporter, nullptr == tmp.get());

    obj.ref();
    REPORTER_ASSERT(reporter, !obj.unique());
    {
        sk_sp<RefClass> tmp2(&obj);
    }
    REPORTER_ASSERT(reporter, obj.unique());
}

static void test_autostarray(skiatest::Reporter* reporter) {
    RefClass obj0(0);
    RefClass obj1(1);
    REPORTER_ASSERT(reporter, obj0.unique());
    REPORTER_ASSERT(reporter, obj1.unique());

    {
        SkAutoSTArray<2, sk_sp<RefClass> > tmp;
        REPORTER_ASSERT(reporter, 0 == tmp.count());

        tmp.reset(0);   // test out reset(0) when already at 0
        tmp.reset(4);   // this should force a new allocation
        REPORTER_ASSERT(reporter, 4 == tmp.count());
        tmp[0].reset(SkRef(&obj0));
        tmp[1].reset(SkRef(&obj1));
        REPORTER_ASSERT(reporter, !obj0.unique());
        REPORTER_ASSERT(reporter, !obj1.unique());

        // test out reset with data in the array (and a new allocation)
        tmp.reset(0);
        REPORTER_ASSERT(reporter, 0 == tmp.count());
        REPORTER_ASSERT(reporter, obj0.unique());
        REPORTER_ASSERT(reporter, obj1.unique());

        tmp.reset(2);   // this should use the preexisting allocation
        REPORTER_ASSERT(reporter, 2 == tmp.count());
        tmp[0].reset(SkRef(&obj0));
        tmp[1].reset(SkRef(&obj1));
    }

    // test out destructor with data in the array (and using existing allocation)
    REPORTER_ASSERT(reporter, obj0.unique());
    REPORTER_ASSERT(reporter, obj1.unique());

    {
        // test out allocating ctor (this should allocate new memory)
        SkAutoSTArray<2, sk_sp<RefClass> > tmp(4);
        REPORTER_ASSERT(reporter, 4 == tmp.count());

        tmp[0].reset(SkRef(&obj0));
        tmp[1].reset(SkRef(&obj1));
        REPORTER_ASSERT(reporter, !obj0.unique());
        REPORTER_ASSERT(reporter, !obj1.unique());

        // Test out resut with data in the array and malloced storage
        tmp.reset(0);
        REPORTER_ASSERT(reporter, obj0.unique());
        REPORTER_ASSERT(reporter, obj1.unique());

        tmp.reset(2);   // this should use the preexisting storage
        tmp[0].reset(SkRef(&obj0));
        tmp[1].reset(SkRef(&obj1));
        REPORTER_ASSERT(reporter, !obj0.unique());
        REPORTER_ASSERT(reporter, !obj1.unique());

        tmp.reset(4);   // this should force a new malloc
        REPORTER_ASSERT(reporter, obj0.unique());
        REPORTER_ASSERT(reporter, obj1.unique());

        tmp[0].reset(SkRef(&obj0));
        tmp[1].reset(SkRef(&obj1));
        REPORTER_ASSERT(reporter, !obj0.unique());
        REPORTER_ASSERT(reporter, !obj1.unique());
    }

    REPORTER_ASSERT(reporter, obj0.unique());
    REPORTER_ASSERT(reporter, obj1.unique());
}

/////////////////////////////////////////////////////////////////////////////

#define kSEARCH_COUNT   91

static void test_search(skiatest::Reporter* reporter) {
    int         i, array[kSEARCH_COUNT];
    SkRandom    rand;

    for (i = 0; i < kSEARCH_COUNT; i++) {
        array[i] = rand.nextS();
    }

    SkTHeapSort<int>(array, kSEARCH_COUNT);
    // make sure we got sorted properly
    for (i = 1; i < kSEARCH_COUNT; i++) {
        REPORTER_ASSERT(reporter, array[i-1] <= array[i]);
    }

    // make sure we can find all of our values
    for (i = 0; i < kSEARCH_COUNT; i++) {
        int index = SkTSearch<int>(array, kSEARCH_COUNT, array[i], sizeof(int));
        REPORTER_ASSERT(reporter, index == i);
    }

    // make sure that random values are either found, or the correct
    // insertion index is returned
    for (i = 0; i < 10000; i++) {
        int value = rand.nextS();
        int index = SkTSearch<int>(array, kSEARCH_COUNT, value, sizeof(int));

        if (index >= 0) {
            REPORTER_ASSERT(reporter,
                            index < kSEARCH_COUNT && array[index] == value);
        } else {
            index = ~index;
            REPORTER_ASSERT(reporter, index <= kSEARCH_COUNT);
            if (index < kSEARCH_COUNT) {
                REPORTER_ASSERT(reporter, value < array[index]);
                if (index > 0) {
                    REPORTER_ASSERT(reporter, value > array[index - 1]);
                }
            } else {
                // we should append the new value
                REPORTER_ASSERT(reporter, value > array[kSEARCH_COUNT - 1]);
            }
        }
    }
}

static void test_utf16(skiatest::Reporter* reporter) {
    // Test non-basic-multilingual-plane unicode.
    static const SkUnichar gUni[] = {
        0x10000, 0x18080, 0x20202, 0xFFFFF, 0x101234
    };
    for (SkUnichar uni : gUni) {
        uint16_t buf[2];
        size_t count = SkUTF::ToUTF16(uni, buf);
        REPORTER_ASSERT(reporter, count == 2);
        size_t count2 = SkUTF::CountUTF16(buf, sizeof(buf));
        REPORTER_ASSERT(reporter, count2 == 1);
        const uint16_t* ptr = buf;
        SkUnichar c = SkUTF::NextUTF16(&ptr, buf + SK_ARRAY_COUNT(buf));
        REPORTER_ASSERT(reporter, c == uni);
        REPORTER_ASSERT(reporter, ptr - buf == 2);
    }
}

DEF_TEST(Utils, reporter) {
    static const struct {
        const char* fUtf8;
        SkUnichar   fUni;
    } gTest[] = {
        { "a",                  'a' },
        { "\x7f",               0x7f },
        { "\xC2\x80",           0x80 },
        { "\xC3\x83",           (3 << 6) | 3    },
        { "\xDF\xBF",           0x7ff },
        { "\xE0\xA0\x80",       0x800 },
        { "\xE0\xB0\xB8",       0xC38 },
        { "\xE3\x83\x83",       (3 << 12) | (3 << 6) | 3    },
        { "\xEF\xBF\xBF",       0xFFFF },
        { "\xF0\x90\x80\x80",   0x10000 },
        { "\xF3\x83\x83\x83",   (3 << 18) | (3 << 12) | (3 << 6) | 3    }
    };

    for (size_t i = 0; i < SK_ARRAY_COUNT(gTest); i++) {
        const char* p = gTest[i].fUtf8;
        const char* stop = p + strlen(p);
        int         n = SkUTF::CountUTF8(p, strlen(p));
        SkUnichar   u1 = SkUTF::NextUTF8(&p, stop);

        REPORTER_ASSERT(reporter, n == 1);
        REPORTER_ASSERT(reporter, u1 == gTest[i].fUni);
        REPORTER_ASSERT(reporter,
                        p - gTest[i].fUtf8 == (int)strlen(gTest[i].fUtf8));
    }

    test_utf16(reporter);
    test_search(reporter);
    test_autounref(reporter);
    test_autostarray(reporter);
}

#define ASCII_BYTE         "X"
#define CONTINUATION_BYTE  "\xA1"
#define LEADING_TWO_BYTE   "\xC2"
#define LEADING_THREE_BYTE "\xE1"
#define LEADING_FOUR_BYTE  "\xF0"
#define INVALID_BYTE       "\xFC"
DEF_TEST(SkUTF_CountUTF8, r) {
    struct {
        int expectedCount;
        const char* utf8String;
    } testCases[] = {
        { 0, "" },
        { 1, ASCII_BYTE },
        { 2, ASCII_BYTE ASCII_BYTE },
        { 1, LEADING_TWO_BYTE CONTINUATION_BYTE },
        { 2, ASCII_BYTE LEADING_TWO_BYTE CONTINUATION_BYTE },
        { 3, ASCII_BYTE ASCII_BYTE LEADING_TWO_BYTE CONTINUATION_BYTE },
        { 1, LEADING_THREE_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 2, ASCII_BYTE LEADING_THREE_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 3, ASCII_BYTE ASCII_BYTE LEADING_THREE_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 1, LEADING_FOUR_BYTE CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 2, ASCII_BYTE LEADING_FOUR_BYTE CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 3, ASCII_BYTE ASCII_BYTE LEADING_FOUR_BYTE CONTINUATION_BYTE CONTINUATION_BYTE
             CONTINUATION_BYTE },
        { -1, INVALID_BYTE },
        { -1, INVALID_BYTE CONTINUATION_BYTE },
        { -1, INVALID_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { -1, INVALID_BYTE CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { -1, LEADING_TWO_BYTE },
        { -1, CONTINUATION_BYTE },
        { -1, CONTINUATION_BYTE CONTINUATION_BYTE },
        { -1, LEADING_THREE_BYTE CONTINUATION_BYTE },
        { -1, CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { -1, LEADING_FOUR_BYTE CONTINUATION_BYTE },
        { -1, CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
    };
    for (auto testCase : testCases) {
        const char* str = testCase.utf8String;
        REPORTER_ASSERT(r, testCase.expectedCount == SkUTF::CountUTF8(str, strlen(str)));
    }
}

DEF_TEST(SkUTF_NextUTF8_ToUTF8, r) {
    struct {
        SkUnichar expected;
        const char* utf8String;
    } testCases[] = {
        { -1, INVALID_BYTE },
        { -1, "" },
        { 0x0058, ASCII_BYTE },
        { 0x00A1, LEADING_TWO_BYTE CONTINUATION_BYTE },
        { 0x1861, LEADING_THREE_BYTE CONTINUATION_BYTE CONTINUATION_BYTE },
        { 0x010330, LEADING_FOUR_BYTE "\x90\x8C\xB0" },
    };
    for (auto testCase : testCases) {
        const char* str = testCase.utf8String;
        SkUnichar uni = SkUTF::NextUTF8(&str, str + strlen(str));
        REPORTER_ASSERT(r, str == testCase.utf8String + strlen(testCase.utf8String));
        REPORTER_ASSERT(r, uni == testCase.expected);
        char buff[5] = {0, 0, 0, 0, 0};
        size_t len = SkUTF::ToUTF8(uni, buff);
        if (buff[len] != 0) {
            ERRORF(r, "unexpected write");
            continue;
        }
        if (uni == -1) {
            REPORTER_ASSERT(r, len == 0);
            continue;
        }
        if (len == 0) {
           ERRORF(r, "unexpected failure.");
           continue;
        }
        if (len > 4) {
           ERRORF(r, "wrote too much");
           continue;
        }
        str = testCase.utf8String;
        REPORTER_ASSERT(r, len == strlen(buff));
        REPORTER_ASSERT(r, len == strlen(str));
        REPORTER_ASSERT(r, 0 == strcmp(str, buff));
    }
}
#undef ASCII_BYTE
#undef CONTINUATION_BYTE
#undef LEADING_TWO_BYTE
#undef LEADING_THREE_BYTE
#undef LEADING_FOUR_BYTE
#undef INVALID_BYTE
