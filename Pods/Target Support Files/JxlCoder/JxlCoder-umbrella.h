#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
#ifndef FOUNDATION_EXPORT
#if defined(__cplusplus)
#define FOUNDATION_EXPORT extern "C"
#else
#define FOUNDATION_EXPORT extern
#endif
#endif
#endif

#import "JxlInternalCoder.h"
#import "RgbaScaler.h"
#import "ScaleInterpolator.h"
#import "half.hpp"
#import "JXLSystemImage.hpp"
#import "RgbRgbaConverter.hpp"
#import "XScaler.hpp"

FOUNDATION_EXPORT double JxlCoderVersionNumber;
FOUNDATION_EXPORT const unsigned char JxlCoderVersionString[];

