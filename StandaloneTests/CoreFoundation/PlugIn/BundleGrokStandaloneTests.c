//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/BundleGrokStandaloneTests.h"
#include "CoreFoundation/PlugIn/OCFBundleGrok.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static int checkType(const char *actual, const char *expected, int line) {
    if (!actual || strcmp(actual, expected) != 0) {
        fprintf(
            stderr,
            "Bundle grok standalone mismatch at source line %d: %s, expected %s.\n",
            line,
            actual ? actual : "(null)",
            expected
        );
        return 1;
    }
    return 0;
}

static int checkValue(uint64_t actual, uint64_t expected, int line) {
    if (actual != expected) {
        fprintf(
            stderr,
            "Bundle grok standalone mismatch at source line %d: %llx, expected %llx.\n",
            line,
            (unsigned long long)actual,
            (unsigned long long)expected
        );
        return 1;
    }
    return 0;
}

static void put16(uint8_t *bytes, uint16_t value) {
    memcpy(bytes, &value, sizeof(value));
}

static void put32(uint8_t *bytes, uint32_t value, int swapped) {
    if (swapped) {
        value = __builtin_bswap32(value);
    }
    memcpy(bytes, &value, sizeof(value));
}

static void put64(uint8_t *bytes, uint64_t value) {
    memcpy(bytes, &value, sizeof(value));
}

// Writes a zip entry header and name; returns the size written.
static size_t putZipEntry(uint8_t *bytes, int local, const char *name, uint16_t method) {
    size_t length = strlen(name);
    size_t header = local ? 30 : 46;
    memset(bytes, 0, header);
    bytes[0] = 'P';
    bytes[1] = 'K';
    bytes[2] = local ? 3 : 1;
    bytes[3] = local ? 4 : 2;
    put16(bytes + 8, method);
    put16(bytes + (local ? 26 : 28), (uint16_t)length);
    memcpy(bytes + header, name, length);
    return header + length;
}

static const char *zipWithMimetype(const char *content, uint16_t method) {
    uint8_t bytes[256];
    memset(bytes, ' ', sizeof(bytes));
    size_t size = putZipEntry(bytes, 1, "mimetype", method);
    memcpy(bytes + size, content, strlen(content));
    return _CFBundleGrokFileTypeForZipFile(-1, bytes, sizeof(bytes), 0);
}

static const char *zipWithNames(const char *first, const char *second) {
    uint8_t bytes[512];
    memset(bytes, 0, sizeof(bytes));
    size_t size = putZipEntry(bytes, 0, first, 0);
    if (second) {
        putZipEntry(bytes + size + 7, 1, second, 8);
    }
    return _CFBundleGrokFileTypeForZipFile(-1, bytes, sizeof(bytes), 0);
}

static int runZipCases(size_t *cases) {
    static const struct {
        const char *content;
        const char *expected;
    } kMimetypes[] = {
        {"application/vnd.oasis.opendocument.spreadsheet", "ods"},
        {"application/vnd.oasis.opendocument.text-master", "odm"},
        {"application/vnd.oasis.opendocument.text", "odt"},
        {"application/x-vnd.oasis.opendocument.chart", "odc"},
        {"APPLICATION/VND.SUN.XML.WRITER.GLOBAL", "sxg"},
        {"application/vnd.sun.xml.writer", "sxw"},
        {"application/vnd.sun.xml.base", "zip"},
        {"application/epub+zip", "epub"},
        {"application/oebps-package+xml", "epub"},
    };

    for (size_t index = 0; index < sizeof(kMimetypes) / sizeof(kMimetypes[0]); ++index) {
        if (checkType(zipWithMimetype(kMimetypes[index].content, 0), kMimetypes[index].expected, __LINE__) !=
            0) {
            return 1;
        }
        ++*cases;
    }
    // A compressed mimetype entry skips the vnd. kinds; the deflated-text prefix only records "odt".
    if (checkType(zipWithMimetype("application/vnd.oasis.opendocument.text", 8), "zip", __LINE__) != 0 ||
        checkType(zipWithMimetype("\x4b\x2c\x28\xc8\xc9\x4c\x4e\x2c", 8), "odt", __LINE__) != 0 ||
        checkType(zipWithNames("[Content_Types].xml", "word/document.xml"), "docx", __LINE__) != 0 ||
        checkType(zipWithNames("_rels/.rels", "xl/workbook.xml"), "xlsx", __LINE__) != 0 ||
        checkType(zipWithNames("_rels/.rels", "powerpoint/a.xml"), "pptx", __LINE__) != 0 ||
        checkType(zipWithNames("word/document.xml", NULL), "zip", __LINE__) != 0 ||
        checkType(zipWithNames("META-INF/manifest.xml", "META-INF/MANIFEST.MF"), "jar", __LINE__) != 0 ||
        checkType(zipWithNames("content.xml", NULL), "odt", __LINE__) != 0 ||
        checkType(zipWithNames("META-INF/", NULL), "jar", __LINE__) != 0 ||
        checkType(zipWithNames("OEBPS/content.opf", "book.smil"), "dtb", __LINE__) != 0 ||
        checkType(zipWithNames("OEBPS/content.OPF", NULL), "oeb", __LINE__) != 0 ||
        checkType(_CFBundleGrokFileTypeForZipFile(-1, "PK\3\4", 4, 0), "zip", __LINE__) != 0) {
        return 1;
    }
    *cases += 12;
    return 0;
}

static int runZipFileCase(const char *directory, size_t *cases) {
    // The header buffer has no entries; the file's last 1,024 bytes hold the central directory.
    uint8_t file[3000];
    memset(file, 0, sizeof(file));
    size_t size = putZipEntry(file + 2200, 0, "[Content_Types].xml", 0);
    putZipEntry(file + 2200 + size, 0, "ppt/presentation.xml", 0);
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/zip", directory);
    int descriptor = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (descriptor < 0 || write(descriptor, file, sizeof(file)) != (ssize_t)sizeof(file)) {
        fputs("Setup failure: cannot write the zip fixture.\n", stderr);
        if (descriptor >= 0) {
            close(descriptor);
        }
        return 77;
    }
    const char *withFile = _CFBundleGrokFileTypeForZipFile(descriptor, file, 64, sizeof(file));
    const char *shortFile = _CFBundleGrokFileTypeForZipFile(descriptor, file, 64, sizeof(file) + 1);
    close(descriptor);
    unlink(path);
    if (checkType(withFile, "pptx", __LINE__) != 0 || checkType(shortFile, "zip", __LINE__) != 0) {
        return 1;
    }
    *cases += 2;
    return 0;
}

static void putOLEName(uint8_t *entry, const char *name, uint8_t highByte) {
    size_t length = strlen(name);
    for (size_t index = 0; index < length; ++index) {
        entry[index * 2] = (uint8_t)name[index];
        entry[index * 2 + 1] = highByte;
    }
    entry[64] = (uint8_t)((length + 1) * 2);
}

static int runOLECases(size_t *cases) {
    uint8_t bytes[1024];
    memset(bytes, 0, sizeof(bytes));
    putOLEName(bytes + 512, "Root Entry", 0);
    putOLEName(bytes + 512 + 256, "WordDocument", 0);
    const char *word = _CFBundleGrokFileTypeForOLEFile(-1, bytes, sizeof(bytes), 512);
    const char *outside = _CFBundleGrokFileTypeForOLEFile(-1, bytes, sizeof(bytes), 513);
    putOLEName(bytes + 512 + 128, "PowerPoint Document", 0x7f);
    const char *powerPoint = _CFBundleGrokFileTypeForOLEFile(-1, bytes, sizeof(bytes), 512);
    memset(bytes + 512, 0, 512);
    putOLEName(bytes + 512 + 384, "Workbook", 0);
    bytes[512 + 384 + 64] = 19;
    const char *workbook = _CFBundleGrokFileTypeForOLEFile(-1, bytes, sizeof(bytes), 512);
    bytes[512 + 384 + 64] = 0x80 | 18;
    const char *negative = _CFBundleGrokFileTypeForOLEFile(-1, bytes, sizeof(bytes), 512);
    if (checkType(word, "doc", __LINE__) != 0 || checkType(outside, "ole", __LINE__) != 0 ||
        checkType(powerPoint, "ppt", __LINE__) != 0 || checkType(workbook, "xls", __LINE__) != 0 ||
        checkType(negative, "ole", __LINE__) != 0) {
        return 1;
    }
    *cases += 5;
    return 0;
}

// Writes a Mach-O header; the load commands follow at 28 or 32.
static size_t putMachHeader(uint8_t *bytes, int swapped, int sixtyFour, uint32_t commands, uint32_t size) {
    memset(bytes, 0, 32);
    put32(bytes, sixtyFour ? 0xfeedfacf : 0xfeedface, swapped);
    put32(bytes + 16, commands, swapped);
    put32(bytes + 20, size, swapped);
    return sixtyFour ? 32 : 28;
}

static size_t putDylib(uint8_t *bytes, int swapped, const char *name) {
    size_t length = 24 + ((strlen(name) + 8) & ~(size_t)7);
    memset(bytes, 0, length);
    put32(bytes, 0xc, swapped);
    put32(bytes + 4, (uint32_t)length, swapped);
    put32(bytes + 8, 24, swapped);
    memcpy(bytes + 24, name, strlen(name));
    return length;
}

static int runX11Cases(size_t *cases) {
    static uint8_t bytes[8192];
    int results[5];
    for (int swapped = 0; swapped < 2; ++swapped) {
        memset(bytes, 0, sizeof(bytes));
        size_t cursor = putMachHeader(bytes, swapped, !swapped, 2, 0);
        size_t first = putDylib(bytes + cursor, swapped, "/usr/lib/libSystem.B.dylib");
        size_t second = putDylib(bytes + cursor + first, swapped, "/usr/X11R6/lib/libX11.6.dylib");
        put32(bytes + 20, (uint32_t)(first + second), swapped);
        results[swapped] =
            _CFBundleGrokX11FromFile(-1, bytes, sizeof(bytes), 0, (uint8_t)swapped, (uint8_t)!swapped);
        // The name must have 20 bytes before the end of the commands.
        put32(bytes + 20, (uint32_t)(first + 24 + 19), swapped);
        results[2 + swapped] =
            _CFBundleGrokX11FromFile(-1, bytes, sizeof(bytes), 0, (uint8_t)swapped, (uint8_t)!swapped);
    }
    results[4] = _CFBundleGrokX11FromFile(-1, bytes, 4095, 0, 1, 0);
    if (checkValue((uint64_t)results[0], 1, __LINE__) != 0 ||
        checkValue((uint64_t)results[1], 1, __LINE__) != 0 ||
        checkValue((uint64_t)results[2], 0, __LINE__) != 0 ||
        checkValue((uint64_t)results[3], 0, __LINE__) != 0 ||
        checkValue((uint64_t)results[4], 0, __LINE__) != 0) {
        return 1;
    }
    *cases += 5;
    return 0;
}

// Writes a segment command with one section; returns its size.
static size_t putSegment(
    uint8_t *bytes, int sixtyFour, const char *segment, const char *section, uint64_t size, uint32_t offset
) {
    size_t length = sixtyFour ? 72 + 80 : 56 + 68;
    memset(bytes, 0, length);
    put32(bytes, sixtyFour ? 0x19 : 1, 0);
    put32(bytes + 4, (uint32_t)length, 0);
    put32(bytes + (sixtyFour ? 64 : 48), 1, 0);
    uint8_t *header = bytes + (sixtyFour ? 72 : 56);
    strncpy((char *)header, section, 16);
    strncpy((char *)header + 16, segment, 16);
    if (sixtyFour) {
        put64(header + 40, size);
        put32(header + 48, offset, 0);
    } else {
        put32(header + 36, (uint32_t)size, 0);
        put32(header + 40, offset, 0);
    }
    return length;
}

static int checkImageInfo(
    const uint8_t *bytes,
    size_t length,
    uint32_t offset,
    int sixtyFour,
    uint8_t found,
    uint32_t first,
    uint32_t second,
    int line
) {
    uint8_t actualFound = 0xa5;
    uint32_t actualFirst = 0xa5a5a5a5;
    uint32_t actualSecond = 0xa5a5a5a5;
    _CFBundleGrokObjcImageInfoFromFile(
        -1, bytes, length, offset, 0, (uint8_t)sixtyFour, &actualFound, &actualFirst, &actualSecond
    );
    if (checkValue(actualFound, found, line) != 0 || checkValue(actualFirst, first, line) != 0 ||
        checkValue(actualSecond, second, line) != 0) {
        return 1;
    }
    return 0;
}

static int runImageInfoCases(size_t *cases) {
    static uint8_t bytes[8192];
    memset(bytes, 0, sizeof(bytes));
    size_t cursor = putMachHeader(bytes, 0, 1, 1, 152);
    putSegment(bytes + cursor, 1, "__DATA", "__objc_imageinfo", 8, 0x1800);
    put32(bytes + 0x1800, 0, 0);
    put32(bytes + 0x1804, 0x40, 0);
    if (checkImageInfo(bytes, sizeof(bytes), 0, 1, 1, 0, 0x40, __LINE__) != 0) {
        return 1;
    }
    // The section offset is relative to the slice offset.
    memset(bytes, 0, sizeof(bytes));
    cursor = putMachHeader(bytes + 4096, 0, 1, 1, 152);
    putSegment(bytes + 4096 + cursor, 1, "__DATA", "__objc_imageinfo", 16, 0x400);
    put32(bytes + 4096 + 0x400, 0x11, 0);
    put32(bytes + 4096 + 0x404, 0x22, 0);
    if (checkImageInfo(bytes, sizeof(bytes), 4096, 1, 1, 0x11, 0x22, __LINE__) != 0) {
        return 1;
    }
    memset(bytes, 0, sizeof(bytes));
    cursor = putMachHeader(bytes, 0, 1, 1, 152);
    putSegment(bytes + cursor, 1, "__DATA", "__objc_imageinfo", UINT64_C(0x100000007), 0x1800);
    if (checkImageInfo(bytes, sizeof(bytes), 0, 1, 1, 0, 0, __LINE__) != 0) {
        return 1;
    }
    memset(bytes, 0, sizeof(bytes));
    cursor = putMachHeader(bytes, 0, 0, 1, 124);
    putSegment(bytes + cursor, 0, "__OBJC", "__image_info", 8, 0x1000);
    put32(bytes + 0x1000, 7, 0);
    put32(bytes + 0x1004, 9, 0);
    if (checkImageInfo(bytes, sizeof(bytes), 0, 0, 1, 7, 9, __LINE__) != 0 ||
        checkImageInfo(bytes, 0x1007, 0, 0, 1, 0, 0, __LINE__) != 0 ||
        checkImageInfo(bytes, 4095, 0, 0, 0, 0, 0, __LINE__) != 0) {
        return 1;
    }
    // NULL outputs are skipped.
    _CFBundleGrokObjcImageInfoFromFile(-1, bytes, sizeof(bytes), 0, 0, 0, NULL, NULL, NULL);
    *cases += 7;
    return 0;
}

// A 4,096-byte window that ends where an inaccessible page begins.
typedef struct {
    uint8_t *mapping;
    size_t mappingLength;
    uint8_t *window;
} GuardedWindow;

static int openGuardedWindow(GuardedWindow *guarded) {
    size_t page = (size_t)getpagesize();
    guarded->mappingLength = 2 * page;
    void *mapping = mmap(NULL, guarded->mappingLength, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mapping == MAP_FAILED) {
        fputs("Setup failure: cannot map the guarded window.\n", stderr);
        return 77;
    }
    guarded->mapping = (uint8_t *)mapping;
    if (mprotect(guarded->mapping + page, page, PROT_NONE) != 0) {
        munmap(mapping, guarded->mappingLength);
        fputs("Setup failure: cannot protect the guard page.\n", stderr);
        return 77;
    }
    guarded->window = guarded->mapping + page - 4096;
    return 0;
}

// The native code reads past these windows (CoreFoundation defect); the restoration must stay inside
// them, which the guard page enforces.
static int runDefectCases(size_t *cases) {
    GuardedWindow guarded;
    int status = openGuardedWindow(&guarded);
    if (status != 0) {
        return status;
    }
    uint8_t *window = guarded.window;
    // A second load command two bytes before the end: cmd would cross the guard page.
    memset(window, 0, 4096);
    size_t cursor = putMachHeader(window, 0, 1, 3, UINT32_MAX);
    putDylib(window + cursor, 0, "/usr/lib/libSystem.B.dylib");
    put32(window + cursor + 4, (uint32_t)(4096 - 2 - cursor), 0);
    uint8_t tail = _CFBundleGrokX11FromFile(-1, window, 4096, 0, 0, 1);
    // An LC_LOAD_DYLIB whose name offset would cross: cmd fits, the name offset does not.
    put32(window + cursor + 4, (uint32_t)(4096 - 8 - cursor), 0);
    put32(window + 4096 - 8, 0xc, 0);
    put32(window + 4096 - 4, 0, 0);
    uint8_t nameOffset = _CFBundleGrokX11FromFile(-1, window, 4096, 0, 0, 1);
    // The wrapped window check: (uint32_t)(offset + 4096) is 0x800.
    uint8_t wrapped = _CFBundleGrokX11FromFile(-1, window, 4096, UINT32_C(0xfffff800), 0, 1);
    if (checkValue(tail, 0, __LINE__) != 0 || checkValue(nameOffset, 0, __LINE__) != 0 ||
        checkValue(wrapped, 0, __LINE__) != 0) {
        munmap(guarded.mapping, guarded.mappingLength);
        return 1;
    }
    // A segment whose only section starts 23 bytes before the end: both names are decided by readable
    // bytes ("__DATA" and its NUL), so the segment is found; the size field is not readable, so no
    // section matches.
    memset(window, 0, 4096);
    cursor = putMachHeader(window, 0, 1, 2, UINT32_MAX);
    size_t sectionStart = 4096 - 23;
    size_t commandStart = sectionStart - 72;
    put32(window + cursor, 2, 0);
    put32(window + cursor + 4, (uint32_t)(commandStart - cursor), 0);
    put32(window + commandStart, 0x19, 0);
    put32(window + commandStart + 4, 152, 0);
    put32(window + commandStart + 64, 1, 0);
    memcpy(window + sectionStart, "__objc_imageinfo", 16);
    memcpy(window + sectionStart + 16, "__DATA", 7);
    status = checkImageInfo(window, 4096, 0, 1, 1, 0, 0, __LINE__);
    // Only four bytes of the segment name are readable and equal: the comparison is undecided and
    // counts as unequal, so the segment is not found and no section matches.
    memset(window + commandStart, 0, 4096 - commandStart);
    sectionStart = 4096 - 20;
    commandStart = sectionStart - 72;
    put32(window + cursor + 4, (uint32_t)(commandStart - cursor), 0);
    put32(window + commandStart, 0x19, 0);
    put32(window + commandStart + 4, 152, 0);
    put32(window + commandStart + 64, 1, 0);
    memcpy(window + sectionStart, "__objc_imageinfo", 16);
    memcpy(window + sectionStart + 16, "__DA", 4);
    if (status == 0) {
        status = checkImageInfo(window, 4096, 0, 1, 0, 0, 0, __LINE__);
    }
    // Section 48 bytes before the end: names fit, so the segment is found; its size field fits but the
    // offset field does not, so no section matches.
    memset(window + commandStart, 0, 4096 - commandStart);
    sectionStart = 4096 - 48;
    commandStart = sectionStart - 72;
    put32(window + cursor + 4, (uint32_t)(commandStart - cursor), 0);
    put32(window + commandStart, 0x19, 0);
    put32(window + commandStart + 4, 152, 0);
    put32(window + commandStart + 64, 1, 0);
    memcpy(window + sectionStart, "__objc_imageinfo", 16);
    memcpy(window + sectionStart + 16, "__DATA", 7);
    put64(window + sectionStart + 40, 8);
    if (status == 0) {
        status = checkImageInfo(window, 4096, 0, 1, 1, 0, 0, __LINE__);
    }
    // A matching section whose image-information check wraps: (uint32_t)(0 + 0xfffffff8 + 8) is 0.
    memset(window, 0, 4096);
    cursor = putMachHeader(window, 0, 1, 1, 152);
    putSegment(window + cursor, 1, "__DATA", "__objc_imageinfo", 8, UINT32_C(0xfffffff8));
    if (status == 0) {
        status = checkImageInfo(window, 4096, 0, 1, 1, 0, 0, __LINE__);
    }
    // The wrapped window check: (uint32_t)(offset + 4096) is 0.
    if (status == 0) {
        status = checkImageInfo(window, 4096, UINT32_C(0xfffff000), 1, 0, 0, 0, __LINE__);
    }
    munmap(guarded.mapping, guarded.mappingLength);
    if (status != 0) {
        return status;
    }
    *cases += 8;
    return 0;
}

static int openDirectory(char *directory, size_t size) {
    char base[PATH_MAX];
    size_t length = confstr(_CS_DARWIN_USER_TEMP_DIR, base, sizeof(base));
    if (length == 0 || length > sizeof(base)) {
        const char *environment = getenv("TMPDIR");
        snprintf(base, sizeof(base), "%s", environment && *environment ? environment : "/tmp/");
    }
    size_t baseLength = strlen(base);
    const char *separator = baseLength > 0 && base[baseLength - 1] == '/' ? "" : "/";
    snprintf(directory, size, "%s%sOpenFoundationGrokStandalone.XXXXXX", base, separator);
    if (!mkdtemp(directory)) {
        fputs("Setup failure: cannot create the grok fixture directory.\n", stderr);
        return 77;
    }
    return 0;
}

// Native _CFBundleGrokFileTypeForZipFile 0x1c025d7c4, _CFBundleGrokFileTypeForOLEFile 0x1c025e57c,
// _CFBundleGrokX11FromFile 0x1c025e7cc, _CFBundleGrokObjcImageInfoFromFile 0x1c025eedc.
// iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// Device comparison: 0x18077acd4, 0x18077bb0c, 0x18077bd98, 0x18077c060,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Expectations are derived from the reading; the defect cases run the restoration against windows that
// end at an inaccessible page and are never run against the native code.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
int runCoreFoundationBundleGrokStandaloneTests(void) {
    size_t cases = 0;
    char directory[PATH_MAX];
    int status = openDirectory(directory, sizeof(directory));
    if (status != 0) {
        return status;
    }
    status = runZipCases(&cases);
    if (status == 0) {
        status = runZipFileCase(directory, &cases);
    }
    rmdir(directory);
    if (status == 0) {
        status = runOLECases(&cases);
    }
    if (status == 0) {
        status = runX11Cases(&cases);
    }
    if (status == 0) {
        status = runImageInfoCases(&cases);
    }
    if (status == 0) {
        status = runDefectCases(&cases);
    }
    if (status != 0) {
        return status;
    }
    printf("Bundle grok standalone cases passed: %zu.\n", cases);
    return 0;
}
