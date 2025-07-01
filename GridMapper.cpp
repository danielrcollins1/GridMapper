/*
	Name: GridMapper.cpp
	Copyright: 2010
	Author: Daniel R. Collins
	Date: 16-05-10
	Description: Application entry point and Windows-UI stuff.
		See file LICENSE for licensing information.
		Contact author at delta@superdan.net
*/
#include "GridMapper.h"
#include "Resource.h"
#include <sstream>

// Constants
const int MAX_LOADSTRING = 100;
const int DefaultMapWidth = 40;
const int DefaultMapHeight = 30;
const int DefaultPrintSquaresPerInch = 4;
const int MinimumGridSize = 10;
const char DefaultFileExt[] = "gmap";
const char FileFilterStr[] = "GridMapper Files (*.gmap)\0*.gmap\0";

// Global variables
HDC BkgdDC;
HPEN BkgdPen;
HBITMAP BkgdBitmap;
HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
GridMap *gridmap = NULL;
int selectedFeature = 0;
bool LButtonCapture = false;
char cmdLine[GRID_FILENAME_MAX] = "\0";

// Function prototypes
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
bool CommandProc(HWND hWnd, int cmdId);
void DestroyObjects();

/*
	Win32 application entry point.
	Initialize strings, instance, accelerators.
	Run main message loop here.
*/
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Copy command line without quotes
	if (strlen(lpCmdLine) >= 2) {
		strncpy(cmdLine, lpCmdLine+1, GRID_FILENAME_MAX);
		cmdLine[strlen(lpCmdLine)-2] = '\0';
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GRIDMAPPER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization
	if (!InitInstance(hInstance, nCmdShow)) {
		return FALSE;
	}
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_GRIDMAPPER);

	// Main message loop
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

/*
	Register the window class.

	This function and its usage is only necessary if you want this code
	to be compatible with Win32 systems prior to the 'RegisterClassEx'
	function that was added to Windows 95. It's important to call this function
	so that the application will get 'well formed' small icons associated
	with it.
*/
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_GRIDMAPPER);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= (LPCSTR)IDC_GRIDMAPPER;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);
	return RegisterClassEx(&wcex);
}

/*
	Saves instance handle and creates main window.
	In this function, we save the instance handle in a global variable and
	create and display the main program window.
*/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance;
	hWnd = CreateWindow(szWindowClass, szTitle,
	                    WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
	                    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL,
	                    hInstance, NULL);
	if (!hWnd)
		return FALSE;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Application startup
	BkgdPen = CreatePen(PS_SOLID, 1, 0x00808080);
	SetSelectedFeature(hWnd, IDM_FLOOR_CLEAR);
	if (!strlen(cmdLine) || !NewMapFromFile(hWnd, cmdLine))
		NewMapFromSpecs(hWnd, DefaultMapWidth, DefaultMapHeight);
	return TRUE;
}

/*
	Processes messages for the main window.
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_PAINT:
			MyPaintWindow(hWnd);
			break;
		case WM_SIZE:
			SetScrollRange(hWnd, false);
			break;
		case WM_HSCROLL:
			HorzScrollHandler(hWnd, wParam);
			break;
		case WM_VSCROLL:
			VertScrollHandler(hWnd, wParam);
			break;
		case WM_MOUSEWHEEL:
			ScrollWheelHandler(hWnd, wParam);
			break;
		case WM_KEYDOWN:
			MyKeyHandler(hWnd, wParam);
			break;
		case WM_COMMAND:
			if (!CommandProc(hWnd, LOWORD(wParam)))
				return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		case WM_LBUTTONUP:
			LButtonCapture = false;
			break;
		case WM_LBUTTONDOWN:
			LButtonCapture = true;
			MyLButtonHandler(hWnd, lParam);
			break;
		case WM_MOUSEMOVE:
			if (LButtonCapture && (wParam & MK_LBUTTON))
				MyLButtonHandler(hWnd, lParam);
			break;
		case WM_CLOSE:
			if (OkDiscardChanges(hWnd))
				return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		case WM_DESTROY:
			DestroyObjects();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*
	Processes application-specific commands for the main window,
	sent via WM_COMMAND. Returns "true" if we handle it.
*/
bool CommandProc(HWND hWnd, int cmdId)
{
	// Catch commands requiring okay to discard changes
	if (cmdId == IDM_NEW || cmdId == IDM_OPEN || cmdId == IDM_EXIT
	        || cmdId == IDM_FILL_MAP || cmdId == IDM_CLEAR_MAP) {
		if (!OkDiscardChanges(hWnd))
			return true;
	}

	// Catch feature-to-draw Tool selections
	if (IDM_FLOOR_FILL <= cmdId && cmdId <= IDM_WALL_SECRET_DOOR) {
		SetSelectedFeature(hWnd, cmdId);
		return true;
	}

	// Parse other menu selections
	switch (cmdId) {
		case IDM_OPEN:
			OpenMap(hWnd);
			break;
		case IDM_SAVE:
			SaveMap(hWnd);
			break;
		case IDM_SAVE_AS:
			SaveMapAs(hWnd);
			break;
		case IDM_COPY:
			CopyMap(hWnd);
			break;
		case IDM_PRINT:
			PrintMap(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_FILL_MAP:
			ClearMap(hWnd, false);
			break;
		case IDM_CLEAR_MAP:
			ClearMap(hWnd, true);
			break;
		case IDM_NEW:
			DialogBox(hInst, (LPCTSTR)IDD_NEWMAP, hWnd, (DLGPROC)NewDialog);
			break;
		case IDM_SET_GRID_SIZE:
			DialogBox(hInst, (LPCTSTR)IDD_SETGRIDSIZE, hWnd,
			          (DLGPROC)GridSizeDialog);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		default:
			return false;
	}
	return true;
}

/*
	Cleans up application at end.
*/
void DestroyObjects()
{
	DeleteObject(BkgdDC);
	DeleteObject(BkgdPen);
	DeleteObject(BkgdBitmap);
	delete gridmap;
}

//-----------------------------------------------------------------------------
// Application-specific functions
//-----------------------------------------------------------------------------

int GetGridSize()
{
	return gridmap->getCellSizePixels();
}

void UpdateBkgdCell(HWND hWnd, int x, int y)
{
	gridmap->paintCell(BkgdDC, x, y, true);
	UpdateEntireWindow(hWnd);
}

void UpdateEntireWindow(HWND hWnd)
{
	RECT rw;
	GetClientRect(hWnd, &rw);
	InvalidateRect(hWnd, &rw, false);
	UpdateWindow(hWnd);
}

void SetScrollRange(HWND hWnd, bool zeroPos)
{
	// Get client rectangle
	RECT rw;
	GetClientRect(hWnd, &rw);

	// Set horizontal scrollbar
	int hMax = (gridmap ? gridmap->getWidthPixels() : 0);
	int hPos = (zeroPos ? 0 : GetHorzScrollPos(hWnd));
	SCROLLINFO infoHorz = {sizeof(SCROLLINFO), SIF_ALL, 0, hMax,
	                       (UINT) rw.right, hPos, 0
	                      };
	SetScrollInfo(hWnd, SB_HORZ, &infoHorz, TRUE);

	// Set vertical scrollbar
	int vMax = (gridmap ? gridmap->getHeightPixels() : 0);
	int vPos = (zeroPos ? 0 : GetVertScrollPos(hWnd));
	SCROLLINFO infoVert = {sizeof(SCROLLINFO), SIF_ALL, 0, vMax,
	                       (UINT) rw.bottom, vPos, 0
	                      };
	SetScrollInfo(hWnd, SB_VERT, &infoVert, TRUE);
}

void HorzScrollHandler(HWND hWnd, WPARAM wParam)
{
	// Get scrollbar info
	SCROLLINFO info = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
	GetScrollInfo(hWnd, SB_HORZ, &info);

	// Set new scrollbar position (via WM_HSCROLL)
	switch (LOWORD(wParam)) {
		case SB_LINEUP:
			info.nPos -= GetGridSize();
			break;
		case SB_LINEDOWN:
			info.nPos += GetGridSize();
			break;
		case SB_PAGEUP:
			info.nPos -= info.nPage;
			break;
		case SB_PAGEDOWN:
			info.nPos += info.nPage;
			break;
		case SB_THUMBPOSITION:
			info.nPos = HIWORD(wParam);
			break;
		case SB_THUMBTRACK:
			info.nPos = HIWORD(wParam);
			break;
		case SB_TOP:
			info.nPos = info.nMin;
			break;
		case SB_BOTTOM:
			info.nPos = info.nMax;
			break;
	}
	SetScrollInfo(hWnd, SB_HORZ, &info, true);
	UpdateEntireWindow(hWnd);
}

void VertScrollHandler(HWND hWnd, WPARAM wParam)
{
	// Get scrollbar info
	SCROLLINFO info = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
	GetScrollInfo(hWnd, SB_VERT, &info);

	// Set new scrollbar position (via WM_VSCROLL)
	switch (LOWORD(wParam)) {
		case SB_LINEUP:
			info.nPos -= GetGridSize();
			break;
		case SB_LINEDOWN:
			info.nPos += GetGridSize();
			break;
		case SB_PAGEUP:
			info.nPos -= info.nPage;
			break;
		case SB_PAGEDOWN:
			info.nPos += info.nPage;
			break;
		case SB_THUMBPOSITION:
			info.nPos = HIWORD(wParam);
			break;
		case SB_THUMBTRACK:
			info.nPos = HIWORD(wParam);
			break;
		case SB_TOP:
			info.nPos = info.nMin;
			break;
		case SB_BOTTOM:
			info.nPos = info.nMax;
			break;
	}
	SetScrollInfo(hWnd, SB_VERT, &info, true);
	UpdateEntireWindow(hWnd);
}

void ScrollWheelHandler(HWND hWnd, WPARAM wParam)
{
	// Get adjustment steps
	int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	int steps = wheelDelta / 120;

	// Handle zoom-in or out (Ctrl pressed)
	if (wParam & MK_CONTROL) {
		int newSize = GetGridSize() + steps;
		if (newSize < MinimumGridSize) {
			newSize = MinimumGridSize;
		}
		ChangeGridSize(hWnd, newSize);
	}

	// Handle movement of scrollbars
	else {
		// Get scrollbar info
		SCROLLINFO info = {sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0};
		int scrollBar = wParam & MK_SHIFT ? SB_HORZ : SB_VERT;
		GetScrollInfo(hWnd, scrollBar, &info);

		// Set new scrollbar position
		info.nPos -= steps * GetGridSize();
		SetScrollInfo(hWnd, scrollBar, &info, true);
		UpdateEntireWindow(hWnd);
	}
}

int GetHorzScrollPos(HWND hWnd)
{
	SCROLLINFO info = {sizeof(SCROLLINFO), SIF_PAGE|SIF_POS, 0, 0, 0, 0, 0};
	GetScrollInfo(hWnd, SB_HORZ, &info);
	return info.nPos;
}

int GetVertScrollPos(HWND hWnd)
{
	SCROLLINFO info = {sizeof(SCROLLINFO), SIF_PAGE|SIF_POS, 0, 0, 0, 0, 0};
	GetScrollInfo(hWnd, SB_VERT, &info);
	return info.nPos;
}

void MyKeyHandler(HWND hWnd, WPARAM wParam)
{
	switch (wParam) {
		case VK_LEFT:
			SendMessage(hWnd, WM_HSCROLL, SB_LINELEFT, 0);
			break;
		case VK_RIGHT:
			SendMessage(hWnd, WM_HSCROLL, SB_LINERIGHT, 0);
			break;
		case VK_UP:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEUP, 0);
			break;
		case VK_DOWN:
			SendMessage(hWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			break;
		case VK_PRIOR:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
			break;
		case VK_NEXT:
			SendMessage(hWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			break;
		case VK_HOME:
			SendMessage(hWnd, WM_HSCROLL, SB_TOP, 0);
			SendMessage(hWnd, WM_VSCROLL, SB_TOP, 0);
			break;
		case VK_END:
			SendMessage(hWnd, WM_HSCROLL, SB_BOTTOM, 0);
			SendMessage(hWnd, WM_VSCROLL, SB_BOTTOM, 0);
			break;
	}
}

void MyPaintWindow(HWND hWnd)
{
	// Set up paint process
	RECT rw;
	PAINTSTRUCT ps;
	GetClientRect(hWnd, &rw);
	HDC hdc = BeginPaint(hWnd, &ps);
	SelectObject(hdc, GetStockObject(GRAY_BRUSH));
	SelectObject(hdc, BkgdPen);

	// If map available, blit from memory & paint background bottom-right
	if (gridmap) {
		BitBlt(hdc, 0, 0, rw.right, rw.bottom, BkgdDC,
		       GetHorzScrollPos(hWnd), GetVertScrollPos(hWnd), SRCCOPY);
		int rightPixel = gridmap->getWidthPixels() - GetHorzScrollPos(hWnd);
		int bottomPixel = gridmap->getHeightPixels() - GetVertScrollPos(hWnd);
		Rectangle(hdc, rightPixel, rw.top, rw.right, rw.bottom);
		Rectangle(hdc, rw.left, bottomPixel, rw.right, rw.bottom);
	}
	else {
		Rectangle(hdc, rw.left, rw.top, rw.right, rw.bottom);
	}
	EndPaint(hWnd, &ps);
}

void MyLButtonHandler(HWND hWnd, LPARAM lParam)
{
	// Make sure we're on the map
	int xPos = LOWORD(lParam) + GetHorzScrollPos(hWnd);
	int yPos = HIWORD(lParam) + GetVertScrollPos(hWnd);
	if (xPos >= gridmap->getWidthPixels()
	        || yPos >= gridmap->getHeightPixels())
		return;

	// Determine type of selected feature
	if (IDM_FLOOR_FILL <= selectedFeature
	        && selectedFeature <= IDM_FLOOR_NWDOOR)
		FloorSelect(hWnd, xPos, yPos);
	else if (IDM_WALL_CLEAR <= selectedFeature
	         && selectedFeature <= IDM_WALL_SECRET_DOOR)
		WallSelect(hWnd, xPos, yPos);
}

void FloorSelect(HWND hWnd, int xPos, int yPos)
{
	// Figure out what cell we're in
	int x = xPos / GetGridSize();
	int y = yPos / GetGridSize();

	// Convert menu item to feature enum
	int newFeature;
	switch (selectedFeature) {
		case IDM_FLOOR_FILL:
			newFeature = FLOOR_FILL;
			break;
		case IDM_FLOOR_CLEAR:
			newFeature = FLOOR_CLEAR;
			break;
		case IDM_FLOOR_NSTAIRS:
			newFeature = FLOOR_NSTAIRS;
			break;
		case IDM_FLOOR_WSTAIRS:
			newFeature = FLOOR_WSTAIRS;
			break;
		case IDM_FLOOR_NEWALL:
			newFeature = FLOOR_NEWALL;
			break;
		case IDM_FLOOR_NWWALL:
			newFeature = FLOOR_NWWALL;
			break;
		case IDM_FLOOR_NEDOOR:
			newFeature = FLOOR_NEDOOR;
			break;
		case IDM_FLOOR_NWDOOR:
			newFeature = FLOOR_NWDOOR;
			break;
		default:
			newFeature = FLOOR_FILL;
	}

	// If this is an actual change, do it & update window
	if (newFeature != gridmap->getCellFloor(x, y)) {
		if (newFeature == FLOOR_FILL) {
			FillCell(hWnd, x, y);
		}
		else {
			gridmap->setCellFloor(x, y, newFeature);
			UpdateBkgdCell(hWnd, x, y);
		}
	}
}

void WallSelect(HWND hWnd, int xPos, int yPos)
{
	// Set click sensitivity
	int INC = GetGridSize() / 3;

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

	// Abort if not far from edge nor close to vertex
	if (dx >= INC*2 && dy >= INC*2) return;
	if (dx < INC*2 && dy < INC*2) return;

	// Convert menu item to feature enum
	int newFeature;
	switch (selectedFeature) {
		case IDM_WALL_CLEAR:
			newFeature = WALL_CLEAR;
			break;
		case IDM_WALL_FILL:
			newFeature = WALL_FILL;
			break;
		case IDM_WALL_SINGLE_DOOR:
			newFeature = WALL_SINGLE_DOOR;
			break;
		case IDM_WALL_DOUBLE_DOOR:
			newFeature = WALL_DOUBLE_DOOR;
			break;
		case IDM_WALL_SECRET_DOOR:
			newFeature = WALL_SECRET_DOOR;
			break;
		default:
			newFeature = WALL_CLEAR;
	}

	// Change appropriate wall
	if (dx < dy)
		ChangeWestWall(hWnd, x, y, newFeature);
	else
		ChangeNorthWall(hWnd, x, y, newFeature);
}

void ChangeWestWall(HWND hWnd, int x, int y, int newFeature)
{
	// Prohibitions
	if (x == 0) return;
	if (gridmap->getCellWWall(x,y) == newFeature) return;
	if (gridmap->getCellFloor(x,y) == FLOOR_FILL) return;
	if (gridmap->getCellFloor(x-1,y) == FLOOR_FILL) return;

	// Make the change
	gridmap->setCellWWall(x, y, newFeature);
	UpdateBkgdCell(hWnd, x-1, y);
	UpdateBkgdCell(hWnd, x, y);
}

void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature)
{
	// Prohibitions
	if (y == 0) return;
	if (gridmap->getCellNWall(x,y) == newFeature) return;
	if (gridmap->getCellFloor(x,y) == FLOOR_FILL) return;
	if (gridmap->getCellFloor(x,y-1) == FLOOR_FILL) return;

	// Make the change
	gridmap->setCellNWall(x, y, newFeature);
	UpdateBkgdCell(hWnd, x, y-1);
	UpdateBkgdCell(hWnd, x, y);
}

void ClearMap(HWND hWnd, bool clear)
{
	SetSelectedFeature(hWnd, clear ? IDM_FLOOR_FILL : IDM_FLOOR_CLEAR);
	gridmap->clearMap(clear ? FLOOR_CLEAR : FLOOR_FILL);
	gridmap->paint(BkgdDC);
	UpdateEntireWindow(hWnd);
}

/*
	Fill a cell's floor.
	We need to wipe out any object, wipe any adjacent walls,
	and repaint all adjacent cells.
*/
void FillCell(HWND hWnd, int x, int y)
{
	// Get height & width
	int width = gridmap->getWidthCells();
	int height = gridmap->getHeightCells();

	// Clear elements
	gridmap->setCellFloor(x, y, FLOOR_FILL);
	gridmap->setCellNWall(x, y, WALL_CLEAR);
	gridmap->setCellWWall(x, y, WALL_CLEAR);
	gridmap->setCellObject(x, y, OBJECT_NONE);
	if (x+1 < width) gridmap->setCellWWall(x+1, y, WALL_CLEAR);
	if (y+1 < width) gridmap->setCellNWall(x, y+1, WALL_CLEAR);

	// Repaint elements
	gridmap->paintCell(BkgdDC, x, y, true);
	if (x-1 >= 0) gridmap->paintCell(BkgdDC, x-1, y, true);
	if (y-1 >= 0) gridmap->paintCell(BkgdDC, x, y-1, true);
	if (x+1 < width) gridmap->paintCell(BkgdDC, x+1, y, true);
	if (y+1 < height) gridmap->paintCell(BkgdDC, x, y+1, true);
	UpdateEntireWindow(hWnd);
}

//-----------------------------------------------------------------------------
// Menu actions
//-----------------------------------------------------------------------------

void SetSelectedFeature(HWND hWnd, int feature)
{
	selectedFeature = feature;
	CheckMenuRadioItem(GetMenu(hWnd), IDM_FLOOR_FILL, IDM_WALL_SECRET_DOOR,
	                   feature, MF_BYCOMMAND);
}

bool OkDiscardChanges(HWND hWnd)
{
	if (!gridmap->isChanged())
		return true;
	int retval = MessageBox(hWnd, "Okay to discard changes made to map?",
	                        "Discard Changes", MB_OKCANCEL | MB_ICONWARNING);
	return (retval == IDOK);
}

void SetBkgdDC(HWND hWnd)
{
	DeleteObject(BkgdDC);
	DeleteObject(BkgdBitmap);
	BkgdDC = CreateCompatibleDC(GetDC(hWnd));
	BkgdBitmap = CreateCompatibleBitmap(GetDC(hWnd),
	                                    gridmap->getWidthPixels(),
	                                    gridmap->getHeightPixels());
	if (BkgdBitmap) {
		SelectObject(BkgdDC, BkgdBitmap);
		gridmap->paint(BkgdDC);
	}
	else {
		MessageBox(hWnd, "Could not create large enough bitmap."
		           "\nMap will not display; try smaller grid size?",
		           "Map Too Large", MB_OK|MB_ICONERROR);
	}
}

void ChangeGridSize(HWND hWnd, int size)
{
	gridmap->setCellSizePixels(size);
	SetBkgdDC(hWnd);
	SetScrollRange(hWnd, true);
	UpdateEntireWindow(hWnd);
}

void SetNewMap(HWND hWnd, GridMap *newmap)
{
	delete gridmap;
	gridmap = newmap;
	SetBkgdDC(hWnd);
	SetScrollRange(hWnd, true);
	SetSelectedFeature(hWnd, IDM_FLOOR_CLEAR);
	UpdateEntireWindow(hWnd);
}

bool NewMapFromSpecs(HWND hWnd, int newWidth, int newHeight)
{
	GridMap *newmap = new GridMap(newWidth, newHeight);
	if (!newmap) {
		MessageBox(hWnd, "Could not create new map.", "Error",
		           MB_OK|MB_ICONERROR);
		return false;
	}
	else {
		SetNewMap(hWnd, newmap);
		return true;
	}
}

bool NewMapFromFile(HWND hWnd, char *filename)
{
	GridMap *newmap = new GridMap(filename);
	if (!newmap) {
		MessageBox(hWnd, "Could not create new map.", "Error",
		           MB_OK|MB_ICONERROR);
		return false;
	}
	else if (!newmap->isFileLoadOk()) {
		MessageBox(hWnd, "Could not read map file.", "Error",
		           MB_OK|MB_ICONERROR);
		delete newmap;
		return false;
	}
	else {
		SetNewMap(hWnd, newmap);
		return true;
	}
}

void OpenMap(HWND hWnd)
{
	char filename[GRID_FILENAME_MAX] = "\0";
	OPENFILENAME info = {sizeof(OPENFILENAME), hWnd, 0,
	                     FileFilterStr, 0, 0, 0, filename, GRID_FILENAME_MAX,
	                     0, 0, 0, 0, 0, 0, 0, DefaultFileExt, 0, 0, 0
	                    };
	if (GetOpenFileName(&info))
		NewMapFromFile(hWnd, filename);
}

void SaveMapAs(HWND hWnd)
{
	char filename[GRID_FILENAME_MAX] = "\0";
	OPENFILENAME info = {sizeof(OPENFILENAME), hWnd, 0,
	                     FileFilterStr, 0, 0, 0, filename, GRID_FILENAME_MAX,
	                     0, 0, 0, 0, OFN_OVERWRITEPROMPT, 0, 0, DefaultFileExt,
	                     0, 0, 0
	                    };
	if (GetSaveFileName(&info)) {
		gridmap->setFilename(filename);
		SaveMap(hWnd);
	}
}

void SaveMap(HWND hWnd)
{
	if (strlen(gridmap->getFilename()))
		gridmap->save();
	else
		SaveMapAs(hWnd);
}

void CopyMap(HWND hWnd)
{
	// Create bitmap with map image
	HDC tempDC = CreateCompatibleDC(GetDC(hWnd));
	HBITMAP hBitmap = CreateCompatibleBitmap(GetDC(hWnd),
	                  gridmap->getWidthPixels(), gridmap->getHeightPixels());
	SelectObject(tempDC, hBitmap);
	BitBlt(tempDC, 0, 0, gridmap->getWidthPixels(),
	       gridmap->getHeightPixels(), BkgdDC, 0, 0, SRCCOPY);

	// Put it on the clipboard
	OpenClipboard(hWnd);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP, hBitmap);
	CloseClipboard();

	// Delete temporary DC
	DeleteDC(tempDC);
}

void PrintMap(HWND hWnd)
{
	// Call print dialog
	PRINTDLG pd = {sizeof(PRINTDLG), hWnd, 0, 0, 0,
	               PD_RETURNDC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	              };
	if (PrintDlg(&pd)) {

		// Calculate print width in pixels
		int printWidth = gridmap->getWidthPixels()
		                 * GetDeviceCaps(pd.hDC, LOGPIXELSX)
		                 / DefaultPrintSquaresPerInch
		                 / gridmap->getCellSizeDefault();

		// Calculate print height in pixels
		int printHeight = gridmap->getHeightPixels()
		                  * GetDeviceCaps(pd.hDC, LOGPIXELSY)
		                  / DefaultPrintSquaresPerInch
		                  / gridmap->getCellSizeDefault();

		// Create the print job
		DOCINFO di = {sizeof(DOCINFO), "GridMapper Document", 0, 0, 0};
		StartDoc(pd.hDC, &di);
		StartPage(pd.hDC);
		StretchBlt(pd.hDC, 0, 0, printWidth, printHeight,
		           BkgdDC, 0, 0, gridmap->getWidthPixels(),
		           gridmap->getHeightPixels(), SRCCOPY);
		EndPage(pd.hDC);
		EndDoc(pd.hDC);
	}
}

/*
-------------------------------------------------------------------------
  PRINTING NOTES: PrintDlg always triggers debug breakpoint on app exit.
  Sample code from MSDN example, Scribble app all do the same thing.
  Working in Win 2K, VC++6.0 here (Knowledge Base references WinNT 3.51).
  In doexit(), looks like "CrtDumpMemoryLeaks" failed.

  See: http://support.microsoft.com/kb/138993/
  "RESOLUTION: The message and the break can be safely ignored.
  To continue with execution in the debugger, you can just click Go on
  the Debug menu, or press the F5 key." (twice, DRC)

  Also, Windows highly recommends putting up a "Cancel" box during
  the spooling process with a callback to handle possible cancel.
  I've skipped that at this time. See article above. (Not related issue.)

  Also, almost every one of the functions above could fail and
  take error-handling code (as shown in MSDN examples I've stripped out).

  Also, UI could take a "Page Setup" menu item -- although:
  (a) most would be disabled, (b) orientation would be one useful thing
  but I'm unsure how to record/use it, (c) would require "Paint Page Hook"
  to draw facsimile of our map in the right area. Also test creates same
  debug breakpoint as the "Print" dialog.

  If map is totally clear you'll notice lack of border on far right/bottom.
  Fix requires making a new background image +1 size, getting pen &
  drawing border, then StretchBlit that. Skipping that at this time.
-------------------------------------------------------------------------
*/

//-----------------------------------------------------------------------------
// Message handlers for dialog boxes
//-----------------------------------------------------------------------------

// "New" dialog box.message handler
LRESULT CALLBACK NewDialog(HWND hDlg, UINT message,
                           WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDC_NEW_WIDTH, DefaultMapWidth, FALSE);
			SetDlgItemInt(hDlg, IDC_NEW_HEIGHT, DefaultMapHeight, FALSE);
			return TRUE;

		case WM_COMMAND:
			int retval = LOWORD(wParam);
			if (retval == IDOK) {
				NewMapFromSpecs(GetWindow(hDlg, GW_OWNER),
				                GetDlgItemInt(hDlg, IDC_NEW_WIDTH, NULL,
				                              FALSE),
				                GetDlgItemInt(hDlg, IDC_NEW_HEIGHT, NULL,
				                              FALSE));
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
LRESULT CALLBACK GridSizeDialog(HWND hDlg, UINT message,
                                WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE,
			              gridmap->getCellSizePixels(), FALSE);
			return TRUE;

		case WM_COMMAND:
			int retval = LOWORD(wParam);
			if (retval == IDOK) {
				int newSize = GetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE, NULL,
				                            FALSE);

				// Check minimum size re: stairs-per-square * 2
				if (newSize < MinimumGridSize) {
					std::stringstream message;
					message << "Minimum grid size is "
					        << MinimumGridSize << " pixels per square.";
					MessageBox(hDlg, message.str().c_str(),
					           "Size Too Small", MB_OK|MB_ICONWARNING);
				}
				else {

					// Check for actual change
					if (newSize != gridmap->getCellSizePixels()) {
						ChangeGridSize(GetWindow(hDlg, GW_OWNER), newSize);
					}
					EndDialog(hDlg, LOWORD(wParam));
				}
				return TRUE;
			}
			else if (retval == IDCANCEL) {
				EndDialog(hDlg, retval);
				return TRUE;
			}
			else if (retval == IDC_DEFAULT) {
				SetDlgItemInt(hDlg, IDC_PXLS_PER_SQUARE,
				              gridmap->getCellSizeDefault(), FALSE);
				return TRUE;
			}
			break;
	}
	return FALSE;
}

// "About" dialog box message handler
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
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
