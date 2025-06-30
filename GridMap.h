/*
=====================================================================
	GRIDMAP.H
		Interface to the gridmap object.
	Copyright (c) 2010 Daniel R. Collins. All rights reserved.
	See the bottom of this file for any licensing information.
=====================================================================
*/

#ifndef GRIDMAP_H
#define GRIDMAP_H

#include <windows.h>

struct GridCell;
#define GRID_FILENAME_MAX 256

// GridMap Interface

class GridMap {
	public:
		// Constructors
		GridMap (int width, int height);
		GridMap (char *filename);
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
		char *getFilename();
		bool isChanged();
		bool isFileLoadOk();

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
		void paintCellFloor (HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellNWall (HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellWWall (HDC hDC, int xPos, int yPos, GridCell cell);
		void paintCellObject (HDC hDC, int xPos, int yPos, GridCell cell);
		void LetterS (HDC hDC, int x, int y);

	private:
		GridCell **grid;
		int width, height, cellSize;
		char filename[GRID_FILENAME_MAX];
		bool changed, fileLoadOk;
};


//--------------------------------------------------------------
// Grid Cell struct
//   Each grid cell controls its own floor, north & west walls,
//   and any object. See available enum values further below.
//--------------------------------------------------------------

struct GridCell {
	char floor, nwall, wwall, object;
};


// Feature type enums
enum {FLOOR_FILL, FLOOR_CLEAR, FLOOR_NSTAIRS, FLOOR_WSTAIRS,
      FLOOR_NEWALL, FLOOR_NWWALL
     };
enum {WALL_CLEAR, WALL_FILL, WALL_SINGLE_DOOR, WALL_DOUBLE_DOOR, WALL_SECRET_DOOR};
enum {OBJECT_NONE, OBJECT_STATUE, OBJECT_RUBBLE, OBJECT_TRAPDOOR_FLOOR,
      OBJECT_TRAPDOOR_CEIL
     };

#endif

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

