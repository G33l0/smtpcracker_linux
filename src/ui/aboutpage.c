#include <ui/aboutpage.h>
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <globals.h>
#include <stdio.h>
#include <strsafe.h>

// --- Static Handles ---
static HWND g_hRichEdit = NULL;
static HMODULE g_hRichEditLib = NULL;

// --- Helper: Append Formatted Text ---
// Updated to remove manual color/background params
void AppendText(HWND hEdit, LPCWSTR text, int size, BOOL bold, LPCWSTR fontFace) {
    // 1. Move selection to the very end
    CHARRANGE cr = { -1, -1 };
    SendMessage(hEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    // 2. Prepare formatting
    CHARFORMAT2W cf = { 0 };
    cf.cbSize = sizeof(CHARFORMAT2W);
    // Only set Size, Bold, and Face. Let colors be default/system.
    cf.dwMask = CFM_SIZE | CFM_BOLD | CFM_FACE;
    
    // Size is in twips (1 point = 20 twips)
    cf.yHeight = size * 20; 
    
    if (bold) cf.dwEffects |= CFE_BOLD;
    else cf.dwEffects &= ~CFE_BOLD;

    if (fontFace) StringCchCopyW(cf.szFaceName, LF_FACESIZE, fontFace);
    else StringCchCopyW(cf.szFaceName, LF_FACESIZE, L"Segoe UI");

    // 3. Apply formatting to the insertion point
    SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // 4. Insert the text
    SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)text);
}

// --- Helper: Refresh Content ---
void PopulateRichEdit(HWND hEdit) {
    if (!hEdit || !IsWindow(hEdit)) return;

    // 1. Clear and Freeze
    SendMessage(hEdit, WM_SETREDRAW, FALSE, 0);
    SetWindowTextW(hEdit, L""); 

    // --- Content ---

    // Title
    AppendText(hEdit, L"🔓 SMTP Cracker - Advanced Email Authentication Tool\r\n\r\n", 16, TRUE, NULL);
    
    // Overview
    AppendText(hEdit, L"📋 Overview\r\n", 13, TRUE, NULL);
    AppendText(hEdit, L"SMTP Cracker is a powerful tool for testing and validating SMTP server credentials. It combines multi-threaded processing with intelligent retry mechanisms.\r\n\r\n", 10, FALSE, NULL);

    // Features
    AppendText(hEdit, L"🚀 Features\r\n", 13, TRUE, NULL);
    
    AppendText(hEdit, L"🔐 Advanced Authentication Testing\r\n", 11, TRUE, NULL);
    AppendText(hEdit, L" • Multi-threaded SMTP credential validation\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L" • Intelligent retry mechanisms with configurable attempts\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L" • Support for various SMTP server configurations\r\n\r\n", 10, FALSE, NULL);

    AppendText(hEdit, L"📧 Email Delivery Testing\r\n", 11, TRUE, NULL);
    AppendText(hEdit, L" • Test email delivery capabilities\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L" • Validate SMTP server functionality\r\n\r\n", 10, FALSE, NULL);

    // Input Format (Code Block)
    AppendText(hEdit, L"📁 Input Format\r\n", 13, TRUE, NULL);
    AppendText(hEdit, L"SMTP credentials should be in combo format:\r\n", 10, FALSE, NULL);
    
    // Code block lines
    AppendText(hEdit, L" email@domain.com:password         \r\n", 10, FALSE, L"Consolas");
    AppendText(hEdit, L" user@gmail.com:mypassword123      \r\n", 10, FALSE, L"Consolas");
    AppendText(hEdit, L" admin@company.org:secretpass      \r\n", 10, FALSE, L"Consolas");
    AppendText(hEdit, L" test@yahoo.com:password456        \r\n", 10, FALSE, L"Consolas");
    // Reset to normal
    AppendText(hEdit, L"\r\n", 10, FALSE, NULL);

    // Configuration
    AppendText(hEdit, L"🎛️ Configuration Options\r\n", 13, TRUE, NULL);
    
    AppendText(hEdit, L"🧵 Thread Settings\r\n", 11, TRUE, NULL);
    AppendText(hEdit, L" • Range: 1-20 concurrent threads\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L" • Recommended: 5-10 threads for optimal performance\r\n\r\n", 10, FALSE, NULL);

    AppendText(hEdit, L"🔄 Retry Settings\r\n", 11, TRUE, NULL);
    AppendText(hEdit, L" • Range: 0-5 retry attempts\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L" • 3 retries: Recommended for reliable results\r\n\r\n", 10, FALSE, NULL);

    // Warning Block
    AppendText(hEdit, L"⚠️ Legal Notice\r\n", 13, TRUE, NULL);
    AppendText(hEdit, L" IMPORTANT: This tool is for authorized security testing and research only.   \r\n", 10, TRUE, NULL);
    AppendText(hEdit, L" Always ensure you have explicit permission before testing any SMTP servers.  \r\n", 10, TRUE, NULL);
    AppendText(hEdit, L" Unauthorized access to email systems is illegal.                             \r\n", 10, TRUE, NULL);
    AppendText(hEdit, L"\r\n", 10, FALSE, NULL);

    // Troubleshooting
    AppendText(hEdit, L"🔧 Troubleshooting\r\n", 13, TRUE, NULL);
    AppendText(hEdit, L"1. Connection Timeouts: Reduce thread count or increase retry attempts\r\n", 10, FALSE, NULL);
    AppendText(hEdit, L"2. Rate Limiting: Lower thread count and add delays between attempts\r\n", 10, FALSE, NULL);
    
    // Restore and Unfreeze
    SendMessage(hEdit, EM_SETSEL, 0, 0);
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
    SendMessage(hEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hEdit, NULL, TRUE);
}

HWND CreateAboutPage(HWND parent) {
    // Ensure Rich Edit 4.1+ is loaded
    if (!g_hRichEditLib) {
        g_hRichEditLib = LoadLibraryW(L"Msftedit.dll");
    }

    // Create the control (MSFTEDIT_CLASS)
    g_hRichEdit = CreateWindowExW(0, MSFTEDIT_CLASS, L"", 
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 0, 0, parent, NULL, main_hInstance, NULL);

    // Set Margins
    RECT rcMsg = { 20, 20, 20, 20 }; 
    SendMessage(g_hRichEdit, EM_SETRECT, 0, (LPARAM)&rcMsg);
    
    // Populate
    PopulateRichEdit(g_hRichEdit);

    return parent;
}

BOOL About_PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ResizeAboutPage(int width, int height) {
    if (g_hRichEdit && IsWindow(g_hRichEdit)) {
        SetWindowPos(g_hRichEdit, NULL, 0, 0, width, height, SWP_NOZORDER);
        
        // Refresh formatting rect on resize to maintain margins
        RECT rcMsg = { 20, 20, width - 20, height - 20 }; 
        SendMessage(g_hRichEdit, EM_SETRECT, 0, (LPARAM)&rcMsg);
    }
}