/*
	Name: GridMap.cpp
	Copyright: 2010
	Author: Daniel R. Collins
	Date: 16-05-10
	Description: Implementation of the GridMap class.
		See file LICENSE for licensing information.
		Contact author at delta@superdan.net
*/
#include "GridMap.h"
#include <stdio.h>
#include <algorithm>
using std::min;
using std::max;

// Constants
const int DEFAULT_CELL_SIZE = 20;
const int STAIRS_PER_SQUARE = 5;
const int WATER_LINES_PER_EDGE = 4;
const float LETTER_S_HEIGHT = 0.70;
const float SQRT2_2 = 0.70710678f;

//------------------------------------------------------------------
// Constructor/ Destructors
//------------------------------------------------------------------

// Constructor taking dimensions
GridMap::GridMap(int _width, int _height)
{
	width = _width;
	height = _height;
	cellSize = DEFAULT_CELL_SIZE;
	grid = new GridCell*[width];
	for (int x = 0; x < width; x++)
		grid[x] = new GridCell[height];
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			grid[x][y].floor = FLOOR_FILL;
			grid[x][y].nwall = WALL_OPEN;
			grid[x][y].wwall = WALL_OPEN;
			grid[x][y].object = 0;
		}
	}
	filename[0] = '\0';
	changed = false;
	fileLoadOk = true;
}

// Destructor
GridMap::~GridMap()
{
	for (int x = 0; x < width; x++)
		delete [] grid[x];
	delete [] grid;
}

// Constructor taking filename
GridMap::GridMap(char *_filename)
{
	// Declarations
	FILE *f;
	char header[4];
	int x;

	// Open file
	if (!(f = fopen(_filename, "rb"))) goto fail;

	// Check header
	fread(header, sizeof(char), 4, f);
	if (strncmp(header, "GM", 2)) {
		goto fail;
	}

	// Read & create other stuff
	fread(&cellSize, sizeof(int), 1, f);
	fread(&width, sizeof(int), 1, f);
	fread(&height, sizeof(int), 1, f);
	grid = new GridCell*[width];
	for (x = 0; x < width; x++) {
		grid[x] = new GridCell[height];
	}
	for (x = 0; x < width; x++) {
		fread(grid[x], sizeof(GridCell), height, f);
	}

	// Clean up
	fclose(f);
	changed = false;
	fileLoadOk = true;
	setFilename(_filename);
	return;

fail:
	grid = NULL;
	width = height = cellSize = 0;
	filename[0] = '\0';
	fileLoadOk = false;
}

// Save to previously stored filename
int GridMap::save()
{
	// Open file
	FILE *f = fopen(filename, "wb");
	if (!f) {
		return 0;
	}

	// Save stuff
	char header[] = "GM\1\0";
	fwrite(header, sizeof(char), 4, f);
	fwrite(&cellSize, sizeof(int), 1, f);
	fwrite(&width, sizeof(int), 1, f);
	fwrite(&height, sizeof(int), 1, f);
	for (int x = 0; x < width; x++) {
		fwrite(grid[x], sizeof(GridCell), height, f);
	}

	// Clean up
	fclose(f);
	changed = false;
	return 1;
}

//------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------

int GridMap::getWidthCells()
{
	return width;
}

int GridMap::getHeightCells()
{
	return height;
}

int GridMap::getWidthPixels()
{
	return width * cellSize;
}

int GridMap::getHeightPixels()
{
	return height * cellSize;
}

int GridMap::getCellSizePixels()
{
	return cellSize;
}

int GridMap::getCellSizeDefault()
{
	return DEFAULT_CELL_SIZE;
}

int GridMap::getCellFloor(int x, int y)
{
	return grid[x][y].floor;
}

int GridMap::getCellNWall(int x, int y)
{
	return grid[x][y].nwall;
}

int GridMap::getCellWWall(int x, int y)
{
	return grid[x][y].wwall;
}

int GridMap::getCellObject(int x, int y)
{
	return grid[x][y].object;
}

bool GridMap::isChanged()
{
	return changed;
}

bool GridMap::isFileLoadOk()
{
	return fileLoadOk;
}

char* GridMap::getFilename()
{
	return filename;
}

/*
	Can we erect a wall partition to the north of a given cell?
	Prohibited if spaced filled on either side.
*/
bool GridMap::canBuildNWall(int x, int y)
{
	if (y == 0) {            // on top border
		return false;
	}
	switch (getCellFloor(x, y-1)) {
		case FLOOR_FILL:
		case FLOOR_SWFILL:
		case FLOOR_SEFILL:
			return false;
	}
	switch (getCellFloor(x, y)) {
		case FLOOR_FILL:
		case FLOOR_NWFILL:
		case FLOOR_NEFILL:
			return false;
	}
	return true;
}

/*
	Can we erect a wall partition to the west of a given cell?
	Prohibited if spaced filled on either side.
*/
bool GridMap::canBuildWWall(int x, int y)
{
	if (x == 0) {            // on left border
		return false;
	}
	switch (getCellFloor(x-1, y)) {
		case FLOOR_FILL:
		case FLOOR_NEFILL:
		case FLOOR_SEFILL:
			return false;
	}
	switch (getCellFloor(x, y)) {
		case FLOOR_FILL:
		case FLOOR_NWFILL:
		case FLOOR_SWFILL:
			return false;
	}
	return true;
}

//------------------------------------------------------------------
// Mutators
//------------------------------------------------------------------

void GridMap::setCellSizePixels(int _cellSize)
{
	cellSize = _cellSize;
}

void GridMap::setCellFloor(int x, int y, int floor)
{
	grid[x][y].floor = floor;
	changed = true;
}

void GridMap::setCellNWall(int x, int y, int wall)
{
	grid[x][y].nwall = wall;
	changed = true;
}

void GridMap::setCellWWall(int x, int y, int wall)
{
	grid[x][y].wwall = wall;
	changed = true;
}

void GridMap::setCellObject(int x, int y, int object)
{
	grid[x][y].object = object;
	changed = true;
}

void GridMap::setFilename(char *name)
{
	strncpy(filename, name, GRID_FILENAME_MAX);
}

// Clear the entire map
void GridMap::clearMap(int _floor)
{
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			grid[x][y].floor = _floor;
			grid[x][y].wwall = WALL_OPEN;
			grid[x][y].nwall = WALL_OPEN;
			grid[x][y].object = OBJECT_NONE;
		}
	}
	changed = true;
}

//------------------------------------------------------------------
// Drawing code
//------------------------------------------------------------------

// Globals
HPEN ThinGrayPen = NULL;
HPEN ThickBlackPen = NULL;

// Paint entire map on device context
void GridMap::paint(HDC hDC)
{
	// Create pens if needed
	if (!ThickBlackPen) {
		ThickBlackPen = CreatePen(PS_SOLID, 3, 0x00000000);
	}
	if (!ThinGrayPen) {
		ThinGrayPen = CreatePen(PS_SOLID, 1, 0x00808080);
	}

	// Draw each grid cell
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			paintCell(hDC, x, y, false);
		}
	}
}

// Paint one cell on device context
void GridMap::paintCell(HDC hDC, int x, int y, bool allWalls)
{
	// Paint everything controlled by this cell
	int xPos = x * cellSize;
	int yPos = y * cellSize;
	paintCellFloor(hDC, xPos, yPos, grid[x][y]);
	paintCellObject(hDC, xPos, yPos, grid[x][y]);
	paintCellNWall(hDC, xPos, yPos, grid[x][y]);
	paintCellWWall(hDC, xPos, yPos, grid[x][y]);

	// Paint other adjacent walls if requested (partial repaint)
	if (allWalls) {
		if (x+1 < width) {
			paintCellWWall(hDC, xPos + cellSize, yPos, grid[x+1][y]);
		}
		if (y+1 < height) {
			paintCellNWall(hDC, xPos, yPos + cellSize, grid[x][y+1]);
		}
	}
}

// Paint one cell's floor
void GridMap::paintCellFloor(HDC hDC, int x, int y, GridCell cell)
{
	// Pick color depending on filled or other
	if (cell.floor == FLOOR_FILL) {
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	}
	else {
		SelectObject(hDC, GetStockObject(WHITE_PEN));
		SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	}
	Rectangle(hDC, x, y, x + cellSize, y + cellSize);

	// Set pen for other features
	SelectObject(hDC, GetStockObject(BLACK_PEN));

	// Stairs N/S
	if (cell.floor == FLOOR_NSTAIRS) {
		int h = cellSize / STAIRS_PER_SQUARE;
		for (int dy = 0; dy < cellSize; dy+=h) {
			MoveToEx(hDC, x, y + dy, NULL);
			LineTo(hDC, x + cellSize, y + dy);
		}
	}

	// Stairs E/W
	if (cell.floor == FLOOR_WSTAIRS) {
		int h = cellSize / STAIRS_PER_SQUARE;
		for (int dx = 0; dx < cellSize; dx+=h) {
			MoveToEx(hDC, x + dx, y, NULL);
			LineTo(hDC, x + dx, y + cellSize);
		}
	}

	// Diagonal Wall NW/SE
	if (cell.floor == FLOOR_NWWALL || cell.floor == FLOOR_NWDOOR) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x + cellSize, y + cellSize);
	}

	// Diagonal Wall NE/SW
	if (cell.floor == FLOOR_NEWALL || cell.floor == FLOOR_NEDOOR) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, x + cellSize, y, NULL);
		LineTo(hDC, x, y + cellSize);
	}

	// Diagonal Door (in either direction)
	if (cell.floor == FLOOR_NEDOOR || cell.floor == FLOOR_NWDOOR) {
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectObject(hDC, GetStockObject(WHITE_BRUSH));

		// Find cell center & door corners
		int halfCell = cellSize / 2;
		int cx = x + halfCell;
		int cy = y + halfCell;
		int offset = (int)(halfCell * SQRT2_2);

		// Set four corners of door & draw polygon
		POINT pts[4] = {
			{cx - offset, cy},
			{cx, cy + offset},
			{cx + offset, cy},
			{cx, cy - offset}
		};
		Polygon(hDC, pts, 4);
	}

	// Diagonal half-fill
	if (FLOOR_NWFILL <= cell.floor && cell.floor <= FLOOR_SEFILL) {

		POINT triangle[3];

		switch (cell.floor) {
			case FLOOR_NWFILL:
				triangle[0] = { x, y };
				triangle[1] = { x + cellSize, y };
				triangle[2] = { x, y + cellSize };
				break;
			case FLOOR_NEFILL:
				triangle[0] = { x + cellSize, y };
				triangle[1] = { x + cellSize, y + cellSize };
				triangle[2] = { x, y };
				break;
			case FLOOR_SWFILL:
				triangle[0] = { x, y + cellSize };
				triangle[1] = { x, y };
				triangle[2] = { x + cellSize, y + cellSize };
				break;
			case FLOOR_SEFILL:
				triangle[0] = { x + cellSize, y + cellSize };
				triangle[1] = { x, y + cellSize };
				triangle[2] = { x + cellSize, y };
				break;
		}

		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Polygon(hDC, triangle, 3);
	}
}

// Paint one cell's north wall
void GridMap::paintCellNWall(HDC hDC, int x, int y, GridCell cell)
{
	// Paint base clear or filled
	SelectObject(hDC, cell.nwall ? ThickBlackPen : ThinGrayPen);
	MoveToEx(hDC, x, y, NULL);
	LineTo(hDC, x + cellSize, y);

	// Set door size, pen, brush
	int h = cellSize / 4; // half door size
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));

	// Single door
	if (cell.nwall == WALL_SINGLE_DOOR) {
		Rectangle(hDC, x+h+1, y-h+1, x+3*h, y+h);
	}

	// Double door
	if (cell.nwall == WALL_DOUBLE_DOOR) {
		Rectangle(hDC, x+2, y-h+1, x+2*h+1, y+h);
		Rectangle(hDC, x+2*h, y-h+1, x+4*h-1, y+h);
	}

	// Secret door
	if (cell.nwall == WALL_SECRET_DOOR) {
		LetterS(hDC, x+h*2, y+1);
	}
}

// Paint one cell's west wall
void GridMap::paintCellWWall(HDC hDC, int x, int y, GridCell cell)
{
	// Paint base clear or filled
	SelectObject(hDC, cell.wwall ? ThickBlackPen : ThinGrayPen);
	MoveToEx(hDC, x, y, NULL);
	LineTo(hDC, x, y + cellSize);

	// Set door size, pen, brush
	int h = cellSize / 4; // half door size
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));

	// Single door
	if (cell.wwall == WALL_SINGLE_DOOR) {
		Rectangle(hDC, x-h+1, y+h+1, x+h, y+3*h);
	}

	// Double door
	if (cell.wwall == WALL_DOUBLE_DOOR) {
		Rectangle(hDC, x-h+1, y+2, x+h, y+2*h+1);
		Rectangle(hDC, x-h+1, y+2*h, x+h, y+4*h-1);
	}

	// Secret door
	if (cell.wwall == WALL_SECRET_DOOR) {
		LetterS(hDC, x, y+h*2+1);
	}
}

/*
	Paint letter 'S' centered at (x, y)
	Represents a secret door
	Also paints background behind letter
*/
void GridMap::LetterS(HDC hDC, int x, int y)
{
	int fontHeight = (int) (cellSize * LETTER_S_HEIGHT);

	// Create a font with desired height
	HFONT hFont =
	    CreateFont(
	        -fontHeight,             // Height of font
	        0,                       // Width (0 = default)
	        0, 0,                    // Escapement & Orientation
	        FW_NORMAL,               // Weight
	        FALSE, FALSE, FALSE,     // Italic, Underline, Strikeout
	        ANSI_CHARSET,
	        OUT_TT_PRECIS,
	        CLIP_DEFAULT_PRECIS,
	        DEFAULT_QUALITY,
	        DEFAULT_PITCH | FF_DONTCARE,
	        TEXT("Arial"));

	HFONT hOldFont = (HFONT) SelectObject(hDC, hFont);

	// Measure text size
	SIZE textSize;
	GetTextExtentPoint32(hDC, TEXT("S"), 1, &textSize);

	// Compute top-left of text to center it at (x, y)
	int textX = x - textSize.cx / 2;
	int textY = y - textSize.cy / 2;

	// Draw the letter "S"
	TextOut(hDC, textX, textY, TEXT("S"), 1);

	// Clean up
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);
}

// Paint one cell's object
void GridMap::paintCellObject(HDC hDC, int x, int y, GridCell cell)
{
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	
	// Water texture
	if (cell.object == OBJECT_WATER) {
		int h = cellSize / WATER_LINES_PER_EDGE;

		// Draw lines from top-left to bottom-right
		for (int offset = -cellSize; offset <= cellSize; offset += h) {
			int startX = x + max(0, offset);
			int startY = y + max(0, -offset);
			int endX = x + min(cellSize, cellSize + offset);
			int endY = y + min(cellSize, cellSize - offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}

		// Draw lines from top-right to bottom-left
		for (int offset = 1; offset <= 2 * cellSize; offset += h) {
			int startX = x + min(cellSize, offset);
			int startY = y + max(0, offset - cellSize);
			int endX = x + max(0, offset - cellSize);
			int endY = y + min(cellSize, offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}
	}
}

//------------------------------------------------------------------
// Feature info function(s)
//------------------------------------------------------------------

// Is this floor type space-filling?
bool IsFloorFillType(FloorType floor)
{
	switch (floor) {
		case FLOOR_FILL:
		case FLOOR_NEFILL:
		case FLOOR_NWFILL:
		case FLOOR_SEFILL:
		case FLOOR_SWFILL:
			return true;
	}
	return false;
}
