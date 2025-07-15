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
#include <cstdlib>
#include <cassert>
#include <cmath>
using std::min;
using std::max;

// Constants
const float TAU = 6.283185307f;
const float SQRT2_2 = 0.70710678f;

//------------------------------------------------------------------
// Feature info function(s)
//------------------------------------------------------------------

// Is this floor type space-filling?
bool IsFloorFillType(FloorType floor)
{
	return floor == FLOOR_FILL || IsFloorDiagonalFill(floor);
}

// Is the floor type open all around?
bool IsFloorOpenType(FloorType floor)
{
	switch (floor) {
		case FLOOR_OPEN:
		case FLOOR_NSTAIRS:
		case FLOOR_WSTAIRS:
		case FLOOR_NEWALL:
		case FLOOR_NWWALL:
		case FLOOR_NEDOOR:
		case FLOOR_NWDOOR:
		case FLOOR_SPIRALSTAIRS:
		case FLOOR_WATER:
			return true;
	}
	return false;
}

// Is this floor type one of the diagonal fills?
bool IsFloorDiagonalFill(FloorType floor)
{
	return FLOOR_NWFILL <= floor && floor <= FLOOR_SEFILL;
}

//------------------------------------------------------------------
// Display code handlers
//------------------------------------------------------------------

/*
	Field displayCode (unsigned int) used thus:
	- Bits 0-9: Cell size in pixels (so, max 1023)
	- Bits 10-29: Reserved
	- Bit 30: Rough edges
	- Bit 31: Hide grid
*/

// Constant bitmasks
const unsigned int MASK_CELL_SIZE = (1u << 10) - 1;
const unsigned int MASK_ROUGH_EDGES = 1u << 30;
const unsigned int MASK_HIDE_GRID = 1u << 31;

// Get cell size minimum
int GridMap::getCellSizeMin()
{
	return 12;
}

// Get cell size default
int GridMap::getCellSizeDefault()
{
	return 20;
}

// Get cell size maximum
int GridMap::getCellSizeMax()
{
	return (int) MASK_CELL_SIZE;
}

// Get current cell size
int GridMap::getCellSizePixels() const
{
	return (int)(displayCode & MASK_CELL_SIZE);
}

// Set current cell size
void GridMap::setCellSizePixels(int cellSize)
{
	assert(cellSize >= getCellSizeMin());
	assert(cellSize <= getCellSizeMax());
	displayCode = (displayCode & ~MASK_CELL_SIZE)
	              | (cellSize & MASK_CELL_SIZE);
}

// Do we want to see rough edges?
bool GridMap::displayRoughEdges() const
{
	return (bool)(displayCode & MASK_ROUGH_EDGES);
}

// Toggle the rough edges display
void GridMap::toggleRoughEdges()
{
	displayCode ^= MASK_ROUGH_EDGES;
}

// Do we want to hide grid lines?
bool GridMap::displayNoGrid() const
{
	return (bool)(displayCode & MASK_HIDE_GRID);
}

// Toggle the hidden grid lines display
void GridMap::toggleNoGrid()
{
	displayCode ^= MASK_HIDE_GRID;
}

//------------------------------------------------------------------
// Constructor/ Destructors
//------------------------------------------------------------------

// Constructor taking dimensions
GridMap::GridMap(int _width, int _height)
{
	width = _width;
	height = _height;
	displayCode = 0;
	setCellSizePixels(getCellSizeDefault());
	grid = new GridCell*[width];
	for (int x = 0; x < width; x++)
		grid[x] = new GridCell[height];
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			grid[x][y].floor = FLOOR_FILL;
			grid[x][y].nwall = WALL_OPEN;
			grid[x][y].wwall = WALL_OPEN;
			grid[x][y].object = OBJECT_NONE;
		}
	}
	filename[0] = '\0';
	changed = false;
	fileLoadOk = true;
	makeStandardPens();
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
	fread(&displayCode, sizeof(int), 1, f);
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
	makeStandardPens();
	return;

fail:
	grid = NULL;
	width = height = displayCode = 0;
	filename[0] = '\0';
	fileLoadOk = false;
}

// Destructor
GridMap::~GridMap()
{
	DeleteObject(ThinGrayPen);
	DeleteObject(ThickBlackPen);
	for (int x = 0; x < width; x++) {
		delete [] grid[x];
	}
	delete [] grid;
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
	fwrite(&displayCode, sizeof(int), 1, f);
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
	return width * getCellSizePixels();
}

int GridMap::getHeightPixels()
{
	return height * getCellSizePixels();
}

FloorType GridMap::getCellFloor(int x, int y)
{
	return (FloorType) grid[x][y].floor;
}

WallType GridMap::getCellNWall(int x, int y)
{
	return (WallType) grid[x][y].nwall;
}

WallType GridMap::getCellWWall(int x, int y)
{
	return (WallType) grid[x][y].wwall;
}

ObjectType GridMap::getCellObject(int x, int y)
{
	return (ObjectType) grid[x][y].object;
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

// Make standard pens
void GridMap::makeStandardPens()
{
	ThinGrayPen = CreatePen(PS_SOLID, 1, 0x00808080);
	ThickBlackPen = CreatePen(PS_SOLID, 3, 0x00000000);
}

// Hash a coordinate (for use as random seed)
unsigned GridMap::cellHash(int x, int y) const
{
	unsigned int h = x;
	h = h * 31 + y;      // Mix x and y with a prime
	h ^= (h >> 16);      // Bit mixing
	h *= 0x85ebca6b;
	h ^= (h >> 13);
	h *= 0xc2b2ae35;
	h ^= (h >> 16);
	return h;
}

// Paint entire map on device context
void GridMap::paint(HDC hDC)
{
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			paintCell(hDC, x, y, false);
		}
	}
}

/*
	Paint one cell on device context

	NOTE ON RECURSION: 
	Recursion here handles rough edges bleeding into neighbor spaces.
	This isn't perfect for partial repaints,
	but we need some depth limit to avoid infinite recursion.
	Also, the higher the limit, the slower it is to zoom in/out
	on a mostly open map with rough edges.
	(E.g.: Draw a tight snaky tunnel trending north or west
	and you'll see some artifacts.)
*/
void GridMap::paintCell(
    HDC hDC, int x, int y, bool partialRepaint, int depth)
{
	// Handle recursion limit for open redraws
	if (depth > 1 && IsFloorOpenType(getCellFloor(x, y))) {
		return;
	}

	// Seed randomizations for this cell
	srand(cellHash(x, y));

	// Paint everything controlled by this cell
	int cellSize = getCellSizePixels();
	int xPos = x * cellSize;
	int yPos = y * cellSize;
	paintCellFloor(hDC, xPos, yPos, grid[x][y]);
	paintCellObject(hDC, xPos, yPos, grid[x][y]);
	paintCellNWall(hDC, xPos, yPos, grid[x][y]);
	paintCellWWall(hDC, xPos, yPos, grid[x][y]);

	// Repaint walls east & south if needed
	if (partialRepaint) {
		if (x+1 < width) {
			paintCellWWall(hDC, xPos + cellSize, yPos, grid[x+1][y]);
		}
		if (y+1 < height) {
			paintCellNWall(hDC, xPos, yPos + cellSize, grid[x][y+1]);
		}
	}

	// Repaint neighbors in case of bleeding rough edges
	if (displayRoughEdges() && IsFloorOpenType(getCellFloor(x, y))) {
		if (x > 0) {
			paintCell(hDC, x - 1, y, true, depth + 1);
		}
		if (y > 0) {
			paintCell(hDC, x, y - 1, true, depth + 1);
		}
		if (x+1 < width) {
			paintCell(hDC, x + 1, y, true, depth + 1);
		}
		if (y+1 < height) {
			paintCell(hDC, x, y + 1, true, depth + 1);
		}
	}
}

// Paint one cell's floor
void GridMap::paintCellFloor(HDC hDC, int x, int y, GridCell cell)
{
	int cellSize = getCellSizePixels();

	// If we're a filled cell with no rough edges,
	// then simply paint a black rectangle and return
	if (cell.floor == FLOOR_FILL && !displayRoughEdges()) {
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Rectangle(hDC, x, y, x + cellSize, y + cellSize);
		return;
	}

	// Paint a white rectangle as background
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	Rectangle(hDC, x, y, x + cellSize, y + cellSize);

	// Set pen for other features
	SelectObject(hDC, GetStockObject(BLACK_PEN));

	// Draw a filled space with rough edges
	if (cell.floor == FLOOR_FILL) {
		assert(displayRoughEdges());
		drawFillSpaceFractal(hDC, x, y);
	}

	// Stairs (series of parallel lines)
	if (cell.floor == FLOOR_NSTAIRS || cell.floor == FLOOR_WSTAIRS) {
		const int stairsPerSquare = 5;
		for (int s = 0; s <= stairsPerSquare; s++) {
			int d = s * cellSize / stairsPerSquare;
			if (cell.floor == FLOOR_NSTAIRS) {
				MoveToEx(hDC, x, y + d, NULL);
				LineTo(hDC, x + cellSize, y + d);
			}
			else {
				MoveToEx(hDC, x + d, y, NULL);
				LineTo(hDC, x + d, y + cellSize);
			}
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

	// Diagonal Door (diamond in square center)
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

	// Diagonal half-filled space
	if (IsFloorDiagonalFill((FloorType) cell.floor)) {
		if (displayRoughEdges()) {
			drawDiagonalFillFractal(hDC, x, y, (FloorType) cell.floor);
		}
		else {
			drawDiagonalFillSmooth(hDC, x, y, (FloorType) cell.floor);
		}
	}

	// Spiral stairs (arc, circle, and spokes)
	if (cell.floor == FLOOR_SPIRALSTAIRS) {

		// Find parameters
		int halfCell = cellSize / 2;
		int radius = halfCell;
		int cx = x + halfCell;
		int cy = y + halfCell;

		// Arc parameters (open at top)
		double arcStartAngle = TAU/4 + 0.5;
		double arcEndAngle   = TAU/4 - 0.5;

		// Calculate bounding rect of the circle
		int left   = cx - radius;
		int top    = cy - radius;
		int right  = cx + radius;
		int bottom = cy + radius;

		// Calculate arc start/end points
		int xStart = cx + (int)(radius * cos(arcStartAngle));
		int yStart = cy - (int)(radius * sin(arcStartAngle));
		int xEnd   = cx + (int)(radius * cos(arcEndAngle));
		int yEnd   = cy - (int)(radius * sin(arcEndAngle));

		// Draw main circle with arc missing
		Arc(hDC, left, top, right, bottom, xStart, yStart, xEnd, yEnd);

		// Draw center circle (percent of outer radius)
		int innerRadius = (int)(radius * 0.20);
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Ellipse(hDC,
		        cx - innerRadius, cy - innerRadius,
		        cx + innerRadius, cy + innerRadius);

		// Compute spoke dimensions
		const int numSpokes = 12;
		double arcSweepAngle = TAU - (arcStartAngle - arcEndAngle);
		double anglePerSpoke = arcSweepAngle / numSpokes;

		// Draw the spokes
		for (int i = 0; i <= numSpokes; ++i) {
			double angle = arcStartAngle + anglePerSpoke * i;
			int xOuter = cx + (int)(radius * cos(angle));
			int yOuter = cy - (int)(radius * sin(angle));
			MoveToEx(hDC, cx, cy, nullptr);
			LineTo(hDC, xOuter, yOuter);
		}
	}

	// Water texture (diagonal hatched lines)
	if (cell.floor == FLOOR_WATER) {

		// Set line increment
		const int linesPerEdge = 4;
		int inc = cellSize / linesPerEdge;

		// Draw lines from top-left to bottom-right
		for (int offset = -cellSize; offset <= cellSize; offset += inc) {
			int startX = x + max(0, offset);
			int startY = y + max(0, -offset);
			int endX = x + min(cellSize, cellSize + offset);
			int endY = y + min(cellSize, cellSize - offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}

		// Draw lines from top-right to bottom-left
		for (int offset = 1; offset <= 2 * cellSize; offset += inc) {
			int startX = x + min(cellSize, offset);
			int startY = y + max(0, offset - cellSize);
			int endX = x + max(0, offset - cellSize);
			int endY = y + min(cellSize, offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}
	}
}

// Paint one cell's north wall
void GridMap::paintCellNWall(HDC hDC, int x, int y, GridCell cell)
{
	int cellSize = getCellSizePixels();

	// Paint grid line as needed
	if (cell.nwall) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x + cellSize, y);
	}
	else if (!displayNoGrid()) {
		SelectObject(hDC, ThinGrayPen);
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x + cellSize, y);
	}

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
	int cellSize = getCellSizePixels();

	// Paint grid line as needed
	if (cell.wwall) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x, y + cellSize);
	}
	else if (!displayNoGrid()) {
		SelectObject(hDC, ThinGrayPen);
		MoveToEx(hDC, x, y, NULL);
		LineTo(hDC, x, y + cellSize);
	}

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
	int fontHeight = (int)(getCellSizePixels() * 0.70);

	// Create a font with desired height
	HFONT hFont =
	    CreateFont(
	        -fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
	        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
	        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial")
	    );
	HFONT hOldFont = (HFONT) SelectObject(hDC, hFont);

	// Measure actual text size
	SIZE textSize;
	GetTextExtentPoint32(hDC, TEXT("S"), 1, &textSize);

	// Compute top-left of text to center it at (x, y)
	int textX = x - textSize.cx / 2;
	int textY = y - textSize.cy / 2;

	// Draw the letter "S"
	SetBkMode(hDC, OPAQUE);
	SetTextAlign(hDC, TA_LEFT | TA_TOP);
	TextOut(hDC, textX, textY, TEXT("S"), 1);

	// Clean up
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);
}

// Paint one cell's object
void GridMap::paintCellObject(HDC hDC, int x, int y, GridCell cell)
{
	int cellSize = getCellSizePixels();
	SelectObject(hDC, GetStockObject(BLACK_PEN));

	// Pillar object (black circle)
	if (cell.object == OBJECT_PILLAR) {
		int halfCell = cellSize / 2;
		int cx = x + halfCell;
		int cy = y + halfCell;
		int radius = (int)(halfCell * 0.50);
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Ellipse(hDC, cx - radius, cy - radius, cx + radius, cy + radius);
	}

	// Statue object (circle with 5-pointed star inside)
	if (cell.object == OBJECT_STATUE) {
		const int NUM_POINTS = 5;

		// Compute center of square and radius of circle
		int halfCell = cellSize / 2;
		int cx = x + halfCell;
		int cy = y + halfCell;
		int radius = (int)(halfCell * 0.70);

		// Draw the circle
		SelectObject(hDC, GetStockObject(WHITE_BRUSH));
		Ellipse(hDC, cx - radius, cy - radius, cx + radius, cy + radius);

		// Get parameters for 10 points (outer and inner vertices)
		int outerR = radius;
		int innerR = (int)(radius * 0.382);  // Golden ratio
		double startAngle = -TAU / 4;
		double angleStep = TAU / (NUM_POINTS * 2);

		// Compute the points
		POINT starPts[10];
		for (int i = 0; i < 10; ++i) {
			double angle = startAngle + i * angleStep;
			int r = (i % 2 == 0) ? outerR : innerR;
			starPts[i].x = cx + (int)(r * cos(angle));
			starPts[i].y = cy + (int)(r * sin(angle));
		}

		// Fill the star
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Polygon(hDC, starPts, 10);
	}

	// Trapdoor object (square with "T" inside)
	if (cell.object == OBJECT_TRAPDOOR) {
		const float ratio = 0.80f;
		int innerSize = (int)(cellSize * ratio);
		int innerX = x + (cellSize - innerSize) / 2;
		int innerY = y + (cellSize - innerSize) / 2;

		// Draw the inner square
		SelectObject(hDC, GetStockObject(NULL_BRUSH));
		Rectangle(hDC, innerX, innerY,
		          innerX + innerSize, innerY + innerSize);

		// Set text alignment and background mode
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER | TA_BASELINE);

		// Create font scaled to 80% of inner square
		int fontHeight = (int)(innerSize * ratio);
		HFONT hFont =
		    CreateFont(
		        -fontHeight, 0, 0, 0, FW_BOLD,
		        FALSE, FALSE, FALSE,
		        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		        DEFAULT_PITCH | FF_SWISS,
		        TEXT("Arial")
		    );
		HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

		// Draw the "T" centered in the inner square
		int textX = innerX + innerSize / 2;
		int textY = innerY + innerSize / 2 + (int)(fontHeight * 0.43);
		TextOut(hDC, textX, textY, TEXT("T"), 1);

		// Cleanup
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
	}

	// Pit object (square with "X" inside)
	if (cell.object == OBJECT_PIT) {

		// Size and position of square
		int innerSize = (int)(cellSize * 0.70);
		int innerX = x + (cellSize - innerSize) / 2;
		int innerY = y + (cellSize - innerSize) / 2;

		// Draw the square outline
		SelectObject(hDC, GetStockObject(NULL_BRUSH));
		Rectangle(hDC, innerX, innerY, innerX + innerSize, innerY + innerSize);

		// Top-left to bottom-right diagonal
		MoveToEx(hDC, innerX, innerY, nullptr);
		LineTo(hDC, innerX + innerSize, innerY + innerSize);

		// Top-right to bottom-left diagonal
		MoveToEx(hDC, innerX + innerSize, innerY, nullptr);
		LineTo(hDC, innerX, innerY + innerSize);
	}

	// Rubble texture (bunch of random "x" characters)
	if (cell.object == OBJECT_RUBBLE) {

		int fontHeight = (int)(cellSize * 0.30);

		// Create font with desied height
		HFONT hFont =
		    CreateFont(
		        -fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas")
		    );
		HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

		// Measure actual text size
		SIZE textSize;
		GetTextExtentPoint32(hDC, TEXT("x"), 1, &textSize);

		// Transparent background so characters don't overwrite fill
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_LEFT | TA_TOP);

		// Draw a number of random "x" characters
		for (int i = 0; i < 10; ++i) {
			int pctx = rand() % 100;
			int pcty = rand() % 100;
			int tx = x + (cellSize - textSize.cx) * pctx / 100;
			int ty = y + (cellSize - textSize.cy) * pcty / 100;
			TextOut(hDC, tx, ty, TEXT("x"), 1);
		}

		// Cleanup
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
	}
}

// Get a random float between -1 and +1
double GridMap::randomUnit() const
{
	return 2.0 * rand() / RAND_MAX - 1.0;
}

// Find the two vertices of a space in a given direction
void GridMap::getVertexPoints(
    int x, int y, POINT& a, POINT& b, Direction dir)
{
	int cellSize = getCellSizePixels();
	switch (dir) {
		case NORTH:
			a = { x, y };
			b = { x + cellSize, y };
			break;
		case SOUTH:
			a = { x + cellSize, y + cellSize };
			b = { x, y + cellSize };
			break;
		case EAST:
			a = { x + cellSize, y };
			b = { x + cellSize, y + cellSize };
			break;
		case WEST:
			a = { x, y + cellSize };
			b = { x, y };
			break;
	}
}

// Generate a fractal line between two points
void GridMap::generateFractalCurveRecursive(
    std::vector<POINT>& points,
    int x1, int y1,
    int x2, int y2,
    double displacement,
    int depthToGo)
{
	// Compute distance
	double dx = x2 - x1;
	double dy = y2 - y1;
	double dist = sqrt(dx * dx + dy * dy);

	// Base case
	if (depthToGo == 0 || dist < 1.0) {
		points.push_back({ x2, y2 });
	}

	// Recursive case
	else {

		// Midpoint
		double mx = (x1 + x2) / 2.0;
		double my = (y1 + y2) / 2.0;

		// Perpendicular vector (-dy, dx), normalized
		double perpX = -dy / dist;
		double perpY = dx / dist;

		// Apply displacement along perpendicular
		double offset = displacement * randomUnit();
		mx += perpX * offset;
		my += perpY * offset;

		// Recursive calls
		generateFractalCurveRecursive(
		    points, x1, y1, (int) mx, (int) my,
		    displacement / 2.0, depthToGo - 1);
		generateFractalCurveRecursive(
		    points, (int) mx, (int) my, x2, y2,
		    displacement / 2.0, depthToGo - 1);
	}
}

// Draw a quadrant of a filled square, with fractal edge
void GridMap::drawFillQuadrantFractal(HDC hDC, int x, int y, Direction dir)
{
	int cellSize = getCellSizePixels();
	POINT center = { x + cellSize / 2, y + cellSize / 2 };

	// Set outer vertices
	POINT a, b;
	getVertexPoints(x, y, a, b, dir);

	// Construct the closed shape
	std::vector<POINT> shape;
	shape.push_back(a);
	generateFractalCurveRecursive(
	    shape, a.x, a.y, b.x, b.y, cellSize * 0.33, 5);
	shape.push_back(center);
	shape.push_back(a);

	// Fill polygon with black
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, shape.data(), (int)(shape.size()));
}

// Draw a quadrant of a filled square, with smooth edge
void GridMap::drawFillQuadrantSmooth(HDC hDC, int x, int y, Direction dir)
{
	int cellSize = getCellSizePixels();
	POINT center = { x + cellSize / 2, y + cellSize / 2 };

	// Set outer vertices
	POINT a, b;
	getVertexPoints(x, y, a, b, dir);

	// Construct the triangle
	POINT triangle[3] = { a, b, center };

	// Fill with black
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, triangle, 3);
}

// Draw a quadrant of a filled space
void GridMap::drawFillQuadrant(
    HDC hDC, int x, int y, Direction dir, bool fractal)
{
	if (fractal) {
		drawFillQuadrantFractal(hDC, x, y, dir);
	}
	else {
		drawFillQuadrantSmooth(hDC, x, y, dir);
	}
}

// Draw a filled space, possibly with fractal edges
void GridMap::drawFillSpaceFractal(HDC hDC, int x, int y)
{
	assert(displayRoughEdges());

	// Convert back to grid coordinates to check neighbors
	int cellSize = getCellSizePixels();
	int gx = x / cellSize;
	int gy = y / cellSize;

	// Draw each quadrant, with fractal edge if neighbor open
	drawFillQuadrant(
	    hDC, x, y, WEST, gx > 1
	    && IsFloorOpenType(getCellFloor(gx - 1, gy)));
	drawFillQuadrant(
	    hDC, x, y, NORTH, gy > 1
	    && IsFloorOpenType(getCellFloor(gx, gy - 1)));
	drawFillQuadrant(
	    hDC, x, y, EAST, gx + 1 < width
	    && IsFloorOpenType(getCellFloor(gx + 1, gy)));
	drawFillQuadrant(
	    hDC, x, y, SOUTH, gy + 1 < height
	    && IsFloorOpenType(getCellFloor(gx, gy + 1)));
}

// Draw a diagonally filled space with fractal edge
void GridMap::drawDiagonalFillFractal(
    HDC hDC, int x, int y, FloorType floor)
{
	assert(IsFloorDiagonalFill(floor));
	int cellSize = getCellSizePixels();

	// Declarations
	std::vector<POINT> shape;

	// Set diagonal endpoints
	POINT start, end;
	if (floor == FLOOR_NEFILL || floor == FLOOR_SWFILL) {
		start = { x, y };
		end   = { x + cellSize, y + cellSize };
	}
	else {
		start = { x + cellSize, y };
		end   = { x, y + cellSize };
	}

	// Set extra vertex
	POINT extraVertex;
	switch (floor) {
		case FLOOR_NWFILL:
			extraVertex = { x, y };
			break;
		case FLOOR_NEFILL:
			extraVertex = { x + cellSize, y };
			break;
		case FLOOR_SEFILL:
			extraVertex = { x + cellSize, y + cellSize };
			break;
		case FLOOR_SWFILL:
			extraVertex = { x, y + cellSize };
			break;
	}

	// Construct the closed shape
	shape.push_back(start);
	generateFractalCurveRecursive(
	    shape, start.x, start.y, end.x, end.y, cellSize * 0.33, 5);
	shape.push_back(extraVertex);
	shape.push_back(start);

	// Fill polygon
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, shape.data(), (int)(shape.size()));
}

// Draw a diagonally filled space (with smooth edge)
void GridMap::drawDiagonalFillSmooth(HDC hDC, int x, int y, FloorType floor)
{
	assert(IsFloorDiagonalFill(floor));
	int cellSize = getCellSizePixels();

	// Construct the triangle
	POINT triangle[3];
	switch (floor) {
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

	// Fill polygon
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, triangle, 3);
}
