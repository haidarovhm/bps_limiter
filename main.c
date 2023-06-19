#include <stdio.h>
#include <stdlib.h>

#include <pcap/pcap.h>

#include "lim.h"


#define PCAP_NEXT_EOF -2
#define PCAP_NEXT_ERR -1


int run_tests(void) {
    const char *res = lim_tests();
    if (res == NULL) {
        printf("tests ok\n");
        return 0;
    }
    printf("tests failed: %s\n", res);
    return 1;
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        return run_tests();
    }

    if (argc != 4 || *argv[1] == '\0' || *argv[2] == '\0' || *argv[3] == '\0') {
        fprintf(stderr, "bad args\n");
        return 1;
    }

    const char *in_file = argv[1];
    const char *out_file = argv[2];
    int bps_rate = atoi(argv[3]);

    if (bps_rate == 0) {
        fprintf(stderr, "bad bps rate");
        return 1;
    }

    char err_buf[PCAP_ERRBUF_SIZE] = {0};

    pcap_t *pcap = pcap_open_offline_with_tstamp_precision(in_file,
        PCAP_TSTAMP_PRECISION_MICRO,
        err_buf);
    if (pcap == NULL) {
        fprintf(stderr, "cannot read pcap file '%s': %s\n",
            in_file,
            err_buf);
        return 1;
    }

    pcap_dumper_t *dumper = pcap_dump_open(pcap, out_file);
    if (dumper == NULL) {
        fprintf(stderr, "cannot dump to file: %s\n",
            pcap_geterr(pcap));
        pcap_close(pcap);
        return 1;
    }


    struct pcap_pkthdr *header = NULL;
    const u_char *data = NULL;
    lim_t limiter;

    int rc;
    int total_cnt = 0, drop_cnt = 0;
    double total_len = 0, first_ts = 0, last_ts;

    lim_init(&limiter, bps_rate);
    while (1) {
        rc = pcap_next_ex(pcap, &header, &data);

        if (rc == PCAP_NEXT_EOF) {
            break;
        }

        if (rc == PCAP_NEXT_ERR) {
            fprintf(stderr, "cannot read next packet: %s\n",
                pcap_geterr(pcap));
            break;
        }

        double ts = header->ts.tv_sec + header->ts.tv_usec / 1e6;

        if (first_ts == 0) {
            first_ts = ts;
        }
        last_ts = ts;

        #ifndef NDEBUG
        printf("pkt[%d]: len=%d ts=%.8lf\n", total_cnt, header->len, ts);
        #endif

        total_cnt++;
        total_len += header->len;

        if (lim_exceeds(&limiter, ts, header->len)) {
            drop_cnt++;
            continue;
        }
        pcap_dump((u_char *)dumper, header, data);
    }

    if (rc == PCAP_NEXT_EOF) {
        double avg_len = total_len / total_cnt;
        double avg_rate = total_len * 8 / (last_ts - first_ts);
        double est_rate = (total_cnt - drop_cnt) * avg_len * 8 / (last_ts - first_ts);

        printf("pkts:\n"
            "    total=%d droped=%d avg_len=%.2f\n"
            "    avg_rate=%.2f est_rate=%.2f\n",
            total_cnt, drop_cnt, avg_len,
            avg_rate, est_rate);
    }

    pcap_dump_close(dumper);
    pcap_close(pcap);
    return rc == PCAP_NEXT_ERR;
}
