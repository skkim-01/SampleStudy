/*
Copyright (c) 2012, Felix J. Ogris
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * EventLogReader - reads and displays entries from the event log on Windows >= 2000
 * $Rev: 139 $
 */

#pragma once

#include <UserEnv.h>
#include <winevt.h>

#if WINVER<=0x502
 /**********************************************************************/
 /* up to XP and 2003                                                  */
 /**********************************************************************/

#define REGISTRY_EVENTLOG_PATH L"SYSTEM\\CurrentControlSet\\Services\\Eventlog\\"
#define CATEGORY_MESSAGE_FILE L"CategoryMessageFile"
#define EVENT_MESSAGE_FILE L"EventMessageFile"
#define PARAMETER_MESSAGE_FILE L"ParameterMessageFile"

/* returns content of registry key <appFolder>\\<fileName>
   return value must be free()'ed if not NULL */
LPWSTR LoadMessageFilePaths(HKEY appFolder, LPWSTR fileName)
{
	DWORD pathsLen = 32768 - sizeof(WCHAR);
	LPWSTR paths = (LPWSTR)malloc(pathsLen + sizeof(WCHAR));
	LONG error;
	DWORD expandedPathsLen;
	LPWSTR expandedPaths;

	for (;;) {
		if (paths == NULL)
			return NULL;

		error = RegQueryValueExW(appFolder, fileName, 0, NULL, (LPBYTE)paths, &pathsLen);

		if (error == ERROR_SUCCESS)
			break;

		if (error != ERROR_MORE_DATA) {
			free(paths);
			return NULL;
		}

		paths = (LPWSTR)realloc(paths, pathsLen + sizeof(WCHAR));
	}

	pathsLen /= sizeof(WCHAR);
	wmemcpy(paths + pathsLen, L"\0", 1);
	expandedPathsLen = 2 * pathsLen;
	expandedPaths = (LPWSTR)malloc(expandedPathsLen * sizeof(WCHAR));

	for (;;) {
		if (expandedPaths == NULL)
			return NULL;

		if (ExpandEnvironmentStringsForUserW(NULL, paths, expandedPaths, expandedPathsLen)) {
			free(paths);
			return expandedPaths;
		}

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			free(paths);
			free(expandedPaths);
			return NULL;
		}

		expandedPathsLen *= 10;
		expandedPaths = (LPWSTR)realloc(expandedPaths, expandedPathsLen * sizeof(WCHAR));
	}
}

/* calls FormatMessage() in a memory allocation loop
   return valuee must be free()'ed if not NULL */
LPWSTR AllocFormatMessage(DWORD flags, LPCVOID source, DWORD messageId, LPWSTR* arguments)
{
	DWORD messageLen = 32768;
	LPWSTR message = (LPWSTR)malloc(messageLen * sizeof(WCHAR));

	for (;;) {
		if (message == NULL)
			return NULL;

		if (FormatMessageW(flags, source, messageId, 0, message, messageLen, (va_list*)arguments) > 0)
			return message;

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			free(message);
			return NULL;
		}

		messageLen *= 10;
		message = (LPWSTR)realloc(message, messageLen * sizeof(WCHAR));
	}
}

/* loads <numMessages> <messageIds> from the file(s) <appFolder>\\<fileName> points to
   returned string in <messages> must be free()'ed if not NULL */
void LoadMessages(HKEY appFolder, LPWSTR fileName, DWORD numMessages, DWORD* messageIds, LPWSTR* messages)
{
	LPWSTR paths;
	LPWSTR path;
	LPWSTR semicolon;
	HMODULE library;
	DWORD flags = FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD i;
	DWORD foundMsgs;

	paths = LoadMessageFilePaths(appFolder, fileName);
	if (paths == NULL)
		return;

	path = paths;

	do {
		semicolon = wcschr(path, L';');
		if (semicolon != NULL)
			*semicolon = L'\0';

		library = LoadLibraryExW(path, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		if (library != NULL) {
			for (i = 0, foundMsgs = 0; (i < numMessages) && (foundMsgs < numMessages); i++) {
				if (messages[i] == NULL)
					messages[i] = AllocFormatMessage(flags, library, messageIds[i], NULL);
				if (messages[i] != NULL)
					foundMsgs++;
			}
			FreeLibrary(library);
		}

		path = semicolon + 1;
	} while ((path != NULL) && (semicolon != NULL) && (foundMsgs < numMessages));

	free(paths);
}

/* returns maximum value of all placeholders ('%' followed by digit(s)) in <message>
   stores placeholders in <placeHolderIds> and count of placeholders in <numPlaceHolder> if not NULL */
DWORD GetMaxPlaceHolder(LPWSTR message, DWORD* numPlaceHolder, DWORD* placeHolderIds)
{
	DWORD maxPlaceHolder = 0;
	DWORD curPlaceHolder;
	LPWSTR p;
	DWORD countPlaceHolder = 0;

	for (p = wcschr(message, L'%'); p != NULL; p = wcschr(p, L'%')) {
		p++;
		if (*p == L'%') {
			p++;
		}
		else if (iswdigit(*p) != 0) {
			for (curPlaceHolder = 0; iswdigit(*p) != 0; p++)
				curPlaceHolder = curPlaceHolder * 10 + *p - L'0';
			if (curPlaceHolder > maxPlaceHolder)
				maxPlaceHolder = curPlaceHolder;
			if (placeHolderIds != NULL)
				placeHolderIds[countPlaceHolder] = curPlaceHolder;
			countPlaceHolder++;
		}
	}

	if (numPlaceHolder != NULL)
		*numPlaceHolder = countPlaceHolder;

	return maxPlaceHolder;
}

/* replaces any placeholder in <message> with strings from <logRec> and
   returns the new message, which must be free()'ed if not NULL
   don't use <message> afterwards! */
LPWSTR ReplaceStrings(PEVENTLOGRECORD logRec, LPWSTR message)
{
	DWORD maxPlaceHolder;
	DWORD minPlaceHolder;
	LPWSTR* placeHolder;
	LPWSTR p;
	DWORD i;
	LPWSTR newMessage;

	maxPlaceHolder = GetMaxPlaceHolder(message, NULL, NULL);
	if (maxPlaceHolder == 0)
		return message;
	minPlaceHolder = min((DWORD)logRec->NumStrings, maxPlaceHolder);

	placeHolder = (LPWSTR*)malloc(maxPlaceHolder * sizeof(LPWSTR));
	if (placeHolder == NULL)
		return message;

	p = (LPWSTR)((LPBYTE)logRec + logRec->StringOffset);
	for (i = 0; i < minPlaceHolder; i++) {
		placeHolder[i] = p;
		p += wcslen(p) + 1;
	}

	for (; i < maxPlaceHolder; i++)
		placeHolder[i] = NULL;

	newMessage = AllocFormatMessage(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
		message, 0, placeHolder);
	free(placeHolder);
	free(message);

	return newMessage;
}

/* replaces any placeholder in <message> with parameter strings found
   in the file <appFolder>\\ParameterMessageFile points to and
   returns the new message, which must be free()'ed if not NULL
   don't use <message> afterwards! */
LPWSTR ReplaceParameters(HKEY appFolder, LPWSTR message)
{
	DWORD maxPlaceHolder;
	DWORD numPlaceHolder;
	LPWSTR* placeHolder;
	DWORD* placeHolderIds;
	LPWSTR newMessage;
	DWORD i;

	maxPlaceHolder = GetMaxPlaceHolder(message, &numPlaceHolder, NULL);
	if (maxPlaceHolder == 0)
		goto RP_ERROR_0;

	placeHolder = (LPWSTR*)malloc(maxPlaceHolder * sizeof(LPWSTR));
	if (placeHolder == NULL)
		goto RP_ERROR_0;

	for (i = 0; i < maxPlaceHolder; i++)
		placeHolder[i] = NULL;

	placeHolderIds = (DWORD*)malloc(numPlaceHolder * sizeof(DWORD));
	if (placeHolderIds == NULL)
		goto RP_ERROR_1;

	GetMaxPlaceHolder(message, NULL, placeHolderIds);
	LoadMessages(appFolder, PARAMETER_MESSAGE_FILE, numPlaceHolder, placeHolderIds, placeHolder);

	free(placeHolderIds);
	newMessage = AllocFormatMessage(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
		message, 0, placeHolder);

	for (i = 0; i < maxPlaceHolder; i++)
		if (placeHolder[i] != NULL)
			free(placeHolder[i]);

	free(placeHolder);
	free(message);
	message = newMessage;

RP_ERROR_1:
	free(placeHolder);

RP_ERROR_0:
	return message;
}

/* removes all null bytes between the strings in <logRec>
   returns pointer to first string */
LPWSTR ConcatStrings(PEVENTLOGRECORD logRec)
{
	LPWSTR start;
	LPWSTR p;
	WORD i;

	start = (LPWSTR)((LPBYTE)logRec + logRec->StringOffset);
	p = start;
	for (i = 1; i < logRec->NumStrings; i++) {
		p += wcslen(p);
		*p = L' ';
		p++;
	}

	return start;
}

/* returns textual representation of <eventTypeCode> */
LPCWSTR EventType(WORD eventTypeCode)
{
	switch (eventTypeCode) {
	case EVENTLOG_SUCCESS:
		return L"Success";
	case EVENTLOG_ERROR_TYPE:
		return L"Error";
	case EVENTLOG_WARNING_TYPE:
		return L"Warning";
	case EVENTLOG_INFORMATION_TYPE:
		return L"Information";
	case EVENTLOG_AUDIT_SUCCESS:
		return L"Audit success";
	case EVENTLOG_AUDIT_FAILURE:
		return L"Audit failure";
	default:
		return L"Unknown";
	}
}

/* parses and prints any event log entries in <buffer> */
void ParseLogBuffer(HKEY eventLogFolder, LPBYTE buffer, DWORD bytesRead)
{
	LPBYTE ende = buffer + bytesRead;
	PEVENTLOGRECORD logRec;
	LPWSTR sourceName;
	LPWSTR computerName;
	HKEY appFolder;
	DWORD messageIds[2];
	LPWSTR messages[2];
	LPWSTR theMessage;
	WORD i;

	for (logRec = (PEVENTLOGRECORD)buffer;
		(LPBYTE)logRec < ende;
		logRec = (PEVENTLOGRECORD)((LPBYTE)logRec + logRec->Length)) {

		sourceName = (LPWSTR)((LPBYTE)logRec + sizeof(EVENTLOGRECORD));
		computerName = sourceName + wcslen(sourceName) + 1;

		messages[0] = NULL;
		messages[1] = NULL;
		messageIds[0] = logRec->EventCategory;
		messageIds[1] = logRec->EventID;

		if (RegOpenKeyExW(eventLogFolder, sourceName, 0, KEY_READ, &appFolder) == ERROR_SUCCESS) {
			/* event source (dll, exe, or so) found in the registry */
			LoadMessages(appFolder, CATEGORY_MESSAGE_FILE, 1, messageIds, messages);
			LoadMessages(appFolder, EVENT_MESSAGE_FILE, 2, messageIds, messages);
			if (messages[1] != NULL) {
				/* event message found and loaded from the event source */
				messages[1] = ReplaceStrings(logRec, messages[1]);
				messages[1] = ReplaceParameters(appFolder, messages[1]);
			}
			RegCloseKey(appFolder);
		}

		if (messages[1] != NULL)
			theMessage = messages[1];
		else if (logRec->NumStrings > 0)
			theMessage = ConcatStrings(logRec);
		else
			theMessage = L"<no message>";

		wprintf(L"RecNum=%lu TimeGen=%lu Type=%s Source=%s Computer=%s Category=%s Message=%s\n",
			logRec->RecordNumber, logRec->TimeGenerated,
			EventType(logRec->EventType), sourceName, computerName,
			(messages[0] != NULL ? messages[0] : L"<no category>"),
			theMessage);

		for (i = 0; i < 2; i++)
			if (messages[i] != NULL)
				free(messages[i]);
	}
}

/* parses and prints any entries from the event log <eventLogName> */
BOOL EventLogReader(LPCWSTR eventLogName)
{
	BOOL result = false;
	HANDLE eventLog;
	DWORD eventLogFolderPathLen;
	LPWSTR eventLogFolderPath;
	HKEY eventLogFolder;
	LPBYTE buffer;
	DWORD bufferSize = 0x10000;
	DWORD status;
	DWORD bytesRead;
	DWORD bytesNeeded;
	DWORD error;

	eventLog = OpenEventLogW(NULL, eventLogName);
	if (eventLog == NULL)
		goto ELR_ERROR_0;

	eventLogFolderPathLen = wcslen(REGISTRY_EVENTLOG_PATH) + wcslen(eventLogName) + 1;
	eventLogFolderPath = (LPWSTR)malloc(eventLogFolderPathLen * sizeof(WCHAR));
	if (eventLogFolderPath == NULL)
		goto ELR_ERROR_1;

	wcscpy_s(eventLogFolderPath, eventLogFolderPathLen, REGISTRY_EVENTLOG_PATH);
	wcscat_s(eventLogFolderPath, eventLogFolderPathLen, eventLogName);

	status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, eventLogFolderPath, 0, KEY_READ, &eventLogFolder);
	free(eventLogFolderPath);
	if (status != ERROR_SUCCESS)
		goto ELR_ERROR_1;

	buffer = (LPBYTE)malloc(bufferSize);
	if (buffer == NULL)
		goto ELR_ERROR_2;

	// # Comment Added
	//  - Retrive All Events
	//   - https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-eventlogrecord
	//   - https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readeventloga
	for (;;) {
		if (ReadEventLogW(eventLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
			0, buffer, bufferSize, &bytesRead, &bytesNeeded)) {
			ParseLogBuffer(eventLogFolder, buffer, bytesRead);
		}
		else {
			error = GetLastError();
			if (error == ERROR_INSUFFICIENT_BUFFER) {
				buffer = (LPBYTE)realloc(buffer, bytesNeeded);
				if (buffer == NULL)
					goto ELR_ERROR_2;
				bufferSize = bytesNeeded;
			}
			else if (error == ERROR_HANDLE_EOF) {
				break;
			}
			else {
				goto ELR_ERROR_3;
			}
		}
	}

	result = true;

ELR_ERROR_3:
	free(buffer);

ELR_ERROR_2:
	RegCloseKey(eventLogFolder);

ELR_ERROR_1:
	CloseEventLog(eventLog);

ELR_ERROR_0:
	return result;
}

#else
/**********************************************************************/
/* Vista, 2008, and newer                                             */
/**********************************************************************/

/* remove these typedefs and LoadWinEvtApiDll() below and
   link against wevtapi.lib if target is Vista, 2008, or newer */
typedef EVT_HANDLE(WINAPI* EvtCreateRenderContextType)(DWORD ValuePathsCount, LPCWSTR* ValuePaths,
	DWORD Flags);

typedef EVT_HANDLE(WINAPI* EvtQueryType)(EVT_HANDLE Session, LPCWSTR Path,
	LPCWSTR Query, DWORD Flags);

typedef BOOL(WINAPI* EvtNextType)(EVT_HANDLE ResultSet, DWORD EventsSize,
	PEVT_HANDLE Events, DWORD Timeout,
	DWORD Flags, __out PDWORD Returned);

typedef BOOL(WINAPI* EvtRenderType)(EVT_HANDLE Context, EVT_HANDLE Fragment,
	DWORD Flags, DWORD BufferSize,
	__out_bcount_part_opt(BufferSize, *BufferUsed) PVOID Buffer,
	__out PDWORD BufferUsed, __out PDWORD PropertyCount);

typedef EVT_HANDLE(WINAPI* EvtOpenPublisherMetadataType)(EVT_HANDLE Session, LPCWSTR PublisherId,
	LPCWSTR LogFilePath, LCID Locale,
	DWORD Flags);

typedef BOOL(WINAPI* EvtFormatMessageType)(EVT_HANDLE PublisherMetadata, EVT_HANDLE Event,
	DWORD MessageId, DWORD ValueCount,
	PEVT_VARIANT Values, DWORD Flags,
	DWORD BufferSize,
	__out_ecount_part_opt(BufferSize, *BufferUsed) LPWSTR Buffer,
	__out PDWORD BufferUsed);

typedef BOOL(WINAPI* EvtCloseType)(EVT_HANDLE Object);

EvtCreateRenderContextType EvtCreateRenderContextFunc;
EvtQueryType EvtQueryFunc;
EvtNextType EvtNextFunc;
EvtRenderType EvtRenderFunc;
EvtOpenPublisherMetadataType EvtOpenPublisherMetadataFunc;
EvtFormatMessageType EvtFormatMessageFunc;
EvtCloseType EvtCloseFunc;

/* load wevtapi.dll manually and setup function pointers
   returns false on error */
BOOL LoadWinEvtApiDll()
{
	HINSTANCE dll = LoadLibrary(L"wevtapi.dll");

	if (dll == NULL)
		goto LWEAD_ERROR_0;

	if ((EvtCreateRenderContextFunc = (EvtCreateRenderContextType)GetProcAddress(dll, "EvtCreateRenderContext")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtQueryFunc = (EvtQueryType)GetProcAddress(dll, "EvtQuery")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtNextFunc = (EvtNextType)GetProcAddress(dll, "EvtNext")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtRenderFunc = (EvtRenderType)GetProcAddress(dll, "EvtRender")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtOpenPublisherMetadataFunc = (EvtOpenPublisherMetadataType)GetProcAddress(dll, "EvtOpenPublisherMetadata")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtFormatMessageFunc = (EvtFormatMessageType)GetProcAddress(dll, "EvtFormatMessage")) == NULL)
		goto LWEAD_ERROR_1;

	if ((EvtCloseFunc = (EvtCloseType)GetProcAddress(dll, "EvtClose")) == NULL)
		goto LWEAD_ERROR_1;

	return true;

LWEAD_ERROR_1:
	FreeLibrary(dll);

LWEAD_ERROR_0:
	return false;
}

/* converts filetime (100ns since 1.1.1601) to epoch (seconds since 1.1.1970) */
ULONGLONG FileTime2Epoch(ULONGLONG filetime)
{
	filetime /= 10000000;
	filetime -= (ULONGLONG)11644473600;
	return filetime;
}

/* renders <logEntry> within a <renderContext>
   return value must be free()'ed if not NULL */
PEVT_VARIANT AllocEvtRender(EVT_HANDLE renderContext, EVT_HANDLE logEntry)
{
	DWORD valuesLen = 32768;
	PEVT_VARIANT values = NULL;
	DWORD bytesUsed;
	DWORD propsCount;

	for (;;) {
		values = (PEVT_VARIANT)realloc(values, valuesLen);
		if (values == NULL)
			return NULL;

		if (EvtRenderFunc(renderContext, logEntry, EvtRenderEventValues, valuesLen, values, &bytesUsed, &propsCount))
			return values;

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			free(values);
			return NULL;
		}

		valuesLen = bytesUsed;
	}
}

/* formats <logEntry> using the <publisherMetadata>
   flags are passed to EvtFormatMessage()
   return value must be free()'ed if not NULL */
LPWSTR AllocEvtFormatMessage(EVT_HANDLE publisherMetadata, EVT_HANDLE logEntry, DWORD flags)
{
	DWORD messageLen = 32768;
	LPWSTR message = NULL;
	DWORD charsUsed;

	for (;;) {
		message = (LPWSTR)realloc(message, messageLen * sizeof(WCHAR));
		if (message == NULL)
			return NULL;

		if (EvtFormatMessageFunc(publisherMetadata, logEntry, 0, 0, NULL, flags, messageLen, message, &charsUsed))
			return message;

		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			free(message);
			return NULL;
		}

		messageLen *= 10;
	}
}

/* parses and prints <logEntry> within a <renderContext>
   returns false on error */
BOOL ParseLogBuffer(EVT_HANDLE renderContext, EVT_HANDLE logEntry)
{
	BOOL result = false;
	PEVT_VARIANT values;
	EVT_HANDLE meta;
	LPWSTR message;
	LPWSTR level;

	values = AllocEvtRender(renderContext, logEntry);
	if (values == NULL)
		goto PLB_ERROR_0;

	meta = EvtOpenPublisherMetadataFunc(NULL, values[2].StringVal, NULL, 0, 0);
	if (meta != NULL)
		message = AllocEvtFormatMessage(meta, logEntry, EvtFormatMessageEvent);
	else
		message = NULL;

	level = AllocEvtFormatMessage(NULL, logEntry, EvtFormatMessageLevel);

	wprintf(L"RecNum=%lu TimeGen=%llu Type=%s Source=%s Computer=%s Message=%s\n",
		values[0].UInt32Val,
		FileTime2Epoch(values[1].FileTimeVal),
		(level != NULL ? level : L"<Unknown type>"),
		values[2].StringVal,
		values[3].StringVal,
		(message != NULL ? message : L"<No message>"));

	result = true;

	if (level != NULL)
		free(level);

	if (message != NULL)
		free(message);

	if (meta != NULL)
		EvtCloseFunc(meta);

	free(values);

PLB_ERROR_0:
	return result;
}

/* parses and prints any entries from the event log <eventLogName> */
BOOL EventLogReader(LPCWSTR eventLogName)
{
	if (!LoadWinEvtApiDll()) return false;

	BOOL result = false;
	LPCWSTR paths[] = { // metadata we're interessted in
		L"/Event/System/EventRecordID",
		L"/Event/System/TimeCreated/@SystemTime",
		L"/Event/System/Provider/@Name",
		L"/Event/System/Computer",
	};
	EVT_HANDLE renderContext;
	EVT_HANDLE query;
	EVT_HANDLE entries[1024];
	DWORD numEntries;
	DWORD i;

	renderContext = EvtCreateRenderContextFunc(sizeof(paths) / sizeof(paths[0]), paths, EvtRenderContextValues);
	if (renderContext == NULL)
		goto ELRV_ERROR_0;

	query = EvtQueryFunc(NULL, eventLogName, L"*", EvtQueryChannelPath | EvtQueryReverseDirection);
	if (query == NULL)
		goto ELRV_ERROR_1;

	// Comment Added
	//  - Retrive All Events
	//   - https://docs.microsoft.com/en-us/windows/win32/api/winevt/
	//   - https://docs.microsoft.com/en-us/windows/win32/api/winevt/nf-winevt-evtnext
	while (EvtNextFunc(query, sizeof(entries) / sizeof(entries[0]), entries, INFINITE, 0, &numEntries)) {
		for (i = 0; i < numEntries; i++) {
			if (!ParseLogBuffer(renderContext, entries[i]))
				goto ELRV_ERROR_2;
			if (!EvtCloseFunc(entries[i]))
				goto ELRV_ERROR_2;
		}
	}

	if (GetLastError() == ERROR_NO_MORE_ITEMS)
		result = true;

ELRV_ERROR_2:
	for (; i < numEntries; i++)
		EvtCloseFunc(entries[i]);

	EvtCloseFunc(query);

ELRV_ERROR_1:
	EvtCloseFunc(renderContext);

ELRV_ERROR_0:
	return result;
}


#endif
