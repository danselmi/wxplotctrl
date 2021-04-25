/////////////////////////////////////////////////////////////////////////////
// Name:        plotdefs.h
// Purpose:     Definitions for wxPlotLib
// Author:      John Labenski
// Modified by:
// Created:     1/08/2005
// RCS-ID:      $Id: plotdefs.h,v 1.3 2006/03/20 03:29:21 jrl1 Exp $
// Copyright:   (c) John Labenski
// Licence:     wxWidgets licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __WX_PLOTDEF_H__
#define __WX_PLOTDEF_H__

#include "wx/defs.h"
#include "wx/wxcrtvararg.h"
#include "wx/geometry.h"

// Check if value is >= min_val and <= max_val
#define RINT(x) (int((x) >= 0 ? ((x) + 0.5) : ((x) - 0.5)))

#include "wx/dynarray.h"
#ifndef WX_DECLARE_OBJARRAY_WITH_DECL // for wx2.4 backwards compatibility
    #define WX_DECLARE_OBJARRAY_WITH_DECL(T, name, expmode) WX_DECLARE_USER_EXPORTED_OBJARRAY(T, name, WXDLLIMPEXP_PLOTCTRL)
#endif


#endif
