## simplesvc

### reference
- [The Complete Service Sample MSDN](https://docs.microsoft.com/en-us/windows/win32/services/the-complete-service-sample)

### environment
- Windows 7 Professional SP1 x86
- Visual Studio 2008

### project property
- property -> c/c++ -> Additional Include Directories : $(ProjectDir)
- property -> c/c++ -> Precompiled Headers : Not Using Precompiled Headers

### Issue
- Windows 10 Pro: Service start with [Error 1053](https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes--1300-1699-)
- The service did not respond to the start or control request in a timely fashion. 

### usage
- Need UAC elevation: administrator
- It will be able to use "SC" commands
```
    -i : install service
    -s : start service
    -p : stop service
    -d : remove service
    -h : help message
```