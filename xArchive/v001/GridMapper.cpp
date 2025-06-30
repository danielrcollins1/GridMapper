/*
=====================================================================
	GRIDMAPPER.CPP 
    Application entry point and Windows-UI stuff.
	Copyright (c) 2010 Daniel R. Collins. All rights reserved.
	See the bottom of this file for any licensing information.
=====================================================================
*/

#include "stdafx.h"
#include "resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								        // Current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];		// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About (HWND, UINT, WPARAM, LPARAM);

// Global variables (DRC)
HPEN BkgdPen;
int selectedFeature;
GridMap *gridmap = NULL;
bool LButtonDown = false;
const int DefaultMapWidth = 40;
const int DefaultMapHeight = 30;
char cmdLine[GRID_FILENAME_MAX] = "\0";

// Forward declarations (DRC)
int  GetGridSize();
bool OkDiscardChanges(HWND hWnd);
void UpdateEntireWindow (HWND hWnd);
void SetSelectedFeature (HWND hWnd, int feature);
void MyPaintWindow (HWND hWnd);
void MyLButtonHandler (HWND hWnd, LPARAM lParam);
void FloorSelect (HWND hWnd, int xPos, int yPos);
void WallSelect (HWND hWnd, int xPos, int yPos);
void ChangeWestWall(HWND hWnd, int x, int y, int newFeature);
void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature);
void FillOpenFileStruct (HWND hWnd, OPENFILENAME *info, 
                         CHAR *nameStr, int nameSize);
int  NewMapFromSpecs (HWND hWnd, int newWidth, int newHeight);
int  NewMapFromFile (HWND hWnd, char *filename);
void SetNewMap(HWND hWnd, GridMap *newmap);
void CopyMap (HWND hWnd);
void OpenMap (HWND hWnd);
void SaveMap (HWND hWnd);
void SaveMapAs (HWND hWnd);
void PrintMap (HWND hWnd);
void CreatePrintJob (PRINTDLG pd, HDC srcDC);

LRESULT CALLBACK	NewDialog (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GridSizeDialog (HWND, UINT, WPARAM, LPARAM);


//
//   FUNCTION: WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
//
//   PURPOSE: Win32 application entry point
//
//   COMMENTS:
//
//        Initialize strings, instance, accelerators.
//        Run main message loop here.
//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
  MSG msg;
	HACCEL hAccelTable;

  // Copy command line w/o quotes (DRC)
  if (strlen(lpCmdLine) >= 2) {
    strncpy(cmdLine, lpCmdLine+1, GRID_FILENAME_MAX);
    cmdLine[strlen(lpCmdLine)-2] = '\0';
  }

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GRIDMAPPER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_GRIDMAPPER);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_GRIDMAPPER);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL; // No blinky, take over background painting (DRC)
	wcex.lpszMenuName	= (LPCSTR)IDC_GRIDMAPPER;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  HWND hWnd;

  hInst = hInstance; // Store instance handle in our global variable

  hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

  if (!hWnd)
    return FALSE;

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  // Put my application startup here (DRC)
  BkgdPen = CreatePen(PS_SOLID, 1, 0x00808080);
  SetSelectedFeature(hWnd, IDM_FLOOR_CLEAR);
  if (!strlen(cmdLine) || !NewMapFromFile(hWnd, cmdLine))
    NewMapFromSpecs(hWnd, DefaultMapWidth, DefaultMapHeight);

  return TRUE;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//    WM_COMMAND	- Process the application menu
//    WM_PAINT	- Paint the main window
//    WM_DESTROY	- Post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

  switch (message) {

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 

      // Catch feature-to-draw selections
      if (IDM_FLOOR_FILL <= wmId && wmId <= IDM_WALL_SECRET_DOOR) {
        SetSelectedFeature(hWnd, wmId);
        return 0;
      }

      // Catch commands requiring okay to discard changes
      if (wmId == IDM_NEW || wmId == IDM_OPEN || wmId == IDM_EXIT
        || wmId == IDM_FILL_MAP || wmId == IDM_CLEAR_MAP) {
        if (!OkDiscardChanges(hWnd))
          return 0;
      }

      // Parse other menu selections
      switch (wmId) {
				case IDM_NEW:
				  DialogBox(hInst, (LPCTSTR)IDD_NEWMAP, hWnd, (DLGPROC)NewDialog);
				  break;
        case IDM_OPEN:
          OpenMap(hWnd);
          break;
        case IDM_SAVE:
          SaveMap(hWnd);
          break;
        case IDM_SAVE_AS:
          SaveMapAs(hWnd);
          break;
        case IDM_PRINT:
          PrintMap(hWnd);
          break;
				case IDM_EXIT:
  		    DestroyWindow(hWnd);
				  break;
        case IDM_COPY:
          CopyMap(hWnd);
          break;
				case IDM_FILL_MAP:
          gridmap->clearMap(FLOOR_FILL);
          SetSelectedFeature(hWnd, IDM_FLOOR_CLEAR);
          UpdateEntireWindow(hWnd);
          break;
				case IDM_CLEAR_MAP:
          gridmap->clearMap(FLOOR_CLEAR);
          SetSelectedFeature(hWnd, IDM_FLOOR_FILL);
          UpdateEntireWindow(hWnd);
				  break;
        case IDM_SET_GRID_SIZE:
				  DialogBox(hInst, (LPCTSTR)IDD_SETGRIDSIZE, hWnd, (DLGPROC)GridSizeDialog);
				  break;
        case IDM_ABOUT:
				  DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				  break;
				default:
				  return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

    case WM_PAINT:
      MyPaintWindow(hWnd);
			break;
    case WM_LBUTTONDOWN: 
      LButtonDown = true;
      MyLButtonHandler(hWnd, lParam);
      break;
    case WM_MOUSEMOVE:
      if (LButtonDown)
        MyLButtonHandler(hWnd, lParam);
      break;
    case WM_LBUTTONUP: 
      LButtonDown = false;
      break;
		case WM_CLOSE:
      if (OkDiscardChanges(hWnd))
  			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


//-------------------------------------------------------------------------------
// End auto-generated functions; DRC functions below.
//-------------------------------------------------------------------------------

int GetGridSize() {
  return gridmap->getCellSizePixels();
}


bool OkDiscardChanges (HWND hWnd) {
  if (!gridmap->isChanged()) 
    return true;
  int retval = MessageBox(hWnd, "Okay to discard changes made to map?", 
    "Discard Changes", MB_OKCANCEL | MB_ICONWARNING);
  return (retval == IDOK);
}


void UpdateEntireWindow (HWND hWnd) {
  RECT rw;
 	GetClientRect(hWnd, &rw);
  InvalidateRect(hWnd, &rw, false);
  UpdateWindow(hWnd);
}


void SetSelectedFeature (HWND hWnd, int feature) {
  selectedFeature = feature;
  CheckMenuRadioItem(GetMenu(hWnd), IDM_FLOOR_FILL, IDM_WALL_SECRET_DOOR, 
    feature, MF_BYCOMMAND);
}


void MyPaintWindow (HWND hWnd) {
  RECT rw;
  PAINTSTRUCT ps;
	GetClientRect(hWnd, &rw);
  HDC hdc = BeginPaint(hWnd, &ps);

  // Paint map
  if (gridmap)
    gridmap->paint(hdc);

  // Paint background  
  SelectObject(hdc, BkgdPen);
  SelectObject(hdc, GetStockObject(GRAY_BRUSH));
  int mapRight = (gridmap ? gridmap->getWidthPixels() : 0);
  int mapBottom = (gridmap ? gridmap->getHeightPixels() : 0);
  Rectangle(hdc, mapRight, rw.top, rw.right, rw.bottom);
  Rectangle(hdc, rw.left, mapBottom, rw.right, rw.bottom);
	EndPaint(hWnd, &ps);
}


void MyLButtonHandler (HWND hWnd, LPARAM lParam) {

  // Make sure we're on the map
  int xPos = LOWORD(lParam);
  int yPos = HIWORD(lParam);
  if (xPos >= gridmap->getWidthPixels()
      || yPos >= gridmap->getHeightPixels())
    return;

  // Determine type of selected feature
  if (IDM_FLOOR_FILL <= selectedFeature 
    && selectedFeature <= IDM_FLOOR_WSTAIRS)
      FloorSelect(hWnd, xPos, yPos);
  else if (IDM_WALL_CLEAR <= selectedFeature 
    && selectedFeature <= IDM_WALL_SECRET_DOOR)
      WallSelect(hWnd, xPos, yPos);
}


void FloorSelect (HWND hWnd, int xPos, int yPos) {

  // Figure out what cell we're in
  int x = xPos / GetGridSize();
  int y = yPos / GetGridSize();

  // Convert menu item to feature enum
  int newFeature;
  switch (selectedFeature) {
    case IDM_FLOOR_FILL:      newFeature = FLOOR_FILL; break;
    case IDM_FLOOR_CLEAR:     newFeature = FLOOR_CLEAR; break;
    case IDM_FLOOR_NSTAIRS:   newFeature = FLOOR_NSTAIRS; break;
    case IDM_FLOOR_WSTAIRS:   newFeature = FLOOR_WSTAIRS; break;
    default: newFeature = FLOOR_FILL;
  }

  // If this is an actual change, do it & update window
  if (newFeature != gridmap->getCellFloor(x, y)) {
    if (newFeature == FLOOR_FILL)
      gridmap->clearCell(x, y);
    else
      gridmap->setCellFloor(x, y, newFeature);
   	RECT rw = {(x-1) * GetGridSize(), (y-1) * GetGridSize(),
      (x+2) * GetGridSize(), (y+2) * GetGridSize()};
    InvalidateRect(hWnd, &rw, false);
    UpdateWindow(hWnd);
  }
}


void WallSelect (HWND hWnd, int xPos, int yPos) {
  int INC = GetGridSize()/3;  // Click sensitivity in pixels

  // Figure out what cell we're near
  int xAdjust = xPos + INC;
  int yAdjust = yPos + INC;
  int x = xAdjust / GetGridSize();
  int y = yAdjust / GetGridSize();
  if (x >= gridmap->getWidthCells() 
    || y >= gridmap->getHeightCells()) return;

  // Find distance to nearby walls
  int dx = xAdjust % GetGridSize();
  int dy = yAdjust % GetGridSize();
  if (dx >= INC*2 && dy >= INC*2) return; // Not far from edge,
  if (dx < INC*2 && dy < INC*2) return;   // but not close to vertex

  // Convert menu item to feature enum
  int newFeature;
  switch (selectedFeature) {
    case IDM_WALL_CLEAR:        newFeature = WALL_CLEAR; break;
    case IDM_WALL_FILL:         newFeature = WALL_FILL; break;
    case IDM_WALL_SINGLE_DOOR:  newFeature = WALL_SINGLE_DOOR; break;
    case IDM_WALL_DOUBLE_DOOR:  newFeature = WALL_DOUBLE_DOOR; break;
    case IDM_WALL_SECRET_DOOR:  newFeature = WALL_SECRET_DOOR; break;
    default: newFeature = WALL_CLEAR;
  }

  // Change appropriate wall
  if (dx < dy)
    ChangeWestWall(hWnd, x, y, newFeature);
  else
    ChangeNorthWall(hWnd, x, y, newFeature);
}


void ChangeWestWall(HWND hWnd, int x, int y, int newFeature) {

  // Prohibitions
  if (x == 0) return;
  if (gridmap->getCellWWall(x,y) == newFeature) return;
  if (gridmap->getCellFloor(x,y) == FLOOR_FILL) return;
  if (gridmap->getCellFloor(x-1,y) == FLOOR_FILL) return;

  // Make the change
  gridmap->setCellWWall(x, y, newFeature);
  RECT rw = {(x-1) * GetGridSize(), (y) * GetGridSize(),
    (x+1) * GetGridSize(), (y+1) * GetGridSize()};
  InvalidateRect(hWnd, &rw, false);
  UpdateWindow(hWnd);
}


void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature) {

  // Prohibitions
  if (y == 0) return;
  if (gridmap->getCellNWall(x,y) == newFeature) return;
  if (gridmap->getCellFloor(x,y) == FLOOR_FILL) return;
  if (gridmap->getCellFloor(x,y-1) == FLOOR_FILL) return;

  // Make the change
  gridmap->setCellNWall(x, y, newFeature);
 	RECT rw = {(x) * GetGridSize(), (y-1) * GetGridSize(),
    (x+1) * GetGridSize(), (y+1) * GetGridSize()};
  InvalidateRect(hWnd, &rw, false);
  UpdateWindow(hWnd);
}


void CopyMap(HWND hWnd) {

  // Create bitmap with map image
  HDC tempDC = CreateCompatibleDC(GetDC(hWnd));
  HBITMAP hBitmap = CreateCompatibleBitmap(GetDC(hWnd), 
    gridmap->getWidthPixels(), gridmap->getHeightPixels());
  SelectObject(tempDC, hBitmap);
  gridmap->paint(tempDC);

  // Put it on the clipboard  
  OpenClipboard(hWnd);
  EmptyClipboard();
  SetClipboardData(CF_BITMAP, hBitmap);
  CloseClipboard();

  // Delete temporary DC
  DeleteDC(tempDC);
}


//-------------------------------------------------------------------------------
// Message handlers for dialog boxes
//-------------------------------------------------------------------------------

// "New" dialog box.message handler
LRESULT CALLBACK NewDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {

		case WM_INITDIALOG:
      SetDlgItemInt(hDlg, IDC_NEW_WIDTH, DefaultMapWidth, FALSE);
      SetDlgItemInt(hDlg, IDC_NEW_HEIGHT, DefaultMapHeight, FALSE);
      return TRUE;

    case WM_COMMAND:
      int retval = LOWORD(wParam);
      if (retval == IDOK) {
        NewMapFromSpecs(GetWindow(hDlg, GW_OWNER), 
          GetDlgItemInt(hDlg, IDC_NEW_WIDTH, NULL, FALSE),
          GetDlgItemInt(hDlg, IDC_NEW_HEIGHT, NULL, FALSE));
        EndDialog(hDlg, LOWORD(wParam));
        return TRUE;
      }
      else if (retval == IDCANCEL) {
        EndDialog(hDlg, retval);
        return TRUE;
      }
      break;
	}
  return FALSE;
}


// "Set Grid Size" dialog box.message handler
LRESULT CALLBACK GridSizeDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {

		case WM_INITDIALOG:
      SetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE, gridmap->getCellSizePixels(), FALSE);
      return TRUE;

    case WM_COMMAND:
      int retval = LOWORD(wParam);
      if (retval == IDOK) {
        int newSize = GetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE, NULL, FALSE);
        if (newSize < 10) { // Based on stairs/square * 2
          MessageBox(hDlg, "Minimum grid size is 10 pixels per square.", 
            "Size Too Small", MB_OK|MB_ICONWARNING);
        }
        else {
          gridmap->setCellSizePixels(newSize);
          UpdateEntireWindow(GetWindow(hDlg, GW_OWNER));
          EndDialog(hDlg, LOWORD(wParam));
        }
        return TRUE;
      }
      else if (retval == IDCANCEL) {
        EndDialog(hDlg, retval);
        return TRUE;
      }
      else if (retval == IDC_DEFAULT) {
        SetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE, gridmap->getCellSizeDefault(), FALSE);
        return TRUE;
      }
      break;
	}
  return FALSE;
}


// "About" dialog box message handler
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {

		case WM_INITDIALOG:
				return TRUE;

    case WM_COMMAND:
      int retval = LOWORD(wParam);
      if (retval == IDOK || retval == IDCANCEL) {
				EndDialog(hDlg, retval);
				return TRUE;
			}
			break;
	}
  return FALSE;
}


//-------------------------------------------------------------------------------
// File menu actions
//-------------------------------------------------------------------------------

void FillOpenFileStruct (HWND hWnd, OPENFILENAME *info, CHAR *nameStr, int nameSize) {
  memset(info, 0, sizeof(OPENFILENAME));
  info->lStructSize = sizeof(OPENFILENAME);
  info->lpstrFilter = "GridMapper Files (*.gmap)\0*.gmap\0";
  info->lpstrDefExt = "gmap";
  info->lpstrFile = nameStr;
  info->nMaxFile = nameSize;
  info->hwndOwner = hWnd;
}


void SetNewMap (HWND hWnd, GridMap *newmap) {
  delete gridmap;
  gridmap = newmap;
  SetSelectedFeature(hWnd, IDM_FLOOR_CLEAR);
  UpdateEntireWindow(hWnd);
}


int NewMapFromSpecs (HWND hWnd, int newWidth, int newHeight) {
  GridMap *newmap = new GridMap(newWidth, newHeight);
  if (!newmap) {
    MessageBox(hWnd, "Could not create new map.", "Error", MB_OK|MB_ICONERROR);
    return 0;
  }
  else {
    SetNewMap(hWnd, newmap);
    return 1;
  }
}


int NewMapFromFile (HWND hWnd, char *filename) {
  GridMap *newmap = new GridMap(filename);
  if (!newmap) {
    MessageBox(hWnd, "Could not create new map.", "Error", MB_OK|MB_ICONERROR);
    return 0;
  }
  else if (!strlen(newmap->getFilename())) {  // signals malformed file
    MessageBox(hWnd, "Could not read map file.", "Error", MB_OK|MB_ICONERROR);
    delete newmap;
    return 0;
  }
  else {
    SetNewMap(hWnd, newmap);
    return 1;
  }
}


void OpenMap (HWND hWnd) {
  OPENFILENAME info;
  char filename[GRID_FILENAME_MAX] = "\0";
  FillOpenFileStruct(hWnd, &info, filename, GRID_FILENAME_MAX);
  if (GetOpenFileName(&info))
    NewMapFromFile(hWnd, filename);
}


void SaveMap (HWND hWnd) {
  if (strlen(gridmap->getFilename())) 
    gridmap->save();
  else
    SaveMapAs(hWnd);
}


void SaveMapAs (HWND hWnd) {
  OPENFILENAME info;
  char fileName[GRID_FILENAME_MAX] = "\0";
  FillOpenFileStruct(hWnd, &info, fileName, GRID_FILENAME_MAX);
  info.Flags = OFN_OVERWRITEPROMPT;
  if (GetSaveFileName(&info)) {
    gridmap->setFilename(fileName);
    SaveMap(hWnd);
  }
}


void PrintMap (HWND hWnd) {

  // Fill print dialog info structure
  PRINTDLG pd;
  memset(&pd, 0, sizeof(PRINTDLG));
  pd.lStructSize = sizeof(PRINTDLG);
  pd.Flags = PD_RETURNDC;
  pd.hwndOwner = hWnd;

  // Call print dialog function
  if (PrintDlg(&pd)) {

    // Create temporary bitmap with map image
    HDC tempDC = CreateCompatibleDC(pd.hDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(pd.hDC, 
      gridmap->getWidthPixels() + 1, gridmap->getHeightPixels() + 1);
    SelectObject(tempDC, hBitmap);
    gridmap->paint(tempDC);
    
    // Create job & clean up
    CreatePrintJob(pd, tempDC);
    DeleteDC(tempDC);
    DeleteDC(pd.hDC);
  }
}


void CreatePrintJob (PRINTDLG pd, HDC srcDC) {

  // Fix printed image size
  const int defaultSquaresPerInch = 4;

  // Calculate print area in pixels
  int printWidth = gridmap->getWidthPixels() 
    * GetDeviceCaps(pd.hDC, LOGPIXELSX)
    / gridmap->getCellSizeDefault() 
    / defaultSquaresPerInch;
  int printHeight = gridmap->getHeightPixels() 
    * GetDeviceCaps(pd.hDC, LOGPIXELSY)
    / gridmap->getCellSizeDefault() 
    / defaultSquaresPerInch;

  // Initialize a DOCINFO structure
  DOCINFO di;
  memset(&di, 0, sizeof(DOCINFO));
  di.cbSize = sizeof(DOCINFO); 
  di.lpszDocName = "GridMapper Document";
  di.lpszOutput = (LPTSTR) NULL; 

  // Create the print job
  StartDoc(pd.hDC, &di);
  StartPage(pd.hDC);
  StretchBlt(pd.hDC, 0, 0, printWidth, printHeight, 
    srcDC, 0, 0, gridmap->getWidthPixels() + 1,
    gridmap->getHeightPixels() + 1, SRCCOPY);
  EndPage(pd.hDC);
  EndDoc(pd.hDC);
}


/*
-------------------------------------------------------------------------
  PRINTING NOTES: In debugger, PrintDlg always calls breakpoint on app exit.
  Go > Print > (Ok or Cancel) > Exit > Debug Breakpoint.
  Doesn't matter whether I DeleteDC() or not.
  Even sample code from MSDN printing example does the same thing.
  See "Printing and Print Spooler Overview", "Using Print Dialog Box..."

  Also, Windows highly recommends putting up a "Cancel" box during
  the spooling process with a callback to handle possible cancel.
  I've skipped that at this time. See article above. (Not related issues.)

  Also, almost every one of the functions above could fail and
  take error-handling code (as shown in MSDN examples I've stripped out).

  Also, UI could take a "Page Setup" menu item -- although:
  (a) most would be disabled, (b) orientation would be one useful thing
  but I'm unsure how to record/use it, (c) would require "Paint Page Hook"
  to draw facsimile of our map in the right area. Also test creates same
  debug breakpoint as the "Print" dialog.
-------------------------------------------------------------------------
*/


/*
=====================================================================
LICENSING INFORMATION

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

The author may be contacted by email at: delta@superdan.net
=====================================================================
*/

