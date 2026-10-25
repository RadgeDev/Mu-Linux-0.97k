#pragma once

#include "ProtocolDefines.h"
#include "User.h"

enum eJewelBankAction
{
	JEWEL_BANK_ACTION_DEPOSIT = 1,
	JEWEL_BANK_ACTION_WITHDRAW = 2,
	JEWEL_BANK_ACTION_DEPOSIT_ALL = 3,
};

enum eJewelBankResult
{
	JEWEL_BANK_RESULT_SUCCESS = 0,
	JEWEL_BANK_RESULT_NO_BALANCE = 1,
	JEWEL_BANK_RESULT_NO_SPACE = 2,
	JEWEL_BANK_RESULT_INVALID_ITEM = 3,
	JEWEL_BANK_RESULT_BUSY = 4,
};

enum eJewelType
{
	JEWEL_TYPE_BLESS = 0,
	JEWEL_TYPE_SOUL = 1,
	JEWEL_TYPE_CHAOS = 2,
	JEWEL_TYPE_LIFE = 3,
	JEWEL_TYPE_CREATION = 4,
	JEWEL_TYPE_MAX = 5,
};

#pragma pack(push, 1)

struct PMSG_JEWEL_BANK_INFO_SEND
{
	PSBMSG_HEAD header; // 0xF8, 0x20
	DWORD Bless;
	DWORD Soul;
	DWORD Chaos;
	DWORD Life;
	DWORD Creation;
};

struct PMSG_JEWEL_BANK_ACTION_RECV
{
	PSBMSG_HEAD header; // 0xF8, 0x21
	BYTE Action;
	BYTE JewelType;
	DWORD Count;
};

struct PMSG_JEWEL_BANK_RESULT_SEND
{
	PSBMSG_HEAD header; // 0xF8, 0x22
	BYTE Result;
	BYTE Action;
	BYTE JewelType;
	DWORD Count;
	DWORD Bless;
	DWORD Soul;
	DWORD Chaos;
	DWORD Life;
	DWORD Creation;
};

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

	int GetJewelIndex(BYTE JewelType);

	void NpcJewelBank(int iNpcIndex, int iObjIndex);

	void CGJewelBankActionRecv(PMSG_JEWEL_BANK_ACTION_RECV* lpMsg, int aIndex);
	void CGJewelBankCloseRecv(int aIndex);

	void GDJewelBankGetSend(int aIndex);
	void DGJewelBankGetRecv(SDHP_JEWEL_BANK_GET_RECV* lpMsg);
	void GDJewelBankSaveSend(int aIndex);

	void GCJewelBankInfoSend(int aIndex);
	void GCJewelBankResultSend(int aIndex, BYTE Result, BYTE Action, BYTE JewelType, DWORD Count);

private:
	void ProcessDeposit(LPOBJ lpObj, BYTE JewelType, DWORD RequestedCount);
	void ProcessDepositAll(LPOBJ lpObj);
	void ProcessWithdraw(LPOBJ lpObj, BYTE JewelType, DWORD RequestedCount);
};

extern CJewelBank gJewelBank;
