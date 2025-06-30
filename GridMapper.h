/*
=====================================================================
	GRIDMAPPER.H 
    Interface to GridMapper application.
	Copyright (c) 2010 Daniel R. Collins. All rights reserved.
	See the bottom of this file for any licensing information.
=====================================================================
*/

#ifndef GRIDMAPPER_H
#define GRIDMAPPER_H

#include "Resource.h"

// Forward declarations (DRC)
int  GetGridSize();
void UpdateBkgdCell (HWND hWnd, int x, int y);
void UpdateEntireWindow (HWND hWnd);
void SetScrollRange (HWND hWnd, bool zeroPos);
void HorzScrollHandler(HWND hWnd, WPARAM wParam);
void VertScrollHandler(HWND hWnd, WPARAM wParam);
int  GetHorzScrollPos (HWND hWnd);
int  GetVertScrollPos (HWND hWnd);
void MyKeyHandler(HWND hWnd, WPARAM wParam); 
void MyPaintWindow (HWND hWnd);
void MyLButtonHandler (HWND hWnd, LPARAM lParam);
void FloorSelect (HWND hWnd, int xPos, int yPos);
void WallSelect (HWND hWnd, int xPos, int yPos);
void ChangeWestWall(HWND hWnd, int x, int y, int newFeature);
void ChangeNorthWall(HWND hWnd, int x, int y, int newFeature);
void ClearMap(HWND hWnd, bool clear);
void FillCell(HWND hWnd, int x, int y);
void SetSelectedFeature (HWND hWnd, int feature);
bool OkDiscardChanges(HWND hWnd);
void SetBkgdDC (HWND hWnd);
void ChangeGridSize (HWND hWnd, int size);
void SetNewMap(HWND hWnd, GridMap *newmap);
bool NewMapFromSpecs (HWND hWnd, int newWidth, int newHeight);
bool NewMapFromFile (HWND hWnd, char *filename);
void OpenMap (HWND hWnd);
void SaveMapAs (HWND hWnd);
void SaveMap (HWND hWnd);
void CopyMap (HWND hWnd);
void PrintMap (HWND hWnd);

LRESULT CALLBACK NewDialog (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GridSizeDialog (HWND, UINT, WPARAM, LPARAM);

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

