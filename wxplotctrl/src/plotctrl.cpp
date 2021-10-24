/////////////////////////////////////////////////////////////////////////////
// Name:        plotctrl.cpp
// Purpose:     wxPlotCtrl
// Author:      John Labenski, Robert Roebling
// Modified by:
// Created:     8/27/2002
// Copyright:   (c) John Labenski, Robert Roebling
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <float.h>
#include <limits.h>

#include "wx/panel.h"
#include "wx/scrolbar.h"
#include "wx/event.h"
#include "wx/timer.h"
#include "wx/dcmemory.h"
#include "wx/msgdlg.h"
#include "wx/geometry.h"
#include "wx/sizer.h"
#include "wx/dcclient.h"
#include "wx/dcscreen.h"
#include "wx/textctrl.h"
#include "wx/splitter.h"
#include "wx/math.h"
#include "wx/image.h"

#include "wx/plotctrl/plotctrl.h"
#include "wx/plotctrl/plotdraw.h"


// MSVC hogs global namespace with these min/max macros - remove them
#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

#define MAX_PLOT_ZOOMS 5
#define TIC_STEPS 3

std::numeric_limits<wxDouble> wxDouble_limits;
const wxDouble wxPlot_MIN_DBL   = wxDouble_limits.min()*10;
const wxDouble wxPlot_MAX_DBL   = wxDouble_limits.max()/10;
const wxDouble wxPlot_MAX_RANGE = wxDouble_limits.max()/5;

// Draw borders around the axes, title, and labels for sizing testing
//#define DRAW_BORDERS

#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxArrayPoint2DDouble);
WX_DEFINE_OBJARRAY(wxArrayRect2DDouble);
WX_DEFINE_OBJARRAY(wxArrayPlotData);

#include "../art/ledgrey.xpm"
#include "../art/ledgreen.xpm"

#include "../art/hand.xpm"
#include "../art/grab.xpm"

static wxCursor s_handCursor;
static wxCursor s_grabCursor;

// same as wxPlotRect2DDouble::Contains, but doesn't convert to wxPoint2DDouble
inline bool wxPlotRect2DDoubleContains(double x, double y, const wxRect2DDouble &rect)
{
    return ((x>=rect.m_x) && (y>=rect.m_y) && (x<=rect.GetRight()) && (y<=rect.GetBottom()));
}

//----------------------------------------------------------------------------
// Event types
//----------------------------------------------------------------------------

// wxPlotCtrlEvent
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_ADD_CURVE)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_DELETING_CURVE)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_DELETED_CURVE)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_MOTION)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_CLICKED)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_DOUBLECLICKED)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_CLICKED)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_DOUBLECLICKED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_ERROR)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_TITLE_EDIT)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_END_TITLE_EDIT)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_BOTTOM_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_LEFT_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGED)

// wxPlotCtrlSelEvent
//DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATING)
//DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATED)
//DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED)

/*
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CREATING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CREATED)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CHANGED)

DEFINE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_CREATE)
*/

// The code below translates the event.GetEventType to a string name for debugging
#define aDEFINE_LOCAL_EVENT_TYPE(t) if (eventType == t) return wxString(wxT(#t));

wxString wxPlotCtrlEvent::GetEventName(wxEventType eventType)
{
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_ADD_CURVE)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_DELETING_CURVE)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_DELETED_CURVE)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGED)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_MOTION)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_CLICKED)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_DOUBLECLICKED)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_CLICKED)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_DOUBLECLICKED)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATED)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGED)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGED)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_ERROR)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_TITLE_EDIT)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_END_TITLE_EDIT)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_BOTTOM_LABEL_EDIT)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_LEFT_LABEL_EDIT)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT)

    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGED)

    // wxPlotCtrlSelEvent
    //DEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATING)
    //DEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATED)
    //DEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGING)
    aDEFINE_LOCAL_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED)

    return wxT("Unknown Event Type");
}

// IDs for the wxPlotCtrl children windows
namespace {
    int ID_BOTTOM_AXIS = wxNewId();
    int ID_LEFT_AXIS = wxNewId();
    int ID_AREA = wxNewId();
    int ID_X_SCROLLBAR = wxNewId();
    int ID_Y_SCROLLBAR = wxNewId();

    // Start the mouse timer with the win_id, stops old if for different id
    int ID_AREA_TIMER = wxNewId();
    int ID_XAXIS_TIMER = wxNewId();
    int ID_YAXIS_TIMER = wxNewId();
// Redraw parts or all of the windows
enum RedrawNeed
{
    REDRAW_NONE        = 0x000,  // do nothing
    REDRAW_PLOT        = 0x001,  // redraw only the plot area
    REDRAW_BOTTOM_AXIS = 0x002,  // redraw x-axis, combine w/ redraw_plot
    REDRAW_LEFT_AXIS   = 0x004,  // redraw y-axis, combine w/ redraw_plot


    REDRAW_WINDOW      = 0x020,  // wxPlotCtrl container window
    REDRAW_WHOLEPLOT   = REDRAW_PLOT | REDRAW_BOTTOM_AXIS | REDRAW_LEFT_AXIS,
    REDRAW_EVERYTHING  = REDRAW_WHOLEPLOT | REDRAW_WINDOW,
    REDRAW_BLOCKER     = 0x100   // don't let OnPaint redraw, used internally
};

}

//-----------------------------------------------------------------------------
// wxPlotCtrlEvent
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotCtrlEvent, wxNotifyEvent)

wxPlotCtrlEvent::wxPlotCtrlEvent(wxEventType commandType, wxWindowID id, wxPlotCtrl *window):
    wxNotifyEvent(commandType, id), curve_(nullptr), curveIndex_(-1),
                  curveDataIndex_(-1), mouseFunc_(wxPlotCtrl::MouseFunction::NOTHING), x_(0.0), y_(0.0)
{
    SetEventObject((wxObject*)window);
}

//-----------------------------------------------------------------------------
// wxPlotCtrlSelEvent
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotCtrlSelEvent, wxPlotCtrlEvent)

wxPlotCtrlSelEvent::wxPlotCtrlSelEvent(wxEventType commandType, wxWindowID id, wxPlotCtrl *window):
    wxPlotCtrlEvent(commandType, id, window),
    selecting_(false)
{}

//-----------------------------------------------------------------------------
// wxPlotCtrlArea
//-----------------------------------------------------------------------------
IMPLEMENT_CLASS(wxPlotCtrl::Area, wxWindow)

BEGIN_EVENT_TABLE(wxPlotCtrl::Area, wxWindow)
    EVT_ERASE_BACKGROUND(wxPlotCtrl::Area::OnEraseBackground)
    EVT_PAINT           (wxPlotCtrl::Area::OnPaint)
    EVT_MOUSE_EVENTS    (wxPlotCtrl::Area::OnMouse)
    EVT_CHAR            (wxPlotCtrl::Area::OnChar)
    EVT_KEY_DOWN        (wxPlotCtrl::Area::OnKeyDown)
    EVT_KEY_UP          (wxPlotCtrl::Area::OnKeyUp)
END_EVENT_TABLE()

wxPlotCtrl::Area::Area(wxPlotCtrl *parent, wxWindowID id):
    wxWindow(),
    host_(parent)
{
    Create(parent, id);
}

bool wxPlotCtrl::Area::Create(wxPlotCtrl *parent, wxWindowID id)
{
    if (!wxWindow::Create(parent, id, wxDefaultPosition, wxSize(100,100),
                  wxNO_BORDER  |wxWANTS_CHARS | wxCLIP_CHILDREN))
        return false;

    SetSizeHints(4, 4); // Don't allow window to get smaller than this!

    return true;
}

void wxPlotCtrl::Area::OnChar(wxKeyEvent &event)
{
    if (host_)
        host_->ProcessAreaCharEvent(event);
}
void wxPlotCtrl::Area::OnKeyDown(wxKeyEvent &event)
{
    if (host_)
        host_->ProcessAreaKeyDownEvent(event);
}
void wxPlotCtrl::Area::OnKeyUp(wxKeyEvent &event)
{
    if (host_)
        host_->ProcessAreaKeyUpEvent(event);
}
void wxPlotCtrl::Area::OnMouse(wxMouseEvent &event)
{
    if (host_)
        host_->ProcessAreaMouseEvent(event);
}

void wxPlotCtrl::Area::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

    int redrawNeed = host_->GetRedrawNeeds();

    if ((redrawNeed & REDRAW_BLOCKER) != 0)
        return;
/*
    wxRegionIterator upd(GetUpdateRegion());
    while (upd)
    {
        //printf("Region %d %d %d %d \n", upd.GetX(), upd.GetY(), upd.GetWidth(), upd.GetHeight()); fflush(stdout);
        Paint(&dc, upd.GetRect());
        upd++;
    }
*/

    if (redrawNeed & REDRAW_PLOT)
    {
        CreateBitmap(host_->GetPlotAreaRect());
        host_->SetRedrawNeeds(redrawNeed & ~REDRAW_PLOT);
    }

    if (bitmap_.Ok())
        dc.DrawBitmap(bitmap_, 0, 0, false);

    if (host_->GetCrossHairCursor() && host_->GetPlotAreaRect().Contains(lastMousePosition_))
        host_->DrawCrosshairCursor(&dc, lastMousePosition_);

    host_->DrawMouseMarker(&dc, host_->GetAreaMouseMarker(), mouseDragRectangle_);
}

void wxPlotCtrl::Area::CreateBitmap(const wxRect &rect)
{
    wxRect refreshRect(rect);
    wxRect clientRect(host_->GetPlotAreaRect());
    refreshRect.Intersect(clientRect);

    if ((refreshRect.width == 0) || (refreshRect.height == 0)) return;

    // if the bitmap need to be recreated then refresh everything
    if (!bitmap_.Ok() || (clientRect.width  != bitmap_.GetWidth()) ||
                          (clientRect.height != bitmap_.GetHeight()))
    {
        bitmap_.Create(clientRect.width, clientRect.height);
        refreshRect = clientRect;
    }

    wxMemoryDC mdc;
    mdc.SelectObject(bitmap_);
    host_->DrawAreaWindow(&mdc, refreshRect);
    mdc.SelectObject(wxNullBitmap);
}

//-----------------------------------------------------------------------------
// wxPlotCtrl::Axis
//-----------------------------------------------------------------------------
IMPLEMENT_CLASS(wxPlotCtrl::Axis, wxWindow)

BEGIN_EVENT_TABLE(wxPlotCtrl::Axis, wxWindow)
    EVT_ERASE_BACKGROUND(wxPlotCtrl::Axis::OnEraseBackground)
    EVT_PAINT           (wxPlotCtrl::Axis::OnPaint)
    EVT_MOUSE_EVENTS    (wxPlotCtrl::Axis::OnMouse)
    EVT_CHAR            (wxPlotCtrl::Axis::OnChar)
END_EVENT_TABLE()

wxPlotCtrl::Axis::Axis(wxPlotCtrl *parent, wxWindowID id):
    host_(parent)
{
    Create(parent, id);
}

bool wxPlotCtrl::Axis::Create(wxPlotCtrl *parent, wxWindowID id)
{
    if (!wxWindow::Create(parent, id, wxDefaultPosition, wxDefaultSize,
                   wxNO_BORDER | wxWANTS_CHARS | wxCLIP_CHILDREN))
        return false;

    SetSizeHints(4, 4); // Don't allow window to get smaller than this!

    if (GetId() == ID_LEFT_AXIS)
        SetCursor(wxCursor(wxCURSOR_SIZENS));
    else
        SetCursor(wxCursor(wxCURSOR_SIZEWE));

    return true;
}


void wxPlotCtrl::Axis::OnChar(wxKeyEvent &event)
{
    if (host_)
        host_->ProcessAxisCharEvent(event);
}
void wxPlotCtrl::Axis::OnMouse(wxMouseEvent &event)
{
    if (host_)
        host_->ProcessAxisMouseEvent(event);
}

void wxPlotCtrl::Axis::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);

    int redrawNeed = host_->GetRedrawNeeds();
    if ((redrawNeed & REDRAW_BLOCKER) != 0)
        return;

    if ((GetId() == ID_BOTTOM_AXIS) && (redrawNeed & REDRAW_BOTTOM_AXIS))
    {
        host_->SetRedrawNeeds(redrawNeed & ~REDRAW_BOTTOM_AXIS);
        CreateBitmap();
    }
    else if ((GetId() == ID_LEFT_AXIS) && (redrawNeed & REDRAW_LEFT_AXIS))
    {
        host_->SetRedrawNeeds(redrawNeed & ~REDRAW_LEFT_AXIS);
        CreateBitmap();
    }

    if (bitmap_.Ok())
        dc.DrawBitmap(bitmap_, 0, 0, false);
}

void wxPlotCtrl::Axis::CreateBitmap()
{
    host_->UpdateWindowSize();
    wxSize clientSize = GetClientSize();
    if ((clientSize.x < 2) || (clientSize.y < 2)) return;

    if (!bitmap_.Ok() || (clientSize.x != bitmap_.GetWidth()) ||
                          (clientSize.y != bitmap_.GetHeight()))
    {
        bitmap_.Create(clientSize.x, clientSize.y);
    }

    wxMemoryDC mdc;
    mdc.SelectObject(bitmap_);
    if (GetId() == ID_BOTTOM_AXIS)
        host_->DrawXAxis(&mdc, true);
    else
        host_->DrawYAxis(&mdc, true);

    mdc.SelectObject(wxNullBitmap);
}

bool wxPlotCtrl::Axis::IsXAxis() const
{
    return GetId() == ID_BOTTOM_AXIS;
}

//-----------------------------------------------------------------------------
// wxPlotCtrl
//-----------------------------------------------------------------------------
IMPLEMENT_ABSTRACT_CLASS(wxPlotCtrl, wxWindow)

BEGIN_EVENT_TABLE(wxPlotCtrl, wxWindow)
    //EVT_ERASE_BACKGROUND (wxPlotCtrl::OnEraseBackground) // clear for MSW
    EVT_SIZE             (wxPlotCtrl::OnSize)
    EVT_PAINT            (wxPlotCtrl::OnPaint)
    EVT_CHAR             (wxPlotCtrl::OnChar)
    EVT_SCROLL           (wxPlotCtrl::OnScroll)
    EVT_IDLE             (wxPlotCtrl::OnIdle)
    EVT_MOUSE_EVENTS     (wxPlotCtrl::OnMouse)
    EVT_TIMER            (wxID_ANY, wxPlotCtrl::OnTimer)

    EVT_TEXT_ENTER       (wxID_ANY, wxPlotCtrl::OnTextEnter)
END_EVENT_TABLE()

wxPlotCtrl::wxPlotCtrl():
    wxWindow(),

    activeCurve_(nullptr),
    activeIndex_(-1),

    cursorCurve_(-1),
    cursorIndex_(-1),

    selectionType_(SelectionType::MULTIPLE),

    showKey_(true),
    keyString_(""),

    showTitle_(false),
    title_("Title"),
    showBottomAxisLabel_(false),
    showLeftAxisLabel_(false),
    bottomAxisLabel_("X-Axis"),
    leftAxisLabel_("Y-Axis"),

    titleFont_(*wxSWISS_FONT),
    titleColour_(*wxBLACK),
    borderColour_(*wxBLACK),

    crosshairCursor_(false),
    drawSymbols_(true),
    drawLines_(true),
    drawSpline_(false),
    drawGrid_(true),
    drawTicks_(false),
    fitOnNewCurve_(true),
    showBottomAxis_(true),
    showLeftAxis_(true),

    zoom_(1.0, 1.0),
    historyViewsIndex_(-1),

    fixedAspectRatio_(false),
    aspectratio_(1.0),

    viewRect_(-10.0, -10.0, 20.0, 20.0),
    curveBoundingRect_(viewRect_),
    defaultPlotRect_(viewRect_),
    areaClientRect_(0, 0, 10, 10),

    bottomAxisTicks_{{}, {}, "%lf", 1.0, 4, true},
//        .format_("%lf"),
//        .step_(1.0),
//        .count_(4),
//        .correct_(true)},
    leftAxisTicks_{{}, {}, "%lf", 1.0, 4, true},
//        .format_("%lf"),
//        .step_(1.0),
//        .count_(4),
//        .correct_(true)},

    areaDrawer_(nullptr),
    bottomAxisDrawer_(nullptr),
    leftAxisDrawer_(nullptr),
    keyDrawer_(nullptr),
    dataCurveDrawer_(nullptr),
    markerDrawer_(nullptr),

    area_(nullptr),
    bottomAxis_(nullptr),
    leftAxis_(nullptr),
    xAxisScrollbar_(nullptr),
    yAxisScrollbar_(nullptr),

    textCtrl_(nullptr),

    activeBitmap_(ledgreen_xpm),
    inactiveBitmap_(ledgrey_xpm),
    focusedWin_(nullptr),
    greedyFocus_(false),

    redrawNeed_(REDRAW_BLOCKER),
    batchCount_(0),

    axisFontSize_(6, 12),
    leftAxisTextWidth_(60),
    areaBorderWidth_(1),
    border_(4),
    minExponential_(1000),
    penPrintWidth_(0.4),

    timer_(nullptr),
    winCapture_(nullptr),

    areaMouseFunc_(MouseFunction::ZOOM),
    areaMouseMarker_(MarkerType::RECT),
    areaMouseCursorid_(wxCURSOR_CROSS),

    mouseCursorid_(wxCURSOR_ARROW)
{
    cursorMarker_.CreateEllipseMarker(wxPoint2DDouble(0,0),
                                       wxSize(2, 2),
                                       wxPen(wxColour(0, 255, 0)));
}

wxPlotCtrl::wxPlotCtrl(wxWindow *parent, wxWindowID id, const wxPoint &pos,
                       const wxSize &size, const wxString &name)
:
    wxPlotCtrl()
{
    Create(parent, id, pos, size, name);
}

bool wxPlotCtrl::Create(wxWindow *parent, wxWindowID win_id,
                        const wxPoint &pos, const wxSize &size,
                        const wxString &name)
{
    redrawNeed_ = REDRAW_BLOCKER; // no paints until finished

    if (!wxWindow::Create(parent, win_id, pos,
                          wxSize(size.x > 20 ? size.x : 20, size.y > 20 ? size.y : 20),
                          wxWANTS_CHARS|wxCLIP_CHILDREN, name))
        return false;

    SetSizeHints(20, 20); // Don't allow window to get smaller than this!

    if (!s_handCursor.Ok())
    {
        wxImage image(wxBitmap(hand_xpm).ConvertToImage());
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, image.GetWidth()/2);
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, image.GetHeight()/2);
        s_handCursor = wxCursor(image);
    }
    if (!s_grabCursor.Ok())
    {
        wxImage image(wxBitmap(grab_xpm).ConvertToImage());
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, image.GetWidth()/2);
        image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, image.GetHeight()/2);
        s_grabCursor = wxCursor(image);
    }

    areaDrawer_       = new wxPlotDrawerArea(this);
    bottomAxisDrawer_ = new wxPlotDrawerXAxis(this, bottomAxisTicks_.positions_, bottomAxisTicks_.labels_);
    leftAxisDrawer_   = new wxPlotDrawerYAxis(this, leftAxisTicks_.positions_, leftAxisTicks_.labels_);
    keyDrawer_        = new wxPlotDrawerKey(this);
    dataCurveDrawer_  = new wxPlotDrawerDataCurve(this);
    markerDrawer_     = new wxPlotDrawerMarker(this);

    wxFont axisFont(GetFont());
    GetTextExtent(wxT("5"), &axisFontSize_.x, &axisFontSize_.y, NULL, NULL, &axisFont);
    if ((axisFontSize_.x < 2) || (axisFontSize_.y < 2)) // don't want to divide by 0
    {
        axisFontSize_.x = 6;
        axisFontSize_.y = 12;
        wxFAIL_MSG(wxT("Can't determine the font size for the axis! I'll guess.\n"
                       "The display might be corrupted, however you may continue."));
    }

    bottomAxisDrawer_->SetTickFont(axisFont);
    leftAxisDrawer_->SetTickFont(axisFont);

    bottomAxis_ = new Axis(this, ID_BOTTOM_AXIS);
    leftAxis_ = new Axis(this, ID_LEFT_AXIS);
    area_  = new Area(this, ID_AREA);
    xAxisScrollbar_ = new wxScrollBar(this, ID_X_SCROLLBAR,
                                       wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
    yAxisScrollbar_ = new wxScrollBar(this, ID_Y_SCROLLBAR,
                                       wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

    area_->SetCursor(wxCURSOR_CROSS);
    area_->SetBackgroundColour(*wxWHITE);
    bottomAxis_->SetBackgroundColour(*wxWHITE);
    leftAxis_->SetBackgroundColour(*wxWHITE);
    wxWindow::SetBackgroundColour(*wxWHITE);

    area_->SetForegroundColour(*wxLIGHT_GREY);

    // update the sizes of the title and axis labels
    SetPlotTitle(GetPlotTitle());
    SetBottomAxisLabel(GetBottomAxisLabel());
    SetLeftAxisLabel(GetLeftAxisLabel());

    Redraw(REDRAW_WHOLEPLOT); // redraw when all done

    return true;
}

wxPlotCtrl::~wxPlotCtrl()
{
    delete areaDrawer_;
    delete bottomAxisDrawer_;
    delete leftAxisDrawer_;
    delete keyDrawer_;
    delete dataCurveDrawer_;
    delete markerDrawer_;
}

void wxPlotCtrl::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);

    DrawActiveBitmap(&dc);
    DrawPlotCtrl(&dc);
}

void wxPlotCtrl::DrawActiveBitmap(wxDC *dc)
{
    wxCHECK_RET(dc, wxT("invalid dc"));

    if (xAxisScrollbar_ && yAxisScrollbar_ &&
        xAxisScrollbar_->IsShown() && yAxisScrollbar_->IsShown())
    {
        wxSize size = GetClientSize();
        int left = xAxisScrollbar_->GetRect().GetRight();
        int top  = yAxisScrollbar_->GetRect().GetBottom();
        wxRect rect(left, top, size.x - left, size.y - top);
        // clear background
        dc->SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->DrawRectangle(rect);
        // center the bitmap
        wxPoint pt(rect.x + (rect.width - 15)/2, rect.y + (rect.width - 15)/2);
        dc->DrawBitmap(focusedWin_ ? activeBitmap_ : inactiveBitmap_,
                       pt.x, pt.y, true);
    }
}
void wxPlotCtrl::DrawPlotCtrl(wxDC *dc)
{
    wxCHECK_RET(dc, wxT("invalid dc"));

    if (showTitle_ && !title_.IsEmpty())
    {
        dc->SetFont(GetPlotTitleFont());
        dc->SetTextForeground(GetPlotTitleColour());
        dc->DrawText(title_, titleRect_.x, titleRect_.y);
    }

    bool drawXLabel = (showBottomAxisLabel_ && !bottomAxisLabel_.IsEmpty());
    bool drawyLabel = (showLeftAxisLabel_ && !leftAxisLabel_.IsEmpty());

    if (drawXLabel || drawyLabel)
    {
        dc->SetFont(GetAxisLabelFont());
        dc->SetTextForeground(GetAxisLabelColour());

        if (drawXLabel)
            dc->DrawText(bottomAxisLabel_, bottomAxisLabelRect_.x, bottomAxisLabelRect_.y);
        if (drawyLabel)
            dc->DrawRotatedText(leftAxisLabel_, leftLabelRect_.x, leftLabelRect_.y + leftLabelRect_.height, 90);
    }

#ifdef DRAW_BORDERS
    // Test code for sizing to show the extent of the axes
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(wxPen(GetBorderColour(), 1, wxSOLID));
    dc->DrawRectangle(titleRect_);
    dc->DrawRectangle(bottomAxisLabelRect_);
    dc->DrawRectangle(leftLabelRect_);
#endif
}

void wxPlotCtrl::setPlotWinMouseCursor(wxStockCursor cursorid)
{
    if (cursorid == mouseCursorid_)
        return;
    mouseCursorid_ = cursorid;
    SetCursor(wxCursor(cursorid));
}

void wxPlotCtrl::OnMouse(wxMouseEvent &event)
{
    if (event.ButtonDown() && IsTextCtrlShown())
    {
        HideTextCtrl(true, true);
        return;
    }

    wxSize size(GetClientSize());
    wxPoint mousePt(event.GetPosition());

    if ((showTitle_  && titleRect_.Contains(mousePt)) ||
        (showBottomAxisLabel_ && bottomAxisLabelRect_.Contains(mousePt)) ||
        (showLeftAxisLabel_ && leftLabelRect_.Contains(mousePt)))
    {
        setPlotWinMouseCursor(wxCURSOR_IBEAM);
    }
    else
        setPlotWinMouseCursor(wxCURSOR_ARROW);

    if (event.ButtonDClick(1) && !IsTextCtrlShown())
    {
        if (showTitle_ && titleRect_.Contains(mousePt))
            ShowTextCtrl(TextCtrlType::EDIT_TITLE, true);
        else if (showBottomAxisLabel_ && bottomAxisLabelRect_.Contains(mousePt))
            ShowTextCtrl(TextCtrlType::EDIT_BOTTOM_AXIS_LABEL, true);
        else if (showLeftAxisLabel_ && leftLabelRect_.Contains(mousePt))
            ShowTextCtrl(TextCtrlType::EDIT_LEFT_AXIS_LABEL, true);
    }
}

void wxPlotCtrl::ShowTextCtrl(TextCtrlType type, bool sendEvent)
{
    switch (type)
    {
        case TextCtrlType::EDIT_TITLE:
        {
            if (textCtrl_)
            {
                if (textCtrl_->GetId() != wxEVT_PLOTCTRL_END_TITLE_EDIT)
                    HideTextCtrl(true, true);
                else
                    return; // already shown
            }

            if (sendEvent)
            {
                wxPlotCtrlEvent pevent(wxEVT_PLOTCTRL_BEGIN_TITLE_EDIT, GetId(), this);
                pevent.SetString(title_);
                if (!DoSendEvent(pevent))
                    return;
            }

            textCtrl_ = new wxTextCtrl(this, wxEVT_PLOTCTRL_END_TITLE_EDIT, GetPlotTitle(),
                                        wxPoint(areaRect_.x, 0),
                                        wxSize(areaRect_.width, titleRect_.height+2*border_),
                                        wxTE_PROCESS_ENTER);

            textCtrl_->SetFont(GetPlotTitleFont());
            textCtrl_->SetForegroundColour(GetPlotTitleColour());
            textCtrl_->SetBackgroundColour(GetBackgroundColour());
            break;
        }
        case TextCtrlType::EDIT_BOTTOM_AXIS_LABEL:
        {
            if (textCtrl_)
            {
                if (textCtrl_->GetId() != wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT)
                    HideTextCtrl(true, true);
                else
                    return; // already shown
            }

            if (sendEvent)
            {
                wxPlotCtrlEvent pevent(wxEVT_PLOTCTRL_BEGIN_BOTTOM_LABEL_EDIT, GetId(), this);
                pevent.SetString(bottomAxisLabel_);
                if (!DoSendEvent(pevent))
                    return;
            }

            textCtrl_ = new wxTextCtrl(this, wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT, GetBottomAxisLabel(),
                                        wxPoint(areaRect_.x, bottomAxisRect_.GetBottom()),
                                        wxSize(areaRect_.width, bottomAxisLabelRect_.height+2*border_),
                                        wxTE_PROCESS_ENTER);

            textCtrl_->SetFont(GetAxisLabelFont());
            textCtrl_->SetForegroundColour(GetAxisLabelColour());
            textCtrl_->SetBackgroundColour(GetBackgroundColour());
            break;
        }
        case TextCtrlType::EDIT_LEFT_AXIS_LABEL:
        {
            if (textCtrl_)
            {
                if (textCtrl_->GetId() != wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT)
                    HideTextCtrl(true, true);
                else
                    return; // already shown
            }

            if (sendEvent)
            {
                wxPlotCtrlEvent pevent(wxEVT_PLOTCTRL_BEGIN_LEFT_LABEL_EDIT, GetId(), this);
                pevent.SetString(leftAxisLabel_);
                if (!DoSendEvent(pevent))
                    return;
            }

            textCtrl_ = new wxTextCtrl(this, wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT, GetLeftAxisLabel(),
                                        wxPoint(0, areaRect_.y+areaRect_.height/2),
                                        wxSize(clientRect_.width - axisFontSize_.y/2, leftLabelRect_.width+2*border_),
                                        wxTE_PROCESS_ENTER);

            textCtrl_->SetFont(GetAxisLabelFont());
            textCtrl_->SetForegroundColour(GetAxisLabelColour());
            textCtrl_->SetBackgroundColour(GetBackgroundColour());
            break;
        }
    }
}

void wxPlotCtrl::HideTextCtrl(bool save_value, bool sendEvent)
{
    long event_type = textCtrl_->GetId();
    wxString value  = textCtrl_->GetValue();

    textCtrl_->Destroy();
    textCtrl_ = NULL;

    if (!save_value)
        return;

    bool changed = false;

    if (event_type == wxEVT_PLOTCTRL_END_TITLE_EDIT)
        changed = (value != GetPlotTitle());
    else if (event_type == wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT)
        changed = (value != GetBottomAxisLabel());
    else if (event_type == wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT)
        changed = (value != GetLeftAxisLabel());

    if (!changed)
        return;

    if (sendEvent)
    {
        wxPlotCtrlEvent event(event_type, GetId(), this);
        event.SetString(value);
        if (!DoSendEvent(event))
            return;
    }

    if (event_type == wxEVT_PLOTCTRL_END_TITLE_EDIT)
        SetPlotTitle(value);
    else if (event_type == wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT)
        SetBottomAxisLabel(value);
    else if (event_type == wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT)
        SetLeftAxisLabel(value);
}

bool wxPlotCtrl::IsTextCtrlShown() const
{
    return textCtrl_ && textCtrl_->IsShown();
}

void wxPlotCtrl::OnTextEnter(wxCommandEvent &event)
{
    // we send a fake event so that we can destroy the textctrl the second time
    if (event.GetId() == 1)
        HideTextCtrl(true, true);
    else
    {
        wxCommandEvent newevt(wxEVT_COMMAND_TEXT_ENTER, 1);
        GetEventHandler()->AddPendingEvent(newevt);
    }
}

void wxPlotCtrl::OnEraseBackground(wxEraseEvent &event)
{
    event.Skip(false);
}

void wxPlotCtrl::OnIdle(wxIdleEvent &event)
{
    CheckFocus();
    event.Skip();
}

bool wxPlotCtrl::CheckFocus()
{
    wxWindow *win = FindFocus();

    if (win == focusedWin_)
        return true;

    if ((win == area_)   ||
        (win == bottomAxis_) ||
        (win == leftAxis_) ||
        (win == this))
    {
        if (!focusedWin_)
        {
            focusedWin_ = win;
            wxClientDC dc(this);
            wxSize size = GetClientSize();
            dc.DrawBitmap(activeBitmap_, size.GetWidth()-15, size.GetHeight()-15,true);
        }
    }
    else if (focusedWin_)
    {
        focusedWin_ = NULL;
        wxClientDC dc(this);
        wxSize size = GetClientSize();
        dc.DrawBitmap(inactiveBitmap_, size.GetWidth()-15, size.GetHeight()-15,true);
    }
    return focusedWin_ != NULL;
}

void wxPlotCtrl::BeginBatch()
{
    batchCount_++;
}

void wxPlotCtrl::EndBatch(bool forceRefresh)
{
    if (batchCount_ > 0)
    {
        batchCount_--;
        if ((batchCount_ <= 0) && forceRefresh)
        {
            Redraw(REDRAW_WHOLEPLOT);
            AdjustScrollBars();
        }
    }
}

int wxPlotCtrl::GetBatchCount() const
{
    return batchCount_;
}

int wxPlotCtrl::GetRedrawNeeds() const
{
    return redrawNeed_;
}

void wxPlotCtrl::SetRedrawNeeds(int need)
{
    redrawNeed_ = need;
}

wxColour wxPlotCtrl::GetBackgroundColour() const
{
    return area_->GetBackgroundColour();
}
bool wxPlotCtrl::SetBackgroundColour(const wxColour &colour)
{
    wxCHECK_MSG(colour.Ok(), false, wxT("invalid colour"));
    area_->SetBackgroundColour(colour);
    bottomAxis_->SetBackgroundColour(colour);
    leftAxis_->SetBackgroundColour(colour);
    wxWindow::SetBackgroundColour(colour);

    Redraw(REDRAW_EVERYTHING);
    return true;
}
wxColour wxPlotCtrl::GetGridColour() const
{
    return area_->GetForegroundColour();
}
void wxPlotCtrl::SetGridColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    area_->SetForegroundColour(colour);
    Redraw(REDRAW_PLOT);
}
wxColour wxPlotCtrl::GetBorderColour() const
{
    return borderColour_;
}
void wxPlotCtrl::SetBorderColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    borderColour_ = colour;
    Redraw(REDRAW_PLOT);
}
void wxPlotCtrl::SetCursorColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    cursorMarker_.GetPen().SetColour(colour);
    wxClientDC dc(area_);
    DrawCurveCursor(&dc);
}
wxColour wxPlotCtrl::GetCursorColour() const
{
    return cursorMarker_.GetPen().GetColour();
}
int wxPlotCtrl::GetCursorSize() const
{
    return cursorMarker_.GetSize().x;
}
void wxPlotCtrl::SetCursorSize(int size)
{
    cursorMarker_.SetSize(wxSize(size, size));
}

wxFont wxPlotCtrl::GetAxisFont() const
{
    return bottomAxisDrawer_->tickFont_; // FIXME
}
wxColour wxPlotCtrl::GetAxisColour() const
{
    return bottomAxisDrawer_->tickColour_; // FIXME
}

void wxPlotCtrl::SetAxisFont(const wxFont &font)
{
    wxCHECK_RET(font.Ok(), wxT("invalid font"));

    if (bottomAxisDrawer_) bottomAxisDrawer_->SetTickFont(font);
    if (leftAxisDrawer_) leftAxisDrawer_->SetTickFont(font);

    int x=6, y=12, decent=0, leading=0;

    GetTextExtent(wxT("5"), &x, &y, &decent, &leading, &font);
    axisFontSize_.x = x+leading;
    axisFontSize_.y = y+decent;

    GetTextExtent(wxT("-5.5e+555"), &x, &y, &decent, &leading, &font);
    leftAxisTextWidth_ = x + leading;

    //axisFontSize_.x = bottomAxis_->GetCharWidth();
    //axisFontSize_.y = bottomAxis_->GetCharHeight();
    if ((axisFontSize_.x < 2) || (axisFontSize_.y < 2)) // don't want to divide by 0
    {
        static bool firstTry = false;

        axisFontSize_.x = 6;
        axisFontSize_.y = 12;
        wxMessageBox(wxT("Can't determine the font size for the axis.\n")
                     wxT("Reverting to a default font."),
                     wxT("Font error"), wxICON_ERROR, this);

        if (!firstTry)
        {
            firstTry = true;
            SetAxisFont(*wxNORMAL_FONT);
        }
        else
            firstTry = false;
    }

    DoSize();
    Redraw(REDRAW_BOTTOM_AXIS | REDRAW_LEFT_AXIS);
}
void wxPlotCtrl::SetAxisColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    if (bottomAxisDrawer_) bottomAxisDrawer_->SetTickColour(colour);
    if (leftAxisDrawer_) leftAxisDrawer_->SetTickColour(colour);
    Redraw(REDRAW_BOTTOM_AXIS | REDRAW_LEFT_AXIS);
}

wxFont wxPlotCtrl::GetAxisLabelFont() const
{
    return bottomAxisDrawer_->labelFont_; // FIXME
}
wxColour wxPlotCtrl::GetAxisLabelColour() const
{
    return bottomAxisDrawer_->labelColour_; // FIXME
}
void wxPlotCtrl::SetAxisLabelFont(const wxFont &font)
{
    wxCHECK_RET(font.Ok(), wxT("invalid font"));
    if (bottomAxisDrawer_) bottomAxisDrawer_->SetLabelFont(font);
    if (leftAxisDrawer_) leftAxisDrawer_->SetLabelFont(font);
    SetBottomAxisLabel(GetBottomAxisLabel());      // FIXME - lazy hack
    SetLeftAxisLabel(GetLeftAxisLabel());
}
void wxPlotCtrl::SetAxisLabelColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    if (bottomAxisDrawer_) bottomAxisDrawer_->SetLabelColour(colour);
    if (leftAxisDrawer_) leftAxisDrawer_->SetLabelColour(colour);
    SetBottomAxisLabel(GetBottomAxisLabel());      // FIXME - lazy hack
    SetLeftAxisLabel(GetLeftAxisLabel());
}

wxFont wxPlotCtrl::GetPlotTitleFont() const
{
    return titleFont_;
}
wxColour wxPlotCtrl::GetPlotTitleColour() const
{
    return titleColour_;
}
void wxPlotCtrl::SetPlotTitleFont(const wxFont &font)
{
    wxCHECK_RET(font.Ok(), wxT("invalid font"));
    titleFont_ = font;
    SetPlotTitle(GetPlotTitle());
}
void wxPlotCtrl::SetPlotTitleColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    titleColour_ = colour;
    SetPlotTitle(GetPlotTitle());
}

wxFont wxPlotCtrl::GetKeyFont() const
{
    return keyDrawer_->font_; // FIXME
}
wxColour wxPlotCtrl::GetKeyColour() const
{
    return keyDrawer_->fontColour_; // FIXME
}
void wxPlotCtrl::SetKeyFont(const wxFont &font)
{
    wxCHECK_RET(font.Ok(), wxT("invalid font"));
    keyDrawer_->SetFont(font);
    Redraw(REDRAW_PLOT);
}
void wxPlotCtrl::SetKeyColour(const wxColour &colour)
{
    wxCHECK_RET(colour.Ok(), wxT("invalid colour"));
    keyDrawer_->SetFontColour(colour);
    Redraw(REDRAW_PLOT);
}
void wxPlotCtrl::SetShowKey(bool show)
{
    showKey_ = show;
    Redraw(REDRAW_PLOT);
}

// ------------------------------------------------------------------------
// Title, axis labels, and key
// --------------------------------
void wxPlotCtrl::SetShowBottomAxis(bool show)
{
    showBottomAxis_ = show;
    DoSize();
}

void wxPlotCtrl::SetShowLeftAxis(bool show)
{
    showLeftAxis_ = show;
    DoSize();
}

bool wxPlotCtrl::GetShowBottomAxis()
{
    return showBottomAxis_;
}

bool wxPlotCtrl::GetShowLeftAxis()
{
    return showLeftAxis_;
}

// Get/Set and show/hide the axis labels
const wxString &wxPlotCtrl::GetBottomAxisLabel() const
{
    return bottomAxisLabel_;
}

const wxString &wxPlotCtrl::GetLeftAxisLabel() const
{
    return leftAxisLabel_;
}

void wxPlotCtrl::SetBottomAxisLabel(const wxString &label)
{
    if (label.IsEmpty())
        bottomAxisLabel_ = wxT("X - Axis");
    else
        bottomAxisLabel_ = label;

    wxFont font = GetAxisLabelFont();
    GetTextExtent(bottomAxisLabel_, &bottomAxisLabelRect_.width, &bottomAxisLabelRect_.height, NULL, NULL, &font);

    bottomAxisLabel_ = label;
    Refresh();
    DoSize();
}

void wxPlotCtrl::SetLeftAxisLabel(const wxString &label)
{
    if (label.IsEmpty())
        leftAxisLabel_ = wxT("Y - Axis");
    else
        leftAxisLabel_ = label;

    wxFont font = GetAxisLabelFont();
    GetTextExtent(leftAxisLabel_, &leftLabelRect_.height, &leftLabelRect_.width, NULL, NULL, &font);

    leftAxisLabel_ = label;

    Refresh();
    DoSize();
}

bool wxPlotCtrl::GetShowBottomAxisLabel() const
{
    return showBottomAxisLabel_;
}

bool wxPlotCtrl::GetShowLeftAxisLabel() const
{
    return showLeftAxisLabel_;
}

void wxPlotCtrl::SetShowBottomAxisLabel(bool show)
{
    showBottomAxisLabel_ = show;
    DoSize();
}

void wxPlotCtrl::SetShowLeftAxisLabel(bool show)
{
    showLeftAxisLabel_ = show;
    DoSize();
}

const wxString &wxPlotCtrl::GetPlotTitle() const
{
    return title_;
}

void wxPlotCtrl::SetPlotTitle(const wxString &title)
{
    if (title.IsEmpty())
        title_ = wxT("Title");
    else
        title_ = title;

    wxFont font = GetPlotTitleFont();
    GetTextExtent(title_, &titleRect_.width, &titleRect_.height, NULL, NULL, &font);

    title_ = title;

    Refresh();
    DoSize();
}

bool wxPlotCtrl::GetShowPlotTitle() const
{
    return showTitle_;
}

void wxPlotCtrl::SetShowPlotTitle(bool show)
{
    showTitle_ = show;
    DoSize();
}

const wxString &wxPlotCtrl::GetKeyString() const
{
    return keyString_;
}

bool wxPlotCtrl::GetShowKey() const
{
    return showKey_;
}

wxPoint wxPlotCtrl::GetKeyPosition() const
{
    return keyDrawer_->keyPosition_;
}
bool wxPlotCtrl::GetKeyInside() const
{
    return keyDrawer_->keyInside_;
}
void wxPlotCtrl::SetKeyPosition(const wxPoint &pos, bool stayInside)
{
    keyDrawer_->keyPosition_ = pos;
    keyDrawer_->keyInside_ = stayInside;
    Redraw(REDRAW_PLOT);
}

void wxPlotCtrl::CreateKeyString()
{
    keyString_.Clear();
    int n, count = curves_.GetCount();
    for (n = 0; n < count; n++)
    {
        wxString key;
        key.Printf(wxT("Curve %d"), n);

        keyString_ += (key + wxT("\n"));
    }
}

long wxPlotCtrl::GetMinExpValue() const
{
    return minExponential_;
}
void wxPlotCtrl::SetMinExpValue(long min)
{
    minExponential_ = min;
}

// ------------------------------------------------------------------------
// Curve Accessors
// ------------------------------------------------------------------------

bool wxPlotCtrl::AddCurve(wxPlotData *curve, bool select, bool sendEvent)
{
    if (!curve || !curve->Ok())
    {
        if (curve)
            delete curve;
        wxCHECK_MSG(false, false, wxT("Invalid curve"));
    }

    curves_.Add(curve);
    dataSelections_.Add(new wxRangeIntSelection());

    CalcBoundingPlotRect();
    CreateKeyString();

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_ADD_CURVE, GetId(), this);
        event.SetCurve(curve, curves_.GetCount()-1);
        DoSendEvent(event);
    }

    batchCount_++;
    if (select)
        SetActiveCurve(curve, sendEvent);
    batchCount_--;

    if (fitOnNewCurve_)
        SetZoom(-1, -1, 0, 0, true);
    else
        Redraw(REDRAW_PLOT);

    return true;
}

bool wxPlotCtrl::DeleteCurve(wxPlotData *curve, bool sendEvent)
{
    int index = curves_.Index(*curve);
    wxCHECK_MSG(index != wxNOT_FOUND, false, wxT("Unknown wxPlotData"));

    return DeleteCurve(index, sendEvent);
}

bool wxPlotCtrl::DeleteCurve(int n, bool sendEvent)
{
    wxCHECK_MSG((n>=-1)&&(n<int(curves_.GetCount())), false, wxT("Invalid curve index"));

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_DELETING_CURVE, GetId(), this);
        event.SetCurveIndex(n);
        if (!DoSendEvent(event))
            return false;
    }

    BeginBatch(); // don't redraw yet

    if (n < 0)
    {
        InvalidateCursor(sendEvent);
        ClearSelectedRanges(-1, sendEvent);
        dataSelections_.Clear();
        curves_.Clear();
    }
    else
    {
        if (cursorCurve_ == n)
            InvalidateCursor(sendEvent);
        else if (cursorCurve_ > n)
            cursorCurve_--;

        ClearSelectedRanges(n, sendEvent);
        dataSelections_.RemoveAt(n);
        curves_.RemoveAt(n);
    }

    int oldActiveIndex = activeIndex_;
    activeIndex_ = -1;
    activeCurve_ = nullptr;

    if (oldActiveIndex >= int(curves_.GetCount()))
    {
        // force this invalid, can't override this, the curve is "gone"
        SetActiveIndex(curves_.GetCount() - 1, sendEvent);
    }
    else if (oldActiveIndex >= 0)
        SetActiveIndex(oldActiveIndex, sendEvent);

    EndBatch(false); // still don't redraw

    CalcBoundingPlotRect();
    CreateKeyString();
    Redraw(REDRAW_PLOT);

    if (sendEvent)
    {
        wxPlotCtrlEvent event1(wxEVT_PLOTCTRL_DELETED_CURVE, GetId(), this);
        event1.SetCurveIndex(n);
        DoSendEvent(event1);
    }

    return true;
}
int wxPlotCtrl::GetCurveCount() const
{
    return curves_.GetCount();
}
bool wxPlotCtrl::CurveIndexOk(int curveIndex) const
{
    return (curveIndex>=0) && (curveIndex < int(curves_.GetCount()));
}
wxPlotData *wxPlotCtrl::GetCurve(int n) const
{
    wxCHECK_MSG((n >= 0) && (n < GetCurveCount()), NULL, wxT("Invalid index"));
    return &(curves_.Item(n));
}
void wxPlotCtrl::SetActiveCurve(wxPlotData *current, bool sendEvent)
{
    wxCHECK_RET(current, wxT("Invalid curve"));

    int index = curves_.Index(*current);
    wxCHECK_RET(index != wxNOT_FOUND, wxT("Unknown PlotCurve"));

    SetActiveIndex(index, sendEvent);
}
wxPlotData *wxPlotCtrl::GetActiveCurve() const
{
    return activeCurve_;
}
void wxPlotCtrl::SetActiveIndex(int curveIndex, bool sendEvent)
{
    wxCHECK_RET((curveIndex < GetCurveCount()), wxT("Invalid index"));

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_CURVE_SEL_CHANGING, GetId(), this);
        event.SetCurve(activeCurve_, activeIndex_);
        if (!DoSendEvent(event))
            return;
    }

    if ((curveIndex >= 0) && curves_.Item(curveIndex).Ok())
    {
        activeIndex_ = curveIndex;
        activeCurve_ = &(curves_.Item(curveIndex));
    }
    else
    {
        activeIndex_ = -1;
        activeCurve_ = NULL;
    }

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_CURVE_SEL_CHANGED, GetId(), this);
        event.SetCurve(activeCurve_, activeIndex_);
        DoSendEvent(event);
    }

    Redraw(REDRAW_PLOT);
}
int wxPlotCtrl::GetActiveIndex() const
{
    return activeIndex_;
}

//-------------------------------------------------------------------------
// Markers
//-------------------------------------------------------------------------
int wxPlotCtrl::AddMarker(const wxPlotMarker &marker)
{
    plotMarkers_.Add(marker);
    return plotMarkers_.GetCount() - 1;
}
void wxPlotCtrl::RemoveMarker(int marker)
{
    wxCHECK_RET((marker >= 0) && (marker < (int)plotMarkers_.GetCount()), wxT("Invalid marker number"));
    plotMarkers_.RemoveAt(marker);
}
void wxPlotCtrl::ClearMarkers()
{
    plotMarkers_.Clear();
}
wxPlotMarker wxPlotCtrl::GetMarker(int marker) const
{
    wxCHECK_MSG((marker >= 0) && (marker < (int)plotMarkers_.GetCount()), wxPlotMarker(),
                wxT("Invalid marker number"));
    return plotMarkers_[marker];
}
wxArrayPlotMarker &wxPlotCtrl::GetMarkerArray()
{
    return plotMarkers_;
}

//-------------------------------------------------------------------------
// Cursor position
//-------------------------------------------------------------------------

void wxPlotCtrl::InvalidateCursor(bool sendEvent)
{
    bool changed = cursorCurve_ >= 0;
    cursorCurve_ = -1;
    cursorIndex_ = -1;
    cursorMarker_.SetPlotPosition(wxPoint2DDouble(0, 0));

    if (sendEvent && changed)
    {
        wxPlotCtrlEvent plotEvent(wxEVT_PLOTCTRL_CURSOR_CHANGED, GetId(), this);
        DoSendEvent(plotEvent);
    }
}
bool wxPlotCtrl::IsCursorValid()
{
    if (cursorCurve_ < 0) return false;

    // sanity check
    if (cursorCurve_ >= int(curves_.GetCount()))
    {
        wxFAIL_MSG(wxT("Invalid cursor index"));
        InvalidateCursor(true);
        return false;
    }

    wxPlotData *plotData = GetCurve(cursorCurve_);
    if (plotData)
    {
        // sanity check
        if (cursorIndex_ < 0)
        {
            wxFAIL_MSG(wxT("Invalid cursor data index"));
            InvalidateCursor(true);
            return false;
        }
        // if the curve shrinks or is bad
        if (!plotData->Ok() || (cursorIndex_ >= (int)plotData->GetCount()))
        {
            InvalidateCursor(true);
            return false;
        }

        cursorMarker_.SetPlotPosition(plotData->GetPoint(cursorIndex_));
    }
    else
    {
        wxDouble x = cursorMarker_.GetPlotRect().m_x;
        cursorMarker_.GetPlotRect().m_y = GetCurve(cursorCurve_)->GetY(x);
    }

    return true;
}
int wxPlotCtrl::GetCursorCurveIndex() const
{
    return cursorCurve_;
}
int wxPlotCtrl::GetCursorDataIndex() const
{
    return cursorIndex_;
}
wxPoint2DDouble wxPlotCtrl::GetCursorPoint()
{
    wxCHECK_MSG(IsCursorValid(), wxPoint2DDouble(0, 0), wxT("invalid cursor"));
    return cursorMarker_.GetPlotPosition();
}

bool wxPlotCtrl::SetCursorDataIndex(int curveIndex, int cursorIndex, bool sendEvent)
{
    wxCHECK_MSG(CurveIndexOk(curveIndex) && GetCurve(curveIndex),
                false, wxT("invalid curve index"));

    wxPlotData *plotData = GetCurve(curveIndex);

    wxCHECK_MSG((cursorIndex >= 0) && plotData->Ok() && (cursorIndex < (int)plotData->GetCount()),
                 false, wxT("invalid index"));

    // do nothing if already set
    if ((cursorCurve_ == curveIndex) && (cursorIndex_ == cursorIndex))
        return false;

    wxPoint2DDouble cursorPt(plotData->GetPoint(cursorIndex));

    if (sendEvent)
    {
        wxPlotCtrlEvent cursorEvent(wxEVT_PLOTCTRL_CURSOR_CHANGING, GetId(), this);
        cursorEvent.SetPosition(cursorPt.m_x, cursorPt.m_y);
        cursorEvent.SetCurve(plotData, curveIndex);
        cursorEvent.SetCurveDataIndex(cursorIndex);
        if (!DoSendEvent(cursorEvent))
            return false;
    }

    int oldCursorCurve = cursorCurve_;
    int oldCursorIndex = cursorIndex_;
    cursorMarker_.SetPlotPosition(cursorPt);
    cursorCurve_ = curveIndex;
    cursorIndex_ = cursorIndex;

    if (sendEvent)
    {
        wxPlotCtrlEvent cursorEvent(wxEVT_PLOTCTRL_CURSOR_CHANGED, GetId(), this);
        cursorEvent.SetPosition(cursorPt.m_x, cursorPt.m_y);
        cursorEvent.SetCurve(plotData, curveIndex);
        cursorEvent.SetCurveDataIndex(cursorIndex);
        DoSendEvent(cursorEvent);
    }

    if ((activeIndex_ == oldCursorCurve) && (activeIndex_ == cursorCurve_))
    {
        RedrawCurve(curveIndex, oldCursorIndex, oldCursorIndex);
        RedrawCurve(curveIndex, cursorIndex_, cursorIndex_);
    }
    else
        Redraw(REDRAW_PLOT);

    return true;
}
bool wxPlotCtrl::SetCursorXPoint(int curveIndex, double x, bool sendEvent)
{
    wxCHECK_MSG(CurveIndexOk(curveIndex), false, wxT("invalid curve index"));

    if (GetCurve(curveIndex))
        return SetCursorDataIndex(curveIndex, GetCurve(curveIndex)->GetIndexFromX(x), sendEvent);

    // do nothing if already set
    if ((cursorCurve_ == curveIndex) && (cursorMarker_.GetPlotRect().m_x == x))
        return false;

    wxPlotData *plotCurve = GetCurve(curveIndex);
    wxPoint2DDouble cursorPt(x, plotCurve->GetY(x));

    if (sendEvent)
    {
        wxPlotCtrlEvent cursorEvent(wxEVT_PLOTCTRL_CURSOR_CHANGING, GetId(), this);
        cursorEvent.SetPosition(cursorPt.m_x, cursorPt.m_y);
        cursorEvent.SetCurve(plotCurve, curveIndex);
        if (!DoSendEvent(cursorEvent))
            return false;
    }

    cursorMarker_.SetPlotPosition(cursorPt);
    cursorCurve_ = curveIndex;
    cursorIndex_ = -1;

    if (sendEvent)
    {
        wxPlotCtrlEvent cursorEvent(wxEVT_PLOTCTRL_CURSOR_CHANGED, GetId(), this);
        cursorEvent.SetPosition(cursorPt.m_x, cursorPt.m_y);
        cursorEvent.SetCurve(plotCurve, curveIndex);
        DoSendEvent(cursorEvent);
    }

    Redraw(REDRAW_PLOT);
    return true;
}

void wxPlotCtrl::MakeCursorVisible(bool center, bool sendEvent)
{
    wxCHECK_RET(IsCursorValid(), wxT("invalid plot cursor"));

    if (center)
    {
        wxPoint2DDouble origin = viewRect_.GetLeftTop() -
                                 viewRect_.GetCentre() +
                                 GetCursorPoint();

        SetOrigin(origin.m_x, origin.m_y, sendEvent);
        return;
    }

    wxPoint2DDouble origin = GetCursorPoint();

    if (viewRect_.Contains(origin))
        return;

    double dx = 4/zoom_.m_x;
    double dy = 4/zoom_.m_y;

    if (origin.m_x < viewRect_.m_x)
        origin.m_x -= dx;
    else if (origin.m_x > viewRect_.GetRight())
        origin.m_x = viewRect_.m_x + (origin.m_x - viewRect_.GetRight()) + dx;
    else
        origin.m_x = viewRect_.m_x;

    if (origin.m_y < viewRect_.m_y)
        origin.m_y -= dy;
    else if (origin.m_y > viewRect_.GetBottom())
        origin.m_y = viewRect_.m_y + (origin.m_y - viewRect_.GetBottom()) + dy;
    else
        origin.m_y = viewRect_.m_y;

    SetOrigin(origin.m_x, origin.m_y, sendEvent);
}

//-------------------------------------------------------------------------
// Selected points, data curves use
//-------------------------------------------------------------------------
bool wxPlotCtrl::HasSelection(int curveIndex) const
{
    if (curveIndex == -1)
    {
        int n, count = dataSelections_.GetCount();
        for (n = 0; n < count; n++)
            if (dataSelections_[n].GetCount() > 0)
                return true;
        return false;
    }

    wxCHECK_MSG(CurveIndexOk(curveIndex), false, wxT("invalid curve index"));
    return dataSelections_[curveIndex].GetCount() > 0;
}

wxRangeIntSelection *wxPlotCtrl::GetDataCurveSelection(int curveIndex) const
{
    wxCHECK_MSG(CurveIndexOk(curveIndex), NULL, wxT("invalid curve index"));
    return &dataSelections_[curveIndex];
}
const wxArrayRangeIntSelection &wxPlotCtrl::GetDataCurveSelections() const
{
    return dataSelections_;
}
bool wxPlotCtrl::UpdateSelectionState(int curveIndex, bool sendEvent)
{
    wxCHECK_MSG(CurveIndexOk(curveIndex), false, wxT("invalid curve index"));
    switch (selectionType_)
    {
        case SelectionType::NONE:
            break; // should have been handled
        case SelectionType::SINGLE:
        {
            if (HasSelection())
                return ClearSelectedRanges(-1, sendEvent);

            break;
        }
        case SelectionType::SINGLE_CURVE:
        {
            int n, count = curves_.GetCount();
            bool done = false;
            for (n = 0; n < count; n++)
            {
                if ((n != curveIndex) && HasSelection(n))
                    done |= ClearSelectedRanges(n, sendEvent);
            }
            return done;
        }
        case SelectionType::SINGLE_PER_CURVE:
        {
            if (HasSelection(curveIndex))
                return ClearSelectedRanges(curveIndex, sendEvent);

            break;
        }
        case SelectionType::MULTIPLE:
            break; // anything goes
        default:
            break;
    }

    return false;
}

bool wxPlotCtrl::DoSelectRectangle(int curveIndex, const wxRect2DDouble &rect,
                                     bool select, bool sendEvent)
{
    wxCHECK_MSG((curveIndex >= -1) && (curveIndex<int(curves_.GetCount())),
                false, wxT("invalid plotcurve index"));
    wxCHECK_MSG((rect.m_width > 0) || (rect.m_height > 0), false, wxT("invalid selection range"));

    if (selectionType_ == SelectionType::NONE)
        return false;

    if (!IsFinite(rect.m_x, wxT("Selection x is NaN")) ||
        !IsFinite(rect.m_y, wxT("Selection y is NaN")) ||
        !IsFinite(rect.m_width, wxT("Selection width is NaN")) ||
        !IsFinite(rect.m_height, wxT("Selection height is NaN")))
        return false;

    bool done = false;

    // Run this code for all the curves if curve == -1 then exit
    if (curveIndex == -1)
    {
        size_t n, curve_count = curves_.GetCount();

        for (n = 0; n < curve_count; n++)
            done |= DoSelectRectangle(n, rect, select, sendEvent);

        return done;
    }

    // check the selection type and clear previous selections if necessary
    if (select)
        UpdateSelectionState(curveIndex, sendEvent);

    bool is_x_range = rect.m_height <= 0;
    bool is_y_range = rect.m_width <= 0;
    double xRangeMin = rect.m_x;
    double xRangeMax = rect.GetRight();
    double yRangeMin = rect.m_y;
    double yRangeMax = rect.GetBottom();

    wxPlotData *plotData = GetCurve(curveIndex);
    if (plotData)
    {
        wxCHECK_MSG(plotData->Ok(), false, wxT("Invalid data curve"));
        wxRect2DDouble r(plotData->GetBoundingRect());

        if ((xRangeMax < r.GetLeft()) || (xRangeMin > r.GetRight()))
            return false;


        int i, count = plotData->GetCount();
        int first_sel = -1;
        double *x_data = plotData->GetXData();
        double *y_data = plotData->GetYData();

        int min = plotData->GetCount() - 1, max = 0;

        wxRangeIntSelection ranges;

        for (i=0; i<count; i++)
        {
            if ((is_x_range && *x_data >= xRangeMin && *x_data <= xRangeMax) ||
                (is_y_range && *y_data >= yRangeMin && *y_data <= yRangeMax) ||
                (!is_x_range && !is_y_range && wxPlotRect2DDoubleContains(*x_data, *y_data, rect)))
            {
                if (select)
                {
                    if (dataSelections_[curveIndex].SelectRange(wxRangeInt(i,i)))
                    {
                        ranges.SelectRange(wxRangeInt(i,i));
                        done = true;
                    }
                }
                else
                {
                    if (dataSelections_[curveIndex].DeselectRange(wxRangeInt(i,i)))
                    {
                        ranges.SelectRange(wxRangeInt(i,i));
                        done = true;
                    }
                }

                min = wxMin(min, i);
                max = wxMin(max, i);

                if (done && (first_sel == -1))
                    first_sel = i;
            }

            x_data++;
            y_data++;
        }

        if (done && (min <= max))
            RedrawCurve(curveIndex, min, max);

        if (sendEvent && done)
        {
            wxPlotCtrlSelEvent event(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED, GetId(), this);
            event.SetCurve(GetCurve(curveIndex), curveIndex);
            event.SetDataSelectionRange(wxRangeInt(first_sel, first_sel), select);
            event.SetDataSelections(ranges);
            DoSendEvent(event);
        }

        return done;

    }
    else
    {
        if (sendEvent && done)
        {
            wxPlotCtrlSelEvent event(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED, GetId(), this);
            event.SetCurve(GetCurve(curveIndex), curveIndex);
            //event.SetCurveSelectionRange(xRange, select);
            DoSendEvent(event);
        }

        return done;
    }
}

bool wxPlotCtrl::DoSelectDataRange(int curveIndex, const wxRangeInt &range,
                                     bool select, bool sendEvent)
{
    wxCHECK_MSG(CurveIndexOk(curveIndex), false, wxT("invalid plotcurve index"));
    wxCHECK_MSG(!range.IsEmpty(), false, wxT("invalid selection range"));

    if (selectionType_ == SelectionType::NONE)
        return false;

    wxPlotData *plotData = GetCurve(curveIndex);
    wxCHECK_MSG(plotData && (range.m_min >= 0) && (range.m_max < (int)plotData->GetCount()), false, wxT("invalid index"));

    // check the selection type and clear previous selections if necessary
    if (select)
        UpdateSelectionState(curveIndex, sendEvent);

    bool done = false;

    if (select)
        done = dataSelections_[curveIndex].SelectRange(range);
    else
        done = dataSelections_[curveIndex].DeselectRange(range);

    if (sendEvent && done)
    {
        wxPlotCtrlSelEvent event(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED, GetId(), this);
        event.SetCurve(GetCurve(curveIndex), curveIndex);
        event.SetDataSelectionRange(range, select);
        event.GetDataSelections().SelectRange(range);
        DoSendEvent(event);
    }

    if (done)
        RedrawCurve(curveIndex, range.m_min, range.m_max);

    return done;
}

int wxPlotCtrl::GetSelectedRangeCount(int curveIndex) const
{
    wxCHECK_MSG(CurveIndexOk(curveIndex), 0, wxT("invalid plotcurve index"));

    return dataSelections_[curveIndex].GetCount();
}
bool wxPlotCtrl::SelectRectangle(int curveIndex, const wxRect2DDouble &rect, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, rect, true, sendEvent);
}
bool wxPlotCtrl::DeselectRectangle(int curveIndex, const wxRect2DDouble &rect, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, rect, false, sendEvent);
}
bool wxPlotCtrl::SelectXRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, wxRect2DDouble(rangeMin, -wxPlot_MAX_DBL, rangeMax - rangeMin, wxPlot_MAX_RANGE), true, sendEvent);
}
bool wxPlotCtrl::DeselectXRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, wxRect2DDouble(rangeMin, -wxPlot_MAX_DBL, rangeMax - rangeMin, wxPlot_MAX_RANGE), false, sendEvent);
}
bool wxPlotCtrl::SelectYRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, wxRect2DDouble(-wxPlot_MAX_DBL, rangeMin, wxPlot_MAX_RANGE, rangeMax - rangeMin), true, sendEvent);
}
bool wxPlotCtrl::DeselectYRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent)
{
    return DoSelectRectangle(curveIndex, wxRect2DDouble(-wxPlot_MAX_DBL, rangeMin, wxPlot_MAX_RANGE, rangeMax - rangeMin), false, sendEvent);
}
bool wxPlotCtrl::SelectDataRange(int curveIndex, const wxRangeInt &range, bool sendEvent)
{
    return DoSelectDataRange(curveIndex, range, true, sendEvent);
}
bool wxPlotCtrl::DeselectDataRange(int curveIndex, const wxRangeInt &range, bool sendEvent)
{
    return DoSelectDataRange(curveIndex, range, false, sendEvent);
}
bool wxPlotCtrl::ClearSelectedRanges(int curveIndex, bool sendEvent)
{
    wxCHECK_MSG((curveIndex >= -1) && (curveIndex<int(curves_.GetCount())),
                false, wxT("invalid plotcurve index"));

    bool done = false;

    if (curveIndex == -1)
    {
        for (size_t n=0; n<curves_.GetCount(); n++)
            done |= ClearSelectedRanges(n, sendEvent);

        return done;
    }
    else
    {
        done = dataSelections_[curveIndex].GetCount() > 0;
        dataSelections_[curveIndex].Clear();
        if (done)
            RedrawCurve(curveIndex, 0, GetCurve(curveIndex)->GetCount()-1);
    }

    if (sendEvent && done)
    {
        wxPlotCtrlSelEvent event(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED, GetId(), this);
        event.SetCurve(GetCurve(curveIndex), curveIndex);
        event.SetDataSelectionRange(wxRangeInt(0, GetCurve(curveIndex)->GetCount() - 1), false);
        DoSendEvent(event);
    }
    return done;
}

// ------------------------------------------------------------------------
// Get/Set origin, size, and Zoom in/out of view, set scaling, size...
// ------------------------------------------------------------------------
/*

// FIXME - can't shift the bitmap due to off by one errors in ClipLineToRect

void wxPlotCtrl::ShiftOrigin(int dx, int dy, bool sendEvent)
{
    if ((dx == 0) && (dy == 0)) return;

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_VIEW_CHANGING, GetId(), this);
        event.SetCurve(activeCurve_, activeIndex_);
        if (DoSendEvent(event)) return;
    }

    {
        wxBitmap tempBitmap(areaClientRect_.width, areaClientRect_.height);
        wxMemoryDC mdc;
        mdc.SelectObject(tempBitmap);
        mdc.DrawBitmap(m_area->m_bitmap, dx, dy, false);
        mdc.SelectObject(wxNullBitmap);
        m_area->m_bitmap = tempBitmap;
    }
    wxRect rx, ry;

    viewRect_.m_x -= dx / zoom_.m_x;
    viewRect_.m_y += dy / zoom_.m_y;

    if (dx != 0)
    {
        rx = wxRect((dx>0 ? -5 : areaClientRect_.width+dx-5), 0, labs(dx)+10, areaClientRect_.height);
        RedrawXAxis(false);
    }
    if (dy != 0)
    {
        ry = wxRect(0, (dy>0 ? -5 : areaClientRect_.height+dy-5), areaClientRect_.width, labs(dy)+10);
        RedrawYAxis(false);
    }

    printf("Shift %d %d rx %d %d %d %d, ry %d %d %d %d\n", dx, dy, rx.x, rx.y, rx.width, rx.height, ry.x, ry.y, ry.width, ry.height); fflush(stdout);

    if (rx.width > 0) m_area->CreateBitmap(rx);
        //m_area->Refresh(false, &rx);
    if (ry.height > 0) m_area->CreateBitmap(ry);
        //m_area->Refresh(false, &ry);

    {
        wxClientDC cdc(m_area);
        cdc.DrawBitmap(m_area->m_bitmap, 0, 0);
    }

    adjustScrollBars();

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_VIEW_CHANGED, GetId(), this);
        event.SetCurve(activeCurve_, activeIndex_);
        DoSendEvent(event);
    }
}
*/

void wxPlotCtrl::SetSelectionType(SelectionType type)
{
    selectionType_ = type;
}

wxPlotCtrl::SelectionType wxPlotCtrl::GetSelectionType() const
{
    return selectionType_;
}

bool wxPlotCtrl::MakeCurveVisible(int curveIndex, bool sendEvent)
{
    if (curveIndex < 0)
        return SetZoom(-1, -1, 0, 0, sendEvent);

    wxCHECK_MSG(curveIndex < GetCurveCount(), false, wxT("Invalid curve index"));
    wxPlotData *curve = GetCurve(curveIndex);
    wxCHECK_MSG(curve && curve->Ok(), false, wxT("Invalid curve"));
    return SetViewRect(curve->GetBoundingRect(), sendEvent);
}

bool wxPlotCtrl::SetOrigin(double origin_x, double origin_y, bool sendEvent)
{
    return SetZoom(zoom_.m_x, zoom_.m_y, origin_x, origin_y, sendEvent);
}

const wxRect2DDouble &wxPlotCtrl::GetViewRect() const
{
    return viewRect_;
}

bool wxPlotCtrl::SetViewRect(const wxRect2DDouble &view, bool sendEvent)
{
    double zoom_x = areaClientRect_.width/view.m_width;
    double zoom_y = areaClientRect_.height/view.m_height;
    return SetZoom(zoom_x, zoom_y, view.m_x, view.m_y, sendEvent);
}

const wxPoint2DDouble &wxPlotCtrl::GetZoom() const
{
    return zoom_;
}

bool wxPlotCtrl::SetZoom(const wxPoint2DDouble &zoom, bool aroundCenter, bool sendEvent)
{
    if (aroundCenter && (zoom.m_x > 0) && (zoom.m_y > 0))
    {
        double origin_x = (viewRect_.GetLeft() + viewRect_.m_width/2.0);
        origin_x -= (viewRect_.m_width/2.0)*zoom_.m_x/zoom.m_x;
        double origin_y = (viewRect_.GetTop() + viewRect_.m_height/2.0);
        origin_y -= (viewRect_.m_height/2.0)*zoom_.m_y/zoom.m_y;
        return SetZoom(zoom.m_x, zoom.m_y, origin_x, origin_y, sendEvent);
    }
    else
        return SetZoom(zoom.m_x, zoom.m_y, viewRect_.GetLeft(), viewRect_.GetTop(), sendEvent);
}

bool wxPlotCtrl::SetZoom(const wxRect &window, bool sendEvent)
{
    if ((window.GetHeight()<1) || (window.GetWidth()<1)) return false;

    double origin_x = GetPlotCoordFromClientX(window.GetX());
    double origin_y = GetPlotCoordFromClientY(window.GetY()+window.GetHeight());
    double zoom_x = zoom_.m_x * double(areaClientRect_.width) /(window.GetWidth());
    double zoom_y = zoom_.m_y * double(areaClientRect_.height)/(window.GetHeight());

    bool ok = SetZoom(zoom_x, zoom_y, origin_x, origin_y, sendEvent);
    if (ok)
        AddHistoryView();
    return ok;
}

bool wxPlotCtrl::SetZoom(double zoom_x, double zoom_y,
                          double origin_x, double origin_y, bool sendEvent)
{
    // fit to window if zoom <= 0
    if (zoom_x <= 0)
    {
        zoom_x = double(areaClientRect_.width)/(curveBoundingRect_.m_width);
        origin_x = curveBoundingRect_.m_x;
    }
    if (zoom_y <= 0)
    {
        zoom_y = double(areaClientRect_.height)/(curveBoundingRect_.m_height);
        origin_y = curveBoundingRect_.m_y;
    }

    if (fixedAspectRatio_)
        FixAspectRatio(&zoom_x, &zoom_y, &origin_x, &origin_y);

    double view_width  = areaClientRect_.width/zoom_x;
    double view_height = areaClientRect_.height/zoom_y;

    if (!IsFinite(zoom_x, wxT("X zoom is NaN"))) return false;
    if (!IsFinite(zoom_y, wxT("Y zoom is NaN"))) return false;
    if (!IsFinite(origin_x, wxT("X origin is not finite"))) return false;
    if (!IsFinite(origin_y, wxT("Y origin is not finite"))) return false;
    if (!IsFinite(view_width, wxT("Plot width is NaN"))) return false;
    if (!IsFinite(view_height, wxT("Plot height is NaN"))) return false;

    bool x_changed = false, y_changed = false;

    if ((viewRect_.m_x != origin_x) || (zoom_.m_x != zoom_x))
        x_changed = true;
    if ((viewRect_.m_y != origin_y) || (zoom_.m_y != zoom_y))
        y_changed = true;

    if (x_changed || y_changed)
    {
        if (sendEvent)
        {
            wxPlotCtrlEvent event(wxEVT_PLOTCTRL_VIEW_CHANGING, GetId(), this);
            event.SetCurve(activeCurve_, activeIndex_);
            event.SetPosition(origin_x, origin_y);
            if (!DoSendEvent(event))
                return false;
        }

        zoom_.m_x = zoom_x;
        zoom_.m_y = zoom_y;

        viewRect_.m_x = origin_x;
        viewRect_.m_y = origin_y;
        viewRect_.m_width  = view_width;
        viewRect_.m_height = view_height;
    }

    // redraw even if unchanged since we expect that it should be different
    Redraw(REDRAW_PLOT |
           (x_changed ? REDRAW_BOTTOM_AXIS : 0) |
           (y_changed ? REDRAW_LEFT_AXIS   : 0));

    if (!batchCount_)
        AdjustScrollBars();

    if (sendEvent && (x_changed || y_changed))
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_VIEW_CHANGED, GetId(), this);
        event.SetCurve(activeCurve_, activeIndex_);
        event.SetPosition(origin_x, origin_y);
        DoSendEvent(event);
    }

    return true;
}

void wxPlotCtrl::SetFixAspectRatio(bool fix, double ratio)
{
    wxCHECK_RET(ratio > 0, wxT("Invalid aspect ratio"));
    fixedAspectRatio_ = fix;
    aspectratio_ = ratio;
}

void wxPlotCtrl::FixAspectRatio(double *zoom_x, double *zoom_y, double *origin_x, double *origin_y)
{
    wxCHECK_RET(zoom_x && zoom_y && origin_x && origin_y, wxT("Invalid parameters"));

    //get the width and height of the view in plot coordinates
    double viewWidth = areaClientRect_.width / (*zoom_x);
    double viewHeight = areaClientRect_.height / (*zoom_y);

    //get the centre of the visible area in plot coordinates
    double xCentre = (*origin_x) + viewWidth / 2;
    double yCentre = (*origin_y) + viewHeight / 2;

    //if zoom in one direction is more than in the other, reduce both to the lower value
    if((*zoom_x) * aspectratio_ > (*zoom_y))
    {
        (*zoom_x) = (*zoom_y) * aspectratio_;
        (*zoom_y) = (*zoom_y);
    }
    else
    {
        (*zoom_x) = (*zoom_x);
        (*zoom_y) = (*zoom_x) / aspectratio_;
    }

    //update the plot coordinate view width and height based on the new zooms
    viewWidth = areaClientRect_.width / (*zoom_x);
    viewHeight = areaClientRect_.height / (*zoom_y);

    //create the new bottom-left corner of the view in plot coordinates
    *origin_x = xCentre - (viewWidth / 2);
    *origin_y = yCentre - (viewHeight / 2);
}

void wxPlotCtrl::SetDefaultBoundingRect(const wxRect2DDouble &rect, bool sendEvent)
{
    wxCHECK_RET(wxFinite(rect.m_x)&&wxFinite(rect.m_y)&&wxFinite(rect.GetRight())&&wxFinite(rect.GetBottom()), wxT("bounding rect is NaN"));
    wxCHECK_RET((rect.m_width > 0) && (rect.m_height > 0), wxT("Plot Size < 0"));
    defaultPlotRect_ = rect;
    CalcBoundingPlotRect();
    SetZoom(areaClientRect_.width/rect.m_width,
             areaClientRect_.height/rect.m_height,
             rect.m_x, rect.m_y, sendEvent);
}

const wxRect2DDouble &wxPlotCtrl::GetDefaultBoundingRect() const
{
    return defaultPlotRect_;
}

void wxPlotCtrl::AddHistoryView()
{
    if (!(wxFinite(viewRect_.GetLeft())&&wxFinite(viewRect_.GetRight())&&wxFinite(viewRect_.GetTop())&&wxFinite(viewRect_.GetBottom()))) return;

    if ((historyViewsIndex_ >= 0)
        && (historyViewsIndex_ < int(historyViews_.GetCount()))
        && (viewRect_ == historyViews_[historyViewsIndex_]))
            return;

    if (int(historyViews_.GetCount()) >= MAX_PLOT_ZOOMS)
    {
        if (historyViewsIndex_ < int(historyViews_.GetCount())-1)
        {
            historyViews_[historyViewsIndex_] = viewRect_;
        }
        else
        {
            historyViews_.RemoveAt(0);
            historyViews_.Add(viewRect_);
        }
    }
    else
    {
        historyViews_.Add(viewRect_);
        historyViewsIndex_++;
    }
}

const wxRect2DDouble &wxPlotCtrl::GetCurveBoundingRect() const
{
    return curveBoundingRect_;
}

const wxRect &wxPlotCtrl::GetPlotAreaRect() const
{
    return areaClientRect_;
}

void wxPlotCtrl::NextHistoryView(bool foward, bool sendEvent)
{
    int count = historyViews_.GetCount();

    // try to set it to the "current" history view
    if ((historyViewsIndex_ > -1) && (historyViewsIndex_ < count))
    {
        if (!(viewRect_ == historyViews_[historyViewsIndex_]))
            SetViewRect(historyViews_[historyViewsIndex_], sendEvent);
    }

    if (foward)
    {
        if ((count > 0) && (historyViewsIndex_ < count - 1))
        {
            historyViewsIndex_++;
            SetViewRect(historyViews_[historyViewsIndex_], sendEvent);
        }
    }
    else
    {
        if (historyViewsIndex_ > 0)
        {
            historyViewsIndex_--;
            SetViewRect(historyViews_[historyViewsIndex_], sendEvent);
        }
        else
            SetZoom(-1, -1, 0, 0, sendEvent);
    }
}

int wxPlotCtrl::GetHistoryViewCount() const
{
    return historyViews_.GetCount();
}
int wxPlotCtrl::GetHistoryViewIndex() const
{
    return historyViewsIndex_;
}
const wxPoint &wxPlotCtrl::GetAreaMouseCoord() const
{
    return area_->lastMousePosition_;
}
wxPoint2DDouble wxPlotCtrl::GetAreaMousePoint() const
{
    return wxPoint2DDouble(GetPlotCoordFromClientX(area_->lastMousePosition_.x),
                           GetPlotCoordFromClientY(area_->lastMousePosition_.y));
}
const wxRect &wxPlotCtrl::GetAreaMouseMarkedRect() const
{
    return area_->mouseDragRectangle_;
}
void wxPlotCtrl::SetAreaMouseFunction(MouseFunction func, bool sendEvent)
{
    if (func == areaMouseFunc_)
        return;

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGING, GetId(), this);
        event.setMouseFunction(func);
        if (!DoSendEvent(event))
            return;
    }

    areaMouseFunc_ = func;

    switch (func)
    {
        case MouseFunction::ZOOM :
        {
            SetAreaMouseCursor(wxCURSOR_MAGNIFIER); //wxCURSOR_CROSS);
            break;
        }
        case MouseFunction::SELECT   :
        case MouseFunction::DESELECT :
        {
            SetAreaMouseCursor(wxCURSOR_ARROW);
            break;
        }
        case MouseFunction::PAN :
        {
            SetAreaMouseCursor(wxCURSOR_HAND);
            SetAreaMouseMarker(MarkerType::NONE);
            break;
        }
        case MouseFunction::NOTHING :
        default :
        {
            SetAreaMouseCursor(wxCURSOR_CROSS);
            SetAreaMouseMarker(MarkerType::NONE);
            break;
        }
    }

    if (sendEvent)
    {
        wxPlotCtrlEvent event(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGED, GetId(), this);
        event.setMouseFunction(func);
        DoSendEvent(event);
    }
}

wxPlotCtrl::MouseFunction wxPlotCtrl::GetAreaMouseFunction() const
{
    return areaMouseFunc_;
}

void wxPlotCtrl::SetAreaMouseMarker(MarkerType type)
{
    if (type == areaMouseMarker_)
        return;

    wxClientDC dc(area_);
    DrawMouseMarker(&dc, areaMouseMarker_, area_->mouseDragRectangle_);
    areaMouseMarker_ = type;
    DrawMouseMarker(&dc, areaMouseMarker_, area_->mouseDragRectangle_);
}

wxPlotCtrl::MarkerType wxPlotCtrl::GetAreaMouseMarker() const
{
    return areaMouseMarker_;
}

void wxPlotCtrl::SetAreaMouseCursor(int cursorid)
{
    if (cursorid == areaMouseCursorid_)
        return;

    areaMouseCursorid_ = cursorid;

    if (cursorid == wxCURSOR_HAND)
        area_->SetCursor(s_handCursor);
    else if (cursorid == CURSOR_GRAB)
        area_->SetCursor(s_grabCursor);
    else
        area_->SetCursor(wxCursor(wxStockCursor(cursorid)));
}

bool wxPlotCtrl::GetCrossHairCursor() const
{
    return crosshairCursor_;
}
void wxPlotCtrl::SetCrossHairCursor(bool useCrosshairCursor)
{
    crosshairCursor_ = useCrosshairCursor;
    area_->lastMousePosition_ = wxPoint(-1,-1);
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetDrawSymbols() const
{
    return drawSymbols_;
}
void wxPlotCtrl::SetDrawSymbols(bool drawSymbols)
{
    drawSymbols_ = drawSymbols;
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetDrawLines() const
{
    return drawLines_;
}
void wxPlotCtrl::SetDrawLines(bool drawlines)
{
    drawLines_ = drawlines;
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetDrawSpline() const
{
    return drawSpline_;
}
void wxPlotCtrl::SetDrawSpline(bool drawSpline)
{
    drawSpline_ = drawSpline;
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetDrawGrid() const
{
    return drawGrid_;
}
void wxPlotCtrl::SetDrawGrid(bool drawGrid)
{
    drawGrid_ = drawGrid;
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetDrawTicks() const
{
    return drawTicks_;
}
void wxPlotCtrl::SetDrawTicks(bool drawTicks)
{
    drawTicks_ = drawTicks;
    Redraw(REDRAW_PLOT);
}
bool wxPlotCtrl::GetFitPlotOnNewCurve() const
{
    return fitOnNewCurve_;
}
void wxPlotCtrl::SetFitPlotOnNewCurve(bool fit)
{
    fitOnNewCurve_ = fit;
}
bool wxPlotCtrl::GetGreedyFocus() const
{
    return greedyFocus_;
}
void wxPlotCtrl::SetGreedyFocus(bool grabFocus)
{
    greedyFocus_ = grabFocus;
}
bool wxPlotCtrl::GetCorrectTicks() const
{
    return bottomAxisTicks_.correct_;
}
void wxPlotCtrl::SetCorrectTicks(bool correct)
{
    bottomAxisTicks_.correct_ = correct;
    leftAxisTicks_.correct_ = correct;
}
double wxPlotCtrl::GetPrintingPenWidth()
{
    return penPrintWidth_;
}
void wxPlotCtrl::SetPrintingPenWidth(double width)
{
    penPrintWidth_ = width;
}
void wxPlotCtrl::OnSize(wxSizeEvent&)
{
    DoSize();
}

void wxPlotCtrl::DoSize(const wxRect &boundingRect, bool setWindowSizes)
{
    if (!yAxisScrollbar_)  // we're not created yet
        return;

    redrawNeed_ = REDRAW_BLOCKER;  // block OnPaints until done

    wxSize size;

    if(boundingRect == wxRect(0, 0, 0, 0))
    {
        UpdateWindowSize();
        size = GetClientSize();
    }
    else
    {
        size.x = boundingRect.width;
        size.y = boundingRect.height;
    }

    // wait until we have a normal size
    if ((size.x < 2) || (size.y < 2)) return;

    int sb_width = yAxisScrollbar_->GetSize().GetWidth();

    clientRect_ = wxRect(0, 0, size.x-sb_width, size.y-sb_width);

    // title and label positions, add padding here
    wxRect titleRect  = showTitle_  ? wxRect(titleRect_).Inflate(border_)  : wxRect(0,0,1,1);
    wxRect xLabelRect = showBottomAxisLabel_ ? wxRect(bottomAxisLabelRect_).Inflate(border_) : wxRect(0,0,1,1);
    wxRect leftLabelRect = showLeftAxisLabel_ ? wxRect(leftLabelRect_).Inflate(border_) : wxRect(0,0,1,1);

    // this is the border around the area, it lets you see about 1 digit extra on axis
    int areaBorder = axisFontSize_.y/2;

    // use the areaBorder between top of y-axis and area as bottom border of title
    if (showTitle_) titleRect.height -= border_;

    int leftAxisWidth = GetShowLeftAxis() ? leftAxisTextWidth_ : 1;
    int bottomAxisHeight = GetShowBottomAxis() ? axisFontSize_.y : areaBorder;

    int areaWidth  = clientRect_.width  - leftLabelRect.GetRight() - leftAxisWidth - 2*areaBorder;
    int areaHeight = clientRect_.height - titleRect.GetBottom() - bottomAxisHeight - xLabelRect.height - areaBorder;

    leftAxisRect_ = wxRect(leftLabelRect.GetRight(),
                         titleRect.GetBottom(),
                         leftAxisWidth,
                         areaHeight + 2*areaBorder);

    bottomAxisRect_ = wxRect(leftAxisRect_.GetRight(),
                         leftAxisRect_.GetBottom() - areaBorder + 1,
                         areaWidth + 2*areaBorder,
                         bottomAxisHeight);

    areaRect_ = wxRect(leftAxisRect_.GetRight() + areaBorder,
                        leftAxisRect_.GetTop() + areaBorder,
                        areaWidth,
                        areaHeight);

    // scrollbar to right and bottom
    if (setWindowSizes)
    {
        yAxisScrollbar_->SetSize(clientRect_.width, 0, sb_width, clientRect_.height);
        xAxisScrollbar_->SetSize(0, clientRect_.height, clientRect_.width, sb_width);

        leftAxis_->Show(GetShowLeftAxis());
        bottomAxis_->Show(GetShowBottomAxis());
        if (GetShowLeftAxis()) leftAxis_->SetSize(leftAxisRect_);
        if (GetShowBottomAxis()) bottomAxis_->SetSize(bottomAxisRect_);

        area_->SetSize(areaRect_);
        UpdateWindowSize();
    }
    else
        areaClientRect_ = wxRect(wxPoint(0,0), areaRect_.GetSize());

    titleRect_.x = areaRect_.x + (areaRect_.width - titleRect_.GetWidth()) / 2;
    //titleRect_.x = clientRect_.width/2-titleRect_.GetWidth()/2; center on whole plot
    titleRect_.y = border_;

    bottomAxisLabelRect_.x = areaRect_.x + areaRect_.width/2 - bottomAxisLabelRect_.width/2;
    bottomAxisLabelRect_.y = bottomAxisRect_.GetBottom() + border_;

    leftLabelRect_.x = border_;
    leftLabelRect_.y = areaRect_.y + areaRect_.height/2 - leftLabelRect_.height/2;

    double zoom_x = areaClientRect_.width/viewRect_.m_width;
    double zoom_y = areaClientRect_.height/viewRect_.m_height;
    if (!IsFinite(zoom_x, wxT("Plot zoom is NaN"))) return;
    if (!IsFinite(zoom_y, wxT("Plot zoom is NaN"))) return;

    if (fixedAspectRatio_)
    {
        FixAspectRatio(&zoom_x, &zoom_y, &viewRect_.m_x, &viewRect_.m_y);

        viewRect_.m_width = areaClientRect_.width/zoom_x;
        viewRect_.m_height = areaClientRect_.height/zoom_y;
    }

    zoom_.m_x = zoom_x;
    zoom_.m_y = zoom_y;

    wxPlotCtrlEvent event(wxEVT_PLOTCTRL_VIEW_CHANGED, GetId(), this);
    event.SetCurve(activeCurve_, activeIndex_);
    DoSendEvent(event);

    redrawNeed_ = 0;
    Redraw(REDRAW_WHOLEPLOT);
}

void wxPlotCtrl::CalcBoundingPlotRect()
{
    int i, count = curves_.GetCount();

    if (count > 0)
    {
        bool valid_rect = false;

        wxRect2DDouble rect = curves_[0].GetBoundingRect();

        if (IsFinite(rect.m_x, wxT("left curve boundary is NaN")) &&
             IsFinite(rect.m_y, wxT("bottom curve boundary is NaN")) &&
             IsFinite(rect.GetRight(), wxT("right curve boundary is NaN")) &&
             IsFinite(rect.GetBottom(), wxT("top curve boundary is NaN")) &&
             (rect.m_width >= 0) && (rect.m_height >= 0))
        {
            valid_rect = true;
        }
        else
            rect = wxNullPlotBounds;

        for (i=1; i<count; i++)
        {
            wxRect2DDouble curveRect = curves_[i].GetBoundingRect();

            if ((curveRect.m_width) <= 0 || (curveRect.m_height <= 0))
                continue;

            wxRect2DDouble newRect;
            if (!valid_rect)
                newRect = curveRect;
            else
                newRect = rect.CreateUnion(curveRect);

            if (IsFinite(newRect.m_x, wxT("left curve boundary is NaN")) &&
                 IsFinite(newRect.m_y, wxT("bottom curve boundary is NaN")) &&
                 IsFinite(newRect.GetRight(), wxT("right curve boundary is NaN")) &&
                 IsFinite(newRect.GetBottom(), wxT("top curve boundary is NaN")) &&
                 (newRect.m_width >= 0) && (newRect.m_height >= 0))
            {
                if (!valid_rect) valid_rect = true;
                rect = newRect;
            }
        }

        // maybe just a single point, center it using default size
        bool zeroWidth = false, zeroHeight = false;

        if (rect.m_width == 0.0)
        {
            zeroWidth = true;
            rect.m_x = defaultPlotRect_.m_x;
            rect.m_width = defaultPlotRect_.m_width;
        }
        if (rect.m_height == 0.0)
        {
            zeroHeight = true;
            rect.m_y = defaultPlotRect_.m_y;
            rect.m_height = defaultPlotRect_.m_height;
        }

        curveBoundingRect_ = rect;

        // add some padding so the edge points can be seen
        double w = (!zeroWidth)  ? rect.m_width/50.0  : 0.0;
        double h = (!zeroHeight) ? rect.m_height/50.0 : 0.0;
        curveBoundingRect_.Inset(-w, -h, -w, -h);
    }
    else
        curveBoundingRect_ = defaultPlotRect_;

    AdjustScrollBars();
}

void wxPlotCtrl::Redraw(int need)
{
    if (batchCount_) return;

    if (need & REDRAW_BOTTOM_AXIS)
    {
        redrawNeed_ |= REDRAW_BOTTOM_AXIS;
        AutoCalcXAxisTicks();
        if (bottomAxisTicks_.correct_)
            CorrectXAxisTicks();
        CalcXAxisTickPositions();
    }
    if (need & REDRAW_LEFT_AXIS)
    {
        redrawNeed_ |= REDRAW_LEFT_AXIS;
        AutoCalcYAxisTicks();
        if (leftAxisTicks_.correct_)
            CorrectYAxisTicks();
        CalcYAxisTickPositions();
    }

    if (need & REDRAW_PLOT)
    {
        redrawNeed_ |= REDRAW_PLOT;
        area_->Refresh(false);
    }

    if (need & REDRAW_BOTTOM_AXIS)
        bottomAxis_->Refresh(false);
    if (need & REDRAW_LEFT_AXIS)
        leftAxis_->Refresh(false);

    if (need & REDRAW_WINDOW)
        Refresh();
}

void wxPlotCtrl::DrawAreaWindow(wxDC *dc, const wxRect &rect)
{
    wxCHECK_RET(dc, wxT("invalid dc"));

    // GTK doesn't like invalid parameters
    wxRect refreshRect = rect;
    wxRect clientRect(GetPlotAreaRect());
    refreshRect.Intersect(clientRect);

    if ((refreshRect.width == 0) || (refreshRect.height == 0)) return;

    dc->SetClippingRegion(refreshRect);

    dc->SetBrush(wxBrush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID));
    dc->SetPen(wxPen(GetBorderColour(), areaBorderWidth_, wxPENSTYLE_SOLID));
    dc->DrawRectangle(clientRect);

    if (GetDrawGrid())
        DrawGridLines(dc, refreshRect);
    if (GetDrawTicks())
        DrawTickMarks(dc, refreshRect);
    DrawMarkers(dc, refreshRect);

    dc->DestroyClippingRegion();

    int i;
    wxPlotData *curve;
    wxPlotData *activeCurve = GetActiveCurve();
    for (i = 0; i < GetCurveCount(); i++)
    {
        curve = GetCurve(i);

        if (curve != activeCurve)
            DrawDataCurve(dc, curve, i, refreshRect);
    }
    // active curve is drawn on top
    if (activeCurve)
        DrawDataCurve(dc, activeCurve, GetActiveIndex(), refreshRect);

    DrawCurveCursor(dc);
    DrawKey(dc);

    // refresh border
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(wxPen(GetBorderColour(), areaBorderWidth_, wxPENSTYLE_SOLID));
    dc->DrawRectangle(clientRect);

    dc->SetPen(wxNullPen);
    dc->SetBrush(wxNullBrush);
}

void wxPlotCtrl::DrawMouseMarker(wxDC *dc, MarkerType type, const wxRect &rect)
{
    wxCHECK_RET(dc, wxT("invalid window"));

    if ((rect.width == 0) || (rect.height == 0))
        return;

    wxRasterOperationMode logical_fn = dc->GetLogicalFunction();
    dc->SetLogicalFunction(wxINVERT);
    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, 1, wxPENSTYLE_DOT));

    switch (type)
    {
        case MarkerType::NONE:
            break;

        case MarkerType::RECT:
        {
            // rects are drawn to width and height - 1, doesn't line up w/ cursor, who cares?
            dc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
            break;
        }
        case MarkerType::VERT:
        {
            if (rect.width != 0)
            {
                int height = GetClientSize().y;
                dc->DrawLine(rect.x, 1, rect.x, height-2);
                dc->DrawLine(rect.GetRight()+1, 1, rect.GetRight()+1, height-2);
            }
            break;
        }
        case MarkerType::HORIZ:
        {
            if (rect.height != 0)
            {
                int width = GetClientSize().x;
                dc->DrawLine(1, rect.y, width-2, rect.y);
                dc->DrawLine(1, rect.GetBottom()+1, width-2, rect.GetBottom()+1);
            }
            break;
        }
        default:
            break;
    }

    dc->SetBrush(wxNullBrush);
    dc->SetPen(wxNullPen);
    dc->SetLogicalFunction(logical_fn);
}

void wxPlotCtrl::DrawCrosshairCursor(wxDC *dc, const wxPoint &pos)
{
    wxCHECK_RET(dc, wxT("invalid window"));

    dc->SetPen(*wxBLACK_PEN);
    wxRasterOperationMode logical_fn = dc->GetLogicalFunction();
    dc->SetLogicalFunction(wxINVERT);

    dc->CrossHair(pos.x, pos.y);

    dc->SetPen(wxNullPen);
    dc->SetLogicalFunction(logical_fn);
}

void wxPlotCtrl::DrawDataCurve(wxDC *dc, wxPlotData *curve, int curve_index, const wxRect &rect)
{
    wxCHECK_RET(dc && dataCurveDrawer_ && curve && curve->Ok(), wxT("invalid curve"));

    dataCurveDrawer_->SetDCRect(rect);
    dataCurveDrawer_->SetPlotViewRect(viewRect_);
    dataCurveDrawer_->Draw(dc, curve, curve_index);
}

void wxPlotCtrl::RedrawCurve(int index, int minIndex, int maxIndex)
{
    if (batchCount_) return;

    wxCHECK_RET((index>=0)&&(index<(int)curves_.GetCount()), wxT("invalid curve index"));

    wxPlotData *plotData = GetCurve(index);
    wxCHECK_RET(plotData, wxT("not a data curve"));

    int count = plotData->GetCount();
    wxCHECK_RET((minIndex <= maxIndex) && (minIndex >= 0) && (maxIndex >= 0) && (minIndex < count) && (maxIndex < count), wxT("invalid data index"));

    wxRect rect(areaClientRect_);

    wxMemoryDC mdc;
    mdc.SelectObject(area_->bitmap_);
    DrawDataCurve(&mdc, plotData, index, rect);
    DrawCurveCursor(&mdc);
    wxClientDC dc(area_);
    dc.Blit(rect.x, rect.y, rect.width, rect.height, &mdc, rect.x, rect.y);
    mdc.SelectObject(wxNullBitmap);
}

void wxPlotCtrl::DrawKey(wxDC *dc)
{
    wxCHECK_RET(dc && keyDrawer_, wxT("invalid window"));
    if (!GetShowKey() || keyString_.IsEmpty())
        return;

    wxRect dcRect(wxPoint(0, 0), GetPlotAreaRect().GetSize());
    keyDrawer_->SetDCRect(dcRect);
    keyDrawer_->SetPlotViewRect(viewRect_);
    keyDrawer_->Draw(dc, keyString_);
}

void wxPlotCtrl::DrawCurveCursor(wxDC *dc)
{
    wxCHECK_RET(dc, wxT("invalid window"));
    if (!IsCursorValid())
        return;

    markerDrawer_->SetPlotViewRect(viewRect_);
    markerDrawer_->SetDCRect(wxRect(wxPoint(0,0), area_->GetClientSize()));
    markerDrawer_->Draw(dc, cursorMarker_);
}

void wxPlotCtrl::DrawGridLines(wxDC *dc, const wxRect &rect)
{
    DrawTickMarksOrGridLines(dc, rect, false);
}

void wxPlotCtrl::DrawTickMarks(wxDC *dc, const wxRect &rect)
{
    DrawTickMarksOrGridLines(dc, rect, true);
}

void wxPlotCtrl::DrawTickMarksOrGridLines(wxDC *dc, const wxRect &rect, bool ticksOnly)
{
    wxRect clientRect(GetPlotAreaRect());
    dc->SetPen(wxPen(GetGridColour(), 1, wxPENSTYLE_SOLID));

    int xtickLength = ticksOnly ? 8 : clientRect.height;
    int ytickLength = ticksOnly ? 8 : clientRect.width;

    int tickPos, i;
    // X-axis ticks
    int tickCount = bottomAxisTicks_.positions_.GetCount();
    for (i = 0; i < tickCount; i++)
    {
        tickPos = bottomAxisTicks_.positions_[i];
        if (tickPos < rect.x)
            continue;
        else if (tickPos > rect.GetRight())
            break;

        dc->DrawLine(tickPos, clientRect.height, tickPos, clientRect.height - xtickLength);
    }

    // Y-axis ticks
    tickCount = leftAxisTicks_.positions_.GetCount();
    for (i=0; i<tickCount; i++)
    {
        tickPos = leftAxisTicks_.positions_[i];
        if (tickPos < rect.y)
            break;
        else if (tickPos > rect.GetBottom())
            continue;

        dc->DrawLine(0, tickPos, ytickLength, tickPos);
    }
}

void wxPlotCtrl::DrawMarkers(wxDC *dc, const wxRect &rect)
{
    wxCHECK_RET(markerDrawer_, wxT("Invalid marker drawer"));
    markerDrawer_->SetPlotViewRect(viewRect_);
    markerDrawer_->SetDCRect(rect);
    markerDrawer_->Draw(dc, plotMarkers_);
}

void wxPlotCtrl::DrawXAxis(wxDC *dc, bool refresh)
{
    wxCHECK_RET(bottomAxisDrawer_, wxT("Invalid x axis drawer"));

    bottomAxisDrawer_->SetPlotViewRect(viewRect_);
    wxSize clientSize = bottomAxisRect_.GetSize();
    bottomAxisDrawer_->SetDCRect(wxRect(wxPoint(0,0), clientSize));
    bottomAxisDrawer_->Draw(dc, refresh);
}

void wxPlotCtrl::DrawYAxis(wxDC *dc, bool refresh)
{
    wxCHECK_RET(leftAxisDrawer_, wxT("Invalid y axis drawer"));

    leftAxisDrawer_->SetPlotViewRect(viewRect_);
    wxSize clientSize = leftAxisRect_.GetSize();
    leftAxisDrawer_->SetDCRect(wxRect(wxPoint(0,0), clientSize));
    leftAxisDrawer_->Draw(dc, refresh);
}

wxRect ScaleRect(const wxRect &rect, double x_scale, double y_scale)
{
    return wxRect(int(rect.x*x_scale+0.5),     int(rect.y*y_scale+0.5),
                   int(rect.width*x_scale+0.5), int(rect.height*y_scale+0.5));
}

void wxPlotCtrl::DrawWholePlot(wxDC *dc, const wxRect &boundingRect, double dpi)
{
    wxCHECK_RET(dc, wxT("invalid dc"));
    wxCHECK_RET(dpi > 0, wxT("Invalid dpi for plot drawing"));

    //set font scale so 1pt = 1pixel at 72dpi
    double fontScale = (double)dpi / 72.0;
    //one pixel wide line equals (penPrintWidth_) millimeters wide
    double penScale = (double)penPrintWidth_ * dpi / 25.4;

    //save old values
    wxFont oldAxisFont      = GetAxisFont();
    wxFont oldAxisLabelFont = GetAxisLabelFont();
    wxFont oldPlotTitleFont = GetPlotTitleFont();
    wxFont oldKeyFont       = GetKeyFont();

    int oldAreaBorderWidth = areaBorderWidth_;
    int oldBorder = border_;
    int oldCursorSize = cursorMarker_.GetSize().x;

    wxPoint2DDouble old_zoom = zoom_;
    wxRect2DDouble  old_view = viewRect_;
    wxRect old_areaClientRect = areaClientRect_;

    //resize border and border pen
    areaBorderWidth_ = RINT(areaBorderWidth_ * penScale);
    border_ = RINT(border_ * penScale);

    //resize the curve cursor
    cursorMarker_.SetSize(wxSize(int(oldCursorSize * penScale), int(oldCursorSize * penScale)));

    //resize the fonts
    wxFont axisFont = GetAxisFont();
    axisFont.SetPointSize(wxMax(2, RINT(axisFont.GetPointSize() * fontScale)));
    SetAxisFont(axisFont);

    wxFont axisLabelFont = GetAxisLabelFont();
    axisLabelFont.SetPointSize(wxMax(2, RINT(axisLabelFont.GetPointSize() * fontScale)));
    SetAxisLabelFont(axisLabelFont);

    wxFont plotTitleFont = GetPlotTitleFont();
    plotTitleFont.SetPointSize(wxMax(2, RINT(plotTitleFont.GetPointSize() * fontScale)));
    SetPlotTitleFont(plotTitleFont);

    wxFont keyFont = GetKeyFont();
    keyFont.SetPointSize(wxMax(2, RINT(keyFont.GetPointSize() * fontScale)));
    SetKeyFont(keyFont);

    //reload the original zoom and view rect in case it was changed by any of the font changes
    zoom_     = old_zoom;
    viewRect_ = old_view;

    //resize all window component rects to the bounding rect
    DoSize(boundingRect, false);
    //AutoCalcTicks();  // don't reset ticks since it might not be WYSIWYG

    //reload the original zoom and view rect in case it was changed by any of the font changes
    zoom_ = wxPoint2DDouble(old_zoom.m_x * double(areaClientRect_.width)/old_areaClientRect.width,
                             old_zoom.m_y * double(areaClientRect_.height)/old_areaClientRect.height);

    wxPrintf(wxT("DPI %g, font %g pen%g\n"), dpi, fontScale, penScale);

    //draw all components to the provided dc
    dc->SetDeviceOrigin(long(boundingRect.x + bottomAxisRect_.GetLeft()),
                        long(boundingRect.y + bottomAxisRect_.GetTop()));
    CalcXAxisTickPositions();
    DrawXAxis(dc, false);

    dc->SetDeviceOrigin(long(boundingRect.x+leftAxisRect_.GetLeft()),
                        long(boundingRect.y+leftAxisRect_.GetTop()));
    CalcYAxisTickPositions();
    DrawYAxis(dc, false);

    dc->SetDeviceOrigin(long(boundingRect.x+areaRect_.GetLeft()),
                        long(boundingRect.y+areaRect_.GetTop()));
    DrawAreaWindow(dc, areaClientRect_);

    dc->SetDeviceOrigin(boundingRect.x, boundingRect.y);
    DrawPlotCtrl(dc);

    dc->SetBrush(*wxTRANSPARENT_BRUSH);
    dc->SetPen(*wxRED_PEN);
    dc->SetDeviceOrigin(boundingRect.x, boundingRect.y);
    dc->DrawRectangle(bottomAxisRect_);
    dc->DrawRectangle(leftAxisRect_);
    dc->DrawRectangle(areaRect_);



    //restore old values
    areaBorderWidth_ = oldAreaBorderWidth;
    border_ = oldBorder;
    cursorMarker_.SetSize(wxSize(oldCursorSize, oldCursorSize));

    SetAxisFont(oldAxisFont);
    SetAxisLabelFont(oldAxisLabelFont);
    SetPlotTitleFont(oldPlotTitleFont);
    SetKeyFont(oldKeyFont);
    zoom_ = old_zoom;
    viewRect_ = old_view;

    //update to window instead of printer
    UpdateWindowSize();
    Redraw(REDRAW_WHOLEPLOT); // recalc ticks for this window
}

// ----------------------------------------------------------------------------
// Axis tick calculations
// ----------------------
void wxPlotCtrl::AutoCalcXAxisTicks()
{
    DoAutoCalcTicks(true);
}
void wxPlotCtrl::AutoCalcYAxisTicks()
{
    DoAutoCalcTicks(false);
}

void wxPlotCtrl::DoAutoCalcTicks(bool xAxis)
{
    double start = 0.0, end = 1.0;
    int i, n, window = 100;

    double   *tickStep = NULL;
    int      *tickCount = NULL;
    wxString *tickFormat = NULL;

    if (xAxis)
    {
        tickStep  = &bottomAxisTicks_.step_;
        tickCount = &bottomAxisTicks_.count_;
        tickFormat = &bottomAxisTicks_.format_;

        window = GetPlotAreaRect().width;
        bottomAxisTicks_.positions_.Clear(); // kill it in case something goes wrong
        start = viewRect_.GetLeft();
        end   = viewRect_.GetRight();
        *tickCount = window/(axisFontSize_.x*10);
    }
    else
    {
        tickStep  = &leftAxisTicks_.step_;
        tickCount = &leftAxisTicks_.count_;
        tickFormat = &leftAxisTicks_.format_;

        window = GetPlotAreaRect().height;
        leftAxisTicks_.positions_.Clear();
        start = viewRect_.GetTop();
        end   = viewRect_.GetBottom();
        double tickCountScale = window/(axisFontSize_.y*2.0) > 2.0 ? 2.0 : 1.5;
        *tickCount = int(window/(axisFontSize_.y*tickCountScale) + 0.5);
    }

    if (window < 5) return; // FIXME

    if (!IsFinite(start, wxT("axis range is not finite")) ||
        !IsFinite(end, wxT("axis range is not finite")))
    {
        *tickCount = 0;
        return;
    }

    double range = end - start;
    double max = fabs(start) > fabs(end) ? fabs(start) : fabs(end);
    double min = fabs(start) < fabs(end) ? fabs(start) : fabs(end);
    bool exponential = (min >= minExponential_) || (max < 1.0/minExponential_) ? true : false;
    int places = exponential ? 1 : int(floor(fabs(log10(max))));

    if (!IsFinite(range, wxT("axis range is not finite")) ||
        !IsFinite(min,   wxT("axis range is not finite")) ||
        !IsFinite(max,   wxT("axis range is not finite")))
    {
        *tickCount = 0;
        return;
    }

    *tickStep = 1.0;
    int int_log_range = int(log10(range));
    if      (int_log_range > 0) {for (i=0; i <  int_log_range; i++) (*tickStep) *= 10;}
    else if (int_log_range < 0) {for (i=0; i < -int_log_range; i++) (*tickStep) /= 10;}

    double stepsizes[TIC_STEPS] = {.1, .2, .5};
    double step10 = (*tickStep) / 10.0;
    int sigFigs = 0;
    int digits = 0;

    for (n = 0; n < 4; ++n)
    {
        for (i = 0; i < TIC_STEPS; ++i)
        {
            *tickStep = step10 * stepsizes[i];

            if (exponential)
                sigFigs = labs(int(log10(max)) - int(log10(*tickStep)));
            else
                sigFigs = (*tickStep) >= 1.0 ? 0 : int(ceil(-log10(*tickStep)));

            if (xAxis)
            {
                digits = 1 + places + (sigFigs > 0 ? 1 + sigFigs : 0) + (exponential ? 4 : 0);
                *tickCount = int(double(window)/double((digits + 3) * axisFontSize_.x) + 0.5);
            }

            if ((range/(*tickStep)) <= (*tickCount)) break;
        }
        if ((range/(*tickStep)) <= (*tickCount)) break;
        step10 *= 10.0;
    }

    //if (!x_axis) wxPrintf(wxT("Ticks %d %lf, %d\n"), n, *tickStep, *tickCount);

    if (sigFigs > 9) sigFigs = 9; // FIXME

    if (exponential) tickFormat->Printf(wxT("%%.%dle"), sigFigs);
    else             tickFormat->Printf(wxT("%%.%dlf"), sigFigs);

    *tickCount = int(ceil(range / (*tickStep))) + 1;

//  note : first_tick = ceil(start / tickStep) * tickStep;
}

void wxPlotCtrl::CorrectXAxisTicks()
{
    double start = ceil(viewRect_.GetLeft() / bottomAxisTicks_.step_) * bottomAxisTicks_.step_;
    wxString label;
    label.Printf(bottomAxisTicks_.format_.c_str(), start);
    if (label.ToDouble(&start))
    {
        double x = GetClientCoordFromPlotX(start);
        double zoom_x = (GetClientCoordFromPlotX(start + bottomAxisTicks_.step_) - x) / bottomAxisTicks_.step_;
        double origin_x = start - x/zoom_x;
        BeginBatch();
        if (!SetZoom(zoom_x, zoom_.m_y, origin_x, viewRect_.GetTop(), true))
            bottomAxisTicks_.count_ = 0;  // oops

        EndBatch(false); // don't draw just block
    }
}
void wxPlotCtrl::CorrectYAxisTicks()
{
    double start = ceil(viewRect_.GetTop() / leftAxisTicks_.step_) * leftAxisTicks_.step_;
    wxString label;
    label.Printf(leftAxisTicks_.format_.c_str(), start);
    if (label.ToDouble(&start))
    {
        double y = GetClientCoordFromPlotY(start);
        double zoom_y = (y-GetClientCoordFromPlotY(start + leftAxisTicks_.step_)) / leftAxisTicks_.step_;
        double origin_y = start - (GetPlotAreaRect().height - y)/zoom_y;
        BeginBatch();
        if (!SetZoom(zoom_.m_x, zoom_y, viewRect_.GetLeft(), origin_y, true))
            leftAxisTicks_.count_ = 0;  // oops

        EndBatch(false);
    }
}

void wxPlotCtrl::CalcXAxisTickPositions()
{
    double current = ceil(viewRect_.GetLeft() / bottomAxisTicks_.step_) * bottomAxisTicks_.step_;
    bottomAxisTicks_.positions_.Clear();
    bottomAxisTicks_.labels_.Clear();
    int i, x, windowWidth = GetPlotAreaRect().width;
    for (i = 0; i < bottomAxisTicks_.count_; i++)
    {
        if (!IsFinite(current, wxT("axis label is not finite"))) return;

        x = GetClientCoordFromPlotX(current);

        if ((x >= -1) && (x < windowWidth + 2))
        {
            bottomAxisTicks_.positions_.Add(x);
            bottomAxisTicks_.labels_.Add(wxString::Format(bottomAxisTicks_.format_.c_str(), current));
        }

        current += bottomAxisTicks_.step_;
    }
}
void wxPlotCtrl::CalcYAxisTickPositions()
{
    double current = ceil(viewRect_.GetTop() / leftAxisTicks_.step_) * leftAxisTicks_.step_;
    leftAxisTicks_.positions_.Clear();
    leftAxisTicks_.labels_.Clear();
    int i, y, windowWidth = GetPlotAreaRect().height;
    for (i = 0; i < leftAxisTicks_.count_; i++)
    {
        if (!IsFinite(current, wxT("axis label is not finite")))
            return;

        y = GetClientCoordFromPlotY(current);

        if ((y >= -1) && (y < windowWidth+2))
        {
            leftAxisTicks_.positions_.Add(y);
            leftAxisTicks_.labels_.Add(wxString::Format(leftAxisTicks_.format_.c_str(), current));
        }

        current += leftAxisTicks_.step_;
    }
}

// ----------------------------------------------------------------------------
// Event processing
// ----------------------------------------------------------------------------

void wxPlotCtrl::ProcessAreaMouseEvent(wxMouseEvent &event)
{
    wxPoint &mousePt   = area_->lastMousePosition_;
    wxRect &mouseDragRectangle = area_->mouseDragRectangle_;

    wxPoint lastMousePt = mousePt;
    mousePt = event.GetPosition();

    if (event.ButtonDown() && IsTextCtrlShown())
    {
        HideTextCtrl(true, true);
        return;
    }

    if (GetGreedyFocus() && (FindFocus() != area_))
        area_->SetFocus();

    double plotX = GetPlotCoordFromClientX(mousePt.x),
           plotY = GetPlotCoordFromClientY(mousePt.y);

    wxClientDC dc(area_);

    // Mouse motion
    if (lastMousePt != area_->lastMousePosition_)
    {
        wxPlotCtrlEvent evt_motion(wxEVT_PLOTCTRL_MOUSE_MOTION, GetId(), this);
        evt_motion.SetPosition(plotX, plotY);
        DoSendEvent(evt_motion);

        // Draw the crosshair cursor
        if (GetCrossHairCursor())
        {
            if (!event.Entering() || area_->HasCapture())
                DrawCrosshairCursor(&dc, lastMousePt);
            if (!event.Leaving() || area_->HasCapture())
                DrawCrosshairCursor(&dc, mousePt);
        }
    }

    // Wheel scrolling up and down
    if (event.GetWheelRotation() != 0)
    {
        double dir = event.GetWheelRotation() > 0 ? 0.25 : -0.25;
        SetOrigin(viewRect_.GetLeft(),
                   viewRect_.GetTop() + dir*viewRect_.m_height, true);
    }

    int active_index = GetActiveIndex();

    // Initial Left down selection
    if (event.LeftDown() || event.LeftDClick())
    {
        if (FindFocus() != area_) // fixme MSW focus problems
            area_->SetFocus();

        if (areaMouseCursorid_ == wxCURSOR_HAND)
            SetAreaMouseCursor(CURSOR_GRAB);

        // send a click or doubleclick event
        wxPlotCtrlEvent click_event(event.ButtonDClick() ? wxEVT_PLOTCTRL_DOUBLECLICKED : wxEVT_PLOTCTRL_CLICKED,
                                 GetId(), this);
        click_event.SetPosition(plotX, plotY);
        DoSendEvent(click_event);

        if (!event.ButtonDClick())
            mouseDragRectangle = wxRect(mousePt, wxSize(0, 0));

        int data_index = -1;
        int curveIndex = -1;

        wxPoint2DDouble dpt(2.0/zoom_.m_x, 2.0/zoom_.m_y);
        wxPoint2DDouble curvePt;

        if (FindCurve(wxPoint2DDouble(plotX, plotY), dpt, curveIndex, data_index, &curvePt))
        {
            wxPlotData *plotData = GetCurve(curveIndex);

            if (plotData)
            {
                wxPlotCtrlEvent pt_click_event(event.ButtonDClick() ? wxEVT_PLOTCTRL_POINT_DOUBLECLICKED : wxEVT_PLOTCTRL_POINT_CLICKED,
                                            GetId(), this);
                pt_click_event.SetPosition(curvePt.m_x, curvePt.m_y);
                pt_click_event.SetCurve(plotData, curveIndex);
                pt_click_event.SetCurveDataIndex(data_index);
                DoSendEvent(pt_click_event);

                // send curve selection switched event
                if (curveIndex != GetActiveIndex())
                    SetActiveIndex(curveIndex, true);

                if (!event.LeftDClick() && (areaMouseFunc_ == MouseFunction::SELECT))
                    SelectDataRange(curveIndex, wxRangeInt(data_index,data_index), true);
                else if (!event.LeftDClick() && (areaMouseFunc_ == MouseFunction::DESELECT))
                    DeselectDataRange(curveIndex, wxRangeInt(data_index,data_index), true);
                else
                    SetCursorDataIndex(curveIndex, data_index, true);

                return;
            }
        }
    }
    // Finished marking rectangle or scrolling, perhaps
    else if (event.LeftUp())
    {
        SetCaptureWindow(NULL);

        if (areaMouseCursorid_ == CURSOR_GRAB)
            SetAreaMouseCursor(wxCURSOR_HAND);

        StopMouseTimer();

        if (mouseDragRectangle == wxRect(0, 0, 0, 0))
            return;

        wxRect rightedRect = mouseDragRectangle;

        // rightedRect always goes from upper-left to lower-right
        //   don't fix mouseDragRectangle since redrawing will be off
        if (rightedRect.width < 0)
        {
            rightedRect.x += rightedRect.width;
            rightedRect.width = -rightedRect.width;
        }
        if (rightedRect.height < 0)
        {
            rightedRect.y += rightedRect.height;
            rightedRect.height = -rightedRect.height;
        }

        // Zoom into image
        if (areaMouseFunc_ == MouseFunction::ZOOM)
        {
            if ((areaMouseMarker_ == MarkerType::RECT) &&
                ((rightedRect.width > 10) && (rightedRect.height > 10)))
                SetZoom(rightedRect, true);
            else if ((areaMouseMarker_ == MarkerType::VERT) &&
                     (rightedRect.width > 10))
                SetZoom(wxRect(rightedRect.x, 0, rightedRect.width, areaClientRect_.height), true);
            else if ((areaMouseMarker_ == MarkerType::HORIZ) &&
                     (rightedRect.height > 10))
                SetZoom(wxRect(0, rightedRect.y, areaClientRect_.width, rightedRect.height), true);
            else
                DrawMouseMarker(&dc, areaMouseMarker_, mouseDragRectangle);
        }
        // Select a range of points
        else if ((areaMouseFunc_ == MouseFunction::SELECT) && (active_index >= 0))
        {
            BeginBatch(); // if you select nothing, you don't get a refresh

            wxRect2DDouble plotRect = GetPlotRectFromClientRect(rightedRect);

            if ((areaMouseMarker_ == MarkerType::VERT) && (plotRect.m_width > 0))
                SelectXRange(active_index, plotRect.m_x, plotRect.GetRight(), true);
            else if ((areaMouseMarker_ == MarkerType::HORIZ) && (plotRect.m_height > 0))
                SelectYRange(active_index, plotRect.m_y, plotRect.GetBottom(), true);
            else if ((plotRect.m_width > 0) || (plotRect.m_height > 0))
                SelectRectangle(active_index, plotRect, true);

            mouseDragRectangle = wxRect(0,0,0,0);
            EndBatch();
        }
        // Deselect a range of points
        else if ((areaMouseFunc_ == MouseFunction::DESELECT) && (active_index >= 0))
        {
            BeginBatch();

            wxRect2DDouble plotRect = GetPlotRectFromClientRect(rightedRect);

            if ((areaMouseMarker_ == MarkerType::VERT) && (plotRect.m_width > 0))
                DeselectXRange(active_index, plotRect.m_x, plotRect.GetRight(), true);
            else if ((areaMouseMarker_ == MarkerType::HORIZ) && (plotRect.m_height > 0))
                DeselectYRange(active_index, plotRect.m_y, plotRect.GetBottom(), true);
            else if ((plotRect.m_width > 0) || (plotRect.m_height > 0))
                DeselectRectangle(active_index, plotRect, true);

            mouseDragRectangle = wxRect(0,0,0,0);
            EndBatch();
        }
        // Nothing to do - erase the rect
        else
        {
            DrawMouseMarker(&dc, areaMouseMarker_, mouseDragRectangle);
        }

        mouseDragRectangle = wxRect(0,0,0,0);
        return;
    }
    // Marking the rectangle or panning around
    else if (event.LeftIsDown() && event.Dragging())
    {
        SetCaptureWindow(area_);

        if (areaMouseCursorid_ == wxCURSOR_HAND)
            SetAreaMouseCursor(CURSOR_GRAB);

        // Move the origin
        if (areaMouseFunc_ == MouseFunction::PAN)
        {
            if (!areaClientRect_.Contains(event.GetPosition()))
            {
                StartMouseTimer(ID_AREA_TIMER);
            }

            mouseDragRectangle = wxRect(0,0,0,0); // no marker

            double dx = mousePt.x - lastMousePt.x;
            double dy = mousePt.y - lastMousePt.y;
            SetOrigin(viewRect_.GetLeft() - dx/zoom_.m_x,
                      viewRect_.GetTop()  + dy/zoom_.m_y, true);
            return;
        }
        else
        {
            if (mouseDragRectangle != wxRect(0,0,0,0))
                DrawMouseMarker(&dc, areaMouseMarker_, mouseDragRectangle);
            else
                mouseDragRectangle = wxRect(mousePt, wxSize(1, 1));

            mouseDragRectangle.width  = mousePt.x - mouseDragRectangle.x;
            mouseDragRectangle.height = mousePt.y - mouseDragRectangle.y;

            DrawMouseMarker(&dc, areaMouseMarker_, mouseDragRectangle);
        }

        return;
    }
    return;
}

void wxPlotCtrl::ProcessAxisMouseEvent(wxMouseEvent &event)
{
    if (event.ButtonDown() && IsTextCtrlShown())
    {
        HideTextCtrl(true, true);
        return;
    }

    wxPoint pos = event.GetPosition();
    Axis *axisWin = (wxPlotCtrl::Axis*)event.GetEventObject();
    wxCHECK_RET(axisWin, wxT("Unknown window"));

    wxPoint &mousePt = axisWin->lastMousePosition_;

    if (event.LeftIsDown() && (axisWin != GetCaptureWindow()))
    {
        SetCaptureWindow(axisWin);
        mousePt = pos;
        return;
    }
    else if (!event.LeftIsDown())
    {
        SetCaptureWindow(NULL);
        StopMouseTimer();
    }
    else if (event.LeftIsDown())
    {
        wxSize winSize = axisWin->GetSize();

        if ((axisWin->IsXAxis() && ((pos.x < 0) || (pos.x > winSize.x))) ||
            (!axisWin->IsXAxis() && ((pos.y < 0) || (pos.y > winSize.y))))
        {
            mousePt = pos;
            StartMouseTimer(axisWin->IsXAxis() ? ID_XAXIS_TIMER : ID_YAXIS_TIMER);
        }
        else if (IsTimerRunning())
            mousePt = pos;
    }

    int wheel = event.GetWheelRotation();

    if (wheel != 0)
    {
        wheel = wheel > 0 ? 1 : wheel < 0 ? -1 : 0;
        double dx = 0, dy = 0;

        if (axisWin->IsXAxis())
            dx = wheel*viewRect_.m_width/4.0;
        else
            dy = wheel*viewRect_.m_height/4.0;

        SetOrigin(viewRect_.GetLeft() + dx,
                  viewRect_.GetTop()  + dy, true);
    }

    if ((event.LeftIsDown() && event.Dragging()) || event.LeftUp())
    {
        double x = viewRect_.GetLeft();
        double y = viewRect_.GetTop();

        if (axisWin->IsXAxis())
            x += (pos.x - mousePt.x)/zoom_.m_x;
        else
            y += (mousePt.y-pos.y)/zoom_.m_y;

        SetOrigin(x, y, true);
    }

    mousePt = pos;
}

void wxPlotCtrl::ProcessAreaCharEvent(wxKeyEvent &event)
{
    OnChar(event);
}

void wxPlotCtrl::ProcessAreaKeyDownEvent(wxKeyEvent &event)
{
    //wxPrintf(wxT("wxPlotCtrl::ProcessAreaKeyDownEvent %d `%c` S%dC%dA%d\n"), int(event.GetKeyCode()), (wxChar)event.GetKeyCode(), event.ShiftDown(), event.ControlDown(), event.AltDown());
    event.Skip(true);

    int  code  = event.GetKeyCode();
    bool alt   = event.AltDown()     || (code == WXK_ALT);
    bool ctrl  = event.ControlDown() || (code == WXK_CONTROL);
    bool shift = event.ShiftDown()   || (code == WXK_SHIFT);

    if (shift && !alt && !ctrl)
        SetAreaMouseFunction(MouseFunction::SELECT, true);
    else if (!shift && ctrl && !alt)
        SetAreaMouseFunction(MouseFunction::DESELECT, true);
    else if (ctrl && shift && alt)
        SetAreaMouseFunction(MouseFunction::PAN, true);
    else // if (!ctrl || !shift || !alt)
        SetAreaMouseFunction(MouseFunction::ZOOM, true);
}

void wxPlotCtrl::ProcessAreaKeyUpEvent(wxKeyEvent &event)
{
    //wxPrintf(wxT("wxPlotCtrl::ProcessAreaKeyUpEvent %d `%c` S%dC%dA%d\n"), int(event.GetKeyCode()), (wxChar)event.GetKeyCode(), event.ShiftDown(), event.ControlDown(), event.AltDown());
    event.Skip(true);

    int  code  = event.GetKeyCode();
    bool alt   = event.AltDown()     && (code != WXK_ALT);
    bool ctrl  = event.ControlDown() && (code != WXK_CONTROL);
    bool shift = event.ShiftDown()   && (code != WXK_SHIFT);

    if (shift && !ctrl && !alt)
        SetAreaMouseFunction(MouseFunction::SELECT, true);
    else if (!shift && ctrl && !alt)
        SetAreaMouseFunction(MouseFunction::DESELECT, true);
    else if (shift && ctrl && alt)
        SetAreaMouseFunction(MouseFunction::PAN, true);
    else // if (!shift && !ctrl && !alt)
        SetAreaMouseFunction(MouseFunction::ZOOM, true);
}

void wxPlotCtrl::ProcessAxisCharEvent(wxKeyEvent &event)
{
    OnChar(event);
}

void wxPlotCtrl::OnChar(wxKeyEvent &event)
{
    //wxPrintf(wxT("wxPlotCtrl::OnChar %d `%c` S%dC%dA%d\n"), int(event.GetKeyCode()), (wxChar)event.GetKeyCode(), event.ShiftDown(), event.ControlDown(), event.AltDown());

    // select the next curve if possible, or cursor point like left mouse
    if (event.GetKeyCode() == WXK_SPACE)
    {
        if (event.ShiftDown() || event.ControlDown())
        {
            if (IsCursorValid())
                DoSelectDataRange(cursorCurve_, wxRangeInt(cursorIndex_,cursorIndex_), !event.ControlDown(), true);
        }
        else
        {
            int count = GetCurveCount();
            if ((count < 1) || ((count == 1) && (activeIndex_ == 0))) return;
            int index = (activeIndex_ + 1 > count - 1) ? 0 : activeIndex_ + 1;
            SetActiveIndex(index, true);
        }
        return;
    }

    // These are reserved for the program
    if (event.ControlDown() || event.AltDown())
    {
        event.Skip(true);
        return;
    }

    switch (event.GetKeyCode())
    {
        // cursor keys moves the plot origin around
        case WXK_LEFT    : SetOrigin(viewRect_.GetLeft() - viewRect_.m_width/10.0, viewRect_.GetTop()); return;
        case WXK_RIGHT   : SetOrigin(viewRect_.GetLeft() + viewRect_.m_width/10.0, viewRect_.GetTop()); return;
        case WXK_UP      : SetOrigin(viewRect_.GetLeft(), viewRect_.GetTop() + viewRect_.m_height/10.0); return;
        case WXK_DOWN    : SetOrigin(viewRect_.GetLeft(), viewRect_.GetTop() - viewRect_.m_height/10.0); return;
        case WXK_PAGEUP  : SetOrigin(viewRect_.GetLeft(), viewRect_.GetTop() + viewRect_.m_height/2.0); return;
        case WXK_PAGEDOWN: SetOrigin(viewRect_.GetLeft(), viewRect_.GetTop() - viewRect_.m_height/2.0); return;

        // Center the plot on the cursor point, or 0,0
        case WXK_HOME :
        {
            if (IsCursorValid())
                MakeCursorVisible(true, true);
            else
                SetOrigin(-viewRect_.m_width/2.0, -viewRect_.m_height/2.0, true);

            return;
        }
        // try to make the current curve fully visible
        case WXK_END :
        {
            wxPlotData *plotData = GetActiveCurve();
            if (plotData)
            {
                wxRect2DDouble bound = plotData->GetBoundingRect();
                bound.Inset(-bound.m_width/80.0, -bound.m_height/80.0);
                SetViewRect(bound, true);
            }

            return;
        }

        // zoom in and out
        case wxT('a'): SetZoom(wxPoint2DDouble(zoom_.m_x/1.5, zoom_.m_y), true); return;
        case wxT('d'): SetZoom(wxPoint2DDouble(zoom_.m_x*1.5, zoom_.m_y), true); return;
        case wxT('w'): SetZoom(wxPoint2DDouble(zoom_.m_x, zoom_.m_y*1.5), true); return;
        case wxT('x'): SetZoom(wxPoint2DDouble(zoom_.m_x, zoom_.m_y/1.5), true); return;

        case wxT('q'): SetZoom(wxPoint2DDouble(zoom_.m_x/1.5, zoom_.m_y*1.5), true); return;
        case wxT('e'): SetZoom(wxPoint2DDouble(zoom_.m_x*1.5, zoom_.m_y*1.5), true); return;
        case wxT('z'): SetZoom(wxPoint2DDouble(zoom_.m_x/1.5, zoom_.m_y/1.5), true); return;
        case wxT('c'): SetZoom(wxPoint2DDouble(zoom_.m_x*1.5, zoom_.m_y/1.5), true); return;

        case wxT('='): {wxRect2DDouble r = GetViewRect(); r.Scale(.67); r.SetCentre(GetAreaMousePoint()); SetViewRect(r, true); return;}
        case wxT('-'): {wxRect2DDouble r = GetViewRect(); r.Scale(1.5); r.SetCentre(GetAreaMousePoint()); SetViewRect(r, true); return;}

        case wxT('s'): MakeCurveVisible(GetActiveIndex(), true); break;

        // Select previous/next point in a curve
        case wxT('<'): case wxT(','):
        {
            double x = GetPlotCoordFromClientX(areaClientRect_.width - 1);
            wxPlotData *plotData = GetActiveCurve();
            if (plotData)
            {
                if (!IsCursorValid())
                    SetCursorDataIndex(activeIndex_, plotData->GetIndexFromX(x, wxPlotData::IndexType::floor), true);
                else if (cursorIndex_ > 0)
                    SetCursorDataIndex(cursorCurve_, cursorIndex_ - 1, true);
            }

            MakeCursorVisible(false, true);

            return;
        }
        case wxT('>'): case wxT('.'):
        {
            double x = GetPlotCoordFromClientX(0);
            wxPlotData *plotData = GetActiveCurve();
            if (plotData)
            {
                int count = plotData->GetCount();

                if (!IsCursorValid())
                    SetCursorDataIndex(activeIndex_, plotData->GetIndexFromX(x, wxPlotData::IndexType::ceil), true);
                else if (cursorIndex_ < count - 1)
                    SetCursorDataIndex(cursorCurve_, cursorIndex_ + 1, true);
            }

            MakeCursorVisible(false, true);

            return;
        }

        // go to the last or next zoom
        case wxT('[') : NextHistoryView(false, true); return;
        case wxT(']') : NextHistoryView(true,  true); return;

        // delete the selected curve
        case WXK_DELETE:
        {
            if (activeCurve_) DeleteCurve(activeCurve_, true);
            return;
        }
        // delete current selection or go to next curve and delete it's selection
        //   finally invalidate cursor
        case WXK_ESCAPE :
        {
            BeginBatch();
            if ((activeIndex_ >= 0) && (GetSelectedRangeCount(activeIndex_) > 0))
            {
                ClearSelectedRanges(activeIndex_, true);
            }
            else
            {
                bool has_cleared = false;

                for (int i=0; i<GetCurveCount(); i++)
                {
                    if (GetSelectedRangeCount(i) > 0)
                    {
                        ClearSelectedRanges(i, true);
                        has_cleared = true;
                        break;
                    }
                }

                if (!has_cleared)
                {
                    if (IsCursorValid())
                        InvalidateCursor(true);
                    else if (activeIndex_ > -1)
                        SetActiveIndex(-1, true);
                }
            }
            EndBatch(); // ESC is also a generic clean up routine too!
            break;
        }

        default: event.Skip(true); break;
    }
}

void wxPlotCtrl::UpdateWindowSize()
{
    areaClientRect_ = wxRect(wxPoint(0,0), area_->GetClientSize());
    // If something happens to make these true, there's a problem
    if (areaClientRect_.width  < 10) areaClientRect_.width  = 10;
    if (areaClientRect_.height < 10) areaClientRect_.height = 10;
}

void wxPlotCtrl::AdjustScrollBars()
{
    double range, thumbsize, position;
    double pagesize;

    range = (curveBoundingRect_.m_width * zoom_.m_x);
    if (!IsFinite(range, wxT("plot's x range is NaN"))) return;
    if (range > 32000) range = 32000; else if (range < 1) range = 1;

    thumbsize = (range * (viewRect_.m_width/curveBoundingRect_.m_width));
    if (!IsFinite(thumbsize, wxT("plot's x range is NaN"))) return;
    if (thumbsize > range) thumbsize = range; else if (thumbsize < 1) thumbsize = 1;

    position = (range * ((viewRect_.GetLeft() - curveBoundingRect_.GetLeft())/curveBoundingRect_.m_width));
    if (!IsFinite(position, wxT("plot's x range is NaN"))) return;
    if (position > range - thumbsize) position = range - thumbsize; else if (position < 0) position = 0;
    pagesize = thumbsize;

    xAxisScrollbar_->SetScrollbar(int(position), int(thumbsize), int(range), int(pagesize));

    range = (curveBoundingRect_.m_height * zoom_.m_y);
    if (!IsFinite(range, wxT("plot's y range is NaN"))) return;
    if (range > 32000) range = 32000; else if (range < 1) range = 1;

    thumbsize = (range * (viewRect_.m_height/curveBoundingRect_.m_height));
    if (!IsFinite(thumbsize, wxT("plot's x range is NaN"))) return;
    if (thumbsize > range) thumbsize = range; else if (thumbsize < 1) thumbsize = 1;

    position = (range - range * ((viewRect_.GetTop() - curveBoundingRect_.GetTop())/curveBoundingRect_.m_height) - thumbsize);
    if (!IsFinite(position, wxT("plot's x range is NaN"))) return;
    if (position > range - thumbsize) position = range - thumbsize; else if (position < 0) position = 0;
    pagesize = thumbsize;

    yAxisScrollbar_->SetScrollbar(int(position), int(thumbsize), int(range), int(pagesize));
}

void wxPlotCtrl::OnScroll(wxScrollEvent &event)
{
    if (event.GetId() == ID_X_SCROLLBAR)
    {
        double range = xAxisScrollbar_->GetRange();
        if (range < 1)
            return;
        double position = xAxisScrollbar_->GetThumbPosition();
        double origin_x = curveBoundingRect_.GetLeft() + curveBoundingRect_.m_width*(position/range);
        if (!IsFinite(origin_x, wxT("plot's x-origin is NaN")))
            return;
        viewRect_.m_x = origin_x;
        Redraw(REDRAW_PLOT | REDRAW_BOTTOM_AXIS);
    }
    else if (event.GetId() == ID_Y_SCROLLBAR)
    {
        double range = yAxisScrollbar_->GetRange();
        if (range < 1)
            return;
        double position = yAxisScrollbar_->GetThumbPosition();
        double thumbsize = yAxisScrollbar_->GetThumbSize();
        double origin_y = curveBoundingRect_.GetTop() + curveBoundingRect_.m_height*((range-position-thumbsize)/range);
        if (!IsFinite(origin_y, wxT("plot's y-origin is NaN")))
            return;
        viewRect_.m_y = origin_y;
        Redraw(REDRAW_PLOT | REDRAW_LEFT_AXIS);
    }
}

bool wxPlotCtrl::IsFinite(double n, const wxString &msg) const
{
    if (!wxFinite(n))
    {
        if (!msg.IsEmpty())
        {
            wxPlotCtrlEvent event(wxEVT_PLOTCTRL_ERROR, GetId(), (wxPlotCtrl*)this);
            event.SetString(msg);
            DoSendEvent(event);
        }

        return false;
    }

    return true;
}

bool wxPlotCtrl::FindCurve(const wxPoint2DDouble &pt, const wxPoint2DDouble &dpt,
                           int &curveIndex, int &data_index, wxPoint2DDouble *curvePt) const
{
    curveIndex = data_index = -1;

    if (!IsFinite(pt.m_x,  wxT("point is not finite"))) return false;
    if (!IsFinite(pt.m_y,  wxT("point is not finite"))) return false;
    if (!IsFinite(dpt.m_x, wxT("point is not finite"))) return false;
    if (!IsFinite(dpt.m_y, wxT("point is not finite"))) return false;

    int curve_count = GetCurveCount();
    if (curve_count < 1) return false;

    for (int n=-1; n<curve_count; n++)
    {
        // find the point in the selected curve first
        if (n == -1)
        {
            if (activeIndex_ >= 0)
                n = activeIndex_;
            else
                n = 0;
        }
        else if (n == activeIndex_)
            continue;

        wxPlotData *plotData = GetCurve(n);

        // find the index of the closest point in a wxPlotData curve
        if (plotData)
        {
            // check if curve has BoundingRect
            wxRect2DDouble rect = plotData->GetBoundingRect();
            if (((rect.m_width > 0) &&
                 ((pt.m_x+dpt.m_x < rect.GetLeft()) || (pt.m_x-dpt.m_x > rect.GetRight()))) ||
                 ((rect.m_height > 0) &&
                 ((pt.m_y+dpt.m_y < rect.GetTop()) || (pt.m_y-dpt.m_y > rect.GetBottom()))))
            {
                if ((n == activeIndex_) && (n > 0)) n = -1; // start back at 0
                continue;
            }

            int index = plotData->GetIndexFromXY(pt.m_x, pt.m_y, dpt.m_x);

            double x = plotData->GetXValue(index);
            double y = plotData->GetYValue(index);

            if ((fabs(x-pt.m_x) <= dpt.m_x) && (fabs(y-pt.m_y) <= dpt.m_y))
            {
                curveIndex = n;
                data_index = index;
                if (curvePt) *curvePt = wxPoint2DDouble(x, y);
                return true;
            }
        }
        // continue searching through curves
        // if on the current then start back at the beginning if not already at 0
        if ((n == activeIndex_) && (n > 0)) n = -1;
    }
    return false;
}

bool wxPlotCtrl::DoSendEvent(wxPlotCtrlEvent &event) const
{
/*
    if (event.GetEventType() != wxEVT_PLOTCTRL_MOUSE_MOTION)
    {
        wxLogDebug(wxT("wxPlotCtrlEvent '%s' CurveIndex: %d, DataIndex: %d, Pos: %lf %lf, MouseFn %d"),
            wxPlotCtrl_GetEventName(event.GetEventType).c_str(),
            event.GetCurveIndex(), event.GetCurveDataIndex(),
            event.GetX(), event.GetY(), event.GetMouseFunction());
    }
*/
    return !GetEventHandler()->ProcessEvent(event) || event.IsAllowed();
}

void wxPlotCtrl::StartMouseTimer(wxWindowID win_id)
{
    if (timer_ && (timer_->GetId() != win_id))
        StopMouseTimer();

    if (!timer_)
        timer_ = new wxTimer(this, win_id);

    if (!timer_->IsRunning())
        timer_->Start(200, true); // one shot timer
}
void wxPlotCtrl::StopMouseTimer()
{
    if (timer_)
    {
        if (timer_->IsRunning())
            timer_->Stop();

        delete timer_;
        timer_ = NULL;
    }
}

bool wxPlotCtrl::IsTimerRunning()
{
    return (timer_ && timer_->IsRunning());
}

void wxPlotCtrl::OnTimer(wxTimerEvent &event)
{
    wxPoint mousePt;

    if (event.GetId() == ID_AREA_TIMER)
        mousePt = area_->lastMousePosition_;
    else if (event.GetId() == ID_XAXIS_TIMER)
        mousePt = bottomAxis_->lastMousePosition_;
    else if (event.GetId() == ID_YAXIS_TIMER)
        mousePt = leftAxis_->lastMousePosition_;
    else
    {
        event.Skip(); // someone else's timer?
        return;
    }

    double dx = (mousePt.x<0) ? -20 : (mousePt.x>GetPlotAreaRect().width) ?  20 : 0;
    double dy = (mousePt.y<0) ?  20 : (mousePt.y>GetPlotAreaRect().height) ? -20 : 0;
    dx /= zoom_.m_x;
    dy /= zoom_.m_y;

    if (((dx == 0) && (dy == 0)) ||
        !SetOrigin(GetViewRect().GetLeft() + dx, GetViewRect().GetTop() + dy, true))
    {
        StopMouseTimer();
    }
    else
        StartMouseTimer(event.GetId()); // restart timer for another round
}

void wxPlotCtrl::SetCaptureWindow(wxWindow *win)
{
    if (winCapture_ && (winCapture_ != win) && winCapture_->HasCapture())
        winCapture_->ReleaseMouse();

    winCapture_ = win;

    if (winCapture_ && (!winCapture_->HasCapture()))
        winCapture_->CaptureMouse();
}

wxWindow *wxPlotCtrl::GetCaptureWindow() const
{
    return winCapture_;
}


