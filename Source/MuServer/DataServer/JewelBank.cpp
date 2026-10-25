#include "stdafx.h"
#include "JewelBank.h"
#include "QueryManager.h"
#include "SocketManager.h"

CJewelBank gJewelBank;

CJewelBank::CJewelBank()
{
}

CJewelBank::~CJewelBank()
{
}

void CJewelBank::GDJewelBankGetRecv(SDHP_JEWEL_BANK_GET_SEND* lpMsg, int index)
{
	SDHP_JEWEL_BANK_GET_RECV pMsg;
	memset(&pMsg, 0, sizeof(pMsg));

	pMsg.header.set(0x08, 0x00, sizeof(pMsg));

	pMsg.index = lpMsg->index;

	char safeAccount[12] = { 0 };
	memcpy(safeAccount, lpMsg->account, sizeof(lpMsg->account));

	memcpy(pMsg.account, safeAccount, sizeof(pMsg.account));

	pMsg.Bless = 0;
	pMsg.Soul = 0;
	pMsg.Chaos = 0;
	pMsg.Life = 0;
	pMsg.Creation = 0;

#ifndef MYSQL
	if (gQueryManager.ExecQuery("SELECT Bless, Soul, Chaos, Life, Creation FROM JewelBank WHERE AccountID = '%s'", safeAccount) == false || gQueryManager.Fetch() == SQL_NO_DATA)
#else
	if (gQueryManager.ExecResultQuery("SELECT Bless, Soul, Chaos, Life, Creation FROM JewelBank WHERE AccountID = '%s'", safeAccount) == false || gQueryManager.Fetch() == false)
#endif
	{
		gQueryManager.Close();

#ifndef MYSQL
		gQueryManager.ExecQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', 0, 0, 0, 0, 0)", safeAccount);
#else
		gQueryManager.ExecUpdateQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', 0, 0, 0, 0, 0)", safeAccount);
#endif
		gQueryManager.Close();
	}
	else
	{
		pMsg.Bless = (DWORD)gQueryManager.GetAsInteger64("Bless");
		pMsg.Soul = (DWORD)gQueryManager.GetAsInteger64("Soul");
		pMsg.Chaos = (DWORD)gQueryManager.GetAsInteger64("Chaos");
		pMsg.Life = (DWORD)gQueryManager.GetAsInteger64("Life");
		pMsg.Creation = (DWORD)gQueryManager.GetAsInteger64("Creation");

		gQueryManager.Close();
	}

	gSocketManager.DataSend(index, (BYTE*)&pMsg, pMsg.header.size);
}

void CJewelBank::GDJewelBankSaveRecv(SDHP_JEWEL_BANK_SAVE_SEND* lpMsg)
{
	char safeAccount[12] = { 0 };
	memcpy(safeAccount, lpMsg->account, sizeof(lpMsg->account));

#ifndef MYSQL
	gQueryManager.ExecQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', %u, %u, %u, %u, %u) ON DUPLICATE KEY UPDATE Bless = %u, Soul = %u, Chaos = %u, Life = %u, Creation = %u",
		safeAccount, lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation,
		lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation);
#else
	gQueryManager.ExecUpdateQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', %u, %u, %u, %u, %u) ON DUPLICATE KEY UPDATE Bless = %u, Soul = %u, Chaos = %u, Life = %u, Creation = %u",
		safeAccount, lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation,
		lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation);
#endif

	gQueryManager.Close();
}
