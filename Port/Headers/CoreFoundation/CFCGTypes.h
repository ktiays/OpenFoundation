//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The CoreGraphics base types CoreFoundation defines (the SDK's CFCGTypes.h). CGFloat is double on every
// pointer size: OpenCoreAnimation and OpenCoreGraphics are built with the arm64 layout of their geometry.

#include <float.h>
#include <stdint.h>
#include <CoreFoundation/CFAvailability.h>

#if defined(__OBJC__) && __has_attribute(objc_boxable)
    #define CF_BOXABLE __attribute__((objc_boxable))
#else
    #define CF_BOXABLE
#endif

#define CGFLOAT_TYPE double
#define CGFLOAT_IS_DOUBLE 1
#define CGFLOAT_MIN DBL_MIN
#define CGFLOAT_MAX DBL_MAX
#define CGFLOAT_EPSILON DBL_EPSILON

typedef CGFLOAT_TYPE CGFloat;
#define CGFLOAT_DEFINED 1

#define CF_DEFINES_CG_TYPES 1

struct CGPoint {
    CGFloat x;
    CGFloat y;
};
typedef struct CF_BOXABLE CGPoint CGPoint;

struct CGSize {
    CGFloat width;
    CGFloat height;
};
typedef struct CF_BOXABLE CGSize CGSize;

struct CGVector {
    CGFloat dx;
    CGFloat dy;
};
typedef struct CF_BOXABLE CGVector CGVector;
#define CGVECTOR_DEFINED 1

struct CGRect {
    CGPoint origin;
    CGSize size;
};
typedef struct CF_BOXABLE CGRect CGRect;

typedef CF_CLOSED_ENUM(uint32_t, CGRectEdge) {
    CGRectMinXEdge,
    CGRectMinYEdge,
    CGRectMaxXEdge,
    CGRectMaxYEdge
};

struct CGAffineTransform {
    CGFloat a, b, c, d;
    CGFloat tx, ty;
};
typedef struct CF_BOXABLE CGAffineTransform CGAffineTransform;

#define CF_DEFINES_CGAFFINETRANSFORMCOMPONENTS 1

struct CGAffineTransformComponents {
    CGSize scale;
    CGFloat horizontalShear;
    CGFloat rotation;
    CGVector translation;
};
typedef struct CGAffineTransformComponents CGAffineTransformComponents;
