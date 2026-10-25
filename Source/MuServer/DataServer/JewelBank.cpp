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

	pMsg.header.set(0x08, 0x00, sizeof(pMsg));

	pMsg.index = lpMsg->index;

	memcpy(pMsg.account, lpMsg->account, sizeof(pMsg.account));

	pMsg.Bless = 0;
	pMsg.Soul = 0;
	pMsg.Chaos = 0;
	pMsg.Life = 0;
	pMsg.Creation = 0;

#ifndef MYSQL
	if (gQueryManager.ExecQuery("SELECT Bless, Soul, Chaos, Life, Creation FROM JewelBank WHERE AccountID = '%s'", lpMsg->account) == false || gQueryManager.Fetch() == SQL_NO_DATA)
#else
	if (gQueryManager.ExecResultQuery("SELECT Bless, Soul, Chaos, Life, Creation FROM JewelBank WHERE AccountID = '%s'", lpMsg->account) == false || gQueryManager.Fetch() == false)
#endif
	{
		gQueryManager.Close();

#ifndef MYSQL
		gQueryManager.ExecQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', 0, 0, 0, 0, 0)", lpMsg->account);
#else
		gQueryManager.ExecUpdateQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', 0, 0, 0, 0, 0)", lpMsg->account);
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
#ifndef MYSQL
	gQueryManager.ExecQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', %u, %u, %u, %u, %u) ON DUPLICATE KEY UPDATE Bless = %u, Soul = %u, Chaos = %u, Life = %u, Creation = %u",
		lpMsg->account, lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation,
		lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation);
#else
	gQueryManager.ExecUpdateQuery("INSERT INTO JewelBank (AccountID, Bless, Soul, Chaos, Life, Creation) VALUES ('%s', %u, %u, %u, %u, %u) ON DUPLICATE KEY UPDATE Bless = %u, Soul = %u, Chaos = %u, Life = %u, Creation = %u",
		lpMsg->account, lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation,
		lpMsg->Bless, lpMsg->Soul, lpMsg->Chaos, lpMsg->Life, lpMsg->Creation);
#endif

	gQueryManager.Close();
}
