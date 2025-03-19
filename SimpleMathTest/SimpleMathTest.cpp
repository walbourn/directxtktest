//-------------------------------------------------------------------------------------
// SimpleMathTest.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#ifndef _WIN32
#include "pch.h"
#endif

#include "SimpleMath.h"

#include "SimpleMathTest.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

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
    static_assert(std::is_nothrow_default_constructible<SimpleMath::Rectangle>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<SimpleMath::Rectangle>::value, "Copy Ctor.");
    static_assert(std::is_nothrow_copy_assignable<SimpleMath::Rectangle>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<SimpleMath::Rectangle>::value, "Move Ctor.");
    static_assert(std::is_nothrow_move_assignable<SimpleMath::Rectangle>::value, "Move Assign.");

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

        if (smallRect != smallRct)
        {
            printf("ERROR: RECT != small\n");
            success = false;
        }

        RECT bigRct = { 15, 32, 1920 + 15, 1080 + 32 };

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
            || ctr.x != (float(smallRect.x) + float(smallRect.width / 2))
            || ctr.y != (float(smallRect.y) + float(smallRect.height / 2)))
        {
            printf("ERROR: Location/Center small\n");
            success = false;
        }

        loc = bigRect.Location();
        ctr = bigRect.Center();

        if (loc.x != float(bigRect.x)
            || loc.y != float(bigRect.y)
            || ctr.x != (float(bigRect.x) + float(bigRect.width / 2))
            || ctr.y != (float(bigRect.y) + float(bigRect.height / 2)))
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

        if (Rectangle(ab) != Rectangle::Intersect(a, b))
        {
            printf("ERROR: Intersect RECT 1\n");
            success = false;
        }

        if (Rectangle(ac) != Rectangle::Intersect(a, c))
        {
            printf("ERROR: Intersect RECT 2\n");
            success = false;
        }

        if (Rectangle(ad) != Rectangle::Intersect(a, d))
        {
            printf("ERROR: Intersect RECT 3\n");
            success = false;
        }

        if (Rectangle(ae) != Rectangle::Intersect(a, e))
        {
            printf("ERROR: Intersect RECT 4\n");
            success = false;
        }

        if (Rectangle(bc) != Rectangle::Intersect(b, c))
        {
            printf("ERROR: Intersect RECT 5\n");
            success = false;
        }

        if (Rectangle(ab) != Rectangle::Intersect(b, a))
        {
            printf("ERROR: Intersect RECT 6\n");
            success = false;
        }

        if (Rectangle(ac) != Rectangle::Intersect(c, a))
        {
            printf("ERROR: Intersect RECT 7\n");
            success = false;
        }

        if (Rectangle(ad) != Rectangle::Intersect(d, a))
        {
            printf("ERROR: Intersect RECT 8\n");
            success = false;
        }

        if (Rectangle(ae) != Rectangle::Intersect(e, a))
        {
            printf("ERROR: Intersect RECT 9\n");
            success = false;
        }

        if (Rectangle(bc) != Rectangle::Intersect(c, b))
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
        if (Rectangle(ab) != Rectangle::Union(a, b))
        {
            printf("ERROR: Union 1\n");
            success = false;
        }

        if (Rectangle(ac) != Rectangle::Union(a, c))
        {
            printf("ERROR: Union 2\n");
            success = false;
        }

        if (Rectangle(ad) != Rectangle::Union(a, d))
        {
            printf("ERROR: Union 3\n");
            success = false;
        }

        if (Rectangle(ae) != Rectangle::Union(a, e))
        {
            printf("ERROR: Union 4\n");
            success = false;
        }

        if (Rectangle(bc) != Rectangle::Union(b, c))
        {
            printf("ERROR: Union 5\n");
            success = false;
        }

        if (Rectangle(ab) != Rectangle::Union(b, a))
        {
            printf("ERROR: Union 6\n");
            success = false;
        }

        if (Rectangle(ac) != Rectangle::Union(c, a))
        {
            printf("ERROR: Union 7\n");
            success = false;
        }

        if (Rectangle(ad) != Rectangle::Union(d, a))
        {
            printf("ERROR: Union 8\n");
            success = false;
        }

        if (Rectangle(ae) != Rectangle::Union(e, a))
        {
            printf("ERROR: Union 9\n");
            success = false;
        }

        if (Rectangle(bc) != Rectangle::Union(c, b))
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
    static_assert(std::is_nothrow_default_constructible<Vector2>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Vector2>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Vector2>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Vector2>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Vector2>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Vector2 upVector( 0, 1.f );
    const Vector2 rightVector( 1.f, 0 );
    const Vector2 v1( 1.f, 2.f );
    const Vector2 v2( 4.f, 5.f );
    const Vector2 v3( 3.f, -23.f );

    if ( upVector == rightVector )
    {
        printf("ERROR: ==\n");
        success = false;
    }

    const Vector2 zero(0.f, 0.f);
    const Vector2 one(1.f, 1.f);
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

    v = +v1;
    if ( v != v1 )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    v = -v1;
    if ( v != Vector2( -v1.x, -v1.y ) )
    {
        printf("ERROR: - (unary)\n");
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
        auto vc = Vector2(Colors::CornflowerBlue);

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

    v = v1;
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

    v *= 2.f;
    if ( v != Vector2(8,20) )
    {
        printf("ERROR: *=f %f %f ... 8 20\n", v.x, v.y );
        success = false;
    }

    v /= 2.f;
    if ( v != Vector2(4,10) )
    {
        printf("ERROR: */f %f %f ... 4 10\n", v.x, v.y );
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
        const Vector2 a(-1.f, 4.f);
        const Vector2 b(2.f, 1.f);
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
        const Vector2 a(-1.f, 4.f);
        const Vector2 b(2.f, 1.f);
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
        const Vector2 a(1.f, 2.f);
        const Vector2 b(3.f, 4.f);

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
        const Vector2 a(1.f, -2.f);
        const Vector2 b(-5.f, 6.f);
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
        const Vector2 value1(1.0f, 12.0f);
        const Vector2 value2(21.0f, 22.0f);
        const Vector2 value3(31.0f, 32.0f);

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
        const Vector2 position1(1.0f, 2.0f);
        const Vector2 position2(-1.0f, 4.0f);
        const Vector2 position3(2.0f, 6.0f);
        const Vector2 position4(3.0f, 8.0f);

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
        const Vector2 p1(0.f, 1.f);
        const Vector2 t1(0.f, tanf(XMConvertToRadians(30.f)));
        const Vector2 p2(-2.f, 2.f);
        const Vector2 t2(0.f, tanf(XMConvertToRadians(-5.f)));

        // 0.5
        Vector2 result(-1.0f, 1.583105f);
        v = Vector2::Hermite(p1, t1, p2, t2, 0.5f);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite(2) 0.5 %f %f\n", v.x, v.y);
            success = false;
        }

        // 0
        v = Vector2::Hermite(p1, t1, p2, t2, 0.f);
        if (!XMVector2NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite 0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 0.f, v);
        if (!XMVector2NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite(2) 0 %f %f\n", v.x, v.y);
            success = false;
        }

        // 1
        v = Vector2::Hermite(p1, t1, p2, t2, 1.f);
        if (!XMVector2NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite 1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 1.f, v);
        if (!XMVector2NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite(2) 1 %f %f\n", v.x, v.y);
            success = false;
        }

        // <0
        result = Vector2(-10.0f, 3.86557627f);
        v = Vector2::Hermite(p1, t1, p2, t2, -1.f);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite <0 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, -1.f, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite(2) <0 %f %f\n", v.x, v.y);
            success = false;
        }

        // >1
        result = Vector2(8.0f, -2.19525433f);
        v = Vector2::Hermite(p1, t1, p2, t2, 2.f);
        if (!XMVector2NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite >1 %f %f\n", v.x, v.y);
            success = false;
        }

        Vector2::Hermite(p1, t1, p2, t2, 2.f, v);
        if (!XMVector2NearEqual(v, result, VEPSILON))
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

        const Vector2 n(0.f, 1.f);
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
        const Vector2 vec(1.f, 2.f);
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
        const Vector2 vec(1.f, 2.f);
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
        const Vector2 vec(1.f, 2.f);
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

        auto buff = std::make_unique<Vector2[]>(std::size(points));

        Vector2::Transform(&points[0], std::size(points), m, buff.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector2 result = Vector2::Transform(points[j], m);
            v = buff[j];
            if (!XMVector2NearEqual(v, result, VEPSILON))
            {
                printf("ERROR: transarr %zu - %f %f ... %f %f \n", j, v.x, v.y, result.x, result.y);
                success = false;
            }
        }

        auto buff4 = std::make_unique<Vector4[]>(std::size(points));

        Vector2::Transform(&points[0], std::size(points), m, buff4.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector4 result;
            Vector2::Transform(points[j], m, result);
            Vector4 vec4 = buff4[j];
            if (!XMVector4NearEqual(vec4, result, VEPSILON2))
            {
                printf("ERROR: transarr(2) %zu - %f %f %f %f ... %f %f %f %f\n", j, vec4.x, vec4.y, vec4.z, vec4.w, result.x, result.y, result.z, result.w);
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

        auto buff = std::make_unique<Vector2[]>(std::size(points));

        Vector2::TransformNormal(&points[0], std::size(points), m, buff.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector2 result = Vector2::TransformNormal(points[j], m);
            v = buff[j];
            if (!XMVector2NearEqual(v, result, VEPSILON2))
            {
                printf("ERROR: transnormarr %zu - %f %f ... %f %f \n", j, v.x, v.y, result.x, result.y);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 + v2;
    if ( v != Vector2( 5.f, 7.f ) )
    {
        printf("ERROR: + %f %f ... 5 7\n", v.x, v.y );
        success = false;
    }

    v = v2 - v1;
    if ( v != Vector2( 3.f, 3.f ) )
    {
        printf("ERROR: - %f %f ... 3 3\n", v.x, v.y );
        success = false;
    }

    v = v1 * v2;
    if ( v != Vector2( 4.f, 10.f ) )
    {
        printf("ERROR: * %f %f ... 4 10\n", v.x, v.y );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector2( 2.f, 4.f ) )
    {
        printf("ERROR: *f %f %f ... 2 4\n", v.x, v.y );
        success = false;
    }

    v = 2.f * v1;
    if ( v != Vector2( 2.f, 4.f ) )
    {
        printf("ERROR: f* %f %f ... 2 4\n", v.x, v.y );
        success = false;
    }

    v = v1 / v2;
    if ( v != Vector2( 0.25f, 0.4f ) )
    {
        printf("ERROR: / %f %f ... 0.25 0.4\n", v.x, v.y );
        success = false;
    }

    v = v1 / 2.f;
    if ( v != Vector2( 0.5f, 1.f ) )
    {
        printf("ERROR: /f %f %f ... 0.5 1\n", v.x, v.y );
        success = false;
    }

    v = 2.f / v2;
    if ( v != Vector2( 0.5f, 0.4f ) )
    {
        printf("ERROR: f/ %f %f ... 0.5 0.4\n", v.x, v.y );
        success = false;
    }

    return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestV3()
{
    // Vector3
    static_assert(std::is_nothrow_default_constructible<Vector3>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Vector3>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Vector3>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Vector3>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Vector3>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Vector3 upVector( 0, 1.f, 0 );
    const Vector3 rightVector( 1.f, 0, 0 );
    const Vector3 v1( 1.f, 2.f, 3.f );
    const Vector3 v2( 4.f, 5.f, 6.f );
    const Vector3 v3( 3.f, -23.f, 100.f );

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

    const Vector3 zero(0.f, 0.f, 0.f);
    const Vector3 one(1.f, 1.f, 1.f);
    const Vector3 forwardVector(0.f, 0.f, -1.f);
    const Vector3 backwardVector(0.f, 0.f, 1.f);
    const Vector3 downVector(0, -1.f, 0);
    const Vector3 leftVector(-1.f, 0, 0);
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

    v = +v1;
    if ( v != v1 )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    v = -v1;
    if ( v != Vector3( -v1.x, -v1.y, -v1.z ) )
    {
        printf("ERROR: - (unary)\n");
        success = false;
    }

    {
        auto vc = Vector3(Colors::CornflowerBlue);

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

    v = v1;
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

    v *= 2.f;
    if ( v != Vector3(8,20,36) )
    {
        printf("ERROR: *=f %f %f %f ... 8 20 36\n", v.x, v.y, v.z );
        success = false;
    }

    v /= 2.f;
    if ( v != Vector3(4,10,18) )
    {
        printf("ERROR: */f %f %f %f ... 4 10 18\n", v.x, v.y, v.z );
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
        const Vector3 a(-1.f, 4.f, -3.f);
        const Vector3 b(2.f, 1.f, -1.f );
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
        const Vector3 a(-1.f, 4.f, -3.f);
        const Vector3 b(2.f, 1.f, -1.f);
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
        const Vector3 a(1.f, 2.f, 3.f);
        const Vector3 b(4.f, 5.f, 6.f);

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
        const Vector3 a(1.f, -2.f, 3.f);
        const Vector3 b(-5.f, 6.f, -7.f);
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
        const Vector3 value1(11.0f, 12.0f, 13.0f);
        const Vector3 value2(21.0f, 22.0f, 23.0f);
        const Vector3 value3(31.0f, 32.0f, 33.0f);

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
        const Vector3 position1(1.0f, 2.0f, 5.f);
        const Vector3 position2(-1.0f, 4.0f, 4.f);
        const Vector3 position3(2.0f, 6.0f, 3.f);
        const Vector3 position4(3.0f, 8.0f, 2.f);

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
        const Vector3 p1(0.f, 1.f, 2.0f);
        const Vector3 t1(0.f, tanf(XMConvertToRadians(30.f)), tanf(XMConvertToRadians(20.f)));
        const Vector3 p2(-2.f, 2.f, 5.f);
        const Vector3 t2(0.f, tanf(XMConvertToRadians(-5.f)), tanf(XMConvertToRadians(-4.f)));

        // 0.5
        Vector3 result(-1.0f, 1.583105f, 3.55423713f);
        v = Vector3::Hermite(p1, t1, p2, t2, 0.5f);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite(2) 0.5 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 0
        v = Vector3::Hermite(p1, t1, p2, t2, 0.f);
        if (!XMVector3NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 0.f, v);
        if (!XMVector3NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite(2) 0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // 1
        v = Vector3::Hermite(p1, t1, p2, t2, 1.f);
        if (!XMVector3NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 1.f, v);
        if (!XMVector3NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite(2) 1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // <0
        result = Vector3(-10.0f, 3.86557627f, 15.6839724f);
        v = Vector3::Hermite(p1, t1, p2, t2, -1.f);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, -1.f, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite(2) <0 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        // >1
        result = Vector3(8.0f, -2.19525433f, -9.551766f);
        v = Vector3::Hermite(p1, t1, p2, t2, 2.f);
        if (!XMVector3NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite >1 %f %f %f\n", v.x, v.y, v.z);
            success = false;
        }

        Vector3::Hermite(p1, t1, p2, t2, 2.f, v);
        if (!XMVector3NearEqual(v, result, VEPSILON))
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
        const Vector3 vec(1.f, 2.f, 3.f);
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
        if (!XMVector4NearEqual(vec4, result4, VEPSILON2))
        {
            printf("ERROR: transmat(3) %f %f %f %f... %f %f %f %f\n", vec4.x, vec4.y, vec4.z, vec4.w, result4.x, result4.y, result4.z, result4.w);
            success = false;
        }
    }

    // Transform (Quaternion)
    {
        const Vector3 vec(1.f, 2.f, 3.f);
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
        const Vector3 vec(1.f, 2.f, 3.f);
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

        auto buff = std::make_unique<Vector3[]>(std::size(points));

        Vector3::Transform(&points[0], std::size(points), m, buff.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector3 result = Vector3::Transform(points[j], m);
            v = buff[j];
            if (!XMVector3NearEqual(v, result, VEPSILON2))
            {
                printf("ERROR: transarr %zu - %f %f %f ... %f %f %f\n", j, v.x, v.y, v.z, result.x, result.y, result.z);
                success = false;
            }
        }

        auto buff4 = std::make_unique<Vector4[]>(std::size(points));

        Vector3::Transform(&points[0], std::size(points), m, buff4.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector4 result;
            Vector3::Transform(points[j], m, result);
            Vector4 vec4 = buff4[j];
            if (!XMVector4NearEqual(vec4, result, VEPSILON2))
            {
                printf("ERROR: transarr(2) %zu - %f %f %f %f ... %f %f %f %f\n", j, vec4.x, vec4.y, vec4.z, vec4.w, result.x, result.y, result.z, result.w);
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

        auto buff = std::make_unique<Vector3[]>(std::size(points));

        Vector3::TransformNormal(&points[0], std::size(points), m, buff.get());

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector3 result = Vector3::TransformNormal(points[j], m);
            v = buff[j];
            if (!XMVector3NearEqual(v, result, VEPSILON2))
            {
                printf("ERROR: transnormarr %zu - %f %f %f ... %f %f %f\n", j, v.x, v.y, v.z, result.x, result.y, result.z);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 + v2;
    if ( v != Vector3( 5.f, 7.f, 9.f) )
    {
        printf("ERROR: + %f %f %f ... 5 7 9\n", v.x, v.y, v.z );
        success = false;
    }

    v = v2 - v1;
    if ( v != Vector3( 3.f, 3.f, 3.f) )
    {
        printf("ERROR: - %f %f %f ... 3 3 3\n", v.x, v.y, v.z );
        success = false;
    }

    v = v1 * v2;
    if ( v != Vector3( 4.f, 10.f, 18.f) )
    {
        printf("ERROR: * %f %f %f ... 4 10 8\n", v.x, v.y, v.z );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector3( 2.f, 4.f, 6.f) )
    {
        printf("ERROR: *f %f %f %f ... 2 4 6\n", v.x, v.y, v.z );
        success = false;
    }

    v = 2.f * v1;
    if ( v != Vector3( 2.f, 4.f, 6.f) )
    {
        printf("ERROR: f* %f %f %f ... 2 4 6\n", v.x, v.y, v.z );
        success = false;
    }

    v = v2 / v1;
    if ( v != Vector3( 4.f, 2.5f, 2.f) )
    {
        printf("ERROR: / %f %f %f ... 4 2.5 2\n", v.x, v.y, v.z );
        success = false;
    }

    v = v1 / 2.f;
    if ( v != Vector3( 0.5f, 1.f, 1.5f) )
    {
        printf("ERROR: /f %f %f %f ... 0.5 1 1.5\n", v.x, v.y, v.z );
        success = false;
    }

    v = 2.f / v2;
    if ( v != Vector3( 0.5f, 0.4f, (2.f/6.f)) )
    {
        printf("ERROR: f/ %f %f %f ... 0.5 4 0.3333 \n", v.x, v.y, v.z );
        success = false;
    }

    return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestV4()
{
    // Vector4
    static_assert(std::is_nothrow_default_constructible<Vector4>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Vector4>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Vector4>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Vector4>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Vector4>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Vector4 upVector( 0, 1.f, 0, 0 );
    const Vector4 rightVector( 1.f, 0, 0, 0 );
    const Vector4 v1( 1.f, 2.f, 3.f, 4.f );
    const Vector4 v2( 4.f, 5.f, 6.f, 7.f );
    const Vector4 v3( 3.f, -23.f, 100.f, 0.f );

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

    const Vector4 zero(0.f, 0.f, 0.f, 0.f);
    const Vector4 one(1.f, 1.f, 1.f, 1.f);
    const Vector4 backwardVector(0.f, 0.f, 1.f, 0.f);
    const Vector4 wVector(0.f, 0.f, 0.f, 1.f);
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

    v = +v1;
    if ( v != v1 )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    v = -v1;
    if ( v != Vector4( -v1.x, -v1.y, -v1.z, -v1.w ) )
    {
        printf("ERROR: - (unary)\n");
        success = false;
    }

    {
        auto vc = Vector4(Colors::CornflowerBlue);

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

    v = v1;
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

    v *= 2.f;
    if ( v != Vector4(8,20,36,56 ) )
    {
        printf("ERROR: *=f %f %f %f %f ... 8 20 36 56\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v /= 2.f;
    if ( v != Vector4(4,10,18,28) )
    {
        printf("ERROR: */f %f %f %f %f ... 4 10 18 28\n", v.x, v.y, v.z, v.w );
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
        const Vector4 a(-1.f, 4.f, -3.f, 1000.0f);
        const Vector4 b(2.f, 1.f, -1.f, 0.f);
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
        const Vector4 a(-1.f, 4.f, -3.f, 1000.0f);
        const Vector4 b(2.f, 1.f, -1.f, 0.f);
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
        const Vector4 a(1.f, 2.f, 3.f, 4.f);
        const Vector4 b(5.f, 6.f, 7.f, 8.f);

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
        const Vector4 a(1.f, -2.f, 3.f, 4.f);
        const Vector4 b(-5.f, 6.f, -7.f, 8.f);
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
        const Vector4 value1(11.0f, 12.0f, 13.0f, 14.f);
        const Vector4 value2(21.0f, 22.0f, 23.0f, 24.f);
        const Vector4 value3(31.0f, 32.0f, 33.0f, 34.f);

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
        const Vector4 position1(1.0f, 2.0f, 5.f, -1.0f);
        const Vector4 position2(-1.0f, 4.0f, 4.f, -2.0f);
        const Vector4 position3(2.0f, 6.0f, 3.f, -6.0f);
        const Vector4 position4(3.0f, 8.0f, 2.f, -8.0f);

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
        const Vector4 p1(0.f, 1.f, 2.0f, 3.f);
        const Vector4 t1(0.f, tanf(XMConvertToRadians(30.f)), tanf(XMConvertToRadians(20.f)), tanf(XMConvertToRadians(10.f)));
        const Vector4 p2(-2.f, 2.f, 5.f, 1.5f);
        const Vector4 t2(0.f, tanf(XMConvertToRadians(-5.f)), tanf(XMConvertToRadians(-4.f)), tanf(XMConvertToRadians(-3.f)));

        // 0.5
        Vector4 result(-1.0f, 1.583105f, 3.55423713f, 2.27859187f);
        v = Vector4::Hermite(p1, t1, p2, t2, 0.5f);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 0.5f, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite(2) 0.5 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 0
        v = Vector4::Hermite(p1, t1, p2, t2, 0.f);
        if (!XMVector4NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 0.f, v);
        if (!XMVector4NearEqual(v, p1, VEPSILON))
        {
            printf("ERROR: hermite(2) 0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // 1
        v = Vector4::Hermite(p1, t1, p2, t2, 1.f);
        if (!XMVector4NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 1.f, v);
        if (!XMVector4NearEqual(v, p2, VEPSILON))
        {
            printf("ERROR: hermite(2) 1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // <0
        result = Vector4(-10.0f, 3.86557627f, 15.6839724f, -5.10049248f);
        v = Vector4::Hermite(p1, t1, p2, t2, -1.f);
        if (!XMVector4NearEqual(v, result, VEPSILON2))
        {
            printf("ERROR: hermite <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, -1.f, v);
        if (!XMVector4NearEqual(v, result, VEPSILON2))
        {
            printf("ERROR: hermite(2) <0 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        // >1
        result = Vector4(8.0f, -2.19525433f, -9.551766f, 9.143023f);
        v = Vector4::Hermite(p1, t1, p2, t2, 2.f);
        if (!XMVector4NearEqual(v, result, VEPSILON))
        {
            printf("ERROR: hermite >1 %f %f %f %f\n", v.x, v.y, v.z, v.w);
            success = false;
        }

        Vector4::Hermite(p1, t1, p2, t2, 2.f, v);
        if (!XMVector4NearEqual(v, result, VEPSILON))
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

        auto buff = std::make_unique<Vector4[]>(std::size(points));

        Vector4::Transform( &points[0], std::size(points), m, buff.get() );

        for (size_t j = 0; j < std::size(points); ++j)
        {
            Vector4 result = Vector4::Transform(points[j], m);
            v = buff[j];
            if (!XMVector4NearEqual(v, result, VEPSILON2))
            {
                printf("ERROR: transarr %zu - %f %f %f %f ... %f %f %f %f\n", j, v.x, v.y, v.z, v.w, result.x, result.y, result.z, result.w);
                success = false;
            }
        }
    }

    // binary operators
    v = v1 + v2;
    if ( v != Vector4( 5.f, 7.f, 9.f, 11.f ) )
    {
        printf("ERROR: + %f %f %f %f ... 5 7 9 11\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v2 - v1;
    if ( v != Vector4( 3.f, 3.f, 3.f, 3.f ) )
    {
        printf("ERROR: - %f %f %f %f ... 3 3 3 3\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v1 * v2;
    if ( v != Vector4( 4.f, 10.f, 18.f, 28.f) )
    {
        printf("ERROR: * %f %f %f %f ... 4 10 8 28\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v1 * 2.f;
    if ( v != Vector4( 2.f, 4.f, 6.f, 8.f) )
    {
        printf("ERROR: *f %f %f %f %f ... 2 4 6 8\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = 2.f * v1;
    if ( v != Vector4( 2.f, 4.f, 6.f, 8.f) )
    {
        printf("ERROR: f* %f %f %f %f ... 2 4 6 8\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v2 / v1;
    if ( v != Vector4( 4.f, 2.5f, 2.f, 1.75f ) )
    {
        printf("ERROR: / %f %f %f %f ... 4 2.5 2 1.75\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = v1 / 2.f;
    if ( v != Vector4( 0.5f, 1.f, 1.5f, 2.f) )
    {
        printf("ERROR: /f %f %f %f %f ... 0.5 1 1.5 2\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    v = 2.f / v1;
    if ( v != Vector4( 2.f, 1.f, (2.f/3.f), 0.5f) )
    {
        printf("ERROR: f/ %f %f %f %f ... 2 1 0.6666 0.5\n", v.x, v.y, v.z, v.w );
        success = false;
    }

    return (success) ? 0 : 1;
}



//-------------------------------------------------------------------------------------
int TestM()
{
    // Matrix
    static_assert(std::is_nothrow_default_constructible<Matrix>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Matrix>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Matrix>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Matrix>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Matrix>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = 1;

    const Matrix a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

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

    b = +a;
    if ( b != a )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    b = -a;
    if ( b != Matrix( -a._11, -a._12, -a._13, -a._14, -a._21, -a._22, -a._23, -a._24, -a._31, -a._32, -a._33, -a._34, -a._41, -a._42, -a._43, -a._44 ) )
    {
        printf("ERROR: - (unary)\n");
        success = false;
    }

    b = a;
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

    {
        constexpr Matrix mrotx(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1);
        constexpr Matrix mroty(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);
        constexpr Matrix mrotz(0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(0, XM_PIDIV2, 0), mrotx);
        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(XM_PIDIV2, 0, 0), mroty);
        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(0, 0, XM_PIDIV2), mrotz);

        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(Vector3(XM_PIDIV2, 0, 0)), mrotx);
        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(Vector3(0, XM_PIDIV2, 0)), mroty);
        VerifyNearEqual(Matrix::CreateFromYawPitchRoll(Vector3(0, 0, XM_PIDIV2)), mrotz);

        VerifyNearEqual(mrotx.ToEuler(), Vector3(XM_PIDIV2, 0, 0));
        VerifyNearEqual(mroty.ToEuler(), Vector3(0, XM_PIDIV2, 0));
        VerifyNearEqual(mrotz.ToEuler(), Vector3(0, 0, XM_PIDIV2));
    }

    {
        constexpr float inc = XM_PIDIV4 / 2.f;
        for (float y = -XM_2PI; y < XM_2PI; y += inc)
        {
            for (float p = -XM_2PI; p < XM_2PI; p += inc)
            {
                for (float r = -XM_2PI; r < XM_2PI; r += inc)
                {
                    Matrix checkm = Matrix::CreateRotationZ(r) * Matrix::CreateRotationX(p) * Matrix::CreateRotationY(y);

                    auto m = Matrix::CreateFromYawPitchRoll(y, p, r);
                    VerifyNearEqual(m, checkm);

                    Vector3 angles(p, y, r);
                    VerifyNearEqual(Matrix::CreateFromYawPitchRoll(angles), checkm);

                    Vector3 ev = m.ToEuler();
                    if (!XMVector3NearEqual(ev, angles, VEPSILON))
                    {
                        // Check for equivalent rotation
                        XMMATRIX check = checkm;
                        XMMATRIX m2 = XMMatrixRotationZ(ev.z) * XMMatrixRotationX(ev.x) * XMMatrixRotationY(ev.y);

                        if (!XMVector4NearEqual(m2.r[0], check.r[0], VEPSILON2)
                            || !XMVector4NearEqual(m2.r[1], check.r[1], VEPSILON2)
                            || !XMVector4NearEqual(m2.r[2], check.r[2], VEPSILON2)
                            || !XMVector4NearEqual(m2.r[3], check.r[3], VEPSILON2))
                        {
                            printf("ERROR: %s:%d: %f %f %f (expecting %f %f %f)\n", __FUNCTION__, __LINE__,
                                ev.x, ev.y, ev.z, p, y, r);
                            success = false;
                        }
                    }
                }
            }
        }
    }

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

    // binary operators
    const Matrix a2(7, 1, 6, 2, 10, 33, 0.5f, 0.25f, 5, 9, 11, 24, 83, 0.1f, 4, 3);

    Matrix m = a + a2;
    if ( m != Matrix( 8, 3, 9, 6, 15, 39, 7.5f, 8.25f, 14, 19, 22, 36, 96, 14.1f, 19, 19) )
    {
        printf("ERROR: +\n\t%f %f %f %f ... 8 3 9 6\n\t%f %f %f %f ... 15 39 7.5 8.25\n\t%f %f %f %f ... 14 19 22 36\n\t%f %f %f %f ... 96 14.1 19 19\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    m = a2 - a;
    if ( m != Matrix( 6, -1.f, 3, -2.f, 5, 27, -6.5f, -7.75f, -4.f, -1.f, 0, 12, 70, -13.9f, -11.f, -13.f) )
    {
        printf("ERROR: -\n\t%f %f %f %f ... 6 -1 3 -2\n\t%f %f %f %f ... 5 27 -6.5 -7.75\n\t%f %f %f %f ... -4 -1 0 12\n\t%f %f %f %f ... 70 -13.9 -11 -13\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    const Matrix a3( 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25);

    m = a * a3;
    if ( m != Matrix( 180, 190, 200, 210, 436, 462, 488, 514, 692, 734, 776, 818, 948, 1006, 1064, 1122 ) )
    {
        printf("ERROR: *\n\t%f %f %f %f ... 180 190 200 210\n\t%f %f %f %f ... 436 462 488 514\n\t%f %f %f %f ... 692 734 776 818\n\t%f %f %f %f ... 948 1006 1064 1122\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    m = a * 2.f;
    if ( m != Matrix(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 ) )
    {
        printf("ERROR: *f\n\t%f %f %f %f ... 2 4 6 8\n\t%f %f %f %f ... 10 12 14 16\n\t%f %f %f %f ... 18 20 22 24\n\t%f %f %f %f ... 26 28 30 32\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    m = 2.f * a;
    if ( m != Matrix(2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 ) )
    {
        printf("ERROR: f*\n\t%f %f %f %f ... 2 4 6 8\n\t%f %f %f %f ... 10 12 14 16\n\t%f %f %f %f ... 18 20 22 24\n\t%f %f %f %f ... 26 28 30 32\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    m = a / 2.f;
    if ( m != Matrix(0.5, 1, 1.5f, 2, 2.5f, 3, 3.5f, 4, 4.5f, 5, 5.5f, 6, 6.5f, 7, 7.5f, 8) )
    {
        printf("ERROR: /f\n\t%f %f %f %f ... 0.5 1 1.5 2\n\t%f %f %f %f ... 2.5 3 3.5 4\n\t%f %f %f %f ... 4.5 5 5.5 6\n\t%f %f %f %f ... 6.5 7 7.5 8\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    m = a3 / a;
    if ( m != Matrix( 10, 5.5f, 4, 3.25f, 2.8f, 2.5f, 16.f/7.f, 2.125f, 2, 1.9f, 20.f/11.f, 1.75f, 22.f/13.f, 23.f/14.f, 24.f/15.f, 25.f/16.f) )
    {
        printf("ERROR: /\n\t%f %f %f %f ... 10 5.5 4 3.25\n\t%f %f %f %f ... 2.8 2.5 2.285714 2.125\n\t%f %f %f %f ... 2 1.9 1.818182 1.75\n\t%f %f %f %f ... 1.692308 1.642857 1.6 1.5625\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }
    m = 2.f / a;
    if ( m != Matrix(2, 1, 2.f/3.f, 0.5f, 0.4, 1.f/3.f, 2.f/7.f, 0.25f, 2.f/9.f, 1.f/5.f, 2.f/11.f, 1.f/6.f, 2.f/13.f, 1.f/7.f, 2.f/15.f, 0.125f ) )
    {
        printf("ERROR: f/\n\t%f %f %f %f ... 2 1 0.6666 0.5\n\t%f %f %f %f ... 0.4 0.33333 0.285714 0.25 \n\t%f %f %f %f ... 0.22222 0.2 0.181818 0.166667\n\t%f %f %f %f ... 0.153846 0.142857 0.133333 0.125\n",
            m._11, m._12, m._13, m._14,
            m._21, m._22, m._23, m._24,
            m._31, m._32, m._33, m._34,
            m._41, m._42, m._43, m._44);
        success = false;
    }

    return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestP()
{
    // Plane
    static_assert(std::is_nothrow_default_constructible<Plane>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Plane>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Plane>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Plane>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Plane>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Plane a(1, 2, 3, 4);
    const Plane b(Vector3(5, 6, 7), 8);

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
        auto vc = Plane(Colors::CornflowerBlue);

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
    static_assert(std::is_nothrow_default_constructible<Quaternion>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Quaternion>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Quaternion>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Quaternion>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Quaternion>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Quaternion a(1, 2, 3, 4);
    const Quaternion b(Vector3(5, 6, 7), 8);

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
        auto vc = Quaternion(Colors::CornflowerBlue);

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

    c = +b;
    if ( c != b )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    c = -b;
    if ( c != Vector4( -b.x, -b.y, -b.z, -b.w ) )
    {
        printf("ERROR: - (unary)\n");
        success = false;
    }

    c = b;
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

    {
        constexpr Quaternion qrotx(0.707107f, 0.000000f, 0.000000f, 0.707107f);
        constexpr Quaternion qroty(0.000000f, 0.707107f, 0.000000f, 0.707107f);
        constexpr Quaternion qrotz(0.000000f, 0.000000f, 0.707107f, 0.707107f);

        VerifyNearEqual(Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), XM_PIDIV2), qrotx);
        VerifyNearEqual(Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), XM_PIDIV2), qroty);
        VerifyNearEqual(Quaternion::CreateFromAxisAngle(Vector3(0, 0, 1), XM_PIDIV2), qrotz);

        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(0, XM_PIDIV2, 0), qrotx);
        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(XM_PIDIV2, 0, 0), qroty);
        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(0, 0, XM_PIDIV2), qrotz);

        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(Vector3(XM_PIDIV2, 0, 0)), qrotx);
        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(Vector3(0, XM_PIDIV2, 0)), qroty);
        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(Vector3(0, 0, XM_PIDIV2)), qrotz);

        VerifyNearEqual(qrotx.ToEuler(), Vector3(XM_PIDIV2, 0, 0));
        VerifyNearEqual(qroty.ToEuler(), Vector3(0, XM_PIDIV2, 0));
        VerifyNearEqual(qrotz.ToEuler(), Vector3(0, 0, XM_PIDIV2));

        VerifyNearEqual(Quaternion::CreateFromRotationMatrix(Matrix::CreateFromYawPitchRoll(0, XM_PIDIV2, 0)), qrotx);
        VerifyNearEqual(Quaternion::CreateFromRotationMatrix(Matrix::CreateFromYawPitchRoll(XM_PIDIV2, 0, 0)), qroty);
        VerifyNearEqual(Quaternion::CreateFromRotationMatrix(Matrix::CreateFromYawPitchRoll(0, 0, XM_PIDIV2)), qrotz);

        float angle = Quaternion::Angle(Quaternion::Identity, qrotx);
        if (!XMScalarNearEqual(angle, XM_PIDIV2, EPSILON))
        {
            printf("ERROR: Angle I,X %f\n", angle);
            success = false;
        }

        angle = Quaternion::Angle(qrotx, qroty);
        if (!XMScalarNearEqual(angle, XM_2PI / 3.f, EPSILON))
        {
            printf("ERROR: Angle X,Y %f\n", angle);
            success = false;
        }

        angle = Quaternion::Angle(qrotx, qrotx);
        if (!XMScalarNearEqual(angle, 0.f, EPSILON))
        {
            printf("ERROR: Angle X,X %f\n", angle);
            success = false;
        }

        {
            constexpr float inc = XM_PIDIV4 / 2.f;
            for (float y = -XM_2PI; y < XM_2PI; y += inc)
            {
                for (float p = -XM_2PI; p < XM_2PI; p += inc)
                {
                    for (float r = -XM_2PI; r < XM_2PI; r += inc)
                    {
                        auto qx = Quaternion(cosf(p / 2.f), 0.f, 0.f, sinf(p / 2.f));
                        auto qy = Quaternion(0.f, cosf(y / 2.f), 0.f, -sinf(y / 2.f));
                        auto qz = Quaternion(0.f, 0.f, cosf(r / 2.f), -sinf(r / 2.f));
                        Quaternion checkq = qz * qx * qy;

                        auto q = Quaternion::CreateFromYawPitchRoll(y, p, r);
                        VerifyNearEqual(q, checkq);

                        Vector3 angles(p, y, r);
                        VerifyNearEqual(Quaternion::CreateFromYawPitchRoll(angles), checkq);

                        Vector3 ev = q.ToEuler();
                        if (!XMVector3NearEqual(ev, angles, VEPSILON))
                        {
                            // Check for equivalent rotation
                            qx = Quaternion(cosf(ev.x / 2.f), 0.f, 0.f, sinf(ev.x / 2.f));
                            qy = Quaternion(0.f, cosf(ev.y / 2.f), 0.f, -sinf(ev.y / 2.f));
                            qz = Quaternion(0.f, 0.f, cosf(ev.z / 2.f), -sinf(ev.z / 2.f));
                            Quaternion q2 = qz * qx * qy;

                            float dot = fabsf(q2.Dot(checkq));
                            if (!XMScalarNearEqual(dot, 1.f, EPSILON2))
                            {
                                printf("ERROR: %s:%d: %f %f %f (expecting %f %f %f)\n", __FUNCTION__, __LINE__,
                                    ev.x, ev.y, ev.z, p, y, r);
                                success = false;
                            }
                        }
                    }
                }
            }
        }
    }

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

    {
        constexpr Quaternion qrotx(0.707107f, 0.000000f, 0.000000f, 0.707107f);
        constexpr Quaternion qroty(0.000000f, 0.707107f, 0.000000f, 0.707107f);
        constexpr Quaternion qrotz(0.000000f, 0.000000f, 0.707107f, 0.707107f);

        Quaternion q;
        qrotx.RotateTowards(qroty, 0.f, q);
        VerifyNearEqual(q, qrotx);

        qrotx.RotateTowards(qroty, XM_PIDIV4, q);
        VerifyNearEqual(q, Quaternion(0.497051775f, 0.f, 0.312459797f, 0.809511602f));

        qrotx.RotateTowards(qroty, XM_PIDIV2, q);
        VerifyNearEqual(q, Quaternion(0.211324900f, 0.f, 0.577350438f, 0.788675308f));

        qrotx.RotateTowards(qrotz, XM_PIDIV4, q);
        VerifyNearEqual(q, Quaternion(0.497051775f, -0.312459797f, 0.f, 0.809511602f));

        qrotx.RotateTowards(qroty, XM_2PI, q);
        VerifyNearEqual(q, qroty);

        qrotx.RotateTowards(qrotz, XM_2PI, q);
        VerifyNearEqual(q, qrotz);

        Quaternion::FromToRotation(Vector3::Forward, Vector3::Forward, q);
        VerifyNearEqual(q, Quaternion::Identity);

        Quaternion::FromToRotation(Vector3::Forward, Vector3::Backward, q);
        VerifyNearEqual(q, Quaternion::CreateFromAxisAngle(Vector3::Down, XM_PI));

        Quaternion::FromToRotation(Vector3::Right, Vector3::Left, q);
        VerifyNearEqual(q, Quaternion::CreateFromAxisAngle(Vector3::Backward, XM_PI));

        q.Quaternion::FromToRotation(Vector3::Right, Vector3::Left);
        VerifyNearEqual(q, Quaternion::CreateFromAxisAngle(Vector3::Backward, XM_PI));

        Quaternion::FromToRotation(Vector3::Forward, Vector3::Up, q);
        VerifyNearEqual(q, qrotx);

        Quaternion::LookRotation(Vector3::Forward, Vector3::Forward, q);
        VerifyNearEqual(q, Quaternion::Identity);

        Quaternion::LookRotation(Vector3::Forward, Vector3::Up, q);
        VerifyNearEqual(q, Quaternion::Identity);

        Quaternion::LookRotation(Vector3::Up, Vector3::Right, q);
        VerifyNearEqual(q, Quaternion(0.5f, 0.f, -0.707107f, 0.5f));

        Quaternion::LookRotation(Vector3::Right, Vector3::Backward, q);
        VerifyNearEqual(q, Quaternion(0.5f, -0.5f, 0.5f, 0.5f));
   }

    // binary operators
    Quaternion q = a + b;
    if ( q != Vector4( 6, 8, 10, 12 ) )
    {
        printf("ERROR: + %f %f %f %f ... 6 8 10 12\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    q = b - a;
    if ( q != Vector4( 4, 4, 4, 4 ) )
    {
        printf("ERROR: - %f %f %f %f ... 4 4 4 4\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    q = a * b;
    if ( q != Vector4( 32, 32, 56, -6.f) )
    {
        printf("ERROR: * %f %f %f %f ... 32 32 56 -6\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    q = a * 2.f;
    if ( q != Vector4( 2.f, 4.f, 6.f, 8.f) )
    {
        printf("ERROR: *f %f %f %f %f ... 2 4 6 8\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    q = 2.f * a;
    if ( q != Vector4( 2.f, 4.f, 6.f, 8.f) )
    {
        printf("ERROR: f* %f %f %f %f ... 2 4 6 8\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    q = b / Quaternion::Identity;
    if ( q != Vector4( 5, 6, 7, 8 ) )
    {
        printf("ERROR: / %f %f %f %f ... 5 6 7 8\n", q.x, q.y, q.z, q.w );
        success = false;
    }

    return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestC()
{
    // Color
    static_assert(std::is_nothrow_default_constructible<Color>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Color>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Color>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Color>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Color>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

    bool success = true;

    const Color a(1, 2, 3);
    const Color b(4, 5, 6, 7);

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
        auto vc = Color(Colors::CornflowerBlue);

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

    c = +b;
    if ( c != b )
    {
        printf("ERROR: + (unary)\n");
        success = false;
    }

    c = -b;
    if ( c != Vector4( -b.x, -b.y, -b.z, -b.w ) )
    {
        printf("ERROR: - (unary)\n");
        success = false;
    }

    c = b;
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

    // binary operators
    c = a + b;
    if ( c != Vector4( 5, 7, 9, 8 ) )
    {
        printf("ERROR: + %f %f %f %f ... 5 7 9 8\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    c = b - a;
    if ( c != Vector4( 3.f, 3.f, 3.f, 6.f ) )
    {
        printf("ERROR: - %f %f %f %f ... 3 3 3 6\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    c = a * b;
    if ( c != Vector4( 4, 10, 18, 7 ) )
    {
        printf("ERROR: * %f %f %f %f ... 4 10 18 7\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    c = a * 2.f;
    if ( c != Vector4( 2.f, 4.f, 6.f, 2.f) )
    {
        printf("ERROR: *f %f %f %f %f ... 2 4 6 2\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    c = 2.f * a;
    if ( c != Vector4( 2.f, 4.f, 6.f, 2.f) )
    {
        printf("ERROR: f* %f %f %f %f ... 2 4 6 2\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    c = b / a;
    if ( c != Vector4( 4.f, 2.5f, 2.f, 7.f ) )
    {
        printf("ERROR: / %f %f %f %f ... 4 2.5 2 7\n", c.x, c.y, c.z, c.w );
        success = false;
    }

    return (success) ? 0 : 1;
}


//-------------------------------------------------------------------------------------
int TestRay()
{
    // Ray
    static_assert(std::is_nothrow_default_constructible<Ray>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Ray>::value, "Copy Ctor.");
    static_assert(std::is_copy_assignable<Ray>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Ray>::value, "Move Ctor.");
    static_assert(std::is_move_assignable<Ray>::value, "Move Assign.");
        // As of DirectXMath 3.13, this type is_nothrow_copy/move_assignable

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
    static_assert(std::is_nothrow_default_constructible<Viewport>::value, "Default Ctor.");
    static_assert(std::is_nothrow_copy_constructible<Viewport>::value, "Copy Ctor.");
    static_assert(std::is_nothrow_copy_assignable<Viewport>::value, "Copy Assign.");
    static_assert(std::is_nothrow_move_constructible<Viewport>::value, "Move Ctor.");
    static_assert(std::is_nothrow_move_assignable<Viewport>::value, "Move Assign.");

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
        if (!XMVector3NearEqual(p, Vector3(231.296143f, 265.606598f, 0.993776858f), VEPSILON3))
        {
            printf("ERROR: Project %f %f %f ... 231.296143 265.606598 0.993776858\n", p.x, p.y, p.z);
            success = false;
        }

        Vector3 r;
        vp2.Project(Vector3(0.5f, 0.75f, 0.25f), proj, view, world, r);
        if (!XMVector3NearEqual(r, Vector3(231.296143f, 265.606598f, 0.993776858f), VEPSILON3))
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

        p = vp2.Unproject(Vector3(231.f,265.f,0.993776858f), proj, view, world);
        if (!XMVector3NearEqual(p, Vector3(0.488145798f, 0.748996973f, 0.260956854f), VEPSILON3))
        {
            printf("ERROR: Unproject %f %f %f ... 0.488145798 0.748996973 0.260956854\n", p.x, p.y, p.z);
            success = false;
        }

        vp2.Unproject(Vector3(231.f, 265.f, 0.993776858f), proj, view, world, r);
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

#if defined(__dxgi1_2_h__) || defined(__d3d11_x_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
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
#endif

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

    for (const auto& it : map)
    {
        if (it.second != expected)
        {
            printf("std::less<%s> error: expecting %d, got %d\n", typeid(T).name(), expected, it.second);
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
#ifdef TEST_D3D11
extern int TestD3D11();
#endif

#ifdef TEST_D3D12
extern int TestD3D12();
#endif

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
#ifdef TEST_D3D11
    { "D3D11", TestD3D11 },
#endif
#ifdef TEST_D3D12
    { "D3D12", TestD3D12 },
#endif
    { "std::less", TestL },
};

#ifdef _WIN32
int __cdecl wmain()
#else
int main()
#endif
{
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    size_t npass = 0;
    bool success = true;

    printf("*** SimpleMathTest (using DirectXMath version %03d)\n", DIRECTX_MATH_VERSION);

    if (!XMVerifyCPUSupport())
    {
        printf("FAILED: XMVerifyCPUSupport reports a failure on this platform\n");
        return 1;
    }

    for( size_t j = 0; j < std::size(g_Tests); ++j )
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
        printf("FAILED, passed %zu tests\n", npass );
        return 1;
    }
}
