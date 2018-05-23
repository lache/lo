#include <czmq.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "lwcontext.h"
#include "lwlog.h"
#include "logic.h"

void RefreshDirectory(LPTSTR);
void RefreshTree(LPTSTR);
void WatchDirectory(LWCONTEXT*, LPTSTR);

static void s_scriptwatch_worker(zsock_t *pipe, void *args) {
	LWCONTEXT* pLwc = args;
	// Send 'worker ready' signal to parent thread
	zsock_signal(pipe, 0);
	// Start watching...
	WatchDirectory(pLwc, TEXT("C:\\w\\src\\github.com\\lache\\lo\\laidoff\\assets\\l"));
	// Reactor loop finished.
	// Send 'worker finished' signal to parent thread
	zsock_signal(pipe, 0);
}

void lwc_start_scriptwatch_thread(LWCONTEXT* pLwc) {
	// Start scriptwatch thread
	pLwc->scriptwatch_actor = zactor_new(s_scriptwatch_worker, pLwc);
}


void WatchDirectory(LWCONTEXT* pLwc, LPTSTR lpDir) {
	HANDLE hDir = CreateFile(
		lpDir,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	int nCounter = 0;
	FILE_NOTIFY_INFORMATION strFileNotifyInfo[1024];
	DWORD dwBytesReturned = 0;

	while (TRUE) {
		if (ReadDirectoryChangesW(hDir, (LPVOID)&strFileNotifyInfo, sizeof(strFileNotifyInfo), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &dwBytesReturned, NULL, NULL) == 0) {
			LOGE(TEXT("Reading Directory Change"));
		} else {
			//LOGI(TEXT("File Modified: %s"), strFileNotifyInfo[0].FileName);
			LOGI(TEXT("Loop: %d"), nCounter++);

			reset_runtime_context_async(pLwc);
		}
	}
}

void WatchDirectory_XX(LWCONTEXT* pLwc, LPTSTR lpDir) {
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[2];
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];

	_tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';

	// Watch the directory for file creation and deletion. 

	dwChangeHandles[0] = FindFirstChangeNotification(
		lpDir,                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE) {
		LOGE("ERROR: FindFirstChangeNotification function failed.");
		ExitProcess(GetLastError());
	}

	// Watch the subtree for directory creation and deletion. 

	dwChangeHandles[1] = FindFirstChangeNotification(
		lpDrive,                       // directory to watch 
		TRUE,                          // watch the subtree 
		FILE_NOTIFY_CHANGE_DIR_NAME);  // watch dir name changes 

	if (dwChangeHandles[1] == INVALID_HANDLE_VALUE) {
		LOGE("ERROR: FindFirstChangeNotification function failed.");
		ExitProcess(GetLastError());
	}


	// Make a final validation check on our handles.

	if ((dwChangeHandles[0] == NULL) || (dwChangeHandles[1] == NULL)) {
		LOGE("ERROR: Unexpected NULL from FindFirstChangeNotification.");
		ExitProcess(GetLastError());
	}

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 

	while (TRUE) {
		// Wait for notification.

		//LOGI("Waiting for notification...");

		dwWaitStatus = WaitForMultipleObjects(2, dwChangeHandles,
			FALSE, 500/*INFINITE*/);

		switch (dwWaitStatus) {
		case WAIT_OBJECT_0:

			// A file was created, renamed, or deleted in the directory.
			// Refresh this directory and restart the notification.

			RefreshDirectory(lpDir);
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE) {
				LOGE("ERROR: FindNextChangeNotification function failed.");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1:

			// A directory was created, renamed, or deleted.
			// Refresh the tree and restart the notification.

			RefreshTree(lpDrive);
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE) {
				LOGE("ERROR: FindNextChangeNotification function failed.");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			//printf("\nNo changes in the timeout period.\n");

			if (pLwc->quit_request) {
				//zloop_timer_end(loop, timer_id);
				// End the reactor
				return;
			}

			break;

		default:
			LOGE("ERROR: Unhandled dwWaitStatus.");
			ExitProcess(GetLastError());
			break;
		}
	}
}

void RefreshDirectory(LPTSTR lpDir) {
	// This is where you might place code to refresh your
	// directory listing, but not the subtree because it
	// would not be necessary.

	LOGI(TEXT("Directory (%s) changed.\n"), lpDir);
}

void RefreshTree(LPTSTR lpDrive) {
	// This is where you might place code to refresh your
	// directory listing, including the subtree.

	LOGI(TEXT("Directory tree (%s) changed.\n"), lpDrive);
}
