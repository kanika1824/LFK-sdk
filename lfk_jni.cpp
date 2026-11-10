// lfk_jni.cpp
#include <jni.h>
#include <string>
#include "lfk_core.hpp"

extern "C" {

JNIEXPORT jobject JNICALL
Java_com_lfk_sdk_LFKBridge_nativeEvaluateSMS(
    JNIEnv* env, 
    jobject thiz, 
    jstring sms_payload, 
    jboolean active_call, 
    jboolean payment_intent
) {
    const char* native_sms = env->GetStringUTFChars(sms_payload, nullptr);
    uint32_t len = native_sms ? std::strlen(native_sms) : 0;

    // Run core evaluation
    lfk::TransactionPayload tx = lfk::SMSParserEngine::parse_notification(native_sms);

    env->ReleaseStringUTFChars(sms_payload, native_sms);

    // Map to Java result object (LFKEvaluationResult)
    jclass result_class = env->FindClass("com/lfk/sdk/LFKEvaluationResult");
    jmethodID constructor = env->GetMethodID(result_class, "<init>", "(ZDLjava/lang/String;)V");

    jstring j_currency = env->NewStringUTF(tx.currency.c_str());
    double computed_amount = static_cast<double>(tx.amount_units) / 100.0;

    jobject j_result = env->NewObject(
        result_class, 
        constructor, 
        (jboolean)tx.parsed_successfully, 
        computed_amount, 
        j_currency
    );

    // Scrub memory before returning
    tx.scrub();

    return j_result;
}

} // extern "C"
