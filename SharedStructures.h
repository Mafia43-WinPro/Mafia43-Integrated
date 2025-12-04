// SharedStructures.h - Common data structures shared across dialogs
#pragma once

#include <afxwin.h>

// Player information structure used in lobby/room
struct RoomPlayerInfo {
	CString strUID;
	CString strName;
	bool bIsAlive;
	bool bIsHost;
};

// Player information structure used in night phase
struct PlayerInfo {
	int id;
	CString name;
	bool alive;
};
