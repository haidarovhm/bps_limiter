#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include "lim.h"
#include "mu_test.h"


void lim_init(lim_t *lim, int rate) {
    (void)memset(lim, 0, sizeof(lim_t));
    lim->max_rate = rate;
    return;
}

bool lim_exceeds(lim_t *lim, double pkt_ts, int pkt_len) {
    if (pkt_ts < lim->last_ts) {
        return true;
    }

    double delta = pkt_ts - lim->last_ts;
    double budget = delta * lim->max_rate + lim->budget;

    if (budget > lim->max_rate) {
        budget = lim->max_rate;
    }

    double pkt_bits = pkt_len * 8;

    #ifndef NDEBUG
    printf("lim: pkt_ts=%.8f last_ts=%.8f delta=%.8f budget=%.8f pkt_bits=%.8f\n",
        pkt_ts, lim->last_ts, delta, budget, pkt_bits);
    #endif

    lim->last_ts = pkt_ts;

    if (pkt_len * 8.0 > budget) {
        return true;
    }
    lim->budget = budget - 8.0 * pkt_len;
    return false;
}


double micros(void) {
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);

    if (rc != 0) {
        perror("gettimeofday failed");
    }
    return tv.tv_sec + tv.tv_usec / 1e6;
}

const char *lim_tests(void) {
    lim_t lim;
    lim_init(&lim, 16); // 2 Bps

    double now = micros();

    mu_assert("2 bytes", lim_exceeds(&lim, now, 2) == false);
    mu_assert("excess byte", lim_exceeds(&lim, now, 1));
    now += 0.2; // 200 msec
    mu_assert("excess byte a moment later", lim_exceeds(&lim, now, 1));

    now += 0.5;
    mu_assert("another byte a half sec later", lim_exceeds(&lim, now, 1) == false);
    mu_assert("excess byte a half sec instanty", lim_exceeds(&lim, now, 1));

    now += 10;
    mu_assert("byte a long later", lim_exceeds(&lim, now, 1) == false);
    now += 1e-3;
    mu_assert("more one after a long later", lim_exceeds(&lim, now, 1) == false);
    now += 1e-3;
    mu_assert("excess byte after a long later", lim_exceeds(&lim, now, 1));

    return NULL;
}
