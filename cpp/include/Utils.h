#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <functional>

namespace utils {

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) tokens.push_back(item);
    return tokens;
}

// Simple deterministic password hash (NOT for production use -- a real system
// must use bcrypt/argon2. Sufficient for demonstrating "secure admin access"
// / "input validation" at academic-project scope: passwords are never stored
// in plain text).
inline std::string simpleHash(const std::string& input, const std::string& salt = "drone_salt_2026") {
    std::hash<std::string> hasher;
    size_t h = hasher(salt + input);
    std::ostringstream os;
    os << std::hex << h;
    return os.str();
}

inline std::string nowTimestamp() {
    time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

inline std::string formatTime(time_t t) {
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

// very small helper to escape quotes for embedding raw strings into JSON
inline std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

inline bool isValidEmail(const std::string& email) {
    auto at = email.find('@');
    auto dot = email.find_last_of('.');
    return at != std::string::npos && dot != std::string::npos && at < dot && at > 0;
}

inline bool isPositiveNumber(double v) { return v > 0.0; }

/*-----------------------------------------------------------------------------
 * Minimal, dependency-free flat-JSON field extractors.
 * These are intentionally NOT a general JSON parser -- they are sufficient
 * for reading the simple, flat {"key":"value", "key2":123} bodies this
 * project's own frontend sends to the HTTP API. For anything more complex,
 * swap in a real library such as nlohmann/json.
 *---------------------------------------------------------------------------*/
inline std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    auto end = json.find('"', pos + 1);
    while (end != std::string::npos && json[end - 1] == '\\') end = json.find('"', end + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

inline double extractJsonNumber(const std::string& json, const std::string& key, double def = 0.0) {
    std::string pattern = "\"" + key + "\"";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return def;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return def;
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '.' || json[end] == '-')) end++;
    if (end == pos) return def;
    try { return std::stod(json.substr(pos, end - pos)); } catch (...) { return def; }
}

} // namespace utils

#endif // UTILS_H
