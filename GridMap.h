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

/*
	Structure for a single grid cell.
	Controls its owns floor, north & west walls, and any object.
*/
struct GridCell {
	char floor, nwall, wwall, object;
};

/*
	Enumerations for cell contents.
	Never reorder/renumber these,
	or you'll break the saved file format!
*/

// Floor types
enum FloorType {
	FLOOR_FAIL = -1,
	FLOOR_FILL, FLOOR_OPEN, FLOOR_NSTAIRS, FLOOR_WSTAIRS,
	FLOOR_NEWALL, FLOOR_NWWALL, FLOOR_NEDOOR, FLOOR_NWDOOR,
	FLOOR_NWFILL, FLOOR_NEFILL, FLOOR_SWFILL, FLOOR_SEFILL,
	FLOOR_SPIRALSTAIRS, FLOOR_WATER
};

// Wall types
enum WallType {
	WALL_FAIL = -1,
	WALL_OPEN, WALL_FILL, WALL_SINGLE_DOOR, WALL_DOUBLE_DOOR,
	WALL_SECRET_DOOR
};

// Object types
enum ObjectType {
	OBJECT_FAIL = -1,
	OBJECT_NONE, OBJECT_PILLAR, OBJECT_STATUE, OBJECT_TRAPDOOR,
	OBJECT_PIT, OBJECT_RUBBLE, OBJECT_STALGMITE, OBJECT_SINKHOLE
};

// Feature info function(s)
bool IsFloorFillType(FloorType floor);

// Filename max length
const int GRID_FILENAME_MAX = 256;

/*
	GridMap interface
*/
class GridMap {
	public:

		// Constructors
		GridMap(int width, int height);
		GridMap(char *filename);
		~GridMap();

		// Accessors
		int getWidthCells();
		int getHeightCells();
		int getWidthPixels();
		int getHeightPixels();
		int getCellSizePixels();
		int getCellSizeDefault();
		int getCellFloor(int x, int y);
		int getCellNWall(int x, int y);
		int getCellWWall(int x, int y);
		int getCellObject(int x, int y);
		bool canBuildNWall(int x, int y);
		bool canBuildWWall(int x, int y);
		bool isChanged();
		bool isFileLoadOk();
		char* getFilename();

		// Mutators
		void setCellSizePixels(int cellSize);
		void setCellFloor(int x, int y, int floor);
		void setCellNWall(int x, int y, int wall);
		void setCellWWall(int x, int y, int wall);
		void setCellObject(int x, int y, int object);
		void clearMap(int floor);
		void setFilename(char *name);

		// Paint on a display context
		void paint(HDC hDC, bool showGrid);
		void paintCell(HDC hDC, int x, int y, bool allWalls, bool showGrid);

		// Save to file
		int save();

	protected:

		// Painting helper functions
		void paintCellFloor(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellObject(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellNWall(
		    HDC hDC, int xPos, int yPos, GridCell cell, bool showGrid);
		void paintCellWWall(
		    HDC hDC, int xPos, int yPos, GridCell cell, bool showGrid);
		void LetterS(HDC hDC, int x, int y);

	private:
		GridCell **grid;
		int width, height, cellSize;
		char filename[GRID_FILENAME_MAX];
		bool changed, fileLoadOk;
};
#endif
