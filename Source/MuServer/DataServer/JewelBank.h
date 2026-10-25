#pragma once

#include "ProtocolDefines.h"

#pragma pack(push, 1)

struct SDHP_JEWEL_BANK_GET_SEND
{
	PSBMSG_HEAD header; // 0x08, 0x00
	WORD index;
	char account[11];
};

struct SDHP_JEWEL_BANK_GET_RECV
{
	PSBMSG_HEAD header; // 0x08, 0x00
	WORD index;
	char account[11];
	DWORD Bless;
	DWORD Soul;
	DWORD Chaos;
	DWORD Life;
	DWORD Creation;
};

struct SDHP_JEWEL_BANK_SAVE_SEND
{
	PSBMSG_HEAD header; // 0x08, 0x01
	WORD index;
	char account[11];
	DWORD Bless;
	DWORD Soul;
	DWORD Chaos;
	DWORD Life;
	DWORD Creation;
};

#pragma pack(pop)

class CJewelBank
{
public:
	CJewelBank();
	virtual ~CJewelBank();

	void GDJewelBankGetRecv(SDHP_JEWEL_BANK_GET_SEND* lpMsg, int index);
	void GDJewelBankSaveRecv(SDHP_JEWEL_BANK_SAVE_SEND* lpMsg);
};

extern CJewelBank gJewelBank;
