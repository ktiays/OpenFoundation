//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The CoreFoundation umbrella header for OpenFoundation's wasm32 CoreFoundation, in the SDK's shape. Only the
// areas the port's consumers use are declared; Port/Headers/README.md lists them and how they are checked.

#include <sys/types.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFByteOrder.h>
#include <CoreFoundation/CFCalendar.h>
#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDate.h>
#include <CoreFoundation/CFDateFormatter.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFLocale.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFPreferences.h>
#include <CoreFoundation/CFPropertyList.h>
#include <CoreFoundation/CFSet.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFTimeZone.h>
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFUUID.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFRunLoop.h>
#include <CoreFoundation/CFStream.h>
#include <CoreFoundation/CFMachPort.h>
#include <CoreFoundation/CFAttributedString.h>
#include <CoreFoundation/CFNotificationCenter.h>
#include <CoreFoundation/CFCGTypes.h>
