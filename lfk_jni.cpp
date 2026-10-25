#include <jni.h>
#include <string>
#include "lfk_core.hpp"

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_lfk_sdk_LFKEngine_nativeParseSMS(JNIEnv* env, jclass /*this*/, jstring sms_jstr) {
    if (!sms_jstr) {
        return env->NewStringUTF("{\"success\":false,\"error\":\"Null SMS string\"}");
    }

    // Convert Java String to C-style string without heap allocation where possible
    const char* sms_chars = env->GetStringUTFChars(sms_jstr, nullptr);
    
    // Execute zero-heap C++ parsing
    lfk::TransactionPayload tx = lfk::SMSParserEngine::parse_notification(sms_chars);
    
    env->ReleaseStringUTFChars(sms_jstr, sms_chars);

    // Format output as JSON string to return to Kotlin/Java
    char json_buffer[256];
    double amount_float = static_cast<double>(tx.amount_units) / 100.0;
    
    const char* type_str = (tx.type == lfk::TxType::DEBIT) ? "DEBIT" : 
                           (tx.type == lfk::TxType::CREDIT) ? "CREDIT" : "UNKNOWN";

    std::snprintf(json_buffer, sizeof(json_buffer),
        "{\"success\":%s,\"type\":\"%s\",\"amount\":%.2f,\"currency\":\"%s\"}",
        tx.parsed_successfully ? "true" : "false",
        type_str,
        amount_float,
        tx.currency.c_str()
    );

    return env->NewStringUTF(json_buffer);
}

} // extern "C"