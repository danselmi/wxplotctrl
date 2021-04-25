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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "plotdata.h"
#endif

#include "wx/txtstrm.h"            // for wxEOL
#include "wx/plotctrl/plotcurv.h"  // includes plotdefs.h

class wxRangeIntSelection;
class wxPlotData;

//-----------------------------------------------------------------------------
// wxPlotData consts and defines
//-----------------------------------------------------------------------------

// arbitray reasonable max size to avoid malloc errors
#define wxPLOTDATA_MAX_SIZE 10000000

enum wxPlotDataLoad_Type
{
    // store the header, ie. any #comment lines BEFORE the first line w/ valid data
    //   use Get(Set)Header() to retrieve it
    wxPLOTDATA_LOAD_HEADER = 0x0001,
    // stop loading datafile if there is a blank line, will continue if a line is merely #commented out
    //  ignores any blank lines before first line w/ data however
    wxPLOTDATA_LOAD_BREAKONBLANKLINE = 0x0010,
    // defaults used for loading a data file
    wxPLOTDATA_LOAD_DEFAULT = wxPLOTDATA_LOAD_HEADER | wxPLOTDATA_LOAD_BREAKONBLANKLINE
};

//-----------------------------------------------------------------------------
// wxPlotData
//
// Notes:
//        You must ALWAYS call CalcBoundingRect() after externally modifying the data
//        otherwise it might not be displayed properly in a wxPlotCtrl
//
//-----------------------------------------------------------------------------

// A uncreated wxPlotCurve for reference
extern const wxPlotData wxNullPlotData;

class wxPlotData : public wxPlotCurve
{
public:
    wxPlotData() : wxPlotCurve() {}
    wxPlotData( const wxPlotData& plotData ):wxPlotCurve() { Create(plotData); }

    wxPlotData( int points, bool zero = true ):wxPlotCurve() { Create(points, zero); }
    wxPlotData( double *x_data, double *y_data, int points, bool static_data = false ):wxPlotCurve()
        { Create( x_data, y_data, points, static_data ); }
    virtual ~wxPlotData() {}

    // Ref the source plotdata
    bool Create( const wxPlotData& plotData );
    // Allocate memory for given number of points, if zero then init to zeroes
    //   don't use uninitialized data, trying to plot it will cause problems
    bool Create( int points, bool zero = true );
    // Assign the malloc(ed) data sets to this plotdata,
    //   if !static_data they'll be free(ed) on destruction
    bool Create( double *x_data, double *y_data, int points, bool static_data = false );

    // Make true (not refed) copy of this,
    //   if copy_all = true then copy header, filename, pens, etc
    bool Copy( const wxPlotData &source, bool copy_all = false );
    // Only copy the header, filename, pens, etc... from the source
    bool CopyExtra( const wxPlotData &source );

    // Resize the data by appending or cropping points to/from the end
    //   if zero then zero any added new points
    bool Resize( int new_size, bool zero = true );
    // Resize the data by appending or cropping points to/from the end.
    //   Sets created values with x starting from last point + dx in steps of
    //   dx with a value of y.
    bool Resize( int new_size, double dx, double y );

    // Append the source curve to the end, Yi data is copied only if both have it.
    wxPlotData Append(const wxPlotData &source) const;
    // Insert a the source curve at data index, Yi data is copied only if both have it.
    wxPlotData Insert(const wxPlotData &source, int index) const;
    // Delete a number of points in the curve, if count < 0 then delete to end
    //   do not delete from 0 to end, it will assert, Destroy the data instead
    wxPlotData Remove(int index, int count = -1) const;
    // Get a sub-section of this curve from index of size count points.
    //   if count < 0 then get data from index to end
    wxPlotData GetSubPlotData(int index, int count = -1) const;

    // Unref the data
    void Destroy();

    // Is there data, has it been properly constructed?
    bool Ok() const;

    // Get the number of points in the data set
    int GetCount() const;

    // calc BoundingRect of the data and determine if X is ordered
    // ALWAYS call CalcBoundingRect after externally modifying the data,
    // especially if reording X quantities and using the wxPlotCtrl
    virtual void CalcBoundingRect();

    // are consecutive x points always > than the previous ones
    bool GetIsXOrdered() const;

    //-------------------------------------------------------------------------
    // Get/Set data values
    //-------------------------------------------------------------------------

    // Get a pointer to the data (call CalcBoundingRect afterwards if changing values)
    double *GetXData() const;
    double *GetYData() const;

    // imaginary Y data, not normally created, but if !NULL then it will be free()ed, see FFT
    double *GetYiData() const;
    // use (double*)malloc(sizeof(double)*GetCount()) to create
    // it'll be free()ed if the PlotData was NOT Created with existing arrays and static=true
    void SetYiData( double *yi_data );

    // Get the point's value at this data index
    double GetXValue( int index ) const;
    double GetYValue( int index ) const;
    wxPoint2DDouble GetPoint( int index ) const;
    // Interpolate if necessary to get the y value at this point,
    //   doesn't fail just returns ends if out of bounds
    double GetY( double x ) const;

    // Set the point at this data index, don't need to call CalcBoundingRect after
    void SetXValue( int index, double x );
    void SetYValue( int index, double y );
    void SetValue(int index, double x, double y);
    void SetPoint(int index, const wxPoint2DDouble &pt) { SetValue(index, pt.m_x, pt.m_y); }

    // Set a range of values starting at start_index for count points.
    //   If count = -1 go to end of data
    void SetXValues( int start_index, int count = -1, double x = 0.0 );
    void SetYValues( int start_index, int count = -1, double y = 0.0 );

    // Set a range of values to be steps starting at x_start with dx increment
    //   starts at start_index for count points, if count = -1 go to end
    void SetXStepValues( int start_index, int count = -1,
                         double x_start = 0.0, double dx = 1.0 );
    void SetYStepValues( int start_index, int count = -1,
                         double y_start = 0.0, double dy = 1.0 );

    enum Index_Type
    {
        index_round,
        index_floor,
        index_ceil
    };

    // find the first occurance of an index whose value is closest (index_round),
    //   or the next lower (index_floor), or next higher (index_ceil), to the given value
    //   always returns a valid index
    int GetIndexFromX( double x, wxPlotData::Index_Type type = index_round ) const;
    int GetIndexFromY( double y, wxPlotData::Index_Type type = index_round ) const;
    // find the first occurance of an index whose value is closest to x,y
    //    if x_range != 0 then limit search between +- x_range (useful for x-ordered data)
    int GetIndexFromXY( double x, double y, double x_range=0 ) const;

    // Find the average of the data starting at start_index for number of count points
    //   if count < 0 then to to last point
    double GetAverage( int start_index = 0, int count = -1 ) const;

    // Get the minimum, maximum, and average x,y values for the ranges including
    //  the indexes where the min/maxes occurred.
    //  returns the number of points used.
    int GetMinMaxAve( const wxRangeIntSelection& rangeSel,
                      wxPoint2DDouble* minXY, wxPoint2DDouble* maxXY,
                      wxPoint2DDouble* ave,
                      int *x_min_index, int *x_max_index,
                      int *y_min_index, int *y_max_index ) const;

    // Returns array of indicies of nearest points where the data crosses the point y
    wxArrayInt GetCrossing( double y_value ) const;

    // Get the index of the first point with the min/max index value
    //   if count == -1 then go to end of dataset
    int GetMinYIndex(int start_index = 0, int end_index = -1) const;
    int GetMaxYIndex(int start_index = 0, int end_index = -1) const;

    //-------------------------------------------------------------------------
    // Data processing functions
    //-------------------------------------------------------------------------

    // Add this offset to each data point (data += offset)
    void OffsetX( double offset, int start_index = 0, int end_index = -1 );
    void OffsetY( double offset, int start_index = 0, int end_index = -1 );
    void OffsetXY( double offsetX, double offsetY, int start_index = 0, int end_index = -1 );

    // Scale the data, multiply by scale around offset value (data = (data-offset)*scale+offset)
    void ScaleX( double scale, double offset = 0.0, int start_index = 0, int end_index = -1 );
    void ScaleY( double scale, double offset = 0.0, int start_index = 0, int end_index = -1 );
    void ScaleXY( double scaleX, double scaleY, double offsetX = 0.0, double offsetY = 0.0,
                  int start_index = 0, int end_index = -1 );

    // Raise the data to the power
    void PowerX( double power, int start_index = 0, int end_index = -1 );
    void PowerY( double power, int start_index = 0, int end_index = -1 );
    void PowerXY( double powerX, double powerY, int start_index = 0, int end_index = -1 );

    enum FuncModify_Type
    {
        add_x,
        add_y,
        mult_x,
        mult_y,
        add_yi,
        mult_yi
    };

    wxPlotData Resample( double start_x, double dx, int points ) const;

    // Take the x values of the curve and resample this curve's x values to match
    //   interpolates if necessary. Only for x ordered curves.
    //   returns empty plotdata if ranges don't match
    wxPlotData Resample( const wxPlotData &source ) const;

    // Add y values of curves 1 (this) and 2, after multiplying each curve by their factors
    //    interpolating between points if necessary, but outlying points are ignored
    //    if factor1,2 are both 1.0 then strictly add them
    //    if factor1 = -1 and factor2 = 1 then subtract this curve (1) from 2...
    wxPlotData Add( const wxPlotData &curve2, double factor1 = 1.0, double factor2 = 1.0 ) const;

    // Runaverage the data using a window of width number of points
    //   use odd number, it'll make it odd anyway
    //   points closer then width/2 to ends are not changed
    //   averaging is performed between start_index for count # points, if count < 0 to end
    wxPlotData RunAverage( int width, int start_index=0, int count = -1 ) const;

    // Simple function that does fabs(y) on the data
    wxPlotData Abs() const;

    // Linearize the y-points (straight line) from start_index for count points
    //   x-values are not changed
    wxPlotData LinearizeY(int start_index, int count = -1) const;

    // Simple derivitive y_(n+1) - y_(n)
    wxPlotData Derivitive() const;

    // Variance = SumOverN(sqrt(E(yn-<y>)^2))/N
    double Variance(int start_index = 0, int count = -1) const;

    // returns a curve that is the variance at each point in a window of width
    wxPlotData VarianceCurve(int width) const;

    // Deviation = sqrt(E(y-y1)^2), don't divide by max-min+1 since
    double Deviation( const wxPlotData &other, int min=0, int max=-1 ) const;
    double CrossCorrelation( const wxPlotData &other, int runave=0, int min=0, int max=-1 ) const;

    // Tries to line up these two curves by shifting the other along the x-axis
    //   it returns the x shift that gives the minimum deviation
    double MinShiftX( const wxPlotData &other ) const;

    //-------------------------------------------------------------------------
    // Get/Set Symbols to use for plotting - CreateSymbol is untested
    //   note: in MSW drawing bitmaps is sloooow! <-- so I haven't bothered finishing
    //-------------------------------------------------------------------------

    // Get the symbol used for marking data points
    wxBitmap GetSymbol(wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL) const;
    // Set the symbol to some arbitray bitmap, make size odd so it can be centered
    void SetSymbol( const wxBitmap &bitmap, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL );
    // Set the symbol from of the available types, using default colours if pen and brush are NULL
    void SetSymbol( wxPlotSymbol_Type type, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL,
                    int width = 5, int height = 5,
                    const wxPen *pen = NULL, const wxBrush *brush = NULL);
    // Get a copy of the symbol thats created for SetSymbol
    wxBitmap CreateSymbol( wxPlotSymbol_Type type, wxPlotPen_Type colour_type=wxPLOTPEN_NORMAL,
                           int width = 5, int height = 5,
                           const wxPen *pen = NULL, const wxBrush *brush = NULL);

    //-----------------------------------------------------------------------
    // Operators

    bool operator == (const wxPlotData& plotData) const
        { return m_refData == plotData.m_refData; }
    bool operator != (const wxPlotData& plotData) const
        { return m_refData != plotData.m_refData; }

    wxPlotData& operator = (const wxPlotData& plotData)
    {
        if ( (*this) != plotData )
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

#endif // _WX_PLOTDATA_H_
