#ifndef _LIM_H_
#define _LIM_H_

#include <time.h>
#include <stdbool.h>


typedef struct {
    double last_ts;
    double max_rate;
    double budget;
} lim_t;

void lim_init(lim_t *lim, int rate);
bool lim_exceeds(lim_t *lim, double ts, int bytes);

const char *lim_tests(void);

#endif
