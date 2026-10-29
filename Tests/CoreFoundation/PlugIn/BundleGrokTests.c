//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/BundleGrokTests.h"
#include "CoreFoundation/PlugIn/OCFBundleGrok.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef const char *(*FileTypeFunction)(int, const void *, size_t, off_t);

typedef uint8_t (*X11Function)(int, const void *, size_t, uint32_t, uint8_t, uint8_t);

typedef void (*ImageInfoFunction)(
    int, const void *, size_t, uint32_t, uint8_t, uint8_t, uint8_t *, uint32_t *, uint32_t *
);

typedef struct {
    FileTypeFunction zip;
    FileTypeFunction ole;
    X11Function x11;
    ImageInfoFunction imageInfo;
} GrokFunctions;

enum {
    kFunctionCount = 4,
    kWindowSize = 4096,
    kDescriptorModes = 4,
};

// Descriptor modes: no descriptor (bytes only), the fixture file, an unseekable pipe (lseek fails, so
// the bytes are used) and a directory (positioning succeeds or fails, reading fails).
enum {
    kModeBytes = 0,
    kModeFile = 1,
    kModePipe = 2,
    kModeDirectory = 3,
};

typedef struct {
    GrokFunctions native;
    GrokFunctions replacement;
    char directory[PATH_MAX];
    char path[PATH_MAX];
    int pipeDescriptors[2];
    int directoryDescriptor;
    uint64_t state;
    size_t cases;
    size_t typed;
    size_t x11Found;
    size_t segmentsFound;
    size_t wordsRead;
} Fixture;

// Output area of _CFBundleGrokObjcImageInfoFromFile, compared byte for byte with canaries.
typedef struct {
    uint32_t before;
    uint8_t found;
    uint8_t padding[3];
    uint32_t first;
    uint32_t second;
    uint32_t after;
} ImageInfoArea;

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static uint64_t randomBelow(uint64_t *state, uint64_t bound) {
    return nextRandom(state) % bound;
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

static void put64(uint8_t *bytes, uint64_t value, int swapped) {
    if (swapped) {
        value = __builtin_bswap64(value);
    }
    memcpy(bytes, &value, sizeof(value));
}

static int writeFixture(Fixture *fixture, const uint8_t *bytes, size_t length) {
    int descriptor = open(fixture->path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (descriptor < 0) {
        fprintf(stderr, "Setup failure: cannot create grok fixture %s.\n", fixture->path);
        return 77;
    }
    size_t written = 0;
    while (written < length) {
        ssize_t count = write(descriptor, bytes + written, length - written);
        if (count <= 0) {
            close(descriptor);
            fputs("Setup failure: cannot write grok fixture.\n", stderr);
            return 77;
        }
        written += (size_t)count;
    }
    close(descriptor);
    return 0;
}

// Returns the descriptor for a mode; the file descriptor is opened fresh and closed by the caller.
static int descriptorFor(Fixture *fixture, int mode) {
    switch (mode) {
        case kModeFile:
            return open(fixture->path, O_RDONLY);
        case kModePipe:
            return fixture->pipeDescriptors[0];
        case kModeDirectory:
            return fixture->directoryDescriptor;
        default:
            return -1;
    }
}

static void releaseDescriptor(int mode, int descriptor) {
    if (mode == kModeFile && descriptor >= 0) {
        close(descriptor);
    }
}

static int compareFileType(
    Fixture *fixture,
    const char *suite,
    FileTypeFunction native,
    FileTypeFunction replacement,
    int mode,
    const void *bytes,
    size_t length,
    off_t argument
) {
    int descriptor = descriptorFor(fixture, mode);
    if (mode == kModeFile && descriptor < 0) {
        fputs("Setup failure: cannot open grok fixture.\n", stderr);
        return 77;
    }
    const char *expected = native(descriptor, bytes, length, argument);
    const char *actual = replacement(descriptor, bytes, length, argument);
    releaseDescriptor(mode, descriptor);
    if (!expected || !actual || strcmp(expected, actual) != 0) {
        fprintf(
            stderr,
            "%s mismatch: native %s, replacement %s, mode %d, length %zu, argument %lld.\n",
            suite,
            expected ? expected : "(null)",
            actual ? actual : "(null)",
            mode,
            length,
            (long long)argument
        );
        return 1;
    }
    if (strcmp(actual, suite[0] == 'Z' ? "zip" : "ole") != 0) {
        ++fixture->typed;
    }
    ++fixture->cases;
    return 0;
}

static int compareMachO(
    Fixture *fixture,
    int mode,
    const void *bytes,
    size_t length,
    uint32_t offset,
    uint8_t swapped,
    uint8_t sixtyFour
) {
    int descriptor = descriptorFor(fixture, mode);
    if (mode == kModeFile && descriptor < 0) {
        fputs("Setup failure: cannot open grok fixture.\n", stderr);
        return 77;
    }
    uint8_t expectedX11 = fixture->native.x11(descriptor, bytes, length, offset, swapped, sixtyFour);
    uint8_t actualX11 = fixture->replacement.x11(descriptor, bytes, length, offset, swapped, sixtyFour);
    unsigned mask = (unsigned)randomBelow(&fixture->state, 16);
    mask = mask >= 8 ? 7 : mask;
    ImageInfoArea expected;
    ImageInfoArea actual;
    memset(&expected, 0xa5, sizeof(expected));
    memset(&actual, 0xa5, sizeof(actual));
    fixture->native.imageInfo(
        descriptor,
        bytes,
        length,
        offset,
        swapped,
        sixtyFour,
        (mask & 1) ? &expected.found : NULL,
        (mask & 2) ? &expected.first : NULL,
        (mask & 4) ? &expected.second : NULL
    );
    fixture->replacement.imageInfo(
        descriptor,
        bytes,
        length,
        offset,
        swapped,
        sixtyFour,
        (mask & 1) ? &actual.found : NULL,
        (mask & 2) ? &actual.first : NULL,
        (mask & 4) ? &actual.second : NULL
    );
    releaseDescriptor(mode, descriptor);
    if (expectedX11 != actualX11) {
        fprintf(
            stderr,
            "X11 mismatch: native %u, replacement %u, mode %d, length %zu, offset %u, swapped %u, 64 %u.\n",
            expectedX11,
            actualX11,
            mode,
            length,
            offset,
            swapped,
            sixtyFour
        );
        return 1;
    }
    if (memcmp(&expected, &actual, sizeof(expected)) != 0) {
        fprintf(
            stderr,
            "Image-information mismatch: native %u %08x %08x, replacement %u %08x %08x, mode %d, length "
            "%zu, offset %u, swapped %u, 64 %u, mask %u.\n",
            expected.found,
            expected.first,
            expected.second,
            actual.found,
            actual.first,
            actual.second,
            mode,
            length,
            offset,
            swapped,
            sixtyFour,
            mask
        );
        return 1;
    }
    fixture->x11Found += actualX11 != 0;
    fixture->segmentsFound += (mask & 1) && actual.found != 0;
    fixture->wordsRead += ((mask & 2) && actual.first != 0) || ((mask & 4) && actual.second != 0);
    fixture->cases += 2;
    return 0;
}

// Zip entry names: every name the code compares, prefix/suffix forms at each length boundary, near
// misses and arbitrary names.
static const char *const kZipNames[] = {
    "mimetype",
    "MIMETYPE",
    "mimetypf",
    "META-INF/",
    "meta-inf/",
    "META-INF.",
    "content.xml",
    "CONTENT.XML",
    "_rels/.rels",
    "[Content_Types].xml",
    "META-INF/MANIFEST.MF",
    "meta-inf/manifest.mf",
    "META-INF/manifest.xml",
    "META-INF/container.xml",
    "OEBPS/content.opf",
    "a.opf",
    ".opf",
    "book.sml",
    "x.smil",
    "a.SMIL",
    "xl/workbook.xml",
    "xl/.xml",
    "xl/a.xm",
    "XL/A.XML",
    "ppt/.xml",
    "ppt/slides/slide1.xml",
    "word/.xml",
    "word/document.xml",
    "excel/.xml",
    "excel/sheet1.xml",
    "powerpoint/.xml",
    "powerpoint/a.xml",
    "powerpoint/slide.xmm",
    "styles.xml",
    "ab",
    "abcd",
    "abcde",
    "",
};

// Mimetype contents: every kind the code compares, unknown kinds and case variants.
static const char *const kZipContents[] = {
    "application/vnd.oasis.opendocument.chart",
    "application/vnd.oasis.opendocument.formula",
    "application/vnd.oasis.opendocument.graphics",
    "application/vnd.oasis.opendocument.text-web",
    "application/vnd.oasis.opendocument.image",
    "application/vnd.oasis.opendocument.text-master",
    "application/vnd.oasis.opendocument.presentation",
    "application/vnd.oasis.opendocument.spreadsheet",
    "application/vnd.oasis.opendocument.text",
    "application/vnd.oasis.opendocument.database",
    "application/x-vnd.oasis.opendocument.text",
    "APPLICATION/VND.OASIS.OPENDOCUMENT.SPREADSHEET",
    "application/vnd.sun.xml.calc",
    "application/vnd.sun.xml.draw",
    "application/vnd.sun.xml.writer.global",
    "application/vnd.sun.xml.impress",
    "application/vnd.sun.xml.math",
    "application/vnd.sun.xml.writer",
    "application/vnd.sun.xml.base",
    "application/x-vnd.sun.xml.writer",
    "application/vnd.ms-excel",
    "application/epub+zip",
    "application/oebps-package+xml",
    "APPLICATION/EPUB+ZIP",
    "application/zip",
    "\x4b\x2c\x28\xc8\xc9\x4c\x4e\x2c",
    "\x4b\x2c\x28\xc8\xc9\x4c\x4e\x2d",
};

static void fillZipBackground(uint8_t *bytes, size_t length, uint64_t *state) {
    static const uint8_t kAlphabet[] = {0, 'P', 'K', 1, 2, 3, 4, 'a', '.', '/', 0xff};
    for (size_t index = 0; index < length; ++index) {
        uint64_t value = nextRandom(state);
        bytes[index] = (value & 3) == 0 ? (uint8_t)(value >> 8) : kAlphabet[(value >> 8) % sizeof(kAlphabet)];
    }
}

// Writes one local or central entry at position, clipped to the buffer.
static void writeZipEntry(uint8_t *bytes, size_t length, size_t position, uint64_t *state) {
    uint8_t entry[256];
    memset(entry, 0, sizeof(entry));
    int local = (nextRandom(state) & 1) != 0;
    const char *name = kZipNames[randomBelow(state, sizeof(kZipNames) / sizeof(kZipNames[0]))];
    int mimetype = local && randomBelow(state, 3) == 0;
    if (mimetype) {
        name = randomBelow(state, 8) == 0 ? "MimeType" : "mimetype";
    }
    size_t nameLength = strlen(name);
    size_t header = local ? 30 : 46;
    entry[0] = 'P';
    entry[1] = 'K';
    entry[2] = local ? 3 : 1;
    entry[3] = local ? 4 : 2;
    uint16_t nameField = (uint16_t)nameLength;
    uint64_t choice = randomBelow(state, 16);
    if (choice == 0) {
        nameField = (uint16_t)randomBelow(state, 64);
    } else if (choice == 1) {
        nameField = (uint16_t)nextRandom(state);
    }
    put16(entry + (local ? 26 : 28), nameField);
    uint16_t extra = (uint16_t)(randomBelow(state, 4) == 0 ? randomBelow(state, 12) : 0);
    if (local) {
        put16(entry + 28, extra);
        uint64_t methodChoice = randomBelow(state, 4);
        put16(entry + 8, methodChoice == 0 ? 8 : (methodChoice == 1 ? (uint16_t)nextRandom(state) : 0));
    } else {
        put16(entry + 26, (uint16_t)nextRandom(state));
        put16(entry + 30, (uint16_t)randomBelow(state, 4));
    }
    memcpy(entry + header, name, nameLength);
    size_t size = header + nameLength;
    if (local) {
        size += extra;
        const char *content =
            kZipContents[randomBelow(state, sizeof(kZipContents) / sizeof(kZipContents[0]))];
        size_t contentLength = strlen(content);
        memcpy(entry + size, content, contentLength);
        size += contentLength;
        size_t padding = randomBelow(state, 40);
        memset(entry + size, randomBelow(state, 2) == 0 ? ' ' : 0, padding);
        size += padding;
    }
    for (size_t index = 0; index < size && position + index < length; ++index) {
        bytes[position + index] = entry[index];
    }
}

static void buildZip(uint8_t *bytes, size_t length, uint64_t *state) {
    fillZipBackground(bytes, length, state);
    if (length == 0) {
        return;
    }
    size_t entries = (size_t)randomBelow(state, 7);
    for (size_t entry = 0; entry < entries; ++entry) {
        size_t position;
        uint64_t choice = randomBelow(state, 6);
        if (choice == 0 && length > 64) {
            position = length - 1 - (size_t)randomBelow(state, 64);
        } else if (choice == 1 && length > 1024) {
            position = length - 1024 + (size_t)randomBelow(state, 1024);
        } else {
            position = (size_t)randomBelow(state, length);
        }
        writeZipEntry(bytes, length, position, state);
    }
}

static const size_t kZipLengths[] = {0, 1, 30, 31, 32, 61, 100, 300, 1023, 1024, 1025, 1500, 2048, 4000};

static int runZipCases(Fixture *fixture) {
    uint8_t head[4096];
    uint8_t file[8192 + 4096];
    for (size_t iteration = 0; iteration < 12000; ++iteration) {
        size_t headLength =
            kZipLengths[randomBelow(&fixture->state, sizeof(kZipLengths) / sizeof(kZipLengths[0]))];
        buildZip(head, headLength, &fixture->state);
        // The file starts with the head; a separate tail holds the trailing window.
        size_t tailLength =
            (size_t)randomBelow(&fixture->state, 3) == 0 ? 0 : 1024 + randomBelow(&fixture->state, 1024);
        size_t gap = (size_t)randomBelow(&fixture->state, 2000);
        memcpy(file, head, headLength);
        fillZipBackground(file + headLength, gap, &fixture->state);
        buildZip(file + headLength + gap, tailLength, &fixture->state);
        size_t fileLength = headLength + gap + tailLength;
        int status = writeFixture(fixture, file, fileLength);
        if (status != 0) {
            return status;
        }
        off_t arguments[4] = {
            (off_t)fileLength,
            (off_t)headLength,
            (off_t)randomBelow(&fixture->state, 1024),
            (off_t)(fileLength + randomBelow(&fixture->state, 2048)),
        };
        for (int mode = 0; mode < kDescriptorModes; ++mode) {
            for (size_t argument = 0; argument < 4; ++argument) {
                if (argument >= 2 && mode != kModeFile && randomBelow(&fixture->state, 2) == 0) {
                    continue;
                }
                status = compareFileType(
                    fixture,
                    "Zip",
                    fixture->native.zip,
                    fixture->replacement.zip,
                    mode,
                    head,
                    headLength,
                    arguments[argument]
                );
                if (status != 0) {
                    return status;
                }
            }
        }
        if (headLength == 0) {
            int descriptor = -1;
            const char *expected = fixture->native.zip(descriptor, NULL, 0, 0);
            const char *actual = fixture->replacement.zip(descriptor, NULL, 0, 0);
            if (!expected || !actual || strcmp(expected, actual) != 0) {
                fputs("Zip mismatch for an empty buffer.\n", stderr);
                return 1;
            }
            ++fixture->cases;
        }
    }
    return 0;
}

static const char *const kOLENames[] = {
    "Book",
    "Workbook",
    "WordDocument",
    "PowerPoint Document",
    "Root Entry",
    "BOOK",
    "Workboo",
    "WordDocumenT",
    "PowerPoint Documenu",
    "",
};

static void writeOLEEntry(uint8_t *entry, uint64_t *state) {
    memset(entry, (int)randomBelow(state, 256), 128);
    if (randomBelow(state, 5) == 0) {
        return;
    }
    const char *name = kOLENames[randomBelow(state, sizeof(kOLENames) / sizeof(kOLENames[0]))];
    size_t length = strlen(name);
    for (size_t index = 0; index < length && index < 32; ++index) {
        entry[index * 2] = (uint8_t)name[index];
        entry[index * 2 + 1] = randomBelow(state, 4) == 0 ? (uint8_t)nextRandom(state) : 0;
    }
    if (length < 32) {
        entry[length * 2] = 0;
        entry[length * 2 + 1] = 0;
    }
    uint64_t choice = randomBelow(state, 8);
    if (choice == 0) {
        entry[64] = (uint8_t)nextRandom(state);
    } else if (choice == 1) {
        entry[64] = (uint8_t)((length + 1) * 2 + 1);
    } else if (choice == 2) {
        entry[64] = (uint8_t)(0x80 | (length + 1) * 2);
    } else {
        entry[64] = (uint8_t)((length + 1) * 2);
    }
    entry[65] = (uint8_t)nextRandom(state);
}

static const size_t kOLELengths[] = {0, 100, 511, 512, 513, 1024, 1536, 2048, 4096};

static int runOLECases(Fixture *fixture) {
    uint8_t bytes[4096];
    for (size_t iteration = 0; iteration < 6000; ++iteration) {
        size_t length =
            kOLELengths[randomBelow(&fixture->state, sizeof(kOLELengths) / sizeof(kOLELengths[0]))];
        for (size_t index = 0; index < length; ++index) {
            bytes[index] = (uint8_t)nextRandom(&fixture->state);
        }
        for (size_t entry = 0; entry + 128 <= length; entry += 128) {
            if (randomBelow(&fixture->state, 3) != 0) {
                writeOLEEntry(bytes + entry, &fixture->state);
            }
        }
        int status = writeFixture(fixture, bytes, length);
        if (status != 0) {
            return status;
        }
        off_t offsets[] = {
            0,
            128,
            512,
            1024,
            (off_t)length - 512,
            (off_t)length - 511,
            (off_t)length,
            (off_t)length + 512,
            (off_t)(128 * randomBelow(&fixture->state, 33)),
            (off_t)1 << 33,
        };
        for (int mode = 0; mode < kDescriptorModes; ++mode) {
            for (size_t index = 0; index < sizeof(offsets) / sizeof(offsets[0]); ++index) {
                if (offsets[index] < 0) {
                    continue;
                }
                status = compareFileType(
                    fixture,
                    "OLE",
                    fixture->native.ole,
                    fixture->replacement.ole,
                    mode,
                    bytes,
                    length,
                    offsets[index]
                );
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    return 0;
}

static const char *const kSegmentNames[] = {
    "__DATA",
    "__OBJC",
    "__DATA_CONST",
    "__TEXT",
    "__DAT",
    "__OBJC2",
    "__data",
    "",
};

static const char *const kSectionNames[] = {
    "__objc_imageinfo",
    "__image_info",
    "__objc_imageinf",
    "__image_info_",
    "__IMAGE_INFO",
    "__text",
    "__objc_classlist",
    "",
};

static const char *const kDylibNames[] = {
    "/usr/X11R6/lib/libX11.6.dylib",
    "/usr/X11R6/lib/libXext.dylib",
    "/usr/X11R6/lib/libX",
    "/usr/X11R6/lib/lib",
    "/usr/X11/lib/libX11.dylib",
    "/usr/lib/libSystem.B.dylib",
    "/USR/X11R6/LIB/LIBX11.DYLIB",
};

// Random words of a generated window keep every byte below 0x80 (and the fill is 0x6e), so no field that
// either implementation can read as a section offset makes offset + section offset + 8 wrap 32 bits.
static const uint32_t kCleanWord = UINT32_C(0x7f7f7f7f);

static const uint64_t kCleanDoubleWord = UINT64_C(0x7f7f7f7f7f7f7f7f);

// Builds a Mach-O load-command window. Bounded windows keep sizeofcmds at most 3,800, so every read of
// either implementation stays at least 140 bytes inside the 4,096-byte window, and allow arbitrary
// command sizes. Open windows set sizeofcmds above 4,064 (the end is the window end) and keep every
// command size exact and the commands below 3,100 bytes, so no scan approaches the window end. Neither
// kind reaches the native over-reads, which are documented and tested only in the standalone suite.
static void
buildMachO(uint8_t *window, uint64_t *state, int swapped, int sixtyFour, int open, uint32_t fileSize) {
    memset(window, 0x6e, kWindowSize);
    const size_t headerSize = sixtyFour ? 32 : 28;
    put32(window, sixtyFour ? 0xfeedfacf : 0xfeedface, swapped);
    put32(window + 4, (uint32_t)nextRandom(state) & kCleanWord, swapped);
    size_t cursor = headerSize;
    // Offsets where an end bound changes the result: command starts and the end of each dylib name's
    // 20-byte comparison.
    size_t boundaries[32];
    size_t boundaryCount = 0;
    uint32_t count = 0;
    size_t commands = 1 + (size_t)randomBelow(state, 12);
    const size_t limit = open ? 3100 : 3900;
    for (size_t command = 0; command < commands; ++command) {
        uint8_t buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        size_t size;
        uint64_t kind = randomBelow(state, 6);
        if (kind <= 2) {
            int segment64 = kind == 2 ? !sixtyFour : sixtyFour;
            size_t base = segment64 ? 72 : 56;
            size_t stride = segment64 ? 80 : 68;
            uint32_t sections = (uint32_t)randomBelow(state, open ? 9 : 12);
            put32(buffer, segment64 ? 0x19 : 1, swapped);
            const char *segment =
                kSegmentNames[randomBelow(state, sizeof(kSegmentNames) / sizeof(kSegmentNames[0]))];
            strncpy((char *)buffer + 8, segment, 16);
            put32(buffer + (segment64 ? 64 : 48), sections, swapped);
            for (uint32_t section = 0; section < sections; ++section) {
                uint8_t *header = buffer + base + section * stride;
                // Half of the names are the ones this width compares.
                const char *sectionName =
                    kSectionNames[randomBelow(state, sizeof(kSectionNames) / sizeof(kSectionNames[0]))];
                const char *segmentName =
                    kSegmentNames[randomBelow(state, sizeof(kSegmentNames) / sizeof(kSegmentNames[0]))];
                if (randomBelow(state, 2) == 0) {
                    sectionName = segment64 ? "__objc_imageinfo" : "__image_info";
                }
                if (randomBelow(state, 2) == 0) {
                    segmentName = segment64 ? "__DATA" : "__OBJC";
                }
                strncpy((char *)header, sectionName, 16);
                strncpy((char *)header + 16, segmentName, 16);
                static const uint64_t kSizes[] = {0, 7, 8, 9, 16, 64};
                uint64_t sectionSize = randomBelow(state, 7) == 6 ? nextRandom(state) & kCleanDoubleWord
                                                                  : kSizes[randomBelow(state, 6)];
                uint32_t sectionOffset;
                uint64_t offsetChoice = randomBelow(state, 5);
                if (offsetChoice == 0) {
                    sectionOffset = fileSize + (uint32_t)randomBelow(state, 64);
                } else if (offsetChoice == 1) {
                    sectionOffset = (uint32_t)randomBelow(state, UINT32_C(0x7fffffff));
                } else {
                    sectionOffset = (uint32_t)randomBelow(state, fileSize + 16u);
                }
                sectionOffset &= kCleanWord;
                if (segment64) {
                    put64(header + 40, sectionSize, swapped);
                    put32(header + 48, sectionOffset, swapped);
                } else {
                    put32(header + 36, (uint32_t)sectionSize, swapped);
                    put32(header + 40, sectionOffset, swapped);
                }
            }
            size = base + sections * stride;
        } else if (kind == 3) {
            const char *name = kDylibNames[randomBelow(state, sizeof(kDylibNames) / sizeof(kDylibNames[0]))];
            uint32_t nameOffset = randomBelow(state, 6) == 0 ? (uint32_t)randomBelow(state, 64) : 24;
            put32(buffer, randomBelow(state, 6) == 0 ? 0xd : 0xc, swapped);
            put32(buffer + 8, nameOffset, swapped);
            size_t nameLength = strlen(name) + 1;
            memcpy(buffer + 24, name, nameLength);
            size = 24 + nameLength + (size_t)randomBelow(state, 9);
            if (boundaryCount < 32) {
                boundaries[boundaryCount++] = cursor + nameOffset + 20;
            }
        } else {
            static const uint32_t kOther[] = {2, 0xb, 0x22, 0x80000028, 0x1b, 0x32};
            put32(buffer, kOther[randomBelow(state, 6)], swapped);
            size = 8 + 4 * (size_t)randomBelow(state, 20);
            for (size_t index = 8; index < size; ++index) {
                buffer[index] = (uint8_t)(nextRandom(state) & 0x7f);
            }
        }
        uint32_t recorded = (uint32_t)size;
        if (!open) {
            uint64_t sizeChoice = randomBelow(state, 8);
            if (sizeChoice == 0) {
                recorded = 0;
            } else if (sizeChoice == 1) {
                recorded = (uint32_t)randomBelow(state, 300);
            } else if (sizeChoice == 2) {
                recorded = (uint32_t)nextRandom(state) & kCleanWord;
            }
        }
        put32(buffer + 4, recorded, swapped);
        if (cursor + size > limit) {
            break;
        }
        memcpy(window + cursor, buffer, size);
        if (boundaryCount < 32) {
            boundaries[boundaryCount++] = cursor;
        }
        cursor += size;
        ++count;
    }
    uint32_t commandCount;
    uint32_t commandsSize;
    if (open) {
        commandCount = (uint32_t)randomBelow(state, count + 3);
        commandsSize = randomBelow(state, 2) == 0
                           ? UINT32_MAX
                           : 4097 - (uint32_t)headerSize + (uint32_t)randomBelow(state, 100000);
    } else {
        commandCount = (uint32_t)randomBelow(state, 65);
        uint64_t sizeChoice = randomBelow(state, 3);
        if (sizeChoice == 0) {
            commandsSize = (uint32_t)(cursor - headerSize);
        } else if (sizeChoice == 1 && boundaryCount > 0) {
            // One byte before, at or after a boundary, measured with either header width.
            size_t boundary = boundaries[randomBelow(state, boundaryCount)] + (size_t)randomBelow(state, 3);
            size_t width = randomBelow(state, 2) == 0 ? 32 : 28;
            commandsSize = boundary > width ? (uint32_t)(boundary - width - 1) : 0;
        } else {
            commandsSize = (uint32_t)randomBelow(state, 3801);
        }
        if (commandsSize > 3800) {
            commandsSize = 3800;
        }
    }
    put32(window + 16, commandCount, swapped);
    put32(window + 20, commandsSize, swapped);
}

static const uint32_t kSliceOffsets[] = {0, 0, 4096, 1234, 8192};

static int runMachOCases(Fixture *fixture) {
    uint8_t window[kWindowSize];
    static uint8_t file[8192 + kWindowSize + 4096];
    for (size_t iteration = 0; iteration < 12000; ++iteration) {
        uint8_t swapped = (uint8_t)(nextRandom(&fixture->state) & 1);
        uint8_t sixtyFour = (uint8_t)(nextRandom(&fixture->state) & 1);
        int open = randomBelow(&fixture->state, 3) == 0;
        uint32_t offset =
            kSliceOffsets[randomBelow(&fixture->state, sizeof(kSliceOffsets) / sizeof(kSliceOffsets[0]))];
        size_t windowLength = randomBelow(&fixture->state, 8) == 0
                                  ? (size_t)randomBelow(&fixture->state, kWindowSize)
                                  : kWindowSize;
        size_t fileLength = offset + windowLength + (size_t)randomBelow(&fixture->state, 4096);
        buildMachO(window, &fixture->state, swapped, sixtyFour, open, (uint32_t)fileLength);
        for (size_t index = 0; index < fileLength; ++index) {
            file[index] = (uint8_t)nextRandom(&fixture->state);
        }
        memcpy(file + offset, window, windowLength);
        int status = writeFixture(fixture, file, fileLength);
        if (status != 0) {
            return status;
        }
        size_t lengths[3] = {
            fileLength,
            (size_t)randomBelow(&fixture->state, fileLength + 1),
            0,
        };
        for (int mode = 0; mode < kDescriptorModes; ++mode) {
            for (size_t lengthIndex = 0; lengthIndex < 3; ++lengthIndex) {
                if (lengthIndex > 0 && randomBelow(&fixture->state, 2) == 0) {
                    continue;
                }
                const void *bytes = lengthIndex == 2 ? NULL : file;
                // Bounded windows may also be read with the other header width; open windows only as built.
                for (uint8_t width = 0; width < 2; ++width) {
                    uint8_t readWidth = width ? (uint8_t)!sixtyFour : sixtyFour;
                    if (width && open) {
                        continue;
                    }
                    status =
                        compareMachO(fixture, mode, bytes, lengths[lengthIndex], offset, swapped, readWidth);
                    if (status != 0) {
                        return status;
                    }
                }
            }
        }
    }
    return 0;
}

static int resolveFunctions(Fixture *fixture) {
    static const uint64_t kAddresses[kFunctionCount] = {
        UINT64_C(0x1c025d7c4),
        UINT64_C(0x1c025e57c),
        UINT64_C(0x1c025e7cc),
        UINT64_C(0x1c025eedc),
    };
    void *nativeFunctions[kFunctionCount];
    void *replacementFunctions[kFunctionCount] = {
        (void *)_CFBundleGrokFileTypeForZipFile,
        (void *)_CFBundleGrokFileTypeForOLEFile,
        (void *)_CFBundleGrokX11FromFile,
        (void *)_CFBundleGrokObjcImageInfoFromFile,
    };
    for (size_t index = 0; index < kFunctionCount; ++index) {
        int status = resolveReferenceAddress(
            "CoreFoundation",
            OF_REFERENCE_CF_UUID,
            kAddresses[index],
            UINT64_C(0x1c014a000),
            &nativeFunctions[index]
        );
        if (status == 0) {
            status = checkInternalImplementation(replacementFunctions[index]);
        }
        if (status != 0) {
            return status;
        }
        if (nativeFunctions[index] == replacementFunctions[index]) {
            fputs("Setup failure: bundle grok reference resolved to the replacement.\n", stderr);
            return 77;
        }
    }
    fixture->native.zip = (FileTypeFunction)nativeFunctions[0];
    fixture->native.ole = (FileTypeFunction)nativeFunctions[1];
    fixture->native.x11 = (X11Function)nativeFunctions[2];
    fixture->native.imageInfo = (ImageInfoFunction)nativeFunctions[3];
    fixture->replacement.zip = (FileTypeFunction)replacementFunctions[0];
    fixture->replacement.ole = (FileTypeFunction)replacementFunctions[1];
    fixture->replacement.x11 = (X11Function)replacementFunctions[2];
    fixture->replacement.imageInfo = (ImageInfoFunction)replacementFunctions[3];
    return 0;
}

static int openFixture(Fixture *fixture) {
    char base[PATH_MAX];
    size_t length = confstr(_CS_DARWIN_USER_TEMP_DIR, base, sizeof(base));
    if (length == 0 || length > sizeof(base)) {
        const char *environment = getenv("TMPDIR");
        snprintf(base, sizeof(base), "%s", environment && *environment ? environment : "/tmp/");
    }
    size_t baseLength = strlen(base);
    const char *separator = baseLength > 0 && base[baseLength - 1] == '/' ? "" : "/";
    snprintf(
        fixture->directory, sizeof(fixture->directory), "%s%sOpenFoundationGrok.XXXXXX", base, separator
    );
    if (!mkdtemp(fixture->directory)) {
        fputs("Setup failure: cannot create the grok fixture directory.\n", stderr);
        return 77;
    }
    snprintf(fixture->path, sizeof(fixture->path), "%s/fixture", fixture->directory);
    if (pipe(fixture->pipeDescriptors) != 0) {
        rmdir(fixture->directory);
        fputs("Setup failure: cannot create the grok pipe.\n", stderr);
        return 77;
    }
    fixture->directoryDescriptor = open(fixture->directory, O_RDONLY);
    if (fixture->directoryDescriptor < 0) {
        close(fixture->pipeDescriptors[0]);
        close(fixture->pipeDescriptors[1]);
        rmdir(fixture->directory);
        fputs("Setup failure: cannot open the grok fixture directory.\n", stderr);
        return 77;
    }
    return 0;
}

static void closeFixture(Fixture *fixture) {
    close(fixture->directoryDescriptor);
    close(fixture->pipeDescriptors[0]);
    close(fixture->pipeDescriptors[1]);
    unlink(fixture->path);
    rmdir(fixture->directory);
}

// Native _CFBundleGrokFileTypeForZipFile 0x1c025d7c4, _CFBundleGrokFileTypeForOLEFile 0x1c025e57c,
// _CFBundleGrokX11FromFile 0x1c025e7cc and _CFBundleGrokObjcImageInfoFromFile 0x1c025eedc (non-external;
// resolved by IDA offset). iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. iOS Device 24A446: 0x18077acd4, 0x18077bb0c, 0x18077bd98, 0x18077c060; UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Fixtures are written to a per-run temporary directory and read through no descriptor, the file, an
// unseekable pipe and a directory. Extension strings are compared by content; the X11 result and the
// image-information outputs (with canaries and NULL output combinations) byte for byte. Inputs never
// reach the documented native over-reads.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
int runCoreFoundationBundleGrokDifferentialTests(void) {
    Fixture fixture;
    memset(&fixture, 0, sizeof(fixture));
    int status = resolveFunctions(&fixture);
    if (status != 0) {
        return status;
    }
    status = openFixture(&fixture);
    if (status != 0) {
        return status;
    }
    fixture.state = UINT64_C(0x6772306b0f11e5);
    status = runZipCases(&fixture);
    if (status == 0) {
        status = runOLECases(&fixture);
    }
    if (status == 0) {
        status = runMachOCases(&fixture);
    }
    closeFixture(&fixture);
    if (status != 0) {
        return status;
    }
    printf(
        "Bundle grok differential cases passed: %zu (four functions; %zu typed archives, %zu X11 images, %zu "
        "segments found, %zu information words read; finite coverage).\n",
        fixture.cases,
        fixture.typed,
        fixture.x11Found,
        fixture.segmentsFound,
        fixture.wordsRead
    );
    return 0;
}
