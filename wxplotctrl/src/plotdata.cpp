/////////////////////////////////////////////////////////////////////////////
// Name:        plotdata.cpp
// Purpose:     wxPlotData container class for wxPlotCtrl
// Author:      John Labenski
// Modified by:
// Created:     12/01/2000
// Copyright:   (c) John Labenski
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include "wx/bitmap.h"
#include "wx/textdlg.h"
#include "wx/msgdlg.h"
#include "wx/dcmemory.h"
#include "wx/file.h"
#include "wx/wfstream.h"
#include "wx/textfile.h"
#include "wx/math.h"
#include "wx/arrimpl.cpp"

#include "wx/plotctrl/plotdata.h"
#include "wx/plotctrl/range.h"

const wxRect2DDouble wxNullPlotBounds(0, 0, 0, 0);
WX_DEFINE_OBJARRAY(wxArrayPen)

#define wxPCHECK_MINMAX_RET(val, min_val, max_val, msg) \
    wxCHECK_RET((int(val)>=int(min_val))&&(int(val)<=int(max_val)), msg)

#define wxPLOTDATA_MAX_DATA_COLUMNS 64

#define CHECK_INDEX_COUNT_MSG(index, count, max_count, ret) \
    wxCHECK_MSG((int(index) >= 0) && (int(index)+int(count) <= int(max_count)), ret, wxT("invalid index or count"))
#define CHECK_INDEX_COUNT_RET(index, count, max_count) \
    wxCHECK_RET((int(index) >= 0) && (int(index)+int(count) <= int(max_count)), wxT("invalid index or count"))

#define CHECK_START_END_INDEX_MSG(start_index, end_index, max_count, ret) \
    wxCHECK_MSG((int(start_index)>=0)&&(int(start_index)<int(max_count))&&(int(end_index)>int(start_index))&&(int(end_index)<int(max_count)), ret, wxT("Invalid data index"))
#define CHECK_START_END_INDEX_RET(start_index, end_index, max_count) \
    wxCHECK_RET((int(start_index)>=0)&&(int(start_index)<int(max_count))&&(int(end_index)>int(start_index))&&(int(end_index)<int(max_count)), wxT("Invalid data index"))

const wxPlotData wxNullPlotData;

//----------------------------------------------------------------------------
// wxPlotDataRefData
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// wxPlotRefData - the wxObject::m_refData used for wxPlotCurves
//   this should be the base class for ref data for your subclassed curves
//
//   The ref data is also of class wxClientDataContainer so that you can
//     attach arbitrary data to it
//----------------------------------------------------------------------------

class wxPlotRefData: public wxObjectRefData, public wxClientDataContainer
{
public:
    wxPlotRefData();
    wxPlotRefData(const wxPlotRefData& data);
    virtual ~wxPlotRefData();

    void Destroy();
    void CopyData(const wxPlotRefData &source);
    void CopyExtra(const wxPlotRefData &source);

    wxRect2DDouble m_boundingRect; // bounds the curve or part to draw
                                   // if width or height <= 0 then no bounds

    wxArrayPen m_pens;
    static wxArrayPen sm_defaultPens;

    int     m_count;

    double *m_Xdata;
    double *m_Ydata;
    bool    m_static;


    wxBitmap m_normalSymbol,
             m_activeSymbol,
             m_selectedSymbol;
};

#define M_PLOTCURVEDATA ((wxPlotRefData*)m_refData)

wxArrayPen wxPlotRefData::sm_defaultPens;

void InitPlotCurveDefaultPens()
{
    static bool s_init_default_pens = false;
    if (!s_init_default_pens)
    {
        s_init_default_pens = true;
        wxPlotRefData::sm_defaultPens.Add(wxPen(wxColour(  0, 0,   0), 1, wxPENSTYLE_SOLID));
        wxPlotRefData::sm_defaultPens.Add(wxPen(wxColour(  0, 0, 255), 1, wxPENSTYLE_SOLID));
        wxPlotRefData::sm_defaultPens.Add(wxPen(wxColour(255, 0,   0), 1, wxPENSTYLE_SOLID));
    }
}

wxPlotRefData::wxPlotRefData()
{
    InitPlotCurveDefaultPens();
    m_pens = sm_defaultPens;

    m_count  = 0;

    m_Xdata  = nullptr;
    m_Ydata  = nullptr;

    m_static = false;

    m_normalSymbol   = wxPlotSymbolNormal;
    m_activeSymbol   = wxPlotSymbolActive;
    m_selectedSymbol = wxPlotSymbolSelected;
}

wxPlotRefData::wxPlotRefData(const wxPlotRefData& data)
{
    CopyData(data);
    CopyExtra(data);
}

wxPlotRefData::~wxPlotRefData()
{
    Destroy();
}

void wxPlotRefData::Destroy()
{
    if (!m_static)
    {
        if (m_Xdata) delete[]m_Xdata;
        if (m_Ydata) delete[]m_Ydata;
    }

    m_count    = 0;
    m_Xdata    = nullptr;
    m_Ydata    = nullptr;
}

void wxPlotRefData::CopyData(const wxPlotRefData &source)
{
    Destroy();

    m_count  = source.m_count;
    m_static = false; // we're creating our own copy

    if (m_count && source.m_Xdata)
    {
        m_Xdata = new double[m_count];
        memcpy(m_Xdata, source.m_Xdata, m_count*sizeof(double));
    }
    if (m_count && source.m_Ydata)
    {
        m_Ydata = new double[m_count];
        memcpy(m_Ydata, source.m_Ydata, m_count*sizeof(double));
    }
}

void wxPlotRefData::CopyExtra(const wxPlotRefData &source)
{
    m_normalSymbol   = source.m_normalSymbol;
    m_activeSymbol   = source.m_activeSymbol;
    m_selectedSymbol = source.m_selectedSymbol;

    m_boundingRect = source.m_boundingRect;
    m_pens         = source.m_pens;
}

#define M_PLOTDATA ((wxPlotRefData*)m_refData)

// Can't load these now since wxWindows must initialize first
wxBitmap wxPlotSymbolNormal;
wxBitmap wxPlotSymbolActive;
wxBitmap wxPlotSymbolSelected;

//----------------------------------------------------------------------------
// Interpolate
//----------------------------------------------------------------------------
double LinearInterpolateX(double x0, double y0, double x1, double y1, double y)
{
    //wxCHECK_MSG((y1 - y0) != 0.0, 0.0, wxT("Divide by zero, LinearInterpolateX()"));
    return ((y - y0)*(x1 - x0)/(y1 - y0) + x0);
}

double LinearInterpolateY(double x0, double y0, double x1, double y1, double x)
{
    //wxCHECK_MSG((x1 - x0) != 0.0, 0.0, wxT("Divide by zero, LinearInterpolateY()"));
    double m = (y1 - y0) / (x1 - x0);
    return (m*x + (y0 - m*x0));
}

//-----------------------------------------------------------------------------
// wxPlotData
//-----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(wxPlotData, wxObject)

wxObjectRefData *wxPlotData::CreateRefData() const
{
    return new wxPlotRefData;
}
wxObjectRefData *wxPlotData::CloneRefData(const wxObjectRefData *data) const
{
    return new wxPlotRefData(*(const wxPlotRefData *)data);
}

bool wxPlotData::Ok() const
{
    return m_refData && (M_PLOTDATA->m_count > 0);
}

void wxPlotData::Destroy()
{
    UnRef();
}

int wxPlotData::GetCount() const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->m_count;
}

bool wxPlotData::Create(const wxPlotData& plotData)
{
    wxCHECK_MSG(plotData.Ok(), false, wxT("Invalid wxPlotData"));

    Ref(plotData);
    CalcBoundingRect(); // just to be sure we're ok
    return true;
}

bool wxPlotData::Create(int points, bool zero)
{
    wxCHECK_MSG(points > 0, false, wxT("Can't create wxPlotData with < 1 points"));

    UnRef();
    m_refData = new wxPlotRefData();

    if (!M_PLOTDATA)
    {
        wxFAIL_MSG(wxT("memory allocation error creating plot"));
        return false;
    }

    M_PLOTDATA->m_count = points;
    M_PLOTDATA->m_Xdata = new double[points];
    M_PLOTDATA->m_Ydata = new double[points];
    if (!M_PLOTDATA->m_Xdata || !M_PLOTDATA->m_Ydata)
    {
        UnRef();
        wxFAIL_MSG(wxT("memory allocation error creating plot"));
        return false;
    }

    if (zero)
    {
        memset(M_PLOTDATA->m_Xdata, 0, points*sizeof(double));
        memset(M_PLOTDATA->m_Ydata, 0, points*sizeof(double));
    }

    return true;
}

bool wxPlotData::Create(double *x_data, double *y_data, int points, bool static_data)
{
    wxCHECK_MSG((points > 0) && x_data && y_data, false,
                wxT("Can't create wxPlotData with < 1 points or invalid data"));

    UnRef();
    m_refData = new wxPlotRefData();

    if (!M_PLOTDATA)
    {
        wxFAIL_MSG(wxT("memory allocation error creating plot"));
        return false;
    }

    M_PLOTDATA->m_Xdata  = x_data;
    M_PLOTDATA->m_Ydata  = y_data;
    M_PLOTDATA->m_count  = points;
    M_PLOTDATA->m_static = static_data;

    CalcBoundingRect();
    return true;
}

bool wxPlotData::Copy(const wxPlotData &source, bool copy_all)
{
    wxCHECK_MSG(source.Ok(), false, wxT("Invalid wxPlotData"));

    int count = source.GetCount();

    if (!Create(count, false)) return false;

    memcpy(M_PLOTDATA->m_Xdata, ((wxPlotRefData *)source.m_refData)->m_Xdata, count*sizeof(double));
    memcpy(M_PLOTDATA->m_Ydata, ((wxPlotRefData *)source.m_refData)->m_Ydata, count*sizeof(double));

    if (copy_all)
        CopyExtra(source);

    CalcBoundingRect();
    return true;
}

bool wxPlotData::CopyExtra(const wxPlotData &source)
{
    wxCHECK_MSG(Ok() && source.Ok(), false, wxT("Invalid wxPlotData"));

    M_PLOTDATA->CopyData(*((wxPlotRefData*)source.GetRefData()));

    return true;
}

void wxPlotData::CalcBoundingRect()
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));

    M_PLOTDATA->m_boundingRect = wxNullPlotBounds;

    double *x_data = M_PLOTDATA->m_Xdata,
           *y_data = M_PLOTDATA->m_Ydata;

    double x = *x_data,
           y = *y_data,
           xmin = x,
           xmax = x,
           ymin = y,
           ymax = y,
           xlast = x;

    bool valid = false;

    int i, count = M_PLOTDATA->m_count;

    for (i=0; i<count; i++)
    {
        x = *x_data++;
        y = *y_data++;

        if ((wxFinite(x) == 0) || (wxFinite(y) == 0)) continue;

        if (!valid) // initialize the bounds
        {
           valid = true;
           xmin = xmax = xlast = x;
           ymin = ymax = y;
           continue;
        }

        if      (x < xmin) xmin = x;
        else if (x > xmax) xmax = x;

        if      (y < ymin) ymin = y;
        else if (y > ymax) ymax = y;

        if (xlast <= x)
            xlast = x;
    }

    if (valid)
        M_PLOTDATA->m_boundingRect = wxRect2DDouble(xmin, ymin, xmax-xmin, ymax-ymin);
    else
        M_PLOTDATA->m_boundingRect = wxNullPlotBounds;
}

double *wxPlotData::GetXData() const
{
    wxCHECK_MSG(Ok(), (double*)NULL, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->m_Xdata;
}
double *wxPlotData::GetYData() const
{
    wxCHECK_MSG(Ok(), (double*)NULL, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->m_Ydata;
}
//----------------------------------------------------------------------------
// Load/Save Get/Set Filename, Header
//----------------------------------------------------------------------------

int NumberParse(double *nums, const wxString &string, int max_nums)
{
    static const wxChar d1 = wxT(',');  // 44; // comma
    static const wxChar d2 = wxT('\t'); //  9; // tab
    static const wxChar d3 = wxT(' ');  // 32; // space
    static const wxChar d4 = wxT('\n'); // 13; // carrage return
    static const wxChar d5 = wxT('\r'); // 10; // line feed

//    char *D = "D";    // for Quick Basic, uses 1D3 not 1E3
//    char *E = "E";

    const wxChar *s = string.GetData();
    wxChar c = 0;

    int i, count = string.Length();
    double number;

    int n = 0;
    int start_word = -1;

    for (i=0; i<=count; i++)
    {
        c = *s;
        if ((c == d1 || c == d2 || c == d2 || c == d3 || c == d4 || c == d5) || (i >= count))
        {
            if (start_word != -1)
            {
                if (string.Mid(start_word, i - start_word).ToDouble(&number))
                {
                    nums[n] = number;
                    n++;
                    if (n > max_nums) return n;
                    start_word = -1;
                }
                else
                    return n;
            }
        }
        else if (start_word == -1) start_word = i;

        if (c == d4 || c == d5) return n;

        s++;
    }
    return n;
}

//----------------------------------------------------------------------------
// Get(X/Y)Data
//----------------------------------------------------------------------------

double wxPlotData::GetXValue(int index) const
{
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->m_count), 0.0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->m_Xdata[index];
}
double wxPlotData::GetYValue(int index) const
{
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->m_count), 0.0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->m_Ydata[index];
}
wxPoint2DDouble wxPlotData::GetPoint(int index) const
{
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->m_count), wxPoint2DDouble(0,0), wxT("Invalid wxPlotData"));
    return wxPoint2DDouble(M_PLOTDATA->m_Xdata[index], M_PLOTDATA->m_Ydata[index]);
}

double wxPlotData::GetY(double x) const
{
    wxCHECK_MSG(Ok(), 0, wxT("invalid wxPlotData"));

    int i = GetIndexFromX(x, index_floor);

    if (M_PLOTDATA->m_Xdata[i] == x)
        return M_PLOTDATA->m_Ydata[i];

    if (i >= M_PLOTDATA->m_count - 1)
        return M_PLOTDATA->m_Ydata[i];

    int i1 = GetIndexFromX(x, index_ceil);

    double y0 = M_PLOTDATA->m_Ydata[i];
    double y1 = M_PLOTDATA->m_Ydata[i1];

    if (y0 == y1)
        return y0;

    return LinearInterpolateY(M_PLOTDATA->m_Xdata[i], y0,
                               M_PLOTDATA->m_Xdata[i1], y1, x);
}

void wxPlotData::SetXValue(int index, double x)
{
    wxCHECK_RET(Ok() && (index < M_PLOTDATA->m_count), wxT("Invalid wxPlotData"));

    if (M_PLOTDATA->m_count == 1)
        M_PLOTDATA->m_boundingRect.m_x = x;
    else
    {
        if (x < M_PLOTDATA->m_boundingRect.m_x)
            M_PLOTDATA->m_boundingRect.SetLeft(x);
        else if (x > M_PLOTDATA->m_boundingRect.GetRight())
            M_PLOTDATA->m_boundingRect.SetRight(x);
        else
            CalcBoundingRect(); // don't know recalc it all
    }

    M_PLOTDATA->m_Xdata[index] = x;

}
void wxPlotData::SetYValue(int index, double y)
{
    wxCHECK_RET(Ok() && (index < M_PLOTDATA->m_count), wxT("Invalid wxPlotData"));

    if (M_PLOTDATA->m_count == 1)
        M_PLOTDATA->m_boundingRect.m_y = y;
    else
    {
        if (y < M_PLOTDATA->m_boundingRect.m_y)
            M_PLOTDATA->m_boundingRect.SetTop(y);
        else if (y > M_PLOTDATA->m_boundingRect.GetBottom())
            M_PLOTDATA->m_boundingRect.SetBottom(y);
        else
            CalcBoundingRect(); // don't know recalc it all
    }

    M_PLOTDATA->m_Ydata[index] = y;
}

void wxPlotData::SetValue(int index, double x, double y)
{
    wxCHECK_RET(Ok() && (index < M_PLOTDATA->m_count), wxT("Invalid wxPlotData"));

    double x_old = M_PLOTDATA->m_Xdata[index];
    double y_old = M_PLOTDATA->m_Ydata[index];

    M_PLOTDATA->m_Xdata[index] = x;
    M_PLOTDATA->m_Ydata[index] = y;

    if (M_PLOTDATA->m_count == 1)
    {
        M_PLOTDATA->m_boundingRect.m_x = x;
        M_PLOTDATA->m_boundingRect.m_y = y;
    }
    else
    {
        if ((x_old <= M_PLOTDATA->m_boundingRect.m_x) ||
             (x_old >= M_PLOTDATA->m_boundingRect.GetRight()) ||
             (y_old >= M_PLOTDATA->m_boundingRect.m_y) ||
             (y_old <= M_PLOTDATA->m_boundingRect.GetBottom()))
            CalcBoundingRect(); // don't know recalc it all
        else
        {
            if (x < M_PLOTDATA->m_boundingRect.m_x)
                M_PLOTDATA->m_boundingRect.m_x = x;
            if (x > M_PLOTDATA->m_boundingRect.GetRight())
                M_PLOTDATA->m_boundingRect.SetRight(x);

            if (y > M_PLOTDATA->m_boundingRect.m_y)
                M_PLOTDATA->m_boundingRect.m_y = y;
            if (y < M_PLOTDATA->m_boundingRect.GetBottom())
                M_PLOTDATA->m_boundingRect.SetBottom(y);
        }
    }
}

void wxPlotData::SetXValues(int start_index, int count, double x)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    if (count == 0) return;
    if (count < 0) count = M_PLOTDATA->m_count-start_index;
    int end_index = start_index + count - 1;
    wxPCHECK_MINMAX_RET(start_index, 0, M_PLOTDATA->m_count-1, wxT("Invalid starting index"));
    wxPCHECK_MINMAX_RET(end_index,   0, M_PLOTDATA->m_count-1, wxT("Invalid ending index"));
    double *x_data = M_PLOTDATA->m_Xdata;
    for (int n = start_index; n <= end_index; n++)
        *x_data++ = x;
}
void wxPlotData::SetYValues(int start_index, int count, double y)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    if (count == 0) return;
    if (count < 0) count = M_PLOTDATA->m_count-start_index;
    int end_index = start_index + count - 1;
    wxPCHECK_MINMAX_RET(start_index, 0, M_PLOTDATA->m_count-1, wxT("Invalid starting index"));
    wxPCHECK_MINMAX_RET(end_index,   0, M_PLOTDATA->m_count-1, wxT("Invalid ending index"));
    double *y_data = M_PLOTDATA->m_Ydata;
    for (int n = start_index; n <= end_index; n++)
        *y_data++ = y;
}

void wxPlotData::SetXStepValues(int start_index, int count, double x_start, double dx)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    if (count == 0) return;
    if (count < 0) count = M_PLOTDATA->m_count-start_index;
    int end_index = start_index + count - 1;
    wxPCHECK_MINMAX_RET(start_index, 0, M_PLOTDATA->m_count-1, wxT("Invalid starting index"));
    wxPCHECK_MINMAX_RET(end_index,   0, M_PLOTDATA->m_count-1, wxT("Invalid ending index"));

    double *x_data = M_PLOTDATA->m_Xdata + start_index;
    for (int i = 0; i < count; i++, x_data++)
        *x_data = x_start + (i * dx);
}
void wxPlotData::SetYStepValues(int start_index, int count, double y_start, double dy)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    if (count == 0) return;
    if (count < 0) count = M_PLOTDATA->m_count-start_index;
    int end_index = start_index + count - 1;
    wxPCHECK_MINMAX_RET(start_index, 0, M_PLOTDATA->m_count-1, wxT("Invalid starting index"));
    wxPCHECK_MINMAX_RET(end_index,   0, M_PLOTDATA->m_count-1, wxT("Invalid ending index"));

    double *y_data = M_PLOTDATA->m_Ydata + start_index;
    for (int i = 0; i < count; i++, y_data++)
        *y_data = y_start + (i * dy);
}

int wxPlotData::GetIndexFromX(double x, wxPlotData::Index_Type type) const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));

    int count = M_PLOTDATA->m_count;
    double *x_data = M_PLOTDATA->m_Xdata;

    int i;
    int index = 0, index_lower = 0, index_higher = 0;
    double closest = fabs(x - *x_data++);

    for (i=1; i<count; i++)
    {
        if (fabs(x - *x_data) < closest)
        {
            if (x == *x_data) return i;

            closest = fabs(x - *x_data);
            index = i;

            if (x > *x_data)
                index_lower = i;
            else
                index_higher = i;
        }
        x_data++;
    }

    // out of bounds so just return the closest
    if ((x < M_PLOTDATA->m_boundingRect.GetLeft()) ||
        (x > M_PLOTDATA->m_boundingRect.GetRight()))
        return index;

    if (type == index_floor) return index_lower;
    if (type == index_ceil) return index_higher;
    return index;
}

int wxPlotData::GetIndexFromY(double y, wxPlotData::Index_Type type) const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));

    int i;
    int index = 0, index_lower = 0, index_higher = 0;
    int count = M_PLOTDATA->m_count;
    double *y_data = M_PLOTDATA->m_Ydata;
    double closest = fabs(y - *y_data++);

    for (i=1; i<count; i++)
    {
        if (fabs(y - *y_data) < closest)
        {
            if (y == *y_data) return i;

            closest = fabs(y - *y_data);
            index = i;

            if (y > *y_data)
                index_lower = i;
            else
                index_higher = i;
        }
        y_data++;
    }

    // out of bounds so just return the closest
    if ((y < M_PLOTDATA->m_boundingRect.GetLeft()) ||
        (y > M_PLOTDATA->m_boundingRect.GetRight()))
        return index;

    if (type == index_floor) return index_lower;
    if (type == index_ceil) return index_higher;
    return index;
}

int wxPlotData::GetIndexFromXY(double x, double y, double x_range) const
{
    wxCHECK_MSG(Ok() && (x_range >= 0), 0, wxT("Invalid wxPlotData"));

    int start = 1, end = M_PLOTDATA->m_count - 1;

    int i, index = start - 1;

    double *x_data = &M_PLOTDATA->m_Xdata[index];
    double *y_data = &M_PLOTDATA->m_Ydata[index];

    double xdiff = (*x_data++) - x;
    double ydiff = (*y_data++) - y;
    double diff = xdiff*xdiff + ydiff*ydiff;
    double min_diff = diff;

    double x_lower = x - x_range, x_higher = x + x_range;

    for (i=start; i<=end; i++)
    {
        if ((x_range != 0) && ((*x_data < x_lower) || (*x_data > x_higher)))
        {
            x_data++;
            y_data++;
            continue;
        }

        xdiff = (*x_data++) - x;
        ydiff = (*y_data++) - y;
        diff = xdiff*xdiff + ydiff*ydiff;

        if (diff < min_diff)
        {
            min_diff = diff;
            index = i;
        }
    }

    return index;
}

double wxPlotData::GetAverage(int start_index, int count) const
{
    wxCHECK_MSG(Ok(), 0.0, wxT("Invalid wxPlotData"));
    if (count < 0) count = M_PLOTDATA->m_count-start_index;
    int end_index = start_index + count - 1;
    wxCHECK_MSG((start_index<M_PLOTDATA->m_count) && (end_index<M_PLOTDATA->m_count), 0.0, wxT("invalid input"));

    double ave = 0.0;
    double *y_data = M_PLOTDATA->m_Ydata + start_index;
    for (int i=start_index; i<=end_index; i++) ave += *y_data++;

    ave /= double(count);
    return ave;
}

int wxPlotData::GetMinMaxAve(const wxRangeIntSelection& rangeSel,
                              wxPoint2DDouble* minXY_, wxPoint2DDouble* maxXY_,
                              wxPoint2DDouble* ave_,
                              int *x_min_index_, int *x_max_index_,
                              int *y_min_index_, int *y_max_index_) const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid data curve"));
    wxCHECK_MSG(rangeSel.GetCount() != 0, 0, wxT("Invalid range selection"));

    int min_index = rangeSel.GetRange(0).m_min;
    //int max_index = rangeSel.GetRange(sel_count-1).m_max;

    wxCHECK_MSG((min_index >= 0) && (min_index < (int)M_PLOTDATA->m_count), 0,
                wxT("Invalid range selection index in data curve"));

    double *x_data = M_PLOTDATA->m_Xdata;
    double *y_data = M_PLOTDATA->m_Ydata;

    double x = x_data[min_index];
    double y = y_data[min_index];

    // Find the X and Y min/max/ave values of the selection
    int x_min_index = min_index, x_max_index = min_index;
    int y_min_index = min_index, y_max_index = min_index;
    double x_min_x = x, x_max_x = x;
    double y_min_y = y, y_max_y = y;
    double ave_x = 0, ave_y = 0;
    int i, j, sel_count = rangeSel.GetCount(), sel_point_count = 0;

    for (i=0; i<sel_count; i++)
    {
        wxRangeInt r = rangeSel.GetRange(i);
        wxCHECK_MSG((r.m_min >= 0) && (r.m_min < (int)M_PLOTDATA->m_count) &&
                    (r.m_max >= 0) && (r.m_max < (int)M_PLOTDATA->m_count), 0,
                    wxT("Invalid range selection index in data curve"));

        for (j=r.m_min; j<=r.m_max; j++) // yes we duplicate first point
        {
            sel_point_count++;
            x = x_data[j];
            y = y_data[j];

            if (x < x_min_x) {x_min_x = x; x_min_index = j;}
            if (x > x_max_x) {x_max_x = x; x_max_index = j;}
            if (y < y_min_y) {y_min_y = y; y_min_index = j;}
            if (y > y_max_y) {y_max_y = y; y_max_index = j;}

            ave_x += x;
            ave_y += y;
        }
    }

    ave_x /= double(sel_point_count);
    ave_y /= double(sel_point_count);

    if (ave_)   *ave_   = wxPoint2DDouble(ave_x, ave_y);
    if (minXY_) *minXY_ = wxPoint2DDouble(x_min_x, y_min_y);
    if (maxXY_) *maxXY_ = wxPoint2DDouble(x_max_x, y_max_y);
    if (x_min_index_) *x_min_index_ = x_min_index;
    if (x_max_index_) *x_max_index_ = x_max_index;
    if (y_min_index_) *y_min_index_ = y_min_index;
    if (y_max_index_) *y_max_index_ = y_max_index;

    return sel_point_count;
}

wxArrayInt wxPlotData::GetCrossing(double y_value) const
{
    wxArrayInt points;
    wxCHECK_MSG(Ok(), points, wxT("Invalid wxPlotData"));

    int i;
    double *y_data = M_PLOTDATA->m_Ydata;
    double y, last_y = M_PLOTDATA->m_Ydata[0];

    for (i=1; i<M_PLOTDATA->m_count; i++)
    {
        y = y_data[i];

        if (((last_y >= y_value) && (y <= y_value)) ||
            ((last_y <= y_value) && (y >= y_value)))
        {
            if (fabs(last_y - y_value) < fabs(y - y_value))
                points.Add(i-1);
            else
                points.Add(i);
        }
        last_y = y;
    }

    return points;
}

//----------------------------------------------------------------------------
// Get/Set bitmap symbol -- FIXME - this is NOT FINISHED OR WORKING
//----------------------------------------------------------------------------

wxBitmap wxPlotData::GetSymbol(wxPlotPen_Type colour_type) const
{
    wxCHECK_MSG(Ok(), M_PLOTDATA->m_normalSymbol, wxT("Invalid wxPlotData"));

    switch (colour_type)
    {
        case wxPLOTPEN_ACTIVE :
            return M_PLOTDATA->m_activeSymbol;
        case wxPLOTPEN_SELECTED :
            return M_PLOTDATA->m_selectedSymbol;
        default : break; //case wxPLOTPEN_NORMAL :
    }

    return M_PLOTDATA->m_normalSymbol;
}

void wxPlotData::SetSymbol(const wxBitmap &bitmap, wxPlotPen_Type colour_type)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    wxCHECK_RET(bitmap.Ok(), wxT("Invalid bitmap"));

    switch (colour_type)
    {
        case wxPLOTPEN_ACTIVE :
            M_PLOTDATA->m_activeSymbol = bitmap;
            break;
        case wxPLOTPEN_SELECTED :
            M_PLOTDATA->m_selectedSymbol = bitmap;
            break;
        default : //case wxPLOTPEN_NORMAL :
            M_PLOTDATA->m_normalSymbol = bitmap;
            break;
    }
}
void wxPlotData::SetSymbol(wxPlotSymbol_Type type, wxPlotPen_Type colour_type, int width , int height,
                            const wxPen *pen, const wxBrush *brush)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));

    switch (colour_type)
    {
        case wxPLOTPEN_ACTIVE :
            M_PLOTDATA->m_activeSymbol = CreateSymbol(type, colour_type, width, height, pen, brush);
            break;
        case wxPLOTPEN_SELECTED :
            M_PLOTDATA->m_selectedSymbol = CreateSymbol(type, colour_type, width, height, pen, brush);
            break;
        default : //case wxPLOTPEN_NORMAL :
            M_PLOTDATA->m_normalSymbol = CreateSymbol(type, colour_type, width, height, pen, brush);
            break;
    }
}

wxBitmap wxPlotData::CreateSymbol(wxPlotSymbol_Type type, wxPlotPen_Type colour_type, int width, int height,
                                   const wxPen *pen, const wxBrush *brush)
{
    wxBitmap b(width, height);

    wxMemoryDC mdc;
    mdc.SelectObject(b);
    mdc.SetPen(*wxWHITE_PEN);
    mdc.SetBrush(*wxWHITE_BRUSH);
    mdc.DrawRectangle(0, 0, width, height);
/*
    if (pen)
        mdc.SetPen(*pen);
    else
        mdc.SetPen(GetNormalPen());
*/
    switch (type)
    {
        case wxPLOTSYMBOL_ELLIPSE :
        {
            mdc.DrawEllipse(width/2, height/2, width/2, height/2);
            break;
        }
        case wxPLOTSYMBOL_RECTANGLE :
        {
            mdc.DrawRectangle(0, 0, width, height);
            break;
        }
        case wxPLOTSYMBOL_CROSS :
        {
            mdc.DrawLine(0, 0, width , height);
            mdc.DrawLine(0, height, width, 0);
            break;
        }
        case wxPLOTSYMBOL_PLUS :
        {
            mdc.DrawLine(0, height/2, width, height/2);
            mdc.DrawLine(width/2, 0, width/2, height);
            break;
        }

        default : break;
    }

    b.SetMask(new wxMask(b, *wxWHITE));

    return b;
}


wxRect2DDouble wxPlotData::GetBoundingRect() const
{
    wxCHECK_MSG(Ok(), wxNullPlotBounds, wxT("invalid plotcurve"));
    return M_PLOTCURVEDATA->m_boundingRect;
}
void wxPlotData::SetBoundingRect(const wxRect2DDouble &rect)
{
    wxCHECK_RET(Ok(), wxT("invalid plotcurve"));
    M_PLOTCURVEDATA->m_boundingRect = rect;
}


//----------------------------------------------------------------------------
// Get/Set Pen
//----------------------------------------------------------------------------

wxPen wxPlotData::GetPen(wxPlotPen_Type colour_type) const
{
    wxCHECK_MSG(Ok(), wxPen(), wxT("invalid plotcurve"));
    wxCHECK_MSG((colour_type >= 0) && (colour_type < (int)M_PLOTCURVEDATA->m_pens.GetCount()), wxPen(), wxT("invalid plot colour"));

    return M_PLOTCURVEDATA->m_pens[colour_type];
}

void wxPlotData::SetPen(wxPlotPen_Type colour_type, const wxPen &pen)
{
    wxCHECK_RET(Ok(), wxT("invalid plotcurve"));
    wxCHECK_RET((colour_type >= 0) && (colour_type < (int)M_PLOTCURVEDATA->m_pens.GetCount()), wxT("invalid plot colour"));

    M_PLOTCURVEDATA->m_pens[colour_type] = pen;
}

wxPen wxPlotData::GetDefaultPen(wxPlotPen_Type colour_type)
{
    InitPlotCurveDefaultPens();
    wxCHECK_MSG((colour_type >= 0) && (colour_type < int(wxPlotRefData::sm_defaultPens.GetCount())), wxPen(), wxT("invalid plot colour"));
    return wxPlotRefData::sm_defaultPens[colour_type];
}

void wxPlotData::SetDefaultPen(wxPlotPen_Type colour_type, const wxPen &pen)
{
    InitPlotCurveDefaultPens();
    wxCHECK_RET((colour_type >= 0) && (colour_type < int(wxPlotRefData::sm_defaultPens.GetCount())), wxT("invalid plot colour"));
    wxPlotRefData::sm_defaultPens[colour_type] = pen;
}


void wxPlotData::SetClientObject(wxClientData *data)
{
    wxCHECK_RET(M_PLOTCURVEDATA, wxT("invalid plotcurve"));
    M_PLOTCURVEDATA->SetClientObject(data);
}
wxClientData *wxPlotData::GetClientObject() const
{
    wxCHECK_MSG(M_PLOTCURVEDATA, NULL, wxT("invalid plotcurve"));
    return M_PLOTCURVEDATA->GetClientObject();
}
void wxPlotData::SetClientData(void *data)
{
    wxCHECK_RET(M_PLOTCURVEDATA, wxT("invalid plotcurve"));
    M_PLOTCURVEDATA->SetClientData(data);
}
void *wxPlotData::GetClientData() const
{
    wxCHECK_MSG(M_PLOTCURVEDATA, NULL, wxT("invalid plotcurve"));
    return M_PLOTCURVEDATA->GetClientData();
}



// ----------------------------------------------------------------------------
// Functions for the wxClipboard
// ----------------------------------------------------------------------------
#include "wx/clipbrd.h"
#if wxUSE_DATAOBJ && wxUSE_CLIPBOARD

const wxChar* wxDF_wxPlotData     = wxT("wxDF_wxPlotData");

static wxPlotData s_clipboardwxPlotData;    // temp storage of clipboard data
static wxString s_clipboardwxPlotData_data; // holds wxNow() to match clipboard data

#include "wx/module.h"
class wxPlotDataModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxPlotDataModule)
public:
    wxPlotDataModule() : wxModule() {}
    bool OnInit()
    {
        return true;
    }
    void OnExit()
    {
        s_clipboardwxPlotData.Destroy();
    }
};
IMPLEMENT_DYNAMIC_CLASS(wxPlotDataModule, wxModule)

wxPlotData wxClipboardGetPlotData()
{
    bool is_opened = wxTheClipboard->IsOpened();
    wxPlotData plotData;

    if (is_opened || wxTheClipboard->Open())
    {
        wxPlotDataObject plotDataObject;
        if (wxTheClipboard->IsSupported(wxDataFormat(wxDF_wxPlotData)) &&
            wxTheClipboard->GetData(plotDataObject) &&
            (plotDataObject.GetText() == s_clipboardwxPlotData_data))
        {
            plotData.Copy(plotDataObject.GetPlotData(), true);
        }

        if (!is_opened)
            wxTheClipboard->Close();
    }

    return plotData;
}
bool wxClipboardSetPlotData(const wxPlotData& plotData)
{
    wxCHECK_MSG(plotData.Ok(), false, wxT("Invalid wxPlotData to copy to clipboard"));
    bool is_opened = wxTheClipboard->IsOpened();

    if (is_opened || wxTheClipboard->Open())
    {
        wxPlotDataObject *plotDataObject = new wxPlotDataObject(plotData);
        bool ret = wxTheClipboard->SetData(plotDataObject);

        if (!is_opened)
            wxTheClipboard->Close();

        return ret;
    }

    return false;
}

// ----------------------------------------------------------------------------
// wxPlotDataObject Clipboard object
// ----------------------------------------------------------------------------

wxPlotDataObject::wxPlotDataObject() : wxTextDataObject()
{
    SetFormat(wxDataFormat(wxDF_wxPlotData));
}
wxPlotDataObject::wxPlotDataObject(const wxPlotData& plotData) : wxTextDataObject()
{
    SetFormat(wxDataFormat(wxDF_wxPlotData));
    SetPlotData(plotData);
}
wxPlotData wxPlotDataObject::GetPlotData() const
{
    return s_clipboardwxPlotData;
}
void wxPlotDataObject::SetPlotData(const wxPlotData& plotData)
{
    s_clipboardwxPlotData_data = wxNow();
    SetText(s_clipboardwxPlotData_data);

    if (plotData.Ok())
        s_clipboardwxPlotData.Copy(plotData, true);
    else
        s_clipboardwxPlotData.Destroy();
}

#endif // wxUSE_DATAOBJ && wxUSE_CLIPBOARD
