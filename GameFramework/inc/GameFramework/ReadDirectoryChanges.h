
#pragma once

#include "CThreadSafeQueue.h"

#include <string> // For std::wstring
#include <utility> // For std::pair

typedef std::pair<DWORD,std::wstring> TDirectoryChangeNotification;

namespace ReadDirectoryChangesPrivate
{
	class CReadChangesServer;
}


class CReadDirectoryChanges
{
public:
	CReadDirectoryChanges(int nMaxChanges=1000);
	~CReadDirectoryChanges();

	void Init();
	void Terminate();

	
	void AddDirectory( const std::wstring& wszDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize=16384 );

	/// <summary>
	/// Return a handle for the Win32 Wait... functions that will be
	/// signaled when there is a queue entry.
	/// </summary>
	HANDLE GetWaitHandle() { return m_Notifications.GetWaitHandle(); }

	bool Pop(DWORD& dwAction, std::wstring& wstrFilename);

	// "Push" is for usage by ReadChangesRequest.  Not intended for external usage.
	void Push(DWORD dwAction, const std::wstring& wstrFilename);

	// Check if the queue overflowed. If so, clear it and return true.
	bool CheckOverflow();

	unsigned int GetThreadId() { return m_dwThreadId; }

protected:
	ReadDirectoryChangesPrivate::CReadChangesServer* m_pServer;

	HANDLE m_hThread;

	unsigned int m_dwThreadId;

	CThreadSafeQueue<TDirectoryChangeNotification> m_Notifications;
};
