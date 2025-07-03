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

// Floor options
enum {FLOOR_FILL, FLOOR_CLEAR, FLOOR_NSTAIRS, FLOOR_WSTAIRS,
      FLOOR_NEWALL, FLOOR_NWWALL, FLOOR_NEDOOR, FLOOR_NWDOOR, FLOOR_WATER, 
	  FLOOR_NWFILL, FLOOR_NEFILL, FLOOR_SWFILL, FLOOR_SEFILL
     };

// Wall options
enum {WALL_CLEAR, WALL_FILL, WALL_SINGLE_DOOR, WALL_DOUBLE_DOOR,
      WALL_SECRET_DOOR
     };

// Object options
enum {OBJECT_NONE, OBJECT_STATUE, OBJECT_RUBBLE,
      OBJECT_TRAPDOOR_FLOOR, OBJECT_TRAPDOOR_CEIL
     };

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

		// Save to file
		int save();

		// Paint on a display context
		void paint(HDC hDC);
		void paintCell(HDC hDC, int x, int y, bool allWalls);

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

	protected:

		// Painting helper functions
		void paintCellFloor(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellNWall(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellWWall(HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellObject(HDC hDC, int xPos, int yPos, GridCell cell);
		void LetterS(HDC hDC, int x, int y);

	private:
		GridCell **grid;
		int width, height, cellSize;
		char filename[GRID_FILENAME_MAX];
		bool changed, fileLoadOk;
};
#endif
