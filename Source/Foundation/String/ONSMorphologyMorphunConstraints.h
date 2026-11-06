//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/String/ONSMorphologyPronoun.h"

// The Objective-C interfaces of the Swift extensions in String/ONSMorphology+Morphun.swift (native @objc
// thunks NSMorphology.morphunConstraints() 0x1c0f7b688 and NSMorphologyPronoun.morphunConstraints()
// 0x1c0f7beb8), for the Objective-C senders only. Swift does not import this header: it defines the methods.
// The categories declare and define nothing.
@interface ONSMorphology (MorphunConstraints)
- (id)_morphunConstraints;
@end

@interface ONSMorphologyPronoun (MorphunConstraints)
- (id)_morphunConstraints;
@end
