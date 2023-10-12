//
//  ScaleInterpolator.h
//  JxclCoder [https://github.com/awxkee/jxl-coder-swift]
//
//  Created by Radzivon Bartoshyk on 03/10/2023.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#ifndef ScaleInterpolator_h
#define ScaleInterpolator_h

#ifdef __cplusplus

#include "half.hpp"
#include <cstdint>

using namespace half_float;

template <typename T>
T fastSin1(T x);

template <typename T>
T fastSin2(T x);

template <typename T>
T fastCos(T x);

template <typename T> 
half_float::half PromoteToHalf(T t, float maxColors);

template <typename D, typename T>
D PromoteTo(T t, float maxColors);

template <typename T>
T DemoteHalfTo(half t, float maxColors);

template <typename D, typename T>
D DemoteTo(T t, float maxColors);

template <typename T>
T CubicBSpline(T t);

#if __arm64__
#include <arm_neon.h>

inline float32x4_t Cos(const float32x4_t d) {

    constexpr float C0 = 0.99940307;
    constexpr float C1 = -0.49558072;
    constexpr float C2 = 0.03679168;
    constexpr float C3 = -0.00434102;
    float32x4_t x2 = vmulq_f32(d, d);
    return vmlaq_f32(vdupq_n_f32(C0), x2, vmlaq_f32(vdupq_n_f32(C1), x2, vmlaq_f32(vdupq_n_f32(C2), x2, vdupq_n_f32(C3))));
}

inline float32x4_t CubicInterpolation(const float32x4_t d,
                               const float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3,
                               const float C, const float B) {

    float32x4_t duplet = vmulq_f32(d, d);
    float32x4_t triplet = vmulq_f32(duplet, d);
    float32x4_t f1 = vaddq_f32(vmulq_n_f32(p0, (-1.0f/6.0f)*B - C), vmulq_n_f32(p1, (-1.5f)*B - C + 2.0f));
    float32x4_t f2 = vaddq_f32(vmulq_n_f32(p2, 1.5f*B + C - 2.0f), vmulq_n_f32(p3, (1.0f/6.0f)*B + C));
    float32x4_t firstRow = vmulq_f32(vaddq_f32(f1, f2), triplet);

    float32x4_t s1 = vaddq_f32(vmulq_n_f32(p0, 0.5f*B + 2.0f*C), vmulq_n_f32(p1, 2.0f*B + C - 3.0f));
    float32x4_t s2 = vsubq_f32(vmulq_n_f32(p2, -2.5f*B-2.0f*C + 3.0f), vmulq_n_f32(p3, C));
    float32x4_t secondRow = vmulq_f32(vaddq_f32(s1, s2), duplet);

    float32x4_t thirdRow = vmulq_f32(vaddq_f32(vmulq_n_f32(p0, -0.5f*B - C), vmulq_n_f32(p2, 0.5f*B+C)), d);

    float32x4_t fourthRow = vaddq_f32(vaddq_f32(vmulq_n_f32(p0, (1.0f/6.0f)*B),
                                                vmulq_n_f32(p1, (-1.0f/3.0f)*B+1.0f)), vmulq_n_f32(p2, (1.0f/6.0f)*B));
    float32x4_t result = vaddq_f32(vaddq_f32(vaddq_f32(firstRow, secondRow), thirdRow), fourthRow);
    return result;
}

inline float32x4_t HannWindow(const float32x4_t d, const float length) {
    float32x4_t x = vabsq_f32(d);
    uint32x4_t mask = vcltq_f32(x, vdupq_n_f32(length / 2));

    x = Cos(vdivq_f32(vmulq_f32(vdupq_n_f32(M_PI), x), vdupq_n_f32(length)));
    x = vmulq_n_f32(vmulq_f32(x, x), length / 2);
    x = vbslq_f32(mask, vdupq_n_f32(0), x);
    return x;
}

inline float32x4_t CatmullRom(const float32x4_t d,
                              const float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3) {

    float32x4_t x = vabsq_f32(d);
    uint32x4_t mask = vcgtq_f32(x, vdupq_n_f32(1));
    x = vbslq_f32(mask, vdupq_n_f32(0), x);
    float32x4_t duplet = vmulq_f32(x, x);
    float32x4_t triplet = vmulq_f32(x, x);
    
    float32x4_t ff1 = vaddq_f32(vmulq_n_f32(p1, 2.0f), vmulq_f32(vsubq_f32(p2, p0), x));
    float32x4_t ff2 = vmulq_f32(vsubq_f32(vaddq_f32(vsubq_f32(vmulq_n_f32(p0, 2.0f), vmulq_n_f32(p1, 5.0)), vmulq_n_f32(p2, 4.0)), p3), duplet);
    float32x4_t ff3 = vmulq_f32(vaddq_f32(vsubq_f32(vsubq_f32(vmulq_n_f32(p1, 3.0f), p0), vmulq_n_f32(p2, 3.0f)), p3), triplet);

    float32x4_t result = vmulq_n_f32(vaddq_f32(vaddq_f32(ff1, ff2), ff3), 0.5f);
    return result;
}

inline float32x4_t SimpleCubic(const float32x4_t d,
                               const float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3) {

    float32x4_t duplet = vmulq_f32(d, d);
    float32x4_t triplet = vmulq_f32(duplet, d);
    float32x4_t f1 = vaddq_f32(vmulq_n_f32(p0, -0.5f), vmulq_n_f32(p1, 1.5f));
    float32x4_t f2 = vaddq_f32(vmulq_n_f32(p2, -1.5f), vmulq_n_f32(p3, 1.0f/2.0f));
    float32x4_t firstRow = vmulq_n_f32(vmulq_f32(vaddq_f32(f1, f2), triplet), 3.0f);

    float32x4_t s1 = vaddq_f32(p0, vmulq_n_f32(p1, -5.0f/2.0f));
    float32x4_t s2 = vaddq_f32(vmulq_n_f32(p2, 2.0f), vmulq_n_f32(p3, -1/2.0f));
    float32x4_t secondRow = vmulq_f32(vaddq_f32(s1, s2), duplet);

    float32x4_t thirdRow = vmulq_f32(vaddq_f32(vmulq_n_f32(p0, -1.0f/2.0f), vmulq_n_f32(p2, 1.0f/2.0f)), d);

    float32x4_t fourthRow = p2;
    float32x4_t result = vaddq_f32(vaddq_f32(vaddq_f32(firstRow, secondRow), thirdRow), fourthRow);
    return result;
}

inline float32x4_t MitchellNetravali(float32x4_t d,
                              float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3) {
    return CubicInterpolation(d, p0, p1, p2, p3, 1.0f/3.0f, 1.0f/3.0f);
}

inline float32x4_t CubicHermite(const float32x4_t d,
                               const float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3) {
    return CubicInterpolation(d, p0, p1, p2, p3, 0.0f, 0.0f);
}

inline float32x4_t CubicBSpline(const float32x4_t d,
                               const float32x4_t p0, const float32x4_t p1, const float32x4_t p2, const float32x4_t p3) {
    return CubicInterpolation(d, p0, p1, p2, p3, 0.0f, 1.0f);
}

#endif

template <typename T>
T SimpleCubic(T t, T A, T B, T C, T D);

template <typename T>
T CubicHermite(T d, T p0, T p1, T p2, T p3);

template <typename T>
T CubicBSpline(T d, T p0, T p1, T p2, T p3);

template <typename T>
T FilterMitchell(T t);

template <typename T>
T MitchellNetravali(T d, T p0, T p1, T p2, T p3);

template <typename T>
T sinc(T x);

template <typename T>
T LanczosWindow(T x, const T a);

template <typename T>
T HannWindow(T x, const T length);

template <typename T>
T CatmullRom(T x);

template <typename T>
T CatmullRom(T x, T p0, T p1, T p2, T p3);

#endif

#endif /* ScaleInterpolator_h */
