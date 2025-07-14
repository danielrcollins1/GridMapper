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
int  GetGridSize();
void UpdateBkgdCell(HWND hWnd, int x, int y);
void UpdateEntireWindow(HWND hWnd);
void SetScrollRange(HWND hWnd, bool zeroPos);
void HorzScrollHandler(HWND hWnd, WPARAM wParam);
void VertScrollHandler(HWND hWnd, WPARAM wParam);
void ScrollWheelHandler(HWND hWnd, WPARAM wParam);
int GetHorzScrollPos(HWND hWnd);
int GetVertScrollPos(HWND hWnd);
void MyKeyHandler(HWND hWnd, WPARAM wParam);
void MyPaintWindow(HWND hWnd);
void MyLButtonHandler(HWND hWnd, LPARAM lParam);
void FloorSelect(HWND hWnd, FloorType floor, int xPos, int yPos);
void ObjectSelect(HWND hWnd, ObjectType object, int xPos, int yPos);
void WallSelect(HWND hWnd, WallType wall, int xPos, int yPos);
void ChangeWestWall(HWND hWnd, int x, int y, int newFeature);
void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature);
void ClearMap(HWND hWnd, bool open);
void FillCell(HWND hWnd, int x, int y);
void SetSelectedFeature(HWND hWnd, int feature);
bool OkDiscardChanges(HWND hWnd);
void SetBkgdDC(HWND hWnd);
void ChangeGridSize(HWND hWnd, int size);
void SetNewMap(HWND hWnd, GridMap *newmap);
bool NewMapFromSpecs(HWND hWnd, int newWidth, int newHeight);
bool NewMapFromFile(HWND hWnd, char *filename);
void OpenMap(HWND hWnd);
void SaveMapAs(HWND hWnd);
void SaveMap(HWND hWnd);
void CopyMap(HWND hWnd);
void PrintMap(HWND hWnd);
void ToggleGridLines(HWND hWnd);
void ToggleRoughEdges(HWND hWnd);
void GetMapCoordsFromWindow(int xWin, int yWin, int& xMap, int& yMap);
LRESULT CALLBACK NewDialog(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GridSizeDialog(HWND, UINT, WPARAM, LPARAM);
FloorType GetFloorTypeFromMenu(int menuID);
WallType GetWallTypeFromMenu(int menuID);
ObjectType GetObjectTypeFromMenu(int menuID);
#endif
