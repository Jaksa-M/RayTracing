#ifndef MAIN_UTILITY_H
#define MAIN_UTILITY_H

inline void switchToFastMode(float& trace_percentage, int32& max_bounces) {
    trace_percentage = 0.1f;
    max_bounces = 2;
}

inline void switchToQualityMode(float& trace_percentage, int32& max_bounces) {
    trace_percentage = 1.0f;
    max_bounces = 5;
}


#endif
