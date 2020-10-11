#define MODSOFT
#define NOGDI
#define NOGRAPHICS
#define NOKERNEL

/*
 * MDI1.C - Application Interface
 *
 * LANGUAGE      : Microsoft C5.1
 * MODEL         : medium
 * ENVIRONMENT   : Microsoft Windows 2.1 SDK
 * STATUS        : operational
 *
 * This module contains all the MDI code to handle the programmatic
 * interface.  This includes window creation, default message handling,
 * and message loop support code.  The MDI desktop window and the
 * document windows have separate creation and message handling
 * routines, while the message loop support is window independant.
 *
 * (C) Copyright 1988
 * Eikon Systems, Inc.
 * 989 E. Hillsdale Blvd, Suite 260
 * Foster City  CA  94404
 *
 */

/*===========================================================================*/
/*                                                                           */
/* History : Taken from Welch's article for Microsoft Systems Journal. Code  */
/*           used by permission of MSJ. Modified by Marc Adler to support    */
/*           MEWEL and to add MS Windows 3.0 compatibility.                  */
/*                                                                           */
/*===========================================================================*/
#include "wmdilib.h"

#define MAXTEXT  80

/* External variables */
extern int  iCurrentPopup;          /* What menu is up */
extern int  iNextPopup;             /* What menu is next */

static BOOL bInMDIDESTROY  = FALSE;
static UINT nTotalMDIDocsCreated = 0;
static BOOL bZoomingIcon   = FALSE;
static BOOL bSwitchingZoom = FALSE;
static HWND _HwndMDIActiveChild = 0;
static BOOL bInMDIActivate = 0;
static HWND hWndBeingDestroyed = NULLHWND;


/* */
extern LRESULT FAR PASCAL MdiClientProc(HWND,UINT,WPARAM,LPARAM);
static VOID PASCAL MdiChangeFrameCaption(HWND, HWND);


#if defined(MEWEL_GUI)
#define DRAWMENUBAR(h) \
{\
  WINDOW *wTmp=WID_TO_WIN(h);\
  wTmp->ulStyle |= WIN_UPDATE_NCAREA;\
  SET_PROGRAM_STATE(STATE_INVALIDRECTS_EXIST);\
}
#else
#define DRAWMENUBAR(h)   InternalDrawMenuBar(h)
#endif

/**/
/*
  MDI Client Info
*/
typedef struct tagMDIClientInfo
{
  HMENU hMainMenu;       /* PROP_MAINMENU        */
  HMENU hWindowPopup;    /* PROP_WINDOWMENU      */
  HMENU hSysMenu;        /* PROP_SYSMENU         */
  int   iItemsBeforeSep; /* PROP_ITEMSBEFORESEP  */
  HWND  bActive;         /* PROP_ACTIVE          */
  HWND  bZoomed;         /* PROP_ZOOM            */
  int   iChildCount;     /* PROP_COUNT           */
  int   iFirstChildID;   /* PROP_FIRSTCHILDID    */
  ATOM  haTitle;         /* PROP_TITLE           */
} MDICLIENTINFO, *PMDICLIENTINFO;

/*
  MDI Child info
*/
typedef struct tagMDIChildInfo
{
  HACCEL  hAccel;        /* PROP_ACCEL           */
  int     iMenuID;       /* PROP_MENUID          */
  BOOL    bIsMDI;        /* PROP_ISMDI           */
  BOOL    bIconized;     /* PROP_ICONIZED        */
  POINT   ptOrigin;      /* PROP_TOP             */
                         /* PROP_LEFT            */
  SIZE    extWindow;     /* PROP_WIDTH           */
                         /* PROP_HEIGHT          */
} MDICHILDINFO, *PMDICHILDINFO;


/*
  This table maps IDM_xxx into system commands
*/
static struct
{
  UINT idmCmd;
  UINT scCmd;
} IDMtoSC[] =
{
  IDM_RESTORE,    SC_RESTORE,
  IDM_MOVE,       SC_MOVE,
  IDM_SIZE,       SC_SIZE,
  IDM_MAXIMIZE,   SC_MAXIMIZE,
  IDM_MINIMIZE,   SC_MINIMIZE,
  IDM_PREVWINDOW, SC_PREVWINDOW,
  IDM_NEXTWINDOW, SC_NEXTWINDOW,
  IDM_CLOSE,      SC_CLOSE,
};


/**/
int FAR PASCAL MDIInitialize(void)
{
  WNDCLASS wc;
  memset((char *) &wc, 0, sizeof(wc));
  wc.lpszClassName = "MDICLIENT";
  wc.lpfnWndProc   = MdiClientProc;
  wc.style         = CS_HREDRAW | CS_VREDRAW;
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE+1);
#endif
  return RegisterClass(&wc);
}


/**/
/*
 * MdiMainDefWindowProc( hwndMain, message, wParam, lParam ) : long;
 *
 *    hwndMain       Handle to MDI desktop
 *    message        Current message
 *    wParam         Word parameter to message
 *    lParam         Long parameter to message
 *
 * Handle the MDI desktop messages.  Messages reach here only after
 * the application MDI desktop window procedure has processed those
 * that it wants.  Major occurances that are recognized in this
 * function include:  parent window activation & deactivation, WINDOW
 * menu messages (esp for switching between children), record keeping
 * of which menu is currently poped up (if any), and sizing of
 * document windows which are maximized.
 *
 */
LRESULT FAR PASCAL DefFrameProc(hwndFrame,hwndClient,message,wParam,lParam)
  HWND hwndFrame;   /* handle of the frame      */
  HWND hwndClient;  /* handle of the MDI client */
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND   hwndActive;       /* Currently active document window */
  HWND   hwndIter;         /* Iterating document windows */
  LONG   lStyle;           /* Window style */
  RECT   rcClient;         /* Client area of MDI desktop */

  /*
    Get a handle to the currently active MDI child window within this frame
  */
  hwndActive = MdiGetProp(hwndClient, PROP_ACTIVE);

  switch (message)
  {
    case WM_ACTIVATE:
      /* MDI child reflects the activation status of the parent */
#if !101292
      /*
        10/12/92 (maa)
          If we show a dialog box from the MDI application, then we do not
        want all of the menu switching and child deactivation stuff to go
        on. Likewise, when we dismiss the dialog box, we don't want all of
        the child activation stuff going on either. So, let's #ifdef out
        this WM_ACTIVATE code and see what happens.
      */
      if (hwndActive)
      {
        switch (wParam)
        {
          case 0: /* Deactivating */
            /*
              If the window which is getting the activation (loword(wParam))
              is the same as the currently active window, don't do anything.
              If not, then deactivate the currently active MDI child.
            */
            if (LOWORD(lParam) != hwndActive)
              MdiDeactivateChild(hwndActive);
            break;

          case 1:
          case 2:
            /* Activating */
            MdiActivateChild(hwndActive, FALSE);
            break;
        }
      }
#endif
      break;


    case WM_COMMAND:
      /* Is this a WINDOW menu message to select an MDI child */
      if (wParam >= MdiGetProp(hwndClient, PROP_FIRSTCHILDID)
          && wParam < MdiGetProp(hwndClient, PROP_FIRSTCHILDID) + 
#if 0
          MdiGetProp(hwndClient, PROP_COUNT))
#else
          nTotalMDIDocsCreated)   /* 09/10/91 (maa)  */
#endif
      {
        /* Iterate all the children of the mdi parent */
        for (hwndIter = GetTopWindow(hwndClient);
             hwndIter != 0;
             hwndIter = GetWindow(hwndIter, GW_HWNDNEXT))
        {
          /* Ignore self & non-mdi children */
          /*
            7/22/91 (maa)
            We want to restore the active child if it is iconized, so added
            a test for !MdiGetProp(PROP_ICONIZED).
          */
          if (hwndIter == hwndActive && !MdiGetProp(hwndIter, PROP_ICONIZED) || 
              !MdiGetProp(hwndIter, PROP_ISMDI))
          {
            continue;
          }

          /* Is this the window? */
          if (wParam == MdiGetProp(hwndIter, PROP_MENUID))
          {
            /* Make a new MDI child the current one */
            if (MdiGetProp(hwndClient, PROP_ZOOM))
            {
              MdiSwitchZoom(hwndIter, hwndActive);
            }
            else if (MdiGetProp(hwndIter, PROP_ICONIZED))
            {
              SendMessage(hwndClient, WM_MDIRESTORE, hwndIter, 0L);
            }

            MdiActivateChild(hwndIter, FALSE);
            bSwitchingZoom = FALSE;
            break;
          }
        }
      }
      else
      {
#if 1
/*
DDH ifdef'ed this to 0, but I don't know why. If we get a WM_COMMAND
with wParam set to IDM_RESTORE, then we must pass this on to the child.
*/
        if (hwndActive)
        {
          switch (wParam)
          {
            case IDM_RESTORE:
            case IDM_MOVE:
            case IDM_SIZE:
            case IDM_MAXIMIZE:
            case IDM_MINIMIZE:
            case IDM_PREVWINDOW:
            case IDM_NEXTWINDOW:
            case IDM_CLOSE     :
              return DefMDIChildProc(hwndActive, message, wParam, lParam);
            default :
              return TRUE;
          }
        }
#endif
      }
      return TRUE;


    case WM_DESTROY:
      /* Let MDI clean up */
#ifndef MEWEL
      /*
        Delete the fake system menu bitmap
      */
      DeleteObject(MdiGetProp(hwndClient, PROP_SYSMENU));
#endif
      DeleteAtom(MdiGetProp(hwndClient, PROP_TITLE));
      MdiFreeMenuKeyHook();
      break;


    case WM_INITMENU:
      /* Maybe the MDI child wants to do something too */
      if (hwndActive)
        SendMessage( hwndActive, message, wParam, lParam );
      break;


    case WM_INITMENUPOPUP:
      /* Keep track of which menu is popped up so that if we hit a left
      ** or right arrow, we know which the next menu to popup is.
      */
      if (HIWORD(lParam))
      {
        iCurrentPopup = POP_MAINSYS;
      }
      else
      {
        iCurrentPopup = LOWORD(lParam);
        if (MdiGetProp(hwndClient, PROP_ZOOM))
          iCurrentPopup--;
      }
      if (hwndActive)
      {
        /* Let the MDI child do something with this too */
        SendMessage( hwndActive, message, wParam, lParam );
      }
      break;

    case WM_MENUCHAR:
      /*
        Processing for the <ALT -> keystroke
      */
      if (wParam == '-')
      {
        /* Bring up active MDI child's system menu */
#ifdef MEWEL
        if (hwndActive)
#else
        if ((LOWORD(lParam) & MF_POPUP) == 0 && hwndActive)
#endif
        {
          /* Is the MDI child zoomed? */
          if ( MdiGetProp( hwndClient, PROP_ZOOM ) )
          {
            /* MDI child system menu is on the main menu bar */
            return MAKELONG( 0, MC_SELECT );
          }

          iNextPopup = POP_CHILDSYS;
#ifdef MEWEL
          /*
             If this window is not zoomed, then activate the window's
             system menu. If the window *is* zoomed, then the system
             menu is sitting on the frame window's menu bar. So,
             return MC_SELECT with the 0th position in the LOWORD.
          */
          _WinFindandInvokeSysMenu(hwndActive);
#endif
          /* This result will cause no menu to hilite, but because
          ** iNextPopup is set, the call right after the message
          ** processing loop will hilite the child system menu
          */
          return MAKELONG( 0, MC_ABORT );
        }
        break;
      }
      break;


    case WM_SETFOCUS:
      /* Make the MDI child reflect the focus status of the MDI main */
      if (hwndActive)
      {
        SetFocus(hwndActive);
      }
      break;


    case WM_SIZE:
      if (wParam != SIZEICONIC)
      {
        /*
          Resize the MDI Client
        */
        GetClientRect(GetParent(hwndClient), &rcClient);
        MoveWindow(hwndClient, 0, 0, rcClient.right, rcClient.bottom, TRUE);

        /*
          Resize the MDI child if it's zoomed.
        */
        if (MdiGetProp(hwndClient, PROP_ZOOM))
        {
          WINDOW *w = WID_TO_WIN(hwndActive);

          WinGetClient(hwndClient, &rcClient);
          lStyle = w->flags;

          AdjustWindowRect(&rcClient, lStyle, FALSE);

          /*
            MoveWindow expects child windows to pass in
            parent-relative coordinates. However, for a maximized MDI
            window, we want to be able to move the window *above* the
            client area. So, momentarily take off the WS_CHILD bits
            so that MoveWindow will not client-translate the coordinates.
          */
          if (lStyle & WS_CHILD)
            w->flags &= ~WS_CHILD;
          MoveWindow(hwndActive, rcClient.left, rcClient.top,
                                 RECT_WIDTH(rcClient), RECT_HEIGHT(rcClient),
                                 TRUE);
          w->flags |= WS_CHILD;
        }
      }
      break;


    case WM_GETTEXT :
      /*
        If we are retrieving the caption of the frame window, and there
        is a maximized child, then we do not want to get the title
        of the child window (which was appended to the frame title).
      */
      if (MdiGetProp(hwndClient, PROP_ZOOM))
      {
        GetAtomName(MdiGetProp(hwndClient,PROP_TITLE), (LPSTR) lParam, wParam);
        return lstrlen((LPSTR) lParam);
      }
      else
        break;

    case WM_SETTEXT :
      /*
        If we are changing the caption of the frame, and there is a child
        maximized, we want to replace the old atom which corresponds to the
        old title with a new atom which corresponds to the new title. We
        also must adjust the caption so that it is "NewTitle - child".
      */
      if (MdiGetProp(hwndClient, PROP_ZOOM))
      {
        DeleteAtom(MdiGetProp(hwndClient, PROP_TITLE));
        MdiSetProp(hwndClient, PROP_TITLE, AddAtom((LPSTR) lParam));
        DefWindowProc(hwndFrame, message, wParam, lParam);
        MdiChangeFrameCaption(hwndClient, MDIGETACTIVEWINDOW());
        return TRUE;
      }
      else
        break;

  } /* end switch (wParam) */

  return DefWindowProc(hwndFrame, message, wParam, lParam);
}


/****************************************************************************/
/*                                                                          */
/* Function : MdiClientProc()                                               */
/*                                                                          */
/* Purpose  : Window proc for the MDI client window.                        */
/*                                                                          */
/* Returns  :                                                               */
/*                                                                          */
/****************************************************************************/
LRESULT FAR PASCAL MdiClientProc(hWndClient,message,wParam,lParam)
  HWND   hWndClient;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  switch (message)
  {
    /*
      When we create the MDI client window, we need to initialize
      its properties (among other things).
    */
    case WM_CREATE         :
    {  
      LPCLIENTCREATESTRUCT pcs = 
        (LPCLIENTCREATESTRUCT) ((LPCREATESTRUCT) lParam)->lpCreateParams;
      HANDLE hInst;
      char   szTitleName[MAXTEXT];
      WINDOW *w = WID_TO_WIN(hWndClient);

      w->pMDIInfo = emalloc(sizeof(MDICLIENTINFO));
      w->ulStyle |= WIN_IS_MDICLIENT;

      hInst = GetWindowWord(GetParent(hWndClient), GWW_HINSTANCE);
      GetWindowText(GetParent(hWndClient), szTitleName, sizeof(szTitleName));

      /* Message hook for tracking keyboard interface to menu */
      MdiSetMenuKeyHook( hInst );

      MdiSetProp( hWndClient, PROP_WINDOWMENU,   pcs->hWindowMenu);
      MdiSetProp( hWndClient, PROP_FIRSTCHILDID, pcs->idFirstChild);
      /*
        We need to record the number of initial items which are in the
        Window" popup.
      */
      MdiSetProp( hWndClient, PROP_ITEMSBEFORESEP, GetMenuItemCount(pcs->hWindowMenu));
      MdiSetProp( hWndClient, PROP_ACTIVE, NULL);
      MdiSetProp( hWndClient, PROP_COUNT, 0);
      MdiSetProp( hWndClient, PROP_ZOOM, NULL);
      MdiSetProp( hWndClient, PROP_MAINMENU, GetMenu(GetParent(hWndClient)));
      MdiSetProp( hWndClient, PROP_TITLE, AddAtom( szTitleName ) );
      MdiSetProp( hWndClient, PROP_SYSMENU, MdiCreateChildSysBitmap(hWndClient ) );

      /*
        Use the scrollbar filler as the appropriate background of the
        MDI client window.
      */
      w->fillchar = WinGetSysChar(SYSCHAR_SCROLLBAR_FILL);
      w->attr     = WinQuerySysColor(NULLHWND, COLOR_APPWORKSPACE);

      break;
    }

    case WM_MDIACTIVATE :
      if (wParam != NULLHWND && wParam != MDIGETACTIVEWINDOW())
      {
#ifdef MODSOFT
        /*
          See the comments under DefMDIChildProc, WM_MDIACTIVATE.
        */
        MdiActivateChild((HWND) wParam, FALSE);
#else
        SendMessage(MDIGETACTIVEWINDOW(), WM_MDIACTIVATE, FALSE, 
                    MAKELONG(wParam, MDIGETACTIVEWINDOW()));
        SendMessage((HWND) wParam, WM_MDIACTIVATE, TRUE, 
                    MAKELONG(wParam, MDIGETACTIVEWINDOW()));
#endif
      }
      break;


    case WM_SETFOCUS :
    {
      /*
        If the MDI client gets a WM_SETFOCUS message, then it should
        set the focus to the active child window. If there is no
        active child, then the focus should go up to the frame window,
        but only if the frame window was not the window which was losing
        the focus in the first place.

        (5/27/94, maa)
        We also put in a check for wParam == NULL, because if the
        WM_SETFOCUS message was send from MEWEL's menu handler, then
        we want to ensure that the focus is returned to the
        window which originally had the focus... we don't want the
        process of invoking a menu to have any unwanted side effects
        with regard to shifting the focus to another window.
      */
      HWND hFocus = MDIGETACTIVEWINDOW();
      HWND hFrame = GetParent(hWndClient);
      if (hFocus != NULL || (HWND) wParam != hFrame && wParam != NULL)
        SetFocus(hFocus ? hFocus : hFrame);
      break;
    }


    case WM_MDICASCADE :
      MDIDocumentArrange(hWndClient, AWP_CASCADED, wParam);
      break;


    case WM_MDICREATE  :
    {
      HWND hWnd;
      LPMDICREATESTRUCT pcs = (LPMDICREATESTRUCT) lParam;

      if (!pcs)
          return 0L;

      /*
        If a child is maximized already, restore it before
        creating the new child.
      */
      if ((hWnd = MDIGETACTIVEWINDOW()) != NULLHWND && 
           MdiGetProp(hWndClient, PROP_ZOOM))
      {
        SendMessage(hWndClient, WM_MDIRESTORE, hWnd, 0L);
      }

      hWnd = MdiChildCreateWindow(
          pcs->szClass,
          pcs->szTitle,
          pcs->style
#if !defined(MOTIF)
          | WS_VISIBLE
#endif
#if defined(MOTIF)
          | WS_CLIPCHILDREN
#endif
          | WS_CHILD
          | WS_CAPTION
          | WS_THICKFRAME
          | WS_SYSMENU
          | WS_CLIPSIBLINGS,
          pcs->x, pcs->y, pcs->cx, pcs->cy,
          hWndClient,
          (HMENU) wParam,
          pcs->hOwner,
          (VOID FAR *) pcs
          );

#if defined(MOTIF)
      ShowWindow(hWnd, SW_SHOW);
#endif

      return MAKELONG(hWnd, 0);
    }

    case WM_MDIDESTROY  :
      bInMDIDESTROY++;
      MdiDestroyChildWindow((HWND) wParam);
      DestroyWindow((HWND) wParam);
      bInMDIDESTROY--;
      return TRUE;

    case WM_MDIGETACTIVE :
      return MAKELONG(MDIGETACTIVEWINDOW(), 
          (MDIGETACTIVEWINDOW() ? MdiGetProp(hWndClient, PROP_ZOOM) : 0));

    case WM_MDIICONARRANGE :
      return MEWELArrangeIconicWindows(hWndClient, TRUE);

    case WM_MDIMAXIMIZE    :
      if (wParam != MDIGETACTIVEWINDOW())
          SendMessage(hWndClient, WM_MDIACTIVATE, wParam, 0L);
      MdiZoomChild((HWND) wParam);
      MdiSetProp(hWndClient, PROP_ZOOM, (HWND) wParam);
      return 0L;

    case WM_MDINEXT        :
      MdiActivateNextChild(MDIGETACTIVEWINDOW());
      return TRUE;

    case WM_MDIRESTORE     :
    {
      /*
        First, see if the client has a zoomed window. If it does,
        then restore it to its normal size.
        If the window we are restoring is iconic, then activate
        it, restore it, and bring it to the top.
      */
      HWND hChild = MdiGetProp(hWndClient, PROP_ZOOM);
      if (hChild)
      {
        MdiSetProp(hWndClient, PROP_ZOOM, NULL);
        MdiRestoreChild(hChild, TRUE);
      }
      if (MdiGetProp((HWND) wParam, PROP_ICONIZED))
      {
        MdiSetProp((HWND) wParam, PROP_ICONIZED, FALSE);
        MdiActivateChild((HWND) wParam, FALSE);
        ShowWindow((HWND) wParam, SW_RESTORE);
        BringWindowToTop((HWND) wParam);
      }
      return 0L;
    }


    case WM_MDISETMENU     :
    {
      HMENU hOldMenuFrame  = (HMENU) GetMenu(GetParent(hWndClient));
      HMENU hOldMenuWindow = (HMENU) MdiGetProp(hWndClient, PROP_WINDOWMENU);
      HMENU hNewMenuFrame  = (HMENU) LOWORD(lParam);
      HMENU hNewMenuWindow = (HMENU) HIWORD(lParam);

      if (hNewMenuWindow)
      {
        /*
          1) Copy the window id's in the old WINDOW menu to the new
             window menu.
          2) Destroy the old WINDOW menu;
          3) Set the new WINDOW menu.
        */
        HWND hChild;
        HWND hwndActive;
        int  i, nMenuItems, nDocs = 0;

        hwndActive = MdiGetProp(hWndClient, PROP_ACTIVE);
        nMenuItems = MdiGetProp(hWndClient, PROP_ITEMSBEFORESEP);
                     
        if (hNewMenuWindow != hOldMenuWindow)
        {
          MdiSetProp(hWndClient, PROP_WINDOWMENU, hNewMenuWindow);
          MdiSetProp(hWndClient, PROP_ITEMSBEFORESEP, 
                              GetMenuItemCount(hNewMenuWindow));

          /*
            Enumerate all of the MDI child windows and add them to the
            new Window pulldown.
          */
          for (hChild = GetTopWindow(hWndClient);
               hChild;
               hChild = GetWindow(hChild, GW_HWNDNEXT))
          {
            if (MdiGetProp(hChild, PROP_ISMDI) && hChild != hWndBeingDestroyed)
            {
              MdiAppendWindowToMenu(hChild, (BOOL) (hChild==hwndActive));
              nDocs++;
            }
          }

#define DHH
#ifdef DHH
          /*
            Delete all of the MDI child entries from the Windows pulldown,
            and get rid of the separator bar while we're at it!
          */
          if (hOldMenuWindow && nDocs)
          {
            nMenuItems = GetMenuItemCount(hOldMenuWindow);
            for (i = 0;  i <= nDocs;  i++)
              DeleteMenu(hOldMenuWindow, nMenuItems-1-i, MF_BYPOSITION);
          }
#else
          /*
            Delete the Window pulldown from the old menubar only
            if we are keeping the same menubar.
          */
          if (!hNewMenuFrame)
            ChangeMenu(hOldMenuFrame, GetMenuItemCount(hOldMenuFrame) - 1,
                       (LPSTR) NULL, 0, MF_BYPOSITION | MF_DELETE);

          /*
            If we are keeping the same menubar and just swapping
            Window pulldowns, add the new Window pulldown to the
            old menubar.
          */
          if (!hNewMenuFrame)
            ChangeMenu(hOldMenuFrame, 0, "&Window", hNewMenuWindow,
                       MF_APPEND | MF_BYPOSITION | MF_POPUP);
#endif
        } /* end if (hNewMenuWindow != hOldMenuWindow) */
      } /* end if (hNewMenuWindow) */


      if (hNewMenuFrame)
      {
#ifdef MODSOFT
        HWND hwndActive = MdiGetProp(hWndClient,PROP_ACTIVE);
        BOOL bZoomed = (BOOL) 
                (GetWindowLong(hwndActive,GWL_STYLE) & WS_MAXIMIZE) != 0;
#endif
#if 0
        /* Don't do this ... it's up to the app to do this */
        DestroyMenu(hOldMenuFrame);  
#endif
#ifdef MODSOFT
        /*
          If the document is zoomed, remove the
          document system menu from the old frame menu
        */
        if (bZoomed)
          if (hOldMenuFrame)
            MdiRestoreMenu(hwndActive);
#endif
        SetMenu(GetParent(hWndClient), hNewMenuFrame);
        MdiSetProp(hWndClient, PROP_MAINMENU, hNewMenuFrame);
#ifdef MODSOFT
        /*
          If the document is zoomed, add a
          document system menu to the new frame menu
        */
        if (bZoomed)
          MdiZoomMenu(hwndActive);
#endif
      } /* end if (hNewMenuFrame) */

      return hOldMenuFrame;
    }


    case WM_MDITILE        :
      MDIDocumentArrange(hWndClient, AWP_TILED, wParam);
      break;


   case WM_NCHITTEST :
   {
     /*
       Ignore clicks in the MDI client area.
     */
     INT iHitCode = (INT) DefWindowProc(hWndClient, message, wParam, lParam);
     return (iHitCode == HTCLIENT) ? HTNOWHERE : iHitCode;
   }

#ifdef MODSOFT
    case WM_ACTIVATE:
    {
      HWND hwndActive = MDIGETACTIVEWINDOW();

      /* MDI child reflects the activation status of the parent */
      if (hwndActive)
      {
        switch (wParam)
        {
          case 1:
          case 2:
            /* Activating */
            if (hwndActive && hwndActive == LOWORD(lParam))
            {
              InternalSysParams.hWndActive = hwndActive;
              SendMessage(hwndActive,WM_ACTIVATE,1,MAKELONG(hWndClient,0));
              SendMessage(hwndActive,WM_NCACTIVATE,1,0L);
            }
            else
            {
              InternalSysParams.hWndActive = NULLHWND;
              SetFocus(GetParent(hWndClient));
            }
            return 0;
          }
        }
        break;
    }
#endif
    }

    return DefWindowProc( hWndClient, message, wParam, lParam );
}


/**/

/*
 * MdiChildCreateWindow( ... ) : HWND;
 *
 *    szClassName    Class name of document
 *    szTitleName    Caption for document
 *    lStyle         Style of document
 *    wLeft          Left position of window
 *    wTop           Top position of window
 *    wWidth         Width of window
 *    wHeight        Height of window
 *    hwndMain       MDI desktop
 *    hmenuChild     Handle to document menu
 *    hInst          Current instance handle
 *    lpCreateParam  Pointer to creation parameters
 *
 * Create an MDI document window.  The parameters exactly match the
 * Windows call CreateWindow() except that the hmenuChild message
 * must be a valid menu handle, whereas the CreateWindow() call expects
 * a child id.  The MDI code keeps track of document information in
 * property lists, such as handles to menus and accelerators, as well
 * as whether this child of the MDI desktop is an MDI document.
 *
 */
static HWND PASCAL MdiChildCreateWindow(szClassName,szTitleName,lStyle,
                                        wLeft,wTop,wWidth,wHeight,
                                        hwndMain, hmenuChild,
                                        hInst, lpCreateParam)
  LPCSTR      szClassName;
  LPCSTR      szTitleName;
  DWORD       lStyle;
  int         wLeft;
  int         wTop;
  int         wWidth;
  int         wHeight;
  HWND        hwndMain;
  HMENU       hmenuChild;
  HANDLE      hInst;
  LPSTR       lpCreateParam;
{
  int         iCount;                 /* Child window ID */
  int         wLeftAct = wLeft;       /* Left position of window */
  int         wTopAct = wTop;         /* Top position of window */
  int         wWidthAct = wWidth;     /* Width of window */
  int         wHeightAct = wHeight;   /* Height of window */
  HWND        hwndActive;       /* Currently active document window */
  HWND        hwndChild;        /* Handle of document window */
  RECT        rcClient;         /* Client area of MDI desktop */
  DWORD       lOrigStyle = lStyle;
  WINDOW      *w;

  (void) hmenuChild;

  /* Calculate size & position */
  iCount = MdiGetProp( hwndMain, PROP_COUNT );

  GetClientRect(hwndMain, &rcClient);

  /*
    Get the proper dimensions if we leave it up to Windows/MEWEL
  */
  if (IS_CW_USEDEFAULT(wLeftAct))
  {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    wLeftAct = 15 * iCount;
#else
    wLeftAct = 2 * iCount;
#endif
    wTopAct = CW_USEDEFAULT;
  }
  if (IS_CW_USEDEFAULT(wTopAct))
  {
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    wTopAct = 15 * iCount;
#else
    wTopAct = 2 * iCount;
#endif
  }
  if (IS_CW_USEDEFAULT(wWidthAct))
  {
    wWidthAct = (RECT_WIDTH(rcClient) * 2) / 3;
  }
  if (IS_CW_USEDEFAULT(wHeight))
  {
    wHeightAct = (RECT_HEIGHT(rcClient) * 2) / 3;
  }

  /*
    Ensure that it falls within client area
  */
  if (IS_CW_USEDEFAULT(wLeft))
    while ( wLeftAct >= rcClient.right )
      wLeftAct -= rcClient.right;
  if (IS_CW_USEDEFAULT(wTop))
    while ( wTopAct >= rcClient.bottom )
      wTopAct -= rcClient.bottom;

  /*
    An MDI window should not be minimized or maximized to start off with.
  */
  lStyle &= ~(WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);

  /*
    Create the MDI Child window
  */
  hwndChild = CreateWindow(szClassName,
                           szTitleName,
                           lStyle | WS_CHILD,
                           wLeftAct, wTopAct, wWidthAct, wHeightAct,
                           hwndMain,
                           (HMENU) 0,
                           hInst,
                           lpCreateParam);

  /* Success? */
  if (!hwndChild)
    return NULLHWND;

  /*
    Allocate the internal MDI data structure and attach it to the child
  */
  if ((w = WID_TO_WIN(hwndChild)) != (WINDOW *) NULL)
    w->pMDIInfo = emalloc(sizeof(MDICHILDINFO));

  /*
    Tell the MDI client how many children we have
  */
  MdiSetProp(hwndMain, PROP_COUNT, ++iCount);

  /*
    Keep important info within the per-window data structure
  */
  MdiSetProp(hwndChild, PROP_ACCEL, 0);
  MdiSetProp(hwndChild,PROP_MENUID,
        MdiGetProp(hwndMain,PROP_FIRSTCHILDID) + (nTotalMDIDocsCreated++));
  MdiSetProp(hwndChild, PROP_ISMDI, TRUE);

  /*
    Now that the window has the WIN_IS_MDIDOC style, create the proper
    system menu.
  */
  w->flags |= WS_SYSMENU;
  WinCreateSysMenu(hwndChild);
#if defined(MOTIF)
  XMEWELCalcNCDecorations(hwndChild);
#endif

  /* Reflect change in WINDOW menu */
  MdiAppendWindowToMenu(hwndChild, TRUE);

  /* Let's activate this MDI child */
  hwndActive = MdiGetProp(hwndMain, PROP_ACTIVE);
  if (MdiGetProp(hwndMain, PROP_ZOOM))
    MdiSwitchZoom(hwndChild, hwndActive);
  MdiActivateChild(hwndChild, FALSE);
  bSwitchingZoom = FALSE;

  /*
    Show menu bar changes now
  */
  DRAWMENUBAR(GetParent(hwndMain));

  /*
    Now apply the minimize or maximize styles.
  */
  if (lOrigStyle & WS_MAXIMIZE)
    SendMessage(hwndChild, WM_SYSCOMMAND, SC_MAXIMIZE, 0L);
  else if (lOrigStyle & WS_MINIMIZE)
    SendMessage(hwndChild, WM_SYSCOMMAND, SC_MINIMIZE, 0L);

  return hwndChild;
}


/*
 * MdiChildDefWindowProc( hwndChild, message, wParam, lParam ) : long;
 *
 *    hwndChild      Handle to document
 *    message        Current message
 *    wParam         Word parameter to message
 *    lParam         Long parameter to message
 *
 * Handle MDI document messages.  Messages reach here only after the
 * application document window proedure has processed those that it
 * wants.  Major occurances that are recognized in this function
 * include:  child system menu choices, window creation, window
 * activation, window destruction, and translating <alt>- into
 * selecting the child system menu.
 *
 */
LRESULT FAR PASCAL DefMDIChildProc(hwndChild, message, wParam, lParam)
  HWND   hwndChild;
  UINT   message;
  WPARAM wParam;
  LPARAM lParam;
{
  HWND hwndMain;         /* Handle to MDI desktop */
  int  i;

  hwndMain = GetParent(hwndChild);
  switch (message)
  {
#if 1
    /*
      4/19/91 (maa)
      A window may never get the WM_CLOSE message if called from
      WM_QUERYENDSESSION.
    */
    case WM_DESTROY:
#endif
    case WM_CLOSE:
      /* We're going away */
      if (!bInMDIDESTROY)
        MdiDestroyChildWindow(hwndChild);
      break;

    /*
      In Windows, these are all generated by the fake child system menu
    */
    case WM_COMMAND:
      for (i = 0;  i < sizeof(IDMtoSC) / sizeof(IDMtoSC[0]);  i++)
        if (IDMtoSC[i].idmCmd == wParam)
        {
          SendMessage(hwndChild, WM_SYSCOMMAND, IDMtoSC[i].scCmd, lParam);
          break;
        }
      break;


    case WM_INITMENU:
#ifdef MODSOFT
      /*
        Is this necessary?  When you have a frame window without a
        system menu, and the MDi document is not zoomed, calling
        MdiInitSystemMenu() causes heap space to be wasted.
      */
#else
      MdiInitSystemMenu(hwndChild);
#endif
      break;


    case WM_MENUCHAR:
      /* Was the <alt>- key hit when no popup currently popped up? */
#ifdef MEWEL
#else
      if ((LOWORD(lParam) & MF_POPUP) == 0)
#endif
      {
        if (wParam == '-')
        {
#ifdef MEWEL
          /*
            If this window is not zoomed, then activate the window's
            system menu. If the window *is* zoomed, then the system
            menu is sitting on the frame window's menu bar. So,
            return MC_SELECT with the 0th position in the LOWORD.
          */
          if (!MdiGetProp(hwndMain, PROP_ZOOM))
          {
            _WinFindandInvokeSysMenu(hwndChild);
            return MAKELONG(0, MC_ABORT);
          }  
#endif
          /* Let Alt-Minus mean activate our system menu */
          return MAKELONG(0, MC_SELECT);
        }
        else
        {
          /* Another <alt> key stroke, let the parent handle it */
          SendMessage(GetParent(hwndMain), WM_SYSCOMMAND, SC_KEYMENU, (DWORD) wParam);
          return MAKELONG(0, MC_ABORT);
        }
      }


    case WM_MENUSELECT:
      iCurrentPopup = POP_CHILDSYS;
      break;


#ifdef CITICORP
    case WM_NCLBUTTONDOWN:
    {	
      WINDOW *w = WID_TO_WIN(hwndChild);
      int    mouserow = HIWORD(lParam);
      /*
        Check for a mouse-button down on the top row of a window.
        If this occurs, then activate the MDI child window.  The
        default window proc. doesn't do this properly (just does
        a BringWindowToTop() -- bug?).
      */
      if (mouserow == w->rect.top)
        MdiActivateChild(hwndChild, FALSE);
      /*
        Send the message on to the default window proc.  It will do
        another BringWindowToTop() but that should be OK.
      */
      break;
    }
#endif


    case WM_MOUSEACTIVATE:
      MdiActivateChild(hwndChild, FALSE);
      break;


    case WM_SYSCOMMAND:
      switch(wParam & 0xFFF0)
      {
        case SC_KEYMENU:
          if (MdiGetProp(hwndMain, PROP_ZOOM) || LOWORD(lParam) != '-')
          {
            /* Pass any other Alt-Key messages to the parent */
            return SendMessage(hwndMain, WM_SYSCOMMAND, wParam, lParam);
          }
          break;

        case SC_MAXIMIZE:
          if (MdiGetProp(hwndMain, PROP_ZOOM))
          {
            MdiSetProp(hwndMain, PROP_ZOOM, NULL);
            MdiRestoreChild(hwndChild, TRUE);
          }
          else
          {
            if (MdiGetProp(hwndChild, PROP_ICONIZED))
              SendMessage(hwndChild, WM_SYSCOMMAND, SC_RESTORE, 0L);
            MdiSetProp(hwndMain, PROP_ZOOM, hwndChild);
            MdiZoomChild(hwndChild);
          }
          return 0L;

        case SC_MINIMIZE:
          if (MdiGetProp(hwndChild, PROP_ICONIZED))
          {
            MdiSetProp(hwndChild, PROP_ICONIZED, FALSE);
            ShowWindow(hwndChild, SW_MINIMIZE);
          }
          else
          {
            if (MdiGetProp(hwndMain, PROP_ZOOM))
              SendMessage(hwndChild, WM_SYSCOMMAND, SC_RESTORE, 0L);
            MdiSetProp(hwndChild, PROP_ICONIZED, TRUE);
            MdiMinimizeChild(hwndChild);
          }
          return 0L;


        case SC_RESTORE:
          if (MdiGetProp(hwndMain, PROP_ZOOM))
          {
            MdiSetProp(hwndMain, PROP_ZOOM, NULL);
            MdiRestoreChild(hwndChild, TRUE);
          }
          else if (MdiGetProp(hwndChild, PROP_ICONIZED))
          {
            MdiSetProp(hwndChild, PROP_ICONIZED, FALSE);
#ifdef CITICORP
            MdiActivateChild(hwndChild, FALSE);
#endif
            ShowWindow(hwndChild, SW_RESTORE);
            BringWindowToTop(hwndChild);
          }
          return 0L;

        case SC_NEXTWINDOW:
          /* This function is achieved only through the keyboard */
          MdiActivateNextChild(hwndChild);
          return 0L;

        case SC_PREVWINDOW:
          /* This function is achieved only through the keyboard */
          MdiActivatePrevChild(hwndChild);
          return 0L;
      }
      break;


    case WM_GETMINMAXINFO :
      /*
        No need to process this in MEWEL, as WinZoom() correctly
        calculates the default maximized size based on the parent's
        client area.
      */
      break;


    case WM_MDIACTIVATE :
      if (!bInMDIActivate)
      {
#ifdef MODSOFT
        /*
          Don't do this here.  Instead, change the handling of WM_MDIACTIVATE
          in DefFrameProc(), so that *it* does these things.  This way,
          the document window does not have to pass WM_MDIACTIVATE to this
          procedure. See Petzold, _Programming_Windows, page 867 - the document
          does not pass WM_MDIACTIVATE to DefMDIChildProc().
        */
#else
        if (wParam)
          MdiActivateChild(hwndChild, FALSE);
        else
          MdiDeactivateChild(hwndChild);
#endif
      }
      break;


    case WM_SETTEXT :
    {
      /*
        If we change the text of an MDI child's caption, then this
        change must be reflected in the "Window" pulldown menu.
      */
      LONG  ulRC;
      HMENU hMenu;
      UINT  wFlags;

      ulRC = DefWindowProc(hwndChild, message, wParam, lParam);

      /*
        Get a handle to the "Window" pulldown and change the item
        which has the command id which matches the child's PROP_MENUID.
      */
      if ((hMenu = MdiGetProp(hwndMain, PROP_WINDOWMENU)) != NULL)
      {
        char szText[128], *pszDot;
        UINT wPropID;

        /*
          Get the command id which the child window was mapped to
        */
        wPropID = MdiGetProp(hwndChild, PROP_MENUID);

        /*
          Get the old menu string, and append the title of the child window
          right after the "%d. " (where %d is the ordinal of the child).
        */
        GetMenuString(hMenu, wPropID, szText, sizeof(szText), MF_BYCOMMAND);
        pszDot = strchr(szText, '.');
        strcpy(pszDot ? pszDot+2 : szText, WID_TO_WIN(hwndChild)->title);

        /*
          Change the text of the menu item.
        */
        wFlags = GetMenuState(hMenu, wPropID, MF_BYCOMMAND);
        ChangeMenu(hMenu, wPropID, szText, wPropID,
                   MF_BYCOMMAND | MF_CHANGE | MF_STRING | wFlags);

      }

      /*
        If the child is maximized and we change the title, then we must
        change the title of the frame window.
      */
      if (WinGetFlags(hwndChild) & WS_MAXIMIZE)
        MdiChangeFrameCaption(hwndMain, hwndChild);

      return ulRC;
    }

  } /* end switch */

  return DefWindowProc(hwndChild, message, wParam, lParam);
}

/**/

/*
 * MdiGetMessage( hwndMain, lpMsg, hWnd, wMin, wMax ) : BOOL;
 *
 *    hwndMain       Handle to MDI desktop
 *    lpMsg          Message structure to receive message
 *    hWnd           Are messages for a specific window only?
 *    wMin           Is there a minimum message number?
 *    wMax           Is there a maximum message number?
 *
 * Get a normal windows message only after checking for keyboard
 * access to the menus.
 *
 */
#if 0
BOOL PASCAL MdiGetMessage(hwndMain, lpMsg, hWnd, wMin, wMax)
  HWND  hwndMain;
  LPMSG lpMsg;
  HWND  hWnd;
  UINT  wMin;
  UINT  wMax;
{
  /* Process keyboard interface to menu */
  MdiMenuMessageLoopUpdate( hwndMain );

  /* Now go get the next message */
  return GetMessage(lpMsg, hWnd, wMin, wMax);
}
#endif
/* */

/*
 * MdiTranslateAccelerators( hwndMain, lpMsg ) : int
 *
 *    hwndMain       Handle to MDI desktop
 *    lpMsg          Message structure containing message
 *
 * Translate this message via one of two accelerator tables:
 *  1) The document's system menu (i.e. <ctrl><F4>)
 *  2) The document's own accelerator table.  This table
 *     is set with the MdiSetAccelerator() call.
 *
 */

BOOL FAR PASCAL TranslateMDISysAccel(hwndMain, lpMsg)
  HWND  hwndMain;
  LPMSG lpMsg;
{
  int    iResult = 0;      /* Result of last translation */
  HANDLE hAccel;           /* Each of the two accel tables */
  HWND   hwndChild;        /* Handle to document */

  /* Is there a document on the MDI desktop? */
  hwndChild = MdiGetProp(hwndMain, PROP_ACTIVE);
  if (hwndChild)
  {
    /* Check for system accelerators */
#ifdef MEWEL
    if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_SYSKEYDOWN)
    {
#if defined(USE_WINDOWS_COMPAT_KEYS)
      if (lpMsg->wParam == VK_F4 && GetKeyState(VK_CONTROL))
#else
      if (lpMsg->wParam == VK_CTRL_F4)
#endif
      {
        lpMsg->hwnd    = hwndChild;
        lpMsg->message = WM_SYSCOMMAND;
        lpMsg->wParam  = SC_CLOSE;
        lpMsg->lParam  = 0L;
      }
      else
      {
        /*
          ALT+' ' should open up the system menu on the frame window.
          This is the grandparent of the child window.
        */
        if (lpMsg->wParam == ' ' && (lpMsg->lParam & 0x20000000L) != 0L)
          lpMsg->hwnd = GetParent(GetParent(hwndChild));
      }
    }
#else
    if ((hAccel = MdiGetProp(hwndMain, PROP_CTRLACCEL)) != 0)
      iResult = TranslateAccelerator(hwndMain, hAccel, lpMsg);
#endif

    if (!iResult)
    {
      /* Check for document accelerators */
      hAccel = MdiGetProp(hwndChild, PROP_ACCEL);
      if (hAccel)
        iResult = TranslateAccelerator(hwndMain, hAccel, lpMsg);
    }
  }

  return (BOOL) iResult;
}

/**/
/*
 * MDI2.C - Child Window Routines
 *
 * LANGUAGE      : Microsoft C5.1
 * MODEL         : medium
 * ENVIRONMENT   : Microsoft Windows 2.1 SDK
 * STATUS        : operational
 *
 * This module contains all the MDI code to handle the activation
 * sequence for and switching activation between the MDI document
 * windows.  It also contains the maximizing and restoring of document
 * windows and especially the activation and switching between them.
 * It also contains the code for the unhide dialog box.
 *
 * (C) Copyright 1988
 * Eikon Systems, Inc.
 * 989 E. Hillsdale Blvd, Suite 260
 * Foster City  CA  94404
 *
 */

/**/

/*
 * MdiDestroyChildWindow( hwndChild) : void;
 *
 *    hwndChild      Handle of document window
 *
 * This routine is called when a document window is closing.  It picks
 * a new active document window and activates it.  If no other document
 * window is available (visible) then it restores the original menu
 * on the MDI desktop and sets the focus to the desktop.
 *
 */
static VOID PASCAL MdiDestroyChildWindow(hwndChild)
  HWND hwndChild;
{
  HWND  hwndNext;         /* Next active document window */
  HWND  hWndClient;       /* Handle to MDI desktop */
  INT   iCount;

  hWndBeingDestroyed = hwndChild;

  /* Remove ourselves from the menus */
  MdiRemoveWindowFromMenu(hwndChild, TRUE);

  /* Let's choose a new MDI child to be active */
  hWndClient = GetParent(hwndChild);
  /* One less window.... */
  if ((iCount = MdiGetProp(hWndClient, PROP_COUNT) - 1) < 0)
    iCount = 0;
  MdiSetProp(hWndClient, PROP_COUNT, iCount);

  hwndNext = MdiChooseNewActiveChild(hwndChild);
  if (hwndNext)
  {
    /* Make this MDI child active */
    if (MdiGetProp(hWndClient, PROP_ZOOM))
      MdiSwitchZoom(hwndNext, hwndChild);
    MdiActivateChild(hwndNext, FALSE);
    bSwitchingZoom = FALSE;
  }
  else
  {
    /* No other MDI child visible */
    if (MdiGetProp(hWndClient, PROP_ZOOM))
    {
      MdiSetProp(hWndClient, PROP_ZOOM, NULL);
      MdiRestoreChild(hwndChild, FALSE);
    }

    /*
      Deactivate the child
    */
    MdiDeactivateChild(hwndChild);
    MdiSetProp(hWndClient, PROP_ACTIVE, NULL);
    /* Focus back to parent */
#ifdef MODSOFT
    _HwndMDIActiveChild = NULLHWND;
    InternalSysParams.hWndActive = NULLHWND;
#endif

#if 32993
    /*
      3/29/93 (maa)
        If we destroy the only child window, then we want the MDI frame
      to be the active window.
    */
    _WinActivate(GetParent(hWndClient), FALSE);
#endif

    SetFocus(hWndClient);
  }

  /* Ensure menu updated */
  DRAWMENUBAR(GetParent(hWndClient));
  hWndBeingDestroyed = NULLHWND;
}

/**/

/*
 * MdiActivateChild(hwndChild, bHidden) : void;
 *
 *    hwndChild      Handle to document window
 *    bHidden        Was document previous hidden?
 *
 * Bring a specific document window to the fore.  Give it the correct
 * title bar, bring it to the top, put the matching menu on the MDI
 * desktop, etc.
 *
 */
static VOID PASCAL MdiActivateChild(HWND hwndChild, BOOL bHidden)
{
  HMENU hmenuWindow;      /* WINDOW submenu */
  HWND  hwndActive;       /* Handle to previous document */
  HWND  hWndClient;       /* Handle to MDI desktop */
  LONG  lStyle;           /* Style of document window */


  /* Check if this MDI child already activated */
  hWndClient = GetParent(hwndChild);
  lStyle = GetWindowLong(hwndChild, GWL_STYLE);

#if !51793
  /*
    5/17/93 (maa)
      This seems to break MFC2
  */
  if ((lStyle & WS_MAXIMIZEBOX) == 0 || bZoomingIcon)
#endif
  {
    /* Change the title bar color */
    SendMessage(hwndChild, WM_NCACTIVATE, TRUE, 0L);

    /* Add system menu & maximize/minimize arrow to title bar */
    SetWindowLong(hwndChild, GWL_STYLE, 
                  lStyle | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
#ifdef MEWEL
    if (GetSystemMenu(hwndChild, FALSE) == NULLHWND)
      WinCreateSysMenu(hwndChild);
#if defined(MOTIF)
    XMEWELCalcNCDecorations(hwndChild);
#endif
#endif

    /* Make changes visible */
    if (bHidden)
    {
#ifdef MODSOFT
      if (!MdiGetProp(hWndClient, PROP_ZOOM))
#endif
      {
        BringWindowToTop(hwndChild);
        ShowWindow(hwndChild, SW_SHOW);
      }
    }
    else
    {
      /* Bring to top & draw nonclient area */
#ifdef MODSOFT
      if (!MdiGetProp(hWndClient, PROP_ZOOM))
#endif
      SetWindowPos(hwndChild,NULL,0,0,0,0,SWP_DRAWFRAME|SWP_NOMOVE|SWP_NOSIZE);
    }

    /* Update this child */
    hwndActive = MdiGetProp(hWndClient, PROP_ACTIVE);
    if (hwndActive != hwndChild)
    {
      /* Remove traces of activeness from previous MDI child */
      if (hwndActive && hwndActive != hwndChild)
      {
        MdiDeactivateChild(hwndActive);
      }

      /* This child now active */
      MdiSetProp(hWndClient, PROP_ACTIVE, hwndChild);
      bInMDIActivate++;
      SendMessage(hwndChild,WM_MDIACTIVATE,TRUE,MAKELONG(hwndChild,hwndActive));
      bInMDIActivate--;
    }

    /* Check our menu on the WINDOW menu */
    hmenuWindow = MdiGetProp(hWndClient, PROP_WINDOWMENU);
    CheckMenuItem(hmenuWindow, MdiGetProp(hwndChild, PROP_MENUID),
                  MF_CHECKED | MF_BYCOMMAND);

    /* Should have focus */
    if (InternalSysParams.hWndFocus != hwndChild)
    {
      SetFocus(hwndChild);
    }

    _HwndMDIActiveChild = hwndChild;
#ifdef MODSOFT
    if (MdiGetProp(hWndClient, PROP_ZOOM))
      MdiZoomChild(hwndChild);
#endif
  }
  bZoomingIcon = FALSE;
}

/**/

/*
 * MdiActivateNextChild(hwndChild) : void;
 *
 *    hwndChild      Handle to current document
 *
 * Activate the next document in the internal windows queue.
 * This function reached via <ctrl><F6> only.
 *
 */
static VOID PASCAL MdiActivateNextChild(hwndChild)
  HWND hwndChild;
{
  HWND hwndActive;       /* Handle to previous document */
  HWND hwndIter;         /* Iterating document windows */
  HWND hWndClient;       /* Handle to MDI desktop */

  for (hwndIter = GetWindow(hwndChild, GW_HWNDNEXT);
       hwndIter != NULL;
       hwndIter = GetWindow(hwndIter, GW_HWNDNEXT))
    /*
      Get next visible, MDI child in internal windows chain
    */
    if (MdiGetProp(hwndIter, PROP_ISMDI) && IsWindowVisible(hwndIter)
        && !IsIconic(hwndIter))
      break;


  /* Did we find another MDI child? */
  if (hwndIter)
  {
    hWndClient = GetParent(hwndIter);
    hwndActive = MdiGetProp(hWndClient, PROP_ACTIVE);
    if (hwndActive == hwndIter)  /* no next window */
      return;
    if (MdiGetProp(hWndClient, PROP_ZOOM))
    {
      /* Activate the new one */
      MdiSwitchZoom(hwndIter, hwndActive);
      MdiActivateChild(hwndIter, FALSE);
      bSwitchingZoom = FALSE;
      /* Place window at end of internal windows queue */
      SetWindowPos(hwndChild,(HWND)1,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    }
    else
    {
      MdiActivateChild(hwndIter, FALSE);
      /* Place window at end of internal windows queue */
      SetWindowPos(hwndChild,(HWND)1,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

      /* Activate the new one */
      MdiActivateChild(hwndIter, FALSE);
    }
  }
}

/**/

/*
 * MdiActivatePrevChild(hwndChild) : void;
 *
 *    hwndChild      Handle to current document
 *
 * Activate the previous document in the internal windows queue.
 * This function reached via <ctrl><shift><F6> only.
 *
 */
static VOID PASCAL MdiActivatePrevChild(hwndChild)
  HWND hwndChild;
{
  HWND hwndActive;       /* Handle to previous document */
  HWND hwndIter;         /* Iterate document windows */
  HWND hWndClient;       /* Handle to MDI desktop */

  for (hwndIter = GetWindow(hwndChild, GW_HWNDLAST);
       hwndIter != NULL;
       hwndIter = GetWindow(hwndIter, GW_HWNDPREV))
    /*
      Get prev visible, MDI window in internal windows chain
    */
    if (MdiGetProp(hwndIter, PROP_ISMDI) && IsWindowVisible(hwndIter)
        && !IsIconic(hwndIter))
      break;

  /* Did we find another MDI child? */
  if (hwndIter)
  {
    /* Activate the new one */
    hWndClient = GetParent(hwndChild);
    hwndActive = MdiGetProp(hWndClient, PROP_ACTIVE);
    if (MdiGetProp(hWndClient, PROP_ZOOM))
    {
      MdiSwitchZoom(hwndIter, hwndActive);
    }
    MdiActivateChild(hwndIter, FALSE);
    bSwitchingZoom = FALSE;
  }
}

/**/

/*
 * MdiDectivateChild(hwndChild) : void;
 *
 *    hwndChild      Handle to document window
 *
 * Put a specific document window to the back.  Either another
 * document is being activated or the application is losing focus.
 *
 */
static VOID PASCAL MdiDeactivateChild(hwndChild)
  HWND hwndChild;
{
  LONG lStyle;           /* Style of document window */

  /* Check if this MDI child already deactivated */
  lStyle = GetWindowLong(hwndChild, GWL_STYLE);
  if (lStyle & WS_MAXIMIZEBOX)
  {
    /*
      Restore the child to normal size if it's maximized
    */
    if (lStyle & WS_MAXIMIZE)
      MdiRestoreChild(hwndChild, FALSE);

    /* Change the title bar color */
    SendMessage(hwndChild, WM_NCACTIVATE, FALSE, 0L);

    /* Remove system menu & maximize arrow from title bar */
    SetWindowLong(hwndChild, GWL_STYLE,
                   lStyle & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU));
#if defined(MOTIF)
    XMEWELCalcNCDecorations(hwndChild);
#endif

    SetWindowPos(hwndChild, NULL, 0, 0, 0, 0,
                 SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOMOVE
                               | SWP_NOSIZE     | SWP_NOZORDER);
    bInMDIActivate++;
    SendMessage(hwndChild, WM_MDIACTIVATE, FALSE, MAKELONG(NULL, hwndChild));
    bInMDIActivate--;
  }

  /* Remove check mark for our window */
  CheckMenuItem(MdiGetProp(GetParent(hwndChild), PROP_WINDOWMENU),
                MdiGetProp(hwndChild, PROP_MENUID),
                MF_UNCHECKED | MF_BYCOMMAND);
}

/**/
static VOID PASCAL _MDIZoomChild(HWND, BOOL);

/*
 * MdiZoomChild(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Bring the specific document to full client area display.  A default
 * maximize puts the border and title bar in the client area.  This
 * routine matches the document client area to the MDI desktop client
 * area.  It also appends the document title to the MDI desktop
 * title.
 *
 */
static VOID PASCAL MdiZoomChild(hwndChild)
  HWND hwndChild;
{
  HWND hWndClient;  /* Handle to MDI desktop */

  /* Change the menu */
  hWndClient = GetParent(hwndChild);
  MdiZoomMenu(hwndChild);
  DRAWMENUBAR(GetParent(hWndClient));

  if (!bSwitchingZoom)
    _MDIZoomChild(hwndChild, FALSE);
  MdiSetProp(hWndClient, PROP_ZOOM, hwndChild);
}

/**/

/*
 * MdiSwitchZoom(hwndNew, hwndCur) : void;
 *
 *    hwndNew        Handle to new document
 *    hwndCur        Handle to previous document
 *
 * This routine switches activation between two document windows
 * when the original document window is maximized.  Every time document
 * windows switch, the sequence of activation is important.  When the
 * documents are maximized, it is crucial to not follow the ordinary
 * sequence, otherwise it is possible to see the other, non-maximized
 * document windows briefly before the new document window covers
 * the client area.
 *
 */
static BOOL PASCAL MdiSwitchZoom(hwndNew, hwndCur)
  HWND hwndNew, hwndCur;
{
  LONG        lStyle;           /* Style of document window */

  /* NEW WINDOW */
  _MDIZoomChild(hwndNew, TRUE);

  /* Update menu */
  MdiZoomMenu(hwndNew);

  /* Display the window */
  ShowWindow(hwndNew, SW_SHOW);

  /* Client area is of the old window */
  InvalidateRect(hwndNew, (LPRECT) NULL, TRUE);

  /* OLD WINDOW */

  /* Mark normal size in the window style */
  lStyle = GetWindowLong(hwndCur, GWL_STYLE);
  SetWindowLong(hwndCur, GWL_STYLE, lStyle & ~WS_MAXIMIZE);

  /* Move the window back to where it was before it zoomed */
  SetWindowPos(hwndCur, (HWND) NULL,
               MdiGetProp(hwndCur, PROP_LEFT),
               MdiGetProp(hwndCur, PROP_TOP),
               MdiGetProp(hwndCur, PROP_WIDTH),
               MdiGetProp(hwndCur, PROP_HEIGHT),
               SWP_NOACTIVATE | SWP_NOZORDER);

  /* Update menu */
  MdiRestoreMenu(hwndCur);
  DRAWMENUBAR(hwndCur);
  bSwitchingZoom = TRUE;
  return bZoomingIcon;
}


static VOID PASCAL _MDIZoomChild(HWND hChild, BOOL bSwitchingZoom)
{
  HWND hWndClient;       /* Handle to MDI desktop */
  LONG lStyle;           /* Style of document window */
  RECT rcClient;         /* Client area of MDI desktop */

  /*
    Get a handle to the MDI client
  */
  hWndClient = GetParent(hChild);

  /*
    Change the title bar
  */
  MdiChangeFrameCaption(hWndClient, hChild);
  
  /*
    If the window is not yet visible, then show it.
  */
  if (!IsWindowVisible(hChild))
    ShowWindow(hChild, SW_SHOWNA);

  /*
    If the window which is going to be zoomed is currently an icon,
    restore it first.
  */
  if (MdiGetProp(hChild, PROP_ICONIZED))
  {
    MdiSetProp(hChild, PROP_ICONIZED, FALSE);
    ShowWindow(hChild, SW_RESTORE);
    bZoomingIcon++;
  }

  /* 
    Remember where this window was in the client area
  */
  GetWindowRect(hChild, &rcClient);
  ScreenToClient(hWndClient, (LPPOINT) &rcClient+0);
  ScreenToClient(hWndClient, (LPPOINT) &rcClient+1);
  MdiSetProp(hChild, PROP_LEFT,   rcClient.left);
  MdiSetProp(hChild, PROP_TOP,    rcClient.top);
  MdiSetProp(hChild, PROP_WIDTH,  RECT_WIDTH(rcClient));
  MdiSetProp(hChild, PROP_HEIGHT, RECT_HEIGHT(rcClient));

  /* 
    Reposition the new window. The call to SetWindowPos will move and
    resize the new window and also bring it to the top.
  */
  WinGetClient(hWndClient, &rcClient);
  lStyle = GetWindowLong(hChild, GWL_STYLE);
  WinScreenRectToClient(hWndClient, &rcClient);
  /*
    Need to do this here cause, when SetWindowPos() redraws the
    window, it has to draw the min-max box correctly...
  */
  SetWindowLong(hChild, GWL_STYLE, lStyle | WS_MAXIMIZE);
  AdjustWindowRect(&rcClient, lStyle, FALSE);

  /*
    If a child window has scrollbars, then we still want the scrollbars to
    appear. So, we need to adjust the client rect.
  */
  if (WinHasScrollbars(hChild, SB_VERT))
  {
    rcClient.right  -= IGetSystemMetrics(SM_CXVSCROLL);
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    rcClient.right  -= IGetSystemMetrics(SM_CXFRAME);
#endif
  }
  if (WinHasScrollbars(hChild, SB_HORZ))
  {
    rcClient.bottom -= IGetSystemMetrics(SM_CYHSCROLL);
#if defined(MEWEL_GUI) || defined(XWINDOWS)
    rcClient.bottom -= IGetSystemMetrics(SM_CYFRAME);
#endif
  }

  SetWindowPos(hChild, (HWND) NULL,
               rcClient.left, rcClient.top,
               RECT_WIDTH(rcClient), RECT_HEIGHT(rcClient),
               (bSwitchingZoom) ? (SWP_NOREDRAW | SWP_NOACTIVATE) : 0);
}

/*
  MdiChangeFrameCaption()
    Changes the caption of the frame window to reflect the title
    of a maximized MDI child.
*/
static VOID PASCAL MdiChangeFrameCaption(hWndClient, hChild)
  HWND hWndClient;
  HWND hChild;
{
  char szMain[128];      /* Text on non-zoomed title bar */
  char szChild[128];     /* Text for document title bar */
  char szTitle[128];     /* Text for title bar */

  /*
    Get the original application caption
  */
  GetAtomName(MdiGetProp(hWndClient, PROP_TITLE), szMain, sizeof(szMain));

  /*
    Get the child caption
  */
  GetWindowText(hChild, szChild, sizeof(szChild));

  /*
    The caption is the app name, a hyphen, and the name of the child
  */
  sprintf(szTitle, "%s - %s", szMain, szChild);

  /*
    Set the caption of the frame window.
    (Don't use SetWindowText, or we'll recurse because of the WM_SETTEXT
     processing in the FrameProc).
  */
  StdWindowWinProc(GetParent(hWndClient), WM_SETTEXT, 0, (LONG) (LPSTR) szTitle);
}

/**/

/*
 * MdiRestoreChild(hwndChild, bHidden) : void;
 *
 *    hwndChild      Handle to document
 *    bShowMove      Should we show this occurance or hide it
 *
 * Bring the specific document back from full client area display.
 * This routine places the document window where it was before the
 * maximize by remembering the position in property lists.  It also
 * restores the original title to the MDI desktop titlebar.
 *
 */
static VOID PASCAL MdiRestoreChild(HWND hwndChild, BOOL bShowMove)
{
  char        szMain[128];      /* Text on non-zoomed title bar */
  HWND        hMDIClient;         /* Handle to MDI desktop */
  LONG        lStyle;           /* Style of document window */

  if (!(GetWindowLong(hwndChild, GWL_STYLE) & (WS_MAXIMIZE | WS_MINIMIZE)))
    return;


  /* Change the menu */
  hMDIClient = GetParent(hwndChild);
  MdiRestoreMenu(hwndChild);
  DRAWMENUBAR(GetParent(hMDIClient));

  /* Reflect in the title bar of the main app */
  GetAtomName(MdiGetProp(hMDIClient, PROP_TITLE), szMain, sizeof(szMain));
  SetWindowText(GetParent(hMDIClient), szMain);

  /* Mark normal size in the window style */
  lStyle = GetWindowLong(hwndChild, GWL_STYLE);
  SetWindowLong(hwndChild, GWL_STYLE, lStyle & ~(WS_MAXIMIZE | WS_MINIMIZE));

  /* Are we HIDING the last visible MDI child */
  if (!bShowMove)
  {
    ShowWindow(hwndChild, SW_HIDE);
  }

  /* Move the window back to where it was before it zoomed */
  /* Also place in beginning of internal windows queue */
  SetWindowPos(hwndChild, (HWND) NULL,
               MdiGetProp(hwndChild, PROP_LEFT),
               MdiGetProp(hwndChild, PROP_TOP),
               MdiGetProp(hwndChild, PROP_WIDTH),
               MdiGetProp(hwndChild, PROP_HEIGHT),
               0);
}

/**/

/*
 * MdiChoose NewActiveChild(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Decide on the next document to activate when the current one is
 * hidden or closed.
 *
 */
static HWND PASCAL MdiChooseNewActiveChild(hwndChild)
  HWND hwndChild;
{
  HWND hwndIter;         /* Iterate document windows */
  HWND hwndIconic = NULLHWND;  

  /* Choose a new child window for activation */
  for (hwndIter = GetWindow(hwndChild, GW_HWNDFIRST);
       hwndIter != NULLHWND;
       hwndIter = GetWindow(hwndIter, GW_HWNDNEXT))
  {
    /*
      Get next visible, MDI child in internal windows chain
      that's not the current one.
    */
    if (MdiGetProp(hwndIter, PROP_ISMDI) && hwndIter != hwndChild &&
        IsWindowVisible(hwndIter))
    {
      /*
        Don't set the active window to an iconic window. However, keep
        track of the first iconic window encountered in case there are
        no normal windows which can be activated.
      */
      if (IsIconic(hwndIter))
      {
        if (hwndIconic == NULLHWND)
          hwndIconic = hwndIter;
      }
      else
        break;
    }
  }

  return (hwndIter != NULLHWND) ? hwndIter : hwndIconic;
}

/**/
static VOID PASCAL MdiMinimizeChild(hwndChild)
  HWND hwndChild;
{
  HWND        hwndClient;         /* Handle to MDI desktop */
  HWND        hwndNext;         /* Next active document window */

  /* One less visible MDI child */
  hwndClient = GetParent(hwndChild);

  /* 
    Show the MDI child as an icon.
    Note - InternalSysParams.hWndActive will go to the frame window.
  */
  ShowWindow(hwndChild, SW_MINIMIZE);

  /* Find us a new active MDI child */
  hwndNext = MdiChooseNewActiveChild(hwndChild);


  /* Juggle the windows around */
  if (hwndNext)
  {
    MdiDeactivateChild(hwndChild);

    /* There is another window to put up */
    if (MdiGetProp(hwndClient, PROP_ZOOM))
    {
      MdiSwitchZoom(hwndNext, hwndChild);
    }
    MdiActivateChild(hwndNext, FALSE);
    bSwitchingZoom = FALSE;
  }
  else
  {
    /* Hiding the last MDI child */
    if (MdiGetProp(hwndClient, PROP_ZOOM))
    {
      MdiSetProp(hwndClient, PROP_ZOOM, NULL);
      MdiRestoreChild(hwndChild, FALSE);
    }

    /*
      7/16/91 (maa)
        If there are no more non-iconized children, then keep the
      activation with the child which was just iconized. Or else,
      we will not be able to access any children with ALT-MINUS.
    */
#if 0
    /* Focus back to parent */
    MdiSetProp(hwndClient, PROP_ACTIVE, NULL);
    SetFocus(hwndClient);
    DRAWMENUBAR(GetParent(hwndClient));
#elif 91792

#if !32993
    /*
      3/29/93 (maa)
        If we are iconizing the only window, then we should not
      deactivate it.
    */
    MdiDeactivateChild(hwndChild);
    MdiActivateChild(hwndChild, FALSE);
#endif

#endif
  }
}


/**/
/*
 * MDI3.C - Menu Handling Routines
 *
 * LANGUAGE      : Microsoft C5.1
 * MODEL         : medium
 * ENVIRONMENT   : Microsoft Windows 2.1 SDK
 * STATUS        : operational
 *
 * This module contains all the MDI code to handle the menus.  It
 * creates as well as places and removes the document system menu from
 * the MDI desktop menu bar.  It puts the title of a recently created
 * or unhidden document on the WINDOW submenu and removes it when
 * destroyed or hidden.  It switches the WINDOW submenu from one menu
 * to another when switching between documents.  Finally it handles
 * most of the keyboard interface to the menus.
 *
 * (C) Copyright 1988
 * Eikon Systems, Inc.
 * 989 E. Hillsdale Blvd, Suite 260
 * Foster City  CA  94404
 *
 */


/* Static variables */
static FARPROC    lpMsgHook;        /* For the keyboard hooks */
static FARPROC    lpOldMsgHook;

/* Global variables */
int               iCurrentPopup = POP_NONE;  /* What menu is up */
int               iNextPopup = POP_NONE;     /* What menuis next */

/**/

/*
 * MdiZoomMenu(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Adjust the document menu to include the MDI system menu.  This
 * happens when a document is maximized.
 *
 */
static VOID PASCAL MdiZoomMenu(hwndChild)
  HWND hwndChild;
{
  HBITMAP     hBitmap;          /* Handle to system menu bitmap */
  HMENU       hmenuSystem;      /* Handle to system menu */
  HWND        hMDIClient;         /* Handle to MDI desktop */
  char        szSysMenu[4];
  HMENU       hMainMenu;

  /*
    Get the handle of the MDI client and get it's menu.
  */
  hMDIClient = GetParent(hwndChild);
  if ((hMainMenu = MdiGetProp(hMDIClient, PROP_MAINMENU)) == NULL &&
      (hMainMenu = GetMenu(GetParent(hMDIClient))) == NULL)
    return;

  if (!IsWindow(hMainMenu))
    return;

  /*
    See if the menubar has a fake system menu icon on it already. If
    so, then we have a "maximized" menubar on the frame already.
  */
  hBitmap  = MdiGetProp(hMDIClient, PROP_SYSMENU);
  if (GetMenuString(hMainMenu, 0, szSysMenu, 2, MF_BYPOSITION) > 0 &&
      szSysMenu[0] == (BYTE) hBitmap)
    return;

  /*
    Tell the menubar that it has a restore icon
  */
  WID_TO_WIN(hMainMenu)->ulStyle |= MFS_RESTOREICON;

  /*
    Insert the fake system menu icon as the first item in the menubar
  */
  szSysMenu[0] = (BYTE) hBitmap;
  szSysMenu[1] = '\0';
  hmenuSystem = MdiGetChildSysMenu();
  ChangeMenu(hMainMenu, 0, szSysMenu,
             hmenuSystem, MF_BYPOSITION | MF_INSERT | MF_POPUP);

  /*
    Place a restore icon on the right side of the menubar
  */
  szSysMenu[0] = '\\';
  szSysMenu[1] = 'a';
  szSysMenu[2] = (BYTE) WinGetSysChar(SYSCHAR_RESTOREICON);
  szSysMenu[3] = '\0';
  ChangeMenu(hMainMenu, 0, szSysMenu,
             IDM_RESTORE, MF_BYPOSITION | MF_APPEND | MF_STRING);
}

/**/

/*
 * MdiRestoreMenu(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Adjust the document menu by removing the MDI system menu.  This
 * happens when a document is restored.
 *
 */
static VOID PASCAL MdiRestoreMenu(hwndChild)
  HWND hwndChild;
{
  /* Change the menu */
  char  szSysMenu[4];
  HMENU hMenu;
  HWND  hMDIClient;

  /*
    Get the handle of the MDI menu
  */
  hMDIClient = GetParent(hwndChild);
  if ((hMenu = MdiGetProp(hMDIClient, PROP_MAINMENU)) == NULL &&
      (hMenu = GetMenu(GetParent(hMDIClient))) == NULL)
    return;

  if (!IsWindow(hMenu))
    return;

  /*
    Tell the menu not to display the restore icon anymore
  */
  WID_TO_WIN(hMenu)->ulStyle &= ~MFS_RESTOREICON;

  /*
    See if the menubar has a system menu icon in the first character.
    If it doesn't, then it doesn't have the fake system menu.
  */
  if (GetMenuString(hMenu, 0, szSysMenu, 3, MF_BYPOSITION) != 1 ||
      szSysMenu[0] != (BYTE) WinGetSysChar(SYSCHAR_MDISYSMENU))
    return;

  /*
    Get rid of the restore icon and the system menu
  */
  ChangeMenu(hMenu, GetMenuItemCount(hMenu)-1, NULL, 0, MF_BYPOSITION|MF_REMOVE);
  ChangeMenu(hMenu, 0, NULL, 0, MF_BYPOSITION | MF_REMOVE);
}

/**/

/*
 * MdiAppendWindowMenu(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Add a newly created document's title to the WINDOW submenu.  The
 * index is calculated by counting the number of items on the submenu.
 *
 */
static VOID PASCAL MdiAppendWindowToMenu(HWND hwndChild, BOOL bCheck)
{
  int         iIndex;           /* Numeric index for WINDOW menu */
  char        szCurrent[MAXTEXT];    /* Text of title */
  char        szText[MAXTEXT];       /* Text for menu */
  HMENU       hmenuWindow;      /* WINDOW menu */
  HWND        hMDIClient;       /* Handle to MDI desktop */

  /* Get important info */
  hMDIClient = GetParent(hwndChild);
  if ((hmenuWindow = MdiGetProp(hMDIClient, PROP_WINDOWMENU)) == NULL)
    return;

  /* Put in the separator */
  if (GetMenuItemCount(hmenuWindow) == (INT) MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP))
  {
    ChangeMenu(hmenuWindow, 0, NULL, 0, MF_APPEND | MF_SEPARATOR);
  }

  /* Now add us to the WINDOW menu */
  iIndex = GetMenuItemCount(hmenuWindow) - MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP);
  GetWindowText(hwndChild, szCurrent, sizeof(szCurrent));
  sprintf((char *) szText, "&%d. %s", iIndex, szCurrent);
  ChangeMenu(hmenuWindow, 0, szText, MdiGetProp(hwndChild, PROP_MENUID),
             MF_APPEND | MF_BYPOSITION | (bCheck ? MF_CHECKED : 0) | MF_STRING);
}

/**/

/*
 * MdiReinsertWindowInMenu(hwndChild) : void;
 *
 *    hwndChild      Handle to document
 *
 * Restore a hidden document's title to the WINDOW menu.  The index is
 * calculated by counting the number of items on the submenu.
 *
 */
#if 0
static VOID PASCAL MdiReinsertWindowInMenu(hwndChild)
  HWND hwndChild;
{
  char  szCurrent[MAXTEXT];    /* Text of title */
  char  szText[MAXTEXT];       /* Text for menu */
  int   iIndex;           /* Numeric index for WINDOW menu */
  HMENU hmenuWindow;      /* WINDOW menu */
  HWND  hMDIClient;         /* Handle to MDI desktop */

  /* Get important info */
  hMDIClient = GetParent(hwndChild);
  hmenuWindow = MdiGetProp(hMDIClient, PROP_WINDOWMENU);

  /* Put in the separator */
  if (GetMenuItemCount(hmenuWindow) == (INT) MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP))
  {
    ChangeMenu(hmenuWindow, 0, NULL, 0, MF_APPEND | MF_SEPARATOR);
  }

  /* Prepare window string */
  iIndex = GetMenuItemCount(hmenuWindow) - MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP);
  GetWindowText(hwndChild, szCurrent, sizeof(szCurrent));
  szCurrent[sizeof(szCurrent) - 1] = '\0';
  sprintf((char *) szText, "&%d. %s", iIndex, szCurrent);

  /* Append us to the bottom */
  ChangeMenu(hmenuWindow, 0, szText, MdiGetProp(hwndChild, PROP_MENUID),
             MF_APPEND | MF_CHECKED | MF_STRING);

  /* All done */
  DRAWMENUBAR(GetParent(hMDIClient));
}
#endif
/**/

/*
 * MdiRemoveWindowFromMenu(hwndChild, bWindowDying) : void;
 *
 *    hwndChild      Handle to document
 *    bWindowDying   Is the window being destroyed
 *
 * Remove the title of a about-to-be-hidden/destroyed document from
 * the WINDOW submenu.  If the document's title wasn't the last item on
 * the submenu then it updates the index for the documents that follow.
 *
 */
static VOID PASCAL MdiRemoveWindowFromMenu(HWND hwndChild, BOOL bWindowDying)
{
  char  szCurrent[MAXTEXT]; /* Text of title */
  char  szText[MAXTEXT];  /* Text for menu */
  int   iIndex;           /* Numeric index for WINDOW menu */
  HMENU hmenuWindow;      /* WINDOW menu */
  HWND  hMDIClient;       /* Handle to MDI desktop */
  UINT  wOldPropID;

  (void) bWindowDying;

  /* Get important info */
  hMDIClient = GetParent(hwndChild);
  hmenuWindow = MdiGetProp(hMDIClient, PROP_WINDOWMENU);
  wOldPropID = MdiGetProp(hwndChild, PROP_MENUID);

  /* Calculate our index */
  for (iIndex = MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP);
       iIndex < GetMenuItemCount(hmenuWindow);
       iIndex++)
  {
    if (GetMenuItemID(hmenuWindow, iIndex) == wOldPropID)
      break;
  }

  /* Delete us from everyone's menu */
  ChangeMenu(hmenuWindow, MdiGetProp(hwndChild, PROP_MENUID), NULL, NULL,
             MF_BYCOMMAND | MF_DELETE);

  /* Adjust the rest of the window */
  if (GetMenuItemCount(hmenuWindow) == (INT) MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP)+1)
  {
    /* Remove separator */
    ChangeMenu(hmenuWindow, MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP), NULL, 0,
               MF_DELETE | MF_BYPOSITION);
  }
  else
  {
    /* Shuffle menus down */
    for ( ;  iIndex < GetMenuItemCount(hmenuWindow);  iIndex++)
    {
      GetMenuString(hmenuWindow, iIndex, szCurrent, sizeof(szCurrent),
                    MF_BYPOSITION);
      sprintf((char *) szText,"&%d. %s", iIndex - MdiGetProp(hMDIClient,PROP_ITEMSBEFORESEP),
                                strchr((char *) szCurrent, ' ') + 1);
      ChangeMenu(hmenuWindow, iIndex, szText, GetMenuItemID(hmenuWindow,iIndex),
                 MF_BYPOSITION | MF_CHANGE | MF_STRING | MF_UNCHECKED);
    }
  }
}

/**/
/*
 * MdiGetChildSysMenu() : HMENU;
 *
 *
 * Create a document system menu.  This menu is only used when the
 * document is maximized.  Otherwise the default system menu is used
 * and the text modified at WM_INITMENU.
 *
 */
static HMENU PASCAL MdiGetChildSysMenu(void)
{
  static HMENU hmenuSystem = 0;      /* Handle to system menu */

  /*
    We keep a single static handle to a fake system menu. If it has
    already been created, then return it.
    Notice that this system menu will never be destroyed, because the
    very last call to ChangeMenu in MdiRestoreMenu uses the MF_REMOVE
    flag, not the MF_DELETE flag. If MF_DELETE was used, then the system
    menu would be destroyed.
  */
  if (hmenuSystem)
    return hmenuSystem;

  WinGetIntlMenuStrings();

  /*
    Create the system menu.

    Note : we cannot use the WInCreateSysMenu call because we are using
    different menu id's than the standard system menu uses. For example,
    for the Close menu, we use IDM_CLOSE, not SC_CLOSE.
  */
  hmenuSystem = CreateMenu();

  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_RESTORE],  IDM_RESTORE,
             MF_APPEND | MF_STRING);
  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_MOVE],     IDM_MOVE,
             MF_APPEND | MF_GRAYED | MF_STRING);
  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_SIZE],     IDM_SIZE,
             MF_APPEND | MF_GRAYED | MF_STRING);
  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_MINIMIZE], IDM_MINIMIZE,
             MF_APPEND |             MF_STRING);
  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_MAXIMIZE], IDM_MAXIMIZE,
             MF_APPEND | MF_GRAYED | MF_STRING);
  ChangeMenu(hmenuSystem, 0, NULL,           0,
             MF_APPEND | MF_SEPARATOR);
  ChangeMenu(hmenuSystem, 0, SysStrings[SYSSTR_CLOSEMDI], IDM_CLOSE,
             MF_APPEND | MF_STRING);
  return hmenuSystem;
}

/**/

/*
 * MdiCreateChildSysBitmap(hMDIClient) : HBITMAP;
 *
 *    hMDIClient       Handle to MDI desktop
 *
 * Create a bitmap for the document system menu (the bitmap that looks
 * like a '-'.  The system already has one, so we use it; however there
 * are two parts to the system bitmap:  one half for parent system menu
 * bitmaps and the other half for the child system menu bitmap.  This
 * routine makes a memory DC for the two part bitmap, another memory DC
 * to receive the second (and smaller) half, creates a half sized
 * bitmap, and bitblt()s the smaller bitmap into the half sized bitmap
 * of ours.
 *
 */
static HBITMAP PASCAL MdiCreateChildSysBitmap(hMDIClient)
  HWND hMDIClient;
{
  (void) hMDIClient;
  return (HBITMAP) WinGetSysChar(SYSCHAR_MDISYSMENU);
}

/**/

/*
 * MdiSetMenuKeyhook(hInst) : void;
 *
 *    hInst          Current instance handle
 *
 * Install our keyboard hook.  This allows document system menus
 * to be part of the keyboard interface to menus.
 *
 */
static VOID PASCAL MdiSetMenuKeyHook(hInst)
  HANDLE hInst;
{
  (void) hInst;
  lpMsgHook = MakeProcInstance((FARPROC) MdiMsgHook, hInst);
#ifndef MEWEL
  lpOldMsgHook = SetWindowsHook(WH_MSGFILTER, lpMsgHook);
#endif
}

/**/

/*
 * MdiMenuMessageLoopUpdate(hMDIClient) : void;
 *
 *    hMDIClient       Handle to MDI desktop
 *
 * Activate the correct popup menu if the right or left arrow key
 * was hit and the document's system menu is involved.  Notice that
 * instead of doing this when the actual event occured, we wait until
 * DispatchMessage() is done.  This way the currently popped up menu
 * closes down in an orderly fashion before the next popup menu is
 * popped up.
 *
 */
#if 0
static VOID PASCAL MdiMenuMessageLoopUpdate(hMDIClient)
  HWND hMDIClient;
{
  int  iOldNext;         /* Preserve current state info */
  HWND hwndActive;       /* Current document window */

  /* With no messages, we're assured that no menu is popped up */
  iCurrentPopup = POP_NONE;

  if (iNextPopup != POP_NONE)
  {
    /*
     * This sequence is used because the SendMessage() call
     * could generate reentrancy problems.
     */
    iOldNext = iNextPopup;
    iNextPopup = POP_NONE;

    /* Hilite the proper popup */
    hwndActive = MdiGetProp(hMDIClient, PROP_ACTIVE);
    switch(iOldNext)
    {
      case POP_MAINSYS:
        SendMessage(hMDIClient, WM_SYSCOMMAND, SC_KEYMENU, (DWORD) ' ');
        break;

      case POP_MAIN1ST:
        SendMessage(hMDIClient, WM_SYSCOMMAND, SC_KEYMENU, (DWORD) 'F');
        break;

      case POP_CHILDSYS:
        if (hwndActive)
          SendMessage(hwndActive, WM_SYSCOMMAND, SC_KEYMENU, (DWORD) '-');
        break;
    }
  }
}
#endif
/**/

/*
 * MdiMsgHook(iContext, wCode, lParam) : LONG;
 *
 *    iContext       What is the key message's destination
 *    wCode          Will this key result in an action
 *    lParm          Pointer to the key message
 *
 * Watch for left and right arrows when a menu is hilited.  This allows
 * these keys to move to the document's system menu between the MDI
 * desktop's system menu and the FILE menu.  Notice that we only set
 * variables here, we don't perform the actions.
 *
 */
static LONG FAR PASCAL MdiMsgHook(iContext, wCode, lParam)
  int  iContext;
  WPARAM wCode;
  LPARAM lParam;
{
  (void) iContext;  (void) wCode;  (void) lParam;
  return 0L;
}

/**/

/*
 * MdiFreeMenuKeyhook(void) : void
 *
 * Free the procedure instance required for the keyboard hook.
 *
 */
static VOID PASCAL MdiFreeMenuKeyHook(void)
{
  FreeProcInstance(lpMsgHook);
}



UINT FAR PASCAL MdiGetProp(hWnd, iProp)
  HWND hWnd;
  UINT iProp;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  PSTR   pClassExtra;

  if (w == (WINDOW *) NULL)
    return 0;

  if ((pClassExtra = w->pMDIInfo) == NULL)
    return 0;

  switch (iProp)
  {
    /* Window data for the MDI parent */
    case  PROP_ACTIVE           :
      return ((PMDICLIENTINFO) pClassExtra)->bActive;
    case  PROP_COUNT            :
      return ((PMDICLIENTINFO) pClassExtra)->iChildCount;
    case  PROP_ZOOM             :
      return ((PMDICLIENTINFO) pClassExtra)->bZoomed;
    case  PROP_MAINMENU         :
      return ((PMDICLIENTINFO) pClassExtra)->hMainMenu;
    case  PROP_WINDOWMENU       :
      return ((PMDICLIENTINFO) pClassExtra)->hWindowPopup;
    case  PROP_SYSMENU          :
      return ((PMDICLIENTINFO) pClassExtra)->hSysMenu;
    case  PROP_TITLE            :
      return ((PMDICLIENTINFO) pClassExtra)->haTitle;
    case  PROP_FIRSTCHILDID     :
      return ((PMDICLIENTINFO) pClassExtra)->iFirstChildID;
    case  PROP_ITEMSBEFORESEP   :
      return ((PMDICLIENTINFO) pClassExtra)->iItemsBeforeSep;

    /* Window data for MDI children */
    case  PROP_LEFT             :
      return ((PMDICHILDINFO) pClassExtra)->ptOrigin.x;
    case  PROP_TOP              :
      return ((PMDICHILDINFO) pClassExtra)->ptOrigin.y;
    case  PROP_WIDTH            :
      return ((PMDICHILDINFO) pClassExtra)->extWindow.cx;
    case  PROP_HEIGHT           :
      return ((PMDICHILDINFO) pClassExtra)->extWindow.cy;
    case  PROP_ACCEL            :
      return ((PMDICHILDINFO) pClassExtra)->hAccel;
    case  PROP_MENUID           :
      return ((PMDICHILDINFO) pClassExtra)->iMenuID;
    case  PROP_ISMDI            :
      return (UINT) (IS_MDIDOC(w) != 0L);
    case  PROP_ICONIZED         :
      return ((PMDICHILDINFO) pClassExtra)->bIconized;
    default                     :
      return 0;
  }
}


UINT FAR PASCAL MdiSetProp(hWnd, iProp, iVal)
  HWND hWnd;
  UINT iProp;
  UINT iVal;
{
  WINDOW *w = WID_TO_WIN(hWnd);
  PSTR   pClassExtra;
  UINT   iOldVal;

  if (w == (WINDOW *) NULL)
    return 0;

  if ((pClassExtra = w->pMDIInfo) == NULL)
  {
    MessageBox(NULLHWND, "MdiSetProp - bad extra", NULL, MB_OK);
    return 0;
  }

  switch (iProp)
  {
    /* Window data for the MDI parent */
    case  PROP_ACTIVE           :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->bActive;
      ((PMDICLIENTINFO) pClassExtra)->bActive = iVal;
      return iOldVal;
    case  PROP_COUNT            :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->iChildCount;
      ((PMDICLIENTINFO) pClassExtra)->iChildCount = iVal;
      return iOldVal;
    case  PROP_ZOOM             :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->bZoomed;
      ((PMDICLIENTINFO) pClassExtra)->bZoomed = iVal;
      return iOldVal;
    case  PROP_MAINMENU         :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->hMainMenu;
      ((PMDICLIENTINFO) pClassExtra)->hMainMenu = iVal;
      return iOldVal;
    case  PROP_WINDOWMENU       :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->hWindowPopup;
      ((PMDICLIENTINFO) pClassExtra)->hWindowPopup = iVal;
      return iOldVal;
    case  PROP_SYSMENU          :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->hSysMenu;
      ((PMDICLIENTINFO) pClassExtra)->hSysMenu = iVal;
      return iOldVal;
    case  PROP_TITLE            :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->haTitle;
      ((PMDICLIENTINFO) pClassExtra)->haTitle = iVal;
      return iOldVal;
    case  PROP_FIRSTCHILDID     :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->iFirstChildID;
      ((PMDICLIENTINFO) pClassExtra)->iFirstChildID = iVal;
      return iOldVal;
    case  PROP_ITEMSBEFORESEP   :
      iOldVal = ((PMDICLIENTINFO) pClassExtra)->iItemsBeforeSep;
      ((PMDICLIENTINFO) pClassExtra)->iItemsBeforeSep = iVal;
      return iOldVal;

    /* Window data for MDI children */
    case  PROP_LEFT             :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->ptOrigin.x;
      ((PMDICHILDINFO) pClassExtra)->ptOrigin.x = iVal;
      return iOldVal;
    case  PROP_TOP              :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->ptOrigin.y;
      ((PMDICHILDINFO) pClassExtra)->ptOrigin.y = iVal;
      return iOldVal;
    case  PROP_WIDTH            :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->extWindow.cx;
      ((PMDICHILDINFO) pClassExtra)->extWindow.cx = iVal;
      return iOldVal;
    case  PROP_HEIGHT           :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->extWindow.cy;
      ((PMDICHILDINFO) pClassExtra)->extWindow.cy = iVal;
      return iOldVal;
    case  PROP_ACCEL            :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->hAccel;
      ((PMDICHILDINFO) pClassExtra)->hAccel = iVal;
      return iOldVal;
    case  PROP_MENUID           :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->iMenuID;
      ((PMDICHILDINFO) pClassExtra)->iMenuID = iVal;
      return iOldVal;
    case  PROP_ISMDI            :
      iOldVal = (UINT) (IS_MDIDOC(w) != 0L);
      if (iVal)
        w->ulStyle |= WIN_IS_MDIDOC;
      else
        w->ulStyle &= ~WIN_IS_MDIDOC;
      return iOldVal;
    case  PROP_ICONIZED         :
      iOldVal = ((PMDICHILDINFO) pClassExtra)->bIconized;
      ((PMDICHILDINFO) pClassExtra)->bIconized = iVal;
      return iOldVal;
    default                     :
      return 0;
  }
}



/*==========================================================================*/
/*  Logfile:    arrange.c                                                   */
/*  Desc:       Arranges the child windows owned by a specified window.     */
/*  Log:                                                                    */
/*  Sun 8 Jul 1990   DavidHollifield                                        */
/*      Create.                                                             */
/* =========================================================================*/

/*==========================================================================
                                 Definitions
==========================================================================*/
#define CASC_EDGE_NUM   2               /* Cascaded arrange edge */
#define CASC_EDGE_DENOM 3               /* Cascaded arrange edge */
#define ICON_PARK_NUM   5               /* Icon park */
#define ICON_PARK_DENOM 3               /* Icon park */

/*==========================================================================*/
/*                                Structures                                */
/*==========================================================================*/
typedef struct tagMultWindowPos
{
  UINT   fs;
  INT    cy;
  INT    cx;
  INT    y;
  INT    x;
  HWND   hwndInsertBehind;
  HWND   hwnd;
} MultWindowPos;
typedef MultWindowPos *PMultWindowPos;

static VOID NEAR PASCAL MDIDocumentArrangeTiled(PRECT, INT, PMultWindowPos, WPARAM);
static VOID NEAR PASCAL MDIDocumentArrangeCascaded(PRECT, INT, PMultWindowPos);
static VOID NEAR PASCAL WinSetMultWindowPos(PMultWindowPos, INT);

/*==========================================================================*/
/*                                                                          */
/*                            MDIDocumentArrange                            */
/*                                                                          */
/*  Usage:      MDIDocumentArrange (hwndMDIDesktop, usArrangeStyle);        */
/*                                                                          */
/*  Input:      hwndMDIDesktop => MDI Desktop window handle                 */
/*              usArrangeStyle => Arrangement style                         */
/*                                                                          */
/*  Output:     None                                                        */
/*                                                                          */
/*  Desc:       Arrange all documents in the MDI desktop                    */
/*                                                                          */
/*  Modifies:   None                                                        */
/*                                                                          */
/*  Notes:      None                                                        */
/*                                                                          */
/*==========================================================================*/
static VOID FAR PASCAL MDIDocumentArrange(hwndMDIDesktop, usArrangeStyle, wParam)
  HWND hwndMDIDesktop;
  UINT usArrangeStyle;                    /* AWP_CASCADED or AWP_TILED */
  WPARAM wParam;                          /* method of tiling */
{
  HWND    hwnd;                           /* Window work area */
  PMultWindowPos    pswpIcon;             /* Ptr to Icon positions */
  PMultWindowPos    pswpWnd;              /* Ptr to Window positions */
  RECT    rect;                           /* Rectangle work area */
  INT     i;                              /* Counter */
  INT     nDocs;                          /* MDI Document count */
  INT     nIcons;                         /* Icon count */
  INT     nWindows;                       /* Window count */
  INT     nIconsPerColumn;

  /*
    Count the number of MDI document windows
  */
  nDocs = 0;
  for (hwnd = GetTopWindow(hwndMDIDesktop);
       hwnd;
       hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    if (MdiGetProp(hwnd, PROP_ISMDI))
      nDocs++;

  if (nDocs == 0)
    return;

  /*
    Allocate space for document and icon positions
  */
  pswpWnd = (PMultWindowPos) emalloc(sizeof(MultWindowPos) * nDocs);
  pswpIcon =(PMultWindowPos) emalloc(sizeof(MultWindowPos) * nDocs);
  if (pswpWnd == NULL || pswpIcon == NULL)
    return;

  /* 
    Traverse the child window list and selectively add the to the arrange lists
  */
  nWindows = nIcons = 0;
  for (hwnd = GetTopWindow(hwndMDIDesktop);
       hwnd;
       hwnd = GetWindow(hwnd, GW_HWNDNEXT))
  {
    /* Make sure the window is a visible MDI window */
    if (!MdiGetProp(hwnd, PROP_ISMDI))
      continue;
    if (!IsWindowVisible(hwnd))
      continue;

    /*
      Skip disabled windows if the MDITILE_SKIPDISABLED flag is set
    */
    if ((wParam & MDITILE_SKIPDISABLED) && !IsWindowEnabled(hwnd))
      continue;


    /* Count icons */
    if (IsIconic(hwnd))
    {
      pswpIcon[nIcons++].hwnd = hwnd;
    }
    else
    {
      /* Count windows (restore any that are maximized) */
      if (IsZoomed(hwnd))
        SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0L);
#ifdef MEWEL
      /*
        Defer processing the active MDI window until the loop ends. This
        way, the active window can appear on top in a cascaded arrangement.
      */
      else if (hwnd != MDIGETACTIVEWINDOW())
#endif
      pswpWnd[nWindows++].hwnd = hwnd;
    }
  } /* end for() */


#ifdef MEWEL
  /*
    Make sure that the active window is the last window in the cascade
  */
  if (!IsIconic(MDIGETACTIVEWINDOW()))
    if ((pswpWnd[nWindows].hwnd = MDIGETACTIVEWINDOW()) != NULLHWND)
      nWindows++;
#endif

  /*
    Get dimensions of desktop window
  */
  GetClientRect(hwndMDIDesktop, (LPRECT) &rect);

#if defined(MEWEL) && defined(MOTIF)
  /*
    We need to compensate for the size of the window manager decorations.
  */
/*
  rect.bottom -= (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
  rect.right  -= (MWM_SHELL_CXFRAME * 2);
*/
#endif

  if (nIcons > 0)
  {
    /*
      Set the icon positions.
    */
    for (i = 0; i < nIcons; i++)
    {
      PMultWindowPos ppswp = &pswpIcon[i];
      POINT pt;

      _GetIconXY(i, ppswp->hwnd, &ppswp->x, &ppswp->y, &ppswp->cx, &ppswp->cy);

      /*
        GetIconXY returns x and y in screen coordinates, but 
        SetWindowPos() will need then in client coordinates.
      */
      pt.x = ppswp->x;
      pt.y = ppswp->y;
      ScreenToClient(hwndMDIDesktop, &pt);
      ppswp->x = pt.x;
      ppswp->y = pt.y;

      ppswp->fs = SWP_SHOWWINDOW;
      ppswp->hwndInsertBehind = (HWND) 1;
    }

#ifdef MEWEL
    /* Make room for the columns of icons */
    nIconsPerColumn = RECT_HEIGHT(rect) / IGetSystemMetrics(SM_CYICONSPACING);
    rect.right -=
        (nIcons / nIconsPerColumn + 1) * IGetSystemMetrics(SM_CYICONSPACING) +
        CX_BETWEEN_ICONS;
#else
    /* Make room for a single row of icons */
    yIcon = IGetSystemMetrics(SM_CYICON);
    rect.bottom -= (yIcon * ICON_PARK_NUM) / ICON_PARK_DENOM;
#endif
  }

  /*
    Manipulate the array of MultWindowPos structures based upon arrange style
  */
  if (usArrangeStyle == AWP_CASCADED)
    MDIDocumentArrangeCascaded(&rect, nWindows, pswpWnd);
  else if (usArrangeStyle == AWP_TILED)
    MDIDocumentArrangeTiled(&rect, nWindows, pswpWnd, wParam);

  /*
    Reposition all windows and icons
  */
  if (nWindows > 0)
    WinSetMultWindowPos(pswpWnd, nWindows);
  if (nIcons > 0)
    WinSetMultWindowPos(pswpIcon, nIcons);

  /*
    Free space used for document and icon positions
  */
  MyFree(pswpWnd);
  MyFree(pswpIcon);
}


/*===========================================================================*/
/*                                                                           */
/*                        MDIDocumentArrangeCascaded                         */
/*                                                                           */
/*  Usage:      MDIDocumentArrangeCascaded (prect, nWindows, pswp);          */
/*                                                                           */
/*  Input:      prect    => Ptr to rectangle within which the arrange will   */
/*                          occur.                                           */
/*              nWindows => Number of windows to arrange.                    */
/*              pswp     => Ptr to array of window positions.                */
/*                                                                           */
/*  Output:     None                                                         */
/*                                                                           */
/*  Desc:       Arrange all MDI documents cascaded.  This functions modifies */
/*              the MultWindowPos structures to contain the necessary coordinates to   */
/*              achieve a full cascade within the rectangle specified by     */
/*              prect.                                                       */
/*                                                                           */
/*  Modifies:   pswp                                                         */
/*                                                                           */
/*  Notes:      None                                                         */
/*                                                                           */
/*===========================================================================*/
static VOID NEAR PASCAL MDIDocumentArrangeCascaded(prect, nWindows, pswp)
  PRECT prect;
  INT  nWindows;
  PMultWindowPos pswp;
{
  RECT  rect;
  INT   xEdge, yEdge;
  INT   xDelta, yDelta;
  INT   nCascade;
  INT   nMaxWindows;
  INT   nPass;
  INT   nMaxPasses;
  INT   x, y, i, j;
  INT   iWidth, iHeight;

  if (nWindows == 0)
    return;

  /* Set cascade parameters */
  rect = *prect;

  /* Get x and y deltas from system values */
#if defined(MEWEL_GUI) || defined(XWINDOWS)
  xDelta = IGetSystemMetrics(SM_CXFRAME) * 2;
#else
  xDelta = 4;
#endif
  yDelta = IGetSystemMetrics(SM_CYCAPTION) * 2;

  /*
    Pre-calc the height and width of the enclosing rectangle.
  */
  iWidth  = RECT_WIDTH(rect);
  iHeight = RECT_HEIGHT(rect);

  /* Get initial cut at yEdge using fraction */
  yEdge = (iHeight * CASC_EDGE_NUM) / CASC_EDGE_DENOM;

  /* Determine maximum number of deltas used per run */
  nMaxWindows = (iHeight - yEdge) / yDelta;

  /* 
    Set x and y edges so full cascade will fill rectangle completely
  */
  xEdge = iWidth - xDelta/2 - (nMaxWindows * xDelta);
  yEdge = iHeight - nMaxWindows * yDelta;
  nMaxWindows++;

  nMaxPasses = ((nWindows - 1) / nMaxWindows) + 1;

  for (nPass=0, j=0;  nPass < nMaxPasses;  nPass++)
  {
    x = rect.left;
    y = rect.top;

    nCascade = min(nWindows, nMaxWindows);
    if (nWindows > nMaxWindows)
      nWindows -= nMaxWindows;

    /*
      Go through one layer of cascades, from top to bottom. Each window
      is offset from the previous in the x direction by xDelta (2 times
      the thickframe width) and in the y direction by yDelta (2 times 
      the caption height).
    */
    for (i = 0;  i < nCascade;  i++)
    {
      pswp[j].x   = x;
      pswp[j].y   = y;
      pswp[j].cx  = xEdge;
      pswp[j].cy  = yEdge;
      pswp[j].fs  = SWP_SHOWWINDOW;
      x += xDelta;
      y += yDelta;
      j++;
    }
  }
}


/*===========================================================================*/
/*                         MDIDocumentArrangeTiled                           */
/*  Usage:      MDIDocumentArrangeTiled (prect, nWindows, pswp);             */
/*  Input:      prect    => Ptr to rectangle within which the arrange will   */
/*                          occur.                                           */
/*              nWindows => Number of window to arrange.                     */
/*              pswp     => Ptr to array of window positions.                */
/*  Output:     None                                                         */
/*  Desc:       Arrange all MDI documents tiled.  This functions modifies    */
/*              the MultWindowPos structures to contain the necessary coordinates to   */
/*              achieve a full tile within the rectangle specified by prect. */
/*  Modifies:   pswp                                                         */
/*  Notes:      None                                                         */
/*===========================================================================*/
static VOID NEAR PASCAL MDIDocumentArrangeTiled(prect, nWindows, pswp, wParam)
  PRECT prect;
  INT   nWindows;
  PMultWindowPos  pswp;
  UINT  wParam;  /* MDITILE_HORIZONTAL(1), MDITILE_VERTICAL(0), 
                    MDITILE_SKIPDISABLED(2) */
{
  INT cDiff;
  INT cExtras;
  INT iColWithExtraRow;
  INT iRow, iCol;
  INT x, y, cx, cy;
  INT usRoot;
  INT iWidth, iHeight;
  INT idx;

#if defined(MOTIF)
  BOOL bUseOwnNC = (BOOL) ((XSysParams.ulOptions & XOPT_USEMEWELSHELL) != 0);
#endif


  if (nWindows == 0)
    return;

  /*
    Pre-calc the height and width of the enclosing rectangle
  */
  iHeight = prect->bottom - prect->top;
  iWidth  = prect->right  - prect->left;

  /*
    Get grid dimensions
  */
  for (usRoot = 0;  (usRoot * usRoot) < nWindows;  usRoot++)
    ;
  cExtras = usRoot * usRoot - nWindows;

  /*
    Find column where number of rows increases and find initial
    difference of rows versus columns
  */
  if (cExtras >= usRoot)
  {
    iColWithExtraRow = cExtras - usRoot;
    cDiff = 2;
  }
  else
  {
    iColWithExtraRow = cExtras;
    cDiff = 1;
  }


  if (wParam & MDITILE_HORIZONTAL)
  {
    /*
      TODO... implement horizontal tiling
    */
#define USE_HORZTILING
#ifdef USE_HORZTILING
    /*
      Assign x coordinates
    */
    y  = prect->top;
    cy = 0;

    for (iRow = 0;  iRow < usRoot;  iRow++)
    {
      y += cy;
      cy = prect->top  + (iHeight * (iRow+1)) / usRoot - y;
#if defined(MOTIF)
      if (!bUseOwnNC)
        cy -= (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif

      for (iCol = 0;  iCol < usRoot - cDiff;  iCol++)
      {
        idx = iCol * usRoot + iRow;
        pswp[idx].y  = y;
        pswp[idx].cy = cy;
        pswp[idx].fs = SWP_SHOWWINDOW;
      }

      if (iRow >= iColWithExtraRow)
      {
        idx = iCol * usRoot + iRow - iColWithExtraRow;
        pswp[idx].y  = y;
        pswp[idx].cy = cy;
        pswp[idx].fs = SWP_SHOWWINDOW;
      }

#if defined(MOTIF)
      if (!bUseOwnNC)
        cy += (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif
    }

    /*
      Assign x coordinates, for columns WITHOUT the extra row
    */
    x  = prect->left;
    cx = 0;

    for (iCol = usRoot - cDiff - 1;  iCol >= 0;  iCol--)
    {
      x += cx;
      cx = prect->left + (iWidth * (usRoot-cDiff-iCol)) / (usRoot-cDiff) - x;
#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx -= (2*MWM_SHELL_CXFRAME);
#endif

      for (iRow = 0;  iRow < iColWithExtraRow;  iRow++)
      {
        idx = iCol * usRoot + iRow;
        pswp[idx].x = x;
        pswp[idx].cx = cx;
      }

#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx += (2*MWM_SHELL_CXFRAME);
#endif
    }

    /* 
      Assign x coordinates, for columns WITH the extra row.
      Do last row first (different offsets).
    */
    x  = prect->left;
    cx = iWidth  / (usRoot - cDiff + 1);
#if defined(MEWEL) && defined(MOTIF)
    if (!bUseOwnNC)
      cx -= (2*MWM_SHELL_CXFRAME);
#endif

    for (iRow = iColWithExtraRow;  iRow < usRoot;  iRow++)
    {
      idx = usRoot * (usRoot - cDiff) + iRow - iColWithExtraRow;
      pswp[idx].x  = x;
      pswp[idx].cx = cx;
    }

    for (iCol = usRoot - cDiff - 1; iCol >= 0; iCol--)
    {
      x += cx;
      cx = prect->left + (iWidth * (usRoot - cDiff - iCol + 1)) /
                                             (usRoot - cDiff + 1) - x;
#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx -= (2*MWM_SHELL_CXFRAME);
#endif

      for (iRow = iColWithExtraRow;  iRow < usRoot;  iRow++)
      {
        idx = iCol * usRoot + iRow;
        pswp[idx].x  = x;
        pswp[idx].cx = cx;
      }
#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx += (2*MWM_SHELL_CXFRAME);
#endif
    }
#endif /* USE_HORZTILING */
  }

  else  /* MDITILE_VERTICAL */
  {
    /*
      Assign x coordinates
    */
    x  = prect->left;
    cx = 0;

    for (iCol = 0;  iCol < usRoot;  iCol++)
    {
      x += cx;
      cx = prect->left + (iWidth * (iCol+1)) / usRoot - x;
#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx -= (2*MWM_SHELL_CXFRAME);
#endif

      for (iRow = 0;  iRow < usRoot - cDiff;  iRow++)
      {
        idx = iRow * usRoot + iCol;
        pswp[idx].x  = x;
        pswp[idx].cx = cx;
        pswp[idx].fs = SWP_SHOWWINDOW;
      }

      if (iCol >= iColWithExtraRow)
      {
        idx = iRow * usRoot + iCol - iColWithExtraRow;
        pswp[idx].x  = x;
        pswp[idx].cx = cx;
        pswp[idx].fs = SWP_SHOWWINDOW;
      }
#if defined(MEWEL) && defined(MOTIF)
      if (!bUseOwnNC)
        cx += (2*MWM_SHELL_CXFRAME);
#endif
    }

    /*
      Assign y coordinates, for columns WITHOUT the extra row
    */
    y  = prect->top;
    cy = 0;

    for (iRow = usRoot - cDiff - 1;  iRow >= 0;  iRow--)
    {
      y += cy;
      cy = prect->top + (iHeight * (usRoot-cDiff-iRow)) / (usRoot-cDiff) - y;
#if defined(MOTIF)
      if (!bUseOwnNC)
        cy -= (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif

      for (iCol = 0;  iCol < iColWithExtraRow;  iCol++)
      {
        idx = iRow * usRoot + iCol;
        pswp[idx].y = y;
        pswp[idx].cy = cy;
      }
#if defined(MOTIF)
      if (!bUseOwnNC)
        cy += (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif
    }

    /* 
      Assign y coordinates, for columns WITH the extra row.
      Do last row first (different offsets).
    */
    y  = prect->top;
    cy = iHeight / (usRoot - cDiff + 1);
#if defined(MOTIF)
    if (!bUseOwnNC)
      cy -= (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif

    for (iCol = iColWithExtraRow;  iCol < usRoot;  iCol++)
    {
      idx = usRoot * (usRoot - cDiff) + iCol - iColWithExtraRow;
      pswp[idx].y  = y;
      pswp[idx].cy = cy;
    }

    for (iRow = usRoot - cDiff - 1; iRow >= 0; iRow--)
    {
      y += cy;
      cy = prect->top + (iHeight * (usRoot - cDiff - iRow + 1)) /
                                             (usRoot - cDiff + 1) - y;
#if defined(MOTIF)
      if (!bUseOwnNC)
        cy -= (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif

      for (iCol = iColWithExtraRow;  iCol < usRoot;  iCol++)
      {
        idx = iRow * usRoot + iCol;
        pswp[idx].y  = y;
        pswp[idx].cy = cy;
      }
#if defined(MOTIF)
      if (!bUseOwnNC)
        cy += (MWM_SHELL_CYCAPTION + 2*MWM_SHELL_CYFRAME);
#endif
    }

  } /* MDITILE_HORIZONTAL */
}



/*==========================================================================
                             WinSetMultWindowPos
    Usage:      WinSetMultWindows (pswp, nWindows);
    Input:      pswp => Ptr to array of MultWindowPos structures
                nWindows => Number of MultWindowPos structures
    Output:     None
    Desc:       Calls SetWindowPos for each entry in the MultWindowPos array.
    Modifies:   None
    Notes:      None
==========================================================================*/
static VOID NEAR PASCAL WinSetMultWindowPos(pswp, nWindows)
  PMultWindowPos pswp;
  INT  nWindows;
{
  register INT i;

  for (i = 0;  i < nWindows;  i++)
  {
    PMultWindowPos ppswp = &pswp[i];
    SetWindowPos(ppswp->hwnd, ppswp->hwndInsertBehind, 
                 ppswp->x, ppswp->y, ppswp->cx, ppswp->cy,
                 ppswp->fs | SWP_NOACTIVATE);
  }
}

