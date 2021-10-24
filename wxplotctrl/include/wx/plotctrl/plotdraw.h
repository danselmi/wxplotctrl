/////////////////////////////////////////////////////////////////////////////
// Name:        plotdraw.h
// Purpose:     wxPlotDrawer and friends
// Author:      John Labenski
// Modified by:
// Created:     6/5/2002
// Copyright:   (c) John Labenski
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PLOTDRAW_H_
#define _WX_PLOTDRAW_H_

#include "wx/plotctrl/plotmark.h"
#include "wx/plotctrl/range.h"

class wxDC;

class wxRangeIntSelection;
class wxRangeDoubleSelection;
class wxArrayRangeIntSelection;

class wxPlotCtrl;
class wxPlotData;
class wxPlotMarker;

//-----------------------------------------------------------------------------
// wxPlotDrawerBase
//-----------------------------------------------------------------------------

class wxPlotDrawerBase: public wxObject
{
public:
    wxPlotDrawerBase(wxPlotCtrl *host);

    virtual void Draw(wxDC *dc, bool refresh) = 0;

    // Get/Get the rect in the DC to draw on
    void SetDCRect(const wxRect &rect);
    const wxRect &GetDCRect() const;

    // Get/Set the rect of the visible area in the plot window
    void SetPlotViewRect(const wxRect2DDouble &rect);
    const wxRect2DDouble &GetPlotViewRect() const;

    // Get/Set the scaling for drawing, fonts, pens, etc are scaled
    void SetPenScale(double scale);
    double GetPenScale() const;
    void SetFontScale(double scale);
    double GetFontScale() const;

protected:
    wxPlotCtrl    *host_;
    wxRect         dcRect_;
    wxRect2DDouble plotViewRect_;
    double         penScale_;    // width scaling factor for pens
    double         fontScale_;   // scaling factor for font sizes

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerBase);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerArea
//-----------------------------------------------------------------------------

class wxPlotDrawerArea: public wxPlotDrawerBase
{
public:
    wxPlotDrawerArea(wxPlotCtrl *host);

    virtual void Draw(wxDC *dc, bool refresh);

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerArea);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerAxisBase
//-----------------------------------------------------------------------------

class wxPlotDrawerAxisBase: public wxPlotDrawerBase
{
public:
    wxPlotDrawerAxisBase(wxPlotCtrl *host, wxArrayInt &tickPositions, wxArrayString &tickLabels);

    virtual void Draw(wxDC *dc, bool refresh) = 0;

    void SetTickFont(const wxFont &font);
    void SetLabelFont(const wxFont &font);

    void SetTickColour(const wxColour &colour);
    void SetLabelColour(const wxColour &colour);

    void SetBackgroundBrush(const wxBrush &brush);

    // implementation
    wxArrayInt    &tickPositions_;
    wxArrayString &tickLabels_;

    wxFont   tickFont_;
    wxFont   labelFont_;
    wxColour tickColour_;
    wxColour labelColour_;

    wxBrush  backgroundBrush_;

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerAxisBase);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerXAxis
//-----------------------------------------------------------------------------

class wxPlotDrawerXAxis: public wxPlotDrawerAxisBase
{
public:
    wxPlotDrawerXAxis(wxPlotCtrl *host, wxArrayInt &tickPositions, wxArrayString &tickLabels);
    virtual void Draw(wxDC *dc, bool refresh);
private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerXAxis);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerYAxis
//-----------------------------------------------------------------------------

class wxPlotDrawerYAxis: public wxPlotDrawerAxisBase
{
public:
    wxPlotDrawerYAxis(wxPlotCtrl *host, wxArrayInt &tickPositions, wxArrayString &tickLabels);
    virtual void Draw(wxDC *dc, bool refresh);
private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerYAxis);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerKey
//-----------------------------------------------------------------------------
class wxPlotDrawerKey: public wxPlotDrawerBase
{
public:
    wxPlotDrawerKey(wxPlotCtrl *host);

    virtual void Draw(wxDC *dc, bool refresh);
    virtual void Draw(wxDC *dc, const wxString &keyString);

    void SetFont(const wxFont &font);
    void SetFontColour(const wxColour &colour);

    void SetKeyPosition(const wxPoint &pos);

    // implementation
    wxFont font_;
    wxColour fontColour_;
    wxPoint keyPosition_;
    bool keyInside_;
    int border_;
    int keyLineWidth_;  // length of line to draw for curve
    int keyLineMargin_; // margin between line and key text

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerKey);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerDataCurve
//-----------------------------------------------------------------------------

class wxPlotDrawerDataCurve: public wxPlotDrawerBase
{
public:
    wxPlotDrawerDataCurve(wxPlotCtrl *host);
    virtual void Draw(wxDC *dc, bool refresh);
    virtual void Draw(wxDC *dc, wxPlotData *plotData, int curveIndex);

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerDataCurve);
};

//-----------------------------------------------------------------------------
// wxPlotDrawerMarkers
//-----------------------------------------------------------------------------

class wxPlotDrawerMarker: public wxPlotDrawerBase
{
public:
    wxPlotDrawerMarker(wxPlotCtrl *host);

    virtual void Draw(wxDC *dc, bool refresh);
    virtual void Draw(wxDC *dc, const wxArrayPlotMarker &markers);
    virtual void Draw(wxDC *dc, const wxPlotMarker &marker);

private:
    DECLARE_ABSTRACT_CLASS(wxPlotDrawerMarker);
};

#endif
