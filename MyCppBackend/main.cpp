#include "crow_all.h"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstdlib> // Required for getenv

using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// --- GOOGLE SEARCH (SERPER.DEV) ---
string googleSearch(string query) {
    string readBuffer;
    CURL* curl = curl_easy_init();
    if(curl) {
        // Keeping Serper key here for now as it's less sensitive than Groq
        string serper_key = "673cae971771d725b4e97ae33f48496170b6e88f";
        string url = "https://google.serper.dev/search";
        string payload = "{\"q\":\"" + query + "\"}";

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

    CROW_ROUTE(app, "/chat").methods("POST"_method)([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        string user_msg = x["message"].s();
        cout << "\n[AZLAN]: " << user_msg << endl;

        // --- DYNAMIC API KEY ---
        // Looks for the key in Render environment variables. 
        // If not found, it falls back to your provided key.
        const char* env_key = std::getenv("GROQ_API_KEY");
        string groq_key = (env_key != NULL) ? string(env_key) : "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";
        
        // --- WEB SEARCH TRIGGER ---
        string web_data = "";
        if (user_msg.find("who") != string::npos || user_msg.find("what") != string::npos || 
            user_msg.find("news") != string::npos || user_msg.find("score") != string::npos) {
            cout << "[SYSTEM]: Fetching Google Search results..." << endl;
            web_data = googleSearch(user_msg);
        }

        // --- PREPARING THE BRAIN ---
        string system_context = "You are Restock AI, a helpful assistant for Azlan. ";
        if (!web_data.empty()) {
            system_context += "Use these Google search results to answer: " + web_data;
        }

        string payload = "{\"model\": \"llama-3.1-8b-instant\", \"messages\": ["
                         "{\"role\": \"system\", \"content\": \"" + system_context + "\"},"
                         "{\"role\": \"user\", \"content\": \"" + user_msg + "\"}]}";

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
        string ai_res = j["choices"][0]["message"]["content"].s();
        cout << "[AI]: " << ai_res << endl;

        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        
        crow::json::wvalue output;
        output["reply"] = ai_res;
        res.body = output.dump();
        return res;
    });

    cout << "===========================================" << endl;
    cout << "   RESTOCK AI 3.0 - GOOGLE SEARCH ACTIVE   " << endl;
    cout << "===========================================" << endl;

    // --- CLOUD PORT CONFIG ---
    // Render tells the app which port to use. If it can't find one, it uses 8080.
    const char* port_env = std::getenv("PORT");
    int port = (port_env != NULL) ? std::stoi(port_env) : 8080;
    
    app.port(port).multithreaded().run();
}