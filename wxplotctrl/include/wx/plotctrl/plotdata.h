/////////////////////////////////////////////////////////////////////////////
// Name:        plotdata.h
// Purpose:     wxPlotData container class for wxPlotCtrl
// Author:      John Labenski
// Modified by:
// Created:     12/1/2000
// Copyright:   (c) John Labenski
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PLOTDATA_H_
#define _WX_PLOTDATA_H_

#include "wx/geometry.h"
#include "wx/clntdata.h"

// Find y at point x along the line from (x0,y0)-(x1,y1), x0 must != x1
extern double LinearInterpolateY(double x0, double y0,
                                  double x1, double y1,
                                  double x);
// Find x at point y along the line from (x0,y0)-(x1,y1), y0 must != y1
extern double LinearInterpolateX(double x0, double y0,
                                  double x1, double y1,
                                  double y);

extern const wxRect2DDouble wxNullPlotBounds;
WX_DECLARE_OBJARRAY_WITH_DECL(wxPen, wxArrayPen, class);

extern wxBitmap wxPlotSymbolNormal;
extern wxBitmap wxPlotSymbolActive;
extern wxBitmap wxPlotSymbolSelected;

class wxPlotData: public wxObject
{
public:
    wxPlotData();
    wxPlotData(const wxPlotData& plotData);

    wxPlotData(int points, bool zero = true);
    wxPlotData(double *xs, double *ys, int points, bool staticData = false);
    virtual ~wxPlotData() = default;

    // Ref the source plotdata
    bool Create(const wxPlotData& plotData);
    // Allocate memory for given number of points, if zero then init to zeroes
    //   don't use uninitialized data, trying to plot it will cause problems
    bool Create(int points, bool zero = true);
    // Assign the malloc(ed) data sets to this plotdata,
    //   if !static_data they'll be free(ed) on destruction
    bool Create(double *xs, double *ys, int points, bool staticData = false);

    enum class PenColorType
    {
        NORMAL,
        ACTIVE,
        SELECTED,
        MAXTYPE
    };

    enum class Symbol
    {
        ELLIPSE,
        RECTANGLE,
        CROSS,
        PLUS,
        MAXTYPE
    };

    // Make true (not refed) copy of this,
    //   if copy_all = true then copy header, filename, pens, etc
    bool Copy(const wxPlotData &source, bool copyAll = false);
    // Only copy the header, filename, pens, etc... from the source
    bool CopyExtra(const wxPlotData &source);

    // Unref the data
    void Destroy();

    // Is there data, has it been properly constructed?
    virtual bool Ok() const;

    // Get the number of points in the data set
    int GetCount() const;

    // calc BoundingRect of the data and determine if X is ordered
    // ALWAYS call CalcBoundingRect after externally modifying the data,
    // especially if reording X quantities and using the wxPlotCtrl
    virtual void CalcBoundingRect();

    //-------------------------------------------------------------------------
    // Get/Set data values
    //-------------------------------------------------------------------------

    // Get a pointer to the data (call CalcBoundingRect afterwards if changing values)
    double *GetXData() const;
    double *GetYData() const;

    // Get the point's value at this data index
    double GetXValue(int index) const;
    double GetYValue(int index) const;
    wxPoint2DDouble GetPoint(int index) const;
    // Interpolate if necessary to get the y value at this point,
    //   doesn't fail just returns ends if out of bounds
    double GetY(double x) const;

    enum class IndexType
    {
        round,
        floor,
        ceil
    };

    // find the first occurance of an index whose value is closest (index_round),
    //   or the next lower (index_floor), or next higher (index_ceil), to the given value
    //   always returns a valid index
    int GetIndexFromX(double x, IndexType type = IndexType::round) const;
    int GetIndexFromY(double y, IndexType type = IndexType::round) const;
    // find the first occurance of an index whose value is closest to x,y
    //    if x_range != 0 then limit search between +- x_range (useful for x-ordered data)
    int GetIndexFromXY(double x, double y, double x_range=0) const;

    //-------------------------------------------------------------------------
    // Get/Set Symbols to use for plotting - CreateSymbol is untested
    //   note: in MSW drawing bitmaps is sloooow! <-- so I haven't bothered finishing
    //-------------------------------------------------------------------------

    // Get the symbol used for marking data points
    wxBitmap GetSymbol(PenColorType type = PenColorType::NORMAL) const;
    // Set the symbol to some arbitray bitmap, make size odd so it can be centered
    void SetSymbol(const wxBitmap &bitmap, PenColorType type = PenColorType::NORMAL);
    // Set the symbol from of the available types, using default colours if pen and brush are NULL
    void SetSymbol(Symbol symbol, PenColorType type = PenColorType::NORMAL,
                    int width = 5, int height = 5,
                    const wxPen *pen = NULL, const wxBrush *brush = NULL);
    // Get a copy of the symbol thats created for SetSymbol
    wxBitmap CreateSymbol(Symbol symbol, PenColorType type = PenColorType::NORMAL,
                           int width = 5, int height = 5,
                           const wxPen *pen = NULL, const wxBrush *brush = NULL);

    // Bounding rect used for drawing the curve and...
    //   if the width or height <= 0 then there's no bounds (or unknown)
    //   wxPlotData : calculated from CalcBoundingRect and is well defined
    //                DON'T call SetBoundingRect unless you know what you're doing
    virtual wxRect2DDouble GetBoundingRect() const;
    virtual void SetBoundingRect(const wxRect2DDouble &rect);

    // Get/Set Pens for Normal, Active, Selected drawing
    //  if these are not set it resorts to the defaults
    wxPen GetPen(PenColorType type) const;
    void SetPen(PenColorType type, const wxPen &pen);

    // Get/Set Default Pens for Normal, Active, Selected drawing for all curves
    //   these are the pens that are used when a wxPlotCurve/Function/Data is created
    //   default: Normal(0,0,0,1,wxSOLID), Active(0,0,255,1,wxSOLID), Selected(255,0,0,1,wxSOLID)
    static wxPen GetDefaultPen(PenColorType type);
    static void SetDefaultPen(PenColorType type, const wxPen &pen);

    //-------------------------------------------------------------------------
    // Get/Set the ClientData in the ref data - see wxClientDataContainer
    //  You can store any extra info here.
    //-------------------------------------------------------------------------

    void SetClientObject(wxClientData *data);
    wxClientData *GetClientObject() const;

    void SetClientData(void *data);
    void *GetClientData() const;

    //-----------------------------------------------------------------------
    // Operators

    bool operator == (const wxPlotData& plotData) const
        {return m_refData == plotData.m_refData;}
    bool operator != (const wxPlotData& plotData) const
        {return m_refData != plotData.m_refData;}

    wxPlotData& operator = (const wxPlotData& plotData)
    {
        if ((*this) != plotData)
            Ref(plotData);
        return *this;
    }

private:
    // ref counting code
    virtual wxObjectRefData *CreateRefData() const;
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

    DECLARE_DYNAMIC_CLASS(wxPlotData)
};

// ----------------------------------------------------------------------------
// Functions for getting/setting a wxPlotData to/from the wxClipboard
// ----------------------------------------------------------------------------

#if wxUSE_DATAOBJ && wxUSE_CLIPBOARD

// Try to get a wxPlotData from the wxClipboard, returns !Ok plotdata on failure
wxPlotData wxClipboardGetPlotData();
// Set the plotdata curve into the wxClipboard, actually just sets a
// wxPlotDataObject which is a string containing wxNow. The plotdata is not
// actually copied to the clipboard since no other program could use it anyway.
// returns sucess
bool wxClipboardSetPlotData(const wxPlotData& plotData);

// ----------------------------------------------------------------------------
// wxPlotDataObject - a wxClipboard object
// ----------------------------------------------------------------------------
#include "wx/dataobj.h"

//#define wxDF_wxPlotData (wxDF_MAX+1010)  // works w/ GTK 1.2 non unicode
extern const wxChar* wxDF_wxPlotData;      // wxT("wxDF_wxPlotData");

class wxPlotDataObject: public wxTextDataObject
{
public:
    wxPlotDataObject();
    wxPlotDataObject(const wxPlotData& plotData);

    wxPlotData GetPlotData() const;
    void SetPlotData(const wxPlotData& plotData);
};

#endif

#endif
