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
void FloorSelect(HWND hWnd, int xPos, int yPos);
void WallSelect(HWND hWnd, int xPos, int yPos);
void ChangeWestWall(HWND hWnd, int x, int y, int newFeature);
void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature);
void ClearMap(HWND hWnd, bool clear);
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
LRESULT CALLBACK NewDialog(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GridSizeDialog(HWND, UINT, WPARAM, LPARAM);
#endif
