#include <math.h>
#include "pav_analysis.h"

float compute_power(const float *x, unsigned int N) {
    float power = 0.0;
    for (unsigned int n = 0; n < N; n++) {
        power += x[n] * x[n];
    }
    power /= N;
    return 10*log10(power);
}

float compute_am(const float *x, unsigned int N) {
    float am = 0.0;
    for (unsigned int n = 0; n < N; n++) {
        am += fabs(x[n]);
    }
    am /= N;
    return am;
}

float compute_zcr(const float *x, unsigned int N, float fm) {
    float zcr = 0.0;
    for (unsigned int n = 1; n < N; n++) {
        if ((x[n-1] > 0 && x[n] < 0) || (x[n-1] < 0 && x[n] > 0)) {
            zcr += 1.0;
        }
    }
    zcr /= (N - 1);
    zcr *= fm;
    return zcr;
}
    