/*
	Name: GridMap.h
	Copyright: 2010
	Author: Daniel R. Collins
	Date: 16-05-10
	Description: Interface to the GridMap object.
		See file LICENSE for licensing information.
		Contact author at delta@superdan.net
*/
#ifndef GRIDMAP_H
#define GRIDMAP_H
#include <windows.h>
#include <vector>

/*
	Structure for a grid coordinate.
*/
struct GridCoord {
	unsigned x, y;
};

/*
	Structure for a single grid cell.
	Controls its owns floor, north & west walls, and any object.
*/
struct GridCell {
	unsigned char floor, nwall, wwall, object;
};

/*
	Enumerations for cell contents.
	Never reorder/renumber these,
	or you'll break the saved file format!
*/

// Floor types
enum FloorType {
	FLOOR_FILL, FLOOR_OPEN, FLOOR_NSTAIRS, FLOOR_WSTAIRS,
	FLOOR_NEWALL, FLOOR_NWWALL, FLOOR_NEDOOR, FLOOR_NWDOOR,
	FLOOR_NWFILL, FLOOR_NEFILL, FLOOR_SWFILL, FLOOR_SEFILL,
	FLOOR_SPIRALSTAIRS, FLOOR_WATER, FLOOR_FAIL = 255
};

// Wall types
enum WallType {
	WALL_OPEN, WALL_FILL, WALL_SINGLE_DOOR, WALL_DOUBLE_DOOR,
	WALL_SECRET_DOOR, WALL_FAIL = 255
};

// Object types
enum ObjectType {
	OBJECT_NONE, OBJECT_PILLAR, OBJECT_STATUE, OBJECT_TRAPDOOR,
	OBJECT_PIT, OBJECT_RUBBLE, OBJECT_STALAGMITE, OBJECT_XMARK,
	OBJECT_FAIL = 255
};

// Cardinal directions
enum Direction {
	NORTH, SOUTH, EAST, WEST
};

// Feature info function(s)
bool IsFloorFillType(FloorType floor);
bool IsFloorOpenType(FloorType floor);
bool IsFloorDiagonalFill(FloorType floor);
bool IsFloorSemiOpen(FloorType floor);

// Filename max length
const int GRID_FILENAME_MAX = 256;

/*
	GridMap interface
*/
class GridMap {
	public:

		// Constructors
		GridMap(unsigned width, unsigned height);
		GridMap(char *filename);
		~GridMap();

		// Accessors
		bool isChanged() const;
		bool isFileLoadOk() const;
		const char* getFilename() const;
		unsigned getWidthCells() const;
		unsigned getHeightCells() const;
		unsigned getWidthPixels() const;
		unsigned getHeightPixels() const;
		FloorType getCellFloor(GridCoord gc) const;
		ObjectType getCellObject(GridCoord gc) const;
		WallType getCellNWall(GridCoord gc) const;
		WallType getCellWWall(GridCoord gc) const;
		bool canBuildNWall(GridCoord gc) const;
		bool canBuildWWall(GridCoord gc) const;

		// Mutators
		void setCellFloor(GridCoord gc, int floor);
		void setCellObject(GridCoord gc, int object);
		void setCellNWall(GridCoord gc, int wall);
		void setCellWWall(GridCoord gc, int wall);
		void clearMap(int floor);
		void setFilename(char *name);

		// Paint on a display context
		void paint(HDC hDC);
		void paintCell(
			GridCoord gc, bool partialRepaint, int recursionDepth = 0);

		// Save to file
		int save();

		// Display settings
		static unsigned getCellSizeMin();
		static unsigned getCellSizeMax();
		static unsigned getCellSizeDefault();
		unsigned getCellSizePixels() const;
		bool displayRoughEdges() const;
		bool displayNoGrid() const;
		void setCellSizePixels(unsigned cellSize);
		void toggleRoughEdges();
		void toggleNoGrid();

	private:

		// Painting helper functions
		unsigned cellHash(GridCoord gc) const;
		void paintCellFloor(POINT p, FloorType floor);
		void paintCellObject(POINT p, ObjectType object);
		void paintCellNWall(POINT p, WallType wall);
		void paintCellWWall(POINT p, WallType wall);
		void drawSecretDoor(POINT p);
		void makeStandardPens();

		// Rough-edge painting functions
		double randomUnit() const;
		bool isExposedEdge(GridCoord gc, Direction dir) const;
		void getVertexPoints(POINT p, POINT& a, POINT& b, Direction dir) const;
		void drawFillSpaceRough(POINT p);
		void drawFillQuadrant(POINT p, Direction dir);
		void drawFillQuadrantSmooth(POINT p, Direction dir);
		void drawFillQuadrantRough(POINT p, Direction dir);
		void drawDiagonalFillSmooth(POINT p, FloorType floor);
		void drawDiagonalFillRough(POINT p, FloorType floor);
		void generateFractalCurveRecursive(
		    POINT start, POINT end, std::vector<POINT>& path,
		    double displacement, int depthToGo);
		
		// Data fields
		GridCell **grid;
		unsigned width, height, displayCode;
		char filename[GRID_FILENAME_MAX];
		bool changed, fileLoadOk;

		// Drawing context handles
		HPEN ThinGrayPen = NULL, ThickBlackPen = NULL;
		HDC hMainDC = NULL;

		// Constants for fractal edges
		const int RECURSION_LIMIT = 4;
		const double DISPLACEMENT_SCALE = 0.25;
};
#endif
