/*===========================================================================*/
/*                                                                           */
/* File    : WVISMAP.C                                                       */
/*                                                                           */
/* Purpose : Maintains the window visibility map.                            */
/*                                                                           */
/* History :                                                                 */
/*                                                                           */
/* (C) Copyright 1989-1992 Marc Adler/Magma Systems     All Rights Reserved  */
/*===========================================================================*/
#include "wprivate.h"
#include "window.h"

/****************************************************************************/
/*                                                                          */
/* Function : WinInitVisMap()                                               */
/*                                                                          */
/* Purpose  : Allocates a visibility map. There is one cell on the map for  */
/*            every character position on the screen. A cell in the         */
/*            vismap has the window handle of the window which is visible   */
/*            in the corresponding position on the screen.                  */
/*                                                                          */
/* Returns  : Nothing.                                                      */
/*                                                                          */
/****************************************************************************/
VOID FAR PASCAL WinInitVisMap(void)
{
  if (WinVisMap != NULL)
    MyFree_far(WinVisMap);
  WinVisMap = (LPUINT) 
      emalloc_far((DWORD) VideoInfo.width * VideoInfo.length * sizeof(UINT));
}

HWND FAR PASCAL VisMapPointToHwnd(row, col)
  int row, col;
{
  if (row < 0 || row >= (int) VideoInfo.length || 
      col < 0 || col >= (int) VideoInfo.width)
    return NULLHWND;
  return WinVisMap[row*VideoInfo.width + col];
}

#ifdef DEBUG
VOID VisMapDump(void)
{
  int  row, col;

  FILE *fp = fopen("VISMAP.DMP", "w");
  if (fp)
  {
    for (row = 0;  row < VideoInfo.length;  row++)
    {
      for (col = 0;  col < VideoInfo.width;  col++)
        fprintf(fp, "%-02d ", WinVisMap[row * VideoInfo.width + col]);
      fprintf(fp, "\n");
    }
    fclose(fp);
  }
}
#endif


static WINDOW *_wComboToUpdate = NULL;


VOID FAR PASCAL WinUpdateVisMap(void)
{
  /*
    Initialize the map to say that the desktop window occupies the
    whole screen.
  */
  limemset(WinVisMap, 0x01, VideoInfo.width * VideoInfo.length);
  _WinUpdateVisMap(InternalSysParams.wDesktop->children);

  /*
    Combobox's listbox kludge...
  */
  if (_wComboToUpdate)
  {
    _WinUpdateVisMap(_wComboToUpdate);
    _wComboToUpdate = NULL;
  }

  SET_PROGRAM_STATE(STATE_SYNC_CARET);
}


VOID FAR PASCAL _WinUpdateVisMap(w)
  WINDOW *w;
{
  RECT   r;
  int    row, iWidth;
  LPUINT pVis;
  BOOL   bIsFrame;
  WINDOW *wP;
  WINDOW *wChild;
  HWND   hWnd;

  if (!w)
    return;

  hWnd = w->win_id;

  /*
    Do the lower stacked sibling windows first, then do the window itself,
    then do the window's children, the window's menu, and the system menu.
  */
  if (w->sibling)
    _WinUpdateVisMap(w->sibling);

  /*
    Check to see that the window and its ancestors are not hidden
  */
  for (wP = w;  wP && wP != InternalSysParams.wDesktop;  )
  {
    if (!(wP->flags & WS_VISIBLE) || TEST_WS_HIDDEN(wP))
      return;
    /*
      Invisible if an ancestor is iconic.
    */
    if (wP != w && (wP->flags & WS_MINIMIZE) && wP->hSysMenu != hWnd)
      return;
    if (IS_DIALOG(wP))
    {
#if !50492
      /*
        Dialog boxes are supposed to be shown even if their parents
        are hidden. This is the case if we decide to do a MessageBox()
        call in response to a WM_INITDIALOG message. The dialog
        box is still hidden at the time of the WM_INITDIALOG, but we
        still need to show the message box. So, ignore the visibility
        settings of the parent, and return TRUE if we encounter a
        dialog box.
      */
      break;
#else
      HWND hP = wP->hWndOwner;
      if (hP)
        wP = WID_TO_WIN(hP);
      else
        break;
#endif
    }
    else
      wP = wP->parent;
  }

  /*
    Generate an ancestor clipping rectangle (which also clips to the
    screen). We turn on the "border drawing" flag so that WinGenAncestor
    returns to us a window rect, not a client rect.
    The "-1" arg signals to WinGenAncestor() that we should clip to
    the window rect, not to the client rect.
  */
  if (!WinGenAncestorClippingRect(hWnd, (HDC) -1, &r))
    return;

  /*
    We have special processing associated with a frame. We don't want
    to clip the children within it.
  */
  bIsFrame = IsFrameClass(w);

  /*
    Get a pointer to the visibility map, and calculate the window width.
  */
  pVis   = WinVisMap + (r.top * VideoInfo.width);
  iWidth = RECT_WIDTH(r);

  for (row = r.top;  row < r.bottom;  row++)
  {
    /*
      Set the area occupied by the window to the window's handle.
    */
    if (bIsFrame)
    {
      /*
        Frames have special processing in order to maintain their
        transparency. If the row is not the top or bottom of the frame,
        then just fill in the vismap for the left and right side.
        If the row is on the top or bottom border of the window (not client)
        rect, then fill in the vismap too.
      */
      if (row > w->rect.top && row < w->rect.bottom-1)
      {
        /*
          Make sure that the side is not clipped off the screen.
        */
        if (r.left == w->rect.left)
          pVis[r.left]  = hWnd;
        if (r.right-1 == w->rect.right-1)
          pVis[r.right-1] = hWnd;
      }
      else if (row == w->rect.top || row == w->rect.bottom-1)
        limemset(pVis + r.left, hWnd, iWidth);
    }
    else
      limemset(pVis + r.left, hWnd, iWidth);
    pVis += VideoInfo.width;
  }

  if (w->children)
  {
    /*
      Update all of the non-popup children. They will get clipped by
      this window's menu and sysmenu.
    */
    for (wChild = w->children;  wChild && (wChild->flags & WS_POPUP);
         wChild = wChild->sibling)
      ;
    _WinUpdateVisMap(wChild);
  }

  if (w->hMenu)
    _WinUpdateVisMap(WID_TO_WIN(w->hMenu));

  {
  /*
    The last thing we update is the window's popups. These popups
    should not get clipped by the window's menubar. So, what we
    do is walk down the window's child list and stop if and when 
    we get to the lowest popup. Then, we temporarily cut off the
    popup list from the rest of the children, update the popups,
    and restore the children chain.
  */
  WINDOW *wPopup = (WINDOW *) NULL;
  for (wChild = w->children;  wChild && (wChild->flags & WS_POPUP);
       wChild = wChild->sibling)
    wPopup = wChild;
  if (wPopup)                         /* Are there any popups?      */
  {
    WINDOW *wSave = wPopup->sibling;  /* Save the last popup's link */
    wPopup->sibling = NULL;           /* Sever the chain at the last popup */
    _WinUpdateVisMap(w->children);    /* Update all of the popups   */
    wPopup->sibling = wSave;          /* Restore the link           */
  }
  }



  /*
    Update all top-level windows which this window owns.
  */

  /*
    Walk down to the last top-level window
  */
  for (wChild = InternalSysParams.wDesktop->children;  wChild && wChild->sibling;  
       wChild = wChild->sibling)
     ;
  /*
    Walk backwards from lowest top-level window to the highest. If
    we find a window which is owned by the window we are updating,
    then update just this owned window. Temporarily sever the owned
    window from any other top-level window so only this one owned
    window gets updated.
  */
  for (  ;  wChild;  wChild = wChild->prevSibling)
    if (wChild->hWndOwner == hWnd &&
        !_IsDialogModal(wChild->win_id)) /* <-- 6/23/93 (Televoice) */
    {
      WINDOW *wSave = wChild->sibling;  /* Save the last popup's link */
      wChild->sibling = NULL;           /* Sever the chain at the last popup */
      _WinUpdateVisMap(wChild);         /* Update all of the popups   */
      wChild->sibling = wSave;          /* Restore the link           */
    }


  if (w->hSysMenu)
    _WinUpdateVisMap(WID_TO_WIN(w->hSysMenu));

  /*
    Update the window's scrollbars
  */
  if (w->hSB[SB_HORZ])
    _WinUpdateVisMap(WID_TO_WIN(w->hSB[SB_HORZ]));
  if (w->hSB[SB_VERT])
    _WinUpdateVisMap(WID_TO_WIN(w->hSB[SB_VERT]));

  /*
    Kludge for visible listboxes in comboboxes so that they get displayed
    above the siblings. We should really have different hWndOwner and
    hParent fields for the listbox.
  */
  if (w->ulStyle & LBS_IN_COMBOBOX)
    _wComboToUpdate = w;
  SET_PROGRAM_STATE(STATE_SYNC_CARET);
}

