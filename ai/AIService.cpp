#include "AIService.h"
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <cmath>

#ifdef USE_CURL
#include <curl/curl.h>

static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t total = size * nmemb;
    out->append(static_cast<char*>(contents), total);
    return total;
}
#endif

AIService::AIService() : provider(Provider::NONE) {
    const char* geminiKey = std::getenv("GEMINI_API_KEY");
    const char* openaiKey = std::getenv("OPENAI_API_KEY");
    if (geminiKey && std::string(geminiKey).size() > 0) {
        provider = Provider::GEMINI;
        apiKey = geminiKey;
    } else if (openaiKey && std::string(openaiKey).size() > 0) {
        provider = Provider::OPENAI;
        apiKey = openaiKey;
    } else {
        provider = Provider::NONE; // fallback / offline mode
    }
}

std::string AIService::callCloudAI(const std::string& prompt) const {
#ifdef USE_CURL
    if (provider == Provider::NONE) return "";
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    struct curl_slist* headers = nullptr;
    std::string url;
    std::string body;

    // Very small, dependency-free JSON string escaping for the prompt.
    std::string escaped;
    for (char c : prompt) {
        if (c == '"' || c == '\\') escaped.push_back('\\');
        if (c == '\n') { escaped += "\\n"; continue; }
        escaped.push_back(c);
    }

    if (provider == Provider::GEMINI) {
        url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        body = "{\"contents\":[{\"parts\":[{\"text\":\"" + escaped + "\"}]}]}";
    } else { // OPENAI
        url = "https://api.openai.com/v1/chat/completions";
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());
        body = "{\"model\":\"gpt-4o-mini\",\"messages\":[{\"role\":\"user\",\"content\":\"" + escaped + "\"}]}";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    // NOTE: a production build should parse this with a proper JSON library
    // (e.g. nlohmann/json). To keep this project dependency-light we return
    // the raw response body; the calling UI layer displays it directly, and
    // main.cpp shows how to pull out the "text"/"content" field with a
    // lightweight substring search for demo purposes.
    return response;
#else
    (void)prompt;
    return ""; // built without USE_CURL -> always use offline fallback
#endif
}

// --------------------------------------------------------------------------
// Each public method below: try the Cloud AI call first (if configured),
// otherwise fall back to a transparent, clearly-labelled rule-based estimate
// computed from the same DSA-derived numbers (distance from Dijkstra, drone
// battery, queue lengths, etc.) so the system is always fully functional.
// --------------------------------------------------------------------------

std::string AIService::predictDeliveryTime(double distanceKm, double droneSpeedKmph,
                                            const std::string& weatherHint) const {
    std::ostringstream prompt;
    prompt << "Predict a realistic drone delivery time in minutes for a distance of "
           << distanceKm << " km at a cruise speed of " << droneSpeedKmph
           << " km/h, weather: " << weatherHint
           << ". Reply with one short sentence including the estimated minutes.";

    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;

    double baseMinutes = (distanceKm / droneSpeedKmph) * 60.0;
    double weatherFactor = (weatherHint == "storm" || weatherHint == "rain") ? 1.35 : 1.05;
    double estimate = baseMinutes * weatherFactor;
    std::ostringstream fallback;
    fallback << "[Offline estimate] Approx. " << std::round(estimate)
              << " minutes at " << droneSpeedKmph << " km/h over " << distanceKm << " km ("
              << weatherHint << " conditions applied).";
    return fallback.str();
}

std::string AIService::suggestBestRoute(const std::string& pickup, const std::string& destination,
                                         double graphDistanceKm) const {
    std::ostringstream prompt;
    prompt << "A drone must travel from " << pickup << " to " << destination
           << ". The shortest graph distance computed via Dijkstra is "
           << graphDistanceKm << " km. Suggest one practical routing tip (altitude, no-fly zone awareness, or corridor choice) in one sentence.";
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;
    return "[Offline suggestion] Follow the Dijkstra-computed shortest corridor (" +
           std::to_string(graphDistanceKm) + " km); maintain cruise altitude and avoid restricted zones near " + destination + ".";
}

std::string AIService::weatherAdvice(const std::string& city) const {
    std::ostringstream prompt;
    prompt << "Give one short practical weather-related safety tip for flying a delivery drone today in " << city << ".";
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;
    return "[Offline advice] No live weather feed configured -- verify local wind speed (<25 km/h) and visibility before dispatching drones to " + city + ".";
}

std::string AIService::batteryRecommendation(double batteryPercent, double distanceKm) const {
    std::ostringstream prompt;
    prompt << "A delivery drone has " << batteryPercent << "% battery and must fly " << distanceKm
           << " km round trip. Recommend whether it is safe to dispatch, in one sentence.";
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;

    double estUsagePercent = distanceKm * 1.4; // simple heuristic: ~1.4% battery per km round trip
    bool safe = (batteryPercent - estUsagePercent) > 15.0;
    std::ostringstream fallback;
    fallback << "[Offline recommendation] Estimated usage ~" << std::round(estUsagePercent)
              << "% for this trip; " << (safe ? "SAFE to dispatch." : "recommend charging before dispatch (low safety margin).");
    return fallback.str();
}

std::string AIService::chatbotResponse(const std::string& userMessage) const {
    std::ostringstream prompt;
    prompt << "You are a helpful customer support assistant for a drone delivery company. "
           << "Answer the customer's question concisely: \"" << userMessage << "\"";
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;
    return "[Offline chatbot] Thanks for your message. Our support team will follow up shortly. "
           "You can track your delivery status anytime from 'Track Package' in your dashboard.";
}

std::string AIService::generateSmartReportSummary(const std::string& rawCsvOrStats) const {
    std::ostringstream prompt;
    prompt << "Summarize the following drone delivery operations data into a short executive summary with key insights:\n"
           << rawCsvOrStats;
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;
    return "[Offline summary] Report generated from live system data. Configure GEMINI_API_KEY or OPENAI_API_KEY "
           "to enable AI-written executive summaries.";
}

std::string AIService::optimizationSuggestions(int pendingCount, int availableDrones) const {
    std::ostringstream prompt;
    prompt << "There are " << pendingCount << " pending deliveries and " << availableDrones
           << " available drones. Suggest one operational optimization in one sentence.";
    std::string aiResp = callCloudAI(prompt.str());
    if (!aiResp.empty()) return aiResp;

    std::ostringstream fallback;
    if (availableDrones == 0 && pendingCount > 0)
        fallback << "[Offline suggestion] No drones available with " << pendingCount << " pending -- prioritize charging/maintenance turnaround or add fleet capacity.";
    else if (pendingCount > availableDrones * 3)
        fallback << "[Offline suggestion] Backlog is high relative to fleet size -- consider batching deliveries along overlapping Dijkstra routes.";
    else
        fallback << "[Offline suggestion] Fleet capacity looks healthy relative to current demand.";
    return fallback.str();
}
