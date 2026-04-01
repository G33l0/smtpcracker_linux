#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <windows.h>

// Structure to hold SMTP Result data (aligns with your project logic)
typedef struct {
    char host[256];
    int port;
    char email[256];
    char password[256];
} SMTP_RESULT;

// File I/O
void SaveResultToFile(const char* filename, const SMTP_RESULT* result);

// UI Updates
void UpdateThreadCountLabel(HWND hLabel, int value);
void UpdateRetryCountLabel(HWND hLabel, int value);

// Telegram Functions (Requires libcurl)
// Returns 1 on success, 0 on failure
int TestTelegram(const char* token, const char* chat_id);
int SendDetailedTelegram(const char* token, const char* chat_id, const SMTP_RESULT* result);
int SendSummaryTelegram(const char* token, const char* chat_id, int valid, int failed, int total);

// Email Notification
void SendEmailNotification(const char* target_email, const SMTP_RESULT* result);

#endif // NOTIFICATIONS_H