#include <windows.h>
#include <globals.h>
#include <utils/err.h>

#define REG_KEY_PATH L"Software\\SMTPCRACKER"

int LoadSettings(void) {
    HKEY hKey;
    LONG status = RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, KEY_READ, &hKey);
    if (status != ERROR_SUCCESS) {
        // If key doesn't exist, we'll just use the defaults initialized in main.c
        return -1;
    }

    DWORD dwValue, dwSize;

    // --- Load Integers ---
    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"ThemeMode", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        g_settings.themeMode = dwValue;

    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"ThemeIndex", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        g_settings.themeIndex = dwValue;

    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"Threads", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        g_settings.numThreads = dwValue;

    dwSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, L"Retries", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS)
        g_settings.maxRetries = dwValue;

    // --- Load Strings ---
    dwSize = sizeof(g_settings.testEmail);
    RegQueryValueExW(hKey, L"TestEmail", NULL, NULL, (LPBYTE)g_settings.testEmail, &dwSize);

    dwSize = sizeof(g_settings.saveFileName);
    RegQueryValueExW(hKey, L"SaveFile", NULL, NULL, (LPBYTE)g_settings.saveFileName, &dwSize);

    dwSize = sizeof(g_settings.telegramToken);
    RegQueryValueExW(hKey, L"TelToken", NULL, NULL, (LPBYTE)g_settings.telegramToken, &dwSize);

    dwSize = sizeof(g_settings.telegramID);
    RegQueryValueExW(hKey, L"TelID", NULL, NULL, (LPBYTE)g_settings.telegramID, &dwSize);

    RegCloseKey(hKey);
    return 0;
}


int SaveSettings(void) {
    HKEY hKey;
    LONG status = RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS) return -1;

    // Save Integers (ThemeMode, ThemeIndex, Threads, Retries)
    RegSetValueExW(hKey, L"ThemeMode", 0, REG_DWORD, (const BYTE*)&g_settings.themeMode, sizeof(DWORD));
    RegSetValueExW(hKey, L"ThemeIndex", 0, REG_DWORD, (const BYTE*)&g_settings.themeIndex, sizeof(DWORD));
    RegSetValueExW(hKey, L"Threads", 0, REG_DWORD, (const BYTE*)&g_settings.numThreads, sizeof(DWORD));
    RegSetValueExW(hKey, L"Retries", 0, REG_DWORD, (const BYTE*)&g_settings.maxRetries, sizeof(DWORD));

    // Save Strings (Email, File, Telegram)
    RegSetValueExW(hKey, L"TestEmail", 0, REG_SZ, (const BYTE*)g_settings.testEmail, (wcslen(g_settings.testEmail) + 1) * sizeof(wchar_t));
    RegSetValueExW(hKey, L"SaveFile", 0, REG_SZ, (const BYTE*)g_settings.saveFileName, (wcslen(g_settings.saveFileName) + 1) * sizeof(wchar_t));
    RegSetValueExW(hKey, L"TelToken", 0, REG_SZ, (const BYTE*)g_settings.telegramToken, (wcslen(g_settings.telegramToken) + 1) * sizeof(wchar_t));
    RegSetValueExW(hKey, L"TelID", 0, REG_SZ, (const BYTE*)g_settings.telegramID, (wcslen(g_settings.telegramID) + 1) * sizeof(wchar_t));

    RegCloseKey(hKey);
    return 0;
}