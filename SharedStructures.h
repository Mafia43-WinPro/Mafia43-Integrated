// SharedStructures.h - Common data structures shared across dialogs
#pragma once

#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H

// Note: This header requires MFC headers (CString) to be included first via pch.h

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

#endif // SHARED_STRUCTURES_H
