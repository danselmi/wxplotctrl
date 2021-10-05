/////////////////////////////////////////////////////////////////////////////
// Name:        plotdraw.cpp
// Purpose:     wxPlotDrawer and friends
// Author:      John Labenski
// Modified by:
// Created:     8/27/2002
// Copyright:   (c) John Labenski
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include "wx/wx.h"
#include "wx/math.h"
#include "wx/dynarray.h"

#include "wx/plotctrl/plotdraw.h"
#include "wx/plotctrl/plotctrl.h"
#include "wx/plotctrl/plotmark.h"
#include "wx/plotctrl/plotdata.h"

#include <math.h>
#include <float.h>
#include <limits.h>

// MSVC hogs global namespace with these min/max macros - remove them
#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif
#ifdef GetYValue   // Visual Studio 7 defines this
    #undef GetYValue
#endif

// Draw borders around the axes, title, and labels for sizing testing
//#define DRAW_BORDERS

//-----------------------------------------------------------------------------
// Consts
//-----------------------------------------------------------------------------

#define INITIALIZE_FAST_GRAPHICS

//inline void wxPLOTCTRL_DRAW_LINE(wxDC *dc, int win, int pen, int x0, int y0, int x1, int y1)
#define wxPLOTCTRL_DRAW_LINE(dc, win, pen, x0, y0, x1, y1) \
    dc->DrawLine(x0, y0, x1, y1);

//inline void wxPLOTCTRL_DRAW_CIRCLE(wxDC *dc, int win, int pen, int x0, int y0)
#define wxPLOTCTRL_DRAW_ELLIPSE(dc, win, pen, x0, y0, w, h) \
    dc->DrawEllipse(x0, y0, w, h);

class wxRangeDouble;
class wxRangeDoubleSelection;

WX_DECLARE_OBJARRAY_WITH_DECL(wxRangeDouble, wxArrayRangeDouble, class);
WX_DECLARE_OBJARRAY_WITH_DECL(wxRangeDoubleSelection, wxArrayRangeDoubleSelection, class);

//=============================================================================
// wxRangeDouble
//=============================================================================

class wxRangeDouble
{
public:
    inline wxRangeDouble(wxDouble min_=0, wxDouble max_=0) : m_min(min_), m_max(max_) {}

    // Get the width of the range
    inline wxDouble GetRange() const {return m_max - m_min;}
    // Get/Set the min/max values of the range
    inline wxDouble GetMin() const {return m_min;}
    inline wxDouble GetMax() const {return m_max;}
    inline void SetMin(wxDouble min_) {m_min = min_;}
    inline void SetMax(wxDouble max_) {m_max = max_;}
    inline void Set(wxDouble min_, wxDouble max_) {m_min = min_, m_max = max_;}

    // Shift the range by i
    void Shift(wxDouble i) {m_min += i; m_max += i;}

    // Is the range empty, min < max
    inline bool IsEmpty() const {return m_min > m_max;}

    // Swap the min and max values
    inline void SwapMinMax() {wxDouble temp = m_min; m_min = m_max; m_max = temp;}

    // returns -1 for i < min, 0 for in range, +1 for i > m_max
    inline int Position(wxDouble i) const {return i < m_min ? -1 : i > m_max ? 1 : 0;}

    // Is this point or the range within this range
    inline bool Contains(wxDouble i) const {return (i>=m_min)&&(i<=m_max);}
    inline bool Contains(const wxRangeDouble &r) const
         {return (r.m_min>=m_min)&&(r.m_max<=m_max);}

    // returns if the range intersects the given range
    inline bool Intersects(const wxRangeDouble& r) const
        {return Intersect(r).IsEmpty();}
    // returns the intersection of the range with the other
    inline wxRangeDouble Intersect(const wxRangeDouble& r) const
        {return wxRangeDouble(wxMax(m_min, r.m_min), wxMin(m_max, r.m_max));}
    // returns the union of the range with the other
    inline wxRangeDouble Union(const wxRangeDouble& r) const
        {return wxRangeDouble(wxMin(m_min, r.m_min), wxMax(m_max, r.m_max));}

    // no touches for double since what would be a good eps value?

    // combine this single point with the range by expanding the m_min/m_max to contain it
    //  if only_if_touching then only combine if there is overlap
    //  returns true if the range has been changed at all, false if not
    bool Combine(wxDouble i);
    bool Combine(const wxRangeDouble &r, bool only_if_touching = false);

    // delete range r from this, return true is anything was done
    //   if r spans this then this and right become wxEmptyRangeInt
    //   else if r is inside of this then this is the left side and right is the right
    //   else if r.m_min > m_min then this is the left side
    //   else if r.m_min < m_min this is the right side
    bool Delete(const wxRangeDouble &r, wxRangeDouble *right=NULL);

    // operators
    // no copy ctor or assignment operator - the defaults are ok

    // comparison
    inline bool operator==(const wxRangeDouble& r) const {return (m_min == r.m_min)&&(m_max == r.m_max);}
    inline bool operator!=(const wxRangeDouble& r) const {return !(*this == r);}

    // Adding ranges unions them to create the largest range
    inline wxRangeDouble operator+(const wxRangeDouble& r) const {return Union(r);}
    inline wxRangeDouble& operator+=(const wxRangeDouble& r) {if(r.m_min<m_min) m_min=r.m_min; if(r.m_max>m_max) m_max=r.m_max; return *this;}
    // Subtracting ranges intersects them to get the smallest range
    inline wxRangeDouble operator-(const wxRangeDouble& r) const {return Intersect(r);}
    inline wxRangeDouble& operator-=(const wxRangeDouble& r) {if(r.m_min>m_min) m_min=r.m_min; if(r.m_max<m_max) m_max=r.m_max; return *this;}

    // Adding/Subtracting with a double shifts the range
    inline wxRangeDouble operator+(const wxDouble i) const {return wxRangeDouble(m_min+i, m_max+i);}
    inline wxRangeDouble operator-(const wxDouble i) const {return wxRangeDouble(m_min-i, m_max-i);}
    inline wxRangeDouble& operator+=(const wxDouble i) {Shift(i); return *this;}
    inline wxRangeDouble& operator-=(const wxDouble i) {Shift(-i); return *this;}

    wxDouble m_min, m_max;
};

//=============================================================================
// wxRangeDoubleSelection - ordered 1D array of wxRangeDoubles, combines to minimze size
//=============================================================================

class wxRangeDoubleSelection
{
public :
    wxRangeDoubleSelection() {}
    wxRangeDoubleSelection(const wxRangeDouble& range) {if (!range.IsEmpty()) m_ranges.Add(range);}
    wxRangeDoubleSelection(const wxRangeDoubleSelection &ranges) {Copy(ranges);}

    // Make a full copy of the source
    void Copy(const wxRangeDoubleSelection &source)
        {
            m_ranges.Clear();
            WX_APPEND_ARRAY(m_ranges, source.GetRangeArray());
        }

    // Get the number of individual ranges
    inline int GetCount() const {return m_ranges.GetCount();}
    // Get the ranges themselves to iterate though for example
    const wxArrayRangeDouble& GetRangeArray() const {return m_ranges;}
    // Get a single range
    const wxRangeDouble& GetRange(int index) const;
    inline const wxRangeDouble& Item(int index) const {return GetRange(index);}
    // Get a range of the min range value and max range value
    wxRangeDouble GetBoundingRange() const;
    // Clear all the ranges
    void Clear() {m_ranges.Clear();}

    // Is this point or range contained in the selection
    inline bool Contains(wxDouble i) const {return Index(i) != wxNOT_FOUND;}
    inline bool Contains(const wxRangeDouble &range) const {return Index(range) != wxNOT_FOUND;}
    // Get the index of the range that contains this, or wxNOT_FOUND
    int Index(wxDouble i) const;
    int Index(const wxRangeDouble &range) const;

    // Get the nearest index of a range, index returned contains i or is the one just below
    //   returns -1 if it's below all the selected ones, or no ranges
    //   returns GetCount() if it's above all the selected ones
    int NearestIndex(wxDouble i) const;

    // Add the range to the selection, returning if anything was done, false if already selected
    bool SelectRange(const wxRangeDouble &range);
    // Remove the range to the selection, returning if anything was done, false if not already selected
    bool DeselectRange(const wxRangeDouble &range);

    // Set the min and max bounds of the ranges, returns true if anything was done
    bool BoundRanges(const wxRangeDouble &range);

    // operators
    inline const wxRangeDouble& operator[](int index) const {return GetRange(index);}

    wxRangeDoubleSelection& operator = (const wxRangeDoubleSelection& other) {Copy(other); return *this;}

protected :
    wxArrayRangeDouble m_ranges;
};

const wxRangeDouble wxEmptyRangeDouble(0, -1);
#include "wx/arrimpl.cpp"

WX_DEFINE_OBJARRAY(wxArrayRangeDouble);
WX_DEFINE_OBJARRAY(wxArrayRangeDoubleSelection);

// differs from wxRect2DDouble::Intersects by allowing for 0 width or height
inline bool wxPlotRect2DDoubleIntersects(const wxRect2DDouble &a, const wxRect2DDouble &b)
{
    return (wxMax(a.m_x, b.m_x) <= wxMin(a.GetRight(), b.GetRight())) &&
           (wxMax(a.m_y, b.m_y) <= wxMin(a.GetBottom(), b.GetBottom()));
}

// same as wxPlotRect2DDouble::Contains, but doesn't convert to wxPoint2DDouble
inline bool wxPlotRect2DDoubleContains(double x, double y, const wxRect2DDouble &rect)
{
    return ((x>=rect.m_x) && (y>=rect.m_y) && (x<=rect.GetRight()) && (y<=rect.GetBottom()));
}

// differs from wxRect2DDouble::GetOutCode by swaping top and bottom for plot origin
//inline wxOutCode wxPlotRect2DDoubleOutCode(double x, double y, const wxRect2DDouble &rect)
//{
//    return wxOutCode((x < rect.m_x         ? wxOutLeft   :
//                     (x > rect.GetRight()  ? wxOutRight  : wxInside)) +
//                     (y < rect.m_y         ? wxOutTop    :
//                     (y > rect.GetBottom() ? wxOutBottom : wxInside)));
//}
#define wxPlotRect2DDoubleOutCode(x, y, rect) \
           wxOutCode(((x) < rect.m_x         ? wxOutLeft   : \
                     ((x) > rect.GetRight()  ? wxOutRight  : wxInside)) + \
                     ((y) < rect.m_y         ? wxOutTop    : \
                     ((y) > rect.GetBottom() ? wxOutBottom : wxInside)))


// modified Cohen-Sutherland Algorithm for line clipping in at most two passes
//   the the original endless loop is too unstable
//   http://www.cc.gatech.edu/grads/h/Hao-wei.Hsieh/Haowei.Hsieh/code1.html for psuedo code
// The line connecting (x0,y0)-(x1,y1) is clipped to rect and which
//   points were clipped is returned.

enum ClipLine_Type
{
    ClippedNeither = 0x0000,
    ClippedFirstX  = 0x0001,
    ClippedFirstY  = 0x0002,
    ClippedFirst   = ClippedFirstX | ClippedFirstY,
    ClippedSecondX = 0x0010,
    ClippedSecondY = 0x0020,
    ClippedSecond  = ClippedSecondX | ClippedSecondY,
    ClippedBoth    = ClippedFirst | ClippedSecond,
    ClippedOut     = 0x0100   // no intersection, so can't clip
};

int ClipLineToRect(double &x0, double &y0,
                    double &x1, double &y1,
                    const wxRect2DDouble &rect)
{
    if (!wxFinite(x0) || !wxFinite(y0) ||
        !wxFinite(x1) || !wxFinite(y1)) return ClippedOut;

    wxOutCode out0 = wxPlotRect2DDoubleOutCode(x0, y0, rect);
    wxOutCode out1 = wxPlotRect2DDoubleOutCode(x1, y1, rect);

    if ((out0 & out1) != wxInside) return ClippedOut;     // both outside on same side
    if ((out0 | out1) == wxInside) return ClippedNeither; // both inside

    int ret = ClippedNeither;

    if (x0 == x1) // vertical line
    {
        if      (out0 & wxOutTop)    {y0 = rect.GetTop();    ret |= ClippedFirstY;}
        else if (out0 & wxOutBottom) {y0 = rect.GetBottom(); ret |= ClippedFirstY;}
        if      (out1 & wxOutTop)    {y1 = rect.GetTop();    ret |= ClippedSecondY;}
        else if (out1 & wxOutBottom) {y1 = rect.GetBottom(); ret |= ClippedSecondY;}
        return ret;
    }
    if (y0 == y1) // horiz line
    {
        if      (out0 & wxOutLeft)  {x0 = rect.GetLeft();  ret |= ClippedFirstX;}
        else if (out0 & wxOutRight) {x0 = rect.GetRight(); ret |= ClippedFirstX;}
        if      (out1 & wxOutLeft)  {x1 = rect.GetLeft();  ret |= ClippedSecondX;}
        else if (out1 & wxOutRight) {x1 = rect.GetRight(); ret |= ClippedSecondX;}
        return ret;
    }

    double x = x0, y = y0;
    wxOutCode out = out0;
    int points_out = 2;
    bool out0_outside = true;
    if (out0 == wxInside)
    {
        out0_outside = false;
        points_out = 1;
        out = out1;
    }
    else if (out1 == wxInside)
        points_out = 1;

    for (int i=0; i<points_out; i++)
    {
        if (out & wxOutTop)
        {
            y = rect.GetTop();
            x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }
        else if (out & wxOutBottom)
        {
            y = rect.GetBottom();
            x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }
        // check left and right
        if (out & wxOutRight)
        {
            x = rect.GetRight();
            y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }
        else if (out & wxOutLeft)
        {
            x = rect.GetLeft();
            y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }
        // check top and bottom again
        if (out & wxOutTop)
        {
            y = rect.GetTop();
            x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }
        else if (out & wxOutBottom)
        {
            y = rect.GetBottom();
            x = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            out = wxPlotRect2DDoubleOutCode(x, y, rect);
        }

        if (!wxFinite(x) || !wxFinite(y)) return ClippedOut;

        if ((i == 0) && out0_outside)
        {
            x0 = x;
            y0 = y;
            ret |= ClippedFirst;
            out = out1;
        }
        else
        {
            x1 = x;
            y1 = y;
            ret |= ClippedSecond;
            return ret;
        }
    }

    return ret;
}

// ----------------------------------------------------------------------------
// wxWindows spline drawing code see dcbase.cpp - inlined
//
// usage - Create a SplineDrawer, Create(...), DrawSpline(x,y), EndSpline()
// ----------------------------------------------------------------------------

#define SPLINE_STACK_DEPTH 20
#define THRESHOLD           4 // number of pixels between spline points

#define SPLINE_PUSH(x1_, y1_, x2_, y2_, x3_, y3_, x4_, y4_) \
    stack_top->x1 = x1_; stack_top->y1 = y1_; \
    stack_top->x2 = x2_; stack_top->y2 = y2_; \
    stack_top->x3 = x3_; stack_top->y3 = y3_; \
    stack_top->x4 = x4_; stack_top->y4 = y4_; \
    stack_top++; \
    m_stack_count++;

#define SPLINE_POP(x1_, y1_, x2_, y2_, x3_, y3_, x4_, y4_) \
    stack_top--; \
    m_stack_count--; \
    x1_ = stack_top->x1; y1_ = stack_top->y1; \
    x2_ = stack_top->x2; y2_ = stack_top->y2; \
    x3_ = stack_top->x3; y3_ = stack_top->y3; \
    x4_ = stack_top->x4; y4_ = stack_top->y4;

class SplineDrawer
{
public:
    SplineDrawer() : m_dc(NULL) {}
    // the wxRect2DDouble rect is the allowed dc area in pixel coords
    // wxRangeDoubleSelection is the ranges to use selPen, also in pixel coords
    // x1_, y1_, x2_, y2_ are the first 2 points to draw
    void Create(wxDC *dc, const wxPen &curPen, const wxPen &selPen,
                const wxRect2DDouble &rect, wxRangeDoubleSelection *rangeSel,
                double x1_, double y1_, double x2_, double y2_)
    {
        m_dc = dc;
        wxCHECK_RET(dc, wxT("invalid window dc"));

        m_selPen   = selPen;
        m_curPen   = curPen;
        m_rangeSel = rangeSel;

        m_rect = rect;

        m_stack_count = 0;

        m_x1 = x1_;
        m_y1 = y1_;
        m_x2 = x2_;
        m_y2 = y2_;
        m_cx1 = (m_x1  + m_x2) / 2.0;
        m_cy1 = (m_y1  + m_y2) / 2.0;
        m_cx2 = (m_cx1 + m_x2) / 2.0;
        m_cy2 = (m_cy1 + m_y2) / 2.0;

        m_last_x = m_x1;
        m_last_y = m_y1;
    }

    // actually do the drawing here
    void DrawSpline(double x, double y);

    // After the last point call this to finish the drawing
    void EndSpline()
    {
        wxCHECK_RET(m_dc, wxT("invalid window dc"));
        if (ClipLineToRect(m_cx1, m_cy1, m_x2, m_y2, m_rect) != ClippedOut)
            m_dc->DrawLine((int)m_cx1, (int)m_cy1, (int)m_x2, (int)m_y2);
    }

private:

    struct SplineStack
    {
        double x1, y1, x2, y2, x3, y3, x4, y4;
    };

    wxDC *m_dc;
    wxRect2DDouble m_rect;

    SplineStack m_splineStack[SPLINE_STACK_DEPTH];
    int m_stack_count;

    double m_cx1, m_cy1, m_cx2, m_cy2, m_cx3, m_cy3, m_cx4, m_cy4;
    double m_x1, m_y1, m_x2, m_y2;
    double m_last_x, m_last_y;

    wxPen m_selPen, m_curPen;
    wxRangeDoubleSelection *m_rangeSel;
};

void SplineDrawer::DrawSpline(double x, double y)
{
    wxCHECK_RET(m_dc, wxT("invalid window dc"));
    wxPen oldPen = m_dc->GetPen();

    bool is_selected = (oldPen == m_selPen);

    m_x1 = m_x2;
    m_y1 = m_y2;
    m_x2 = x;
    m_y2 = y;
    m_cx4 = (m_x1 + m_x2) / 2.0;
    m_cy4 = (m_y1 + m_y2) / 2.0;
    m_cx3 = (m_x1 + m_cx4) / 2.0;
    m_cy3 = (m_y1 + m_cy4) / 2.0;

    double xmid, ymid;
    double xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4;

    SplineStack *stack_top = m_splineStack;
    m_stack_count = 0;

    SPLINE_PUSH(m_cx1, m_cy1, m_cx2, m_cy2, m_cx3, m_cy3, m_cx4, m_cy4);

    while (m_stack_count > 0)
    {
        SPLINE_POP(xx1, yy1, xx2, yy2, xx3, yy3, xx4, yy4);

        xmid = (xx2 + xx3)/2.0;
        ymid = (yy2 + yy3)/2.0;
        if ((fabs(xx1 - xmid) < THRESHOLD) && (fabs(yy1 - ymid) < THRESHOLD) &&
            (fabs(xmid - xx4) < THRESHOLD) && (fabs(ymid - yy4) < THRESHOLD))
        {
            // FIXME - is this really necessary, better safe than sorry?
            double t1_last_x = m_last_x;
            double t1_last_y = m_last_y;
            double t1_xx1    = xx1;
            double t1_yy1    = yy1;
            if (ClipLineToRect(t1_last_x, t1_last_y, t1_xx1, t1_yy1, m_rect) != ClippedOut)
            {
                if (m_rangeSel && (m_rangeSel->Contains((m_last_x + xx1)/2) != is_selected))
                {
                    is_selected = is_selected ? false : true;
                    if (is_selected)
                        m_dc->SetPen(m_selPen);
                    else
                        m_dc->SetPen(m_curPen);
                }

                m_dc->DrawLine((int)t1_last_x, (int)t1_last_y, (int)t1_xx1, (int)t1_yy1);
            }

            double t2_xx1  = xx1;
            double t2_yy1  = yy1;
            double t2_xmid = xmid;
            double t2_ymid = ymid;
            if (ClipLineToRect(t2_xx1, t2_yy1, t2_xmid, t2_ymid, m_rect) != ClippedOut)
            {
                if (m_rangeSel && (m_rangeSel->Contains((xx1+xmid)/2) != is_selected))
                {
                    is_selected = is_selected ? false : true;
                    if (is_selected)
                        m_dc->SetPen(m_selPen);
                    else
                        m_dc->SetPen(m_curPen);
                }

                m_dc->DrawLine((int)t2_xx1, (int)t2_yy1, (int)t2_xmid, (int)t2_ymid);
            }

            m_last_x = xmid;
            m_last_y = ymid;
        }
        else
        {
            wxCHECK_RET(m_stack_count < SPLINE_STACK_DEPTH - 2, wxT("Spline stack overflow"));
            SPLINE_PUSH(xmid, ymid, (xmid + xx3)/2.0, (ymid + yy3)/2.0,
                        (xx3 + xx4)/2.0, (yy3 + yy4)/2.0, xx4, yy4);
            SPLINE_PUSH(xx1, yy1, (xx1 + xx2)/2.0, (yy1 + yy2)/2.0,
                        (xx2 + xmid)/2.0, (yy2 + ymid)/2.0, xmid, ymid);
        }
    }

    m_cx1 = m_cx4;
    m_cy1 = m_cy4;
    m_cx2 = (m_cx1 + m_x2) / 2.0;
    m_cy2 = (m_cy1 + m_y2) / 2.0;

    m_dc->SetPen(oldPen);
}

//***************************************************************************


//-----------------------------------------------------------------------------
// wxPlotDrawerAxisBase
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerBase, wxObject)

wxPlotDrawerAxisBase::wxPlotDrawerAxisBase(wxPlotCtrl* owner):
    wxPlotDrawerBase(owner)
{
    m_tickFont    = *wxNORMAL_FONT;
    m_labelFont   = *wxSWISS_FONT;
    m_tickColour  = wxColour(0,0,0);
    m_labelColour = wxColour(0,0,0);

    m_tickPen         = wxPen(m_tickColour, wxPENSTYLE_SOLID);
    m_backgroundBrush = wxBrush(wxColour(255,255,255), wxBRUSHSTYLE_SOLID);
}

//-----------------------------------------------------------------------------
// wxPlotDrawerArea
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerArea, wxPlotDrawerBase)

void wxPlotDrawerArea::Draw(wxDC *dc, bool refresh)
{
}

//-----------------------------------------------------------------------------
// wxPlotDrawerAxisBase
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerAxisBase, wxPlotDrawerBase)

//-----------------------------------------------------------------------------
// wxPlotDrawerXAxis
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerXAxis, wxPlotDrawerAxisBase)

void wxPlotDrawerXAxis::Draw(wxDC *dc, bool refresh)
{
    wxCHECK_RET(dc, wxT("Invalid dc"));

    wxRect dcRect(GetDCRect());

    // Draw background
    if (refresh)
    {
        dc->SetBrush(m_backgroundBrush);
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->DrawRectangle(dcRect);
    }

    wxFont tickFont = m_tickFont;
    if (m_font_scale != 1)
        tickFont.SetPointSize(wxMax(2, RINT(tickFont.GetPointSize() * m_font_scale)));

    dc->SetTextForeground(m_tickColour);
    dc->SetFont(tickFont);

    wxString label;

    // center the text in the window
    int x, y;
    dc->GetTextExtent(wxT("5"), &x, &y);
    int y_pos = (GetDCRect().height - y)/2 + 2; // FIXME I want to center this
    // double current = ceil(m_viewRect.GetLeft() / m_xAxisTick_step) * m_xAxisTick_step;
    int i, count = m_tickPositions.GetCount();
    for (i = 0; i < count; i++)
    {
        dc->DrawText(m_tickLabels[i], m_tickPositions[i], y_pos);

//        if (!IsFinite(current, wxT("axis label is not finite")))
//            break;
//        label.Printf(m_xAxisTickFormat.c_str(), current);
//        dc->DrawText(label, m_xAxisTicks[i], y_pos);
//        current += m_xAxisTick_step;
    }

#ifndef NDEBUG
    // Test code for sizing to show the extent of the axes
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(*wxRED_PEN);
    dc->DrawRectangle(wxRect(wxPoint(0,0), dc->GetSize()));
#endif
}

//-----------------------------------------------------------------------------
// wxPlotDrawerYAxis
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerYAxis, wxPlotDrawerAxisBase)

void wxPlotDrawerYAxis::Draw(wxDC *dc, bool refresh)
{
    wxCHECK_RET(dc, wxT("Invalid dc"));

    wxRect dcRect(GetDCRect());

    // Draw background
    if (refresh)
    {
        dc->SetBrush(m_backgroundBrush);
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->DrawRectangle(dcRect);
    }

    wxFont tickFont = m_tickFont;
    if (m_font_scale != 1)
        tickFont.SetPointSize(wxMax(2, RINT(tickFont.GetPointSize() * m_font_scale)));

    dc->SetTextForeground(m_tickColour);
    dc->SetFont(tickFont);

    wxString label;
    // double current = ceil(m_viewRect.GetTop() / m_yAxisTick_step) * m_yAxisTick_step;
    int i, count = m_tickLabels.GetCount(), lcnt = m_tickPositions.GetCount();
    for (i = 0; i < count && i < lcnt; i++)
    {
        dc->DrawText(m_tickLabels[i], 2,
                     m_tickPositions[i]);

//        if (!IsFinite(current, wxT("axis label is not finite")))
//            break;
//        label.Printf(m_yAxisTickFormat.c_str(), current);
//        dc->DrawText(label, 2, m_yAxisTicks[i]);
//        current += m_yAxisTick_step;
    }

#ifndef NDEBUG
    // Test code for sizing to show the extent of the axes
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(*wxRED_PEN);
    dc->DrawRectangle(wxRect(wxPoint(0,0), dc->GetSize()));
#endif
}

//-----------------------------------------------------------------------------
// wxPlotDrawerKey
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerKey, wxPlotDrawerBase)

wxPlotDrawerKey::wxPlotDrawerKey(wxPlotCtrl* owner)
                :wxPlotDrawerBase(owner)
{
    m_font = *wxNORMAL_FONT;
    m_fontColour = wxColour(0, 0, 0);
    m_keyPosition = wxPoint(100, 100);
    m_border = 5;
    m_key_inside = true;
    m_key_line_width = 20;
    m_key_line_margin = 5;
}

void wxPlotDrawerKey::Draw(wxDC *dc, const wxString& keyString_)
{
    wxCHECK_RET(dc && m_owner, wxT("Invalid dc"));

    if (keyString_.IsEmpty())
        return;

    wxString keyString = keyString_;

/*
    // GTK - kills X if font size is too small
    double x_scale = 1, y_scale = 1;
    dc->GetUserScale(&x_scale, &y_scale);

    wxFont font = m_owner->GetKeyFont();
    if (0 && x_scale != 1)
    {
        font.SetPointSize(wxMax(int(font.GetPointSize()/x_scale), 4));
        if (!font.Ok())
            font = GetKeyFont();
    }
*/

    wxFont keyFont = m_font;
    if (m_font_scale != 1)
        keyFont.SetPointSize(wxMax(2, RINT(keyFont.GetPointSize() * m_font_scale)));

    int key_line_width  = RINT(m_key_line_width  * m_pen_scale);
    int key_line_margin = RINT(m_key_line_margin * m_pen_scale);

    dc->SetFont(keyFont);
    dc->SetTextForeground(m_fontColour);

    wxRect keyRect;
    int heightLine = 0;

    dc->GetMultiLineTextExtent(keyString, &keyRect.width, &keyRect.height, &heightLine);

    wxRect dcRect(GetDCRect());
    wxSize areaSize = dcRect.GetSize();

    keyRect.x = 30 + int((m_keyPosition.x*.01)*areaSize.x);
    keyRect.y = areaSize.y - int((m_keyPosition.y*.01)*areaSize.y);

    if (m_key_inside)
    {
        keyRect.x = wxMax(30, keyRect.x);
        keyRect.x = wxMin(areaSize.x - keyRect.width - m_border, keyRect.GetRight());

        keyRect.y = wxMax(m_border, keyRect.y);
        keyRect.y = wxMin(areaSize.y - keyRect.height - m_border, keyRect.y);
    }

    int h = keyRect.y;
    int i = 0;

    while (!keyString.IsEmpty())
    {
        wxString subkey = keyString.BeforeFirst(wxT('\n')).Strip(wxString::both);
        keyString = keyString.AfterFirst(wxT('\n'));
        if (subkey.IsEmpty()) break;

        if (m_owner && m_owner->GetCurve(i))
        {
            wxPen keyPen = m_owner->GetCurve(i)->GetPen(wxPLOTPEN_NORMAL);
            if (m_pen_scale != 1)
                keyPen.SetWidth(int(keyPen.GetWidth() * m_pen_scale));

            if(keyPen.GetWidth() < 3) keyPen.SetWidth(3);
            dc->SetPen(keyPen);
            dc->DrawLine(keyRect.x - (key_line_width + key_line_margin), h + heightLine/2,
                        keyRect.x - key_line_margin, h + heightLine/2);
        }

        dc->DrawText(subkey, keyRect.x, h);

        h += heightLine;
        i++;
    }

    dc->SetPen(wxNullPen);
    dc->SetFont(wxNullFont);
}

//-----------------------------------------------------------------------------
// wxPlotDrawerDataCurve
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerDataCurve, wxPlotDrawerBase)

void wxPlotDrawerDataCurve::Draw(wxDC *dc, wxPlotData* curve, int curve_index)
{
    wxCHECK_RET(dc && m_owner && curve && curve->Ok(), wxT("invalid curve"));
    INITIALIZE_FAST_GRAPHICS

    wxRect dcRect(GetDCRect());

    wxRect2DDouble viewRect(GetPlotViewRect()); //m_viewRect);
    wxRect2DDouble subViewRect(m_owner->GetPlotRectFromClientRect(dcRect));
    wxRect2DDouble curveRect(curve->GetBoundingRect());
    if (!wxPlotRect2DDoubleIntersects(curveRect, subViewRect)) return;

/*  // FIXME - drawing symbol bitmaps in MSW is very slow
    wxBitmap bitmap;
    if (curve == GetActiveCurve())
        bitmap = curve->GetSymbol(wxPLOTPEN_ACTIVE);
    else
        bitmap = curve->GetSymbol(wxPLOTPEN_NORMAL);

    if (!bitmap.Ok())
    {
        if (curve == GetActiveCurve())
            bitmap = wxPlotSymbolCurrent;
        else
            bitmap = wxPlotSymbolNormal;
    }

    int bitmapHalfWidth = bitmap.GetWidth()/2;
    int bitmapHalfHeight = bitmap.GetHeight()/2;
*/

    // find the starting and ending indexes into the data curve
    int n, n_start = 0, n_end = curve->GetCount();

    // set the pens to draw with
    wxPen currentPen = (curve_index == m_owner->GetActiveIndex()) ? curve->GetPen(wxPLOTPEN_ACTIVE)
                                                                  : curve->GetPen(wxPLOTPEN_NORMAL);
    wxPen selectedPen = curve->GetPen(wxPLOTPEN_SELECTED);
    if (m_pen_scale != 1)
    {
        currentPen.SetWidth(int(currentPen.GetWidth() * m_pen_scale));
        selectedPen.SetWidth(int(selectedPen.GetWidth() * m_pen_scale));
    }

    dc->SetPen(currentPen);

    // handle the selected ranges and initialize the starting range
    const wxArrayRangeInt &ranges = m_owner->GetDataCurveSelection(curve_index)->GetRangeArray();
    int n_range = 0, range_count = ranges.GetCount();
    int min_sel = -1, max_sel = -1;
    for (n_range=0; n_range<range_count; n_range++)
    {
        const wxRangeInt& range = ranges[n_range];
        if ((range.m_max >= n_start) || (range.m_min >= n_start))
        {
            min_sel = range.m_min;
            max_sel = range.m_max;
            if (range.Contains(n_start))
                dc->SetPen(selectedPen);

            break;
        }
    }

    // data variables
    const double *x_data = &curve->GetXData()[n_start];
    const double *y_data = &curve->GetYData()[n_start];

    int i0, j0, i1, j1;        // curve coords in pixels
    double x0, y0, x1, y1;     // original curve coords
    double xx0, yy0, xx1, yy1; // clipped curve coords

    x0 = *x_data;
    y0 = *y_data;

    int clipped = ClippedNeither;

    bool draw_lines   = m_owner->GetDrawLines();
    bool draw_symbols = m_owner->GetDrawSymbols();
    bool draw_spline  = m_owner->GetDrawSpline();

    SplineDrawer sd;
    wxRangeDoubleSelection dblRangeSel;

    if (draw_spline)
    {
        wxRangeDouble viewRange(viewRect.m_x, viewRect.GetRight());

        for (int r = n_range; r < range_count; r++)
        {
            wxRangeDouble plotRange(curve->GetXValue(ranges[r].m_min),
                                    curve->GetXValue(ranges[r].m_max));

            if (viewRange.Intersects(plotRange))
            {
                double min_x = m_owner->GetClientCoordFromPlotX(plotRange.m_min);
                double max_x = m_owner->GetClientCoordFromPlotX(plotRange.m_max);
                dblRangeSel.SelectRange(wxRangeDouble(min_x, max_x));
            }
            else
                break;
        }

        // spline starts 2 points back for smoothness
        int s_start = n_start > 1 ? -2 : n_start > 0 ? -1 : 0;
        sd.Create(dc, currentPen, selectedPen,
                  wxRect2DDouble(dcRect.x, dcRect.y, dcRect.width, dcRect.height),
                  &dblRangeSel,
                  m_owner->GetClientCoordFromPlotX(x_data[s_start]),
                  m_owner->GetClientCoordFromPlotY(y_data[s_start]),
                  m_owner->GetClientCoordFromPlotX(x_data[s_start+1]),
                  m_owner->GetClientCoordFromPlotY(y_data[s_start+1]));
    }

    for (n = n_start; n < n_end; n++)
    {
        x1 = *x_data++;
        y1 = *y_data++;

        if (draw_spline)
            sd.DrawSpline(m_owner->GetClientCoordFromPlotX(x1),
                          m_owner->GetClientCoordFromPlotY(y1));

        xx0 = x0; yy0 = y0; xx1 = x1; yy1 = y1;
        clipped = ClipLineToRect(xx0, yy0, xx1, yy1, viewRect);
        if (clipped != ClippedOut)
        {
            i0 = m_owner->GetClientCoordFromPlotX(xx0);
            j0 = m_owner->GetClientCoordFromPlotY(yy0);
            i1 = m_owner->GetClientCoordFromPlotX(xx1);
            j1 = m_owner->GetClientCoordFromPlotY(yy1);

            if (draw_lines && ((i0 != i1) || (j0 != j1)))
            {
                wxPLOTCTRL_DRAW_LINE(dc, window, pen, i0, j0, i1, j1);
            }

            if (n == min_sel)
                dc->SetPen(selectedPen);

            if (draw_symbols && !((clipped & ClippedSecond) != 0) &&
                ((i0 != i1) || (j0 != j1) || (n == min_sel) || (n == n_start)))
            {
                //dc->DrawBitmap(bitmap, i1 - bitmapHalfWidth, j1 - bitmapHalfHeight, true);
                wxPLOTCTRL_DRAW_ELLIPSE(dc, window, pen, i1, j1, 2, 2);
            }
        }
        else if (n == min_sel)
        {
            dc->SetPen(selectedPen);
        }

        if (n == max_sel)
        {
            dc->SetPen(currentPen);
            if (n_range < range_count - 1)
            {
                n_range++;
                min_sel = ranges[n_range].m_min;
                max_sel = ranges[n_range].m_max;
            }
        }

        x0 = x1;
        y0 = y1;
    }

    if (draw_spline)
    {
        // want an extra point at the end to smooth it out
        if (n_end < (int)curve->GetCount() - 1)
            sd.DrawSpline(m_owner->GetClientCoordFromPlotX(*x_data),
                          m_owner->GetClientCoordFromPlotY(*y_data));

        sd.EndSpline();
    }

    dc->SetPen(wxNullPen);
}

//-----------------------------------------------------------------------------
// wxPlotDrawerMarkers
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotDrawerMarker, wxPlotDrawerBase)

void wxPlotDrawerMarker::Draw(wxDC *dc, const wxPlotMarker& marker)
{
    // drawing multiple markers is faster, so just drawing a single one takes a hit
    wxArrayPlotMarker markers;
    markers.Add(marker);
    Draw(dc, markers);
}

void wxPlotDrawerMarker::Draw(wxDC *dc, const wxArrayPlotMarker& markers)
{
    wxCHECK_RET(dc && m_owner, wxT("dc or owner"));
    INITIALIZE_FAST_GRAPHICS

    wxRect dcRect(GetDCRect());
    wxRect2DDouble subViewRect = m_owner->GetPlotRectFromClientRect(dcRect);

    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int n, count = markers.GetCount();
    for (n = 0; n < count; n++)
    {
        const wxPlotMarker &marker = markers[n];
        wxCHECK_RET(marker.Ok(), wxT("Invalid marker"));
        wxRect2DDouble r = marker.GetPlotRect();
        x0 = r.m_x;
        y0 = r.m_y;
        x1 = r.GetRight();
        y1 = r.GetBottom();

        if (marker.GetPen().Ok())
            dc->SetPen(marker.GetPen());
        if (marker.GetBrush().Ok())
            dc->SetBrush(marker.GetBrush());

        // determine what to draw
        int marker_type = marker.GetMarkerType();
        wxSize size     = marker.GetSize();

        if (marker_type == wxPLOTMARKER_BITMAP)
        {
            wxBitmap bmp(marker.GetBitmap());
            int w = bmp.GetWidth(), h = bmp.GetHeight();
            // FIXME - add scaling and shifting later - maybe
            int i0 = m_owner->GetClientCoordFromPlotX(x0);
            int j0 = m_owner->GetClientCoordFromPlotY(y0);
            dc->DrawBitmap(bmp, RINT(i0 - w/2.0), RINT(j0 - h/2.0), true);
        }
        else if (marker_type == wxPLOTMARKER_LINE)
        {
            if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
            {
                int i0 = m_owner->GetClientCoordFromPlotX(x0);
                int j0 = m_owner->GetClientCoordFromPlotY(y0);
                int i1 = m_owner->GetClientCoordFromPlotX(x1);
                int j1 = m_owner->GetClientCoordFromPlotY(y1);
                wxPLOTCTRL_DRAW_LINE(dc, window, pen, i0, j0, i1, j1);
            }
        }
        else if (marker_type == wxPLOTMARKER_ELLIPSE)
        {
            // fixed pixel size
            if ((size.x > 0) && (size.y > 0))
            {
                if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
                {
                    int i0 = m_owner->GetClientCoordFromPlotX(x0);
                    int j0 = m_owner->GetClientCoordFromPlotY(y0);
                    wxPLOTCTRL_DRAW_ELLIPSE(dc, window, pen, i0, j0, size.x, size.y);
                }
            }
/*
            else if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
            {
                int i0 = m_owner->GetClientCoordFromPlotX(x0);
                int j0 = m_owner->GetClientCoordFromPlotY(y0);
                int i1 = m_owner->GetClientCoordFromPlotX(x1);
                int j1 = m_owner->GetClientCoordFromPlotY(y1);
                wxPLOTCTRL_DRAW_ELLIPSE(dc, window, pen, i0, j0, i1, j1);
            }
*/
        }
        else // figure out the rest
        {
            // we may ignore type if rect is in certain states

            bool is_horiz = r.m_width  < 0;
            bool is_vert  = r.m_height < 0;

            bool cross = is_horiz && is_vert;

            if (is_horiz)
            {
                x0 = subViewRect.m_x - subViewRect.m_width; // push outside win
                x1 = subViewRect.GetRight() + subViewRect.m_width;
            }
            if (is_vert)
            {
                y0 = subViewRect.m_y - subViewRect.m_height;
                y1 = subViewRect.GetBottom() + subViewRect.m_height;
            }

            if ((marker_type == wxPLOTMARKER_POINT) || ((x0 == x1) && (y0 == y1)))
            {
                if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
                {
                    int i0 = m_owner->GetClientCoordFromPlotX(x0);
                    int j0 = m_owner->GetClientCoordFromPlotY(y0);
                    dc->DrawPoint(i0, j0);
                }
            }
            else if ((marker_type == wxPLOTMARKER_VERT_LINE) || ((x0 == x1) && (y0 != y1)))
            {
                if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
                {
                    int i0 = m_owner->GetClientCoordFromPlotX(x0);
                    int j0 = m_owner->GetClientCoordFromPlotY(y0);
                    int j1 = m_owner->GetClientCoordFromPlotY(y1);
                    wxPLOTCTRL_DRAW_LINE(dc, window, pen, i0, j0, i0, j1);
                }
            }
            else if ((marker_type == wxPLOTMARKER_HORIZ_LINE) || ((y0 == y1) && (x0 != x1)))
            {
                if (ClipLineToRect(x0, y0, x1, y1, subViewRect) != ClippedOut)
                {
                    int i0 = m_owner->GetClientCoordFromPlotX(x0);
                    int i1 = m_owner->GetClientCoordFromPlotX(x1);
                    int j0 = m_owner->GetClientCoordFromPlotY(y0);
                    wxPLOTCTRL_DRAW_LINE(dc, window, pen, i0, j0, i1, j0);
                }
            }
            else if ((marker_type == wxPLOTMARKER_CROSS) || cross)
            {


            }
            else                                // rectangle
            {
                wxRect2DDouble clippedRect(x0, y0, x1 - x0, y1 - y0);
                clippedRect.Intersect(subViewRect);
                int pen_width = dc->GetPen().GetWidth() + 2;

                int i0 = m_owner->GetClientCoordFromPlotX(clippedRect.m_x);
                int i1 = m_owner->GetClientCoordFromPlotX(clippedRect.GetRight());
                int j0 = m_owner->GetClientCoordFromPlotY(clippedRect.m_y);
                int j1 = m_owner->GetClientCoordFromPlotY(clippedRect.GetBottom());
                if (r.m_x < subViewRect.m_x)  i0 -= pen_width;
                if (r.m_y < subViewRect.m_y)  j0 -= pen_width;
                if (r.GetRight()  > subViewRect.GetRight())  i1 += pen_width;
                if (r.GetBottom() > subViewRect.GetBottom()) j1 += pen_width;

                dc->SetClippingRegion(dcRect);
                dc->DrawRectangle(i0, j0, i1 - i0 + 1, j1 - j0 + 1);
                dc->DestroyClippingRegion();
            }
        }
    }
}

//=============================================================================
// wxRangeDouble
//=============================================================================

bool wxRangeDouble::Combine(double i)
{
    if      (i < m_min) {m_min = i; return true;}
    else if (i > m_max) {m_max = i; return true;}
    return false;
}

bool wxRangeDouble::Combine(const wxRangeDouble &r, bool only_if_touching)
{
    if (only_if_touching)
    {
        if (Contains(r))
        {
            *this+=r;
            return true;
        }
    }
    else
    {
        bool added = false;
        if (r.m_min < m_min) {m_min = r.m_min; added = true;}
        if (r.m_max > m_max) {m_max = r.m_max; added = true;}
        return added;
    }
    return false;
}

bool wxRangeDouble::Delete(const wxRangeDouble &r, wxRangeDouble *right)
{
    if (!Contains(r))
        return false;

    if (right) *right = wxEmptyRangeDouble;

    if (r.m_min <= m_min)
    {
        if (r.m_max >= m_max)
        {
            *this = wxEmptyRangeDouble;
            return true;
        }

        m_min = r.m_max;
        return true;
    }

    if (r.m_max >= m_max)
    {
        m_max = r.m_min;
        return true;
    }

    if (right)
        *right = wxRangeDouble(r.m_max, m_max);

    m_max = r.m_min;
    return true;
}

//=============================================================================
// wxRangeDoubleSelection
//=============================================================================
const wxRangeDouble& wxRangeDoubleSelection::GetRange(int index) const
{
    wxCHECK_MSG((index>=0) && (index<int(m_ranges.GetCount())), wxEmptyRangeDouble, wxT("Invalid index"));
    return m_ranges[index];
}

wxRangeDouble wxRangeDoubleSelection::GetBoundingRange() const
{
    if (int(m_ranges.GetCount()) < 1) return wxEmptyRangeDouble;
    return wxRangeDouble(m_ranges[0].m_min, m_ranges[m_ranges.GetCount()-1].m_max);
}

int wxRangeDoubleSelection::Index(wxDouble i) const
{
    int count = m_ranges.GetCount();
    if (count < 1) return wxNOT_FOUND;

    if (i < m_ranges[0].m_min) return wxNOT_FOUND;
    if (i > m_ranges[count-1].m_max) return wxNOT_FOUND;

    // Binary search
    int res, tmp, lo = 0, hi = count;

    while (lo < hi)
    {
        tmp = (lo + hi)/2;
        res = m_ranges[tmp].Position(i);

        if (res == 0)
            return tmp;
        else if (res < 0)
            hi = tmp;
        else //if (res > 0)
            lo = tmp + 1;
    }

    return wxNOT_FOUND;

}

int wxRangeDoubleSelection::Index(const wxRangeDouble &r) const
{
    int i, count = m_ranges.GetCount();
    for (i=0; i<count; i++) if (m_ranges[i].Contains(r)) return i;
    return wxNOT_FOUND;
}

int wxRangeDoubleSelection::NearestIndex(wxDouble i) const
{
    int count = m_ranges.GetCount();
    if (count < 1) return -1;

    if (i < m_ranges[0].m_min) return -1;
    if (i > m_ranges[count-1].m_max) return count;

    // Binary search
    int res, tmp, lo = 0, hi = count;

    while (lo < hi)
    {
        tmp = (lo + hi)/2;
        res = m_ranges[tmp].Position(i);

        if (res == 0)
            return tmp;
        else if ((i >= m_ranges[tmp].m_max) && (i < m_ranges[wxMin(tmp+1, count-1)].m_min))
            return tmp;
        else if (res < 0)
            hi = tmp;
        else //if (res > 0)
            lo = tmp + 1;
    }

    // oops shouldn't get here
    wxCHECK_MSG(0, -1, wxT("Error calculating NearestIndex in wxRangeDoubleSelection"));
}

bool wxRangeDoubleSelection::SelectRange(const wxRangeDouble &range)
{
    wxCHECK_MSG(!range.IsEmpty(), false, wxT("Invalid Selection Range"));

    // Try to find a range that includes this one and combine it, else insert it, else append it
    bool done = false;
    int i, count = m_ranges.GetCount();
    int nearest = count > 0 ? NearestIndex(range.m_min) : -1;

    if (nearest < 0)
    {
        if (!((count > 0) && m_ranges[0].Combine(range, true)))
            m_ranges.Insert(range, 0);
        return true;
    }
    else if (nearest == count)
    {
        if (!((count > 0) && m_ranges[count-1].Combine(range, true)))
            m_ranges.Add(range);
        return true;
    }
    else
    {
        if (m_ranges[nearest].Contains(range))
            return false;

        for (i=nearest; i<count; i++)
        {
            if (m_ranges[i].Combine(range, true))
            {
                done = true;
                break;
            }
            else if (range.m_max < m_ranges[i].m_min)
            {
                m_ranges.Insert(range, i);
                return true;
            }
        }
        for (i=wxMax(nearest-1, 1); i<int(m_ranges.GetCount()); i++)
        {
            if (range.m_max+1 < m_ranges[i-1].m_min)
                break;
            else if (m_ranges[i-1].Combine(m_ranges[i], true))
            {
                m_ranges.RemoveAt(i);
                i--;
            }
        }
    }

#ifdef CHECK_RANGES
    printf("Selecting ranges %g %g count %d\n", range.m_min, range.m_max, m_ranges.GetCount());

    for (i=1; i<int(m_ranges.GetCount()); i++)
    {
        if (m_ranges[i-1].Contains(m_ranges[i]))
            printf("Error in Selecting ranges %g %g, %g %g count %d\n", m_ranges[i-1].m_min, m_ranges[i-1].m_max, m_ranges[i].m_min, m_ranges[i].m_max, m_ranges.GetCount());
        //if (m_ranges[i-1].Touches(m_ranges[i]))
        //    printf("Could have minimzed ranges %g %g, %g %g count %d\n", m_ranges[i-1].m_min, m_ranges[i-1].m_max, m_ranges[i].m_min, m_ranges[i].m_max, m_ranges.GetCount());
    }
    fflush(stdout);
#endif // CHECK_RANGES

    return done;
}

bool wxRangeDoubleSelection::DeselectRange(const wxRangeDouble &range)
{
    wxCHECK_MSG(!range.IsEmpty(), false, wxT("Invalid Selection Range"));

    bool done = false;
    int i, count = m_ranges.GetCount();
    int nearest = count > 0 ? NearestIndex(range.m_min) : -1;

    if ((nearest < 0) || (nearest == count))
        return false;

    wxRangeDouble r;
    for (i=nearest; i<int(m_ranges.GetCount()); i++)
    {
        if (range.m_max < m_ranges[i].m_min)
            break;
        else if (m_ranges[i].Delete(range, &r))
        {
            if (m_ranges[i].IsEmpty())
            {
                m_ranges.RemoveAt(i);
                i = (i > 0) ? i-1 : -1;
            }
            else if (!r.IsEmpty())
                m_ranges.Insert(r, i+1);

            done = true;
        }
    }

    return done;
}

bool wxRangeDoubleSelection::BoundRanges(const wxRangeDouble& range)
{
    wxCHECK_MSG(!range.IsEmpty(), false, wxT("Invalid Bounding Range"));
    int i, count = m_ranges.GetCount();
    bool done = false;

    for (i = 0; i < count; i++)
    {
        if (m_ranges[i].m_min >= range.m_min)
            break;

        if (m_ranges[i].m_max < range.m_min) // range is out of bounds
        {
            done = true;
            m_ranges.RemoveAt(i);
            count--;
            i--;
        }
        else
        {
            done = true;
            m_ranges[i].m_min = range.m_min;
            break;
        }
    }

    for (i = m_ranges.GetCount() - 1; i >= 0; i--)
    {
        if (m_ranges[i].m_max <= range.m_max)
            break;

        if (m_ranges[i].m_min > range.m_max) // range is out of bounds
        {
            done = true;
            m_ranges.RemoveAt(i);
        }
        else
        {
            done = true;
            m_ranges[i].m_max = range.m_max;
            break;
        }
    }

    return done;
}

