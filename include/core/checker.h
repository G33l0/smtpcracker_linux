#ifndef CHECKER_H
#define CHECKER_H

#include <windows.h>


typedef struct {
    char email[128];
    char host[128];
    char port[16];
    char pass[128];
    char status[64];
    char delivery[64];
} RESULT_DATA;

typedef struct {
    char** combos;
    int total;
    int current;
    BOOL running;
    HANDLE mutex;
    HANDLE semaphore;
} WORKER_STATE;
extern WORKER_STATE g_state; 
void HandleUploadCombo(HWND hwnd);
void StartCheckingProcess();
unsigned __stdcall SmtpThreadProc(void* param);
#endif