### evdescriber
- Retrive Windows Event Log

### Start With
- [Windows Event Log MSDN](https://docs.microsoft.com/en-us/windows/win32/wes/windows-event-log)

### Event Soruces
- [event-sources](https://docs.microsoft.com/en-us/windows/win32/eventlog/event-sources)
```
Registry: HKLM\SYSTEM\CurrentControlSet\Services\EventLog
```

### Event Key
- [event-key](https://docs.microsoft.com/en-us/windows/win32/eventlog/eventlog-key)
```
Application
Security
System
```

### Sequence
```
1. Open Event Log Handle

2. Read Event: Store EVENTLOGRECORD

3. Get Message Files From Registry: HKLM\SYSTEM\CurrentControlSet\Services\EventLog\[EventKey]\[EventSoruce]
 - EventMessageFile
 - ParameterMessageFile
 - CategoryMessageFile

4. Load Messages
```
