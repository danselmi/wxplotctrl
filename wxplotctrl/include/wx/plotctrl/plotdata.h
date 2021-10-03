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

#include "wx/txtstrm.h"            // for wxEOL
#include "wx/geometry.h"
#include "wx/clntdata.h"

class wxRangeIntSelection;
class wxPlotData;

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

// Find y at point x along the line from (x0,y0)-(x1,y1), x0 must != x1
extern double LinearInterpolateY(double x0, double y0,
                                  double x1, double y1,
                                  double x);
// Find x at point y along the line from (x0,y0)-(x1,y1), y0 must != y1
extern double LinearInterpolateX(double x0, double y0,
                                  double x1, double y1,
                                  double y);

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

// defines wxArrayDouble for use as necessary
//WX_DEFINE_USER_EXPORTED_ARRAY_DOUBLE(double, wxArrayDouble, class WXDLLIMPEXP_PLOTCTRL);

// wxNullPlotBounds = wxRect2DDouble(0,0,0,0)
extern const wxRect2DDouble wxNullPlotBounds;
WX_DECLARE_OBJARRAY_WITH_DECL(wxPen, wxArrayPen, class);

extern wxBitmap wxPlotSymbolNormal;
extern wxBitmap wxPlotSymbolActive;
extern wxBitmap wxPlotSymbolSelected;

enum wxPlotSymbol_Type
{
    wxPLOTSYMBOL_ELLIPSE,
    wxPLOTSYMBOL_RECTANGLE,
    wxPLOTSYMBOL_CROSS,
    wxPLOTSYMBOL_PLUS,
    wxPLOTSYMBOL_MAXTYPE
};

enum wxPlotPen_Type
{
    wxPLOTPEN_NORMAL,
    wxPLOTPEN_ACTIVE,
    wxPLOTPEN_SELECTED,
    wxPLOTPEN_MAXTYPE
};

//-----------------------------------------------------------------------------
// wxPlotData consts and defines
//-----------------------------------------------------------------------------

// arbitray reasonable max size to avoid malloc errors
#define wxPLOTDATA_MAX_SIZE 10000000

//-----------------------------------------------------------------------------
// wxPlotData
//
// Notes:
//        You must ALWAYS call CalcBoundingRect() after externally modifying the data
//        otherwise it might not be displayed properly in a wxPlotCtrl
//
//-----------------------------------------------------------------------------

class wxPlotData : public wxObject
{
public:
    wxPlotData():
        wxObject() {}
    wxPlotData(const wxPlotData& plotData):wxObject() {Create(plotData);}

    wxPlotData(int points, bool zero = true):wxObject() {Create(points, zero);}
    wxPlotData(double *x_data, double *y_data, int points, bool static_data = false):wxObject()
        {Create(x_data, y_data, points, static_data);}
    virtual ~wxPlotData(){}

    // Ref the source plotdata
    bool Create(const wxPlotData& plotData);
    // Allocate memory for given number of points, if zero then init to zeroes
    //   don't use uninitialized data, trying to plot it will cause problems
    bool Create(int points, bool zero = true);
    // Assign the malloc(ed) data sets to this plotdata,
    //   if !static_data they'll be free(ed) on destruction
    bool Create(double *x_data, double *y_data, int points, bool static_data = false);

    // Make true (not refed) copy of this,
    //   if copy_all = true then copy header, filename, pens, etc
    bool Copy(const wxPlotData &source, bool copy_all = false);
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

    // Set the point at this data index, don't need to call CalcBoundingRect after
    void SetXValue(int index, double x);
    void SetYValue(int index, double y);
    void SetValue(int index, double x, double y);
    void SetPoint(int index, const wxPoint2DDouble &pt) {SetValue(index, pt.m_x, pt.m_y);}

    // Set a range of values starting at start_index for count points.
    //   If count = -1 go to end of data
    void SetXValues(int start_index, int count = -1, double x = 0.0);
    void SetYValues(int start_index, int count = -1, double y = 0.0);

    // Set a range of values to be steps starting at x_start with dx increment
    //   starts at start_index for count points, if count = -1 go to end
    void SetXStepValues(int start_index, int count = -1,
                        double x_start = 0.0, double dx = 1.0);
    void SetYStepValues(int start_index, int count = -1,
                        double y_start = 0.0, double dy = 1.0);

    enum Index_Type
    {
        index_round,
        index_floor,
        index_ceil
    };

    // find the first occurance of an index whose value is closest (index_round),
    //   or the next lower (index_floor), or next higher (index_ceil), to the given value
    //   always returns a valid index
    int GetIndexFromX(double x, wxPlotData::Index_Type type = index_round) const;
    int GetIndexFromY(double y, wxPlotData::Index_Type type = index_round) const;
    // find the first occurance of an index whose value is closest to x,y
    //    if x_range != 0 then limit search between +- x_range (useful for x-ordered data)
    int GetIndexFromXY(double x, double y, double x_range=0) const;

    // Find the average of the data starting at start_index for number of count points
    //   if count < 0 then to to last point
    double GetAverage(int start_index = 0, int count = -1) const;

    // Get the minimum, maximum, and average x,y values for the ranges including
    //  the indexes where the min/maxes occurred.
    //  returns the number of points used.
    int GetMinMaxAve(const wxRangeIntSelection& rangeSel,
                      wxPoint2DDouble* minXY, wxPoint2DDouble* maxXY,
                      wxPoint2DDouble* ave,
                      int *x_min_index, int *x_max_index,
                      int *y_min_index, int *y_max_index) const;

    // Returns array of indicies of nearest points where the data crosses the point y
    wxArrayInt GetCrossing(double y_value) const;

    enum FuncModify_Type
    {
        add_x,
        add_y,
        mult_x,
        mult_y,
        add_yi,
        mult_yi
    };

    //-------------------------------------------------------------------------
    // Get/Set Symbols to use for plotting - CreateSymbol is untested
    //   note: in MSW drawing bitmaps is sloooow! <-- so I haven't bothered finishing
    //-------------------------------------------------------------------------

    // Get the symbol used for marking data points
    wxBitmap GetSymbol(wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL) const;
    // Set the symbol to some arbitray bitmap, make size odd so it can be centered
    void SetSymbol(const wxBitmap &bitmap, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL);
    // Set the symbol from of the available types, using default colours if pen and brush are NULL
    void SetSymbol(wxPlotSymbol_Type type, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL,
                    int width = 5, int height = 5,
                    const wxPen *pen = NULL, const wxBrush *brush = NULL);
    // Get a copy of the symbol thats created for SetSymbol
    wxBitmap CreateSymbol(wxPlotSymbol_Type type, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL,
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
    wxPen GetPen(wxPlotPen_Type colour_type) const;
    void SetPen(wxPlotPen_Type colour_type, const wxPen &pen);

    // Get/Set Default Pens for Normal, Active, Selected drawing for all curves
    //   these are the pens that are used when a wxPlotCurve/Function/Data is created
    //   default: Normal(0,0,0,1,wxSOLID), Active(0,0,255,1,wxSOLID), Selected(255,0,0,1,wxSOLID)
    static wxPen GetDefaultPen(wxPlotPen_Type colour_type);
    static void SetDefaultPen(wxPlotPen_Type colour_type, const wxPen &pen);

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

class wxPlotDataObject : public wxTextDataObject
{
public:
    wxPlotDataObject();
    wxPlotDataObject(const wxPlotData& plotData);

    wxPlotData GetPlotData() const;
    void SetPlotData(const wxPlotData& plotData);
};

#endif // wxUSE_DATAOBJ && wxUSE_CLIPBOARD

#endif
