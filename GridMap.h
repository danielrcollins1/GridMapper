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
	OBJECT_PIT, OBJECT_RUBBLE, OBJECT_STALGMITE, OBJECT_SINKHOLE,
	OBJECT_FAIL = 255
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
		void setCellFloor(int x, int y, int floor);
		void setCellNWall(int x, int y, int wall);
		void setCellWWall(int x, int y, int wall);
		void setCellObject(int x, int y, int object);
		void clearMap(int floor);
		void setFilename(char *name);

		// Paint on a display context
		void paint(HDC hDC);
		void paintCell(HDC hDC, int x, int y, bool allWalls);

		// Save to file
		int save();

		// Display settings
		static int getCellSizeMin();
		static int getCellSizeMax();
		static int getCellSizeDefault();
		int getCellSizePixels() const;
		bool displayRoughEdges() const;
		bool displayNoGrid() const;
		void setCellSizePixels(int cellSize);
		void toggleRoughEdges();
		void toggleNoGrid();

	protected:

		// Painting helper functions
		void paintCellFloor(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellObject(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellNWall(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellWWall(HDC hDC, int xPos, int yPos, GridCell cell);
		void LetterS(HDC hDC, int x, int y);
		unsigned cellHash(int x, int y) const;
		void makeStandardPens();

		// Rough-edge painting functions
		double randomUnit() const;
		void generateFractalCurveRecursive(
		    std::vector<POINT>& points,
		    int x1, int y1, int x2, int y2,
		    double displacement, int depthToGo);
		std::vector<POINT> generateFractalCurveWithNoise(
		    int x, int y, int length);
		void drawFractalCapAndFill(HDC hDC, int x, int y);
		void generateFractalDiagonalRecursive(
		    std::vector<POINT>& points,
		    int x1, int y1,
		    int x2, int y2,
		    double displacement,
		    int depthToGo);
		void drawFractalDiagonalFill(
		    HDC hDC, int x, int y, FloorType floor);
		void drawDiagonalFill(
		    HDC hDC, int x, int y, FloorType floor);

	private:
		GridCell **grid;
		unsigned int width, height, displayCode;
		char filename[GRID_FILENAME_MAX];
		bool changed, fileLoadOk;
		HPEN ThinGrayPen = NULL, ThickBlackPen = NULL;
};
#endif
