// SharedStructures.h - Common data structures shared across dialogs
#pragma once

#ifndef SHARED_STRUCTURES_H
#define SHARED_STRUCTURES_H

// Note: This header requires MFC headers (CString) to be included first via pch.h

// Player information structure used in lobby/room
struct RoomPlayerInfo {
	CString strUID;
	CString strName;
	int nPlayerNumber;  // 서버에서 할당한 플레이어 번호 (1, 2, 3...)
	bool bIsAlive;
	bool bIsHost;
};

// Player information structure used in night phase
struct PlayerInfo {
	int id;
	int nPlayerNumber;  // 플레이어 번호
	CString name;
	bool alive;
};

#endif // SHARED_STRUCTURES_H
