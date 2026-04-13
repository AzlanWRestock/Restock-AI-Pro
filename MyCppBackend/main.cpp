#include "crow_all.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

using namespace std;

// IMPROVED CLEANUP: Escapes quotes and backslashes to prevent JSON crashes
string cleanJson(string str) {
    string output;
    for (char c : str) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\n') output += " ";
        else if (c == '\r') output += " ";
        else if (c == '\t') output += " ";
        else output += c;
    }
    return output;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// --- GOOGLE SEARCH ENGINE (SERPER.DEV) ---
string googleSearch(string query) {
    string readBuffer;
    CURL* curl = curl_easy_init();
    if(curl) {
        const char* s_env = std::getenv("SERPER_API_KEY");
        // Your current key as fallback
        string serper_key = (s_env != NULL) ? string(s_env) : "673cae971771d725b4e97ae33f48496170b6e88f";
        
        string url = "https://google.serper.dev/search";
        // Create a safe payload for the search request
        string payload = "{\"q\":\"" + cleanJson(query) + "\"}";

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("X-API-KEY: " + serper_key).c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer; // Return raw; we clean it before sending to AI
}

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "<h1>Restock AI Pro - Engine 1.0</h1><p>Status: ONLINE</p>";
    });

    CROW_ROUTE(app, "/chat").methods("POST"_method)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        string user_msg = x["message"].s();
        cout << "\n[USER]: " << user_msg << endl;

        const char* g_env = std::getenv("GROQ_API_KEY");
        // Your current key as fallback
        string groq_key = (g_env != NULL) ? string(g_env) : "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";
        
        string web_data = "";
        // Only search if the message is substantial
        if (user_msg.length() > 3) { 
            cout << "[SYSTEM]: Searching the web for: " << user_msg << endl;
            web_data = googleSearch(user_msg);
        }

        // Clean the search data BEFORE adding it to the context
        string safe_web_data = cleanJson(web_data);
        string system_context = "You are Restock AI. Answer using this LIVE data: " + (safe_web_data.empty() ? "No live data found." : safe_web_data);
        
        string safe_user_msg = cleanJson(user_msg);

        // Build the Final JSON Payload
        string payload = "{\"model\": \"llama-3.1-8b-instant\", \"messages\": ["
                         "{\"role\": \"system\", \"content\": \"" + system_context + "\"},"
                         "{\"role\": \"user\", \"content\": \"" + safe_user_msg + "\"}]}";

        string aiBuffer;
        CURL* curl = curl_easy_init();
        if(curl) {
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("Authorization: Bearer " + groq_key).c_str());
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &aiBuffer);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        auto j = crow::json::load(aiBuffer);
        string ai_res = "Engine is still warming up! Give me 10 more seconds and ask again.";

        if (j && j.count("choices") && j["choices"].size() > 0) {
            ai_res = j["choices"][0]["message"]["content"].s();
        } else {
            // This is vital for debugging in Render logs
            cout << "[DEBUG] Raw AI Buffer Response: " << aiBuffer << endl;
        }
        
        cout << "[AI]: " << ai_res << endl;

        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        crow::json::wvalue output;
        output["reply"] = ai_res;
        res.body = output.dump();
        return res;
    });

    const char* port_env = std::getenv("PORT");
    int port = (port_env != NULL) ? std::stoi(port_env) : 8080;
    app.port(port).multithreaded().run();
}