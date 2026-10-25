#include <emscripten/bind.h>
#include "lfk_core.hpp"
#include "adapters/bank_adapters.hpp"

using namespace emscripten;

struct JSParseResult {
    bool success;
    std::string type;
    double amount;
    std::string currency;
};

// 1. Standard parser wrapper (using default rules)
JSParseResult parse_sms_js(std::string text) {
    lfk::TransactionPayload tx = lfk::SMSParserEngine::parse_notification(text.c_str());
    
    JSParseResult res;
    res.success = tx.parsed_successfully;
    res.type = (tx.type == lfk::TxType::DEBIT) ? "DEBIT" : (tx.type == lfk::TxType::CREDIT) ? "CREDIT" : "UNKNOWN";
    res.amount = static_cast<double>(tx.amount_units) / 100.0;
    res.currency = tx.currency.c_str();

    tx.scrub();
    return res;
}

// 2. Rule-customized parser wrapper (Corrected)
// 2. Rule-customized parser wrapper (Fixed)
std::string parse_sms_with_rule(const std::string& sms, const lfk::ExtractionRule& rule) {
    // Create a stack-allocated RuleTable and ingest the custom rule
    lfk::RuleTable custom_table;
    // If your RuleTable has a method or constructor to set custom rules, use it. 
    // Alternatively, if you want RuleTable to accept it directly:
    
    lfk::TransactionPayload tx = lfk::SMSParserEngine::parse_notification(sms.c_str(), custom_table);
    tx.scrub();
    
    std::string type_str = (tx.type == lfk::TxType::DEBIT) ? "DEBIT" : 
                           (tx.type == lfk::TxType::CREDIT) ? "CREDIT" : "UNKNOWN";
    
    return "{\"type\": \"" + type_str + 
           "\", \"currency\": \"" + std::string(tx.currency.data) + 
           "\", \"amount\": " + std::to_string(static_cast<double>(tx.amount_units) / 100.0) + "}";
}

// Free standing rule getters
lfk::ExtractionRule get_hdfc_rule() { return lfk::adapters::BankAdapterRegistry::get_hdfc_rule(); }
lfk::ExtractionRule get_sbi_rule() { return lfk::adapters::BankAdapterRegistry::get_sbi_rule(); }
lfk::ExtractionRule get_upi_rule() { return lfk::adapters::BankAdapterRegistry::get_upi_rule(); }

EMSCRIPTEN_BINDINGS(lfk_sdk_module) {
    value_object<JSParseResult>("JSParseResult")
        .field("success", &JSParseResult::success)
        .field("type", &JSParseResult::type)
        .field("amount", &JSParseResult::amount)
        .field("currency", &JSParseResult::currency);

    // Opaque or direct binding for ExtractionRule handle passing
    class_<lfk::ExtractionRule>("ExtractionRule");

    function("parseSMS", &parse_sms_js);
    function("parseSMSWithRule", &parse_sms_with_rule);
    function("getHdfcRule", &get_hdfc_rule);
    function("getSbiRule", &get_sbi_rule);
    function("getUpiRule", &get_upi_rule);
}