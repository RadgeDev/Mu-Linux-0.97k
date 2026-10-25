#include "stdafx.h"
#include "JewelBank.h"
#include "GameMain.h"
#include "DSProtocol.h"
#include "ItemManager.h"
#include "Notice.h"
#include "Util.h"

CJewelBank gJewelBank;

CJewelBank::CJewelBank()
{
}

CJewelBank::~CJewelBank()
{
}

int CJewelBank::GetJewelIndex(BYTE type)
{
	switch (type)
	{
		case JEWEL_TYPE_BLESS: return GET_ITEM(14, 13);
		case JEWEL_TYPE_SOUL:  return GET_ITEM(14, 14);
		case JEWEL_TYPE_CHAOS: return GET_ITEM(12, 15);
		case JEWEL_TYPE_LIFE:  return GET_ITEM(14, 16);
		case JEWEL_TYPE_CREATION: return GET_ITEM(14, 22);
		default: return -1;
	}
}

void CJewelBank::NpcJewelBank(int iNpcIndex, int iObjIndex)
{
	if (gObjIsConnectedGP(iObjIndex) == 0)
	{
		return;
	}

	LPOBJ lpObj = &gObj[iObjIndex];

	if (lpObj->Interface.use != 0)
	{
		return;
	}

	lpObj->Interface.use = 1;
	lpObj->Interface.type = INTERFACE_JEWEL_BANK;
	lpObj->Interface.state = 0;

	this->GCJewelBankInfoSend(iObjIndex);
}

void CJewelBank::GCJewelBankInfoSend(int aIndex)
{
	LPOBJ lpObj = &gObj[aIndex];

	PMSG_JEWEL_BANK_INFO_SEND pMsg;

	pMsg.header.set(0xF8, 0x20, sizeof(pMsg));

	pMsg.Bless = lpObj->JewelBankBless;
	pMsg.Soul  = lpObj->JewelBankSoul;
	pMsg.Chaos = lpObj->JewelBankChaos;
	pMsg.Life  = lpObj->JewelBankLife;
	pMsg.Creation = lpObj->JewelBankCreation;

	DataSend(aIndex, (BYTE*)&pMsg, pMsg.header.size);
}

void CJewelBank::CGJewelBankCloseRecv(int aIndex)
{
	if (OBJECT_RANGE(aIndex) == 0)
	{
		return;
	}

	LPOBJ lpObj = &gObj[aIndex];

	if (lpObj->Interface.type == INTERFACE_JEWEL_BANK)
	{
		lpObj->Interface.use = 0;
		lpObj->Interface.type = INTERFACE_NONE;
		lpObj->Interface.state = 0;
	}
}

void CJewelBank::CGJewelBankActionRecv(PMSG_JEWEL_BANK_ACTION_RECV* lpMsg, int aIndex)
{
	if (gObjIsConnectedGP(aIndex) == 0)
	{
		return;
	}

	LPOBJ lpObj = &gObj[aIndex];

	if (lpObj->Interface.use == 0 || lpObj->Interface.type != INTERFACE_JEWEL_BANK)
	{
		return;
	}

	if (lpObj->JewelBankBusy)
	{
		this->GCJewelBankResultSend(aIndex, JEWEL_BANK_RESULT_BUSY, lpMsg->Action, lpMsg->JewelType, 0);
		return;
	}

	if (lpMsg->JewelType >= JEWEL_TYPE_MAX)
	{
		this->GCJewelBankResultSend(aIndex, JEWEL_BANK_RESULT_INVALID_ITEM, lpMsg->Action, lpMsg->JewelType, 0);
		return;
	}

	if (lpMsg->Action == JEWEL_BANK_ACTION_DEPOSIT)
	{
		this->ProcessDeposit(lpObj, lpMsg->JewelType, lpMsg->Count);
	}
	else if (lpMsg->Action == JEWEL_BANK_ACTION_WITHDRAW)
	{
		this->ProcessWithdraw(lpObj, lpMsg->JewelType, lpMsg->Count);
	}
	else
	{
		this->GCJewelBankResultSend(aIndex, JEWEL_BANK_RESULT_INVALID_ITEM, lpMsg->Action, lpMsg->JewelType, 0);
	}
}

void CJewelBank::ProcessDeposit(LPOBJ lpObj, BYTE JewelType, DWORD RequestedCount)
{
	int itemIndex = this->GetJewelIndex(JewelType);
	if (itemIndex == -1)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_INVALID_ITEM, JEWEL_BANK_ACTION_DEPOSIT, JewelType, 0);
		return;
	}

	int availableCount = gItemManager.GetInventoryItemCount(lpObj, itemIndex, 0);
	if (availableCount <= 0)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_NO_BALANCE, JEWEL_BANK_ACTION_DEPOSIT, JewelType, 0);
		return;
	}

	DWORD countToDeposit = RequestedCount;
	if (countToDeposit == 0 || countToDeposit > (DWORD)availableCount)
	{
		countToDeposit = (DWORD)availableCount;
	}

	DWORD* pBalance = nullptr;
	switch (JewelType)
	{
		case JEWEL_TYPE_BLESS: pBalance = &lpObj->JewelBankBless; break;
		case JEWEL_TYPE_SOUL:  pBalance = &lpObj->JewelBankSoul; break;
		case JEWEL_TYPE_CHAOS: pBalance = &lpObj->JewelBankChaos; break;
		case JEWEL_TYPE_LIFE:  pBalance = &lpObj->JewelBankLife; break;
		case JEWEL_TYPE_CREATION: pBalance = &lpObj->JewelBankCreation; break;
		default: return;
	}

	if (*pBalance + countToDeposit < *pBalance)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_INVALID_ITEM, JEWEL_BANK_ACTION_DEPOSIT, JewelType, 0);
		return;
	}

	lpObj->JewelBankBusy = true;

	gItemManager.DeleteInventoryItemCount(lpObj, itemIndex, 0, (int)countToDeposit);

	*pBalance += countToDeposit;

	this->GDJewelBankSaveSend(lpObj->Index);
	GDInventoryItemSaveSend(lpObj->Index);

	gItemManager.GCItemListSend(lpObj->Index);

	lpObj->JewelBankBusy = false;

	this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_SUCCESS, JEWEL_BANK_ACTION_DEPOSIT, JewelType, countToDeposit);
}

void CJewelBank::ProcessWithdraw(LPOBJ lpObj, BYTE JewelType, DWORD RequestedCount)
{
	int itemIndex = this->GetJewelIndex(JewelType);
	if (itemIndex == -1 || RequestedCount == 0)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_INVALID_ITEM, JEWEL_BANK_ACTION_WITHDRAW, JewelType, 0);
		return;
	}

	DWORD* pBalance = nullptr;
	switch (JewelType)
	{
		case JEWEL_TYPE_BLESS: pBalance = &lpObj->JewelBankBless; break;
		case JEWEL_TYPE_SOUL:  pBalance = &lpObj->JewelBankSoul; break;
		case JEWEL_TYPE_CHAOS: pBalance = &lpObj->JewelBankChaos; break;
		case JEWEL_TYPE_LIFE:  pBalance = &lpObj->JewelBankLife; break;
		case JEWEL_TYPE_CREATION: pBalance = &lpObj->JewelBankCreation; break;
		default: return;
	}

	if (*pBalance < RequestedCount)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_NO_BALANCE, JEWEL_BANK_ACTION_WITHDRAW, JewelType, 0);
		return;
	}

	DWORD countToWithdraw = RequestedCount;
	if (countToWithdraw > 32)
	{
		countToWithdraw = 32;
	}

	int emptySlots = gItemManager.GetInventoryEmptySlotCount(lpObj);
	if (emptySlots < (int)countToWithdraw)
	{
		this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_NO_SPACE, JEWEL_BANK_ACTION_WITHDRAW, JewelType, 0);
		return;
	}

	lpObj->JewelBankBusy = true;

	*pBalance -= countToWithdraw;

	DWORD delivered = 0;
	for (DWORD i = 0; i < countToWithdraw; i++)
	{
		CItem item;
		item.Convert(itemIndex, 0, 0, 0, 0);
		BYTE slot = gItemManager.InventoryInsertItem(lpObj->Index, item);
		if (slot != 0xFF)
		{
			delivered++;
		}
	}

	if (delivered < countToWithdraw)
	{
		*pBalance += (countToWithdraw - delivered);
	}

	this->GDJewelBankSaveSend(lpObj->Index);
	GDInventoryItemSaveSend(lpObj->Index);

	gItemManager.GCItemListSend(lpObj->Index);

	lpObj->JewelBankBusy = false;

	this->GCJewelBankResultSend(lpObj->Index, JEWEL_BANK_RESULT_SUCCESS, JEWEL_BANK_ACTION_WITHDRAW, JewelType, delivered);
}

void CJewelBank::GDJewelBankGetSend(int aIndex)
{
	LPOBJ lpObj = &gObj[aIndex];

	if (gObjIsAccountValid(aIndex, lpObj->Account) == 0)
	{
		return;
	}

	SDHP_JEWEL_BANK_GET_SEND pMsg;

	pMsg.header.set(0x08, 0x00, sizeof(pMsg));

	pMsg.index = aIndex;

	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));

	gDataServerConnection.DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void CJewelBank::DGJewelBankGetRecv(SDHP_JEWEL_BANK_GET_RECV* lpMsg)
{
	if (OBJECT_RANGE(lpMsg->index) == 0)
	{
		return;
	}

	LPOBJ lpObj = &gObj[lpMsg->index];

	if (gObjIsAccountValid(lpMsg->index, lpMsg->account) == 0)
	{
		return;
	}

	lpObj->JewelBankBless = lpMsg->Bless;
	lpObj->JewelBankSoul = lpMsg->Soul;
	lpObj->JewelBankChaos = lpMsg->Chaos;
	lpObj->JewelBankLife = lpMsg->Life;
	lpObj->JewelBankCreation = lpMsg->Creation;
	lpObj->JewelBankLoaded = true;
	lpObj->JewelBankBusy = false;
	lpObj->JewelBankPendingWithdrawCount = 0;
}

void CJewelBank::GDJewelBankSaveSend(int aIndex)
{
	LPOBJ lpObj = &gObj[aIndex];

	if (gObjIsAccountValid(aIndex, lpObj->Account) == 0)
	{
		return;
	}

	SDHP_JEWEL_BANK_SAVE_SEND pMsg;

	pMsg.header.set(0x08, 0x01, sizeof(pMsg));

	pMsg.index = aIndex;

	memcpy(pMsg.account, lpObj->Account, sizeof(pMsg.account));

	pMsg.Bless = lpObj->JewelBankBless;
	pMsg.Soul  = lpObj->JewelBankSoul;
	pMsg.Chaos = lpObj->JewelBankChaos;
	pMsg.Life  = lpObj->JewelBankLife;
	pMsg.Creation = lpObj->JewelBankCreation;

	gDataServerConnection.DataSend((BYTE*)&pMsg, pMsg.header.size);
}

void CJewelBank::GCJewelBankResultSend(int aIndex, BYTE Result, BYTE Action, BYTE JewelType, DWORD Count)
{
	LPOBJ lpObj = &gObj[aIndex];

	PMSG_JEWEL_BANK_RESULT_SEND pMsg;

	pMsg.header.set(0xF8, 0x22, sizeof(pMsg));

	pMsg.Result = Result;
	pMsg.Action = Action;
	pMsg.JewelType = JewelType;
	pMsg.Count = Count;
	pMsg.Bless = lpObj->JewelBankBless;
	pMsg.Soul  = lpObj->JewelBankSoul;
	pMsg.Chaos = lpObj->JewelBankChaos;
	pMsg.Life  = lpObj->JewelBankLife;
	pMsg.Creation = lpObj->JewelBankCreation;

	DataSend(aIndex, (BYTE*)&pMsg, pMsg.header.size);
}
