#ifndef LFK_CORE_HPP
#define LFK_CORE_HPP

#include <cstdint>
#include <cstring>

namespace lfk {

enum class TxType { UNKNOWN, DEBIT, CREDIT };

struct StaticString {
    char data[32];
    uint8_t len;

    void set(const char* src, uint8_t max_len) {
        len = 0;
        while (src[len] != '\0' && len < max_len - 1) {
            data[len] = src[len];
            len++;
        }
        data[len] = '\0';
    }

    const char* c_str() const { return data; }

    bool contains(const char* substr) const {
        return std::strstr(data, substr) != nullptr;
    }
};

// OTA Extraction Rule structure (statically sized for zero heap use)
struct ExtractionRule {
    StaticString debit_keyword;
    StaticString credit_keyword;
    StaticString currency_symbol;
    bool active;
};

// Rule Table managing active OTA rules on device
class RuleTable {
private:
    ExtractionRule current_rule;
    bool has_custom_rule;

public:
    RuleTable() : has_custom_rule(false) {
        // Set safe defaults
        current_rule.debit_keyword.set("debited", 32);
        current_rule.credit_keyword.set("credited", 32);
        current_rule.currency_symbol.set("INR", 4);
        current_rule.active = true;
    }

    bool ingest_ota_payload(const uint8_t* payload, size_t size) {
        if (!payload || size != sizeof(ExtractionRule)) {
            return false;
        }
        // Zero-copy / direct memory copy into static storage block
        std::memcpy(&current_rule, payload, sizeof(ExtractionRule));
        has_custom_rule = true;
        return true;
    }

    const ExtractionRule& get_active_rule() const {
        return current_rule;
    }
};

struct TransactionPayload {
    bool parsed_successfully;
    TxType type;
    uint64_t amount_units; // Stored in fixed-point cents
    StaticString currency;
    StaticString merchant;

    void scrub() {
        // Volatile memory scrubbing for security
        volatile uint8_t* ptr = reinterpret_cast<volatile uint8_t*>(this);
        for (size_t i = 0; i < sizeof(TransactionPayload); ++i) {
            ptr[i] = 0;
        }
    }
};

class SMSParserEngine {
public:
    // Overload 1: Default parsing
    static TransactionPayload parse_notification(const char* sms) {
        RuleTable default_table;
        return parse_notification(sms, default_table);
    }

    // Overload 2: Rule-driven zero-heap parsing
    static TransactionPayload parse_notification(const char* sms, const RuleTable& rules) {
        TransactionPayload tx;
        std::memset(&tx, 0, sizeof(TransactionPayload));

        if (!sms) return tx;

        const ExtractionRule& rule = rules.get_active_rule();

        if (rule.active) {
            if (rule.debit_keyword.len > 0 && std::strstr(sms, rule.debit_keyword.c_str()) != nullptr) {
                tx.type = TxType::DEBIT;
                tx.parsed_successfully = true;
            } else if (rule.credit_keyword.len > 0 && std::strstr(sms, rule.credit_keyword.c_str()) != nullptr) {
                tx.type = TxType::CREDIT;
                tx.parsed_successfully = true;
            }
        }

        if (!tx.parsed_successfully) {
            return tx;
        }

        // --- Robust Zero-Heap Linear Time Numerical Extractor ---
        tx.amount_units = 0;
        const char* p = sms;
        bool found_digits = false;
        uint64_t integer_part = 0;
        uint64_t fractional_part = 0;
        int decimal_places = -1;
        bool number_finished = false;

        while (*p != '\0' && !number_finished) {
            if (*p >= '0' && *p <= '9') {
                found_digits = true;
                if (decimal_places == -1) {
                    integer_part = (integer_part * 10) + (*p - '0');
                } else {
                    decimal_places++;
                    if (decimal_places <= 2) {
                        fractional_part = (fractional_part * 10) + (*p - '0');
                    }
                }
            } else if (*p == '.' && decimal_places == -1 && found_digits) {
                // Only treat as decimal point if we've already seen digits (skips "Rs.")
                decimal_places = 0;
            } else if (*p == ',' && decimal_places == -1) {
                // Ignore thousand separators within the integer part
            } else {
                if (found_digits) {
                    number_finished = true;
                }
            }
            p++;
        }

        // Normalize to fixed-point cents
        if (decimal_places == 1) {
            fractional_part *= 10;
        }
        tx.amount_units = (integer_part * 100) + (decimal_places == -1 ? 0 : fractional_part);
        // -------------------------------------------------------------------------
        
        tx.currency.set(rule.currency_symbol.c_str(), 4);
        tx.merchant.set("UPI Merchant", 16);

        return tx;
    }
};

} // namespace lfk

#endif // LFK_CORE_HPP