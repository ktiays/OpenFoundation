//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Number/ONSValue.h"

OF_EXPORT id ONSStringFromPoint(ValuePoint point);
OF_EXPORT id ONSStringFromSize(ValueSize size);
OF_EXPORT id ONSStringFromRect(ValueRect rect);
OF_EXPORT BOOL ONSShouldRetainWithZone(id object, struct _NSZone *requestedZone);
