//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObjCRuntime.h>

#if defined(__OBJC__)
NS_ENUM(NSInteger) {
    NSFileNoSuchFileError = 4,
    NSFileLockingError = 255,
    NSFileReadUnknownError = 256,
    NSFileReadNoPermissionError = 257,
    NSFileReadInvalidFileNameError = 258,
    NSFileReadCorruptFileError = 259,
    NSFileReadNoSuchFileError = 260,
    NSFileReadInapplicableStringEncodingError = 261,
    NSFileReadUnsupportedSchemeError = 262,
    NSFileReadTooLargeError = 263,
    NSFileReadUnknownStringEncodingError = 264,
    NSFileWriteUnknownError = 512,
    NSFileWriteNoPermissionError = 513,
    NSFileWriteInvalidFileNameError = 514,
    NSFileWriteFileExistsError = 516,
    NSFileWriteInapplicableStringEncodingError = 517,
    NSFileWriteUnsupportedSchemeError = 518,
    NSFileWriteOutOfSpaceError = 640,
    NSFileWriteVolumeReadOnlyError = 642,
    NSKeyValueValidationError = 1024,
    NSFormattingError = 2048,
    NSUserCancelledError = 3072,
    NSFeatureUnsupportedError = 3328,
    NSPropertyListReadCorruptError = 3840,
    NSPropertyListReadUnknownVersionError = 3841,
    NSPropertyListReadStreamError = 3842,
    NSPropertyListWriteStreamError = 3851,
    NSPropertyListWriteInvalidError = 3852,
    NSPropertyListErrorMinimum = 3840,
    NSPropertyListErrorMaximum = 4095,
    NSCoderReadCorruptError = 4864,
    NSCoderValueNotFoundError = 4865,
    NSCoderInvalidValueError = 4866,
    NSCoderErrorMinimum = 4864,
    NSCoderErrorMaximum = 4991
};
#endif
