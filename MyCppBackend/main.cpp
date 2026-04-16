#include "crow_all.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <algorithm>

using namespace std;

// The "Deep Clean" function to prevent JSON errors in browsers
string cleanJson(string str) {
    string output;
    for (char c : str) {
        switch (c) {
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += " "; break;
            case '\r': output += " "; break;
            case '\t': output += " "; break;
            default:
                if ('\x00' <= c && c <= '\x1f') continue;
                output += c;
                break;
        }
    }
    return output;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string googleSearch(string query) {
    string readBuffer;
    CURL* curl = curl_easy_init();
    if(curl) {
        const char* s_env = std::getenv("SERPER_API_KEY");
        string serper_key = (s_env != NULL) ? string(s_env) : "673cae971771d725b4e97ae33f48496170b6e88f";
        string url = "https://google.serper.dev/search";
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
    return readBuffer;
}

int main() {
    crow::SimpleApp app;

    // Health Check Route
    CROW_ROUTE(app, "/")([](){
        return "Restock AI Engine 1.3 - Status: Active";
    });

    // Main Chat Route with CORS support
    CROW_ROUTE(app, "/chat").methods("POST"_method, "OPTIONS"_method)([](const crow::request& req) {
        
        // --- HANDLE BROWSER SECURITY (The '204' Fix) ---
        if (req.method == "OPTIONS"_method) {
            crow::response res;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.code = 204; 
            return res;
        }

        // --- PROCESS AI REQUEST ---
        auto x = crow::json::load(req.body);
        if (!x) return crow::response(400, "Bad Request");
        
        string user_msg = x["message"].s();
        const char* g_env = std::getenv("GROQ_API_KEY");
        string groq_key = (g_env != NULL) ? string(g_env) : "IN_RENDER";
        
        string web_data = googleSearch(user_msg);
        string safe_web_data = cleanJson(web_data);
        string system_context = "You are Restock AI. User is Azlan. Context: " + safe_web_data;
        string safe_user_msg = cleanJson(user_msg);

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
        string ai_res = "Engine is slightly delayed. Try again in 5 seconds.";

        if (j && j.count("choices")) {
            ai_res = j["choices"][0]["message"]["content"].s();
        }

        // Final Response with all necessary headers
        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Content-Type", "application/json");
        
        crow::json::wvalue output;
        output["reply"] = ai_res;
        res.body = output.dump();
        return res;
    });

    const char* port_env = std::getenv("PORT");
    int port = (port_env != NULL) ? std::stoi(port_env) : 10000;
    app.port(port).multithreaded().run();
}