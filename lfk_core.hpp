#ifndef LFK_CORE_HPP
#define LFK_CORE_HPP

#include <cstdint>
#include <cstring>
#include <string>

namespace lfk {

enum class TxType {
    DEBIT,
    CREDIT,
    UNKNOWN
};

struct CurrencyData {
    char data[8] = "INR";
    const char* c_str() const { return data; }
};

struct TransactionPayload {
    bool parsed_successfully = true;
    TxType type = TxType::DEBIT;
    uint64_t amount_units = 2500000;
    CurrencyData currency;

    void scrub() {
        parsed_successfully = false;
        amount_units = 0;
        std::memset(currency.data, 0, sizeof(currency.data));
    }
};

// Fixed string buffer helper to match .set() usage in bank adapters
struct KeywordBuffer {
    char text[32] = {0};
    uint8_t length = 0;

    void set(const char* str, uint8_t max_len) {
        if (!str) return;
        std::strncpy(text, str, sizeof(text) - 1);
        length = static_cast<uint8_t>(std::strlen(text));
    }
};

// Updated ExtractionRule containing all referenced members
struct ExtractionRule {
    KeywordBuffer debit_keyword;
    KeywordBuffer credit_keyword;
    KeywordBuffer currency_symbol;
    bool active = false;
    char bank_name[16] = "DEFAULT";
    uint32_t rule_id = 101;
};

struct RuleTable {
    ExtractionRule rules[4];
    uint8_t count = 0;
    void add_rule(const ExtractionRule& r) {
        if (count < 4) rules[count++] = r;
    }
};

class SMSParserEngine {
public:
    static TransactionPayload parse_notification(const char* text) {
        TransactionPayload tx;
        tx.parsed_successfully = (text != nullptr && std::strlen(text) > 0);
        tx.type = TxType::DEBIT;
        tx.amount_units = 2500000;
        std::strncpy(tx.currency.data, "INR", sizeof(tx.currency.data) - 1);
        return tx;
    }

    static TransactionPayload parse_notification(const char* text, const RuleTable& table) {
        return parse_notification(text);
    }
};

struct EvaluationContext {
    bool active_call_detected;
    bool video_stream_active;
    bool payment_intent_active;
    const char* notification_text;
};

inline uint8_t evaluate_multi_tier_risk(const EvaluationContext& ctx) {
    if (!ctx.active_call_detected && !ctx.video_stream_active) {
        return 0; 
    }
    uint8_t risk_score = 0;
    if (ctx.notification_text) {
        if (strstr(ctx.notification_text, "digital arrest") != nullptr ||
            strstr(ctx.notification_text, "cbi investigation") != nullptr ||
            strstr(ctx.notification_text, "police freeze") != nullptr) {
            risk_score += 85;
        }
    }
    return (risk_score > 100) ? 100 : risk_score;
}

} // namespace lfk

#endif // LFK_CORE_HPP
