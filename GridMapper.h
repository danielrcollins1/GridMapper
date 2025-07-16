/*
	Name: GridMapper.h
	Copyright: 2010
	Author: Daniel R. Collins
	Date: 03-05-10
	Description: Interface to GridMapper application.
		See file LICENSE for licensing information.
		Contact author at delta@superdan.net
*/
#ifndef GRIDMAPPER_H
#define GRIDMAPPER_H
#include <windows.h>
#include "GridMap.h"

// Function prototypes
void InitGridMapper();
void InitFirstMap();
unsigned GetGridSize();
unsigned GetHorzScrollPos();
unsigned GetVertScrollPos();
void UpdateEntireWindow();
void UpdateBkgdCell(GridCoord gc);
void SetScrollRange(bool zeroPos);
void HorzScrollHandler(WPARAM wParam);
void VertScrollHandler(WPARAM wParam);
void ScrollWheelHandler(WPARAM wParam);
void MyKeyHandler(WPARAM wParam);
void MyPaintWindow();
void MyLButtonHandler(LPARAM lParam);
void FloorSelect(FloorType floor, POINT p);
void ObjectSelect(ObjectType object, POINT p);
void WallSelect(WallType wall, POINT p);
void ChangeWestWall(GridCoord gc, int newFeature);
void ChangeNorthWall(GridCoord gc, int newFeature);
void ClearMap(bool open);
void FillCell(GridCoord gc);
void SetSelectedFeature(int feature);
bool OkDiscardChanges();
void SetBkgdDC();
void ChangeGridSize(int size);
void SetNewMap(GridMap *newmap);
bool NewMapFromSpecs(int newWidth, int newHeight);
bool NewMapFromFile(char *filename);
void OpenMap();
void SaveMapAs();
void SaveMap();
void CopyMap();
void PrintMap();
void ToggleGridLines();
void ToggleRoughEdges();
void DestroyObjects();
bool ProcessCommand(int cmdId);
GridCoord GetGridCoordFromWindow(POINT p);
FloorType GetFloorTypeFromMenu(int menuID);
WallType GetWallTypeFromMenu(int menuID);
ObjectType GetObjectTypeFromMenu(int menuID);
#endif
