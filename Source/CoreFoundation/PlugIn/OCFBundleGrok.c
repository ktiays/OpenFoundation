//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleGrok.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xlocale.h>

// Native unnamed table of _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c0456de5 (six
// five-byte NUL-padded extensions, 30 bytes, after _CFBundleGrokX11FromFile.libX11name), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1809654e5, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// 30 bytes agree. The native code returns the table address plus 0, 5, 10, 15, 20 or 25.
// kSunExtensions is a local descriptive name.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
static const char kSunExtensions[6][5] = {"sxc", "sxd", "sxg", "sxi", "sxm", "sxw"};

// Native unnamed table of _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c0456e04 (nine
// five-byte NUL-padded extensions, 45 bytes), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. Non-external.
// iOS Device 24A446: 0x180965504, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// 45 bytes agree. The native code returns the table address plus 0, 5, ..., 40.
// kOpenDocumentExtensions is a local descriptive name.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
static const char kOpenDocumentExtensions[9][5] = {
    "odc",
    "odf",
    "odg",
    "oth",
    "odi",
    "odm",
    "odp",
    "ods",
    "odt",
};

// Native _CFBundleGrokFileTypeForZipFile, iOS Simulator 24A434, 0x1c025d7c4,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077acd4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokFileTypeForZipFile at 0x18077acd4 keeps the META-INF/MANIFEST.MF
// flag in a stack slot instead of a register, indexes the signature bytes from the buffer base and
// places blocks differently; every comparison, bound, call, argument and result agrees.
// The only caller, _CFBundleGrokFileType at 0x1c025c96c, passes the descriptor, the header buffer and its
// length; the code uses fileLength (signed) only as the end of the trailing 1,024-byte window. The first
// pass scans `bytes` for local ("PK\3\4", name length at +26, 30-byte header) and central ("PK\1\2",
// +28, 46 bytes) entries whose name fits; a local "mimetype" entry may decide the result. Otherwise the
// trailing window is scanned the same way (without the mimetype rule) and the recorded names decide.
// Comparisons are strncasecmp_l with a NULL locale. Native pointer comparisons are expressed with
// uintptr_t; they are the same comparisons.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
const char *_CFBundleGrokFileTypeForZipFile(int fd, const void *bytes, size_t length, off_t fileLength) {
    const uint8_t *buffer = (const uint8_t *)bytes;
    const char *result = "zip";
    int hasMetaInf = 0;
    int hasContentXML = 0;
    int hasManifestXML = 0;
    int hasPresentationXML = 0;
    int hasSpreadsheetXML = 0;
    int hasSMIL = 0;
    int hasOPF = 0;
    int hasDocumentXML = 0;
    int hasRels = 0;
    int hasContentTypes = 0;
    int hasManifestMF = 0;
    int keepScanning = 1;
    if (length >= 31) {
        const uintptr_t bufferEnd = (uintptr_t)buffer + length;
        const OCFIndex scanLimit = (OCFIndex)(length - 30);
        OCFIndex offset = 0;
        do {
            const uint8_t *entry = buffer + offset;
            uint32_t headerLength = 0;
            uint32_t lengthField = 0;
            int isEntry = 0;
            int isLocal = 0;
            if (entry[0] == 'P' && entry[1] == 'K') {
                if (entry[2] == 3) {
                    if (entry[3] == 4) {
                        isEntry = 1;
                        isLocal = 1;
                        headerLength = 30;
                        lengthField = 26;
                    }
                } else if (entry[2] == 1 && entry[3] == 2) {
                    isEntry = 1;
                    headerLength = 46;
                    lengthField = 28;
                }
            }
            keepScanning = 1;
            if (isEntry) {
                uint16_t nameLength;
                memcpy(&nameLength, entry + lengthField, sizeof(nameLength));
                if (offset + (OCFIndex)headerLength + (OCFIndex)nameLength <= (OCFIndex)length) {
                    const char *name = (const char *)entry + headerLength;
                    if (isLocal && nameLength == 8 && strncasecmp_l(name, "mimetype", 8, NULL) == 0) {
                        uint16_t fileNameLength;
                        uint16_t extraLength;
                        uint16_t method;
                        memcpy(&fileNameLength, entry + 26, sizeof(fileNameLength));
                        memcpy(&extraLength, entry + 28, sizeof(extraLength));
                        memcpy(&method, entry + 8, sizeof(method));
                        const uintptr_t content = (uintptr_t)entry + fileNameLength + extraLength + 30;
                        const char *contentType = (const char *)content;
                        if (content + 56 <= bufferEnd && method == 0 &&
                            (strncasecmp_l(contentType, "application/vnd.", 16, NULL) == 0 ||
                             strncasecmp_l(contentType, "application/x-vnd.", 18, NULL) == 0)) {
                            const char *kind = contentType + (contentType[15] == '.' ? 16 : 18);
                            if (strncasecmp_l(kind, "sun.xml.", 8, NULL) == 0) {
                                const char *application = kind + 8;
                                int index = -1;
                                if (strncasecmp_l(application, "calc", 4, NULL) == 0) {
                                    index = 0;
                                } else if (strncasecmp_l(application, "draw", 4, NULL) == 0) {
                                    index = 1;
                                } else if (strncasecmp_l(application, "writer.global", 13, NULL) == 0) {
                                    index = 2;
                                } else if (strncasecmp_l(application, "impress", 7, NULL) == 0) {
                                    index = 3;
                                } else if (strncasecmp_l(application, "math", 4, NULL) == 0) {
                                    index = 4;
                                } else if (strncasecmp_l(application, "writer", 6, NULL) == 0) {
                                    index = 5;
                                }
                                if (index >= 0) {
                                    result = kSunExtensions[index];
                                    keepScanning = 0;
                                }
                            } else if (strncasecmp_l(kind, "oasis.opendocument.", 19, NULL) == 0) {
                                const char *application = kind + 19;
                                int index = -1;
                                if (strncasecmp_l(application, "chart", 5, NULL) == 0) {
                                    index = 0;
                                } else if (strncasecmp_l(application, "formula", 7, NULL) == 0) {
                                    index = 1;
                                } else if (strncasecmp_l(application, "graphics", 8, NULL) == 0) {
                                    index = 2;
                                } else if (strncasecmp_l(application, "text-web", 8, NULL) == 0) {
                                    index = 3;
                                } else if (strncasecmp_l(application, "image", 5, NULL) == 0) {
                                    index = 4;
                                } else if (strncasecmp_l(application, "text-master", 11, NULL) == 0) {
                                    index = 5;
                                } else if (strncasecmp_l(application, "presentation", 12, NULL) == 0) {
                                    index = 6;
                                } else if (strncasecmp_l(application, "spreadsheet", 11, NULL) == 0) {
                                    index = 7;
                                } else if (strncasecmp_l(application, "text", 4, NULL) == 0) {
                                    index = 8;
                                }
                                if (index >= 0) {
                                    result = kOpenDocumentExtensions[index];
                                    keepScanning = 0;
                                }
                            }
                        } else {
                            uint32_t firstWord = 0;
                            uint32_t secondWord = 0;
                            int hasMagic = 0;
                            if (content + 41 <= bufferEnd && method == 8) {
                                memcpy(&firstWord, contentType, sizeof(firstWord));
                                if (firstWord == UINT32_C(0xc8282c4b)) {
                                    memcpy(&secondWord, contentType + 4, sizeof(secondWord));
                                    hasMagic = secondWord == UINT32_C(0x2c4e4cc9);
                                }
                            }
                            if (hasMagic) {
                                result = "odt";
                            } else if (
                                content + 29 <= bufferEnd &&
                                strncasecmp_l(contentType, "application/oebps-package+xml", 29, NULL) == 0
                            ) {
                                result = "epub";
                                keepScanning = 0;
                            } else if (
                                content + 20 <= bufferEnd &&
                                strncasecmp_l(contentType, "application/epub+zip", 20, NULL) == 0
                            ) {
                                result = "epub";
                                keepScanning = 0;
                            }
                        }
                    } else {
                        int checkSuffixes = 0;
                        switch (nameLength) {
                            case 9:
                                if (strncasecmp_l(name, "META-INF/", 9, NULL) == 0) {
                                    hasMetaInf = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 11:
                                if (strncasecmp_l(name, "content.xml", 11, NULL) == 0) {
                                    hasContentXML = 1;
                                } else if (strncasecmp_l(name, "_rels/.rels", 11, NULL) == 0) {
                                    hasRels = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 19:
                                if (strncasecmp_l(name, "[Content_Types].xml", 19, NULL) == 0) {
                                    hasContentTypes = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 20:
                                if (strncasecmp_l(name, "META-INF/MANIFEST.MF", 20, NULL) == 0) {
                                    hasManifestMF = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 21:
                                if (strncasecmp_l(name, "META-INF/manifest.xml", 21, NULL) == 0) {
                                    hasManifestXML = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            default:
                                checkSuffixes = nameLength >= 5;
                                break;
                        }
                        if (checkSuffixes) {
                            const char *nameEnd = name + nameLength;
                            if (strncasecmp_l(nameEnd - 4, ".opf", 4, NULL) == 0) {
                                hasOPF = 1;
                            } else if (strncasecmp_l(nameEnd - 4, ".sml", 4, NULL) == 0) {
                                hasSMIL = 1;
                            } else if (nameLength >= 6) {
                                if (strncasecmp_l(nameEnd - 5, ".smil", 5, NULL) == 0) {
                                    hasSMIL = 1;
                                } else if (nameLength >= 8) {
                                    if (strncasecmp_l(name, "xl/", 3, NULL) == 0 &&
                                        strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                        hasSpreadsheetXML = 1;
                                    } else if (nameLength != 8) {
                                        if (strncasecmp_l(name, "ppt/", 4, NULL) == 0 &&
                                            strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                            hasPresentationXML = 1;
                                        } else if (nameLength >= 10) {
                                            if (strncasecmp_l(name, "word/", 5, NULL) == 0 &&
                                                strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                hasDocumentXML = 1;
                                            } else if (nameLength != 10) {
                                                if (strncasecmp_l(name, "excel/", 6, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                    hasSpreadsheetXML = 1;
                                                } else if (
                                                    nameLength >= 16 &&
                                                    strncasecmp_l(name, "powerpoint/", 11, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0
                                                ) {
                                                    hasPresentationXML = 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    offset += (uint32_t)(headerLength + nameLength - 1);
                }
            }
            if (!keepScanning) {
                break;
            }
            ++offset;
        } while (offset < scanLimit);
    }
    if (keepScanning) {
        uint8_t *allocated = NULL;
        const uint8_t *window = NULL;
        if (fileLength >= 1024) {
            const off_t windowOffset = fileLength - 1024;
            if (fd >= 0 && lseek(fd, windowOffset, SEEK_SET) == windowOffset) {
                allocated =
                    (uint8_t *)malloc_type_malloc(1024, (malloc_type_id_t)UINT64_C(0x100004077774924));
                if (allocated != NULL && read(fd, allocated, 1024) > 1023) {
                    window = allocated;
                }
            } else if (length >= 1024) {
                window = buffer + length - 1024;
            }
        }
        if (window != NULL) {
            OCFIndex offset = 0;
            do {
                const uint8_t *entry = window + offset;
                uint32_t headerLength = 0;
                uint32_t lengthField = 0;
                int isEntry = 0;
                if (entry[0] == 'P' && entry[1] == 'K') {
                    if (entry[2] == 3) {
                        if (entry[3] == 4) {
                            isEntry = 1;
                            headerLength = 30;
                            lengthField = 26;
                        }
                    } else if (entry[2] == 1 && entry[3] == 2) {
                        isEntry = 1;
                        headerLength = 46;
                        lengthField = 28;
                    }
                }
                if (isEntry) {
                    uint16_t nameLength;
                    memcpy(&nameLength, entry + lengthField, sizeof(nameLength));
                    if (offset + (OCFIndex)headerLength + (OCFIndex)nameLength <= 1024) {
                        const char *name = (const char *)entry + headerLength;
                        int checkSuffixes = 0;
                        switch (nameLength) {
                            case 9:
                                if (strncasecmp_l(name, "META-INF/", 9, NULL) == 0) {
                                    hasMetaInf = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 11:
                                if (strncasecmp_l(name, "content.xml", 11, NULL) == 0) {
                                    hasContentXML = 1;
                                } else if (strncasecmp_l(name, "_rels/.rels", 11, NULL) == 0) {
                                    hasRels = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 19:
                                if (strncasecmp_l(name, "[Content_Types].xml", 19, NULL) == 0) {
                                    hasContentTypes = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 20:
                                if (strncasecmp_l(name, "META-INF/MANIFEST.MF", 20, NULL) == 0) {
                                    hasManifestMF = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            case 21:
                                if (strncasecmp_l(name, "META-INF/manifest.xml", 21, NULL) == 0) {
                                    hasManifestXML = 1;
                                } else {
                                    checkSuffixes = 1;
                                }
                                break;
                            default:
                                checkSuffixes = nameLength >= 5;
                                break;
                        }
                        if (checkSuffixes) {
                            const char *nameEnd = name + nameLength;
                            if (strncasecmp_l(nameEnd - 4, ".opf", 4, NULL) == 0) {
                                hasOPF = 1;
                            } else if (strncasecmp_l(nameEnd - 4, ".sml", 4, NULL) == 0) {
                                hasSMIL = 1;
                            } else if (nameLength >= 6) {
                                if (strncasecmp_l(nameEnd - 5, ".smil", 5, NULL) == 0) {
                                    hasSMIL = 1;
                                } else if (nameLength >= 8) {
                                    if (strncasecmp_l(name, "xl/", 3, NULL) == 0 &&
                                        strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                        hasSpreadsheetXML = 1;
                                    } else if (nameLength != 8) {
                                        if (strncasecmp_l(name, "ppt/", 4, NULL) == 0 &&
                                            strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                            hasPresentationXML = 1;
                                        } else if (nameLength >= 10) {
                                            if (strncasecmp_l(name, "word/", 5, NULL) == 0 &&
                                                strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                hasDocumentXML = 1;
                                            } else if (nameLength != 10) {
                                                if (strncasecmp_l(name, "excel/", 6, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0) {
                                                    hasSpreadsheetXML = 1;
                                                } else if (
                                                    nameLength >= 16 &&
                                                    strncasecmp_l(name, "powerpoint/", 11, NULL) == 0 &&
                                                    strncasecmp_l(nameEnd - 4, ".xml", 4, NULL) == 0
                                                ) {
                                                    hasPresentationXML = 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        offset += (uint32_t)(headerLength + nameLength - 1);
                    }
                }
            } while (offset++ < 993);
        }
        const int hasPackageParts = hasRels != 0 || hasContentTypes != 0;
        if (hasManifestMF) {
            result = "jar";
        } else if (hasPackageParts && hasDocumentXML) {
            result = "docx";
        } else if (hasSpreadsheetXML && hasPackageParts) {
            result = "xlsx";
        } else if ((uint8_t)hasPresentationXML != 0 && hasPackageParts) {
            result = "pptx";
        } else if (hasContentXML || hasManifestXML) {
            result = "odt";
        } else if (hasMetaInf) {
            result = "jar";
        } else if (hasOPF && hasSMIL) {
            result = "dtb";
        } else if (hasOPF) {
            result = "oeb";
        }
        if (allocated != NULL) {
            free(allocated);
        }
    }
    return result;
}

// Native _CFBundleGrokFileTypeForOLEFile, iOS Simulator 24A434, 0x1c025e57c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077bb0c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokFileTypeForOLEFile at 0x18077bb0c forms the entry address with
// BFI and counts the compared characters from one; every read, comparison and result agrees.
// The only caller, _CFBundleGrokFileType at 0x1c025c7b8, passes ((uint32_t)word at header +0x30 << 9)
// + 512 as offset. The 512 bytes at offset are read as four 128-byte entries. The byte at +64 of an entry,
// as a signed char halved toward zero, selects a name: 5 "Book" and 9 "Workbook" ("xls"), 13
// "WordDocument" ("doc"), 20 "PowerPoint Document" ("ppt"). That many characters minus one are compared
// with the low bytes of the entry's 16-bit units; the first matching entry decides, otherwise "ole".
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
const char *_CFBundleGrokFileTypeForOLEFile(int fd, const void *bytes, size_t length, off_t offset) {
    const char *result = "ole";
    uint8_t *allocated = NULL;
    const uint8_t *directory = NULL;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == offset) {
        allocated = (uint8_t *)malloc_type_malloc(512, (malloc_type_id_t)UINT64_C(0x100004077774924));
        if (allocated == NULL) {
            return result;
        }
        if (read(fd, allocated, 512) > 511) {
            directory = allocated;
        }
    } else if (length >= (size_t)(offset + 512)) {
        directory = (const uint8_t *)bytes + offset;
    }
    if (directory != NULL) {
        for (uint64_t index = 0; index < 4; ++index) {
            const uint8_t *entry = directory + 128 * index;
            const int characters = (int8_t)entry[64] / 2;
            const char *name;
            const char *type;
            switch (characters) {
                case 5:
                    name = "Book";
                    type = "xls";
                    break;
                case 9:
                    name = "Workbook";
                    type = "xls";
                    break;
                case 13:
                    name = "WordDocument";
                    type = "doc";
                    break;
                case 20:
                    name = "PowerPoint Document";
                    type = "ppt";
                    break;
                default:
                    continue;
            }
            const uint64_t count = (uint32_t)(characters - 1);
            int equal = 1;
            for (uint64_t character = 0; character < count; ++character) {
                if (entry[character * 2] != (uint8_t)name[character]) {
                    equal = 0;
                    break;
                }
            }
            if (equal) {
                result = type;
                break;
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    return result;
}

// Native _CFBundleGrokX11FromFile, iOS Simulator 24A434, 0x1c025e7cc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077bd98, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokX11FromFile at 0x18077bd98 compares load-command offsets instead
// of addresses and advances the offset before testing the result; every read, bound, call and result
// agrees.
// Callers: _CFBundleGrokFileType (offset 0, four swapped/sixtyFour pairs) and
// _CFBundleGrokMachTypeForFatFile (a 32-bit slice offset). A 4,096-byte window at offset holds a Mach-O
// header; ncmds (+16) and sizeofcmds (+20) are byte-swapped when `swapped`. Load commands start at 32
// (sixtyFour) or 28 and end at min(start + sizeofcmds, 4096). The result is 1 when an LC_LOAD_DYLIB
// (12) command's name, at its 32-bit offset +8 and with 20 bytes inside the end, begins with the 19
// characters of libX11name. Native pointer walks are expressed as offsets from the header; the
// comparisons are the same.
// CoreFoundation defect: _CFBundleGrokX11FromFile at 0x1c025e7cc (iOS Device 0x18077bd98; confidence:
// verified) checks only that a load command starts before the end, then reads its 32-bit cmd (+0), name
// offset (+8, LC_LOAD_DYLIB only) and cmdsize (+4); a command within 11 bytes of a 4,096-byte end reads
// past the heap window or the caller's bytes. The bytes-path check (uint32_t)(offset + 4096) <= length
// also wraps, so an offset above 0xfffff000 reads the window outside the caller's bytes. The restoration
// never reads outside the window or `bytes`: a field that does not fit ends the scan with result 0, and a
// window that does not fit returns 0. Every in-range result is unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
uint8_t _CFBundleGrokX11FromFile(
    int fd, const void *bytes, size_t length, uint32_t offset, uint8_t swapped, uint8_t sixtyFour
) {
    // Native _CFBundleGrokX11FromFile.libX11name, iOS Simulator 24A434, 0x1c0456dd1 (20 bytes with the
    // NUL; the sizeof bound of the name check), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
    // verified. Non-external. iOS Device 24A446: 0x1809654d1, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
    // confidence: verified; the 20 bytes agree.
    static const char libX11name[] = "/usr/X11R6/lib/libX";
    uint8_t *allocated = NULL;
    const uint8_t *header;
    uint64_t readable;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset) {
        allocated = (uint8_t *)malloc_type_malloc(4096, (malloc_type_id_t)UINT64_C(0xe7329b55));
        if (allocated == NULL) {
            return 0;
        }
        if (read(fd, allocated, 4096) <= 4095) {
            free(allocated);
            return 0;
        }
        header = allocated;
        readable = 4096;
    } else {
        if (length < (uint32_t)(offset + 4096)) {
            return 0;
        }
        // CoreFoundation defect: the wrapped window check above is kept; a window outside `bytes` is
        // not read.
        if ((uint64_t)offset + 4096 > length) {
            return 0;
        }
        header = (const uint8_t *)bytes + offset;
        readable = length - offset;
    }
    uint8_t result = 0;
    uint32_t commandCount;
    uint32_t commandsSize;
    memcpy(&commandCount, header + 16, sizeof(commandCount));
    memcpy(&commandsSize, header + 20, sizeof(commandsSize));
    if (swapped) {
        commandCount = __builtin_bswap32(commandCount);
        commandsSize = __builtin_bswap32(commandsSize);
    }
    const uint64_t headerSize = sixtyFour ? 32 : 28;
    const uint64_t endOffset = commandsSize > 4096 - headerSize ? 4096 : headerSize + commandsSize;
    if (commandCount != 0 && headerSize < endOffset) {
        uint64_t commandOffset = headerSize;
        uint32_t index = 1;
        for (;;) {
            // CoreFoundation defect: each load-command field must lie inside the readable bytes.
            if (readable - commandOffset < 4) {
                result = 0;
                break;
            }
            uint32_t command;
            memcpy(&command, header + commandOffset, sizeof(command));
            if (swapped) {
                command = __builtin_bswap32(command);
            }
            result = 0;
            if (command == 12) {
                if (readable - commandOffset < 12) {
                    break;
                }
                uint32_t nameOffset;
                memcpy(&nameOffset, header + commandOffset + 8, sizeof(nameOffset));
                if (swapped) {
                    nameOffset = __builtin_bswap32(nameOffset);
                }
                if (commandOffset + nameOffset + sizeof(libX11name) <= endOffset) {
                    result = strncmp(
                                 (const char *)header + commandOffset + nameOffset,
                                 libX11name,
                                 sizeof(libX11name) - 1
                             ) == 0;
                }
            }
            if (readable - commandOffset < 8) {
                result = 0;
                break;
            }
            uint32_t commandSize;
            memcpy(&commandSize, header + commandOffset + 4, sizeof(commandSize));
            if (swapped) {
                commandSize = __builtin_bswap32(commandSize);
            }
            if (result || index >= commandCount) {
                break;
            }
            commandOffset += commandSize;
            ++index;
            if (commandOffset >= endOffset) {
                break;
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    return result;
}

// Native _CFBundleGrokObjcImageInfoFromFile, iOS Simulator 24A434, 0x1c025eedc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18077c060, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: _CFBundleGrokObjcImageInfoFromFile at 0x18077c060 compares load-command and
// section offsets instead of addresses and keeps more state in stack slots; every read, bound, call,
// store and result agrees.
// Callers: _CFBundleGrokFileType (offset 0) and _CFBundleGrokMachTypeForFatFile (a slice offset, not
// swapped, 64-bit). The Mach-O window is read as in _CFBundleGrokX11FromFile. In LC_SEGMENT_64 (0x19;
// nsects at +64, 80-byte sections from +72) or LC_SEGMENT (1; +48, 68-byte sections from +56) commands,
// each section's segment name (+16) is compared with "__DATA" or "__OBJC" and its name (+0) with
// "__objc_imageinfo" or "__image_info", 16 bytes each with strncmp, segment name first. A matching
// segment name sets *segmentFound; a section matching both stops the scan with its size (the low 32 bits
// of the 64-bit field at +40, or +36) and 32-bit offset (+48, or +40). With a size of at least 8, the two
// 32-bit words at offset + section offset (32-bit sum) are read from the file, or else from `bytes` when
// (uint32_t)(offset + section offset + 8) <= length; otherwise both words are 0. Every non-NULL output
// is stored after the window is freed, also on failure. Native pointer walks are expressed as offsets
// from the header; the comparisons are the same.
// CoreFoundation defect: _CFBundleGrokObjcImageInfoFromFile at 0x1c025eedc (iOS Device 0x18077c060;
// confidence: verified) checks only that a load command or section starts before the end. It then reads
// cmd, nsects (+64 or +48) and cmdsize, both 16-byte names and, on a match, the size and offset fields,
// up to 52 bytes past the start, outside the 4,096-byte heap window or the caller's bytes. The window
// check (uint32_t)(offset + 4096) <= length and the image-information check
// (uint32_t)(offset + section offset + 8) <= length wrap, and the bytes are then read at the unwrapped
// 64-bit address. The restoration never reads outside the window or `bytes`. A name comparison that the
// readable bytes cannot decide counts as unequal; any other field that does not fit ends the scan as if
// no section matched; a window or image-information words that do not fit are treated as failed checks.
// Every in-range result is unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationBundleGrok.md.
void _CFBundleGrokObjcImageInfoFromFile(
    int fd,
    const void *bytes,
    size_t length,
    uint32_t offset,
    uint8_t swapped,
    uint8_t sixtyFour,
    uint8_t *segmentFound,
    uint32_t *firstInfoWord,
    uint32_t *secondInfoWord
) {
    uint8_t foundSegment = 0;
    uint32_t firstWord = 0;
    uint32_t secondWord = 0;
    uint8_t *allocated = NULL;
    const uint8_t *header = NULL;
    uint64_t readable = 0;
    if (fd >= 0 && lseek(fd, offset, SEEK_SET) == (off_t)offset) {
        allocated = (uint8_t *)malloc_type_malloc(4096, (malloc_type_id_t)UINT64_C(0x298ae5f0));
        if (allocated != NULL && read(fd, allocated, 4096) > 4095) {
            header = allocated;
            readable = 4096;
        }
    } else if (length >= (uint32_t)(offset + 4096) && (uint64_t)offset + 4096 <= length) {
        // CoreFoundation defect: the second condition keeps the wrapped window check from reading
        // outside `bytes`.
        header = (const uint8_t *)bytes + offset;
        readable = length - offset;
    }
    if (header != NULL) {
        uint32_t commandCount;
        uint32_t commandsSize;
        memcpy(&commandCount, header + 16, sizeof(commandCount));
        memcpy(&commandsSize, header + 20, sizeof(commandsSize));
        if (swapped) {
            commandCount = __builtin_bswap32(commandCount);
            commandsSize = __builtin_bswap32(commandsSize);
        }
        const uint64_t headerSize = sixtyFour ? 32 : 28;
        const uint32_t segmentCommand = sixtyFour ? 0x19 : 1;
        const uint64_t sectionCountOffset = sixtyFour ? 64 : 48;
        const uint64_t sectionsOffset = sixtyFour ? 72 : 56;
        const uint64_t sectionStride = sixtyFour ? 80 : 68;
        const uint64_t sizeFieldOffset = sixtyFour ? 40 : 36;
        const uint64_t offsetFieldOffset = sixtyFour ? 48 : 40;
        const char *segmentName = sixtyFour ? "__DATA" : "__OBJC";
        const char *sectionName = sixtyFour ? "__objc_imageinfo" : "__image_info";
        const uint64_t segmentNameLength = 6;
        const uint64_t sectionNameLength = sixtyFour ? 16 : 12;
        const uint64_t endOffset = commandsSize > 4096 - headerSize ? 4096 : headerSize + commandsSize;
        uint32_t sectionLength = 0;
        uint32_t sectionOffset = 0;
        if (commandCount != 0 && headerSize < endOffset) {
            uint64_t commandOffset = headerSize;
            uint32_t index = 0;
            for (;;) {
                int keepScanning = 1;
                // CoreFoundation defect: each field read below must lie inside the readable bytes.
                if (readable - commandOffset < 4) {
                    break;
                }
                uint32_t command;
                memcpy(&command, header + commandOffset, sizeof(command));
                if (swapped) {
                    command = __builtin_bswap32(command);
                }
                if (command == segmentCommand) {
                    if (readable - commandOffset < sectionCountOffset + 4) {
                        break;
                    }
                    uint32_t sectionCount;
                    memcpy(&sectionCount, header + commandOffset + sectionCountOffset, sizeof(sectionCount));
                    if (swapped) {
                        sectionCount = __builtin_bswap32(sectionCount);
                    }
                    if (sectionCount != 0 && commandOffset + sectionsOffset < endOffset) {
                        uint64_t sectionStart = commandOffset + sectionsOffset;
                        uint32_t sectionIndex = 1;
                        for (;;) {
                            const uint64_t available = readable - sectionStart;
                            const char *section = (const char *)header + sectionStart;
                            int segmentDifference = 1;
                            if (available >= 32) {
                                segmentDifference = strncmp(section + 16, segmentName, 16);
                            } else if (available > 16) {
                                // CoreFoundation defect: an undecided comparison counts as unequal.
                                segmentDifference = strncmp(section + 16, segmentName, available - 16);
                                if (segmentDifference == 0 && available - 16 <= segmentNameLength) {
                                    segmentDifference = 1;
                                }
                            }
                            if (segmentDifference == 0) {
                                foundSegment = 1;
                            }
                            int sectionDifference;
                            if (available >= 16) {
                                sectionDifference = strncmp(section, sectionName, 16);
                            } else {
                                // CoreFoundation defect: an undecided comparison counts as unequal.
                                sectionDifference = strncmp(section, sectionName, available);
                                if (sectionDifference == 0 && available <= sectionNameLength) {
                                    sectionDifference = 1;
                                }
                            }
                            if ((sectionDifference | segmentDifference) == 0) {
                                if (available < offsetFieldOffset + 4) {
                                    goto scanned;
                                }
                                if (sixtyFour) {
                                    uint64_t size;
                                    memcpy(&size, section + sizeFieldOffset, sizeof(size));
                                    if (swapped) {
                                        size = __builtin_bswap64(size);
                                    }
                                    sectionLength = (uint32_t)size;
                                } else {
                                    memcpy(&sectionLength, section + sizeFieldOffset, sizeof(sectionLength));
                                    if (swapped) {
                                        sectionLength = __builtin_bswap32(sectionLength);
                                    }
                                }
                                memcpy(&sectionOffset, section + offsetFieldOffset, sizeof(sectionOffset));
                                if (swapped) {
                                    sectionOffset = __builtin_bswap32(sectionOffset);
                                }
                                keepScanning = 0;
                                break;
                            }
                            if (sectionIndex >= sectionCount) {
                                break;
                            }
                            ++sectionIndex;
                            sectionStart += sectionStride;
                            if (sectionStart >= endOffset) {
                                break;
                            }
                        }
                    }
                }
                if (readable - commandOffset < 8) {
                    break;
                }
                uint32_t commandSize;
                memcpy(&commandSize, header + commandOffset + 4, sizeof(commandSize));
                if (swapped) {
                    commandSize = __builtin_bswap32(commandSize);
                }
                if (!keepScanning) {
                    break;
                }
                if (++index >= commandCount) {
                    break;
                }
                commandOffset += commandSize;
                if (commandOffset < headerSize || commandOffset >= endOffset) {
                    break;
                }
            }
        }
    scanned:
        if (sectionLength >= 8) {
            const uint32_t position = offset + sectionOffset;
            uint32_t words[2];
            if (fd >= 0 && lseek(fd, position, SEEK_SET) == (off_t)position && read(fd, words, 8) >= 8) {
                firstWord = words[0];
                secondWord = words[1];
                if (swapped) {
                    firstWord = __builtin_bswap32(firstWord);
                    secondWord = __builtin_bswap32(secondWord);
                }
            } else if (
                length >= (uint32_t)(offset + sectionOffset + 8) &&
                (uint64_t)offset + sectionOffset + 8 <= length
            ) {
                // CoreFoundation defect: the second condition keeps the wrapped check from reading
                // outside `bytes`.
                const uint8_t *information = (const uint8_t *)bytes + offset + sectionOffset;
                memcpy(&firstWord, information, sizeof(firstWord));
                memcpy(&secondWord, information + 4, sizeof(secondWord));
                if (swapped) {
                    firstWord = __builtin_bswap32(firstWord);
                    secondWord = __builtin_bswap32(secondWord);
                }
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    if (segmentFound != NULL) {
        *segmentFound = foundSegment;
    }
    if (firstInfoWord != NULL) {
        *firstInfoWord = firstWord;
    }
    if (secondInfoWord != NULL) {
        *secondInfoWord = secondWord;
    }
}
