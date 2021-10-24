/////////////////////////////////////////////////////////////////////////////
// Name:        plotctrl.h
// Purpose:     wxPlotCtrl
// Author:      John Labenski, Robert Roebling
// Modified by:
// Created:     6/5/2002
// Copyright:   (c) John Labenski, Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PLOTCTRL_H_
#define _WX_PLOTCTRL_H_

#include <limits>

#include "wx/bitmap.h"
#include "wx/window.h"

#include "wx/plotctrl/plotdata.h"
#include "wx/plotctrl/plotmark.h"
#include "wx/plotctrl/range.h"

// Check if value is >= min_val and <= max_val
#define RINT(x) (int((x) >= 0 ? ((x) + 0.5) : ((x) - 0.5)))

class wxScrollBar;

class wxPlotCtrlEvent;

class wxPlotDrawerArea;
class wxPlotDrawerXAxis;
class wxPlotDrawerYAxis;
class wxPlotDrawerKey;
class wxPlotDrawerDataCurve;
class wxPlotDrawerMarker;

//-----------------------------------------------------------------------------
// wxPlot Constants
//-----------------------------------------------------------------------------

extern std::numeric_limits<wxDouble> wxDouble_limits;

extern const wxDouble wxPlot_MIN_DBL;
extern const wxDouble wxPlot_MAX_DBL;
extern const wxDouble wxPlot_MAX_RANGE;

#define CURSOR_GRAB (wxCURSOR_MAX+100)  // A hand cursor with fingers closed

WX_DECLARE_OBJARRAY_WITH_DECL(wxPoint2DDouble, wxArrayPoint2DDouble, class);
WX_DECLARE_OBJARRAY_WITH_DECL(wxRect2DDouble,  wxArrayRect2DDouble, class);
WX_DECLARE_OBJARRAY_WITH_DECL(wxPlotData,      wxArrayPlotData, class);



//-----------------------------------------------------------------------------
// wxPlotCtrl - window to display wxPlotData, public interface
//
// notes:
//    call CalcBoundingRect() whenever you modify a curve's values so that
//    the default size of the plot is correct, see wxPlotData::GetBoundingRect
//
//-----------------------------------------------------------------------------

class wxPlotCtrl: public wxWindow
{
public:
    wxPlotCtrl();
    wxPlotCtrl(wxWindow *parent, wxWindowID win_id = wxID_ANY,
               const wxPoint &pos = wxDefaultPosition,
               const wxSize &size = wxDefaultSize,
               const wxString &name = wxT("wxPlotCtrl"));
    bool Create(wxWindow *parent, wxWindowID id = wxID_ANY,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                const wxString &name = wxT("wxPlotCtrl"));

    virtual ~wxPlotCtrl();

    // What title, axis label the textctrl should edit
    enum class TextCtrlType
    {
        EDIT_TITLE,
        EDIT_BOTTOM_AXIS_LABEL,
        EDIT_LEFT_AXIS_LABEL,
    };

    // What is the function of the mouse during left down and dragging
    enum class MouseFunction
    {
        NOTHING,   // do nothing
        ZOOM,      // zoom into the plot
        SELECT,    // select points in the active curve
        DESELECT,  // deselect points in the active curve
        PAN        // offset the origin
    };

    // What sort of marker should be drawn for mouse left down and dragging
    enum class MarkerType
    {
        NONE,   // draw nothing
        RECT,   // draw a rectangle
        VERT,   // draw two vertical lines
        HORIZ   // draw two horizonal lines
    };

// How does the selection mechanism act to selections
    enum SelectionType
    {
        NONE,         // no selections
        SINGLE,       // only one selection in one curve at a time
        SINGLE_CURVE, // only one curve may have selections at once
        SINGLE_PER_CURVE, // multiple curves may one have one selection each
        MULTIPLE      // multiple curves may have multiple selections
    };

//-----------------------------------------------------------------------------
// wxPlotCtrl::Area - window where the plot is drawn (privately used in wxPlotCtrl)
//-----------------------------------------------------------------------------

    class Area: public wxWindow
    {
    public:
        Area(wxPlotCtrl *parent, wxWindowID id);
        bool Create(wxPlotCtrl *parent, wxWindowID id);
        virtual ~Area() = default;

        void CreateBitmap(const wxRect &rect);

        void OnEraseBackground(wxEraseEvent &){}
        void OnPaint(wxPaintEvent &event);
        void OnMouse(wxMouseEvent &event);
        void OnChar(wxKeyEvent &event);
        void OnKeyDown(wxKeyEvent &event);
        void OnKeyUp(wxKeyEvent &event);

        wxRect mouseDragRectangle_; // mouse drag rectangle, or 0,0,0,0 when not dragging
        wxPoint lastMousePosition_;
        wxBitmap bitmap_;
        wxPlotCtrl *host_;

    private:
        DECLARE_CLASS(Area)
        DECLARE_EVENT_TABLE()
    };

//-----------------------------------------------------------------------------
// wxPlotCtrlAxis - X or Y axis window (privately used in wxPlotCtrl)
//-----------------------------------------------------------------------------

    class Axis: public wxWindow
    {
    public:
        Axis(wxPlotCtrl *parent, wxWindowID id);
        bool Create(wxPlotCtrl *parent, wxWindowID id);
        virtual ~Axis() = default;

        void CreateBitmap();

        bool IsXAxis() const;

        void OnEraseBackground(wxEraseEvent &) {}
        void OnPaint(wxPaintEvent &event);
        void OnMouse(wxMouseEvent &event);
        void OnChar(wxKeyEvent &event);

        wxPoint lastMousePosition_;
        wxBitmap bitmap_;
        wxPlotCtrl *host_;

    private:
        DECLARE_CLASS(Axis)
        DECLARE_EVENT_TABLE()
    };
    // ------------------------------------------------------------------------
    // Curve Accessors
    // ------------------------------------------------------------------------

    // Add a curve to the plot, takes ownership of the curve and deletes it
    bool AddCurve(wxPlotData *curve, bool select = true, bool sendEvent = false);
    // Add a curve to the plot, increases ref count
    bool AddCurve(const wxPlotData &curve, bool select = true, bool sendEvent = false);
    // Delete this curve
    bool DeleteCurve(wxPlotData *curve, bool sendEvent = false);
    // Delete this curve, if curve_index = -1, delete all curves
    bool DeleteCurve(int curve_index, bool sendEvent = false);

    // Total number of curves associated with the plotctrl
    int GetCurveCount() const;
    bool CurveIndexOk(int curveIndex) const;

    // Get the curve at this index
    wxPlotData *GetCurve(int curveIndex) const;
    // Sets the currently active curve, NULL for none active
    void SetActiveCurve(wxPlotData *curve, bool sendEvent = false);
    // Gets the currently active curve, NULL if none
    wxPlotData *GetActiveCurve() const;

    // Set the curve_index curve active, use -1 to have none selected
    void SetActiveIndex(int curveIndex, bool sendEvent = false);
    // Get the index of the active curve, returns -1 if none active
    int GetActiveIndex() const;

    //-------------------------------------------------------------------------
    // Markers
    //-------------------------------------------------------------------------

    // Add a marker to be displayed
    int AddMarker(const wxPlotMarker &marker);

    void RemoveMarker(int marker);
    void ClearMarkers();

    wxPlotMarker GetMarker(int marker) const;
    wxArrayPlotMarker &GetMarkerArray();

    //-------------------------------------------------------------------------
    // Cursor position - a single selected point in a curve
    //-------------------------------------------------------------------------

    // Hide the cursor
    void InvalidateCursor(bool sendEvent = false);
    // Does the cursor point to a valid curve and if a data curve a valid data index
    bool IsCursorValid();
    // Get the index of the curve that the cursor is associated with, -1 if none
    int GetCursorCurveIndex() const;
    // Get the index into the wxPlotData curve of the cursor, -1 if not on a data curve
    int GetCursorDataIndex() const;
    // Get the location of the cursor, valid for all curve types if cursor valid
    wxPoint2DDouble GetCursorPoint();
    // Set the curve and the index into the wxPlotData of the cursor
    //   curve_index must point to a data curve and cursor_index valid in data
    bool SetCursorDataIndex(int curve_index, int cursor_index, bool sendEvent = false);
    // Set the curve and the x-value of the cursor, valid for all curve types
    //    if curve_index is a wxPlotData curve it finds nearest index
    bool SetCursorXPoint(int curve_index, double x, bool sendEvent = false);
    // The cursor must be valid, if center then it centers the plot on the cursor
    //    if !center then make the cursor just barely visible by shifting the view
    void MakeCursorVisible(bool center, bool sendEvent = false);

    //-------------------------------------------------------------------------
    // Selected points
    //-------------------------------------------------------------------------

    // Is anything selected in a particular curve or any curve if index = -1
    bool HasSelection(int curve_index = -1) const;

    // the selections of wxPlotData curves are of the indexes of the data
    //   for curves that are wxPlotCurves or wxPlotFunctions the selection is empty
    const wxArrayRangeIntSelection &GetDataCurveSelections() const;
    // Get the particluar selection for the curve at index curve_index
    wxRangeIntSelection *GetDataCurveSelection(int curveIndex) const;

    // Get the number of individual selections of this curve
    int GetSelectedRangeCount(int curveIndex) const;

    // Selects points in a curve using a rectangular selection (see select range)
    //   this works for all plotcurve classes, for wxPlotData they're converted to the indexes however
    //   if there's nothing to select or already selected it returns false
    //   if curveIndex == -1 then try to select points in all curves
    bool SelectRectangle(int curveIndex, const wxRect2DDouble &rect, bool sendEvent = false);
    bool DeselectRectangle(int curveIndex, const wxRect2DDouble &rect, bool sendEvent = false);

    // Select a single point wxRangeDouble(pt,pt) or a data range wxRangeDouble(pt1, pt2)
    //   this works for all plotcurve classes, for wxPlotData they're converted to the indexes however
    //   if there's nothing to select or already selected it returns false
    //   if curveIndex == -1 then try to select points in all curves
    bool SelectXRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent = false);
    bool DeselectXRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent = false);
    bool SelectYRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent = false);
    bool DeselectYRange(int curveIndex, double rangeMin, double rangeMax, bool sendEvent = false);

    // Select a single point wxRangeInt(pt, pt) or a range of points wxRangeInt(pt1, pt2)
    //   if there's nothing to select or already selected it returns false,
    //   this ONLY works for wxPlotData curves
    bool SelectDataRange(int curve_index, const wxRangeInt &range, bool sendEvent = false);
    bool DeselectDataRange(int curve_index, const wxRangeInt &range, bool sendEvent = false);

    // Clear the ranges, if curve_index = -1 then clear them all
    bool ClearSelectedRanges(int curveIndex, bool sendEvent = false);

    // internal use, or not...
    virtual bool DoSelectRectangle(int curveIndex, const wxRect2DDouble &rect, bool select, bool sendEvent = false);
    virtual bool DoSelectDataRange(int curveIndex, const wxRangeInt &range, bool select, bool sendEvent = false);
    // called from DoSelect... when selecting to ensure that the current selection
    // matches the SetSelectionType by unselecting as appropriate
    // The input curveIndex implies that a selection will be made for that curve
    // This is not called for a deselection event.
    virtual bool UpdateSelectionState(int curveIndex, bool sendEvent);

    // Set how the selections mechanism operates, see enum wxPlotCtrlSelection_Type
    //   You are responsible to clean up the selections if you change this,
    //   however it won't fail, but may be out of sync.
    void SetSelectionType(SelectionType type);
    SelectionType GetSelectionType() const;

    // ------------------------------------------------------------------------
    // Get/Set origin, size, and Zoom in/out of view, set scaling, size...
    // ------------------------------------------------------------------------

    // make this curve fully visible or -1 to make all curves visible
    //   uses wxPlotCurve::GetBoundingRect()
    //   data curves have known sizes, function curves use default rect, unless set
    bool MakeCurveVisible(int curve_index, bool sendEvent = false);

    // Set the origin of the plot window
    bool SetOrigin(double origin_x, double origin_y, bool sendEvent = false);

    // Get the bounds of the plot window view in plot coords
    const wxRect2DDouble &GetViewRect() const;
    // Set the bounds of the plot window
    bool SetViewRect(const wxRect2DDouble &view, bool sendEvent = false);

    // Get the zoom factor = (pixel size of window)/(GetViewRect().m_width or height)
    const wxPoint2DDouble &GetZoom() const;

    // Zoom, if zoom_x or zoom_y <= 0 then fit that axis to window and center it
    bool SetZoom(const wxPoint2DDouble &zoom, bool aroundCenter = true, bool sendEvent = false);
    virtual bool SetZoom(double zoom_x, double zoom_y, double origin_x, double origin_y, bool sendEvent = false);

    // Zoom in client coordinates, window.[xy] is top left (unlike plot axis)
    bool SetZoom(const wxRect &window, bool sendEvent = false);

    // Set/Get the default size the plot should take when either no curves are
    //   loaded or only plot(curves/functions) that have no bounds are loaded
    //   The width and the height must both be > 0
    void SetDefaultBoundingRect(const wxRect2DDouble &rect, bool sendEvent = false);
    const wxRect2DDouble &GetDefaultBoundingRect() const;

    // Get the bounding rect of all the curves,
    //    equals the default if no curves or no bounds on the curves
    const wxRect2DDouble &GetCurveBoundingRect() const;

    // Get client rect of the wxPlotCtrlArea window, 0, 0, client_width, client_height
    const wxRect &GetPlotAreaRect() const;

    // The history of mouse drag rects are saved (mouseFunc_zoom)
    void NextHistoryView(bool foward, bool sendEvent = false);
    int GetHistoryViewCount() const;
    int GetHistoryViewIndex() const;

    // Fix the aspect ratio of the x and y axes, if set then when the zoom is
    //  set the smaller of the two (x or y) zooms is multiplied by the ratio
    //  to calculate the other.
    void SetFixAspectRatio(bool fix, double ratio = 1.0);
    void FixAspectRatio(double *zoom_x, double *zoom_y, double *origin_x, double *origin_y);

    // ------------------------------------------------------------------------
    // Mouse Functions for the area window
    // ------------------------------------------------------------------------

    // The current (last) pixel position of the mouse in the plotArea
    const wxPoint &GetAreaMouseCoord() const;

    // The current plotArea position of the mouse cursor
    wxPoint2DDouble GetAreaMousePoint() const;

    // Get the rect during dragging mouse, else 0
    const wxRect &GetAreaMouseMarkedRect() const;

    // Set what the mouse will do for different actions
    void SetAreaMouseFunction(MouseFunction func, bool sendEvent = false);
    MouseFunction GetAreaMouseFunction() const;

    // Set what sort of marker should be drawn when dragging mouse
    void SetAreaMouseMarker(MarkerType type);
    MarkerType GetAreaMouseMarker() const;

    // Set the mouse cursor wxCURSOR_XXX + CURSOR_GRAB for the plot area
    void SetAreaMouseCursor(int cursorid);

    // ------------------------------------------------------------------------
    // Options
    // ------------------------------------------------------------------------

    // Use a full width/height crosshair as a cursor
    bool GetCrossHairCursor() const;
    void SetCrossHairCursor(bool useCrosshairCursor = true);

    // Draw the data curve symbols on the plotctrl
    bool GetDrawSymbols() const;
    void SetDrawSymbols(bool drawSymbols = true);

    // Draw the interconnecting straight lines between data points
    bool GetDrawLines() const;
    void SetDrawLines(bool drawlines = true);

    // Draw the interconnecting splines between data points
    bool GetDrawSpline() const;
    void SetDrawSpline(bool drawSpline = true);

    // Draw the plot grid over the whole window, else just tick marks at edge
    bool GetDrawGrid() const;
    void SetDrawGrid(bool drawGrid = true);
    bool GetDrawTicks() const;
    void SetDrawTicks(bool drawTicks = true);

    // Try to fit the window to show all curves when a new curve is added
    bool GetFitPlotOnNewCurve() const;
    void SetFitPlotOnNewCurve(bool fit = true);

    // Set the focus to this window if the mouse enters it, otherwise you have to click
    //   sometimes convenient, but often this is annoying
    bool GetGreedyFocus() const;
    void SetGreedyFocus(bool grabFocus = false);

    // turn on or off the Correct Ticks functions.  Turning this off allows a graph
    // that scrolls to scroll smoothly in the direction expected.  Turning this
    // on (default) gives better accuracy for 'mouse hover' information display
    bool GetCorrectTicks() const;
    void SetCorrectTicks(bool correct = true);

    // get/set the width of a 1 pixel pen in mm for printing
    double GetPrintingPenWidth();
    void SetPrintingPenWidth(double width);

    // ------------------------------------------------------------------------
    // Colours & Fonts for windows, labels, title...
    // ------------------------------------------------------------------------

    // Get/Set the background colour of all the plot windows, default white
    wxColour GetBackgroundColour() const;
    virtual bool SetBackgroundColour(const wxColour &colour);

    // Get/Set the colour of the grid lines in the plot area, default grey
    wxColour GetGridColour() const;
    void SetGridColour(const wxColour &colour);

    // Get/Set the colour of the border around the plot area, default black
    wxColour GetBorderColour() const;
    void SetBorderColour(const wxColour &colour);

    // Get/Set the colour of the cursor marker, default green
    wxColour GetCursorColour() const;
    void SetCursorColour(const wxColour &colour);
    // Get/Set the cursor size, the size of the circle drawn for the cursor.
    //   set size to 0 to not have the cursor shown (default = 2)
    int GetCursorSize() const;
    void SetCursorSize(int size);

    // Get/Set the axis numbers font and colour, default normal & black
    wxFont GetAxisFont() const;
    wxColour GetAxisColour() const;
    void SetAxisFont(const wxFont &font);
    void SetAxisColour(const wxColour &colour);

    // Get/Set axis label fonts and colour, default swiss and black
    wxFont GetAxisLabelFont() const;
    wxColour GetAxisLabelColour() const;
    void SetAxisLabelFont(const wxFont &font);
    void SetAxisLabelColour(const wxColour &colour);

    // Get/Set the title font and colour, default swiss and black
    wxFont GetPlotTitleFont() const;
    wxColour GetPlotTitleColour() const;
    void SetPlotTitleFont(const wxFont &font);
    void SetPlotTitleColour(const wxColour &colour);

    // Get/Set the key font and colour
    wxFont GetKeyFont() const;
    wxColour GetKeyColour() const;
    void SetKeyFont(const wxFont &font);
    void SetKeyColour(const wxColour &colour);

    // ------------------------------------------------------------------------
    // Title, axis labels, and key values and visibility
    // ------------------------------------------------------------------------

    // Get/Set showing x and/or y axes
    void SetShowBottomAxis(bool show);
    void SetShowLeftAxis(bool show);
    bool GetShowBottomAxis();
    bool GetShowLeftAxis();

    // Get/Set and show/hide the axis labels
    const wxString &GetBottomAxisLabel() const;
    const wxString &GetLeftAxisLabel() const;
    void SetBottomAxisLabel(const wxString &label);
    void SetLeftAxisLabel(const wxString &label);
    bool GetShowBottomAxisLabel() const;
    bool GetShowLeftAxisLabel() const;
    void SetShowBottomAxisLabel(bool show);
    void SetShowLeftAxisLabel(bool show);

    // Get/Set and show/hide the title
    const wxString &GetPlotTitle() const;
    void SetPlotTitle(const wxString &title);
    bool GetShowPlotTitle() const;
    void SetShowPlotTitle(bool show);

    // Show a key with the function/data names, pos is %width and %height (0-100)
    const wxString &GetKeyString() const;
    bool GetShowKey() const;
    void SetShowKey(bool show);
    wxPoint GetKeyPosition() const;
    bool GetKeyInside() const;
    void SetKeyPosition(const wxPoint &pos, bool stayInside = true);

    // used internally to update the key string from the curve names
    virtual void CreateKeyString();

    // set the minimum value to be displayed as an exponential on the axes
    long GetMinExpValue() const;
    void SetMinExpValue(long min);

    // ------------------------------------------------------------------------
    // Title, axis label editor control
    // ------------------------------------------------------------------------

    // Sends the wxEVT_PLOTCTRL_BEGIN_TITLE_(X/Y_LABEL)_EDIT event if sendEvent
    //  which can be vetoed
    void ShowTextCtrl(TextCtrlType type, bool sendEvent = false);
    // Sends the wxEVT_PLOTCTRL_END_TITLE_(X/Y_LABEL)_EDIT event if sendEvent
    //  which can be vetoed
    void HideTextCtrl(bool saveValue = true, bool sendEvent = false);
    bool IsTextCtrlShown() const;

    // ------------------------------------------------------------------------
    // Event processing
    // ------------------------------------------------------------------------

    // EVT_MOUSE_EVENTS from the area and axis windows are passed to these functions
    virtual void ProcessAreaMouseEvent(wxMouseEvent &event);
    virtual void ProcessAxisMouseEvent(wxMouseEvent &event);

    // EVT_CHAR from the area and axis windows are passed to these functions
    virtual void ProcessAreaCharEvent(wxKeyEvent &event);
    virtual void ProcessAreaKeyDownEvent(wxKeyEvent &event);
    virtual void ProcessAreaKeyUpEvent(wxKeyEvent &event);
    virtual void ProcessAxisCharEvent(wxKeyEvent &event);

    void OnChar(wxKeyEvent &event);
    void OnScroll(wxScrollEvent &event);
    void OnPaint(wxPaintEvent &event);
    void OnEraseBackground(wxEraseEvent &event);
    void OnIdle(wxIdleEvent &event);
    void OnMouse(wxMouseEvent &event);
    void OnTextEnter(wxCommandEvent &event);

    // ------------------------------------------------------------------------
    // Drawing functions
    // ------------------------------------------------------------------------

    // call BeginBatch to disable redrawing EndBatch to reenable and refresh
    // when batchcount == 0, if !forceRefresh then don't refresh when batch == 0
    void BeginBatch();
    void EndBatch(bool forceRefresh = true);
    int GetBatchCount() const;

    // Redraw parts of the plotctrl using combinations of RedrawNeed
    void Redraw(int need);
    // Get/Set the redraw need variable (this is for internal use, see redraw())
    int GetRedrawNeeds() const;
    void SetRedrawNeeds(int need);

    // Draw a marker in lower right signifying that this has the focus
    virtual void DrawActiveBitmap(wxDC *dc);
    // Draw the wxPlotCtrl (this window)
    virtual void DrawPlotCtrl(wxDC *dc);
    // Draw the area window
    virtual void DrawAreaWindow(wxDC *dc, const wxRect &rect);
    // Draw a wxPlotData derived curve
    virtual void DrawDataCurve(wxDC *dc, wxPlotData *curve, int curve_index, const wxRect &rect);
    // Draw the key
    virtual void DrawKey(wxDC *dc);
    // Draw the left click drag marker, type is wxPlotCtrl_Marker_Type
    virtual void DrawMouseMarker(wxDC *dc, MarkerType type, const wxRect &rect);
    // Draw a crosshair cursor at the point (mouse cursor)
    virtual void DrawCrosshairCursor(wxDC *dc, const wxPoint &pos);
    // Draw the cursor marking a single point in a curve wxPlotCtrl::GetCursorPoint
    virtual void DrawCurveCursor(wxDC *dc);
    // Draw the tick marks or grid lines
    virtual void DrawTickMarks(wxDC *dc, const wxRect &rect);
    // Draw the tick marks or grid lines
    virtual void DrawGridLines(wxDC *dc, const wxRect &rect);
    void DrawTickMarksOrGridLines(wxDC *dc, const wxRect &rect, bool ticksOnly);
    // Draw markers
    virtual void DrawMarkers(wxDC *dc, const wxRect &rect);

    // redraw this wxPlotData between these two indexes (for (de)select redraw)
    virtual void RedrawCurve(int index, int minIndex, int maxIndex);

    // Draw the X or Y axis onto the dc
    virtual void DrawXAxis(wxDC *dc, bool refresh);
    virtual void DrawYAxis(wxDC *dc, bool refresh);

    // Draw the plot axes and plotctrl on this wxDC for printing, sort of WYSIWYG
    //   the plot is drawn to fit inside the boundingRect (i.e. the margins)
    void DrawWholePlot(wxDC *dc, const wxRect &boundingRect, double dpi = 72);

    // ------------------------------------------------------------------------
    // Axis tick calculations
    // ------------------------------------------------------------------------

    // find the optimal number of ticks, step size, and format string
    //void AutoCalcTicks()      {AutoCalcXAxisTicks(); AutoCalcYAxisTicks();}
    void AutoCalcXAxisTicks();
    void AutoCalcYAxisTicks();
    virtual void DoAutoCalcTicks(bool xAxis);
    // slightly correct the Zoom and origin to exactly match tick marks
    //  otherwise when the mouse is over '1' you may get 0.99999 or 1.000001
    //void CorrectTicks() {CorrectXAxisTicks(); CorrectYAxisTicks();}
    void CorrectXAxisTicks();
    void CorrectYAxisTicks();
    // Find the correct dc coords for the tick marks and label strings, internal use
    virtual void CalcXAxisTickPositions();
    virtual void CalcYAxisTickPositions();

    // ------------------------------------------------------------------------
    // Utilities
    // ------------------------------------------------------------------------

    // Find a curve at pt, in rect of size +- dxdyPt, starting with active curve
    // return sucess, setting curve_index, data_index if data curve, and if
    // curvePt fills the exact point in the curve.
    bool FindCurve(const wxPoint2DDouble &pt, const wxPoint2DDouble &dxdyPt,
                   int &curveIndex, int &data_index, wxPoint2DDouble *curvePt = NULL) const;

    // if n is !finite send wxEVT_PLOTCTRL_ERROR if msg is not empty
    bool IsFinite(double n, const wxString &msg = wxEmptyString) const;

    // call this whenever you adjust the size of a data curve
    //    this necessary to know the default zoom to show them
    void CalcBoundingPlotRect();

    // Client (pixels) to/from plot (double) coords
    inline double GetPlotCoordFromClientX(int clientx) const
        {return (clientx/zoom_.m_x + viewRect_.GetLeft());}
    inline double GetPlotCoordFromClientY(int clienty) const
        {return ((areaClientRect_.height - clienty)/zoom_.m_y + viewRect_.GetTop());}
    inline wxRect2DDouble GetPlotRectFromClientRect(const wxRect &clientRect) const
        {
            return wxRect2DDouble(GetPlotCoordFromClientX(clientRect.x),
                                   GetPlotCoordFromClientY(clientRect.GetBottom()),
                                   clientRect.width/zoom_.m_x,
                                   clientRect.height/zoom_.m_y);
        }

    inline int GetClientCoordFromPlotX(double plotx) const
        {double x = zoom_.m_x*(plotx - viewRect_.GetLeft()) + 0.5; return x < INT_MAX ? int(x) : INT_MAX;}
    inline int GetClientCoordFromPlotY(double ploty) const
        {double y = areaClientRect_.height - zoom_.m_y*(ploty - viewRect_.GetTop()) + 0.5; return y < INT_MAX ? int(y) : INT_MAX;}
    inline wxRect GetClientRectFromPlotRect(const wxRect2DDouble &plotRect) const
        {
            double w = plotRect.m_width*zoom_.m_x + 0.5;
            double h = plotRect.m_height*zoom_.m_y + 0.5;
            return wxRect(GetClientCoordFromPlotX(plotRect.m_x),
                           GetClientCoordFromPlotY(plotRect.GetBottom()),
                           w < INT_MAX ? int(w) : INT_MAX,
                           h < INT_MAX ? int(h) : INT_MAX);
        }

    // Get the windows

    // internal use size adjustment
    void AdjustScrollBars();
    void UpdateWindowSize();
    void DoSize(const wxRect &boundingRect = wxRect(0, 0, 0, 0), bool setWindowSizes = true);

    // who's got the focus, if this then draw the bitmap to show it, internal
    bool CheckFocus();

    // send event returning true if it's allowed
    bool DoSendEvent(wxPlotCtrlEvent &event) const;

    void StartMouseTimer(wxWindowID win_id);
    void StopMouseTimer();
    bool IsTimerRunning();
    void OnTimer(wxTimerEvent &event);

    // A locker for the captured window, set to NULL to release
    void SetCaptureWindow(wxWindow *win);
    wxWindow *GetCaptureWindow() const;

    struct AxisTicks
    {
        wxArrayInt positions_; // pixel coordinates of the tic marks
        wxArrayString labels_; // the tick labels
        wxString format_;      // format %lg for example
        double step_;          // step size between ticks
        int count_;            // how many ticks fit?
        bool correct_;         // tick correction
    };
protected:
    void OnSize(wxSizeEvent &event);

    wxArrayPlotData curves_;                   // all the curves
    wxPlotData *activeCurve_;                  // currently active curve
    int activeIndex_;                          // index in array of currently active curve

    wxPlotMarker cursorMarker_;                // marker to draw for cursor
    int cursorCurve_;                          // index into plot curve array
    int cursorIndex_;                          // if data curve, index in curve

    wxArrayRangeIntSelection dataSelections_;  // for wxPlotData
    SelectionType selectionType_;

    wxArrayPlotMarker plotMarkers_;            // extra markers to draw

    bool showKey_;                             // show the key
    wxString keyString_;                       // the key string

    // title and label
    bool showTitle_;
    wxString title_;
    bool showBottomAxisLabel_;
    bool showLeftAxisLabel_;
    wxString bottomAxisLabel_;
    wxString leftAxisLabel_;
    wxRect titleRect_;
    wxRect bottomAxisLabelRect_;
    wxRect leftLabelRect_;

    // fonts and colours
    wxFont titleFont_;
    wxColour titleColour_;
    wxColour borderColour_;

    // option variables
    bool crosshairCursor_;
    bool drawSymbols_;
    bool drawLines_;
    bool drawSpline_;
    bool drawGrid_;
    bool drawTicks_;
    bool fitOnNewCurve_;
    bool showBottomAxis_;
    bool showLeftAxis_;

    // rects of the positions of each window - remember for DrawPlotCtrl
    wxRect bottomAxisRect_, leftAxisRect_;
    wxRect areaRect_, clientRect_;

    // zooms
    wxPoint2DDouble zoom_;
    wxArrayRect2DDouble historyViews_;
    int historyViewsIndex_;
    void AddHistoryView();              // maintains small list of views

    bool   fixedAspectRatio_;
    double aspectratio_;

    // bounding rect, (GetLeft,GetTop) is lower left in screen coords
    wxRect2DDouble viewRect_;          // part of the plot currently displayed
    wxRect2DDouble curveBoundingRect_; // total extent of the plot - CalcBoundingPlotRect
    wxRect2DDouble defaultPlotRect_;   // default extent of the plot, fallback
    wxRect areaClientRect_;            // rect of (wxPoint(0,0), PlotArea.GetClientSize())

    AxisTicks bottomAxisTicks_;
    AxisTicks leftAxisTicks_;

    // drawers
    wxPlotDrawerArea *areaDrawer_;
    wxPlotDrawerXAxis *bottomAxisDrawer_;
    wxPlotDrawerYAxis *leftAxisDrawer_;
    wxPlotDrawerKey *keyDrawer_;
    wxPlotDrawerDataCurve *dataCurveDrawer_;
    wxPlotDrawerMarker *markerDrawer_;

    // windows
    Area *area_;
    Axis *bottomAxis_;
    Axis *leftAxis_;
    wxScrollBar *xAxisScrollbar_;
    wxScrollBar *yAxisScrollbar_;

    // textctrl for label/title editor, created and deleted as necessary
    wxTextCtrl *textCtrl_;

    // focusing and bitmap to display focus
    wxBitmap activeBitmap_;
    wxBitmap inactiveBitmap_;
    wxWindow *focusedWin_;
    bool greedyFocus_;

    // remember what needs to be repainted so unnecessary EVT_PAINTS are skipped
    int redrawNeed_;
    int batchCount_;

    wxSize axisFontSize_;      // pixel size of the number '5' for axis font
    int    leftAxisTextWidth_; // size of "-5e+005" for max y axis width
    int    areaBorderWidth_;   // width of area border pen (default 1)
    int    border_;            // width of border between labels and axes
    long   minExponential_;    // minimum number displayed as an exponential
    double penPrintWidth_;     // width of a 1 pixel pen in mm when printed

    wxTimer  *timer_;          // don't use, see accessor functions
    wxWindow *winCapture_;     // don't use, see accessor functions

    MouseFunction areaMouseFunc_;
    MarkerType    areaMouseMarker_;
    int           areaMouseCursorid_;

    void setPlotWinMouseCursor(wxStockCursor cursorid);
    int mouseCursorid_;

private:
    DECLARE_ABSTRACT_CLASS(wxPlotCtrl)
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
// wxPlotCtrlEvent
//-----------------------------------------------------------------------------

class wxPlotCtrlEvent: public wxNotifyEvent
{
public:
    wxPlotCtrlEvent(wxEventType commandType = wxEVT_NULL,
                    wxWindowID id = wxID_ANY,
                    wxPlotCtrl *window = NULL);

    wxPlotCtrlEvent(const wxPlotCtrlEvent &event) : wxNotifyEvent(event),
        curve_(event.curve_), curveIndex_(event.curveIndex_),
        curveDataIndex_(event.curveDataIndex_),
        mouseFunc_(event.mouseFunc_), x_(event.x_), y_(event.y_) {}

    // position of the mouse cursor, double click, single point selection or 1st selected point
    double GetX() const {return x_;}
    double GetY() const {return y_;}
    void SetPosition(double x, double y) {x_ = x; y_ = y;}

    int GetCurveDataIndex() const {return curveDataIndex_;}
    void SetCurveDataIndex(int data_index) {curveDataIndex_ = data_index;}

    // pointer to the curve, NULL if not appropriate for the event type
    wxPlotData *GetCurve() const {return curve_;}
    void SetCurve(wxPlotData *curve, int curveIndex)
        {curve_ = curve; curveIndex_ = curveIndex;}

    // index of the curve in wxPlotCtrl::GetCurve(index)
    int  GetCurveIndex() const {return curveIndex_;}
    void SetCurveIndex(int curve_index) {curveIndex_ = curve_index;}

    wxPlotCtrl *GetPlotCtrl() const {return wxDynamicCast(GetEventObject(), wxPlotCtrl);}

    wxPlotCtrl::MouseFunction getMouseFunction() const {return mouseFunc_;}
    void setMouseFunction(wxPlotCtrl::MouseFunction func) {mouseFunc_ = func;}

    static wxString GetEventName(wxEventType eventType);

protected:
    virtual wxEvent *Clone() const {return new wxPlotCtrlEvent(*this);}

    wxPlotData *curve_;
    int curveIndex_;
    int curveDataIndex_;
    wxPlotCtrl::MouseFunction mouseFunc_;
    double x_, y_;

private:
    DECLARE_ABSTRACT_CLASS(wxPlotCtrlEvent);
};

//-----------------------------------------------------------------------------
// wxPlotCtrlEvent
//-----------------------------------------------------------------------------

class wxPlotCtrlSelEvent: public wxPlotCtrlEvent
{
public:
    wxPlotCtrlSelEvent(wxEventType commandType = wxEVT_NULL,
                       wxWindowID id = wxID_ANY,
                       wxPlotCtrl *window = NULL);

    wxPlotCtrlSelEvent(const wxPlotCtrlSelEvent &event):
        wxPlotCtrlEvent(event),
        dataRange_(event.dataRange_),
        dataSelection_(event.dataSelection_),
        selecting_(event.selecting_) {}

    // for SELection events the range specifies the new (de)selection range
    // note : for unordered data sets an event is only sent after all selections are made

    wxRangeInt GetDataSelectionRange() const {return dataRange_;}
    void SetDataSelectionRange(const wxRangeInt &range, bool selecting)
        {dataRange_ = range; selecting_ = selecting;}

    // for a wxPlotData this is filled with the (de)selected ranges.
    //   there will only be more than one for unordered wxPlotDatas
    const wxRangeIntSelection &GetDataSelections() const {return dataSelection_;}
    wxRangeIntSelection &GetDataSelections() {return dataSelection_;}
    void SetDataSelections(const wxRangeIntSelection &ranges) {dataSelection_ = ranges;}

    // range is selected as opposed to being deselected
    bool IsSelecting() const {return selecting_;}

protected:
    virtual wxEvent *Clone() const {return new wxPlotCtrlSelEvent(*this);}

    wxRangeInt dataRange_;
    wxRangeIntSelection dataSelection_;
    bool selecting_;

private:
    DECLARE_ABSTRACT_CLASS(wxPlotCtrlSelEvent);
};

// ----------------------------------------------------------------------------
// wxPlotCtrlEvent event types
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()

// wxPlotCtrlEvent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_ADD_CURVE,          0) // a curve has been added
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_DELETING_CURVE,     0) // a curve is about to be deleted, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_DELETED_CURVE,      0) // a curve has been deleted

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGING, 0) // curve selection changing, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_CURVE_SEL_CHANGED,  0) // curve selection has changed

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_MOTION,       0) // mouse moved
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_CLICKED,            0) // mouse left or right clicked
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_DOUBLECLICKED,      0) // mouse left or right doubleclicked
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_CLICKED,      0) // clicked on a plot point (+-2pixels)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_POINT_DOUBLECLICKED,0) // dclicked on a plot point (+-2pixels)

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATING,  0) // mouse left down and drag begin
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CHANGING,  0) // mouse left down and dragging
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CREATED,   0) // mouse left down and drag end

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGING,      0) // zoom or origin of plotctrl is about to change
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VIEW_CHANGED,       0) // zoom or origin of plotctrl has changed

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGING,    0) // cursor point/curve is about to change, event.Skip(false) to prevent
                                     // if the cursor is invalidated since
                                     // the curve is gone you cannot prevent it
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_CURSOR_CHANGED,     0) // cursor point/curve changed

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_ERROR,              0) // an error has occured, see event.GetString()
                                     // usually nonfatal NaN overflow errors

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_TITLE_EDIT,   0) // title is about to be edited, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_END_TITLE_EDIT,     0) // title has been edited and changed, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_X_LABEL_EDIT, 0) // x label is about to be edited, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_END_X_LABEL_EDIT,   0) // x label has been edited and changed, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_BEGIN_Y_LABEL_EDIT, 0) // y label is about to be edited, event.Skip(false) to prevent
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_END_Y_LABEL_EDIT,   0) // y label has been edited and changed, event.Skip(false) to prevent

DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGING,0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGED, 0)

// wxPlotCtrlSelEvent
//DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATING,0)
//DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CREATED, 0)
//DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGING,0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED,   0)

// unused
/*
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CREATING, 0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CREATED,  0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CHANGING, 0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_VALUE_SEL_CHANGED,  0)
DECLARE_EVENT_TYPE(wxEVT_PLOTCTRL_AREA_SEL_CHANGED,   0)
*/
END_DECLARE_EVENT_TYPES()
// ----------------------------------------------------------------------------
// wxPlotCtrlEvent macros
// ----------------------------------------------------------------------------

typedef void (wxEvtHandler::*wxPlotCtrlEventFunction)(wxPlotCtrlEvent&);
#define wxPlotCtrlEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxPlotCtrlEventFunction, &func)
#define wx__DECLARE_PLOTCTRLEVT(evt, id, fn)         wx__DECLARE_EVT1(evt, id, wxPlotCtrlEventHandler(fn))

#define EVT_PLOTCTRL_ADD_CURVE(id, fn)               wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_ADD_CURVE,               id, fn)
#define EVT_PLOTCTRL_DELETING_CURVE(id, fn)          wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_DELETING_CURVE,          id, fn)
#define EVT_PLOTCTRL_DELETED_CURVE(id, fn)           wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_DELETED_CURVE,           id, fn)

#define EVT_PLOTCTRL_CURVE_SEL_CHANGING(id, fn)      wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_CURVE_SEL_CHANGING,      id, fn)
#define EVT_PLOTCTRL_CURVE_SEL_CHANGED(id, fn)       wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_CURVE_SEL_CHANGED,       id, fn)

#define EVT_PLOTCTRL_MOUSE_MOTION(id, fn)            wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_MOUSE_MOTION,            id, fn)
#define EVT_PLOTCTRL_CLICKED(id, fn)                 wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_CLICKED,                 id, fn)
#define EVT_PLOTCTRL_DOUBLECLICKED(id, fn)           wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_DOUBLECLICKED,           id, fn)
#define EVT_PLOTCTRL_POINT_CLICKED(id, fn)           wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_POINT_CLICKED,           id, fn)
#define EVT_PLOTCTRL_POINT_DOUBLECLICKED(id, fn)     wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_POINT_DOUBLECLICKED,     id, fn)

#define EVT_PLOTCTRL_AREA_SEL_CREATING(id, fn)       wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_AREA_SEL_CREATING,       id, fn)
#define EVT_PLOTCTRL_AREA_SEL_CHANGING(id, fn)       wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_AREA_SEL_CHANGING,       id, fn)
#define EVT_PLOTCTRL_AREA_SEL_CREATED(id, fn)        wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_AREA_SEL_CREATED,        id, fn)

#define EVT_PLOTCTRL_VIEW_CHANGING(id, fn)           wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_VIEW_CHANGING,           id, fn)
#define EVT_PLOTCTRL_VIEW_CHANGED(id, fn)            wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_VIEW_CHANGED,            id, fn)

#define EVT_PLOTCTRL_CURSOR_CHANGING(id, fn)         wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_CURSOR_CHANGING,         id, fn)
#define EVT_PLOTCTRL_CURSOR_CHANGED(id, fn)          wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_CURSOR_CHANGED,          id, fn)

#define EVT_PLOTCTRL_ERROR(id, fn)                   wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_ERROR,                   id, fn)

#define EVT_PLOTCTRL_BEGIN_TITLE_EDIT(id, fn)        wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_BEGIN_TITLE_EDIT,        id, fn)
#define EVT_PLOTCTRL_END_TITLE_EDIT(id, fn)          wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_END_TITLE_EDIT,          id, fn)
#define EVT_PLOTCTRL_BEGIN_BOTTOM_LABEL_EDIT(id, fn) wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_BEGIN_BOTTOM_LABEL_EDIT, id, fn)
#define EVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT(id, fn)   wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_END_BOTTOM_LABEL_EDIT,   id, fn)
#define EVT_PLOTCTRL_BEGIN_LEFT_LABEL_EDIT(id, fn)   wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_BEGIN_LEFT_LABEL_EDIT,   id, fn)
#define EVT_PLOTCTRL_END_LEFT_LABEL_EDIT(id, fn)     wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_END_LEFT_LABEL_EDIT,     id, fn)

#define EVT_PLOTCTRL_MOUSE_FUNC_CHANGING(id, fn)     wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGING,     id, fn)
#define EVT_PLOTCTRL_MOUSE_FUNC_CHANGED(id, fn)      wx__DECLARE_PLOTCTRLEVT(wxEVT_PLOTCTRL_MOUSE_FUNC_CHANGED,      id, fn)

typedef void (wxEvtHandler::*wxPlotCtrlSelEventFunction)(wxPlotCtrlSelEvent&);
#define wxPlotCtrlSelEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxPlotCtrlSelEventFunction, &func)
#define wx__DECLARE_PLOTCTRLSELEVT(evt, id, fn) wx__DECLARE_EVT1(evt, id, wxPlotCtrlSelEventHandler(fn))

//#define EVT_PLOTCTRL_RANGE_SEL_CREATING(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_RANGE_SEL_CREATING, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlSelEventFunction, & fn), (wxObject *) NULL),
//#define EVT_PLOTCTRL_RANGE_SEL_CREATED(id, fn)  DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_RANGE_SEL_CREATED,  id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlSelEventFunction, & fn), (wxObject *) NULL),
//#define EVT_PLOTCTRL_RANGE_SEL_CHANGING(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_RANGE_SEL_CHANGING, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlSelEventFunction, & fn), (wxObject *) NULL),
#define EVT_PLOTCTRL_RANGE_SEL_CHANGED(id, fn)   wx__DECLARE_PLOTCTRLSELEVT(wxEVT_PLOTCTRL_RANGE_SEL_CHANGED,  id, fn)

/*
#define EVT_PLOTCTRL_VALUE_SEL_CREATING(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_VALUE_SEL_CREATING, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlEventFunction, & fn), (wxObject *) NULL),
#define EVT_PLOTCTRL_VALUE_SEL_CREATED(id, fn)  DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_VALUE_SEL_CREATED,  id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlEventFunction, & fn), (wxObject *) NULL),
#define EVT_PLOTCTRL_VALUE_SEL_CHANGING(id, fn) DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_VALUE_SEL_CHANGING, id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlEventFunction, & fn), (wxObject *) NULL),
#define EVT_PLOTCTRL_VALUE_SEL_CHANGED(id, fn)  DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_VALUE_SEL_CHANGED,  id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlEventFunction, & fn), (wxObject *) NULL),
#define EVT_PLOTCTRL_AREA_SEL_CHANGED(id, fn)   DECLARE_EVENT_TABLE_ENTRY(wxEVT_PLOTCTRL_AREA_SEL_CHANGED,   id, wxID_ANY, (wxObjectEventFunction) (wxEventFunction) wxStaticCastEvent(wxPlotCtrlEventFunction, & fn), (wxObject *) NULL),
*/

#endif

