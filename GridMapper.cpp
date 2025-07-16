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
#include <cassert>
#include <ctime>

// Constants
const int MAX_LOADSTRING = 100;
const int DefaultMapWidth = 40;
const int DefaultMapHeight = 30;
const int DefaultPrintSquaresPerInch = 4;
const int ScrollWheelIncrement = 120;
const char DefaultFileExt[] = "gmap";
const char FileFilterStr[] = "GridMapper Files (*.gmap)\0*.gmap\0";

// Global variables
HWND hMainWnd;
HINSTANCE hInst;
HDC BkgdDC;
HPEN BkgdPen;
HBITMAP BkgdBitmap;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
GridMap *gridmap = NULL;
int selectedFeature = 0;
bool LButtonCapture = false;

// Function prototypes
ATOM MyRegisterClass(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NewDialog(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GridSizeDialog(HWND, UINT, WPARAM, LPARAM);

/*
	Win32 application entry point.
	Initialize strings, instance, accelerators.
	Run main message loop here.
*/
int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GRIDMAPPER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization
	if (InitInstance(hInstance, nCmdShow)) {

		// Initialize our custom application
		InitGridMapper();

		// Load hotkey accelerators
		HACCEL hAccelTable =
		    LoadAccelerators(hInstance, (LPCTSTR)IDC_GRIDMAPPER);

		// Main message loop
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return msg.wParam;
	}
	return FALSE;
}

/*
	Register the window class.
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

	// Register the class
	return RegisterClassEx(&wcex);
}

/*
	Saves instance handle and creates main window.
	In this function, we save the instance handle in a global variable and
	create and display the main program window.
*/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;
	hMainWnd =
	    CreateWindow(
	        szWindowClass, szTitle,
	        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
	        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL,
	        hInstance, NULL);
	if (hMainWnd) {
		ShowWindow(hMainWnd, nCmdShow);
		UpdateWindow(hMainWnd);
		return TRUE;
	}
	return FALSE;
}

/*
	Initializes custom stuff for GridMapper application.
*/
void InitGridMapper()
{
	srand((unsigned int) time(NULL));
	BkgdPen = CreatePen(PS_SOLID, 1, 0x00808080);
	InitFirstMap();
}

/*
	Initialize the first map on application startup.
*/
void InitFirstMap()
{
	// Try to open file from command line
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc > 1) {
		char buffer[512];
		WideCharToMultiByte(
		    CP_UTF8, 0, argv[1], -1, buffer, sizeof(buffer), NULL, NULL);
		NewMapFromFile(buffer);
	}
	LocalFree(argv);

	// If no file loaded, make a blank map
	if (!gridmap) {
		NewMapFromSpecs(DefaultMapWidth, DefaultMapHeight);
	}
}

/*
	Processes messages for the main window.
*/
LRESULT CALLBACK WndProc(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_PAINT:
			MyPaintWindow();
			break;
		case WM_SIZE:
			SetScrollRange(false);
			break;
		case WM_HSCROLL:
			HorzScrollHandler(wParam);
			break;
		case WM_VSCROLL:
			VertScrollHandler(wParam);
			break;
		case WM_MOUSEWHEEL:
			ScrollWheelHandler(wParam);
			break;
		case WM_KEYDOWN:
			MyKeyHandler(wParam);
			break;
		case WM_COMMAND:
			if (!ProcessCommand(LOWORD(wParam)))
				return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		case WM_LBUTTONUP:
			LButtonCapture = false;
			break;
		case WM_LBUTTONDOWN:
			LButtonCapture = true;
			MyLButtonHandler(lParam);
			break;
		case WM_MOUSEMOVE:
			if (LButtonCapture && (wParam & MK_LBUTTON))
				MyLButtonHandler(lParam);
			break;
		case WM_CLOSE:
			if (OkDiscardChanges())
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
bool ProcessCommand(int cmdId)
{
	// Catch commands requiring okay to discard changes
	if (cmdId == IDM_NEW || cmdId == IDM_OPEN || cmdId == IDM_EXIT
	        || cmdId == IDM_FILL_MAP || cmdId == IDM_CLEAR_MAP) {
		if (!OkDiscardChanges())
			return true;
	}

	// Catch feature-to-draw Tool selections
	if (START_BASIC_FLOOR_TOOLS <= cmdId && cmdId <= END_OBJECT_TOOLS) {
		SetSelectedFeature(cmdId);
		return true;
	}

	// Parse other menu selections
	switch (cmdId) {
		case IDM_OPEN:
			OpenMap();
			break;
		case IDM_SAVE:
			SaveMap();
			break;
		case IDM_SAVE_AS:
			SaveMapAs();
			break;
		case IDM_COPY:
			CopyMap();
			break;
		case IDM_PRINT:
			PrintMap();
			break;
		case IDM_EXIT:
			DestroyWindow(hMainWnd);
			break;
		case IDM_FILL_MAP:
			ClearMap(false);
			break;
		case IDM_CLEAR_MAP:
			ClearMap(true);
			break;
		case IDM_HIDE_GRID:
			ToggleGridLines();
			break;
		case IDM_ROUGH_EDGES:
			ToggleRoughEdges();
			break;
		case IDM_NEW:
			DialogBox(
				hInst, (LPCTSTR) IDD_NEWMAP, 
				hMainWnd, (DLGPROC) NewDialog);
			break;
		case IDM_ABOUT:
			DialogBox(
				hInst, (LPCTSTR) IDD_ABOUTBOX, 
				hMainWnd, (DLGPROC) About);
			break;
		case IDM_SET_GRID_SIZE:
			DialogBox(
			    hInst, (LPCTSTR) IDD_SETGRIDSIZE, 
				hMainWnd, (DLGPROC) GridSizeDialog);
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

unsigned GetGridSize()
{
	return gridmap->getCellSizePixels();
}

void UpdateBkgdCell(GridCoord gc)
{
	gridmap->paintCell(gc, true);
	UpdateEntireWindow();
}

void UpdateEntireWindow()
{
	RECT rw;
	GetClientRect(hMainWnd, &rw);
	InvalidateRect(hMainWnd, &rw, false);
	UpdateWindow(hMainWnd);
}

void SetScrollRange(bool zeroPos)
{
	// Get client rectangle
	RECT rw;
	GetClientRect(hMainWnd, &rw);

	// Set horizontal scrollbar
	int hMax = (gridmap ? gridmap->getWidthPixels() : 0);
	int hPos = (zeroPos ? 0 : GetHorzScrollPos());
	SCROLLINFO infoHorz = {
		sizeof(SCROLLINFO), SIF_ALL, 0, hMax,
		(UINT) rw.right, hPos, 0
	};
	SetScrollInfo(hMainWnd, SB_HORZ, &infoHorz, TRUE);

	// Set vertical scrollbar
	int vMax = (gridmap ? gridmap->getHeightPixels() : 0);
	int vPos = (zeroPos ? 0 : GetVertScrollPos());
	SCROLLINFO infoVert = {
		sizeof(SCROLLINFO), SIF_ALL, 0, vMax,
		(UINT) rw.bottom, vPos, 0
	};
	SetScrollInfo(hMainWnd, SB_VERT, &infoVert, TRUE);
}

void HorzScrollHandler(WPARAM wParam)
{
	// Get scrollbar info
	SCROLLINFO info = {
		sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0
	};
	GetScrollInfo(hMainWnd, SB_HORZ, &info);

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
	SetScrollInfo(hMainWnd, SB_HORZ, &info, true);
	UpdateEntireWindow();
}

void VertScrollHandler(WPARAM wParam)
{
	// Get scrollbar info
	SCROLLINFO info = {
		sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0
	};
	GetScrollInfo(hMainWnd, SB_VERT, &info);

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
	SetScrollInfo(hMainWnd, SB_VERT, &info, true);
	UpdateEntireWindow();
}

void ScrollWheelHandler(WPARAM wParam)
{
	// Get adjustment steps
	int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
	int steps = wheelDelta / ScrollWheelIncrement;

	// Handle zoom-in or out (Ctrl pressed)
	if (wParam & MK_CONTROL) {
		int newSize = GetGridSize() + steps;
		newSize = std::min(newSize, (int) gridmap->getCellSizeMax());
		newSize = std::max(newSize, (int) gridmap->getCellSizeMin());
		ChangeGridSize(newSize);
	}

	// Handle movement of scrollbars
	else {
		// Get scrollbar info
		SCROLLINFO info = {
			sizeof(SCROLLINFO), SIF_ALL, 0, 0, 0, 0, 0
		};
		int scrollBar = wParam & MK_SHIFT ? SB_HORZ : SB_VERT;
		GetScrollInfo(hMainWnd, scrollBar, &info);

		// Set new scrollbar position
		info.nPos -= steps * GetGridSize();
		SetScrollInfo(hMainWnd, scrollBar, &info, true);
		UpdateEntireWindow();
	}
}

unsigned GetHorzScrollPos()
{
	SCROLLINFO info = {
		sizeof(SCROLLINFO), SIF_PAGE|SIF_POS, 0, 0, 0, 0, 0
	};
	GetScrollInfo(hMainWnd, SB_HORZ, &info);
	return info.nPos;
}

unsigned GetVertScrollPos()
{
	SCROLLINFO info = {
		sizeof(SCROLLINFO), SIF_PAGE|SIF_POS, 0, 0, 0, 0, 0
	};
	GetScrollInfo(hMainWnd, SB_VERT, &info);
	return info.nPos;
}

void MyKeyHandler(WPARAM wParam)
{
	switch (wParam) {
		case VK_LEFT:
			SendMessage(hMainWnd, WM_HSCROLL, SB_LINELEFT, 0);
			break;
		case VK_RIGHT:
			SendMessage(hMainWnd, WM_HSCROLL, SB_LINERIGHT, 0);
			break;
		case VK_UP:
			SendMessage(hMainWnd, WM_VSCROLL, SB_LINEUP, 0);
			break;
		case VK_DOWN:
			SendMessage(hMainWnd, WM_VSCROLL, SB_LINEDOWN, 0);
			break;
		case VK_PRIOR:
			SendMessage(hMainWnd, WM_VSCROLL, SB_PAGEUP, 0);
			break;
		case VK_NEXT:
			SendMessage(hMainWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			break;
		case VK_HOME:
			SendMessage(hMainWnd, WM_HSCROLL, SB_TOP, 0);
			SendMessage(hMainWnd, WM_VSCROLL, SB_TOP, 0);
			break;
		case VK_END:
			SendMessage(hMainWnd, WM_HSCROLL, SB_BOTTOM, 0);
			SendMessage(hMainWnd, WM_VSCROLL, SB_BOTTOM, 0);
			break;
	}
}

void MyPaintWindow()
{
	// Set up paint process
	RECT rw;
	PAINTSTRUCT ps;
	GetClientRect(hMainWnd, &rw);
	HDC hdc = BeginPaint(hMainWnd, &ps);
	SelectObject(hdc, GetStockObject(GRAY_BRUSH));
	SelectObject(hdc, BkgdPen);

	// If map available, blit from memory & paint background bottom-right
	if (gridmap) {
		BitBlt(
		    hdc, 0, 0, rw.right, rw.bottom, BkgdDC,
		    GetHorzScrollPos(), GetVertScrollPos(), SRCCOPY);
		int rightPixel = gridmap->getWidthPixels() - GetHorzScrollPos();
		int bottomPixel = gridmap->getHeightPixels() - GetVertScrollPos();
		Rectangle(hdc, rightPixel, rw.top, rw.right, rw.bottom);
		Rectangle(hdc, rw.left, bottomPixel, rw.right, rw.bottom);
	}
	else {
		Rectangle(hdc, rw.left, rw.top, rw.right, rw.bottom);
	}
	EndPaint(hMainWnd, &ps);
}

void MyLButtonHandler(LPARAM lParam)
{
	// Extract click position
	POINT p = {
		(LONG)(LOWORD(lParam) + GetHorzScrollPos()),
		(LONG)(HIWORD(lParam) + GetVertScrollPos())
	};

	// Handle if we're on map area
	if (p.x < (LONG) gridmap->getWidthPixels() &&
	        p.y < (LONG) gridmap->getHeightPixels()) {

		// Place floors
		FloorType floor = GetFloorTypeFromMenu(selectedFeature);
		if (floor != FLOOR_FAIL) {
			FloorSelect(floor, p);
			return;
		}

		// Place objects
		ObjectType object = GetObjectTypeFromMenu(selectedFeature);
		if (object != OBJECT_FAIL) {
			ObjectSelect(object, p);
			return;
		}

		// Place walls
		WallType wall = GetWallTypeFromMenu(selectedFeature);
		if (wall != WALL_FAIL) {
			WallSelect(wall, p);
			return;
		}
	}
}

GridCoord GetGridCoordFromWindow(POINT p)
{
	int gridSize = GetGridSize();
	return {
		(unsigned)(p.x / gridSize),
		(unsigned)(p.y / gridSize)
	};
}

void FloorSelect(FloorType floor, POINT p)
{
	GridCoord gc = GetGridCoordFromWindow(p);
	if (gridmap->getCellFloor(gc) != floor) {
		gridmap->setCellFloor(gc, floor);
		if (IsFloorFillType(floor)) {
			FillCell(gc);
		}
		else {
			UpdateBkgdCell(gc);
		}
	}
}

void ObjectSelect(ObjectType object, POINT p)
{
	GridCoord gc = GetGridCoordFromWindow(p);
	if (gridmap->getCellFloor(gc) != FLOOR_FILL
	        && gridmap->getCellObject(gc) != object) {
		gridmap->setCellObject(gc, object);
		UpdateBkgdCell(gc);
	}
}

void WallSelect(WallType wall, POINT p)
{
	// Set click sensitivity
	int gridSize = GetGridSize();
	int INC = gridSize / 3;

	// Figure out what cell we're near
	POINT adjusted = {p.x + INC, p.y + INC};
	GridCoord gc = GetGridCoordFromWindow(adjusted);

	// Handle if we're on map area
	if (gc.x < gridmap->getWidthCells()
	        && gc.y < gridmap->getHeightCells()) {

		// Find distance to nearby walls
		int dx = adjusted.x % gridSize;
		int dy = adjusted.y % gridSize;

		// Abort if not far from edge nor close to vertex
		if (dx >= INC*2 && dy >= INC*2) return;
		if (dx < INC*2 && dy < INC*2) return;

		// Change appropriate wall
		if (dx < dy) {
			ChangeWestWall(gc, wall);
		}
		else {
			ChangeNorthWall(gc, wall);
		}
	}
}

void ChangeWestWall(GridCoord gc, int newFeature)
{
	if (gridmap->canBuildWWall(gc)
	        && gridmap->getCellWWall(gc) != newFeature) {
		gridmap->setCellWWall(gc, newFeature);
		UpdateBkgdCell({gc.x-1, gc.y});
		UpdateBkgdCell(gc);
	}
}

void ChangeNorthWall(GridCoord gc, int newFeature)
{
	if (gridmap->canBuildNWall(gc)
	        && gridmap->getCellNWall(gc) != newFeature) {
		gridmap->setCellNWall(gc, newFeature);
		UpdateBkgdCell({gc.x, gc.y-1});
		UpdateBkgdCell(gc);
	}
}

void ClearMap(bool open)
{
	gridmap->clearMap(open ? FLOOR_OPEN : FLOOR_FILL);
	gridmap->paint(BkgdDC);
	UpdateEntireWindow();
	SetSelectedFeature(open ? IDM_FLOOR_FILL : IDM_FLOOR_OPEN);
}

void ToggleGridLines()
{
	gridmap->toggleNoGrid();
	bool hideGrid = gridmap->displayNoGrid();
	CheckMenuItem(
	    GetMenu(hMainWnd), IDM_HIDE_GRID,
	    MF_BYCOMMAND | (hideGrid ? MF_CHECKED : MF_UNCHECKED));
	SetBkgdDC();
	UpdateEntireWindow();
}

void ToggleRoughEdges()
{
	gridmap->toggleRoughEdges();
	bool roughEdges = gridmap->displayRoughEdges();
	CheckMenuItem(
	    GetMenu(hMainWnd), IDM_ROUGH_EDGES,
	    MF_BYCOMMAND | (roughEdges ? MF_CHECKED : MF_UNCHECKED));
	SetBkgdDC();
	UpdateEntireWindow();
}

/*
	Perform a space-filling operation.
	The new cell floor should be set before calling this function.
	Now we need to wipe out any object, wipe ineligible adjacent walls,
	and repaint all adjacent cells.
*/
void FillCell(GridCoord gc)
{
	// Get height & width
	unsigned width = gridmap->getWidthCells();
	unsigned height = gridmap->getHeightCells();

	// Clear elements
	if (gridmap->getCellFloor(gc) == FLOOR_FILL)
		gridmap->setCellObject(gc, OBJECT_NONE);
	if (!gridmap->canBuildWWall(gc))
		gridmap->setCellWWall(gc, WALL_OPEN);
	if (!gridmap->canBuildNWall(gc))
		gridmap->setCellNWall(gc, WALL_OPEN);
	if (gc.x+1 < width && !gridmap->canBuildWWall({gc.x+1, gc.y}))
		gridmap->setCellWWall({gc.x+1, gc.y}, WALL_OPEN);
	if (gc.y+1 < height && !gridmap->canBuildNWall({gc.x, gc.y+1}))
		gridmap->setCellNWall({gc.x, gc.y+1}, WALL_OPEN);

	// Repaint prior cells
	if (gc.x > 0)
		gridmap->paintCell({gc.x-1, gc.y}, true);
	if (gc.y > 0)
		gridmap->paintCell({gc.x, gc.y-1}, true);

	// Paint this cell
	gridmap->paintCell(gc, true);

	// Repaint later cells
	if (gc.x+1 < width)
		gridmap->paintCell({gc.x+1, gc.y}, true);
	if (gc.y+1 < height)
		gridmap->paintCell({gc.x, gc.y+1}, true);

	// Update window
	UpdateEntireWindow();
}

// Map a menu item to a grid map floor feature
FloorType GetFloorTypeFromMenu(int menuID)
{
	switch (menuID) {
		case IDM_FLOOR_FILL:
			return FLOOR_FILL;
		case IDM_FLOOR_OPEN:
			return FLOOR_OPEN;
		case IDM_FLOOR_NSTAIRS:
			return FLOOR_NSTAIRS;
		case IDM_FLOOR_WSTAIRS:
			return FLOOR_WSTAIRS;
		case IDM_FLOOR_NEWALL:
			return FLOOR_NEWALL;
		case IDM_FLOOR_NWWALL:
			return FLOOR_NWWALL;
		case IDM_FLOOR_NEDOOR:
			return FLOOR_NEDOOR;
		case IDM_FLOOR_NWDOOR:
			return FLOOR_NWDOOR;
		case IDM_FLOOR_NWFILL:
			return FLOOR_NWFILL;
		case IDM_FLOOR_NEFILL:
			return FLOOR_NEFILL;
		case IDM_FLOOR_SWFILL:
			return FLOOR_SWFILL;
		case IDM_FLOOR_SEFILL:
			return FLOOR_SEFILL;
		case IDM_FLOOR_SPIRALSTAIRS:
			return FLOOR_SPIRALSTAIRS;
		case IDM_FLOOR_WATER:
			return FLOOR_WATER;
		default:
			return FLOOR_FAIL;
	}
}

// Map a menu item to a grid map wall feature
WallType GetWallTypeFromMenu(int menuID)
{
	switch (menuID) {
		case IDM_WALL_OPEN:
			return WALL_OPEN;
		case IDM_WALL_FILL:
			return WALL_FILL;
		case IDM_WALL_SINGLE_DOOR:
			return WALL_SINGLE_DOOR;
		case IDM_WALL_DOUBLE_DOOR:
			return WALL_DOUBLE_DOOR;
		case IDM_WALL_SECRET_DOOR:
			return WALL_SECRET_DOOR;
		default:
			return WALL_FAIL;
	}
}

// Map a menu item to a grid map object feature
ObjectType GetObjectTypeFromMenu(int menuID)
{
	switch (menuID) {
		case IDM_OBJECT_CLEAR:
			return OBJECT_NONE;
		case IDM_OBJECT_PILLAR:
			return OBJECT_PILLAR;
		case IDM_OBJECT_STATUE:
			return OBJECT_STATUE;
		case IDM_OBJECT_RUBBLE:
			return OBJECT_RUBBLE;
		case IDM_OBJECT_TRAPDOOR:
			return OBJECT_TRAPDOOR;
		case IDM_OBJECT_PIT:
			return OBJECT_PIT;
		case IDM_OBJECT_STALAGMITE:
			return OBJECT_STALAGMITE;
		case IDM_OBJECT_XMARK:
			return OBJECT_XMARK;
		default:
			return OBJECT_FAIL;
	}
}

//-----------------------------------------------------------------------------
// Menu actions
//-----------------------------------------------------------------------------

// Set & clear radio buttons in tools menu & submenus
void SetSelectedFeature(int feature)
{
	selectedFeature = feature;
	HMENU hMenu = GetMenu(hMainWnd);

	// Main menu
	CheckMenuRadioItem(
	    hMenu, START_BASIC_FLOOR_TOOLS, END_BASIC_WALL_TOOLS,
	    feature, MF_BYCOMMAND);

	// Diagonals submenu
	HMENU hDiagMenu = GetSubMenu(GetSubMenu(hMenu, 1), 13);
	CheckMenuRadioItem(
	    hDiagMenu, START_DIAGONAL_TOOLS, END_DIAGONAL_TOOLS,
	    feature, MF_BYCOMMAND);

	// Objects submenu
	HMENU hObjectsMenu = GetSubMenu(GetSubMenu(hMenu, 1), 14);
	CheckMenuRadioItem(
	    hObjectsMenu, START_OBJECT_TOOLS, END_OBJECT_TOOLS,
	    feature, MF_BYCOMMAND);
}

bool OkDiscardChanges()
{
	if (!gridmap->isChanged())
		return true;
	int retval =
	    MessageBox(
	        hMainWnd, "Okay to discard changes made to map?",
	        "Discard Changes", MB_OKCANCEL | MB_ICONWARNING);
	return (retval == IDOK);
}

void SetBkgdDC()
{
	DeleteObject(BkgdDC);
	DeleteObject(BkgdBitmap);
	BkgdDC = CreateCompatibleDC(GetDC(hMainWnd));
	BkgdBitmap =
	    CreateCompatibleBitmap(
	        GetDC(hMainWnd),
	        gridmap->getWidthPixels(),
	        gridmap->getHeightPixels());
	if (BkgdBitmap) {
		SelectObject(BkgdDC, BkgdBitmap);
		gridmap->paint(BkgdDC);
	}
	else {
		MessageBox(
		    hMainWnd, "Could not create large enough bitmap."
		    "\nMap will not display; try smaller grid size?",
		    "Map Too Large", MB_OK|MB_ICONERROR);
	}
}

void ChangeGridSize(int size)
{
	gridmap->setCellSizePixels(size);
	SetBkgdDC();
	SetScrollRange(true);
	UpdateEntireWindow();
}

void SetNewMap(GridMap *newmap)
{
	if (gridmap) {
		delete gridmap;
	}
	gridmap = newmap;
	SetBkgdDC();
	SetScrollRange(true);
	UpdateEntireWindow();

	// Set menu selections
	HMENU hMenu = GetMenu(hMainWnd);
	SetSelectedFeature(IDM_FLOOR_OPEN);
	CheckMenuItem(hMenu, IDM_HIDE_GRID, MF_BYCOMMAND |
	              (gridmap->displayNoGrid() ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(hMenu, IDM_ROUGH_EDGES, MF_BYCOMMAND |
	              (gridmap->displayRoughEdges() ? MF_CHECKED : MF_UNCHECKED));
}

bool NewMapFromSpecs(int newWidth, int newHeight)
{
	GridMap *newmap = new GridMap(newWidth, newHeight);
	if (!newmap) {
		MessageBox(
		    hMainWnd, "Could not create new map.", "Error",
		    MB_OK|MB_ICONERROR);
		return false;
	}
	else {
		SetNewMap(newmap);
		return true;
	}
}

bool NewMapFromFile(char *filename)
{
	GridMap *newmap = new GridMap(filename);
	if (!newmap) {
		MessageBox(
		    hMainWnd, "Could not create new map.", "Error",
		    MB_OK|MB_ICONERROR);
		return false;
	}
	else if (!newmap->isFileLoadOk()) {
		char msg[512];
		sprintf_s(
		    msg, sizeof(msg), "Could not read map file:\n%s", filename);
		MessageBox(
		    hMainWnd, msg, "Error", MB_OK|MB_ICONERROR);
		delete newmap;
		return false;
	}
	else {
		SetNewMap(newmap);
		return true;
	}
}

void OpenMap()
{
	char filename[GRID_FILENAME_MAX] = "\0";
	OPENFILENAME info = {
		sizeof(OPENFILENAME), hMainWnd, 0,
		FileFilterStr, 0, 0, 0, filename, GRID_FILENAME_MAX,
		0, 0, 0, 0, 0, 0, 0, DefaultFileExt, 0, 0, 0
	};
	if (GetOpenFileName(&info))
		NewMapFromFile(filename);
}

void SaveMapAs()
{
	char filename[GRID_FILENAME_MAX] = "\0";
	OPENFILENAME info = {
		sizeof(OPENFILENAME), hMainWnd, 0,
		FileFilterStr, 0, 0, 0, filename, GRID_FILENAME_MAX,
		0, 0, 0, 0, OFN_OVERWRITEPROMPT, 0, 0, DefaultFileExt,
		0, 0, 0
	};
	if (GetSaveFileName(&info)) {
		gridmap->setFilename(filename);
		SaveMap();
	}
}

void SaveMap()
{
	if (strlen(gridmap->getFilename()))
		gridmap->save();
	else
		SaveMapAs();
}

void CopyMap()
{
	// Create bitmap with map image
	HDC tempDC = CreateCompatibleDC(GetDC(hMainWnd));
	HBITMAP hBitmap =
	    CreateCompatibleBitmap(
	        GetDC(hMainWnd),
	        gridmap->getWidthPixels(), gridmap->getHeightPixels());
	SelectObject(tempDC, hBitmap);
	BitBlt(
	    tempDC, 0, 0, gridmap->getWidthPixels(),
	    gridmap->getHeightPixels(), BkgdDC, 0, 0, SRCCOPY);

	// Put it on the clipboard
	OpenClipboard(hMainWnd);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP, hBitmap);
	CloseClipboard();

	// Delete temporary DC
	DeleteDC(tempDC);
}

void PrintMap()
{
	// Call print dialog
	PRINTDLG pd = {
		sizeof(PRINTDLG), hMainWnd, 0, 0, 0,
		PD_RETURNDC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	if (PrintDlg(&pd)) {

		// Calculate print width in pixels
		int printWidth =
		    gridmap->getWidthPixels()
		    * GetDeviceCaps(pd.hDC, LOGPIXELSX)
		    / DefaultPrintSquaresPerInch
		    / gridmap->getCellSizeDefault();

		// Calculate print height in pixels
		int printHeight =
		    gridmap->getHeightPixels()
		    * GetDeviceCaps(pd.hDC, LOGPIXELSY)
		    / DefaultPrintSquaresPerInch
		    / gridmap->getCellSizeDefault();

		// Create the print job
		DOCINFO di = {sizeof(DOCINFO), "GridMapper Document", 0, 0, 0};
		StartDoc(pd.hDC, &di);
		StartPage(pd.hDC);
		StretchBlt(
		    pd.hDC, 0, 0, printWidth, printHeight,
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
LRESULT CALLBACK NewDialog(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_INITDIALOG:
			SetDlgItemInt(hDlg, IDC_NEW_WIDTH, DefaultMapWidth, FALSE);
			SetDlgItemInt(hDlg, IDC_NEW_HEIGHT, DefaultMapHeight, FALSE);
			return TRUE;

		case WM_COMMAND:
			int retval = LOWORD(wParam);
			if (retval == IDOK) {
				NewMapFromSpecs(
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
LRESULT CALLBACK GridSizeDialog(
    HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {

		case WM_INITDIALOG:
			SetDlgItemInt(
			    hDlg, IDC_PXLS_PER_SQUARE,
			    gridmap->getCellSizePixels(), FALSE);
			return TRUE;

		case WM_COMMAND:
			int retval = LOWORD(wParam);
			if (retval == IDOK) {
				int newSize =
				    GetDlgItemInt(
				        hDlg, IDC_PXLS_PER_SQUARE, NULL,
				        FALSE);

				// Check size too small
				if (newSize < (int) gridmap->getCellSizeMin()) {
					std::stringstream message;
					message << "Minimum grid size is "
					        << gridmap->getCellSizeMin()
					        << " pixels per square.";
					MessageBox(
					    hDlg, message.str().c_str(),
					    "Size Too Small", MB_OK|MB_ICONWARNING);
				}

				// Check size too large
				else if (newSize > (int) gridmap->getCellSizeMax()) {
					std::stringstream message;
					message << "Maximum grid size is "
					        << gridmap->getCellSizeMax()
					        << " pixels per square.";
					MessageBox(
					    hDlg, message.str().c_str(),
					    "Size Too Large", MB_OK|MB_ICONWARNING);
				}

				// Handle acceptable size
				else {
					ChangeGridSize(newSize);
					EndDialog(hDlg, LOWORD(wParam));
				}
				return TRUE;
			}
			else if (retval == IDCANCEL) {
				EndDialog(hDlg, retval);
				return TRUE;
			}
			else if (retval == IDC_DEFAULT) {
				SetDlgItemInt(
				    hDlg, IDC_PXLS_PER_SQUARE,
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
