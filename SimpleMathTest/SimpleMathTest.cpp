//-------------------------------------------------------------------------------------
// SimpleMathTest.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include <d3d11.h>
#include "SimpleMath.h"

#include <DirectXColors.h>

#include <stdio.h>

#include <functional>
#include <map>
#include <memory>
#include <vector>

using namespace DirectX;
using namespace DirectX::SimpleMath;

static const float EPSILON = 0.000001f;
static const float EPSILON2 = 0.00001f;
static const float EPSILON3 = 0.001f;

static const XMVECTORF32 VEPSILON = { EPSILON, EPSILON, EPSILON, EPSILON };
static const XMVECTORF32 VEPSILON2 = { EPSILON2, EPSILON2, EPSILON2, EPSILON2 };
static const XMVECTORF32 VEPSILON3 = { EPSILON3, EPSILON3, EPSILON3, EPSILON3 };


void FormatValue(bool value, char* output, size_t outputSize) { strcpy_s(output, outputSize, value ? "true" : "false"); }
void FormatValue(float value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f", value); }
void FormatValue(uint32_t value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%08x", value); }
void FormatValue(Vector3 const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f %f %f", value.x, value.y, value.z); }
void FormatValue(Vector4 const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w); }
void FormatValue(Color const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w); }
void FormatValue(Plane const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w); }
void FormatValue(Quaternion const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w); }
void FormatValue(Matrix const& value, char* output, size_t outputSize) { sprintf_s(output, outputSize, "\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n", value._11, value._12, value._13, value._14, value._21, value._22, value._23, value._24, value._31, value._32, value._33, value._34, value._41, value._42, value._43, value._44); }


struct near_equal_to
{
    bool operator() (float a, float b) const
    {
        return XMScalarNearEqual(a, b, EPSILON);
    }

    bool operator() (FXMVECTOR a, FXMVECTOR b) const
    {
        return XMVector4NearEqual(a, b, VEPSILON);
    }

    bool operator() (CXMMATRIX a, CXMMATRIX b) const
    {
        return XMVector4NearEqual(a.r[0], b.r[0], VEPSILON) &&
            XMVector4NearEqual(a.r[1], b.r[1], VEPSILON) &&
            XMVector4NearEqual(a.r[2], b.r[2], VEPSILON) &&
            XMVector4NearEqual(a.r[3], b.r[3], VEPSILON);
    }
};


template<typename TValue, typename TCompare>
bool VerifyValue(TValue const& value, TValue const& expected, TCompare const& compare, char const* file, int line)
{
    if (compare(value, expected))
        return true;

    char valueString[256];
    char expectedString[256];

    FormatValue(value, valueString, sizeof(valueString));
    FormatValue(expected, expectedString, sizeof(expectedString));

    printf("ERROR: %s:%d: %s (expecting %s)\n", file, line, valueString, expectedString);

    return false;
}


#define VerifyEqual(value, expected) \
    success &= VerifyValue(value, expected, std::equal_to<decltype(value)>(), __FUNCTION__, __LINE__)

#define VerifyNearEqual(value, expected) \
    success &= VerifyValue(value, expected, near_equal_to(), __FUNCTION__, __LINE__)


//-------------------------------------------------------------------------------------
namespace
{
    std::vector<SimpleMath::Rectangle> CreateIntersectedRectangles(const SimpleMath::Rectangle& target)
    {
        using Rectangle = SimpleMath::Rectangle;

        long x = target.x;
        long y = target.y;
        long w = target.width;
        long h = target.height;
        long hw = target.width / 2;
        long hh = target.height / 2;
        long cx = x + hw;
        long cy = y + hh;

#ifdef DIRECTX_EMULATE_MAKE_UNIQUE
        std::vector<Rectangle> rects;
        rects.push_back(Rectangle(x - hw,  y - hh, w, h)); // Left Upper corner
        rects.push_back(Rectangle(x,       y - hh, w, h)); // Upper
        rects.push_back(Rectangle(cx,      y - hh, w, h)); // Right Upper corner
        rects.push_back(Rectangle(cx,      y,      w, h)); // Right
        rects.push_back(Rectangle(cx,      cy,     w, h)); // Right bottom corner
        rects.push_back(Rectangle(x,       cy,     w, h)); // Bottom
        rects.push_back(Rectangle(x - hw,  cy,     w, h)); // Left Bottom corner
        rects.push_back(Rectangle(x - hw,  y,      w, h)); // Left
#else
        std::vector<Rectangle> rects = {
            Rectangle(x - hw,  y - hh, w, h), // Left Upper coner
            Rectangle(x,       y - hh, w, h), // Upper
            Rectangle(cx,      y - hh, w, h), // Right Upper coner
            Rectangle(cx,      y,      w, h), // Right
            Rectangle(cx,      cy,     w, h), // Right bottom coner
            Rectangle(x,       cy,     w, h), // Bottom
            Rectangle(x - hw,  cy,     w, h), // Left Bottom coner
            Rectangle(x - hw,  y,      w, h), // Left
        };
#endif
        return rects;
    }

    std::vector<SimpleMath::Rectangle> CreateDisjointedRectangles(const SimpleMath::Rectangle& target)
    {
        using Rectangle = SimpleMath::Rectangle;

        long x = target.x;
        long y = target.y;
        long w = target.width;
        long h = target.height;

#ifdef DIRECTX_EMULATE_MAKE_UNIQUE
        std::vector<Rectangle> rects;
        rects.push_back(Rectangle(x - w,   y - h,  w, h)); // Left Upper corner
        rects.push_back(Rectangle(x,       y - h,  w, h)); // Upper
        rects.push_back(Rectangle(x + w,   y - h,  w, h)); // Right Upper corner
        rects.push_back(Rectangle(x + w,   y,      w, h)); // Right
        rects.push_back(Rectangle(x + w,   y + h,  w, h)); // Right bottom corner
        rects.push_back(Rectangle(x,       y + h,  w, h)); // Bottom
        rects.push_back(Rectangle(x - w,   y + h,  w, h)); // Left Bottom corner
        rects.push_back(Rectangle(x - w,   y,      w, h)); // Left
#else
        std::vector<Rectangle> rects = {
            Rectangle(x - w,   y - h,  w, h), // Left Upper coner
            Rectangle(x,       y - h,  w, h), // Upper
            Rectangle(x + w,   y - h,  w, h), // Right Upper coner
            Rectangle(x + w,   y,      w, h), // Right
            Rectangle(x + w,   y + h,  w, h), // Right bottom coner
            Rectangle(x,       y + h,  w, h), // Bottom
            Rectangle(x - w,   y + h,  w, h), // Left Bottom coner
            Rectangle(x - w,   y,      w, h), // Left
        };
#endif
        return rects;
    }
}

int TestRect()
{
    // Rectangle
    bool success = true;

    using Rectangle = SimpleMath::Rectangle;

    Rectangle empty;
    Rectangle rectZero(0, 0, 0, 0);
    if (!(rectZero == empty))
    {
        printf("ERROR: ==\n");
        success = false;
    }

    if (rectZero != empty)
    {
        printf("ERROR: !=\n");
        success = false;
    }

    if (!empty.IsEmpty())
    {
        printf("ERROR: IsEmpty\n");
        success = false;
    }

    Rectangle unit(0, 0, 1, 1);
    if (unit.x != 0
        || unit.y != 0
        || unit.width != 1
        || unit.height != 1)
    {
        printf("ERROR: Rectangle ctor unit\n");
        success = false;
    }

    if (rectZero == unit)
    {
        printf("ERROR: ==\n");
        success = false;
    }

    Rectangle simple(0, 0, 100, 100);
    if (simple.x != 0
        || simple.y != 0
        || simple.width != 100
        || simple.height != 100)
    {
        printf("ERROR: Rectangle ctor simple\n");
        success = false;
    }

    if (unit == simple)
    {
        printf("ERROR: ==\n");
        success = false;
    }

    Rectangle smallRect(50, 75, 100, 200);
    if (smallRect.x != 50
        || smallRect.y != 75
        || smallRect.width != 100
        || smallRect.height != 200)
    {
        printf("ERROR: Rectangle ctor sm\n");
        success = false;
    }

    Rectangle bigRect(15, 32, 1920, 1080);
    if (bigRect.x != 15
        || bigRect.y != 32
        || bigRect.width != 1920
        || bigRect.height != 1080)
    {
        printf("ERROR: Rectangle ctor big\n");
        success = false;
    }

    if (smallRect == bigRect)
    {
        printf("ERROR: ==\n");
        success = false;
    }

    {
        RECT rct = { 15, 32, 1920, 1080 };

        Rectangle r(rct);

        if (r.x != 15
            || r.y != 32
            || r.width != (1920 - 15)
            || r.height != (1080 - 32))
        {
            printf("ERROR: RECT ctor\n");
            success = false;
        }
    }

    {
        RECT smallRct = { 50, 75, 100 + 50, 200 + 75 };

        if (smallRct != smallRect)
        {
            printf("ERROR: RECT != small\n");
            success = false;
        }

        if (smallRect != smallRct)
        {
            printf("ERROR: RECT != small\n");
            success = false;
        }

        RECT bigRct = { 15, 32, 1920 + 15, 1080 + 32 };

        if (bigRct != bigRect)
        {
            printf("ERROR: RECT != big\n");
            success = false;
        }

        if (bigRect != bigRct)
        {
            printf("ERROR: RECT != big\n");
            success = false;
        }

        if (smallRect == bigRct)
        {
            printf("ERROR: RECT ==\n");
            success = false;
        }
    }

    {
        RECT rct = bigRect;

        if (rct.left != bigRect.x
            || rct.right != (bigRect.x + bigRect.width)
            || rct.top != bigRect.y
            || rct.bottom != (bigRect.y + bigRect.height))
        {
            printf("ERROR: op RECT\n");
            success = false;
        }
    }

    {
        Rectangle r = smallRect;
        if (r.x != 50
            || r.y != 75
            || r.width != 100
            || r.height != 200)
        {
            printf("ERROR: assignment opt\n");
            success = false;
        }

        r = bigRect;
        if (r.x != 15
            || r.y != 32
            || r.width != 1920
            || r.height != 1080)
        {
            printf("ERROR: assignment opt\n");
            success = false;
        }
    }

    // Location/Center
    {
        Vector2 loc = smallRect.Location();
        Vector2 ctr = smallRect.Center();

        if (loc.x != float(smallRect.x)
            || loc.y != float(smallRect.y)
            || ctr.x != (float(smallRect.x) + float(smallRect.width / 2.f))
            || ctr.y != (float(smallRect.y) + float(smallRect.height / 2.f)))
        {
            printf("ERROR: Location/Center small\n");
            success = false;
        }

        loc = bigRect.Location();
        ctr = bigRect.Center();

        if (loc.x != float(bigRect.x)
            || loc.y != float(bigRect.y)
            || ctr.x != (float(bigRect.x) + float(bigRect.width / 2.f))
            || ctr.y != (float(bigRect.y) + float(bigRect.height / 2.f)))
        {
            printf("ERROR: Location/Center big\n");
            success = false;
        }
    }

    // Contains
    if (!simple.Contains(0, 0))
    {
        printf("ERROR: Contains 1\n");
        success = false;
    }

    if ( !simple.Contains(Vector2(0.f,0.f)))
    {
        printf("ERROR: Contains v1\n");
        success = false;
    }

    if (!simple.Contains(50, 50))
    {
        printf("ERROR: Contains 2\n");
        success = false;
    }

    if (!simple.Contains(Vector2(50.f, 50.f)))
    {
        printf("ERROR: Contains 2v\n");
        success = false;
    }

    if (simple.Contains(100, 0))
    {
        printf("ERROR: Contains 3\n");
        success = false;
    }

    if (simple.Contains(Vector2(100.f, 0.f)))
    {
        printf("ERROR: Contains 3v\n");
        success = false;
    }

    if (simple.Contains(0, 100))
    {
        printf("ERROR: Contains 4\n");
        success = false;
    }

    if (simple.Contains(Vector2(0.f, 100.f)))
    {
        printf("ERROR: Contains 4v\n");
        success = false;
    }

    if (simple.Contains(-1, 0))
    {
        printf("ERROR: Contains 5\n");
        success = false;
    }

    if (simple.Contains(Vector2(-1.f, 0.f)))
    {
        printf("ERROR: Contains 5v\n");
        success = false;
    }

    if (simple.Contains(0, -1))
    {
        printf("ERROR: Contains 6\n");
        success = false;
    }

    if (simple.Contains(Vector2(0.f, -1.f)))
    {
        printf("ERROR: Contains 6v\n");
        success = false;
    }

    {
        Rectangle b = simple;
        b.Inflate(-2, -2);

        if (!simple.Contains(b))
        {
            printf("ERROR: Contains R1\n");
            success = false;
        }

        if (!simple.Contains(simple))
        {
            printf("ERROR: Contains R2\n");
            success = false;
        }
    }

    {
        Rectangle b = simple;
        b.Inflate(-2, -2);

        RECT c = b;
        if (!simple.Contains(c))
        {
            printf("ERROR: Contains R3\n");
            success = false;
        }

        RECT d = simple;
        if (!simple.Contains(d))
        {
            printf("ERROR: Contains R4\n");
            success = false;
        }
    }

    if (!simple.Contains(unit))
    {
        printf("ERROR: Contains R5\n");
        success = false;
    }

    if (unit.Contains(simple))
    {
        printf("ERROR: Contains R6\n");
        success = false;
    }

    if (smallRect.Contains(unit))
    {
        printf("ERROR: Contains R7\n");
        success = false;
    }

    if (bigRect.Contains(unit))
    {
        printf("ERROR: Contains R8\n");
        success = false;
    }

    if (smallRect.Contains(bigRect))
    {
        printf("ERROR: Contains R9\n");
        success = false;
    }

    if (!bigRect.Contains(smallRect))
    {
        printf("ERROR: Contains R10\n");
        success = false;
    }

    {
        auto rects1 = CreateIntersectedRectangles(simple);
        for (auto& r : rects1)
        {
            if (simple.Contains(r))
            {
                printf("ERROR: Contains intersected\n");
                success = false;
            }
        }

        auto rects2 = CreateDisjointedRectangles(simple);
        for (auto& r : rects2)
        {
            if (simple.Contains(r))
            {
                printf("ERROR: Contains disjointed\n");
                success = false;
            }
        }
    }

    // Inflate
    {
        Rectangle r = smallRect;
        r.Inflate(1, 2);
        if (r.x != (50 - 1)
            || r.y != (75 - 2)
            || r.width != (100 + 1)
            || r.height != (200 + 2))
        {
            printf("ERROR: Inflate\n");
            success = false;
        }

        r = bigRect;
        r.Inflate(-1, -2);
        if (r.x != (15 + 1)
            || r.y != (32 + 2)
            || r.width != (1920 - 1)
            || r.height != (1080 - 2))
        {
            printf("ERROR: Inflate\n");
            success = false;
        }
    }

    // Intersects
    {
        Rectangle b = simple;
        b.Inflate(-2, -2);

        if (!simple.Intersects(b))
        {
            printf("ERROR: Intersects R1\n");
            success = false;
        }

        if (!simple.Intersects(simple))
        {
            printf("ERROR: Intersects R2\n");
            success = false;
        }
    }

    {
        Rectangle b = simple;
        b.Inflate(-2, -2);

        RECT c = b;
        if (!simple.Intersects(c))
        {
            printf("ERROR: Intersects R3\n");
            success = false;
        }

        RECT d = simple;
        if (!simple.Intersects(d))
        {
            printf("ERROR: Intersects R4\n");
            success = false;
        }
    }

    if (!simple.Intersects(unit))
    {
        printf("ERROR: Intersects R5\n");
        success = false;
    }

    if (!unit.Intersects(simple))
    {
        printf("ERROR: Intersects R6\n");
        success = false;
    }

    if (smallRect.Intersects(unit))
    {
        printf("ERROR: Intersects R7\n");
        success = false;
    }

    if (bigRect.Intersects(unit))
    {
        printf("ERROR: Intersects R8\n");
        success = false;
    }

    if (!smallRect.Intersects(bigRect))
    {
        printf("ERROR: Intersects R9\n");
        success = false;
    }

    if (!bigRect.Intersects(smallRect))
    {
        printf("ERROR: Intersects R10\n");
        success = false;
    }

    {
        auto rects1 = CreateIntersectedRectangles(simple);
        for (auto& r : rects1)
        {
            if (!simple.Intersects(r))
            {
                printf("ERROR: Intersects intersected\n");
                success = false;
            }
        }

        auto rects2 = CreateDisjointedRectangles(simple);
        for (auto& r : rects2)
        {
            if (simple.Intersects(r))
            {
                printf("ERROR: Intersects disjointed\n");
                success = false;
            }
        }
    }

    // Offset
    {
        Rectangle r = smallRect;
        r.Offset(1, 2);
        if (r.x != (50 + 1)
            || r.y != (75 + 2)
            || r.width != 100
            || r.height != 200)
        {
            printf("ERROR: Offset\n");
            success = false;
        }

        r = bigRect;
        r.Offset(-1, -2);
        if (r.x != (15 - 1)
            || r.y != (32 - 2)
            || r.width != 1920
            || r.height != 1080)
        {
            printf("ERROR: Offset\n");
            success = false;
        }
    }

    // Intersect
    {
        Rectangle a(10, 20, 4, 5);
        Rectangle b(12, 15, 100, 7);
        Rectangle c(0, 0, 10, 23);
        Rectangle d(10, 20, 0, 0);
        Rectangle e(0, 0, 0, 0);

        Rectangle ab(12, 20, 2, 2);
        Rectangle ac(0, 0, 0, 0);
        Rectangle ad(0, 0, 0, 0);
        Rectangle ae(0, 0, 0, 0);
        Rectangle bc(0, 0, 0, 0);

        if (ab != Rectangle::Intersect(a, b))
        {
            printf("ERROR: Intersect 1\n");
            success = false;
        }

        if (ac != Rectangle::Intersect(a, c))
        {
            printf("ERROR: Intersect 2\n");
            success = false;
        }

        if (ad != Rectangle::Intersect(a, d))
        {
            printf("ERROR: Intersect 3\n");
            success = false;
        }

        if (ae != Rectangle::Intersect(a, e))
        {
            printf("ERROR: Intersect 4\n");
            success = false;
        }

        if (bc != Rectangle::Intersect(b, c))
        {
            printf("ERROR: Intersect 5\n");
            success = false;
        }

        if (ab != Rectangle::Intersect(b, a))
        {
            printf("ERROR: Intersect 6\n");
            success = false;
        }

        if (ac != Rectangle::Intersect(c, a))
        {
            printf("ERROR: Intersect 7\n");
            success = false;
        }

        if (ad != Rectangle::Intersect(d, a))
        {
            printf("ERROR: Intersect 8\n");
            success = false;
        }

        if (ae != Rectangle::Intersect(e, a))
        {
            printf("ERROR: Intersect 9\n");
            success = false;
        }

        if (bc != Rectangle::Intersect(c, b))
        {
            printf("ERROR: Intersect 10\n");
            success = false;
        }
    }

    {
        RECT a = { 10, 20, 10 + 4, 20 + 5 };
        RECT b = { 12, 15, 12 + 100, 15 + 7 };
        RECT c = { 0, 0, 10, 23 };
        RECT d = { 10, 20, 10 + 0, 20 + 0 };
        RECT e = { 0, 0, 0, 0 };

        RECT ab = { 12, 20, 12 + 2, 20 + 2 };
        RECT ac = { 0, 0, 0, 0 };
        RECT ad = { 0, 0, 0, 0 };
        RECT ae = { 0, 0, 0, 0 };
        RECT bc = { 0, 0, 0, 0 };

        if (ab != Rectangle::Intersect(a, b))
        {
            printf("ERROR: Intersect RECT 1\n");
            success = false;
        }

        if (ac != Rectangle::Intersect(a, c))
        {
            printf("ERROR: Intersect RECT 2\n");
            success = false;
        }

        if (ad != Rectangle::Intersect(a, d))
        {
            printf("ERROR: Intersect RECT 3\n");
            success = false;
        }

        if (ae != Rectangle::Intersect(a, e))
        {
            printf("ERROR: Intersect RECT 4\n");
            success = false;
        }

        if (bc != Rectangle::Intersect(b, c))
        {
            printf("ERROR: Intersect RECT 5\n");
            success = false;
        }

        if (ab != Rectangle::Intersect(b, a))
        {
            printf("ERROR: Intersect RECT 6\n");
            success = false;
        }

        if (ac != Rectangle::Intersect(c, a))
        {
            printf("ERROR: Intersect RECT 7\n");
            success = false;
        }

        if (ad != Rectangle::Intersect(d, a))
        {
            printf("ERROR: Intersect RECT 8\n");
            success = false;
        }

        if (ae != Rectangle::Intersect(e, a))
        {
            printf("ERROR: Intersect RECT 9\n");
            success = false;
        }

        if (bc != Rectangle::Intersect(c, b))
        {
            printf("ERROR: Intersect RECT 10\n");
            success = false;
        }
    }

    // Union
    {
        Rectangle a(10, 20, 4, 5);
        Rectangle b(12, 15, 100, 7);
        Rectangle c(0, 0, 10, 23);
        Rectangle d(10, 20, 0, 0);
        Rectangle e(0, 0, 0, 0);

        Rectangle ab(10, 15, 102, 10);
        Rectangle ac(0, 0, 14, 25);
        Rectangle ad(10, 20, 4, 5);
        Rectangle ae(0, 0, 14, 25);
        Rectangle bc(0, 0, 112, 23);

        // Test non-ref overloads.
        if (ab != Rectangle::Union(a, b))
        {
            printf("ERROR: Union 1\n");
            success = false;
        }

        if (ac != Rectangle::Union(a, c))
        {
            printf("ERROR: Union 2\n");
            success = false;
        }

        if (ad != Rectangle::Union(a, d))
        {
            printf("ERROR: Union 3\n");
            success = false;
        }

        if (ae != Rectangle::Union(a, e))
        {
            printf("ERROR: Union 4\n");
            success = false;
        }

        if (bc != Rectangle::Union(b, c))
        {
            printf("ERROR: Union 5\n");
            success = false;
        }

        if (ab != Rectangle::Union(b, a))
        {
            printf("ERROR: Union 6\n");
            success = false;
        }

        if (ac != Rectangle::Union(c, a))
        {
            printf("ERROR: Union 7\n");
            success = false;
        }

        if (ad != Rectangle::Union(d, a))
        {
            printf("ERROR: Union 8\n");
            success = false;
        }

        if (ae != Rectangle::Union(e, a))
        {
            printf("ERROR: Union 9\n");
            success = false;
        }

        if (bc != Rectangle::Union(c, b))
        {
            printf("ERROR: Union 10\n");
            success = false;
        }
    }

    {
        RECT a = { 10, 20, 10 + 4, 20 + 5 };
        RECT b = { 12, 15, 12 + 100, 15 + 7 };
        RECT c = { 0, 0, 10, 23 };
        RECT d = { 10, 20, 10 + 0, 20 + 0 };
        RECT e = { 0, 0, 0, 0 };

        RECT ab = { 10, 15, 10 + 102, 15 + 10 };
        RECT ac = { 0, 0, 14, 25 };
        RECT ad = { 10, 20, 10 + 4, 20 + 5 };
        RECT ae = { 0, 0, 14, 25 };
        RECT bc = { 0, 0, 112, 23 };

        // Test non-ref overloads.
        if (ab != Rectangle::Union(a, b))
        {
            printf("ERROR: Union 1\n");
            success = false;
        }

        if (ac != Rectangle::Union(a, c))
        {
            printf("ERROR: Union 2\n");
            success = false;
        }

        if (ad != Rectangle::Union(a, d))
        {
            printf("ERROR: Union 3\n");
            success = false;
        }

        if (ae != Rectangle::Union(a, e))
        {
            printf("ERROR: Union 4\n");
            success = false;
        }

        if (bc != Rectangle::Union(b, c))
        {
            printf("ERROR: Union 5\n");
            success = false;
        }

        if (ab != Rectangle::Union(b, a))
        {
            printf("ERROR: Union 6\n");
            success = false;
        }

        if (ac != Rectangle::Union(c, a))
        {
            printf("ERROR: Union 7\n");
            success = false;
        }

        if (ad != Rectangle::Union(d, a))
        {
            printf("ERROR: Union 8\n");
            success = false;
        }

        if (ae != Rectangle::Union(e, a))
        {
            printf("ERROR: Union 9\n");
            success = false;
        }

        if (bc != Rectangle::Union(c, b))
        {
            printf("ERROR: Union 10\n");
            success = false;
        }
    }

    return (success) ? 0 : 1;
}

//-------------------------------------------------------------------------------------
int TestV2()
{
	// Vector2 
    bool success = true;

    Vector2 upVector( 0, 1.f );
    Vector2 rightVector( 1.f, 0 );
    Vector2 v1( 1.f, 2.f );
    Vector2 v2( 4.f, 5.f );
    Vector2 v3( 3.f, -23.f );

    if ( upVector == rightVector )
    {
        printf("ERROR: ==\n");
        success = false;
    }

    Vector2 zero(0.f, 0.f);
    Vector2 one(1.f, 1.f);
    if ( zero != Vector2::Zero
         || one != Vector2::One
         || rightVector != Vector2::UnitX
         || upVector != Vector2::UnitY )
    {
        printf("ERROR: constants\n");
        success = false;
    }

    if ( upVector != upVector )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    Vector2 v = v1;
    if ( v != v1 )
    {
        printf("ERROR: =\n");
        success = false;
    }

    {
        XMFLOAT2 xm(6.f, -2.f);
        Vector2 vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON))
        {
            printf("ERROR: XMFLOAT2 ctor\n");
            success = false;
        }
    }

    {
        Vector2 vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Vector2 vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    {
        XMFLOAT2 xm(6.f, -2.f);
        Vector2 vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON))
        {
            printf("ERROR: XMFLOAT2 =\n");
            success = false;
        }
    }

    v += v2;
    if ( v != Vector2(5,7) )
    {
        printf("ERROR: += %f %f ... 5 7\n", v.x, v.y );
        success = false;
    }

    v -= v1;
    if ( v != Vector2(4,5) )
    {
        printf("ERROR: -= %f %f ... 4 5\n", v.x, v.y );
        success = false;
    }

    v *= v1;
    if ( v != Vector2(4,10) )   
    {
        printf("ERROR: *= %f %f ... 4 10\n", v.x, v.y );
        success = false;
    }

    // InBounds
    if ( !v1.InBounds(v2)
         || v2.InBounds(v1))
    {
        printf("ERROR: InBounds\n");
        success = false;
    }

    // Length/LengthSquared
    {
        float len = v2.Length();

        if ( !XMScalarNearEqual( len, 6.403124f, EPSILON ) )
        {
            printf("ERROR: len %f\n", len );
            success = false;
        }

        len = v2.LengthSquared();

        if ( !XMScalarNearEqual( len, 41, EPSILON ) )
        {
            printf("ERROR: len^2 %f\n", len );
            success = false;
        }
    }

    // Dot
    {

        float dot = upVector.Dot( rightVector );
        if ( !XMScalarNearEqual( dot, 0.f, EPSILON ) )
        {
            printf("ERROR: dot %f\n", dot );
            success = false;
        }

        dot = v1.Dot( v2 );

        if ( !XMScalarNearEqual( dot, 14.f, EPSILON ) )
        {
            printf("ERROR: dot %f ... 14\n", dot );
            success = false;
        }
    }

    // Cross
    v = v1.Cross(v2);
    if ( v != Vector2( -3, -3 ) )
    {
        printf("ERROR: cross %f %f ... -3 -3\n", v.x, v.y );
        success = false;
    }

    v1.Cross(v2, v);
    if (v != Vector2(-3, -3))
    {
        printf("ERROR: cross(2) %f %f ... -3 -3\n", v.x, v.y);
        success = false;
    }

    // Normalize
    v2.Normalize( v );
    if ( !XMVector2NearEqual(v, Vector2( 0.624695f, 0.780869f ), VEPSILON ) )
    {
        printf("ERROR: norm %f %f ... 0.624695 0.780869\n", v.x, v.y );
        success = false;
    }

    v = v2;
    v.Normalize();
    if (!XMVector2NearEqual(v, Vector2(0.624695f, 0.780869f), VEPSILON))
    {
        printf("ERROR: norm(2) %f %f ... 0.624695 0.780869\n", v.x, v.y);
        success = false;
    }

    // Clamp
    v3.Clamp( v1, v2, v );
    if ( v != Vector2(3,2) )
    {
        printf("ERROR: clamp %f %f ... 3 2\n", v.x, v.y );
        success = false;
    }

    v = v3;
    v.Clamp(v1, v2);
    if (v != Vector2(3, 2))
    {
        printf("ERROR: clamp(2) %f %f ... 3 2\n", v.x, v.y);
        success = false;
    }

    // Distance/DistanceSquared
    {
        float dist = Vector2::Distance( v1, v2 );
        if ( !XMScalarNearEqual( dist, 4.242640f, EPSILON ) )
        {
            printf("ERROR: dist %f\n", dist );
            success = false;
        }

        dist = Vector2::DistanceSquared( v1, v2 );
        if ( !XMScalarNearEqual( dist, 18, EPSILON ) )
        {
            printf("ERROR: dist^2 %f\n", dist );
            success = false;
        }
    }
    
    // Min
    {
        Vector2 a(-1.f, 4.f);
        Vector2 b(2.f, 1.f);
        Vector2 result(-1.f, 1.f);

        v = Vector2::Min(a, b);
        if (v != result)
        {
            printf("ERROR: min %f %f ... -1, 1\n", v.x, v.y);
            success = false;
        }

        Vector2::Min(a, b, v);
        if (v != result)
        {
            printf("ERROR: min(2) %f %f ... -1, 1\n", v.x, v.y);
            success = false;
        }
    }

    // Max
    {
        Vector2 a(-1.f, 4.f);
        Vector2 b(2.f, 1.f);
        Vector2 result(2.f, 4.f);

        v = Vector2::Max(a, b);
        if (v != result)
        {
            printf("ERROR: max %f %f ... 2, 4\n", v.x, v.y);
            success = false;
        }

        Vector2::Max(a, b, v);
        if (v != result)
        {
            printf("ERROR: max(2) %f %f ... 2, 4\n", v.x, v.y);
            success = false;
        }
    }

    // Lerp
    {
        Vector2 a(1.f, 2.f);
        Vector2 b(3.f, 4.f);
        
        Vector2 result(2.f, 3.f);
        v = Vector2::Lerp(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: lerp 0.5 %f %f ... 2, 3\n", v.x, v.y);
            success = false;
        }

        Vector2::Lerp(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: lerp(2) 0.5 %f %f ... 2, 3\n", v.x, v.y);
            success = false;
        }

        v = Vector2::Lerp(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: lerp 0 %f %f ... 1, 2\n", v.x, v.y);
            success = false;
        }

        Vector2::Lerp(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: lerp(2) 0 %f %f ... 1, 2\n", v.x, v.y);
            success = false;
        }

        v = Vector2::Lerp(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: lerp 1 %f %f ... 3, 4\n", v.x, v.y);
            success = false;
        }

        Vector2::Lerp(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: lerp(2) 1 %f %f ... 3, 4\n", v.x, v.y);
            success = false;
        }
    }

    // SmoothStep
    {
        Vector2 a(1.f, -2.f);
        Vector2 b(-5.f, 6.f);
        Vector2 result(-2.f, 2.f);

        // 0.5
        v = Vector2::SmoothStep(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0.25
        result.x = 0.0625f; result.y = -0.75f;
        v = Vector2::SmoothStep(a, b, 0.25f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.25 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 0.25f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.25 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0.75
        result.x = -4.0625f; result.y = 4.75f;
        v = Vector2::SmoothStep(a, b, 0.75f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.75 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 0.75f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.75 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0
        v = Vector2::SmoothStep(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: smoothstep 0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) 0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 1
        v = Vector2::SmoothStep(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: smoothstep 1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) 1 %f %f\n", v.x, v.y);
            success = false;
        }

        // less than 0
        v = Vector2::SmoothStep(a, b, -1.f);
        if (v != a)
        {
            printf("ERROR: smoothstep <0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, -1.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) <0 %f %f\n", v.x, v.y);
            success = false;
        }

        // greater than 1
        v = Vector2::SmoothStep(a, b, 2.f);
        if (v != b)
        {
            printf("ERROR: smoothstep >1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::SmoothStep(a, b, 2.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) >1 %f %f\n", v.x, v.y);
            success = false;
        }
    }

    // Barycentric
    {
        Vector2 value1(1.0f, 12.0f);
        Vector2 value2(21.0f, 22.0f);
        Vector2 value3(31.0f, 32.0f);

        // 0 0
        v = Vector2::Barycentric(value1, value2, value3, 0.f, 0.f);
        if (v != value1)
        {
            printf("ERROR: bary 0,0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Barycentric(value1, value2, value3, 0.f, 0.f, v);
        if (v != value1)
        {
            printf("ERROR: bary(2) 0,0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 1 0
        v = Vector2::Barycentric(value1, value2, value3, 1.f, 0.f);
        if (v != value2)
        {
            printf("ERROR: bary 1,0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Barycentric(value1, value2, value3, 1.f, 0.f, v);
        if (v != value2)
        {
            printf("ERROR: bary(2) 1,0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0 1
        v = Vector2::Barycentric(value1, value2, value3, 0.f, 1.f);
        if (v != value3)
        {
            printf("ERROR: bary 0,1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Barycentric(value1, value2, value3, 0.f, 1.f, v);
        if (v != value3)
        {
            printf("ERROR: bary(2) 0,1 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0.5 0.5
        Vector2 result = Vector2::Lerp(value2, value3, 0.5f);
        v = Vector2::Barycentric(value1, value2, value3, 0.5f, 0.5f);
        if (v != result)
        {
            printf("ERROR: bary 0.5,0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Barycentric(value1, value2, value3, 0.5f, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: bary(2) 0.5,0.5 %f %f\n", v.x, v.y);
            success = false;
        }
    }

    // CatmullRom
    {
        Vector2 position1(1.0f, 2.0f);
        Vector2 position2(-1.0f, 4.0f);
        Vector2 position3(2.0f, 6.0f);
        Vector2 position4(3.0f, 8.0f);

        // 0.5
        Vector2 result(0.3125f, 5.0f);
        v = Vector2::CatmullRom(position1, position2, position3, position4, 0.5f);
        if (v != result)
        {
            printf("ERROR: catmull 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::CatmullRom(position1, position2, position3, position4, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0
        v = Vector2::CatmullRom(position1, position2, position3, position4, 0.f);
        if (v != position2)
        {
            printf("ERROR: catmull 0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::CatmullRom(position1, position2, position3, position4, 0.f, v);
        if (v != position2)
        {
            printf("ERROR: catmull(2) 0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 1
        v = Vector2::CatmullRom(position1, position2, position3, position4, 1.f);
        if (v != position3)
        {
            printf("ERROR: catmull 1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::CatmullRom(position1, position2, position3, position4, 1.f, v);
        if (v != position3)
        {
            printf("ERROR: catmull(2) 1 %f %f\n", v.x, v.y);
            success = false;
        }

        // less than 0
        result = Vector2(8.0f, 2.0f);
        v = Vector2::CatmullRom(position1, position2, position3, position4, -1.f);
        if (v != result)
        {
            printf("ERROR: catmull <0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::CatmullRom(position1, position2, position3, position4, -1.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) <0 %f %f\n", v.x, v.y);
            success = false;
        }

        // greater than 1
        result = Vector2(-4.0f, 8.0f);
        v = Vector2::CatmullRom(position1, position2, position3, position4, 2.f);
        if (v != result)
        {
            printf("ERROR: catmull >1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::CatmullRom(position1, position2, position3, position4, 2.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) >1 %f %f\n", v.x, v.y);
            success = false;
        }
    }

    // Hermite
    {
        Vector2 p1(0.f, 1.f);
        Vector2 t1(0.f, tanf(XMConvertToRadians(30.f)));
        Vector2 p2(-2.f, 2.f);
        Vector2 t2(0.f, tanf(XMConvertToRadians(-5.f)));

        // 0.5
        Vector2 result(-1.0f, 1.583105f);
        v = Vector2::Hermite(p1, t1, p2, t2, 0.5f);
        if (v != result)
        {
            printf("ERROR: hermite 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0
        v = Vector2::Hermite(p1, t1, p2, t2, 0.f);
        if (v != p1)
        {
            printf("ERROR: hermite 0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 0.f, v);
        if (v != p1)
        {
            printf("ERROR: hermite(2) 0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 1
        v = Vector2::Hermite(p1, t1, p2, t2, 1.f);
        if (v != p2)
        {
            printf("ERROR: hermite 1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 1.f, v);
        if (v != p2)
        {
            printf("ERROR: hermite(2) 1 %f %f\n", v.x, v.y);
            success = false;
        }

        // <0
        result = Vector2(-10.0f, 3.86557627f);
        v = Vector2::Hermite(p1, t1, p2, t2, -1.f);
        if (v != result)
        {
            printf("ERROR: hermite <0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, -1.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) <0 %f %f\n", v.x, v.y);
            success = false;
        }

        // >1
        result = Vector2(8.0f, -2.19525433f);
        v = Vector2::Hermite(p1, t1, p2, t2, 2.f);
        if (v != result)
        {
            printf("ERROR: hermite >1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 2.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) >1 %f %f\n", v.x, v.y);
            success = false;
        }
    }

    // Reflect
    {
        Vector2 a(1.f, 1.f);
        a.Normalize();

        // XZ plane
        Vector2 n(0.f, 1.f);
        Vector2 result(a.x, -a.y);
        v = Vector2::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        // XY plane
        n = Vector2(0.f, 0.f);
        result = Vector2(a.x, a.y);
        v = Vector2::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        // YZ plane
        n = Vector2(1.f, 0.f);
        result = Vector2(-a.x, a.y);
        v = Vector2::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }
    }

    // Refract
    {
        Vector2 a(1.f, 1.f);
        a.Normalize();

        Vector2 n(0.f, 1.f);
        Vector2 result(0.940452f, -0.339926f);
        v = Vector2::Refract(a, n, 1.33f);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Refract(a, n, 1.33f, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }
    }

    // Transform (Matrix)
    {
        Vector2 vec(1.f, 2.f);
        XMMATRIX m = XMMatrixRotationX( XMConvertToRadians(30.f) ) *
                     XMMatrixRotationY( XMConvertToRadians(30.f) ) *
                     XMMatrixRotationZ( XMConvertToRadians(30.f) ) *
                     XMMatrixTranslation( 10.f, 20.f, 30.f );
        Vector2 result(10.316987f, 22.183012f);

        v = Vector2::Transform(vec, m);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Transform(vec, m, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector4 vec4;
        Vector4 result4(10.316987f, 22.183012f, 30.366026f, 1.f);
        Vector2::Transform(vec, m, vec4);
        if (!XMVector4NearEqual(vec4, result4, VEPSILON))
        {
            printf("ERROR: transmat(3) %f %f %f %f... %f %f %f %f\n", vec4.x, vec4.y, vec4.z, vec4.w, result4.x, result4.y, result4.z, result4.w);
            success = false;
        }
    }

    // Transform (Quaternion)
    {
        Vector2 vec(1.f, 2.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
                     XMMatrixRotationY(XMConvertToRadians(30.f)) *
                     XMMatrixRotationZ(XMConvertToRadians(30.f));
        XMVECTOR q = XMQuaternionRotationMatrix(m);

        Vector2 result = Vector2::Transform(vec, m);
        v = Vector2::Transform(vec, q);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::Transform(vec, q, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }
    }

    // TransformNormal
    {
        Vector2 vec(1.f, 2.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
                     XMMatrixRotationY(XMConvertToRadians(30.f)) *
                     XMMatrixRotationZ(XMConvertToRadians(30.f)) *
                     XMMatrixTranslation(10.f, 20.f, 30.f);
        Vector2 result(0.3169873f, 2.18301272f);

        v = Vector2::TransformNormal(vec, m);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transnorm %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }

        Vector2::TransformNormal(vec, m, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transnorm(2) %f %f ... %f %f\n", v.x, v.y, result.x, result.y);
            success = false;
        }
    }

    // Transform (Array)
    {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(10.f), XMConvertToRadians(20.f), XMConvertToRadians(30.f))
            * XMMatrixTranslation(10.f, 20.f, 30.f);

        static const Vector2 points [] = {
            Vector2(1.f, 2.f),
            Vector2(0.1f, 0.2f),
            Vector2(1.1f, 1.2f),
            Vector2(2.1f, 2.2f),
            Vector2(10, 20),
        };

        std::unique_ptr<Vector2[]> buff(new Vector2[_countof(points)]);

        Vector2::Transform(&points[0], _countof(points), m, buff.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector2 result = Vector2::Transform(points[j], m);
            v = buff[j];
            if (!XMVector2NearEqual(v, result, VEPSILON))
            {
                printf("ERROR: transarr %Iu - %f %f ... %f %f \n", j, v.x, v.y, result.x, result.y);
                success = false;
            }
        }

        std::unique_ptr<Vector4[]> buff4(new Vector4[_countof(points)]);

        Vector2::Transform(&points[0], _countof(points), m, buff4.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector4 result;
            Vector2::Transform(points[j], m, result);
            Vector4 vec4 = buff4[j];
            if (!XMVector4NearEqual(vec4, result, VEPSILON2))
            {
                printf("ERROR: transarr(2) %Iu - %f %f %f %f ... %f %f %f %f\n", j, vec4.x, vec4.y, vec4.z, vec4.w, result.x, result.y, result.z, result.w);
                success = false;
            }
        }
    }

    // TransformNormal (Array)
    {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(10.f), XMConvertToRadians(20.f), XMConvertToRadians(30.f))
            * XMMatrixTranslation(10.f, 20.f, 30.f);

        static const Vector2 points [] = {
            Vector2(1.f, 2.f),
            Vector2(0.1f, 0.2f),
            Vector2(1.1f, 1.2f),
            Vector2(2.1f, 2.2f),
            Vector2(10, 20),
        };

        std::unique_ptr<Vector2[]> buff(new Vector2[_countof(points)]);

        Vector2::TransformNormal(&points[0], _countof(points), m, buff.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector2 result = Vector2::TransformNormal(points[j], m);
            v = buff[j];
            if (!XMVector2NearEqual(v, result, VEPSILON))
            {
                printf("ERROR: transnormarr %Iu - %f %f ... %f %f \n", j, v.x, v.y, result.x, result.y);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 * v2;
    if ( v != Vector2( 4, 10 ) )
    {
        printf("ERROR: * %f %f ... 4 10\n", v.x, v.y );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector2( 2, 4 ) )
    {
        printf("ERROR: *f %f %f ... 2 4\n", v.x, v.y );
        success = false;
    }

    v = v1 / 2;
    if ( v != Vector2( 0.5f, 1.f ) )
    {
        printf("ERROR: / %f %f ... 0.5 1\n", v.x, v.y );
        success = false;
    }

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestV3()
{
	// Vector3
    bool success = true;

    Vector3 upVector( 0, 1.f, 0 );
    Vector3 rightVector( 1.f, 0, 0 );
    Vector3 v1( 1.f, 2.f, 3.f );
    Vector3 v2( 4.f, 5.f, 6.f );
    Vector3 v3( 3.f, -23.f, 100.f );

    if ( upVector == rightVector )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    if ( upVector != upVector )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    {
        XMFLOAT3 xm(6.f, -2.f, 7.f);
        Vector3 vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON))
        {
            printf("ERROR: XMFLOAT3 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT3 xm(6.f, -2.f, 7.f);
        Vector3 vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON))
        {
            printf("ERROR: XMFLOAT3 =\n");
            success = false;
        }
    }

    Vector3 zero(0.f, 0.f, 0.f);
    Vector3 one(1.f, 1.f, 1.f);
    Vector3 forwardVector(0.f, 0.f, -1.f);
    Vector3 backwardVector(0.f, 0.f, 1.f);
    Vector3 downVector(0, -1.f, 0);
    Vector3 leftVector(-1.f, 0, 0);
    if (zero != Vector3::Zero
        || one != Vector3::One
        || rightVector != Vector3::UnitX
        || upVector != Vector3::UnitY
        || backwardVector != Vector3::UnitZ
        || upVector != Vector3::Up
        || downVector != Vector3::Down
        || rightVector != Vector3::Right
        || leftVector != Vector3::Left
        || forwardVector != Vector3::Forward
        || backwardVector != Vector3::Backward )
    {
        printf("ERROR: constants\n");
        success = false;
    }

    Vector3 v = v1;
    if ( v != v1 )
    {
        printf("ERROR: =\n");
        success = false;
    }

    {
        Vector3 vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vc.z, Colors::CornflowerBlue.f[2], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Vector3 vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vt.z, Colors::MidnightBlue.f[2], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    v += v2;
    if ( v != Vector3(5,7,9) )
    {
        printf("ERROR: += %f %f %f ... 5 7 9\n", v.x, v.y, v.z );
        success = false;
    }

    v -= v1;
    if ( v != Vector3(4,5,6) )
    {
        printf("ERROR: -= %f %f %f ... 4 5 6\n", v.x, v.y, v.z );
        success = false;
    }

    v *= v1;
    if ( v != Vector3(4,10,18) )
    {
        printf("ERROR: *= %f %f %f ... 4 10 18\n", v.x, v.y, v.z );
        success = false;
    }

    // InBounds
    if ( !v1.InBounds(v2)
         || v2.InBounds(v1) )
    {
        printf("ERROR: InBounds\n");
        success = false;
    }

    // Length/LengthSquared
    {
        float len = v2.Length();

        if ( !XMScalarNearEqual( len, 8.774964f, EPSILON ) )
        {
            printf("ERROR: len %f\n", len );
            success = false;
        }

        len = v2.LengthSquared();

        if ( !XMScalarNearEqual( len, 77, EPSILON ) )
        {
            printf("ERROR: len^2 %f\n", len );
            success = false;
        }
    }

    // Dot
    {

        float dot = upVector.Dot( rightVector );
        if ( !XMScalarNearEqual( dot, 0.f, EPSILON ) )
        {
            printf("ERROR: dot %f\n", dot );
            success = false;
        }

        dot = v1.Dot( v2 );

        if ( !XMScalarNearEqual( dot, 32.f, EPSILON ) )
        {
            printf("ERROR: dot %f ... 32\n", dot );
            success = false;
        }
    }

    // Cross
    v = v1.Cross(v2);
    if ( v != Vector3( -3.f, 6.f, -3.f ) )
    {
        printf("ERROR: cross %f %f %f ... 0.5 1 1.5\n", v.x, v.y, v.z );
        success = false;
    }

    v1.Cross(v2, v);
    if (v != Vector3(-3.f, 6.f, -3.f))
    {
        printf("ERROR: cross(2) %f %f %f ... 0.5 1 1.5\n", v.x, v.y, v.z);
        success = false;
    }

    // Normalize
    v2.Normalize( v );
    if ( !XMVector3NearEqual(v, Vector3( 0.455842f, 0.569803f, 0.683763f ), VEPSILON ) )
    {
        printf("ERROR: norm %f %f %f ... 0.455842 0.569803 0.683763\n", v.x, v.y, v.z );
        success = false;
    }

    v = v2;
    v.Normalize();
    if (!XMVector3NearEqual(v, Vector3(0.455842f, 0.569803f, 0.683763f), VEPSILON))
    {
        printf("ERROR: norm(2) %f %f %f ... 0.455842 0.569803 0.683763\n", v.x, v.y, v.z);
        success = false;
    }

    // Clamp
    v3.Clamp( v1, v2, v );
    if ( v != Vector3(3,2,6) )
    {
        printf("ERROR: clamp %f %f %f ... 3 2 6\n", v.x, v.y, v.z );
        success = false;
    }

    v = v3;
    v.Clamp(v1, v2);
    if (v != Vector3(3, 2, 6))
    {
        printf("ERROR: clamp(2) %f %f %f ... 3 2 6\n", v.x, v.y, v.z);
        success = false;
    }

    // Distance/DistanceSquared
    {
        float dist = Vector3::Distance( v1, v2 );
        if ( !XMScalarNearEqual( dist, 5.196152f, EPSILON ) )
        {
            printf("ERROR: dist %f\n", dist );
            success = false;
        }

        dist = Vector3::DistanceSquared( v1, v2 );
        if ( !XMScalarNearEqual( dist, 27.f, EPSILON ) )
        {
            printf("ERROR: dist^2 %f\n", dist );
            success = false;
        }
    }
    
    // Min
    {
        Vector3 a(-1.f, 4.f, -3.f);
        Vector3 b(2.f, 1.f, -1.f );
        Vector3 result(-1.f, 1.f, -3.f);

        v = Vector3::Min(a, b);
        if (v != result)
        {
            printf("ERROR: min %f %f %f ... -1, 1, -3\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Min(a, b, v);
        if (v != result)
        {
            printf("ERROR: min(2) %f %f %f ... -1, 1, -3\n", v.x, v.y, v.z);
            success = false;
            success = false;
        }
    }

    // Max
    {
        Vector3 a(-1.f, 4.f, -3.f);
        Vector3 b(2.f, 1.f, -1.f);
        Vector3 result(2.0f, 4.0f, -1.0f);

        v = Vector3::Max(a, b);
        if (v != result)
        {
            printf("ERROR: max %f %f %f ... 2, 4, -1\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Max(a, b, v);
        if (v != result)
        {
            printf("ERROR: max(2) %f %f %f ... 2, 4, -1\n", v.x, v.y, v.z);
            success = false;
            success = false;
        }
    }

    // Lerp
    {
        Vector3 a(1.f, 2.f, 3.f);
        Vector3 b(4.f, 5.f, 6.f);

        Vector3 result(2.5f, 3.5f, 4.5f);
        v = Vector3::Lerp(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: lerp 0.5 %f %f %f ... 2.5, 3.5, 4.5\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Lerp(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: lerp(2) 0.5 %f %f %f ... 2.5, 3.5, 4.5\n", v.x, v.y, v.z);
            success = false;
        }

        v = Vector3::Lerp(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: lerp 0 %f %f %f ... 1, 2, 3\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Lerp(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: lerp(2) 0 %f %f %f ... 1, 2,3\n", v.x, v.y, v.z);
            success = false;
        }

        v = Vector3::Lerp(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: lerp 1 %f %f %f ... 4, 5, 6\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Lerp(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: lerp(2) 1 %f %f %f ... 4, 5, 6\n", v.x, v.y, v.z);
            success = false;
        }
    }

    // SmoothStep
    {
        Vector3 a(1.f, -2.f, 3.f);
        Vector3 b(-5.f, 6.f, -7.f);
        Vector3 result(-2.f, 2.f, -2.f);

        // 0.5
        v = Vector3::SmoothStep(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0.25
        result = Vector3(0.0625f, -0.75f, 1.4375f);
        v = Vector3::SmoothStep(a, b, 0.25f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.25 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 0.25f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.25 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0.75
        result = Vector3(-4.0625f, 4.75f, -5.4375f);
        v = Vector3::SmoothStep(a, b, 0.75f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.75 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 0.75f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.75 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0
        v = Vector3::SmoothStep(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: smoothstep 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 1
        v = Vector3::SmoothStep(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: smoothstep 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // less than 0
        v = Vector3::SmoothStep(a, b, -1.f);
        if (v != a)
        {
            printf("ERROR: smoothstep <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, -1.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // greater than 1
        v = Vector3::SmoothStep(a, b, 2.f);
        if (v != b)
        {
            printf("ERROR: smoothstep >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::SmoothStep(a, b, 2.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }
    }

    // Barycentric
    {
        Vector3 value1(11.0f, 12.0f, 13.0f);
        Vector3 value2(21.0f, 22.0f, 23.0f);
        Vector3 value3(31.0f, 32.0f, 33.0f);

        // 0 0
        v = Vector3::Barycentric(value1, value2, value3, 0.f, 0.f);
        if (v != value1)
        {
            printf("ERROR: bary 0,0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Barycentric(value1, value2, value3, 0.f, 0.f, v);
        if (v != value1)
        {
            printf("ERROR: bary(2) 0,0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 1 0
        v = Vector3::Barycentric(value1, value2, value3, 1.f, 0.f);
        if (v != value2)
        {
            printf("ERROR: bary 1,0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Barycentric(value1, value2, value3, 1.f, 0.f, v);
        if (v != value2)
        {
            printf("ERROR: bary(2) 1,0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0 1
        v = Vector3::Barycentric(value1, value2, value3, 0.f, 1.f);
        if (v != value3)
        {
            printf("ERROR: bary 0,1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Barycentric(value1, value2, value3, 0.f, 1.f, v);
        if (v != value3)
        {
            printf("ERROR: bary(2) 0,1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0.5 0.5
        Vector3 result = Vector3::Lerp(value2, value3, 0.5f);
        v = Vector3::Barycentric(value1, value2, value3, 0.5f, 0.5f);
        if (v != result)
        {
            printf("ERROR: bary 0.5,0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Barycentric(value1, value2, value3, 0.5f, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: bary(2) 0.5,0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }
    }

    // CatmullRom
    {
        Vector3 position1(1.0f, 2.0f, 5.f);
        Vector3 position2(-1.0f, 4.0f, 4.f);
        Vector3 position3(2.0f, 6.0f, 3.f);
        Vector3 position4(3.0f, 8.0f, 2.f);

        // 0.5
        Vector3 result(0.3125f, 5.0f, 3.5f);
        v = Vector3::CatmullRom(position1, position2, position3, position4, 0.5f);
        if (v != result)
        {
            printf("ERROR: catmull 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::CatmullRom(position1, position2, position3, position4, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0
        v = Vector3::CatmullRom(position1, position2, position3, position4, 0.f);
        if (v != position2)
        {
            printf("ERROR: catmull 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::CatmullRom(position1, position2, position3, position4, 0.f, v);
        if (v != position2)
        {
            printf("ERROR: catmull(2) 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 1
        v = Vector3::CatmullRom(position1, position2, position3, position4, 1.f);
        if (v != position3)
        {
            printf("ERROR: catmull 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::CatmullRom(position1, position2, position3, position4, 1.f, v);
        if (v != position3)
        {
            printf("ERROR: catmull(2) 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // less than 0
        result = Vector3(8.0f, 2.0f,5.f);
        v = Vector3::CatmullRom(position1, position2, position3, position4, -1.f);
        if (v != result)
        {
            printf("ERROR: catmull <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::CatmullRom(position1, position2, position3, position4, -1.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // greater than 1
        result = Vector3(-4.0f, 8.0f, 2.f);
        v = Vector3::CatmullRom(position1, position2, position3, position4, 2.f);
        if (v != result)
        {
            printf("ERROR: catmull >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::CatmullRom(position1, position2, position3, position4, 2.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }
    }

    // Hermite
    {
        Vector3 p1(0.f, 1.f, 2.0f);
        Vector3 t1(0.f, tanf(XMConvertToRadians(30.f)), tanf(XMConvertToRadians(20.f)));
        Vector3 p2(-2.f, 2.f, 5.f);
        Vector3 t2(0.f, tanf(XMConvertToRadians(-5.f)), tanf(XMConvertToRadians(-4.f)));

        // 0.5
        Vector3 result(-1.0f, 1.583105f, 3.55423713f);
        v = Vector3::Hermite(p1, t1, p2, t2, 0.5f);
        if (v != result)
        {
            printf("ERROR: hermite 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0
        v = Vector3::Hermite(p1, t1, p2, t2, 0.f);
        if (v != p1)
        {
            printf("ERROR: hermite 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 0.f, v);
        if (v != p1)
        {
            printf("ERROR: hermite(2) 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 1
        v = Vector3::Hermite(p1, t1, p2, t2, 1.f);
        if (v != p2)
        {
            printf("ERROR: hermite 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 1.f, v);
        if (v != p2)
        {
            printf("ERROR: hermite(2) 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // <0
        result = Vector3(-10.0f, 3.86557627f, 15.6839724f);
        v = Vector3::Hermite(p1, t1, p2, t2, -1.f);
        if (v != result)
        {
            printf("ERROR: hermite <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, -1.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // >1
        result = Vector3(8.0f, -2.19525433f, -9.551766f);
        v = Vector3::Hermite(p1, t1, p2, t2, 2.f);
        if (v != result)
        {
            printf("ERROR: hermite >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 2.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }
    }

    // Reflect
    {
        Vector3 a(1.f, 1.f, 1.f);
        a.Normalize();

        // XZ plane
        Vector3 n(0.f, 1.f, 0.f);
        Vector3 result(a.x, -a.y, a.z);
        v = Vector3::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        // XY plane
        n = Vector3(0.f, 0.f, 1.f);
        result = Vector3(a.x, a.y, -a.z);
        v = Vector3::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        // YZ plane
        n = Vector3(1.f, 0.f, 0.f);
        result = Vector3(-a.x, a.y, a.z);
        v = Vector3::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }
    }

    // Refract
    {
        Vector3 a(-.5f, -5.f, 0.f);
        a.Normalize();

        Vector3 n(0.f, 1.f, 0.f);
        Vector3 result(-0.132340f, -0.991204f, 0.f);
        v = Vector3::Refract(a, n, 1.33f);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Refract(a, n, 1.33f, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }
    }

    // Transform (Matrix)
    {
        Vector3 vec(1.f, 2.f, 3.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
            XMMatrixRotationY(XMConvertToRadians(30.f)) *
            XMMatrixRotationZ(XMConvertToRadians(30.f)) *
            XMMatrixTranslation(10.f, 20.f, 30.f);
        Vector3 result(12.191987f, 21.533493f, 32.616024f);

        v = Vector3::Transform(vec, m);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Transform(vec, m, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector4 vec4;
        Vector4 result4(12.191987f, 21.533493f, 32.616024f, 1.f);
        Vector3::Transform(vec, m, vec4);
        if (!XMVector4NearEqual(vec4, result4, VEPSILON))
        {
            printf("ERROR: transmat(3) %f %f %f %f... %f %f %f %f\n", vec4.x, vec4.y, vec4.z, vec4.w, result4.x, result4.y, result4.z, result4.w);
            success = false;
        }
    }

    // Transform (Quaternion)
    {
        Vector3 vec(1.f, 2.f, 3.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
            XMMatrixRotationY(XMConvertToRadians(30.f)) *
            XMMatrixRotationZ(XMConvertToRadians(30.f));
        XMVECTOR q = XMQuaternionRotationMatrix(m);

        Vector3 result = Vector3::Transform(vec, m);
        v = Vector3::Transform(vec, q);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::Transform(vec, q, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }
    }

    // TransformNormal
    {
        Vector3 vec(1.f, 2.f, 3.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
            XMMatrixRotationY(XMConvertToRadians(30.f)) *
            XMMatrixRotationZ(XMConvertToRadians(30.f)) *
            XMMatrixTranslation(10.f, 20.f, 30.f);
        Vector3 result(2.19198728f, 1.53349364f, 2.61602545f);

        v = Vector3::TransformNormal(vec, m);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transnorm %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }

        Vector3::TransformNormal(vec, m, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transnorm(2) %f %f %f ... %f %f %f\n", v.x, v.y, v.z, result.x, result.y, result.z);
            success = false;
        }
    }

    // Transform (Array)
    {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(10.f), XMConvertToRadians(20.f), XMConvertToRadians(30.f))
            * XMMatrixTranslation(10.f, 20.f, 30.f);

        static const Vector3 points [] = {
            Vector3(1.f, 2.f, 3.f),
            Vector3(0.1f, 0.2f, 0.3f),
            Vector3(1.1f, 1.2f, 1.3f),
            Vector3(2.1f, 2.2f, 2.3f),
            Vector3(10, 20, 30),
        };

        std::unique_ptr<Vector3[]> buff(new Vector3[_countof(points)]);

        Vector3::Transform(&points[0], _countof(points), m, buff.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector3 result = Vector3::Transform(points[j], m);
            v = buff[j];
            if (!XMVector3NearEqual(v, result, VEPSILON))
            {
                printf("ERROR: transarr %Iu - %f %f %f ... %f %f %f\n", j, v.x, v.y, v.z, result.x, result.y, result.z);
                success = false;
            }
        }

        std::unique_ptr<Vector4[]> buff4(new Vector4[_countof(points)]);

        Vector3::Transform(&points[0], _countof(points), m, buff4.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector4 result;
            Vector3::Transform(points[j], m, result);
            Vector4 vec4 = buff4[j];
            if (!XMVector4NearEqual(vec4, result, VEPSILON2))
            {
                printf("ERROR: transarr(2) %Iu - %f %f %f %f ... %f %f %f %f\n", j, vec4.x, vec4.y, vec4.z, vec4.w, result.x, result.y, result.z, result.w);
                success = false;
            }
        }
    }

    // TransformNormal (Array)
    {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(10.f), XMConvertToRadians(20.f), XMConvertToRadians(30.f))
            * XMMatrixTranslation(10.f, 20.f, 30.f);

        static const Vector3 points [] = {
            Vector3(1.f, 2.f, 3.f),
            Vector3(0.1f, 0.2f, 0.3f),
            Vector3(1.1f, 1.2f, 1.3f),
            Vector3(2.1f, 2.2f, 2.3f),
            Vector3(10, 20, 30),
        };

        std::unique_ptr<Vector3[]> buff(new Vector3[_countof(points)]);

        Vector3::TransformNormal(&points[0], _countof(points), m, buff.get());

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector3 result = Vector3::TransformNormal(points[j], m);
            v = buff[j];
            if (!XMVector3NearEqual(v, result, VEPSILON))
            {
                printf("ERROR: transnormarr %Iu - %f %f %f ... %f %f %f\n", j, v.x, v.y, v.z, result.x, result.y, result.z);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 * v2;
    if ( v != Vector3( 4, 10, 18) )
    {
        printf("ERROR: * %f %f %f ... 4 10 8\n", v.x, v.y, v.z );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector3( 2, 4, 6) )
    {
        printf("ERROR: *f %f %f %f ... 2 4 6\n", v.x, v.y, v.z );
        success = false;
    }

    v = v1 / 2;
    if ( v != Vector3( 0.5f, 1.f, 1.5f) )
    {
        printf("ERROR: / %f %f %f ... 0.5 1 1.5\n", v.x, v.y, v.z );
        success = false;
    }


	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestV4()
{
	// Vector4
    bool success = true;

    Vector4 upVector( 0, 1.f, 0, 0 );
    Vector4 rightVector( 1.f, 0, 0, 0 );
    Vector4 v1( 1.f, 2.f, 3.f, 4.f );
    Vector4 v2( 4.f, 5.f, 6.f, 7.f );
    Vector4 v3( 3.f, -23.f, 100.f, 0.f );

    if ( upVector == rightVector )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    if ( upVector != upVector )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Vector4 vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Vector4 vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 =\n");
            success = false;
        }
    }

    Vector4 zero(0.f, 0.f, 0.f, 0.f);
    Vector4 one(1.f, 1.f, 1.f, 1.f);
    Vector4 backwardVector(0.f, 0.f, 1.f, 0.f);
    Vector4 wVector(0.f, 0.f, 0.f, 1.f);
    if (zero != Vector4::Zero
        || one != Vector4::One
        || rightVector != Vector4::UnitX
        || upVector != Vector4::UnitY
        || backwardVector != Vector4::UnitZ
        || wVector != Vector4::UnitW)
    {
        printf("ERROR: constants\n");
        success = false;
    }

    Vector4 v = v1;
    if ( v != v1 )
    {
        printf("ERROR: =\n");
        success = false;
    }

    {
        Vector4 vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vc.z, Colors::CornflowerBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vc.w, Colors::CornflowerBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Vector4 vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vt.z, Colors::MidnightBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vt.w, Colors::MidnightBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    v += v2;
    if ( v != Vector4(5,7,9,11) )
    {
        printf("ERROR: += %f %f %f %f ... 5 7 9 11\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v -= v1;
    if ( v != Vector4(4,5,6,7) )
    {
        printf("ERROR: -= %f %f %f %f ... 4 5 6 7\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v *= v1;
    if ( v != Vector4(4,10,18,28) )
    {
        printf("ERROR: *= %f %f %f %f ... 4 10 18 28\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    // InBounds
    if (!v1.InBounds(v2)
        || v2.InBounds(v1))
    {
        printf("ERROR: InBounds\n");
        success = false;
    }

    // Length/LengthSquared
    {
        float len = v2.Length();

        if ( !XMScalarNearEqual( len, 11.224972f, EPSILON ) )
        {
            printf("ERROR: len %f\n", len );
            success = false;
        }

        len = v2.LengthSquared();

        if ( !XMScalarNearEqual( len, 126, EPSILON ) )
        {
            printf("ERROR: len^2 %f\n", len );
            success = false;
        }
    }

    // Dot
    {

        float dot = upVector.Dot( rightVector );
        if ( !XMScalarNearEqual( dot, 0.f, EPSILON ) )
        {
            printf("ERROR: dot %f\n", dot );
            success = false;
        }

        dot = v1.Dot( v2 );

        if ( !XMScalarNearEqual( dot, 60.f, EPSILON ) )
        {
            printf("ERROR: dot %f ... 60\n", dot );
            success = false;
        }
    }

    // Cross
    v = v1.Cross(v2,v3);
    if ( v != Vector4( 669, -891, -225, 447 ) )
    {
        printf("ERROR: cross %f %f %f %f ... 669, -891, -225, 447\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v1.Cross(v2, v3, v);
    if (v != Vector4(669, -891, -225, 447))
    {
        printf("ERROR: cross(2) %f %f %f %f ... 669, -891, -225, 447\n", v.x, v.y, v.z, v.w);
        success = false;
    }

    // Normalize
    v2.Normalize( v );
    if ( !XMVector4NearEqual(v, Vector4( 0.356348f, 0.445435f, 0.534522f, 0.623610f ), VEPSILON ) )
    {
        printf("ERROR: norm %f %f %f %f ... 0.356348 0.445435 0.534522 0.623610\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v2;
    v.Normalize();
    if (!XMVector4NearEqual(v, Vector4(0.356348f, 0.445435f, 0.534522f, 0.623610f), VEPSILON))
    {
        printf("ERROR: norm(2) %f %f %f %f ... 0.356348 0.445435 0.534522 0.623610\n", v.x, v.y, v.z, v.w);
        success = false;
    }

    // Clamp
    v3.Clamp( v1, v2, v );
    if ( v != Vector4(3,2,6,4) )
    {
        printf("ERROR: clamp %f %f %f %f ... 3 2 6 4\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v.Clamp(v1, v2);
    if (v != Vector4(3, 2, 6, 4))
    {
        printf("ERROR: clamp(2) %f %f %f %f ... 3 2 6 4\n", v.x, v.y, v.z, v.w);
        success = false;
    }

    // Distance/DistanceSquared
    {
        float dist = Vector4::Distance( v1, v2 );
        if ( !XMScalarNearEqual( dist, 6, EPSILON ) )
        {
            printf("ERROR: dist %f\n", dist );
            success = false;
        }

        dist = Vector4::DistanceSquared( v1, v2 );
        if ( !XMScalarNearEqual( dist, 36, EPSILON ) )
        {
            printf("ERROR: dist^2 %f\n", dist );
            success = false;
        }
    }

    // Min
    {
        Vector4 a(-1.f, 4.f, -3.f, 1000.0f);
        Vector4 b(2.f, 1.f, -1.f, 0.f);
        Vector4 result(-1.f, 1.f, -3.f, 0.f);

        v = Vector4::Min(a, b);
        if (v != result)
        {
            printf("ERROR: min %f %f %f %f ... -1, 1, -3, 0\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Min(a, b, v);
        if (v != result)
        {
            printf("ERROR: min(2) %f %f %f %f ... -1, 1, -3, 0\n", v.x, v.y, v.z, v.w);
            success = false;
            success = false;
        }
    }

    // Max
    {
        Vector4 a(-1.f, 4.f, -3.f, 1000.0f);
        Vector4 b(2.f, 1.f, -1.f, 0.f);
        Vector4 result(2.0f, 4.0f, -1.0f, 1000.0f);

        v = Vector4::Max(a, b);
        if (v != result)
        {
            printf("ERROR: max %f %f %f %f ... 2, 4, -1, 1000\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Max(a, b, v);
        if (v != result)
        {
            printf("ERROR: max(2) %f %f %f %f ... 2, 4, -1, 1000\n", v.x, v.y, v.z, v.w);
            success = false;
            success = false;
        }
    }

    // Lerp
    {
        Vector4 a(1.f, 2.f, 3.f, 4.f);
        Vector4 b(5.f, 6.f, 7.f, 8.f);

        Vector4 result(3.f, 4.f, 5.f, 6.f);
        v = Vector4::Lerp(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: lerp 0.5 %f %f %f %f ... 3, 4, 5, 6\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Lerp(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: lerp(2) 0.5 %f %f %f %f ... 3, 4, 5, 6\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        v = Vector4::Lerp(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: lerp 0 %f %f %f %f ... 1, 2, 3, 4\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Lerp(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: lerp(2) 0 %f %f %f %f ... 1, 2, 3, 4\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        v = Vector4::Lerp(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: lerp 1 %f %f %f %f ... 5, 6, 7, 8\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Lerp(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: lerp(2) 1 %f %f %f %f ... 5, 6, 7, 8\n", v.x, v.y, v.z, v.w);
            success = false;
        }
    }

    // SmoothStep
    {
        Vector4 a(1.f, -2.f, 3.f, 4.f);
        Vector4 b(-5.f, 6.f, -7.f, 8.f);
        Vector4 result(-2.f, 2.f, -2.f, 6.0);

        // 0.5
        v = Vector4::SmoothStep(a, b, 0.5f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0.25
        result = Vector4(0.0625f, -0.75f, 1.4375f, 4.625f);
        v = Vector4::SmoothStep(a, b, 0.25f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.25 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 0.25f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.25 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0.75
        result = Vector4(-4.0625f, 4.75f, -5.4375f, 7.375f);
        v = Vector4::SmoothStep(a, b, 0.75f);
        if (v != result)
        {
            printf("ERROR: smoothstep 0.75 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 0.75f, v);
        if (v != result)
        {
            printf("ERROR: smoothstep(2) 0.75 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0
        v = Vector4::SmoothStep(a, b, 0.f);
        if (v != a)
        {
            printf("ERROR: smoothstep 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 0.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 1
        v = Vector4::SmoothStep(a, b, 1.f);
        if (v != b)
        {
            printf("ERROR: smoothstep 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 1.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // less than 0
        v = Vector4::SmoothStep(a, b, -1.f);
        if (v != a)
        {
            printf("ERROR: smoothstep <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, -1.f, v);
        if (v != a)
        {
            printf("ERROR: smoothstep(2) <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // greater than 1
        v = Vector4::SmoothStep(a, b, 2.f);
        if (v != b)
        {
            printf("ERROR: smoothstep >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::SmoothStep(a, b, 2.f, v);
        if (v != b)
        {
            printf("ERROR: smoothstep(2) >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }
    }

    // Barycentric
    {
        Vector4 value1(11.0f, 12.0f, 13.0f, 14.f);
        Vector4 value2(21.0f, 22.0f, 23.0f, 24.f);
        Vector4 value3(31.0f, 32.0f, 33.0f, 34.f);

        // 0 0
        v = Vector4::Barycentric(value1, value2, value3, 0.f, 0.f);
        if (v != value1)
        {
            printf("ERROR: bary 0,0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Barycentric(value1, value2, value3, 0.f, 0.f, v);
        if (v != value1)
        {
            printf("ERROR: bary(2) 0,0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 1 0
        v = Vector4::Barycentric(value1, value2, value3, 1.f, 0.f);
        if (v != value2)
        {
            printf("ERROR: bary 1,0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Barycentric(value1, value2, value3, 1.f, 0.f, v);
        if (v != value2)
        {
            printf("ERROR: bary(2) 1,0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0 1
        v = Vector4::Barycentric(value1, value2, value3, 0.f, 1.f);
        if (v != value3)
        {
            printf("ERROR: bary 0,1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Barycentric(value1, value2, value3, 0.f, 1.f, v);
        if (v != value3)
        {
            printf("ERROR: bary(2) 0,1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0.5 0.5
        Vector4 result = Vector4::Lerp(value2, value3, 0.5f);
        v = Vector4::Barycentric(value1, value2, value3, 0.5f, 0.5f);
        if (v != result)
        {
            printf("ERROR: bary 0.5,0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Barycentric(value1, value2, value3, 0.5f, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: bary(2) 0.5,0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }
    }

    // CatmullRom
    {
        Vector4 position1(1.0f, 2.0f, 5.f, -1.0f);
        Vector4 position2(-1.0f, 4.0f, 4.f, -2.0f);
        Vector4 position3(2.0f, 6.0f, 3.f, -6.0f);
        Vector4 position4(3.0f, 8.0f, 2.f, -8.0f);

        // 0.5
        Vector4 result(0.3125f, 5.0f, 3.5f, -3.9375f);
        v = Vector4::CatmullRom(position1, position2, position3, position4, 0.5f);
        if (v != result)
        {
            printf("ERROR: catmull 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::CatmullRom(position1, position2, position3, position4, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0
        v = Vector4::CatmullRom(position1, position2, position3, position4, 0.f);
        if (v != position2)
        {
            printf("ERROR: catmull 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::CatmullRom(position1, position2, position3, position4, 0.f, v);
        if (v != position2)
        {
            printf("ERROR: catmull(2) 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 1
        v = Vector4::CatmullRom(position1, position2, position3, position4, 1.f);
        if (v != position3)
        {
            printf("ERROR: catmull 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::CatmullRom(position1, position2, position3, position4, 1.f, v);
        if (v != position3)
        {
            printf("ERROR: catmull(2) 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // less than 0
        result = Vector4(8.0f, 2.0f, 5.f, -6.0f);
        v = Vector4::CatmullRom(position1, position2, position3, position4, -1.f);
        if (v != result)
        {
            printf("ERROR: catmull <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::CatmullRom(position1, position2, position3, position4, -1.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // greater than 1
        result = Vector4(-4.0f, 8.0f, 2.f, -3.0f);
        v = Vector4::CatmullRom(position1, position2, position3, position4, 2.f);
        if (v != result)
        {
            printf("ERROR: catmull >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::CatmullRom(position1, position2, position3, position4, 2.f, v);
        if (v != result)
        {
            printf("ERROR: catmull(2) >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }
    }

    // Hermite
    {
        Vector4 p1(0.f, 1.f, 2.0f, 3.f);
        Vector4 t1(0.f, tanf(XMConvertToRadians(30.f)), tanf(XMConvertToRadians(20.f)), tanf(XMConvertToRadians(10.f)));
        Vector4 p2(-2.f, 2.f, 5.f, 1.5f);
        Vector4 t2(0.f, tanf(XMConvertToRadians(-5.f)), tanf(XMConvertToRadians(-4.f)), tanf(XMConvertToRadians(-3.f)));

        // 0.5
        Vector4 result(-1.0f, 1.583105f, 3.55423713f, 2.27859187f);
        v = Vector4::Hermite(p1, t1, p2, t2, 0.5f);
        if (v != result)
        {
            printf("ERROR: hermite 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0
        v = Vector4::Hermite(p1, t1, p2, t2, 0.f);
        if (v != p1)
        {
            printf("ERROR: hermite 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 0.f, v);
        if (v != p1)
        {
            printf("ERROR: hermite(2) 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 1
        v = Vector4::Hermite(p1, t1, p2, t2, 1.f);
        if (v != p2)
        {
            printf("ERROR: hermite 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 1.f, v);
        if (v != p2)
        {
            printf("ERROR: hermite(2) 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // <0
        result = Vector4(-10.0f, 3.86557627f, 15.6839724f, -5.10049248f);
        v = Vector4::Hermite(p1, t1, p2, t2, -1.f);
        if (v != result)
        {
            printf("ERROR: hermite <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, -1.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // >1
        result = Vector4(8.0f, -2.19525433f, -9.551766f, 9.143023f);
        v = Vector4::Hermite(p1, t1, p2, t2, 2.f);
        if (v != result)
        {
            printf("ERROR: hermite >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 2.f, v);
        if (v != result)
        {
            printf("ERROR: hermite(2) >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }
    }

    // Reflect
    {
        Vector4 a(1.f, 1.f, 1.f, 1.f);
        a.Normalize();

        // XZ plane
        Vector4 n(0.f, 1.f, 0.f, 0.f);
        Vector4 result(a.x, -a.y, a.z, a.w);
        v = Vector4::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        // XY plane
        n = Vector4(0.f, 0.f, 1.f, 0.f);
        result = Vector4(a.x, a.y, -a.z, a.w);
        v = Vector4::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        // YZ plane
        n = Vector4(1.f, 0.f, 0.f, 0.f);
        result = Vector4(-a.x, a.y, a.z, a.w);
        v = Vector4::Reflect(a, n);
        if (v != result)
        {
            printf("ERROR: reflect %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Reflect(a, n, v);
        if (v != result)
        {
            printf("ERROR: reflect(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }
    }

    // Refract
    {
        Vector4 a(-.5f, -5.f, 0.f, 0.f);
        a.Normalize();

        Vector4 n(0.f, 1.f, 0.f, 0.f);
        Vector4 result(-0.132340f, -0.991204f, 0.f, 0.f);
        v = Vector4::Refract(a, n, 1.33f);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Refract(a, n, 1.33f, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: refract %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }
    }

    // Transform (Matrix)
    {
        Vector4 vec(1.f, 2.f, 3.f, 0.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
            XMMatrixRotationY(XMConvertToRadians(30.f)) *
            XMMatrixRotationZ(XMConvertToRadians(30.f)) *
            XMMatrixTranslation(10.f, 20.f, 30.f);
        Vector4 result(2.19198728f, 1.53349376f, 2.61602545f, 0.0f);

        v = Vector4::Transform(vec, m);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec, m, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        vec.w = 1.f;
        result = Vector4(12.19198728f, 21.53349376f, 32.61602545f, 1.0f);
        v = Vector4::Transform(vec, m);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec, m, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transmat(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }
    }

    // Transform (Quaternion)
    {
        Vector4 vec(1.f, 2.f, 3.f, 0.f);
        XMMATRIX m = XMMatrixRotationX(XMConvertToRadians(30.f)) *
            XMMatrixRotationY(XMConvertToRadians(30.f)) *
            XMMatrixRotationZ(XMConvertToRadians(30.f));
        XMVECTOR q = XMQuaternionRotationMatrix(m);

        Vector4 result = Vector4::Transform(vec, m);
        v = Vector4::Transform(vec, q);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec, q, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        vec.w = 1.f;
        result.w = 1.f;
        v = Vector4::Transform(vec, q);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec, q, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(2) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector2 vec2(1.f, 2.f);
        Vector2::Transform(vec2, m, result);
        v = Vector4::Transform(vec2, q);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(3) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec2, q, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(4) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector3 vec3(1.f, 2.f, 3.f);
        Vector3::Transform(vec3, m, result);
        v = Vector4::Transform(vec3, q);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(5) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }

        Vector4::Transform(vec3, q, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: transquat(6) %f %f %f %f ... %f %f %f %f\n", v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
            success = false;
        }
    }

    // Transform (Array)
    {
        XMMATRIX m = XMMatrixRotationRollPitchYaw(XMConvertToRadians(10.f), XMConvertToRadians(20.f), XMConvertToRadians(30.f))
                     * XMMatrixTranslation(10.f, 20.f, 30.f);

        static const Vector4 points[] = {
            Vector4(1.f, 2.f, 3.f, 4.f),
            Vector4(0.1f, 0.2f, 0.3f, 1.f),
            Vector4(1.1f, 1.2f, 1.3f, 0.f),
            Vector4(2.1f, 2.2f, 2.3f, 1.f),
            Vector4(10, 20, 30, 1.f),
        };

        std::unique_ptr<Vector4[]> buff(new Vector4[_countof(points)]);
        
        Vector4::Transform( &points[0], _countof(points), m, buff.get() );

        for (size_t j = 0; j < _countof(points); ++j)
        {
            Vector4 result = Vector4::Transform(points[j], m);
            v = buff[j];
            if (!XMVector4NearEqual(v, result, VEPSILON2))
            {
                printf("ERROR: transarr %Iu - %f %f %f %f ... %f %f %f %f\n", j, v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 * v2;
    if ( v != Vector4( 4, 10, 18, 28) )
    {
        printf("ERROR: * %f %f %f %f ... 4 10 8 28\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector4( 2, 4, 6, 8) )
    {
        printf("ERROR: *f %f %f %f %f ... 2 4 6 8\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v1 / 2;
    if ( v != Vector4( 0.5f, 1.f, 1.5f, 2) )
    {
        printf("ERROR: / %f %f %f %f ... 0.5 1 1.5 x\n", v.x, v.y, v.z, v.w );
        success = false;
    }


	return (success) ? 0 : 1;
}



//-------------------------------------------------------------------------------------
int TestM()
{
	// Matrix
    bool success = 1;

    Matrix a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

    VerifyEqual(a._11, 1.f);
    VerifyEqual(a._12, 2.f);
    VerifyEqual(a._13, 3.f);
    VerifyEqual(a._14, 4.f);

    VerifyEqual(a._21, 5.f);
    VerifyEqual(a._22, 6.f);
    VerifyEqual(a._23, 7.f);
    VerifyEqual(a._24, 8.f);

    VerifyEqual(a._31, 9.f);
    VerifyEqual(a._32, 10.f);
    VerifyEqual(a._33, 11.f);
    VerifyEqual(a._34, 12.f);

    VerifyEqual(a._41, 13.f);
    VerifyEqual(a._42, 14.f);
    VerifyEqual(a._43, 15.f);
    VerifyEqual(a._44, 16.f);

    VerifyEqual(Matrix(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), a);
    VerifyEqual(Matrix(), Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9)), Matrix(1, 2, 3, 0, 4, 5, 6, 0, 7, 8, 9, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix(Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8), Vector4(9, 10, 11, 12), Vector4(13, 14, 15, 16)), a);

    float floatData[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    VerifyEqual(Matrix(floatData), a);

    VerifyEqual(Matrix(XMMATRIX(floatData)), a);

    XMMATRIX x = a;
    VerifyEqual(Matrix(x), a);

    VerifyEqual(a == Matrix(x), true);
    VerifyEqual(a == Matrix(), false);

    VerifyEqual(a != Matrix(x), false);
    VerifyEqual(a != Matrix(), true);

    Matrix z(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    VerifyEqual(z == z, true);
    VerifyEqual(z != Matrix(), true);

    Matrix c0(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4);
    Matrix c1(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 2, 1);

    VerifyEqual(c0 == c0, true);
    VerifyEqual(c1 == c1, true);
    VerifyEqual(c0 != c1, true);

    Matrix b;
    b = a;
    VerifyEqual(b, a);

    {
        XMFLOAT4X4 xm(6.f, -2.f, 7.f, 80.f,
                      4.f, -3.f, 2.f, 1.f,
                      125.f, 126.f, 127.f, 128.f,
                      0.1f, 0.2f, 0.3f, 0.4f);
        Matrix mi(xm);

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(xm._14, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(xm._24, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(xm._34, mi._34, EPSILON)
            || !XMScalarNearEqual(xm._41, mi._41, EPSILON)
            || !XMScalarNearEqual(xm._42, mi._42, EPSILON)
            || !XMScalarNearEqual(xm._43, mi._43, EPSILON)
            || !XMScalarNearEqual(xm._44, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT4X4 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4X4 xm(6.f, -2.f, 7.f, 80.f,
                      4.f, -3.f, 2.f, 1.f,
                      125.f, 126.f, 127.f, 128.f,
                      0.1f, 0.2f, 0.3f, 0.4f);
        Matrix mi;
        mi = xm;

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(xm._14, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(xm._24, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(xm._34, mi._34, EPSILON)
            || !XMScalarNearEqual(xm._41, mi._41, EPSILON)
            || !XMScalarNearEqual(xm._42, mi._42, EPSILON)
            || !XMScalarNearEqual(xm._43, mi._43, EPSILON)
            || !XMScalarNearEqual(xm._44, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT4X4 =\n");
            success = false;
        }
    }

    {
        XMFLOAT3X3 xm(6.f, -2.f, 7.f,
                      4.f, -3.f, 2.f,
                      125.f, 126.f, 127.f);
        Matrix mi(xm);

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(0.f, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(0.f, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(0.f, mi._34, EPSILON)
            || !XMScalarNearEqual(0.f, mi._41, EPSILON)
            || !XMScalarNearEqual(0.f, mi._42, EPSILON)
            || !XMScalarNearEqual(0.f, mi._43, EPSILON)
            || !XMScalarNearEqual(1.f, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT3X3 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT3X3 xm(6.f, -2.f, 7.f,
                      4.f, -3.f, 2.f,
                      125.f, 126.f, 127.f);
        Matrix mi;
        mi = xm;

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(0.f, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(0.f, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(0.f, mi._34, EPSILON)
            || !XMScalarNearEqual(0.f, mi._41, EPSILON)
            || !XMScalarNearEqual(0.f, mi._42, EPSILON)
            || !XMScalarNearEqual(0.f, mi._43, EPSILON)
            || !XMScalarNearEqual(1.f, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT3X3 =\n");
            success = false;
        }
    }

    {
        XMFLOAT4X3 xm(6.f, -2.f, 7.f,
                      4.f, -3.f, 2.f,
                      125.f, 126.f, 127.f,
                      0.1f, 0.2f, 0.3f);
        Matrix mi(xm);

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(0.f, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(0.f, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(0.f, mi._34, EPSILON)
            || !XMScalarNearEqual(xm._41, mi._41, EPSILON)
            || !XMScalarNearEqual(xm._42, mi._42, EPSILON)
            || !XMScalarNearEqual(xm._43, mi._43, EPSILON)
            || !XMScalarNearEqual(1.f, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT4X3 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4X3 xm(6.f, -2.f, 7.f,
                      4.f, -3.f, 2.f,
                      125.f, 126.f, 127.f,
                      0.1f, 0.2f, 0.3f);
        Matrix mi;
        mi = xm;

        if (!XMScalarNearEqual(xm._11, mi._11, EPSILON)
            || !XMScalarNearEqual(xm._12, mi._12, EPSILON)
            || !XMScalarNearEqual(xm._13, mi._13, EPSILON)
            || !XMScalarNearEqual(0.f, mi._14, EPSILON)
            || !XMScalarNearEqual(xm._21, mi._21, EPSILON)
            || !XMScalarNearEqual(xm._22, mi._22, EPSILON)
            || !XMScalarNearEqual(xm._23, mi._23, EPSILON)
            || !XMScalarNearEqual(0.f, mi._24, EPSILON)
            || !XMScalarNearEqual(xm._31, mi._31, EPSILON)
            || !XMScalarNearEqual(xm._32, mi._32, EPSILON)
            || !XMScalarNearEqual(xm._33, mi._33, EPSILON)
            || !XMScalarNearEqual(0.f, mi._34, EPSILON)
            || !XMScalarNearEqual(xm._41, mi._41, EPSILON)
            || !XMScalarNearEqual(xm._42, mi._42, EPSILON)
            || !XMScalarNearEqual(xm._43, mi._43, EPSILON)
            || !XMScalarNearEqual(1.f, mi._44, EPSILON) )
        {
            printf("ERROR: XMFLOAT4X3 =\n");
            success = false;
        }
    }

    b += Matrix(4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1);
    VerifyEqual(b, Matrix(5, 5, 5, 5, 9, 9, 9, 9, 13, 13, 13, 13, 17, 17, 17, 17));

    b -= Matrix(4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1);
    VerifyEqual(b, a);

    b *= Matrix();
    VerifyEqual(b, a);

    b *= Matrix(2, 0, 0, 0, 1, 1, 1, 1, 0, 0, -1, 0, 0, 0, 0, 1);
    VerifyEqual(b, Matrix(4, 2, -1, 6, 16, 6, -1, 14, 28, 10, -1, 22, 40, 14, -1, 30));

    b *= 2;
    VerifyEqual(b, Matrix(8, 4, -2, 12, 32, 12, -2, 28, 56, 20, -2, 44, 80, 28, -2, 60));

    b /= 2;
    VerifyEqual(b, Matrix(4, 2, -1, 6, 16, 6, -1, 14, 28, 10, -1, 22, 40, 14, -1, 30));
    
    b /= Matrix(2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2);
    VerifyEqual(b, Matrix(2, 1, -0.5f, 3, 8, 3, -0.5f, 7, 14, 5, -0.5f, 11, 20, 7, -0.5f, 15));

    VerifyEqual(+a, a);
    VerifyEqual(-a, Matrix(-1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15, -16));

    VerifyEqual(a.Up(), Vector3(5, 6, 7));
    VerifyEqual(a.Down(), Vector3(-5, -6, -7));
    VerifyEqual(a.Right(), Vector3(1, 2, 3));
    VerifyEqual(a.Left(), Vector3(-1, -2, -3));
    VerifyEqual(a.Backward(), Vector3(9, 10, 11));
    VerifyEqual(a.Forward(), Vector3(-9, -10, -11));
    VerifyEqual(a.Translation(), Vector3(13, 14, 15));

    b = Matrix();
    b.Up(Vector3(3, 6, 4));
    b.Right(Vector3(2, 5, 7));
    b.Backward(Vector3(8, 9, 10));
    b.Translation(Vector3(23, 42, 666));
    VerifyEqual(b, Matrix(2, 5, 7, 0, 3, 6, 4, 0, 8, 9, 10, 0, 23, 42, 666, 1));

    b.Down(Vector3(3, 6, 4));
    b.Left(Vector3(2, 5, 7));
    b.Forward(Vector3(8, 9, 10));
    VerifyEqual(b, Matrix(-2, -5, -7, 0, -3, -6, -4, 0, -8, -9, -10, 0, 23, 42, 666, 1));

    Vector3 scale;
    Quaternion rotation;
    Vector3 translation;

    Matrix c(1, 0, 0, 0,
             0, 0, 2, 0,
             0, 1, 0, 0,
             23, 42, 0, 1);

    VerifyEqual(c.Decompose(scale, rotation, translation), true);
    VerifyEqual(scale, Vector3(1, -2, 1));
    VerifyNearEqual(rotation, Quaternion(0.707107f, 0.000000f, 0.000000f, -0.707107f));
    VerifyEqual(translation, Vector3(23, 42, 0));
    VerifyEqual(translation, c.Translation());

    c.Transpose(b);
    VerifyEqual(b, Matrix(1, 0, 0, 23, 0, 0, 1, 42, 0, 2, 0, 0, 0, 0, 0, 1));
    VerifyEqual(c.Transpose(), Matrix(1, 0, 0, 23, 0, 0, 1, 42, 0, 2, 0, 0, 0, 0, 0, 1));

    c.Invert(b);
    VerifyEqual(b, Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, 0.5f, 0, 0, -23, 0, -42, 1));
    VerifyEqual(c.Invert(), Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, 0.5f, 0, 0, -23, 0, -42, 1));

    VerifyEqual(c.Determinant(), -2.f);

    VerifyEqual(Matrix::Identity, Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    
    VerifyEqual(Matrix::CreateTranslation(23, 42, 666), Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 23, 42, 666, 1));
    VerifyEqual(Matrix::CreateTranslation(Vector3(23, 42, 666)), Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 23, 42, 666, 1));
    
    VerifyEqual(Matrix::CreateScale(23), Matrix(23, 0, 0, 0, 0, 23, 0, 0, 0, 0, 23, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix::CreateScale(23, 42, 666), Matrix(23, 0, 0, 0, 0, 42, 0, 0, 0, 0, 666, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix::CreateScale(Vector3(23, 42, 666)), Matrix(23, 0, 0, 0, 0, 42, 0, 0, 0, 0, 666, 0, 0, 0, 0, 1));

    VerifyNearEqual(Matrix::CreateRotationX(XM_PIDIV2), Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateRotationY(XM_PIDIV2), Matrix(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateRotationZ(XM_PIDIV2), Matrix(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

    VerifyNearEqual(Matrix::CreateFromAxisAngle(Vector3(1, 0, 0), XM_PIDIV2), Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateFromAxisAngle(Vector3(0, 1, 0), XM_PIDIV2), Matrix(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateFromAxisAngle(Vector3(0, 0, 1), XM_PIDIV2), Matrix(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

    VerifyNearEqual(Matrix::CreatePerspectiveFieldOfView(XM_PIDIV2, 1, 1, 100), Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1.010101f, -1, 0, 0, -1.010101f, 0));
    VerifyNearEqual(Matrix::CreatePerspective(1, 1, 1, 100), Matrix(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, -1.010101f, -1, 0, 0, -1.010101f, 0));
    VerifyNearEqual(Matrix::CreatePerspectiveOffCenter(-1, 1, 1, -1, 1, 100), Matrix(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1.010101f, -1, 0, 0, -1.010101f, 0));
    VerifyNearEqual(Matrix::CreateOrthographic(1, 1, 1, 100), Matrix(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, -0.010101f, 0, 0, 0, -0.010101f, 1));
    VerifyNearEqual(Matrix::CreateOrthographicOffCenter(-1, 1, 1, -1, 1, 100), Matrix(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -0.010101f, 0, 0, 0, -0.010101f, 1));

    VerifyEqual(Matrix::CreateLookAt(Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0)), Matrix(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix::CreateWorld(Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0)), Matrix(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1));

    VerifyNearEqual(Matrix::CreateFromQuaternion(Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), XM_PIDIV2)), Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1));
    
    VerifyNearEqual(Matrix::CreateFromYawPitchRoll(0, XM_PIDIV2, 0), Matrix(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateFromYawPitchRoll(XM_PIDIV2, 0, 0), Matrix(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1));
    VerifyNearEqual(Matrix::CreateFromYawPitchRoll(0, 0, XM_PIDIV2), Matrix(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

    VerifyEqual(Matrix::CreateShadow(Vector3(0, -1, 0), Plane(0, -1, 0, 0)), Matrix(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
    VerifyEqual(Matrix::CreateReflection(Plane(0, 1, 0, 0)), Matrix(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

    VerifyEqual(Matrix::Lerp(a, c, 0.25f), Matrix(1, 1.5f, 2.25f, 3, 3.75f, 4.5f, 5.75f, 6, 6.75f, 7.75f, 8.25f, 9, 15.5f, 21, 11.25f, 12.25f));
    
    Matrix::Lerp(a, c, 0.25f, b);
    VerifyEqual(b, Matrix(1, 1.5f, 2.25f, 3, 3.75f, 4.5f, 5.75f, 6, 6.75f, 7.75f, 8.25f, 9, 15.5f, 21, 11.25f, 12.25f));

    VerifyNearEqual(Matrix::Transform(Matrix::CreateScale(2), Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), XM_PIDIV2)), Matrix(2, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 1));

    Matrix::Transform(Matrix::CreateScale(2), Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), XM_PIDIV2), b);
    VerifyNearEqual(b, Matrix(2, 0, 0, 0, 0, 0, 2, 0, 0, -2, 0, 0, 0, 0, 0, 1));

    VerifyEqual(a + Matrix(4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1), Matrix(5, 5, 5, 5, 9, 9, 9, 9, 13, 13, 13, 13, 17, 17, 17, 17));
    VerifyEqual(a - Matrix(4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1), Matrix(-3, -1, 1, 3, 1, 3, 5, 7, 5, 7, 9, 11, 9, 11, 13, 15));
    VerifyEqual(a * Matrix(), a);
    VerifyEqual(a * Matrix(2, 0, 0, 0, 1, 1, 1, 1, 0, 0, -1, 0, 0, 0, 0, 1), Matrix(4, 2, -1, 6, 16, 6, -1, 14, 28, 10, -1, 22, 40, 14, -1, 30));
    VerifyEqual(a * 2, Matrix(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32));
    VerifyEqual(2 * a, Matrix(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32));
    VerifyEqual(a / 2, Matrix(0.5f, 1, 1.5f, 2, 2.5f, 3, 3.5f, 4, 4.5f, 5, 5.5f, 6, 6.5f, 7, 7.5f, 8));
    VerifyEqual(a / Matrix(1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2), Matrix(1, 2, 3, 4, 2.5f, 3, 3.5f, 4, 9, 10, 11, 12, 6.5f, 7, 7.5f, 8));

    // CreateBillboard
    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 4.f, -5.f), cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(3.f, 4.f, -5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 4.f, 15.f), cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(0.f) * Matrix::CreateTranslation(3.f, 4.f, 15.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(13.f, 4.f, 5.f), cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(90.f)) * Matrix::CreateTranslation(13.f, 4.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(-7.f, 4.f, 5.f), cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(-7.f, 4.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 14.f, 5.f), cameraPosition, Vector3::Backward),
                        Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(3.f, 14.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, -6.f, 5.f), cameraPosition, Vector3::Backward),
                        Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(0.f) * Matrix::CreateTranslation(3.f, -6.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(13.f, 4.f, 5.f), cameraPosition, Vector3::Backward),
                        Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateTranslation(13.f, 4.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(-7.f, 4.f, 5.f), cameraPosition, Vector3::Backward),
                        Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(-7.f, 4.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 14.f, 5.f), cameraPosition, Vector3::Left),
                        Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(3.f, 14.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, -6.f, 5.f), cameraPosition, Vector3::Left),
                        Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateTranslation(3.f, -6.f, 5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 4.f, -5.f), cameraPosition, Vector3::Left),
                        Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(3.f, 4.f, -5.f));

        VerifyNearEqual(Matrix::CreateBillboard(Vector3(3.f, 4.f, 15.f), cameraPosition, Vector3::Left),
                        Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(0.f) * Matrix::CreateTranslation(3.f, 4.f, 15.f));

        VerifyNearEqual(Matrix::CreateBillboard(cameraPosition, cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z));

        VerifyNearEqual(Matrix::CreateBillboard(cameraPosition, cameraPosition, Vector3::Up, &Vector3::Right),
                        Matrix::CreateRotationY(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z));
    }

    // CreateConstrainedBillboard
    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 4.f, -5.f);
        auto expected = Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition += Vector3::Up * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition -= Vector3::Up * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 4.f, 15.f);
        auto expected = Matrix::CreateRotationY(0.f) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition += Vector3::Up * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition -= Vector3::Up * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(13.f, 4.f, 5.f);
        auto expected = Matrix::CreateRotationY(XMConvertToRadians(90.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition += Vector3::Up * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition -= Vector3::Up * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(-7.f, 4.f, 5.f);
        auto expected = Matrix::CreateRotationY(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition += Vector3::Up * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);

        cameraPosition -= Vector3::Up * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 14.f, 5.f);
        auto expected = Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(180.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition += Vector3::Backward * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition -= Vector3::Backward * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, -6.f, 5.f);
        auto expected = Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(0.f) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition += Vector3::Backward * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition -= Vector3::Backward * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(13.f, 4.f, 5.f);
        auto expected = Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(90.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition += Vector3::Backward * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition -= Vector3::Backward * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(-7.f, 4.f, 5.f);
        auto expected = Matrix::CreateRotationX(XMConvertToRadians(90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(-90.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition += Vector3::Backward * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);

        cameraPosition -= Vector3::Backward * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Backward), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 14.f, 5.f);
        auto expected = Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(-90.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition += Vector3::Left * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition -= Vector3::Left * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, -6.f, 5.f);
        auto expected = Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(90.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition += Vector3::Left * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition -= Vector3::Left * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 4.f, -5.f);
        auto expected = Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(XMConvertToRadians(180.f)) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition += Vector3::Left * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition -= Vector3::Left * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);
        Vector3 objectPosition(3.f, 4.f, 15.f);
        auto expected = Matrix::CreateRotationZ(XMConvertToRadians(90.f)) * Matrix::CreateRotationX(0.f) *  Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z);
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition += Vector3::Left * 10.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);

        cameraPosition -= Vector3::Left * 30.f;
        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Left), expected);
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(cameraPosition, cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z));
    }

    {
        Vector3 cameraPosition(3.f, 4.f, 5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(cameraPosition, cameraPosition, Vector3::Up, &Vector3::Right),
                        Matrix::CreateRotationY(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z));
    }

    {
        Vector3 objectPosition(3.f, 4.f, 5.f);
        Vector3 cameraPosition(3.f, 14.f, 5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up),
                        Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z));
    }

    {
        Vector3 objectPosition(3.f, 4.f, 5.f);
        Vector3 cameraPosition(3.f, 4.f, -5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Forward),
                        Matrix::CreateRotationX(XMConvertToRadians(-90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z));
    }

    {
        Vector3 objectPosition(3.f, 4.f, 5.f);
        Vector3 cameraPosition(3.f, 14.f, 5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up, nullptr, &Vector3::Forward),
                        Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z));
    }

    {
        Vector3 objectPosition(3.f, 4.f, 5.f);
        Vector3 cameraPosition(3.f, 14.f, 5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Up, nullptr, &Vector3::Up),
            Matrix::CreateRotationY(XMConvertToRadians(180.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z));
    }

    {
        Vector3 objectPosition(3.f, 4.f, 5.f);
        Vector3 cameraPosition(3.f, 4.f, -5.f);

        VerifyNearEqual(Matrix::CreateConstrainedBillboard(objectPosition, cameraPosition, Vector3::Forward, nullptr, &Vector3::Forward),
                        Matrix::CreateRotationX(XMConvertToRadians(-90.f)) * Matrix::CreateRotationZ(XMConvertToRadians(-90.f)) * Matrix::CreateTranslation(objectPosition.x, objectPosition.y, objectPosition.z));
    }

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestP()
{
	// Plane
    bool success = true;

    Plane a(1, 2, 3, 4);
    Plane b(Vector3(5, 6, 7), 8);
    
    VerifyEqual(a.x, 1.f);
    VerifyEqual(a.y, 2.f);
    VerifyEqual(a.z, 3.f);
    VerifyEqual(a.w, 4.f);

    VerifyEqual(b.x, 5.f);
    VerifyEqual(b.y, 6.f);
    VerifyEqual(b.z, 7.f);
    VerifyEqual(b.w, 8.f);

    VerifyEqual(Plane(), Plane(0, 1, 0, 0));
    VerifyEqual(Plane(Vector4(1, 2, 3, 4)), Plane(1, 2, 3, 4));
    VerifyEqual(Plane(XMVectorSet(1, 2, 3, 4)), Plane(1, 2, 3, 4));

    float floatValues[] = { 1, 2, 3, 4 };
    VerifyEqual(Plane(floatValues), Plane(1, 2, 3, 4));

    VerifyEqual(Plane(Vector3(0, 0, 23), Vector3(1, 0, 23), Vector3(0, 1, 23)), Plane(0, 0, 1, -23));
    VerifyEqual(Plane(Vector3(1, 2, 3), Vector3(0, 1, 0)), Plane(0, 1, 0, -2));

    XMVECTOR v = a;
    VerifyEqual(Plane(v), a);

    VerifyEqual(a == Plane(1, 2, 3, 4), true);
    VerifyEqual(a == b, false);

    VerifyEqual(a != Plane(1, 2, 3, 4), false);
    VerifyEqual(a != b, true);

    Plane c = a;
    VerifyEqual(c, a);
    c = b;
    VerifyEqual(c, b);

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Plane vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Plane vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 =\n");
            success = false;
        }
    }

    {
        Plane vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vc.z, Colors::CornflowerBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vc.w, Colors::CornflowerBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Plane vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vt.z, Colors::MidnightBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vt.w, Colors::MidnightBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    VerifyEqual(c.Normal(), Vector3(5, 6, 7));
    c.Normal(Vector3(0, 1, 2));
    VerifyEqual(c.Normal(), Vector3(0, 1, 2));

    VerifyEqual(c.D(), 8.f);
    c.D(23);
    VerifyEqual(c.D(), 23.f);

    c.Normalize();
    VerifyNearEqual(c, Plane(0, 0.447214f, 0.894427f, 10.285913f));

    a.Normalize(c);
    VerifyNearEqual(c, Plane(0.267261f, 0.534522f, 0.801784f, 1.069045f));

    VerifyEqual(Plane(0, 1, 0, 23).Dot(Vector4(23, 42, 5, 3)), 111.f);
    VerifyEqual(Plane(0, 1, 0, 23).DotCoordinate(Vector3(23, 42, 5)), 65.f);
    VerifyEqual(Plane(0, 1, 0, 23).DotNormal(Vector3(23, 42, 5)), 42.f);

    Matrix transform( 0, 2, 0, 0,
                     -1, 0, 0, 0,
                      0, 0, 1, 1,
                      0, 0, 0, 1);

    VerifyEqual(Plane::Transform(Plane(1, 5, 10, 23), transform), Plane(-5, 2, 10, 33));

    Plane::Transform(Plane(1, 5, 10, 23), transform, c);
    VerifyEqual(c, Plane(-5, 2, 10, 33));

    Quaternion quat = Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), XM_PIDIV2);

    VerifyNearEqual(Plane::Transform(Plane(1, 5, 10, 23), quat), Plane(10, 5, -1, 23));

    Plane::Transform(Plane(1, 5, 10, 23), quat, c);
    VerifyNearEqual(c, Plane(10, 5, -1, 23));

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestQ()
{
	// Quaternion
    bool success = true;

    Quaternion a(1, 2, 3, 4);
    Quaternion b(Vector3(5, 6, 7), 8);
    
    VerifyEqual(a.x, 1.f);
    VerifyEqual(a.y, 2.f);
    VerifyEqual(a.z, 3.f);
    VerifyEqual(a.w, 4.f);

    VerifyEqual(b.x, 5.f);
    VerifyEqual(b.y, 6.f);
    VerifyEqual(b.z, 7.f);
    VerifyEqual(b.w, 8.f);

    VerifyEqual(Quaternion(), Quaternion(0, 0, 0, 1));
    VerifyEqual(Quaternion::Identity, Quaternion(0, 0, 0, 1));

    VerifyEqual(Quaternion(Vector4(1, 2, 3, 4)), Quaternion(1, 2, 3, 4));
    VerifyEqual(Quaternion(XMVectorSet(1, 2, 3, 4)), Quaternion(1, 2, 3, 4));

    float floatValues[] = { 1, 2, 3, 4 };
    VerifyEqual(Quaternion(floatValues), Quaternion(1, 2, 3, 4));

    XMVECTOR v = a;
    VerifyEqual(Quaternion(v), a);

    VerifyEqual(a == Quaternion(1, 2, 3, 4), true);
    VerifyEqual(a == b, false);

    VerifyEqual(a != Quaternion(1, 2, 3, 4), false);
    VerifyEqual(a != b, true);

    Quaternion c = a;
    VerifyEqual(c, a);
    c = b;
    VerifyEqual(c, b);

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Quaternion vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Quaternion vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 =\n");
            success = false;
        }
    }

    {
        Quaternion vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vc.z, Colors::CornflowerBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vc.w, Colors::CornflowerBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Quaternion vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vt.z, Colors::MidnightBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vt.w, Colors::MidnightBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    c += a;
    VerifyEqual(c, Quaternion(6, 8, 10, 12));

    c -= a;
    VerifyEqual(c, Quaternion(5, 6, 7, 8));

    c *= a;
    VerifyEqual(c, Quaternion(24, 48, 48, -6));

    c /= a;
    VerifyNearEqual(c, Quaternion(5, 6, 7, 8));

    c = b;
    c *= 2;
    VerifyEqual(c, Quaternion(10, 12, 14, 16));

    VerifyEqual(+a, Quaternion(1, 2, 3, 4));
    VerifyEqual(-a, Quaternion(-1, -2, -3, -4));

    VerifyNearEqual(a.Length(), 5.477226f);
    VerifyNearEqual(a.LengthSquared(), 30.f);

    c = a;
    c.Normalize();
    VerifyNearEqual(c, Quaternion(0.182574f, 0.365148f, 0.547723f, 0.730297f));

    a.Normalize(c);
    VerifyNearEqual(c, Quaternion(0.182574f, 0.365148f, 0.547723f, 0.730297f));

    c = a;
    c.Conjugate();
    VerifyEqual(c, Quaternion(-1, -2, -3, 4));

    a.Conjugate(c);
    VerifyEqual(c, Quaternion(-1, -2, -3, 4));
    
    Quaternion(1, 0, 0, 1).Inverse(c);
    VerifyEqual(c, Quaternion(-0.5f, 0, 0, 0.5f));

    VerifyEqual(a.Dot(b), 70.f);

    VerifyNearEqual(Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), XM_PIDIV2), Quaternion(0.000000f, 0.707107f, 0.000000f, 0.707107f));
    VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(0, XM_PIDIV2, 0), Quaternion(0.707107f, 0.000000f, 0.000000f, 0.707107f));
    VerifyNearEqual(Quaternion::CreateFromRotationMatrix(Matrix::CreateFromYawPitchRoll(0, XM_PIDIV2, 0)), Quaternion(0.707107f, 0.000000f, 0.000000f, 0.707107f));

    Quaternion::Lerp(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f), 0.25f, c);
    VerifyNearEqual(c, Quaternion(0.588348f, 0.196116f, 0.000000f, 0.784465f));

    c = Quaternion::Lerp(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f), 0.25f);
    VerifyNearEqual(c, Quaternion(0.588348f, 0.196116f, 0.000000f, 0.784465f));

    Quaternion::Slerp(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f), 0.25f, c);
    VerifyNearEqual(c, Quaternion(0.577350f, 0.211325f, 0.000000f, 0.788675f));

    c = Quaternion::Slerp(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f), 0.25f);
    VerifyNearEqual(c, Quaternion(0.577350f, 0.211325f, 0.000000f, 0.788675f));

    Quaternion::Concatenate(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f), c);
    VerifyNearEqual(c, Quaternion(0.5f, 0.5f, 0.5f, 0.5f));

    c = Quaternion::Concatenate(Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f));
    VerifyNearEqual(c, Quaternion(0.5f, 0.5f, 0.5f, 0.5f));

    VerifyEqual(Quaternion(1, 2, 3, 4) + Quaternion(5, 6, 7, 8), Quaternion(6, 8, 10, 12));
    VerifyEqual(Quaternion(1, 2, 3, 4) - Quaternion(5, 6, 7, 8), Quaternion(-4, -4, -4, -4));
    VerifyEqual(Quaternion(1, 2, 3, 4) * 3, Quaternion(3, 6, 9, 12));
    VerifyEqual(3 * Quaternion(1, 2, 3, 4), Quaternion(3, 6, 9, 12));
    
    VerifyNearEqual(Quaternion(0, 0.707107f, 0, 0.707107f) * Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0.5f, 0.5f, 0.5f, 0.5f));
    VerifyNearEqual(Quaternion(0.5f, 0.5f, 0.5f, 0.5f) / Quaternion(0.707107f, 0, 0, 0.707107f), Quaternion(0, 0.707107f, 0, 0.707107f));

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestC()
{
	// Color
    bool success = true;

    Color a(1, 2, 3);
    Color b(4, 5, 6, 7);
    
    VerifyEqual(a.x, 1.f);
    VerifyEqual(a.y, 2.f);
    VerifyEqual(a.z, 3.f);
    VerifyEqual(a.w, 1.f);

    VerifyEqual(b.x, 4.f);
    VerifyEqual(b.y, 5.f);
    VerifyEqual(b.z, 6.f);
    VerifyEqual(b.w, 7.f);

    VerifyEqual(Color(), Color(0, 0, 0, 1));
    VerifyEqual(Color(Vector3(1, 2, 3)), Color(1, 2, 3, 1));
    VerifyEqual(Color(Vector4(1, 2, 3, 4)), Color(1, 2, 3, 4));

    float floatValues[] = { 1, 2, 3, 4 };
    VerifyEqual(Color(floatValues), Color(1, 2, 3, 4));

    VerifyEqual(Color(XMVectorSet(1, 2, 3, 4)), Color(1, 2, 3, 4));
    VerifyNearEqual(Color(PackedVector::XMCOLOR(0x12345678)), Color(0.203922f, 0.337255f, 0.470588f, 0.070588f));
    VerifyNearEqual(Color(PackedVector::XMUBYTEN4(0x12345678)), Color(0.470588f, 0.337255f, 0.203922f, 0.070588f));

    XMVECTOR v = a;
    VerifyEqual(Color(v), a);

    float const* asFloat = a;
    VerifyEqual(asFloat[0], 1.f);
    VerifyEqual(asFloat[1], 2.f);
    VerifyEqual(asFloat[2], 3.f);
    VerifyEqual(asFloat[3], 1.f);

    VerifyEqual(a == Color(1, 2, 3), true);
    VerifyEqual(a == Color(4, 5, 6), false);

    VerifyEqual(a != Color(1, 2, 3), false);
    VerifyEqual(a != Color(4, 5, 6), true);

    Color c;
    c = b;
    VerifyEqual(c, b);

    {
        Color x;
        x = PackedVector::XMCOLOR(0x12345678);
        VerifyNearEqual(x, Color(0.203922f, 0.337255f, 0.470588f, 0.070588f));

        x = PackedVector::XMUBYTEN4(0x12345678);
        VerifyNearEqual(x, Color(0.470588f, 0.337255f, 0.203922f, 0.070588f));
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Color vi(xm);

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 ctor\n");
            success = false;
        }
    }

    {
        XMFLOAT4 xm(6.f, -2.f, 7.f, 80.f);
        Color vi;
        vi = xm;

        if (!XMScalarNearEqual(xm.x, vi.x, EPSILON)
            || !XMScalarNearEqual(xm.y, vi.y, EPSILON)
            || !XMScalarNearEqual(xm.z, vi.z, EPSILON)
            || !XMScalarNearEqual(xm.w, vi.w, EPSILON))
        {
            printf("ERROR: XMFLOAT4 =\n");
            success = false;
        }
    }

    {
        Color vc = Colors::CornflowerBlue;

        if (!XMScalarNearEqual(vc.x, Colors::CornflowerBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vc.y, Colors::CornflowerBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vc.z, Colors::CornflowerBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vc.w, Colors::CornflowerBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 ctor\n");
            success = false;
        }

        Color vt;
        vt = Colors::MidnightBlue;

        if (!XMScalarNearEqual(vt.x, Colors::MidnightBlue.f[0], EPSILON)
            || !XMScalarNearEqual(vt.y, Colors::MidnightBlue.f[1], EPSILON)
            || !XMScalarNearEqual(vt.z, Colors::MidnightBlue.f[2], EPSILON)
            || !XMScalarNearEqual(vt.w, Colors::MidnightBlue.f[3], EPSILON))
        {
            printf("ERROR: XMVECTORF32 =\n");
            success = false;
        }
    }

    c += a;
    VerifyEqual(c, Color(5, 7, 9, 8));

    c -= a;
    VerifyEqual(c, Color(4, 5, 6, 7));

    c *= 2;
    VerifyEqual(c, Color(8, 10, 12, 14));

    c *= a;
    VerifyEqual(c, Color(8, 20, 36, 14));

    c /= a;
    VerifyEqual(c, Color(8, 10, 12, 14));

    VerifyEqual(+a, Color(1, 2, 3, 1));
    VerifyEqual(-a, Color(-1, -2, -3, -1));
    
    VerifyEqual(a.R(), 1.f);
    VerifyEqual(a.G(), 2.f);
    VerifyEqual(a.B(), 3.f);
    VerifyEqual(a.A(), 1.f);

    VerifyEqual(Color(PackedVector::XMCOLOR(0x12345678)).BGRA().c, 0x12345678u);
    VerifyEqual(Color(PackedVector::XMUBYTEN4(0x12345678)).RGBA().v, 0x12345678u);
    
    VerifyEqual(b.ToVector3(), Vector3(4, 5, 6));
    VerifyEqual(b.ToVector4(), Vector4(4, 5, 6, 7));

    c = Color(0, 0.5f, 1, 23);
    c.Negate();
    VerifyEqual(c, Color(1, 0.5f, 0, 23));

    Color(0, 0.5f, 1, 23).Negate(c);
    VerifyEqual(c, Color(1, 0.5f, 0, 23));

    c = Color(-1, 0, 0.5f, 23);
    c.Saturate();
    VerifyEqual(c, Color(0, 0, 0.5f, 1));

    Color(0.5f, -1, 1, 23).Saturate(c);
    VerifyEqual(c, Color(0.5f, 0, 1, 1));

    c = Color(0, 0.5f, 1, 0.5f);
    c.Premultiply();
    VerifyEqual(c, Color(0, 0.25f, 0.5f, 0.5f));

    Color(0, 0.5f, 1, 0.25f).Premultiply(c);
    VerifyEqual(c, Color(0, 0.125f, 0.25f, 0.25f));

    c = Color(0.25f, 0.5f, 0);
    c.AdjustSaturation(2);
    VerifyNearEqual(c, Color(0.089175f, 0.589175f, -0.410825f));

    Color(0.25f, 0.5f, 0).AdjustSaturation(0.5f, c);
    VerifyNearEqual(c, Color(0.330413f, 0.455413f, 0.205412f));

    c = Color(0.25f, 0.5f, 0);
    c.AdjustContrast(2);
    VerifyNearEqual(c, Color(0, 0.5f, -0.5f));

    Color(0.25f, 0.5f, 0).AdjustContrast(0.5f, c);
    VerifyNearEqual(c, Color(0.375f, 0.5f, 0.25f));

    Color::Modulate(a, b, c);
    VerifyEqual(c, Color(4, 10, 18, 7));

    VerifyEqual(Color::Modulate(a, b), Color(4, 10, 18, 7));

    Color::Lerp(a, b, 0.25f, c);
    VerifyEqual(c, Color(1.75f, 2.75, 3.75, 2.5f));

    VerifyEqual(Color::Lerp(a, b, 0.25f), Color(1.75f, 2.75, 3.75, 2.5f));

    VerifyEqual(a + b, Color(5, 7, 9, 8));
    VerifyEqual(a - b, Color(-3, -3, -3, -6));
    VerifyEqual(a * b, Color(4, 10, 18, 7));
    VerifyEqual(a * 3, Color(3, 6, 9, 3));
    VerifyNearEqual(a / b, Color(0.25f, 0.4f, 0.5f, 0.142857f));
    VerifyEqual(3 * a, Color(3, 6, 9, 3));

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestRay()
{
	// Ray
    bool success = true;

    Ray a;
    VerifyEqual(a.position, Vector3(0, 0, 0));
    VerifyEqual(a.direction, Vector3(0, 0, 1));

    Ray b(Vector3(23, 42, 666), Vector3(0, 1, 0));
    VerifyEqual(b.position, Vector3(23, 42, 666));
    VerifyEqual(b.direction, Vector3(0, 1, 0));

    VerifyEqual(a == Ray(), true);
    VerifyEqual(b == Ray(Vector3(23, 42, 666), Vector3(0, 1, 0)), true);
    VerifyEqual(a == b, false);

    VerifyEqual(a != Ray(), false);
    VerifyEqual(b != Ray(Vector3(23, 42, 666), Vector3(0, 1, 0)), false);
    VerifyEqual(a != b, true);

    float dist;

    VerifyEqual(b.Intersects(BoundingSphere(Vector3(23, -200, 666), 10), dist), false);

    VerifyEqual(b.Intersects(BoundingSphere(Vector3(23, 200, 666), 10), dist), true);
    VerifyEqual(dist, 148.f);

    VerifyEqual(b.Intersects(BoundingBox(Vector3(23, -200, 600), Vector3(5, 10, 100)), dist), false);

    VerifyEqual(b.Intersects(BoundingBox(Vector3(23, 200, 600), Vector3(5, 10, 100)), dist), true);
    VerifyEqual(dist, 148.f);

    VerifyEqual(b.Intersects(Vector3(10, 0, 666), Vector3(50, 0, 300), Vector3(50, 0, 800), dist), false);

    VerifyEqual(b.Intersects(Vector3(10, 100, 666), Vector3(50, 100, 300), Vector3(50, 100, 800), dist), true);
    VerifyEqual(dist, 58.f);

    VerifyEqual(b.Intersects(Plane(Vector3(10, 0, 666), Vector3(50, 0, 300), Vector3(50, 0, 800)), dist), false);

    VerifyEqual(b.Intersects(Plane(Vector3(10, 100, 666), Vector3(50, 100, 300), Vector3(50, 100, 800)), dist), true);
    VerifyEqual(dist, 58.f);

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestVP()
{
    // Viewport
    bool success = true;

    Viewport vp1;
    if (!XMScalarNearEqual(vp1.x, 0.f, EPSILON)
        || !XMScalarNearEqual(vp1.y, 0.f, EPSILON)
        || !XMScalarNearEqual(vp1.width, 0.f, EPSILON)
        || !XMScalarNearEqual(vp1.height, 0.f, EPSILON)
        || !XMScalarNearEqual(vp1.minDepth, 0.f, EPSILON)
        || !XMScalarNearEqual(vp1.maxDepth, 1.f, EPSILON))
    {
            printf("ERROR: vp1 ctor\n");
            success = false;
    }

    Viewport vp2(0.f, 0.f, 640.f, 480.f);
    if (!XMScalarNearEqual(vp2.x, 0.f, EPSILON)
        || !XMScalarNearEqual(vp2.y, 0.f, EPSILON)
        || !XMScalarNearEqual(vp2.width, 640.f, EPSILON)
        || !XMScalarNearEqual(vp2.height, 480.f, EPSILON)
        || !XMScalarNearEqual(vp2.minDepth, 0.f, EPSILON)
        || !XMScalarNearEqual(vp2.maxDepth, 1.f, EPSILON))
    {
            printf("ERROR: vp2 ctor\n");
            success = false;
    }

    Viewport vp3(0.f, 0.f, 1024, 768.f, 1.f, 100.f);
    if (!XMScalarNearEqual(vp3.x, 0.f, EPSILON)
        || !XMScalarNearEqual(vp3.y, 0.f, EPSILON)
        || !XMScalarNearEqual(vp3.width, 1024.f, EPSILON)
        || !XMScalarNearEqual(vp3.height, 768.f, EPSILON)
        || !XMScalarNearEqual(vp3.minDepth, 1.f, EPSILON)
        || !XMScalarNearEqual(vp3.maxDepth, 100.f, EPSILON))
    {
            printf("ERROR: vp3 ctor\n");
            success = false;
    }

    Viewport vp4(0.f, 0.f, 1920.f, 1080.f);
    if (!XMScalarNearEqual(vp4.x, 0.f, EPSILON)
        || !XMScalarNearEqual(vp4.y, 0.f, EPSILON)
        || !XMScalarNearEqual(vp4.width, 1920.f, EPSILON)
        || !XMScalarNearEqual(vp4.height, 1080.f, EPSILON)
        || !XMScalarNearEqual(vp4.minDepth, 0.f, EPSILON)
        || !XMScalarNearEqual(vp4.maxDepth, 1.f, EPSILON))
    {
            printf("ERROR: vp4 ctor\n");
            success = false;
    }

    Viewport vp5(23.f, 42.f, 666.f, 1234.f);
    if (!XMScalarNearEqual(vp5.x, 23.f, EPSILON)
        || !XMScalarNearEqual(vp5.y, 42.f, EPSILON)
        || !XMScalarNearEqual(vp5.width, 666.f, EPSILON)
        || !XMScalarNearEqual(vp5.height, 1234.f, EPSILON)
        || !XMScalarNearEqual(vp5.minDepth, 0.f, EPSILON)
        || !XMScalarNearEqual(vp5.maxDepth, 1.f, EPSILON))
    {
            printf("ERROR: vp5 ctor\n");
            success = false;
    }

    if ( vp1 == vp2 )
    {
        printf("ERROR: ==\n");
        success = false;
    }

    if ( vp2 != vp2 )
    {
        printf("ERROR: !=\n");
        success = false;
    }

    {
        Viewport other(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp(other);

        if (!XMScalarNearEqual(vp.x, other.x, EPSILON)
            || !XMScalarNearEqual(vp.y, other.y, EPSILON)
            || !XMScalarNearEqual(vp.width, other.width, EPSILON)
            || !XMScalarNearEqual(vp.height, other.height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, other.minDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, other.maxDepth, EPSILON))
        {
            printf("ERROR: copy ctor\n");
            success = false;
        }
    }

    {
        Viewport other(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp;
        vp = other;

        if (!XMScalarNearEqual(vp.x, other.x, EPSILON)
            || !XMScalarNearEqual(vp.y, other.y, EPSILON)
            || !XMScalarNearEqual(vp.width, other.width, EPSILON)
            || !XMScalarNearEqual(vp.height, other.height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, other.minDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, other.maxDepth, EPSILON))
        {
            printf("ERROR: =\n");
            success = false;
        }
    }

    {
        RECT rct;
        rct.left = 23;
        rct.top = 42;
        rct.right = rct.left + 666;
        rct.bottom = rct.top + 1234;
        Viewport vp(rct);

        if (!XMScalarNearEqual(vp.x, 23.f, EPSILON)
            || !XMScalarNearEqual(vp.y, 42.f, EPSILON)
            || !XMScalarNearEqual(vp.width, 666.f, EPSILON)
            || !XMScalarNearEqual(vp.height, 1234.f, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, 0.f, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, 1.f, EPSILON))
        {
            printf("ERROR: RECT ctor\n");
            success = false;
        }
    }

    {
        RECT rct;
        rct.left = 23;
        rct.top = 42;
        rct.right = rct.left + 666;
        rct.bottom = rct.top + 1234;
        Viewport vp;
        vp = rct;

        if (!XMScalarNearEqual(vp.x, 23.f, EPSILON)
            || !XMScalarNearEqual(vp.y, 42.f, EPSILON)
            || !XMScalarNearEqual(vp.width, 666.f, EPSILON)
            || !XMScalarNearEqual(vp.height, 1234.f, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, 0.f, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, 1.f, EPSILON))
        {
            printf("ERROR: RECT =\n");
            success = false;
        }
    }

    {
        CD3D11_VIEWPORT d3d11vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp(d3d11vp);

        if (!XMScalarNearEqual(vp.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D11_VIEWPORT ctor\n");
            success = false;
        }
    }

    {
        CD3D11_VIEWPORT d3d11vp(23.f, 42.f, 666.f, 1234.f, 0.f, 1.f);
        Viewport vp;
        vp = d3d11vp;

        if (!XMScalarNearEqual(vp.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: D3D11_VIEWPORT =\n");
            success = false;
        }
    }

    {
        D3D11_VIEWPORT d3d11vp = vp5;

        if (!XMScalarNearEqual(vp5.x, d3d11vp.TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp5.y, d3d11vp.TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp5.width, d3d11vp.Width, EPSILON)
            || !XMScalarNearEqual(vp5.height, d3d11vp.Height, EPSILON)
            || !XMScalarNearEqual(vp5.minDepth, d3d11vp.MinDepth, EPSILON)
            || !XMScalarNearEqual(vp5.maxDepth, d3d11vp.MaxDepth, EPSILON))
        {
            printf("ERROR: operator D3D11_VIEWPORT\n");
            success = false;
        }
    }

    {
        const D3D11_VIEWPORT* d3d11vp = vp5.Get11();

        if (!XMScalarNearEqual(vp5.x, d3d11vp->TopLeftX, EPSILON)
            || !XMScalarNearEqual(vp5.y, d3d11vp->TopLeftY, EPSILON)
            || !XMScalarNearEqual(vp5.width, d3d11vp->Width, EPSILON)
            || !XMScalarNearEqual(vp5.height, d3d11vp->Height, EPSILON)
            || !XMScalarNearEqual(vp5.minDepth, d3d11vp->MinDepth, EPSILON)
            || !XMScalarNearEqual(vp5.maxDepth, d3d11vp->MaxDepth, EPSILON))
        {
            printf("ERROR: Get\n");
            success = false;
        }
    }

    {
        if ( vp1.AspectRatio() != 0.f )
        {
            printf("ERROR: AspectRatio vp1\n");
            success = false;
        }

        float expected = float(vp2.width) / float(vp2.height);
        if ( !XMScalarNearEqual( vp2.AspectRatio(), expected, EPSILON ) )
        {
            printf("ERROR: AspectRatio vp2\n");
            success = false;
        }

        expected = float(vp3.width) / float(vp3.height);
        if ( !XMScalarNearEqual( vp3.AspectRatio(), expected, EPSILON ) )
        {
            printf("ERROR: AspectRatio vp3\n");
            success = false;
        }

        expected = float(vp4.width) / float(vp4.height);
        if ( !XMScalarNearEqual( vp4.AspectRatio(), expected, EPSILON ) )
        {
            printf("ERROR: AspectRatio vp4\n");
            success = false;
        }

        expected = float(vp5.width) / float(vp5.height);
        if ( !XMScalarNearEqual( vp5.AspectRatio(), expected, EPSILON ) )
        {
            printf("ERROR: AspectRatio vp3\n");
            success = false;
        }
    }

    {
        Vector3 p = vp2.Project(Vector3(1, -1, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity);
        if (!XMVector3NearEqual(p, Vector3(640.f, 480.f, 1.f), VEPSILON))
        {
            printf("ERROR: Project %f %f %f ... 640 480 1\n", p.x, p.y, p.z);
            success = false;
        }

        Vector3 r;
        vp2.Project(Vector3(1, -1, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity, r);
        if (!XMVector3NearEqual(r, Vector3(640.f, 480.f, 1.f), VEPSILON))
        {
            printf("ERROR: Project(2) %f %f %f ... 640 480 1\n", p.x, p.y, p.z);
            success = false;
        }

        p = vp3.Project(Vector3(1, -1, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity);
        if (!XMVector3NearEqual(p, Vector3(1024.f, 768.f, 100.f), VEPSILON))
        {
            printf("ERROR: Project %f %f %f ... 1024 748 100\n", p.x, p.y, p.z);
            success = false;
        }

        vp3.Project(Vector3(1, -1, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity, r);
        if (!XMVector3NearEqual(r, Vector3(1024.f, 768.f, 100.f), VEPSILON))
        {
            printf("ERROR: Project(2) %f %f %f ... 1024 748 100\n", p.x, p.y, p.z);
            success = false;
        }

        p = vp2.Unproject(Vector3(640.f, 480.f, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity);
        if (!XMVector3NearEqual(p, Vector3(1.f, -1.f, 1.f), VEPSILON))
        {
            printf("ERROR: Unproject %f %f %f ... 1 -1 1\n", p.x, p.y, p.z);
            success = false;
        }

        vp2.Unproject(Vector3(640.f, 480.f, 1), Matrix::Identity, Matrix::Identity, Matrix::Identity, r);
        if (!XMVector3NearEqual(r, Vector3(1.f, -1.f, 1.f), VEPSILON))
        {
            printf("ERROR: Unproject(2) %f %f %f ... 1 -1 1\n", p.x, p.y, p.z);
            success = false;
        }

        p = vp3.Unproject(Vector3(1024.f, 768.f, 100.f), Matrix::Identity, Matrix::Identity, Matrix::Identity);
        if (!XMVector3NearEqual(p, Vector3(1.f, -1.f, 1.f), VEPSILON))
        {
            printf("ERROR: Unproject %f %f %f ... 1 -1 1\n", p.x, p.y, p.z);
            success = false;
        }

        vp3.Unproject(Vector3(1024.f, 768.f, 100.f), Matrix::Identity, Matrix::Identity, Matrix::Identity, r);
        if (!XMVector3NearEqual(r, Vector3(1.f, -1.f, 1.f), VEPSILON))
        {
            printf("ERROR: Unproject(2) %f %f %f ... 1 -1 1\n", p.x, p.y, p.z);
            success = false;
        }
    }

    {
        Matrix world = Matrix::CreateWorld(Vector3(1, 2, 3), Vector3::UnitX, Vector3::UnitZ);
        Matrix view = Matrix::CreateLookAt(Vector3(10, 10, 10), Vector3(0, 0, 0), Vector3::UnitY);
        Matrix proj = Matrix::CreatePerspectiveFieldOfView(XM_PI / 4.f, 640.f / 480.f, 0.1f, 100.f);

        Vector3 p = vp2.Project(Vector3(0.5f, 0.75f, 0.25f), proj, view, world);
        if (!XMVector3NearEqual(p, Vector3(231.296143f, 265.606598, 0.993776858f), VEPSILON3))
        {
            printf("ERROR: Project %f %f %f ... 231.296143 265.606598 0.993776858\n", p.x, p.y, p.z);
            success = false;
        }

        Vector3 r;
        vp2.Project(Vector3(0.5f, 0.75f, 0.25f), proj, view, world, r);
        if (!XMVector3NearEqual(r, Vector3(231.296143f, 265.606598, 0.993776858f), VEPSILON3))
        {
            printf("ERROR: Project(2) %f %f %f ... 231.296143 265.606598 0.993776858\n", p.x, p.y, p.z);
            success = false;
        }

        p = vp3.Project(Vector3(0.5f, 0.75f, 0.25f), proj, view, world);
        if (!XMVector3NearEqual(p, Vector3(370.073822f, 424.970551f, 99.3839111f), VEPSILON3))
        {
            printf("ERROR: Project %f %f %f ... 370.073822 424.970551 99.3839111 \n", p.x, p.y, p.z);
            success = false;
        }

        vp3.Project(Vector3(0.5f, 0.75f, 0.25f), proj, view, world, r);
        if (!XMVector3NearEqual(r, Vector3(370.073822f, 424.970551f, 99.3839111f), VEPSILON3))
        {
            printf("ERROR: Project(2) %f %f %f ... 370.073822 424.970551 99.3839111 \n", p.x, p.y, p.z);
            success = false;
        }

        p = vp2.Unproject(Vector3(231.f,265.f,0.993776858), proj, view, world);
        if (!XMVector3NearEqual(p, Vector3(0.488145798f, 0.748996973f, 0.260956854f), VEPSILON3))
        {
            printf("ERROR: Unproject %f %f %f ... 0.488145798 0.748996973 0.260956854\n", p.x, p.y, p.z);
            success = false;
        }

        vp2.Unproject(Vector3(231.f, 265.f, 0.993776858), proj, view, world, r);
        if (!XMVector3NearEqual(r, Vector3(0.488145798f, 0.748996973f, 0.260956854f), VEPSILON3))
        {
            printf("ERROR: Unproject(2) %f %f %f ... 0.488145798 0.748996973 0.260956854\n", p.x, p.y, p.z);
            success = false;
        }

        p = vp3.Unproject(Vector3(370.073822f,424.970551f,99.3839111f), proj, view, world);
        if (!XMVector3NearEqual(p, Vector3(0.499990046f, 0.749911547f, 0.250027150f), VEPSILON3))
        {
            printf("ERROR: Unproject %f %f %f ... 0.499990046 0.749911547 0.250027150\n", p.x, p.y, p.z);
            success = false;
        }

        vp3.Unproject(Vector3(370.073822f, 424.970551f, 99.3839111f), proj, view, world, r);
        if (!XMVector3NearEqual(r, Vector3(0.499990046f, 0.749911547f, 0.250027150f), VEPSILON3))
        {
            printf("ERROR: Unproject(2) %f %f %f ... 0.499990046 0.749911547 0.250027150\n", p.x, p.y, p.z);
            success = false;
        }
    }

    {
        RECT rct = Viewport::ComputeDisplayArea(DXGI_SCALING_NONE, 640, 480, 1024, 1024);
        if (rct.left != 0
            || rct.right != 640
            || rct.top != 0
            || rct.bottom != 480)
        {
            printf("ERROR: DisplayArea 480 none\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING_STRETCH, 640, 480, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 0
            || rct.bottom != 1024)
        {
            printf("ERROR: DisplayArea 480 stretch\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 640, 480, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 128
            || rct.bottom != 896)
        {
            printf("ERROR: DisplayArea 480 aspectratio letter-box\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 640, 480, 1024, 400);
        if (rct.left != 245
            || rct.right != 778
            || rct.top != 0
            || rct.bottom != 400)
        {
            printf("ERROR: DisplayArea 480 aspectratio pillar-box\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING_NONE, 1280, 720, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 0
            || rct.bottom != 720)
        {
            printf("ERROR: DisplayArea 720 none\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING_STRETCH, 1280, 720, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 0
            || rct.bottom != 1024)
        {
            printf("ERROR: DisplayArea 720 stretch\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 1280, 720, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 224
            || rct.bottom != 800)
        {
            printf("ERROR: DisplayArea 720 aspectratio letter-box\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 1280, 720, 1024, 500);
        if (rct.left != 67
            || rct.right != 956
            || rct.top != 0
            || rct.bottom != 500)
        {
            printf("ERROR: DisplayArea 720 aspectratio pillar-box\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING_NONE, 1920, 1080, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 0
            || rct.bottom != 1024)
        {
            printf("ERROR: DisplayArea 1080 none\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING_STRETCH, 1920, 1080, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 0
            || rct.bottom != 1024)
        {
            printf("ERROR: DisplayArea 1080 stretch\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 1920, 1080, 1024, 1024);
        if (rct.left != 0
            || rct.right != 1024
            || rct.top != 224
            || rct.bottom != 800)
        {
            printf("ERROR: DisplayArea 1080 aspectratio letter-box\n");
            success = false;
        }

        rct = Viewport::ComputeDisplayArea(DXGI_SCALING(2) /*DXGI_SCALING_ASPECT_RATIO_STRETCH*/, 1920, 1080, 1024, 500);
        if (rct.left != 67
            || rct.right != 956
            || rct.top != 0
            || rct.bottom != 500)
        {
            printf("ERROR: DisplayArea 1080 aspectratio pillar-box\n");
            success = false;
        }
    }

    {
        RECT rct = Viewport::ComputeTitleSafeArea(0, 0);

        if (rct.left != 0
            || rct.right != 0
            || rct.top != 0
            || rct.bottom != 0)
        {
            printf("ERROR: TitleSafe null\n");
            success = false;
        }

        rct = Viewport::ComputeTitleSafeArea(640, 480);
        if (rct.left != 32
            || rct.right != 607
            || rct.top != 24
            || rct.bottom != 455)
        {
            printf("ERROR: TitleSafe 480p\n");
            success = false;
        }

        rct = Viewport::ComputeTitleSafeArea(1280, 720);
        if (rct.left != 64
            || rct.right != 1215
            || rct.top != 36
            || rct.bottom != 683)
        {
            printf("ERROR: TitleSafe 720p\n");
            success = false;
        }

        rct = Viewport::ComputeTitleSafeArea(1920, 1080);
        if (rct.left != 96
            || rct.right != 1823
            || rct.top != 54
            || rct.bottom != 1025)
        {
            printf("ERROR: TitleSafe 1080p\n");
            success = false;
        }
    }

	return (success) ? 0 : 1;
}

template<typename T>
int EnsureSorted(std::map<T, int>& map)
{
    bool success = true;

    int expected = 1;

    for (auto it = map.begin(); it != map.end(); ++it)
    {
        if (it->second != expected)
        {
            printf("std::less<%s> error: expecting %d, got %d\n", typeid(T).name(), expected, it->second);
            success = false;
        }

        ++expected;
    }

	return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestL()
{
    // std::less
    using Rectangle = SimpleMath::Rectangle;

    std::map<Rectangle, int> maprct;
    std::map<Vector2, int> mapv2;
    std::map<Vector3, int> mapv3;
    std::map<Vector4, int> mapv4;
    std::map<Matrix, int> mapm;
    std::map<Plane, int> mapp;
    std::map<Quaternion, int> mapq;
    std::map<Color, int> mapc;
    std::map<Ray, int> mapray;
    std::map<Viewport, int> mapvp;

    maprct[Rectangle(0, 0, 100, 100)] = 3;
    maprct[Rectangle(10, 20, 4, 5)] = 5;
    maprct[Rectangle(12, 15, 100, 7)] = 6;
    maprct[Rectangle(0, 0, 10, 23)] = 2;
    maprct[Rectangle(10, 20, 0, 0)] = 4;
    maprct[Rectangle(0, 0, 0, 0)] = 1;

    Rectangle ab(12, 20, 2, 2);

    mapv2[ Vector2(3.f, 2.f) ] = 4;
    mapv2[ Vector2(1.f, 2.f) ] = 1;
    mapv2[ Vector2(2.f, 2.f) ] = 3;
    mapv2[ Vector2(2.f, 1.f) ] = 2;

    mapv3[ Vector3(3.f, 2.f, 3.f) ] = 6;
    mapv3[ Vector3(1.f, 2.f, 3.f) ] = 1;
    mapv3[ Vector3(2.f, 3.f, 3.f) ] = 5;
    mapv3[ Vector3(2.f, 1.f, 3.f) ] = 2;
    mapv3[ Vector3(2.f, 2.f, 3.f) ] = 4;
    mapv3[ Vector3(2.f, 2.f, 1.f) ] = 3;

    mapv4[ Vector4(3.f, 2.f, 3.f, 4.f) ] = 8;
    mapv4[ Vector4(1.f, 2.f, 3.f, 4.f) ] = 1;
    mapv4[ Vector4(2.f, 3.f, 3.f, 4.f) ] = 7;
    mapv4[ Vector4(2.f, 1.f, 3.f, 4.f) ] = 2;
    mapv4[ Vector4(2.f, 2.f, 3.f, 4.f) ] = 6;
    mapv4[ Vector4(2.f, 2.f, 1.f, 4.f) ] = 3;
    mapv4[ Vector4(2.f, 2.f, 2.f, 3.f) ] = 5;
    mapv4[ Vector4(2.f, 2.f, 2.f, 1.f) ] = 4;

    mapm[ Matrix(1, 2, 3, 4, 2, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) ] = 1;
    mapm[ Matrix(1, 2, 6, 4, 5, 6, 7, 4, 9, 10, 11, 12, 13, 14, 15, 16) ] = 4;
    mapm[ Matrix(1, 2, 3, 4, 8, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16) ] = 2;
    mapm[ Matrix(1, 2, 3, 4, 8, 6, 7, 8, 9, 10, 11, 12, 19, 14, 15, 16) ] = 3;

    mapp[ Plane(3.f, 2.f, 3.f, 4.f) ] = 8;
    mapp[ Plane(1.f, 2.f, 3.f, 4.f) ] = 1;
    mapp[ Plane(2.f, 3.f, 3.f, 4.f) ] = 7;
    mapp[ Plane(2.f, 1.f, 3.f, 4.f) ] = 2;
    mapp[ Plane(2.f, 2.f, 3.f, 4.f) ] = 6;
    mapp[ Plane(2.f, 2.f, 1.f, 4.f) ] = 3;
    mapp[ Plane(2.f, 2.f, 2.f, 3.f) ] = 5;
    mapp[ Plane(2.f, 2.f, 2.f, 1.f) ] = 4;

    mapq[ Quaternion(3.f, 2.f, 3.f, 4.f) ] = 8;
    mapq[ Quaternion(1.f, 2.f, 3.f, 4.f) ] = 1;
    mapq[ Quaternion(2.f, 3.f, 3.f, 4.f) ] = 7;
    mapq[ Quaternion(2.f, 1.f, 3.f, 4.f) ] = 2;
    mapq[ Quaternion(2.f, 2.f, 3.f, 4.f) ] = 6;
    mapq[ Quaternion(2.f, 2.f, 1.f, 4.f) ] = 3;
    mapq[ Quaternion(2.f, 2.f, 2.f, 3.f) ] = 5;
    mapq[ Quaternion(2.f, 2.f, 2.f, 1.f) ] = 4;

    mapc[ Color(3.f, 2.f, 3.f, 4.f) ] = 8;
    mapc[ Color(1.f, 2.f, 3.f, 4.f) ] = 1;
    mapc[ Color(2.f, 3.f, 3.f, 4.f) ] = 7;
    mapc[ Color(2.f, 1.f, 3.f, 4.f) ] = 2;
    mapc[ Color(2.f, 2.f, 3.f, 4.f) ] = 6;
    mapc[ Color(2.f, 2.f, 1.f, 4.f) ] = 3;
    mapc[ Color(2.f, 2.f, 2.f, 3.f) ] = 5;
    mapc[ Color(2.f, 2.f, 2.f, 1.f) ] = 4;

    mapray[ Ray(Vector3(3.f, 2.f, 3.f), Vector3(1, 1, 1)) ] = 12;
    mapray[ Ray(Vector3(1.f, 2.f, 3.f), Vector3(2, 3, 4)) ] = 1;
    mapray[ Ray(Vector3(2.f, 3.f, 3.f), Vector3(3, 5, 2)) ] = 11;
    mapray[ Ray(Vector3(2.f, 1.f, 3.f), Vector3(4, 9, 5)) ] = 2;
    mapray[ Ray(Vector3(2.f, 2.f, 3.f), Vector3(5, 8, 2)) ] = 10;
    mapray[ Ray(Vector3(2.f, 2.f, 1.f), Vector3(6, 7, 1)) ] = 3;

    mapray[ Ray(Vector3(2, 2, 2), Vector3(3.f, 2.f, 3.f)) ] = 9;
    mapray[ Ray(Vector3(2, 2, 2), Vector3(1.f, 2.f, 3.f)) ] = 4;
    mapray[ Ray(Vector3(2, 2, 2), Vector3(2.f, 3.f, 3.f)) ] = 8;
    mapray[ Ray(Vector3(2, 2, 2), Vector3(2.f, 1.f, 3.f)) ] = 5;
    mapray[ Ray(Vector3(2, 2, 2), Vector3(2.f, 2.f, 3.f)) ] = 7;
    mapray[ Ray(Vector3(2, 2, 2), Vector3(2.f, 2.f, 1.f)) ] = 6;

    mapvp[Viewport(0.f, 0.f, 1024, 768.f, 1.f, 100.f)] = 3;
    mapvp[Viewport()] = 1;
    mapvp[Viewport(0.f, 0.f, 1920.f, 1080.f)] = 4;
    mapvp[Viewport(0.f, 0.f, 640.f, 480.f)] = 2;
    mapvp[Viewport(23.f, 42.f, 666.f, 1234.f)] = 5;

    return EnsureSorted(maprct) |
           EnsureSorted(mapv2) |
           EnsureSorted(mapv3) |
           EnsureSorted(mapv4) |
           EnsureSorted(mapm) |
           EnsureSorted(mapp) |
           EnsureSorted(mapq) |
           EnsureSorted(mapc) |
           EnsureSorted(mapray) |
           EnsureSorted(mapvp);
}


//-------------------------------------------------------------------------------------
typedef int (*TestFN)();

static struct Test
{
	const char *    name;
	TestFN          func;
} g_Tests[] = 
{ 
    { "Rectangle", TestRect },
    { "Vector2", TestV2 },
    { "Vector3", TestV3 },
    { "Vector4", TestV4 },
    { "Matrix", TestM },
    { "Plane", TestP },
    { "Quaternion", TestQ },
    { "Color", TestC },
    { "Ray", TestRay },
    { "Viewport", TestVP },
    { "std::less", TestL },
};

int __cdecl main()
{
    size_t npass = 0;
    bool success = true;

    for( size_t j = 0; j < _countof(g_Tests); ++j )
    {
        printf("%s: ", g_Tests[j].name );
        if ( !g_Tests[j].func() )
        {
            printf("Pass\n");
            ++npass;
        }
        else
        {
            success = false;
            printf("FAILED\n");
        }
    }

    if ( success )
    {
        printf("Passed all tests\n");
        return 0;
    }
    else
    {
        printf("FAILED, passed %Iu tests\n", npass );
        return 1;
    }
}

