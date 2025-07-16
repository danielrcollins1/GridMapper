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
		default:
			return false;
	}
}

// Is this floor type one of the diagonal fills?
bool IsFloorDiagonalFill(FloorType floor)
{
	return FLOOR_NWFILL <= floor && floor <= FLOOR_SEFILL;
}

// Is this floor type either fully or partially open?
bool IsFloorSemiOpen(FloorType floor)
{
	return IsFloorOpenType(floor) || IsFloorDiagonalFill(floor);
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
	displayCode =
	    (displayCode & ~MASK_CELL_SIZE)
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
GridMap::GridMap(unsigned _width, unsigned _height)
{
	width = _width;
	height = _height;
	displayCode = 0;
	setCellSizePixels(getCellSizeDefault());
	grid = new GridCell*[width];
	for (unsigned x = 0; x < width; x++)
		grid[x] = new GridCell[height];
	for (unsigned x = 0; x < width; x++) {
		for (unsigned y = 0; y < height; y++) {
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
	for (unsigned x = 0; x < width; x++) {
		grid[x] = new GridCell[height];
	}
	for (unsigned x = 0; x < width; x++) {
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
	for (unsigned x = 0; x < width; x++) {
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
	for (unsigned x = 0; x < width; x++) {
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

unsigned GridMap::getWidthCells()
{
	return width;
}

unsigned GridMap::getHeightCells()
{
	return height;
}

unsigned GridMap::getWidthPixels()
{
	return width * getCellSizePixels();
}

unsigned GridMap::getHeightPixels()
{
	return height * getCellSizePixels();
}

FloorType GridMap::getCellFloor(GridCoord gc)
{
	assert(gc.x < width && gc.y < height);
	return (FloorType) grid[gc.x][gc.y].floor;
}

WallType GridMap::getCellNWall(GridCoord gc)
{
	assert(gc.x < width && gc.y < height);
	return (WallType) grid[gc.x][gc.y].nwall;
}

WallType GridMap::getCellWWall(GridCoord gc)
{
	assert(gc.x < width && gc.y < height);
	return (WallType) grid[gc.x][gc.y].wwall;
}

ObjectType GridMap::getCellObject(GridCoord gc)
{
	assert(gc.x < width && gc.y < height);
	return (ObjectType) grid[gc.x][gc.y].object;
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
bool GridMap::canBuildNWall(GridCoord gc)
{
	if (gc.y == 0) {            // on top border
		return false;
	}
	switch (getCellFloor({gc.x, gc.y-1})) {
		case FLOOR_FILL:
		case FLOOR_SWFILL:
		case FLOOR_SEFILL:
			return false;
		default:
			break;
	}
	switch (getCellFloor(gc)) {
		case FLOOR_FILL:
		case FLOOR_NWFILL:
		case FLOOR_NEFILL:
			return false;
		default:
			break;
	}
	return true;
}

/*
	Can we erect a wall partition to the west of a given cell?
	Prohibited if spaced filled on either side.
*/
bool GridMap::canBuildWWall(GridCoord gc)
{
	if (gc.x == 0) {            // on left border
		return false;
	}
	switch (getCellFloor({gc.x-1, gc.y})) {
		case FLOOR_FILL:
		case FLOOR_NEFILL:
		case FLOOR_SEFILL:
			return false;
		default:
			break;
	}
	switch (getCellFloor(gc)) {
		case FLOOR_FILL:
		case FLOOR_NWFILL:
		case FLOOR_SWFILL:
			return false;
		default:
			break;
	}
	return true;
}

//------------------------------------------------------------------
// Mutators
//------------------------------------------------------------------

void GridMap::setCellFloor(GridCoord gc, int floor)
{
	assert(gc.x < width && gc.y < height);
	grid[gc.x][gc.y].floor = floor;
	changed = true;
}

void GridMap::setCellNWall(GridCoord gc, int wall)
{
	assert(gc.x < width && gc.y < height);
	grid[gc.x][gc.y].nwall = wall;
	changed = true;
}

void GridMap::setCellWWall(GridCoord gc, int wall)
{
	assert(gc.x < width && gc.y < height);
	grid[gc.x][gc.y].wwall = wall;
	changed = true;
}

void GridMap::setCellObject(GridCoord gc, int object)
{
	assert(gc.x < width && gc.y < height);
	grid[gc.x][gc.y].object = object;
	changed = true;
}

void GridMap::setFilename(char *name)
{
	strncpy(filename, name, GRID_FILENAME_MAX);
}

// Clear the entire map
void GridMap::clearMap(int _floor)
{
	for (unsigned x = 0; x < width; x++) {
		for (unsigned y = 0; y < height; y++) {
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
unsigned GridMap::cellHash(GridCoord gc) const
{
	unsigned int h = gc.x;
	h = h * 31 + gc.y;         // Mix x and y with a prime
	h ^= (h >> 16);            // Bit mixing
	h *= 0x85ebca6b;
	h ^= (h >> 13);
	h *= 0xc2b2ae35;
	h ^= (h >> 16);
	return h;
}

// Paint entire map on device context
void GridMap::paint(HDC hDC)
{
	for (unsigned x = 0; x < width; x++) {
		for (unsigned y = 0; y < height; y++) {
			paintCell(hDC, {x, y}, false);
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
    HDC hDC, GridCoord gc, bool partialRepaint, int depth)
{
	// Handle recursion limit for open redraws
	if (depth > 1 && IsFloorSemiOpen(getCellFloor(gc))) {
		return;
	}

	// Seed randomizations for this cell
	srand(cellHash(gc));

	// Paint everything controlled by this cell
	int cellSize = getCellSizePixels();
	POINT p = {(LONG)(gc.x * cellSize), (LONG)(gc.y * cellSize)};
	paintCellFloor(hDC, p, getCellFloor(gc));
	paintCellObject(hDC, p, getCellObject(gc));
	paintCellNWall(hDC, p, getCellNWall(gc));
	paintCellWWall(hDC, p, getCellWWall(gc));

	// Repaint walls east & south if needed
	if (partialRepaint) {
		if (gc.x+1 < width) {
			paintCellWWall(
			    hDC, {p.x + cellSize, p.y}, getCellWWall({gc.x+1, gc.y}));
		}
		if (gc.y+1 < height) {
			paintCellNWall(
			    hDC, {p.x, p.y + cellSize}, getCellNWall({gc.x, gc.y+1}));
		}
	}

	// Repaint neighbors in case of bleeding rough edges
	if (displayRoughEdges() && IsFloorSemiOpen(getCellFloor(gc))) {
		if (gc.x > 0) {
			paintCell(hDC, {gc.x-1, gc.y}, true, depth + 1);
		}
		if (gc.y > 0) {
			paintCell(hDC, {gc.x, gc.y-1}, true, depth + 1);
		}
		if (gc.x+1 < width) {
			paintCell(hDC, {gc.x+1, gc.y}, true, depth + 1);
		}
		if (gc.y+1 < height) {
			paintCell(hDC, {gc.x, gc.y+1}, true, depth + 1);
		}
	}
}

// Paint one cell's floor
void GridMap::paintCellFloor(HDC hDC, POINT p, FloorType floor)
{
	int cellSize = getCellSizePixels();

	// If we're a filled cell with no rough edges,
	// then simply paint a black rectangle and return
	if (floor == FLOOR_FILL && !displayRoughEdges()) {
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Rectangle(hDC, p.x, p.y, p.x + cellSize, p.y + cellSize);
		return;
	}

	// Paint a white rectangle as background
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	Rectangle(hDC, p.x, p.y, p.x + cellSize, p.y + cellSize);

	// Set pen for other features
	SelectObject(hDC, GetStockObject(BLACK_PEN));

	// Draw a filled space with rough edges
	if (floor == FLOOR_FILL) {
		assert(displayRoughEdges());
		drawFillSpaceRough(hDC, p);
	}

	// Stairs (series of parallel lines)
	if (floor == FLOOR_NSTAIRS || floor == FLOOR_WSTAIRS) {
		const int stairsPerSquare = 5;
		for (int s = 0; s <= stairsPerSquare; s++) {
			int d = s * cellSize / stairsPerSquare;
			if (floor == FLOOR_NSTAIRS) {
				MoveToEx(hDC, p.x, p.y + d, NULL);
				LineTo(hDC, p.x + cellSize, p.y + d);
			}
			else {
				MoveToEx(hDC, p.x + d, p.y, NULL);
				LineTo(hDC, p.x + d, p.y + cellSize);
			}
		}
	}

	// Diagonal Wall NW/SE
	if (floor == FLOOR_NWWALL || floor == FLOOR_NWDOOR) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, p.x, p.y, NULL);
		LineTo(hDC, p.x + cellSize, p.y + cellSize);
	}

	// Diagonal Wall NE/SW
	if (floor == FLOOR_NEWALL || floor == FLOOR_NEDOOR) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, p.x + cellSize, p.y, NULL);
		LineTo(hDC, p.x, p.y + cellSize);
	}

	// Diagonal Door (diamond in square center)
	if (floor == FLOOR_NEDOOR || floor == FLOOR_NWDOOR) {
		SelectObject(hDC, GetStockObject(BLACK_PEN));
		SelectObject(hDC, GetStockObject(WHITE_BRUSH));

		// Find cell center & door corners
		int halfCell = cellSize / 2;
		int cx = p.x + halfCell;
		int cy = p.y + halfCell;
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
	if (IsFloorDiagonalFill(floor)) {
		if (displayRoughEdges()) {
			drawDiagonalFillRough(hDC, p, floor);
		}
		else {
			drawDiagonalFillSmooth(hDC, p, floor);
		}
	}

	// Spiral stairs (arc, circle, and spokes)
	if (floor == FLOOR_SPIRALSTAIRS) {

		// Find parameters
		int halfCell = cellSize / 2;
		int radius = halfCell;
		int cx = p.x + halfCell;
		int cy = p.y + halfCell;

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
	if (floor == FLOOR_WATER) {

		// Set line increment
		const int linesPerEdge = 4;
		int inc = cellSize / linesPerEdge;

		// Draw lines from top-left to bottom-right
		for (int offset = -cellSize; offset <= cellSize; offset += inc) {
			int startX = p.x + max(0, offset);
			int startY = p.y + max(0, -offset);
			int endX = p.x + min(cellSize, cellSize + offset);
			int endY = p.y + min(cellSize, cellSize - offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}

		// Draw lines from top-right to bottom-left
		for (int offset = 1; offset <= 2 * cellSize; offset += inc) {
			int startX = p.x + min(cellSize, offset);
			int startY = p.y + max(0, offset - cellSize);
			int endX = p.x + max(0, offset - cellSize);
			int endY = p.y + min(cellSize, offset);
			MoveToEx(hDC, startX, startY, NULL);
			LineTo(hDC, endX, endY);
		}
	}
}

// Paint one cell's north wall
void GridMap::paintCellNWall(HDC hDC, POINT p, WallType wall)
{
	int cellSize = getCellSizePixels();

	// Paint grid line as needed
	if (wall != WALL_OPEN) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, p.x, p.y, NULL);
		LineTo(hDC, p.x + cellSize, p.y);
	}
	else if (!displayNoGrid()) {
		SelectObject(hDC, ThinGrayPen);
		MoveToEx(hDC, p.x, p.y, NULL);
		LineTo(hDC, p.x + cellSize, p.y);
	}

	// Set door size, pen, brush
	int h = cellSize / 4; // half door size
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));

	// Single door
	if (wall == WALL_SINGLE_DOOR) {
		Rectangle(hDC, p.x+h+1, p.y-h+1, p.x+3*h, p.y+h);
	}

	// Double door
	if (wall == WALL_DOUBLE_DOOR) {
		Rectangle(hDC, p.x+2, p.y-h+1, p.x+2*h+1, p.y+h);
		Rectangle(hDC, p.x+2*h, p.y-h+1, p.x+4*h-1, p.y+h);
	}

	// Secret door
	if (wall == WALL_SECRET_DOOR) {
		LetterS(hDC, {p.x+h*2, p.y+1});
	}
}

// Paint one cell's west wall
void GridMap::paintCellWWall(HDC hDC, POINT p, WallType wall)
{
	int cellSize = getCellSizePixels();

	// Paint grid line as needed
	if (wall != WALL_OPEN) {
		SelectObject(hDC, ThickBlackPen);
		MoveToEx(hDC, p.x, p.y, NULL);
		LineTo(hDC, p.x, p.y + cellSize);
	}
	else if (!displayNoGrid()) {
		SelectObject(hDC, ThinGrayPen);
		MoveToEx(hDC, p.x, p.y, NULL);
		LineTo(hDC, p.x, p.y + cellSize);
	}

	// Set door size, pen, brush
	int h = cellSize / 4; // half door size
	SelectObject(hDC, GetStockObject(BLACK_PEN));
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));

	// Single door
	if (wall == WALL_SINGLE_DOOR) {
		Rectangle(hDC, p.x-h+1, p.y+h+1, p.x+h, p.y+3*h);
	}

	// Double door
	if (wall == WALL_DOUBLE_DOOR) {
		Rectangle(hDC, p.x-h+1, p.y+2, p.x+h, p.y+2*h+1);
		Rectangle(hDC, p.x-h+1, p.y+2*h, p.x+h, p.y+4*h-1);
	}

	// Secret door
	if (wall == WALL_SECRET_DOOR) {
		LetterS(hDC, {p.x, p.y+h*2+1});
	}
}

/*
	Paint letter 'S' centered at point p
	Represents a secret door
	Also paints background behind letter
*/
void GridMap::LetterS(HDC hDC, POINT p)
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
	int textX = p.x - textSize.cx / 2;
	int textY = p.y - textSize.cy / 2;

	// Draw the letter "S"
	SetBkMode(hDC, OPAQUE);
	SetTextAlign(hDC, TA_LEFT | TA_TOP);
	TextOut(hDC, textX, textY, TEXT("S"), 1);

	// Clean up
	SelectObject(hDC, hOldFont);
	DeleteObject(hFont);
}

// Paint one cell's object
void GridMap::paintCellObject(HDC hDC, POINT p, ObjectType object)
{
	int cellSize = getCellSizePixels();
	SelectObject(hDC, GetStockObject(BLACK_PEN));

	// Pillar object (black circle)
	if (object == OBJECT_PILLAR) {
		int halfCell = cellSize / 2;
		int cx = p.x + halfCell;
		int cy = p.y + halfCell;
		int radius = (int)(halfCell * 0.50);
		SelectObject(hDC, GetStockObject(BLACK_BRUSH));
		Ellipse(hDC, cx - radius, cy - radius, cx + radius, cy + radius);
	}

	// Statue object (circle with 5-pointed star inside)
	if (object == OBJECT_STATUE) {
		const int NUM_POINTS = 5;

		// Compute center of square and radius of circle
		int halfCell = cellSize / 2;
		int cx = p.x + halfCell;
		int cy = p.y + halfCell;
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
	if (object == OBJECT_TRAPDOOR) {
		const float ratio = 0.80f;
		int innerSize = (int)(cellSize * ratio);
		int innerX = p.x + (cellSize - innerSize) / 2;
		int innerY = p.y + (cellSize - innerSize) / 2;

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
	if (object == OBJECT_PIT) {

		// Size and position of square
		int innerSize = (int)(cellSize * 0.70);
		int innerX = p.x + (cellSize - innerSize) / 2;
		int innerY = p.y + (cellSize - innerSize) / 2;

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
	if (object == OBJECT_RUBBLE) {

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
			int tx = p.x + (cellSize - textSize.cx) * pctx / 100;
			int ty = p.y + (cellSize - textSize.cy) * pcty / 100;
			TextOut(hDC, tx, ty, TEXT("x"), 1);
		}

		// Cleanup
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
	}

	// X-Mark (character "X" in center of square)
	if (object == OBJECT_XMARK) {

		int fontHeight = (int)(cellSize * 0.65);

		// Create font
		HFONT hFont =
		    CreateFont(
		        -fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Segoe UI")
		    );
		HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

		// Set alignment and background mode
		SetTextAlign(hDC, TA_CENTER);
		SetBkMode(hDC, TRANSPARENT);

		// Get text metrics
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);
		int textHeight = tm.tmHeight;

		// Compute center of square
		int centerX = p.x + cellSize / 2;
		int centerY = p.y + cellSize / 2 - textHeight / 2;

		// Draw the character
		TextOut(hDC, centerX, centerY, TEXT("X"), 1);

		// Cleanup
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
	}

	// Stalagmite (circle with partial spokes, random location)
	if (object == OBJECT_STALAGMITE) {

		double circleFraction = 0.30 + 0.01 * (rand() % 20);
		int circleDiameter = (int)(cellSize * circleFraction);
		int radius = circleDiameter / 2;

		// Clamp random position to stay inside square
		int maxOffset = cellSize - circleDiameter;
		int pctx = rand() % 100;
		int pcty = rand() % 100;
		int randX = p.x + pctx * maxOffset / 100;
		int randY = p.y + pcty * maxOffset / 100;
		int cx = randX + radius;
		int cy = randY + radius;

		// Draw the filled circle
		Ellipse(hDC, cx - radius, cy - radius, cx + radius, cy + radius);

		// Draw partial spokes
		const int numLines = 4;
		for (int i = 0; i < numLines; ++i) {
			double angle = i * (TAU / numLines);
			int xOuter = cx + (int)(radius * cos(angle));
			int yOuter = cy + (int)(radius * sin(angle));
			int xInner = cx + (int)((radius / 2.0) * cos(angle));
			int yInner = cy + (int)((radius / 2.0) * sin(angle));
			MoveToEx(hDC, xOuter, yOuter, NULL);
			LineTo(hDC, xInner, yInner);
		}
	}
}

// Get a random float between -1 and +1
double GridMap::randomUnit() const
{
	return 2.0 * rand() / RAND_MAX - 1.0;
}

// Find the two vertices of a space in a given direction
void GridMap::getVertexPoints(POINT p, POINT& a, POINT& b, Direction dir)
{
	int cellSize = getCellSizePixels();
	switch (dir) {
		case NORTH:
			a = { p.x, p.y };
			b = { p.x + cellSize, p.y };
			break;
		case SOUTH:
			a = { p.x + cellSize, p.y + cellSize };
			b = { p.x, p.y + cellSize };
			break;
		case EAST:
			a = { p.x + cellSize, p.y };
			b = { p.x + cellSize, p.y + cellSize };
			break;
		case WEST:
			a = { p.x, p.y + cellSize };
			b = { p.x, p.y };
			break;
	}
}

// Generate a fractal line between two points
void GridMap::generateFractalCurveRecursive(
    POINT start, POINT end, std::vector<POINT>& path,
    double displacement, int depthToGo)
{
	// Compute distance
	double dx = end.x - start.x;
	double dy = end.y - start.y;
	double dist = sqrt(dx * dx + dy * dy);

	// Base case
	if (depthToGo == 0 || dist < 1.0) {
		path.push_back(end);
	}

	// Recursive case
	else {

		// Midpoint
		double mx = (start.x + end.x) / 2.0;
		double my = (start.y + end.y) / 2.0;

		// Perpendicular vector (-dy, dx), normalized
		double perpX = -dy / dist;
		double perpY = dx / dist;

		// Apply displacement along perpendicular
		double offset = displacement * randomUnit();
		mx += perpX * offset;
		my += perpY * offset;
		POINT midpoint = {(LONG) mx, (LONG) my};

		// Recursive calls
		generateFractalCurveRecursive(
		    start, midpoint, path, displacement / 2.0, depthToGo - 1);
		generateFractalCurveRecursive(
		    midpoint, end, path, displacement / 2.0, depthToGo - 1);
	}
}

// Draw a quadrant of a filled square, with fractal edge
void GridMap::drawFillQuadrantRough(HDC hDC, POINT p, Direction dir)
{
	// Get dimensions
	int cellSize = getCellSizePixels();
	POINT center = { p.x + cellSize / 2, p.y + cellSize / 2 };

	// Set outer vertices
	POINT a, b;
	getVertexPoints(p, a, b, dir);

	// Construct the closed shape
	std::vector<POINT> shape;
	shape.push_back(a);
	generateFractalCurveRecursive(
	    a, b, shape, cellSize * DISPLACEMENT_SCALE, RECURSION_LIMIT);
	shape.push_back(center);
	shape.push_back(a);

	// Fill polygon with black
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, shape.data(), (int)(shape.size()));
}

// Draw a quadrant of a filled square, with smooth edge
void GridMap::drawFillQuadrantSmooth(HDC hDC, POINT p, Direction dir)
{
	int cellSize = getCellSizePixels();
	POINT center = { p.x + cellSize / 2, p.y + cellSize / 2 };

	// Set outer vertices
	POINT a, b;
	getVertexPoints(p, a, b, dir);

	// Construct the triangle
	POINT triangle[3] = { a, b, center };

	// Fill with black
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, triangle, 3);
}

// Draw a quadrant of a filled space
void GridMap::drawFillQuadrant(HDC hDC, POINT p, Direction dir)
{
	assert(displayRoughEdges());

	// Convert back to grid coordinates to check neighbors
	unsigned gx = (unsigned)(p.x / getCellSizePixels());
	unsigned gy = (unsigned)(p.y / getCellSizePixels());
	GridCoord gc = {gx, gy};

	// Rough-up if exposed edge
	if (isExposedEdge(gc, dir)) {
		drawFillQuadrantRough(hDC, p, dir);
	}
	else {
		drawFillQuadrantSmooth(hDC, p, dir);
	}
}

/*
	Determine if a given cell edge is an exposed surface
	(boundary between fill & open spaces, possibly roughed)
	Assume given cell is filled inside indicated edge
*/
bool GridMap::isExposedEdge(GridCoord gc, Direction dir)
{
	// Note: This could maybe be expanded to diagonal fills
	assert(getCellFloor(gc) == FLOOR_FILL);

	// Neighbor coordinate
	GridCoord nc;

	switch (dir) {
		case WEST:
			if (gc.x > 0) {
				nc = {gc.x-1, gc.y};
				return (IsFloorOpenType(getCellFloor(nc))
				        || getCellFloor(nc) == FLOOR_NWFILL
				        || getCellFloor(nc) == FLOOR_SWFILL);
			}
			break;

		case EAST:
			if (gc.x < width-1) {
				nc = {gc.x+1, gc.y};
				return (IsFloorOpenType(getCellFloor(nc))
				        || getCellFloor(nc) == FLOOR_NEFILL
				        || getCellFloor(nc) == FLOOR_SEFILL);
			}
			break;

		case NORTH:
			if (gc.y > 0) {
				nc = {gc.x, gc.y-1};
				return (IsFloorOpenType(getCellFloor(nc))
				        || getCellFloor(nc) == FLOOR_NEFILL
				        || getCellFloor(nc) == FLOOR_NWFILL);
			}
			break;

		case SOUTH:
			if (gc.y < height-1) {
				nc = {gc.x, gc.y+1};
				return (IsFloorOpenType(getCellFloor(nc))
				        || getCellFloor(nc) == FLOOR_SEFILL
				        || getCellFloor(nc) == FLOOR_SWFILL);
			}
			break;
	}
	return false;
}

// Draw a filled space, possibly with fractal edges
void GridMap::drawFillSpaceRough(HDC hDC, POINT p)
{
	assert(displayRoughEdges());

	// Draw each quadrant
	drawFillQuadrant(hDC, p, NORTH);
	drawFillQuadrant(hDC, p, EAST);
	drawFillQuadrant(hDC, p, SOUTH);
	drawFillQuadrant(hDC, p, WEST);
}

// Draw a diagonally filled space with fractal edge
void GridMap::drawDiagonalFillRough(HDC hDC, POINT p, FloorType floor)
{
	assert(IsFloorDiagonalFill(floor));

	// Get dimensions
	int cellSize = getCellSizePixels();

	// Set diagonal endpoints
	POINT start, end;
	if (floor == FLOOR_NEFILL || floor == FLOOR_SWFILL) {
		start = { p.x, p.y };
		end   = { p.x + cellSize, p.y + cellSize };
	}
	else {
		start = { p.x + cellSize, p.y };
		end   = { p.x, p.y + cellSize };
	}

	// Set extra vertex
	POINT extraVertex;
	switch (floor) {
		case FLOOR_NWFILL:
			extraVertex = { p.x, p.y };
			break;
		case FLOOR_NEFILL:
			extraVertex = { p.x + cellSize, p.y };
			break;
		case FLOOR_SEFILL:
			extraVertex = { p.x + cellSize, p.y + cellSize };
			break;
		case FLOOR_SWFILL:
			extraVertex = { p.x, p.y + cellSize };
			break;
		default:
			assert(FALSE);
			break;
	}

	// Construct the closed shape
	std::vector<POINT> shape;
	shape.push_back(start);
	generateFractalCurveRecursive(
	    start, end, shape, cellSize * DISPLACEMENT_SCALE, RECURSION_LIMIT);
	shape.push_back(extraVertex);
	shape.push_back(start);

	// Fill polygon
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, shape.data(), (int)(shape.size()));
}

// Draw a diagonally filled space (with smooth edge)
void GridMap::drawDiagonalFillSmooth(HDC hDC, POINT p, FloorType floor)
{
	assert(IsFloorDiagonalFill(floor));
	int cellSize = getCellSizePixels();

	// Construct the triangle
	POINT triangle[3];
	switch (floor) {
		case FLOOR_NWFILL:
			triangle[0] = { p.x, p.y };
			triangle[1] = { p.x + cellSize, p.y };
			triangle[2] = { p.x, p.y + cellSize };
			break;
		case FLOOR_NEFILL:
			triangle[0] = { p.x + cellSize, p.y };
			triangle[1] = { p.x + cellSize, p.y + cellSize };
			triangle[2] = { p.x, p.y };
			break;
		case FLOOR_SWFILL:
			triangle[0] = { p.x, p.y + cellSize };
			triangle[1] = { p.x, p.y };
			triangle[2] = { p.x + cellSize, p.y + cellSize };
			break;
		case FLOOR_SEFILL:
			triangle[0] = { p.x + cellSize, p.y + cellSize };
			triangle[1] = { p.x, p.y + cellSize };
			triangle[2] = { p.x + cellSize, p.y };
			break;
		default:
			assert(FALSE);
			break;
	}

	// Fill polygon
	SelectObject(hDC, GetStockObject(BLACK_BRUSH));
	Polygon(hDC, triangle, 3);
}
