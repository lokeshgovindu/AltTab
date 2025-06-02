#include "framework.h"
#include "Utils.h"
#include <algorithm>
#include <comdef.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <string>
#include <taskschd.h>
#include <Windows.h>
#include <lmcons.h>

#include "fuzzywuzzy.h"
#include <shellapi.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

bool EnableConsoleWindow() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (!AllocConsole())
            return false; // Could throw here
    }
    
    FILE* stdinNew = nullptr;
    freopen_s(&stdinNew,  "CONIN$",  "r+", stdin);
    FILE* stdoutNew = nullptr;
    freopen_s(&stdoutNew, "CONOUT$", "w+", stdout);
    FILE* stderrNew = nullptr;
    freopen_s(&stderrNew, "CONOUT$", "w+", stderr);

    return true;
}

std::string Trim(const std::string& str, const std::string& seps)
{
    if (str.empty()) return str;
    size_t p = str.find_first_not_of(seps);
    if (p == std::string::npos) return "";
    size_t q = str.find_last_not_of(seps);
    return str.substr(p, q - p + 1);
}

std::vector<std::string> Split(const std::string& s, const std::string& seps) {
    std::vector<std::string> ret;
    for (size_t p = 0, q; p != std::string::npos; p = q) {
        p = s.find_first_not_of(seps, p);
        if (p == std::string::npos)
            break;
        q = s.find_first_of(seps, p);
        ret.push_back(s.substr(p, q - p));
    }
    return ret;
}

std::vector<std::wstring> Split(const std::wstring& s, const std::wstring& seps) {
    std::vector<std::wstring> ret;
    for (size_t p = 0, q; p != std::wstring::npos; p = q) {
        p = s.find_first_not_of(seps, p);
        if (p == std::wstring::npos)
            break;
        q = s.find_first_of(seps, p);
        ret.push_back(s.substr(p, q - p));
    }
    return ret;
}

bool EqualsIgnoreCase(const std::string& str1, const std::string& t) {
    return str1.size() == t.size() && std::equal(str1.begin(), str1.end(), t.begin(), [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
}

bool EqualsIgnoreCase(const std::wstring& str1, const std::wstring& t) {
    return str1.size() == t.size() && std::equal(str1.begin(), str1.end(), t.begin(), [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
}

std::wstring ToLower(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), towlower);
    return result;
}

std::wstring ToUpper(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), towupper);
    return result;
}

// Convert wstring to UTF-8 string
std::string WStrToUTF8(const std::wstring& wstr) {
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    std::string utf8str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &utf8str[0], sizeNeeded, nullptr, nullptr);
    return utf8str;
}

// Convert UTF-8 string to wstring
std::wstring UTF8ToWStr(const std::string& utf8str) {
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), static_cast<int>(utf8str.length()), nullptr, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), static_cast<int>(utf8str.length()), &wstr[0], sizeNeeded);
    return wstr;
}

std::string GetWindowTitleExA(HWND hWnd) {
    const int bufferSize = GetWindowTextLengthA(hWnd) + 1;
    char* windowTitle = new char[bufferSize];
    GetWindowTextA(hWnd, windowTitle, bufferSize);
    std::string ret = windowTitle;
    delete[] windowTitle;
    return ret;
}

std::wstring GetWindowTitleExW(HWND hWnd) {
    const int bufferSize = GetWindowTextLengthW(hWnd) + 1;
    wchar_t* windowTitle = new wchar_t[bufferSize];
    GetWindowTextW(hWnd, windowTitle, bufferSize);
    std::wstring ret = windowTitle;
    delete[] windowTitle;
    return ret;
}

bool InStr(const std::wstring& str, const std::wstring& substr) {
    return std::search(
               str.begin(),
               str.end(),
               substr.begin(),
               substr.end(),
               [](wchar_t c1, wchar_t c2) { return std::toupper(c1) == std::toupper(c2); })
           != str.end();
}

#define BUFFER_SIZE 1024

std::string ToLower(const char* s) {
    std::string ret;
    for (int i = 0; s[i] != '\0'; ++i) {
        if (isupper(s[i])) {
            ret += (char)(tolower(s[i]));
        } else {
            ret += s[i];
        }
    }
    return ret;
}

std::string ToNarrow(const wchar_t* szBufW) {
    char szBuf[BUFFER_SIZE];
    size_t count;
    errno_t err;

    err = wcstombs_s(&count, szBuf, (size_t)BUFFER_SIZE, szBufW, (size_t)BUFFER_SIZE);
    if (err != 0) {
        throw std::exception("Failed to convert to a multi byte character string.");
    }
    return ::ToLower(szBuf);
}

//double GetRatioA(const char* s1, const char* s2) {
//    //printf("s1 = [%s], s2 = [%s]\n", s1, s2);
//    return ::ratio(ToLower(s1), ToLower(s2));
//}
//
//double GetPartialRatioA(const char* s1, const char* s2) {
//    //printf("s1 = [%s], s2 = [%s]\n", s1, s2);
//    return ::partial_ratio(ToLower(s1), ToLower(s2));
//}

double GetRatioW(const wchar_t* s1, const wchar_t* s2) {
    try {
        //wprintf(L"s1 = [%s], s2 = [%s]\n", s1, s2);
        return ::ratio(s1, s2);
    } catch (...) {
        return 0.0;
    }
}

struct ScopedTimer {
    ScopedTimer() : m_Elapsed(0.) {
        QueryPerformanceCounter(&m_tStartTime);
    }

    ~ScopedTimer() {
        QueryPerformanceCounter(&m_tStopTime);
        QueryPerformanceFrequency(&m_tFrequency);
        m_Elapsed = (double)(m_tStopTime.QuadPart - m_tStartTime.QuadPart) / (double)m_tFrequency.QuadPart;

        printf_s("Elapsed : %f\n", m_Elapsed);
    }

private:
    LARGE_INTEGER m_tStartTime;
    LARGE_INTEGER m_tStopTime;
    LARGE_INTEGER m_tFrequency;
    std::string   m_Started;
    std::string   m_Ended;
    double        m_Elapsed;
    std::string   m_Name;
};

double GetPartialRatioW(const wchar_t* s1, const wchar_t* s2) {
    //ScopedTimer st;
    try {
        return partial_ratio(s1, s2);
    } catch (...) {
        return 0.0;
    }
}

double GetPartialRatioW(const std::wstring& s1, const std::wstring& s2) {
    //ScopedTimer st;
    try {
        return partial_ratio(ToLower(s1), ToLower(s2));
    } catch (...) {
        return 0.0;
    }
}

// Helper function to scale a value based on DPI
int ScaleValueForDPI(int value, int dpi) {
    // For simplicity, you can use a linear scaling factor
    // For more precise scaling, you may need to adjust according to DPI settings
    return MulDiv(value, dpi, 96);
}

// Helper function to get the DPI of a window
int GetDPIForWindow(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    return dpi;
}

// Helper macros from wix.
// TODO: use "s" and "..." parameters to report errors from these functions.
#define ExitOnFailure(x, s, ...)                                                                                       \
    if (FAILED(x)) {                                                                                                   \
        goto LExit;                                                                                                    \
    }
#define ExitWithLastError(x, s, ...)                                                                                   \
    {                                                                                                                  \
        DWORD Dutil_er = ::GetLastError();                                                                             \
        x = HRESULT_FROM_WIN32(Dutil_er);                                                                              \
        if (!FAILED(x)) {                                                                                              \
            x = E_FAIL;                                                                                                \
        }                                                                                                              \
        goto LExit;                                                                                                    \
    }
#define ExitFunction()                                                                                                 \
    { goto LExit; }

const DWORD USERNAME_DOMAIN_LEN = DNLEN + UNLEN + 2; // Domain Name + '\' + User Name + '\0'
const DWORD USERNAME_LEN = UNLEN + 1;                // User Name + '\0'

// These task related stuff is copied from https://github.com/microsoft/AltTab
bool create_auto_start_task_for_this_user(bool runElevated) {
    HRESULT hr = S_OK;

    WCHAR username_domain[USERNAME_DOMAIN_LEN];
    WCHAR username[USERNAME_LEN];

    std::wstring wstrTaskName;

    ITaskService*       pService                  = NULL;
    ITaskFolder*        pTaskFolder               = NULL;
    ITaskDefinition*    pTask                     = NULL;
    IRegistrationInfo*  pRegInfo                  = NULL;
    ITaskSettings*      pSettings                 = NULL;
    ITriggerCollection* pTriggerCollection        = NULL;
    IRegisteredTask*    pRegisteredTask           = NULL;

    // ------------------------------------------------------
    // Get the Domain/Username for the trigger.
    if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
        ExitWithLastError(hr, "Getting username failed: %x", hr);
    }
    if (!GetEnvironmentVariable(L"USERDOMAIN", username_domain, USERNAME_DOMAIN_LEN)) {
        ExitWithLastError(hr, "Getting the user's domain failed: %x", hr);
    }
    wcscat_s(username_domain, L"\\");
    wcscat_s(username_domain, username);

    // Task Name.
    wstrTaskName = L"Autorun for ";
    wstrTaskName += username;

    // Get the executable path passed to the custom action.
    WCHAR wszExecutablePath[MAX_PATH];
    GetModuleFileName(NULL, wszExecutablePath, MAX_PATH);

    // ------------------------------------------------------
    // Create an instance of the Task Service.
    hr = CoCreateInstance(
        CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
    ExitOnFailure(hr, "Failed to create an instance of ITaskService: %x", hr);

    // Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    ExitOnFailure(hr, "ITaskService::Connect failed: %x", hr);

    // ------------------------------------------------------
    // Get the AltTab task folder. Creates it if it doesn't exist.
    hr = pService->GetFolder(_bstr_t(L"\\AltTab"), &pTaskFolder);
    if (FAILED(hr)) {
        // Folder doesn't exist. Get the Root folder and create the AltTab subfolder.
        ITaskFolder* pRootFolder = NULL;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        ExitOnFailure(hr, "Cannot get Root Folder pointer: %x", hr);
        hr = pRootFolder->CreateFolder(_bstr_t(L"\\AltTab"), _variant_t(L""), &pTaskFolder);
        if (FAILED(hr)) {
            pRootFolder->Release();
            ExitOnFailure(hr, "Cannot create AltTab task folder: %x", hr);
        }
    }

    // If the task exists, just enable it.
    {
        IRegisteredTask* pExistingRegisteredTask = NULL;
        hr = pTaskFolder->GetTask(_bstr_t(wstrTaskName.c_str()), &pExistingRegisteredTask);
        if (SUCCEEDED(hr)) {
            // Task exists, try enabling it.
            hr = pExistingRegisteredTask->put_Enabled(VARIANT_TRUE);
            pExistingRegisteredTask->Release();
            if (SUCCEEDED(hr)) {
                // Function enable. Sounds like a success.
                ExitFunction();
            }
        }
    }

    // Create the task builder object to create the task.
    hr = pService->NewTask(0, &pTask);
    ExitOnFailure(hr, "Failed to create a task definition: %x", hr);

    // ------------------------------------------------------
    // Get the registration info for setting the identification.
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    ExitOnFailure(hr, "Cannot get identification pointer: %x", hr);
    hr = pRegInfo->put_Author(_bstr_t(username_domain));
    ExitOnFailure(hr, "Cannot put identification info: %x", hr);

    // ------------------------------------------------------
    // Create the settings for the task
    hr = pTask->get_Settings(&pSettings);
    ExitOnFailure(hr, "Cannot get settings pointer: %x", hr);

    hr = pSettings->put_StartWhenAvailable(VARIANT_FALSE);
    ExitOnFailure(hr, "Cannot put_StartWhenAvailable setting info: %x", hr);
    hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
    ExitOnFailure(hr, "Cannot put_StopIfGoingOnBatteries setting info: %x", hr);
    hr = pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")); // Unlimited
    ExitOnFailure(hr, "Cannot put_ExecutionTimeLimit setting info: %x", hr);
    hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
    ExitOnFailure(hr, "Cannot put_DisallowStartIfOnBatteries setting info: %x", hr);
    hr = pSettings->put_Priority(4);
    ExitOnFailure(hr, "Cannot put_Priority setting info : %x", hr);

    // ------------------------------------------------------
    // Get the trigger collection to insert the logon trigger.
    hr = pTask->get_Triggers(&pTriggerCollection);
    ExitOnFailure(hr, "Cannot get trigger collection: %x", hr);

    // Add the logon trigger to the task.
    {
        ITrigger* pTrigger = NULL;
        ILogonTrigger* pLogonTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        ExitOnFailure(hr, "Cannot create the trigger: %x", hr);

        hr = pTrigger->QueryInterface(IID_ILogonTrigger, reinterpret_cast<void**>(&pLogonTrigger));
        pTrigger->Release();
        ExitOnFailure(hr, "QueryInterface call failed for ILogonTrigger: %x", hr);

        hr = pLogonTrigger->put_Id(_bstr_t(L"Trigger1"));

        // Timing issues may make explorer not be started when the task runs.
        // Add a little delay to mitigate this.
        hr = pLogonTrigger->put_Delay(_bstr_t(L"PT03S"));

        // Define the user. The task will execute when the user logs on.
        // The specified user must be a user on this computer.
        hr = pLogonTrigger->put_UserId(_bstr_t(username_domain));
        pLogonTrigger->Release();
        ExitOnFailure(hr, "Cannot add user ID to logon trigger: %x", hr);
    }

    // ------------------------------------------------------
    // Add an Action to the task. This task will execute the path passed to this custom action.
    {
        IActionCollection* pActionCollection = NULL;
        IAction* pAction = NULL;
        IExecAction* pExecAction = NULL;

        // Get the task action collection pointer.
        hr = pTask->get_Actions(&pActionCollection);
        ExitOnFailure(hr, "Cannot get Task collection pointer: %x", hr);

        // Create the action, specifying that it is an executable action.
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        pActionCollection->Release();
        ExitOnFailure(hr, "Cannot create the action: %x", hr);

        // QI for the executable task pointer.
        hr = pAction->QueryInterface(IID_IExecAction, reinterpret_cast<void**>(&pExecAction));
        pAction->Release();
        ExitOnFailure(hr, "QueryInterface call failed for IExecAction: %x", hr);

        // Set the path of the executable to AltTab (passed as CustomActionData).
        hr = pExecAction->put_Path(_bstr_t(wszExecutablePath));
        pExecAction->Release();
        ExitOnFailure(hr, "Cannot set path of executable: %x", hr);
    }

    // ------------------------------------------------------
    // Create the principal for the task
    {
        IPrincipal* pPrincipal = NULL;
        hr = pTask->get_Principal(&pPrincipal);
        ExitOnFailure(hr, "Cannot get principal pointer: %x", hr);

        // Set up principal information:
        hr = pPrincipal->put_Id(_bstr_t(L"Principal1"));

        hr = pPrincipal->put_UserId(_bstr_t(username_domain));

        hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);

        if (runElevated) {
            hr = pPrincipal->put_RunLevel(_TASK_RUNLEVEL::TASK_RUNLEVEL_HIGHEST);
        } else {
            hr = pPrincipal->put_RunLevel(_TASK_RUNLEVEL::TASK_RUNLEVEL_LUA);
        }
        pPrincipal->Release();
        ExitOnFailure(hr, "Cannot put principal run level: %x", hr);
    }
    // ------------------------------------------------------
    //  Save the task in the AltTab folder.
    {
        _variant_t SDDL_FULL_ACCESS_FOR_EVERYONE = L"D:(A;;FA;;;WD)";
        hr = pTaskFolder->RegisterTaskDefinition(
            _bstr_t(wstrTaskName.c_str()),
            pTask,
            TASK_CREATE_OR_UPDATE,
            _variant_t(username_domain),
            _variant_t(),
            TASK_LOGON_INTERACTIVE_TOKEN,
            SDDL_FULL_ACCESS_FOR_EVERYONE,
            &pRegisteredTask);
        ExitOnFailure(hr, "Error saving the Task : %x", hr);
    }

LExit:
    if (pService)
        pService->Release();
    if (pTaskFolder)
        pTaskFolder->Release();
    if (pTask)
        pTask->Release();
    if (pRegInfo)
        pRegInfo->Release();
    if (pSettings)
        pSettings->Release();
    if (pTriggerCollection)
        pTriggerCollection->Release();
    if (pRegisteredTask)
        pRegisteredTask->Release();

    return (SUCCEEDED(hr));
}

bool delete_auto_start_task_for_this_user() {
    HRESULT hr = S_OK;

    WCHAR username[USERNAME_LEN];
    std::wstring wstrTaskName;

    ITaskService* pService = NULL;
    ITaskFolder* pTaskFolder = NULL;

    // ------------------------------------------------------
    // Get the Username for the task.
    if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
        ExitWithLastError(hr, "Getting username failed: %x", hr);
    }

    // Task Name.
    wstrTaskName = L"Autorun for ";
    wstrTaskName += username;

    // ------------------------------------------------------
    // Create an instance of the Task Service.
    hr = CoCreateInstance(
        CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
    ExitOnFailure(hr, "Failed to create an instance of ITaskService: %x", hr);

    // Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    ExitOnFailure(hr, "ITaskService::Connect failed: %x", hr);

    // ------------------------------------------------------
    // Get the AltTab task folder.
    hr = pService->GetFolder(_bstr_t(L"\\AltTab"), &pTaskFolder);
    if (FAILED(hr)) {
        // Folder doesn't exist. No need to disable a non-existing task.
        hr = S_OK;
        ExitFunction();
    }

    // ------------------------------------------------------
    // If the task exists, disable.
    {
        IRegisteredTask* pExistingRegisteredTask = NULL;
        hr = pTaskFolder->GetTask(_bstr_t(wstrTaskName.c_str()), &pExistingRegisteredTask);
        if (SUCCEEDED(hr)) {
            // Task exists, try disabling it.
            hr = pTaskFolder->DeleteTask(_bstr_t(wstrTaskName.c_str()), 0);
        }
    }

LExit:
    if (pService)
        pService->Release();
    if (pTaskFolder)
        pTaskFolder->Release();

    return (SUCCEEDED(hr));
}

std::wstring GetApplicationPath() {
    wchar_t applicationPath[MAX_PATH] = { 0 };
    GetModuleFileName(nullptr, applicationPath, MAX_PATH);
    return std::wstring(applicationPath);
}

bool is_auto_start_task_active_for_this_user() {
    HRESULT hr = S_OK;

    WCHAR username[USERNAME_LEN];
    std::wstring wstrTaskName;

    ITaskService* pService = NULL;
    ITaskFolder* pTaskFolder = NULL;

    // ------------------------------------------------------
    // Get the Username for the task.
    if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
        ExitWithLastError(hr, "Getting username failed: %x", hr);
    }

    // Task Name.
    wstrTaskName = L"Autorun for ";
    wstrTaskName += username;

    // ------------------------------------------------------
    // Create an instance of the Task Service.
    hr = CoCreateInstance(
        CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
    ExitOnFailure(hr, "Failed to create an instance of ITaskService: %x", hr);

    // Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    ExitOnFailure(hr, "ITaskService::Connect failed: %x", hr);

    // ------------------------------------------------------
    // Get the AltTab task folder.
    hr = pService->GetFolder(_bstr_t(L"\\AltTab"), &pTaskFolder);
    ExitOnFailure(hr, "ITaskFolder doesn't exist: %x", hr);

    // ------------------------------------------------------
    // If the task exists, disable.
    {
        IRegisteredTask* pExistingRegisteredTask = NULL;
        hr = pTaskFolder->GetTask(_bstr_t(wstrTaskName.c_str()), &pExistingRegisteredTask);
        if (SUCCEEDED(hr)) {
            // Task exists, get its value.
            VARIANT_BOOL is_enabled;
            hr = pExistingRegisteredTask->get_Enabled(&is_enabled);
            pExistingRegisteredTask->Release();
            if (SUCCEEDED(hr)) {
                // Got the value. Return it.
                hr = (is_enabled == VARIANT_TRUE) ? S_OK : E_FAIL; // Fake success or fail to return the value.
                ExitFunction();
            }
        }
    }

LExit:
    if (pService)
        pService->Release();
    if (pTaskFolder)
        pTaskFolder->Release();

    return (SUCCEEDED(hr));
}

// Returns true if the current process is running with elevated privileges
bool IsProcessElevated(const bool use_cached_value) {
    auto detection_func = []() {
        HANDLE token = nullptr;
        bool elevated = false;

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
            TOKEN_ELEVATION elevation;
            DWORD size;
            if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
                elevated = (elevation.TokenIsElevated != 0);
            }
        }

        if (token) {
            CloseHandle(token);
        }

        return elevated;
    };
    static const bool cached_value = detection_func();
    return use_cached_value ? cached_value : detection_func();
}

bool RestartApplication() {
    std::wstring applicationPath = GetApplicationPath();

    // Replace the existing process with a new instance of the same program
    intptr_t ret = _wexecl(applicationPath.c_str(), applicationPath.c_str(), nullptr);

    // If _wexecl returns, it means the new process failed to start
    if (ret == -1) {
        // Handle error
        DWORD error = GetLastError();
        std::wcerr << L"Failed to restart application. Error code: " << std::hex << error << std::endl;
        return false;
    }
    // If we reach here, the new process was started successfully
    // The current process will exit, and the new process will take over
    ExitProcess(0); // Ensure the current process exits
}

bool RunAsAdmin(const std::wstring& command, const std::wstring& args) {
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.hwnd = nullptr;
    sei.lpVerb = L"runas"; // Run as administrator
    sei.lpFile = command.c_str();
    sei.lpParameters = args.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            // User cancelled the elevation prompt
            return false;
        }
        return false; // Other errors
    }
    WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);
    return true;
}

void RelaunchAsAdminAndExit(const bool withElvatedArg) {
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS; // Optional: to wait or check the new process
    sei.lpVerb = L"runas";               // Triggers UAC elevation
    sei.lpFile = exePath;                // Path to this executable
    if (withElvatedArg) {
        sei.lpParameters = L"--elevated"; // Optional: to pass a flag
    } else {
        sei.lpParameters = L""; // No additional parameters
    }
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei)) {
        // Optional: wait for new elevated process to start up
        // WaitForInputIdle(sei.hProcess, 1000);
        // CloseHandle(sei.hProcess);

        // Exit current process immediately
        ExitProcess(0);
    } else {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            MessageBoxW(nullptr, L"User declined UAC prompt.", L"Info", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(nullptr, L"Failed to launch elevated process.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

bool IsTaskRunWithHighestPrivileges() {
    HRESULT hr = S_OK;

    WCHAR username[USERNAME_LEN];
    std::wstring wstrTaskName;

    bool result = false;
    ITaskService* pService = nullptr;
    ITaskFolder* pTaskFolder = nullptr;
    ITaskFolder* pRootFolder = nullptr;
    IRegisteredTask* pTask = nullptr;
    ITaskDefinition* pDef = nullptr;
    IPrincipal* pPrincipal = nullptr;

    // ------------------------------------------------------
    // Get the Username for the task.
    if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
        ExitWithLastError(hr, "Getting username failed: %x", hr);
    }

    // Task Name.
    wstrTaskName = L"Autorun for ";
    wstrTaskName += username;

    // Get the executable path passed to the custom action.
    WCHAR wszExecutablePath[MAX_PATH];
    GetModuleFileName(NULL, wszExecutablePath, MAX_PATH);

    // ------------------------------------------------------
    // Create an instance of the Task Service.
    hr = CoCreateInstance(
        CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, reinterpret_cast<void**>(&pService));
    ExitOnFailure(hr, "Failed to create an instance of ITaskService: %x", hr);

    // Connect to the task service.
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    ExitOnFailure(hr, "ITaskService::Connect failed: %x", hr);

    // ------------------------------------------------------
    // Get the AltTab task folder. Creates it if it doesn't exist.
    hr = pService->GetFolder(_bstr_t(L"\\AltTab"), &pTaskFolder);
    if (FAILED(hr)) {
        // Folder doesn't exist. Get the Root folder and create the AltTab subfolder.
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        ExitOnFailure(hr, "Cannot get Root Folder pointer: %x", hr);
        hr = pRootFolder->CreateFolder(_bstr_t(L"\\AltTab"), _variant_t(L""), &pTaskFolder);
        if (FAILED(hr)) {
            pRootFolder->Release();
            ExitOnFailure(hr, "Cannot create AltTab task folder: %x", hr);
        }
    }

    hr = pTaskFolder->GetTask(_bstr_t(wstrTaskName.c_str()), &pTask);
    if (SUCCEEDED(hr)) {
        // Task exists, check if it runs with highest privileges.
        hr = pTask->get_Definition(&pDef);
        if (SUCCEEDED(hr)) {
            hr = pDef->get_Principal(&pPrincipal);
            if (SUCCEEDED(hr)) {
                _TASK_RUNLEVEL runLevel;
                hr = pPrincipal->get_RunLevel(&runLevel);
                if (SUCCEEDED(hr) && runLevel == TASK_RUNLEVEL_HIGHEST) {
                    result = true;
                }
            }
        }
    }

LExit:
    if (pPrincipal)
        pPrincipal->Release();
    if (pDef)
        pDef->Release();
    if (pTask)
        pTask->Release();
    if (pRootFolder)
        pRootFolder->Release();
    if (pService)
        pService->Release();
    return result;
}

bool CheckSingleInstance(const std::wstring& mutexName) {
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running
        CloseHandle(hMutex);
        return false;
    }
    // No other instance is running
    return true;
}

bool InitializeCOM() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        // Handle the error
        std::wcerr << L"Failed to initialize COM library. Error code: " << std::hex << hr << std::endl;
        return false;
    }

    // Set COM security levels
    hr = CoInitializeSecurity(
        nullptr,
        -1,
        nullptr,
        nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE,
        nullptr);

    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize COM security. Error code: " << std::hex << hr << std::endl;
        CoUninitialize();
        return false;
    }

    return true;
}

void UninitializeCOM() {
    CoUninitialize();
}
