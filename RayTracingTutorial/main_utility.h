#ifndef MAIN_UTILITY_H
#define MAIN_UTILITY_H

inline void switchToFastMode(float& trace_percentage, int& reflection_depth) {
    trace_percentage = 0.1f;
    reflection_depth = 2;
}

inline void switchToQualityMode(float& trace_percentage, int& reflection_depth) {
    trace_percentage = 1.0f;
    reflection_depth = 5;
}


#endif
