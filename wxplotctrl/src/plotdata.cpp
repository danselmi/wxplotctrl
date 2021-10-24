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

    wxRect2DDouble boundingRect_; // bounds the curve or part to draw
                                   // if width or height <= 0 then no bounds
    wxArrayPen pens_;
    static wxArrayPen defaultPens_;

    int     count_;

    double *xs_;
    double *ys_;
    bool    static_;

    wxBitmap normalSymbol_,
             activeSymbol_,
             selectedSymbol_;
};

#define M_PLOTCURVEDATA ((wxPlotRefData*)m_refData)

wxArrayPen wxPlotRefData::defaultPens_;

void InitPlotCurveDefaultPens()
{
    static bool initDefaultPens = false;
    if (!initDefaultPens)
    {
        initDefaultPens = true;
        wxPlotRefData::defaultPens_.Add(wxPen(wxColour(  0, 0,   0), 1, wxPENSTYLE_SOLID));
        wxPlotRefData::defaultPens_.Add(wxPen(wxColour(  0, 0, 255), 1, wxPENSTYLE_SOLID));
        wxPlotRefData::defaultPens_.Add(wxPen(wxColour(255, 0,   0), 1, wxPENSTYLE_SOLID));
    }
}

wxPlotRefData::wxPlotRefData():
    count_(0),
    xs_(nullptr),
    ys_(nullptr),
    static_(false)
{
    InitPlotCurveDefaultPens();
    pens_ = defaultPens_;

    normalSymbol_   = wxPlotSymbolNormal;
    activeSymbol_   = wxPlotSymbolActive;
    selectedSymbol_ = wxPlotSymbolSelected;
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
    if (!static_)
    {
        if (xs_) delete[]xs_;
        if (ys_) delete[]ys_;
    }

    count_ = 0;
    xs_ = nullptr;
    ys_ = nullptr;
}

void wxPlotRefData::CopyData(const wxPlotRefData &source)
{
    Destroy();

    count_  = source.count_;
    static_ = false; // we're creating our own copy

    if (count_ && source.xs_)
    {
        xs_ = new double[count_];
        memcpy(xs_, source.xs_, count_*sizeof(double));
    }
    if (count_ && source.ys_)
    {
        ys_ = new double[count_];
        memcpy(ys_, source.ys_, count_*sizeof(double));
    }
}

void wxPlotRefData::CopyExtra(const wxPlotRefData &source)
{
    normalSymbol_   = source.normalSymbol_;
    activeSymbol_   = source.activeSymbol_;
    selectedSymbol_ = source.selectedSymbol_;

    boundingRect_ = source.boundingRect_;
    pens_         = source.pens_;
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

wxPlotData::wxPlotData():
    wxObject()
{}

wxPlotData::wxPlotData(const wxPlotData& plotData):
    wxObject()
{
    Create(plotData);
}

wxPlotData::wxPlotData(int points, bool zero):
    wxObject()
{
    Create(points, zero);
}

wxPlotData::wxPlotData(double *xs, double *ys, int points, bool staticData):
    wxObject()
{
    Create(xs, ys, points, staticData);
}

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
    return m_refData && (M_PLOTDATA->count_ > 0);
}

void wxPlotData::Destroy()
{
    UnRef();
}

int wxPlotData::GetCount() const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->count_;
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

    M_PLOTDATA->count_ = points;
    M_PLOTDATA->xs_ = new double[points];
    M_PLOTDATA->ys_ = new double[points];
    if (!M_PLOTDATA->xs_ || !M_PLOTDATA->ys_)
    {
        UnRef();
        wxFAIL_MSG(wxT("memory allocation error creating plot"));
        return false;
    }

    if (zero)
    {
        memset(M_PLOTDATA->xs_, 0, points*sizeof(double));
        memset(M_PLOTDATA->ys_, 0, points*sizeof(double));
    }

    return true;
}

bool wxPlotData::Create(double *xs, double *ys, int points, bool staticData)
{
    wxCHECK_MSG((points > 0) && xs && ys, false,
                wxT("Can't create wxPlotData with < 1 points or invalid data"));

    UnRef();
    m_refData = new wxPlotRefData();

    if (!M_PLOTDATA)
    {
        wxFAIL_MSG(wxT("memory allocation error creating plot"));
        return false;
    }

    M_PLOTDATA->xs_  = xs;
    M_PLOTDATA->ys_  = ys;
    M_PLOTDATA->count_  = points;
    M_PLOTDATA->static_ = staticData;

    CalcBoundingRect();
    return true;
}

bool wxPlotData::Copy(const wxPlotData &source, bool copyAll)
{
    wxCHECK_MSG(source.Ok(), false, wxT("Invalid wxPlotData"));

    int count = source.GetCount();

    if (!Create(count, false)) return false;

    memcpy(M_PLOTDATA->xs_, ((wxPlotRefData *)source.m_refData)->xs_, count*sizeof(double));
    memcpy(M_PLOTDATA->ys_, ((wxPlotRefData *)source.m_refData)->ys_, count*sizeof(double));

    if (copyAll)
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

    M_PLOTDATA->boundingRect_ = wxNullPlotBounds;

    double *xs = M_PLOTDATA->xs_,
           *ys = M_PLOTDATA->ys_;

    double x = *xs,
           y = *ys,
           xmin = x,
           xmax = x,
           ymin = y,
           ymax = y,
           xlast = x;

    bool valid = false;

    int i, count = M_PLOTDATA->count_;

    for (i=0; i<count; i++)
    {
        x = *xs++;
        y = *ys++;

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
        M_PLOTDATA->boundingRect_ = wxRect2DDouble(xmin, ymin, xmax-xmin, ymax-ymin);
    else
        M_PLOTDATA->boundingRect_ = wxNullPlotBounds;
}

double *wxPlotData::GetXData() const
{
    wxCHECK_MSG(Ok(), (double*)NULL, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->xs_;
}
double *wxPlotData::GetYData() const
{
    wxCHECK_MSG(Ok(), (double*)NULL, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->ys_;
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
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->count_), 0.0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->xs_[index];
}
double wxPlotData::GetYValue(int index) const
{
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->count_), 0.0, wxT("Invalid wxPlotData"));
    return M_PLOTDATA->ys_[index];
}
wxPoint2DDouble wxPlotData::GetPoint(int index) const
{
    wxCHECK_MSG(Ok() && (index < M_PLOTDATA->count_), wxPoint2DDouble(0,0), wxT("Invalid wxPlotData"));
    return wxPoint2DDouble(M_PLOTDATA->xs_[index], M_PLOTDATA->ys_[index]);
}

double wxPlotData::GetY(double x) const
{
    wxCHECK_MSG(Ok(), 0, wxT("invalid wxPlotData"));

    int i = GetIndexFromX(x, IndexType::floor);

    if (M_PLOTDATA->xs_[i] == x)
        return M_PLOTDATA->ys_[i];

    if (i >= M_PLOTDATA->count_ - 1)
        return M_PLOTDATA->ys_[i];

    int i1 = GetIndexFromX(x, IndexType::ceil);

    double y0 = M_PLOTDATA->ys_[i];
    double y1 = M_PLOTDATA->ys_[i1];

    if (y0 == y1)
        return y0;

    return LinearInterpolateY(M_PLOTDATA->xs_[i], y0,
                               M_PLOTDATA->xs_[i1], y1, x);
}

int wxPlotData::GetIndexFromX(double x, wxPlotData::IndexType type) const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));

    int count = M_PLOTDATA->count_;
    double *x_data = M_PLOTDATA->xs_;

    int i;
    int index = 0, indexLower = 0, indexHigher = 0;
    double closest = fabs(x - *x_data++);

    for (i=1; i<count; i++)
    {
        if (fabs(x - *x_data) < closest)
        {
            if (x == *x_data) return i;

            closest = fabs(x - *x_data);
            index = i;

            if (x > *x_data)
                indexLower = i;
            else
                indexHigher = i;
        }
        x_data++;
    }

    // out of bounds so just return the closest
    if ((x < M_PLOTDATA->boundingRect_.GetLeft()) ||
        (x > M_PLOTDATA->boundingRect_.GetRight()))
        return index;

    if (type == IndexType::floor) return indexLower;
    if (type == IndexType::ceil) return indexHigher;
    return index;
}

int wxPlotData::GetIndexFromY(double y, wxPlotData::IndexType type) const
{
    wxCHECK_MSG(Ok(), 0, wxT("Invalid wxPlotData"));

    int i;
    int index = 0, indexLower = 0, indexHigher = 0;
    int count = M_PLOTDATA->count_;
    double *ys = M_PLOTDATA->ys_;
    double closest = fabs(y - *ys++);

    for (i=1; i<count; i++)
    {
        if (fabs(y - *ys) < closest)
        {
            if (y == *ys) return i;

            closest = fabs(y - *ys);
            index = i;

            if (y > *ys)
                indexLower = i;
            else
                indexHigher = i;
        }
        ys++;
    }

    // out of bounds so just return the closest
    if ((y < M_PLOTDATA->boundingRect_.GetLeft()) ||
        (y > M_PLOTDATA->boundingRect_.GetRight()))
        return index;

    if (type == IndexType::floor)
        return indexLower;
    if (type == IndexType::ceil)
        return indexHigher;
    return index;
}

int wxPlotData::GetIndexFromXY(double x, double y, double x_range) const
{
    wxCHECK_MSG(Ok() && (x_range >= 0), 0, wxT("Invalid wxPlotData"));

    int start = 1, end = M_PLOTDATA->count_ - 1;

    int i, index = start - 1;

    double *xs = &M_PLOTDATA->xs_[index];
    double *ys = &M_PLOTDATA->ys_[index];

    double xdiff = (*xs++) - x;
    double ydiff = (*ys++) - y;
    double diff = xdiff*xdiff + ydiff*ydiff;
    double min_diff = diff;

    double x_lower = x - x_range, x_higher = x + x_range;

    for (i=start; i<=end; i++)
    {
        if ((x_range != 0) && ((*xs < x_lower) || (*xs > x_higher)))
        {
            xs++;
            ys++;
            continue;
        }

        xdiff = (*xs++) - x;
        ydiff = (*ys++) - y;
        diff = xdiff*xdiff + ydiff*ydiff;

        if (diff < min_diff)
        {
            min_diff = diff;
            index = i;
        }
    }

    return index;
}

//----------------------------------------------------------------------------
// Get/Set bitmap symbol -- FIXME - this is NOT FINISHED OR WORKING
//----------------------------------------------------------------------------

wxBitmap wxPlotData::GetSymbol(PenColorType type) const
{
    wxCHECK_MSG(Ok(), M_PLOTDATA->normalSymbol_, wxT("Invalid wxPlotData"));

    switch (type)
    {
        case PenColorType::ACTIVE:
            return M_PLOTDATA->activeSymbol_;
        case PenColorType::SELECTED:
            return M_PLOTDATA->selectedSymbol_;
        //case PenColorType::NORMAL:
        default:
            break;
    }

    return M_PLOTDATA->normalSymbol_;
}

void wxPlotData::SetSymbol(const wxBitmap &bitmap, PenColorType type)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));
    wxCHECK_RET(bitmap.Ok(), wxT("Invalid bitmap"));

    switch (type)
    {
    case PenColorType::ACTIVE:
            M_PLOTDATA->activeSymbol_ = bitmap;
            break;
        case PenColorType::SELECTED:
            M_PLOTDATA->selectedSymbol_ = bitmap;
            break;
        //case PenColorType:::NORMAL:
        default:
            M_PLOTDATA->normalSymbol_ = bitmap;
            break;
    }
}
void wxPlotData::SetSymbol(Symbol symbol, PenColorType type, int width , int height,
                            const wxPen *pen, const wxBrush *brush)
{
    wxCHECK_RET(Ok(), wxT("Invalid wxPlotData"));

    switch (type)
    {
        case PenColorType::ACTIVE:
            M_PLOTDATA->activeSymbol_ = CreateSymbol(symbol, type, width, height, pen, brush);
            break;
        case PenColorType::SELECTED:
            M_PLOTDATA->selectedSymbol_ = CreateSymbol(symbol, type, width, height, pen, brush);
            break;
        //case PenColorType:::NORMAL:
        default:
            M_PLOTDATA->normalSymbol_ = CreateSymbol(symbol, type, width, height, pen, brush);
            break;
    }
}

wxBitmap wxPlotData::CreateSymbol(Symbol symbol, PenColorType type, int width, int height,
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
    switch (symbol)
    {
        case Symbol::ELLIPSE:
        {
            mdc.DrawEllipse(width/2, height/2, width/2, height/2);
            break;
        }
        case Symbol::RECTANGLE:
        {
            mdc.DrawRectangle(0, 0, width, height);
            break;
        }
        case Symbol::CROSS:
        {
            mdc.DrawLine(0, 0, width , height);
            mdc.DrawLine(0, height, width, 0);
            break;
        }
        case Symbol::PLUS:
        {
            mdc.DrawLine(0, height/2, width, height/2);
            mdc.DrawLine(width/2, 0, width/2, height);
            break;
        }

        default:
            break;
    }

    b.SetMask(new wxMask(b, *wxWHITE));

    return b;
}


wxRect2DDouble wxPlotData::GetBoundingRect() const
{
    wxCHECK_MSG(Ok(), wxNullPlotBounds, wxT("invalid plotcurve"));
    return M_PLOTCURVEDATA->boundingRect_;
}
void wxPlotData::SetBoundingRect(const wxRect2DDouble &rect)
{
    wxCHECK_RET(Ok(), wxT("invalid plotcurve"));
    M_PLOTCURVEDATA->boundingRect_ = rect;
}


//----------------------------------------------------------------------------
// Get/Set Pen
//----------------------------------------------------------------------------

size_t PenColorType2Uint(wxPlotData::PenColorType type)
{
    if (type == wxPlotData::PenColorType::NORMAL)
        return 0;
    else if (type == wxPlotData::PenColorType::ACTIVE)
        return 1;
    //else if (type == wxPlotData::PenColorType::SELECTED)
        return 2;
    return 2;
}

wxPen wxPlotData::GetPen(PenColorType type) const
{
    wxCHECK_MSG(Ok(), wxPen(), wxT("invalid plotcurve"));

    return M_PLOTCURVEDATA->pens_[PenColorType2Uint(type)];
}

void wxPlotData::SetPen(PenColorType type, const wxPen &pen)
{
    wxCHECK_RET(Ok(), wxT("invalid plotcurve"));

    M_PLOTCURVEDATA->pens_[PenColorType2Uint(type)] = pen;
}

wxPen wxPlotData::GetDefaultPen(PenColorType type)
{
    InitPlotCurveDefaultPens();
    return wxPlotRefData::defaultPens_[PenColorType2Uint(type)];
}

void wxPlotData::SetDefaultPen(PenColorType type, const wxPen &pen)
{
    InitPlotCurveDefaultPens();
    wxPlotRefData::defaultPens_[PenColorType2Uint(type)] = pen;
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
    wxPlotDataModule():
        wxModule()
    {}
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
