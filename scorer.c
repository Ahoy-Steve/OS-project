#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define NAME_LEN 64
#define ISSUE_LEN 50
#define DESCRIPTION_LEN 50
#define PATH_LEN 256

typedef struct {
    double longitude;
    double latitude;
}COORDINATE_T;

typedef struct {
    int id;
    char name[NAME_LEN];
    COORDINATE_T gps;
    char issue[ISSUE_LEN];
    int severity; // 1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char description[DESCRIPTION_LEN];
}REPORT_T;

typedef struct {
    char name[NAME_LEN];
    int score;
    int count;
}INSPECTOR_T;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"Usage: scorer <district_id>\n");
        return 1;
    }

    char *district_id = argv[1];
    char report_path[PATH_LEN];
    snprintf(report_path, sizeof(report_path), "%s/reports.dat", district_id);

    int fd = open(report_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"Error opening report file\n");
        return 1;
    }

    INSPECTOR_T inspectors[64];
    int num_inspectors = 0;
    REPORT_T report;

    while (read(fd, &report, sizeof(report)) == sizeof(REPORT_T)) {
        int found_inspector = 0;
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(inspectors[i].name, report.name) == 0) {
                inspectors[i].score = report.severity;
                inspectors[i].count++;
                found_inspector = 1;
                break;
            }
        }
        if (!found_inspector && num_inspectors < 64) {
            strncpy(inspectors[num_inspectors].name, report.name, NAME_LEN - 1);
            inspectors[num_inspectors].score = report.severity;
            inspectors[num_inspectors].count = 1;
            num_inspectors++;
        }
    }

    close(fd);

    printf("=-=-=-= District: %s =-=-=-= \n", district_id);
    if (num_inspectors == 0) {
        printf("No reports.\n");
    }else {
        for (int i = 0; i < num_inspectors; i++) {
            printf("Inspector: %-20s Reports: %-5d Score: %d\n", inspectors[i].name, inspectors[i].count, inspectors[i].score);
        }
    }

    printf("\n");

    return 0;
}