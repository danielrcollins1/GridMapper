/*
=====================================================================
	GRIDMAP.CPP 
    Implementation of the GridMap class.
	Copyright (c) 2010 Daniel R. Collins. All rights reserved.
	See the bottom of this file for any licensing information.
=====================================================================
*/

// Includes
#include "stdafx.h"


// Globals
#define DEFAULT_CELL_SIZE 20
#define STAIRS_PER_SQUARE 5
#define LETTER_S_WIDTH  0.20
#define LETTER_S_HEIGHT 0.35


//------------------------------------------------------------------
// Constructor/ Destructors
//------------------------------------------------------------------

GridMap::GridMap (int _width, int _height) {
  width = _width;
  height = _height;
  cellSize = DEFAULT_CELL_SIZE;
  grid = new GridCell*[width];
  for (int x = 0; x < width; x++)
    grid[x] = new GridCell[height];
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      grid[x][y].floor = FLOOR_FILL;
      grid[x][y].nwall = WALL_CLEAR;
      grid[x][y].wwall = WALL_CLEAR;
      grid[x][y].object = 0;
    }
  }
  filename[0] = '\0';
  changed = false;
  fileLoadOk = true;
}


GridMap::~GridMap() {
  for (int x = 0; x < width; x++)
    delete [] grid[x];
  delete [] grid;
}


// Open from a specified filename

GridMap::GridMap (char *_filename) {

  // Declarations
  FILE *f;
  char header[4];
  int x;

  // Open file
  if (!(f = fopen(_filename, "rb"))) goto fail;

  // Check header
  fread(header, sizeof(char), 4, f);
  if (strncmp(header, "GM", 2)) goto fail;

  // Read & create other stuff
  fread(&cellSize, sizeof(int), 1, f);
  fread(&width, sizeof(int), 1, f);
  fread(&height, sizeof(int), 1, f);
  grid = new GridCell*[width];
  for (x = 0; x < width; x++)
    grid[x] = new GridCell[height];
  for (x = 0; x < width; x++)
    fread(grid[x], sizeof(GridCell), height, f);

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

int GridMap::save() {

  // Open file
  FILE *f = fopen(filename, "wb");
  if (!f) return 0;

  // Save stuff
  char header[] = "GM\1\0";
  fwrite(header, sizeof(char), 4, f);
  fwrite(&cellSize, sizeof(int), 1, f);
  fwrite(&width, sizeof(int), 1, f);
  fwrite(&height, sizeof(int), 1, f);
  for (int x = 0; x < width; x++) 
    fwrite(grid[x], sizeof(GridCell), height, f);

  // Clean up
  fclose(f);
  changed = false;
  return 1;
}


//------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------

int GridMap::getWidthCells() { return width; }
int GridMap::getHeightCells() { return height; }
int GridMap::getWidthPixels() { return width * cellSize; }
int GridMap::getHeightPixels() { return height * cellSize; }
int GridMap::getCellSizePixels() { return cellSize; }
int GridMap::getCellSizeDefault() { return DEFAULT_CELL_SIZE; }
int GridMap::getCellFloor(int x, int y) { return grid[x][y].floor; }
int GridMap::getCellNWall(int x, int y) { return grid[x][y].nwall; }
int GridMap::getCellWWall(int x, int y) { return grid[x][y].wwall; }
int GridMap::getCellObject(int x, int y) { return grid[x][y].object; }
char *GridMap::getFilename() { return filename; }
bool GridMap::isChanged() { return changed; }
bool GridMap::isFileLoadOk() { return fileLoadOk; }



//------------------------------------------------------------------
// Mutators
//------------------------------------------------------------------

void GridMap::setCellSizePixels(int _cellSize) {
  cellSize = _cellSize; 
}

void GridMap::setCellFloor(int x, int y, int floor) {
  grid[x][y].floor = floor; 
  changed = true; 
}

void GridMap::setCellNWall(int x, int y, int wall) {
  grid[x][y].nwall = wall; 
  changed = true; 
}

void GridMap::setCellWWall(int x, int y, int wall) {
  grid[x][y].wwall = wall; 
  changed = true; 
}

void GridMap::setCellObject(int x, int y, int object) {
  grid[x][y].object = object; 
  changed = true; 
}

void GridMap::setFilename(char *name) {
  strncpy(filename, name, GRID_FILENAME_MAX); 
}


// Clear the entire map
void GridMap::clearMap(int _floor) {
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      grid[x][y].floor = _floor;
      grid[x][y].wwall = WALL_CLEAR;
      grid[x][y].nwall = WALL_CLEAR;
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


// Grid painting implementations

void GridMap::paint(HDC hDC) {

  // Create pens if needed
  if (!ThickBlackPen) 
    ThickBlackPen = CreatePen(PS_SOLID, 3, 0x00000000);
  if (!ThinGrayPen)
    ThinGrayPen = CreatePen(PS_SOLID, 1, 0x00808080);

  // Draw each grid cell 
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      paintCell(hDC, x, y, false);
    }
  }
}


void GridMap::paintCell(HDC hDC, int x, int y, bool allWalls) {

  // Paint everything controlled by this cell
  int xPos = x * cellSize;
  int yPos = y * cellSize;
  paintCellFloor(hDC, xPos, yPos, grid[x][y]);
  paintCellNWall(hDC, xPos, yPos, grid[x][y]);
  paintCellWWall(hDC, xPos, yPos, grid[x][y]);
  paintCellObject(hDC, xPos, yPos, grid[x][y]);

  // Paint other adjacent walls if requested (partial repaint)
  if (allWalls) {
    if (x+1 < width)
      paintCellWWall(hDC, xPos + cellSize, yPos, grid[x+1][y]);
    if (y+1 < height)
      paintCellNWall(hDC, xPos, yPos + cellSize, grid[x][y+1]);
  }
}


void GridMap::paintCellFloor (HDC hDC, int x, int y, GridCell cell) {

  // Paint base clear or filled
  SelectObject(hDC, GetStockObject(cell.floor ? WHITE_PEN : BLACK_PEN));
  SelectObject(hDC, GetStockObject(cell.floor ? WHITE_BRUSH : BLACK_BRUSH));
  Rectangle(hDC, x, y, x + cellSize, y + cellSize);

  // Set stairs size, pen
  int h = cellSize / STAIRS_PER_SQUARE;
  SelectObject(hDC, GetStockObject(BLACK_PEN));

  // Stairs N/S
  if (cell.floor == FLOOR_NSTAIRS) {
    for (int dy = 0; dy < cellSize; dy+=h) {
      MoveToEx(hDC, x, y + dy, NULL);
      LineTo(hDC, x + cellSize, y + dy);
    }
  }

  // Stairs E/W
  if (cell.floor == FLOOR_WSTAIRS) {
    for (int dx = 0; dx < cellSize; dx+=h) {
      MoveToEx(hDC, x + dx, y, NULL);
      LineTo(hDC, x + dx, y + cellSize);
    }
  }

  // Diagonal Wall NW/SE
  if (cell.floor == FLOOR_NWWALL) {
	  SelectObject(hDC, ThickBlackPen);
	  MoveToEx(hDC, x, y, NULL);
	  LineTo(hDC, x + cellSize, y + cellSize);
  }

  // Diagonal Wall NE/SW
  if (cell.floor == FLOOR_NEWALL) {
	  SelectObject(hDC, ThickBlackPen);
	  MoveToEx(hDC, x + cellSize, y, NULL);
	  LineTo(hDC, x, y + cellSize);
  }
}


void GridMap::paintCellNWall (HDC hDC, int x, int y, GridCell cell) {

  // Paint base clear or filled
  SelectObject(hDC, cell.nwall ? ThickBlackPen : ThinGrayPen);
  MoveToEx(hDC, x, y, NULL);
  LineTo(hDC, x + cellSize, y);

  // Set door size, pen, brush
  int h = cellSize/4; // half door size
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


void GridMap::paintCellWWall (HDC hDC, int x, int y, GridCell cell) {

  // Paint base clear or filled
  SelectObject(hDC, cell.wwall ? ThickBlackPen : ThinGrayPen);
  MoveToEx(hDC, x, y, NULL);
  LineTo(hDC, x, y + cellSize);

  // Set door size, pen, brush
  int h = cellSize/4; // half door size
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
    LetterS(hDC, x, y+h*2);
  }
}


void GridMap::LetterS (HDC hDC, int x, int y) {

  // Compute width & height increments
  int j = (int)((float) cellSize * LETTER_S_WIDTH); 
  int k = (int)((float) cellSize * LETTER_S_HEIGHT); 

  // Center the "S" at (x,y)
  Arc(hDC, x-j, y-k, x+j, y, x+j, y-k/2, x, y);
  Arc(hDC, x-j, y, x+j, y+k, x-j, y+k/2, x, y);
}


void GridMap::paintCellObject (HDC hDC, int x, int y, GridCell cell) {
  // For future expansion
}


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

