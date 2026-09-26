#ifndef AI_SERVICE_H
#define AI_SERVICE_H

#include <string>

/*******************************************************************************
 * AIService
 * -----------------------------------------------------------------------------
 * Thin wrapper around Cloud AI APIs (Google Gemini / OpenAI). This project
 * intentionally does NOT ship or train any local model -- every AI feature is
 * a network call to a hosted API, guarded by an API key supplied via
 * environment variable.
 *
 * Provider is selected at construction time. If no key is present in the
 * environment, every method degrades gracefully to a clearly-labelled
 * rule-based fallback so the rest of the system (which is pure DSA/C++) still
 * compiles and runs end-to-end without an internet connection or paid key --
 * this keeps the "core logic is C++" requirement completely independent of
 * AI availability.
 ******************************************************************************/
class AIService {
public:
    enum class Provider { GEMINI, OPENAI, NONE };

    AIService();

    Provider activeProvider() const { return provider; }
    bool isCloudAIAvailable() const { return provider != Provider::NONE; }

    std::string predictDeliveryTime(double distanceKm, double droneSpeedKmph,
                                     const std::string& weatherHint = "clear") const;

    std::string suggestBestRoute(const std::string& pickup, const std::string& destination,
                                  double graphDistanceKm) const;

    std::string weatherAdvice(const std::string& city) const;

    std::string batteryRecommendation(double batteryPercent, double distanceKm) const;

    std::string chatbotResponse(const std::string& userMessage) const;

    std::string generateSmartReportSummary(const std::string& rawCsvOrStats) const;

    std::string optimizationSuggestions(int pendingCount, int availableDrones) const;

private:
    Provider provider;
    std::string apiKey;

    // Performs the actual HTTPS call to the selected provider's completion
    // endpoint. Implemented with libcurl in AIService.cpp. Returns empty
    // string on any network/parse failure (callers then use the fallback).
    std::string callCloudAI(const std::string& prompt) const;
};

#endif // AI_SERVICE_H
