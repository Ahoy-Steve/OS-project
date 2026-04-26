#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define NAME_LEN 64
#define ISSUE_LEN 50
#define DESCRIPTION_LEN 256
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

void create_district(char *district_id) {
    //Create the district directory
    mkdir(district_id, 0750);
    chmod(district_id, 0750);

    //Build the paths
    char reports_path[PATH_LEN];
    char cfg_path[PATH_LEN];
    char log_path[PATH_LEN];

    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);
    snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", district_id);
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

    //Creating the files and setting their permissions
    int input;

    input = open(reports_path, O_WRONLY | O_CREAT, 0664);
    if (input >= 0) {
        chmod(reports_path, 0664);
        close(input);
    }

    input = open(cfg_path, O_WRONLY | O_CREAT, 0640);
    if (input >= 0) {
        chmod(cfg_path, 0640);
        close(input);
    }

    input = open(log_path, O_WRONLY | O_CREAT, 0664);
    if (input >= 0) {
        chmod(log_path, 0644);
        close(input);
    }
}

int check_permissions(char *path, mode_t required_mask) {
    struct stat st;
    if (stat(path, &st) < 0) {
        perror(path);
        return 1;
    }
    return (st.st_mode & required_mask) == required_mask;
}

void mode_to_string(mode_t mode, char *mode_str) {
    mode_str[0] = (mode & S_IRUSR) ? 'r' : '-';
    mode_str[1] = (mode & S_IWUSR) ? 'w' : '-';
    mode_str[2] = (mode & S_IXUSR) ? 'x' : '-';
    mode_str[3] = (mode & S_IRGRP) ? 'r' : '-';
    mode_str[4] = (mode & S_IWGRP) ? 'w' : '-';
    mode_str[5] = (mode & S_IXGRP) ? 'x' : '-';
    mode_str[6] = (mode & S_IROTH) ? 'r' : '-';
    mode_str[7] = (mode & S_IWOTH) ? 'w' : '-';
    mode_str[8] = (mode & S_IXOTH) ? 'x' : '-';
    mode_str[9] = '\0';
}

void log_action(char *district_id, char *role, char *user, char *action) {
    char log_path[PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

    if (strcmp(role, "inspector") == 0) {
        return;
    }

    struct stat st;
    if (stat(log_path, &st) == 0 && !(st.st_mode & S_IWUSR)) {
        fprintf(stderr, "Permission denied: owner-write bit not set on log.\n");
        return;
    }

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open log");
        return;
    }

    char time_buff[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", tm_info);

    char entry[512];
    int len = snprintf(entry, sizeof(entry), "[%s] role=%s user=%s action=%s\n", time_buff, role, user, action);

    write(fd, entry, len);
    close(fd);
}

void check_symlink(char *district_id) {
    char link_name[PATH_LEN];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);

    struct stat lst;
    if (lstat(link_name, &lst) < 0) {
        return;
    }

    if (S_ISLNK(lst.st_mode)) {
        struct stat tgt;
        if (stat(link_name, &tgt) < 0) {
            fprintf(stderr, "Warning: active_reports-%s is a dangling symlink.\n", district_id);
        }
    }
}

void add(char *role, char *user, char *district_id) {
    create_district(district_id);

    char reports_path[PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    int fd = open(reports_path, O_RDWR | O_CREAT, 0664);
    if (fd < 0) {
        perror("open reports_path");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    int noRecords = file_size / sizeof(REPORT_T);

    REPORT_T report;
    memset(&report, 0, sizeof(REPORT_T));

    report.id = noRecords + 1;
    strncpy(report.name, user, NAME_LEN - 1);
    report.timestamp = time(NULL);

    printf("X: ");
    scanf("%lf", &report.gps.longitude);

    printf("Y: ");
    scanf("%lf", &report.gps.latitude);

    printf("Issue category (road/lighting/flooding/other): ");
    scanf("%49s", report.issue);

    printf("Severity level (1/2/3): ");
    scanf("%d", &report.severity);

    printf("Description: ");
    getchar();
    fgets(report.description, DESCRIPTION_LEN, stdin);
    report.description[strcspn(report.description, "\n")] = '\0';

    if (write(fd, &report, sizeof(REPORT_T)) != sizeof(REPORT_T)) {
        perror("write");
    }
    else {
        printf("Report %d added to the district: %s\n", report.id, district_id);
    }
    close(fd);
    log_action(district_id, role, user, "Added a report");

    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    unlink(link_name);
    symlink(reports_path, link_name);
}

void list(char *role, char *user, char *district_id) {
    check_symlink(district_id);

    char reports_path[PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);
    struct stat st;
    if (stat(reports_path, &st) < 0) {
        perror("stat");
        return;
    }

    char permisions[10];
    mode_to_string(st.st_mode, permisions);

    char time_buf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("File: %s\n", reports_path);
    printf("Permisions: %s\n", permisions);
    printf("Size: %lld bytes\n", (long long) st.st_size);
    printf("Last modified: %s\n\n", time_buf);

    int fd = open(reports_path, O_RDONLY);
    if (fd < 0) {
        perror("open reports.dat in list");
    }

    REPORT_T report;
    int count = 0;
    printf("%-5s %-20s %-15s %-8s %s\n", "ID", "Inspector's name", "Issue category", "Severity", "Timestamp" );
    printf("\n");
    while (read(fd, &report, sizeof(REPORT_T)) == sizeof(REPORT_T)) {
        char time_buff[32];
        struct tm *t = localtime(&report.timestamp);
        strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", t);
        printf("%-5d %-20s %-15s %-8d %s\n", report.id, report.name, report.issue, report.severity, time_buff);
        count++;
    }

    if (count == 0) printf("No reports found.");
    close(fd);
    log_action(district_id, role, user, "List reports");
}

void view (char *role, char *user, char *district_id, int report_id) {
    char reports_path[PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    int fd = open(reports_path, O_RDONLY);
    if (fd < 0) {
        perror("open reports.dat in view");
    }

    off_t offset = (off_t)(report_id - 1) * sizeof(REPORT_T);
    if (lseek(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "Report %d not found.\n", report_id);
        close(fd);
        return;
    }

    REPORT_T report;
    if (read(fd, &report, sizeof(REPORT_T)) != sizeof(REPORT_T)) {
        fprintf(stderr, "Report %d not found or file corrupted.\n", report_id);
        close(fd);
        return;
    }

    if (report_id != report.id) {
        fprintf(stderr, "Report %d not identified correctly.\n", report_id);
        close(fd);
        return;
    }

    char time_buff[64];
    struct tm *t = localtime(&report.timestamp);
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", t);

    printf("=-=-= Report %d =-=-=\n", report.id);
    printf("Inspector's name: %s\n", report.name);
    printf("GPS:    X: %.6f   Y: %.6f\n", report.gps.longitude, report.gps.latitude);
    printf("Issue category: %s\n", report.issue);
    printf("Severity level: %d\n", report.severity);
    printf("Timestamp: %s\n", time_buff);
    printf("Description: %s\n", report.description);

    close(fd);
    log_action(district_id, role, user, "Viewed report");
}

void remove_report(char *role, char *user, char *district_id, int report_id) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Permisson denied: only managers can remove reports.\n");
        return;
    }

    char reports_path[PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);
    struct stat st;
    if (stat(reports_path, &st) == 0) {
        if (!(st.st_mode & S_IWUSR)) {
            fprintf(stderr, "Permission denied: reports.dat owner-write bit not set.\n");
            return;
        }
    }

    int fd = open(reports_path, O_RDWR);
    if (fd < 0) {
        perror("open reports.dat in remove_report");
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    int num_records = file_size / sizeof(REPORT_T);

    int target_index = -1;
    REPORT_T report;

    for (int i = 0; i < num_records; i++) {
        lseek(fd, (off_t)i * sizeof(REPORT_T), SEEK_SET);
        read(fd, &report, sizeof(REPORT_T));
        if (report.id == report_id) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        fprintf(stderr, "Report %d not found.\n", report_id);
        close(fd);
        return;
    }

    for (int i = target_index + 1; i < num_records; i++) {
        lseek(fd, (off_t)i * sizeof(REPORT_T), SEEK_SET);
        read(fd, &report, sizeof(REPORT_T));

        lseek(fd, (off_t)(i-1) * sizeof(REPORT_T), SEEK_SET);
        write(fd, &report, sizeof(REPORT_T));
    }

    off_t new_file_size = (off_t)(num_records-1) * sizeof(REPORT_T);
    ftruncate(fd, new_file_size);

    printf("Report %d deleted from district %s.\n", report_id, district_id);
    close(fd);
    log_action(district_id, role, user, "Removed report");
}

void update_threshold(char *role, char *user, char *district_id, int threshold) {

    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Permission denied: only managers can update threshold.\n");
        return;
    }

    char cfg_path[PATH_LEN];
    snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", district_id);

    struct stat st;
    if (stat(cfg_path, &st) < 0) {
        perror("stat district.cfg");
        return;
    }

    if ((st.st_mode & 0777) != 0640) {
        fprintf(stderr, "Warning: Expected 640, got %o. Refusing to write.\n", st.st_mode & 0777);
        return;
    }

    int fd = open(cfg_path, O_WRONLY| O_TRUNC, 0640);
    if (fd < 0) {
        perror("open district.cfg");
        return;
    }

    char buffer[64];
    int len = snprintf(buffer, sizeof(buffer),"severity_threshold = %d\n", threshold);
    write(fd, buffer, len);
    close(fd);

    printf("Threshold updated to %d in district %s.\n", threshold, district_id);
    log_action(district_id, role, user, "Updated threshold");
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    // Find the first and second colons in the string
    const char *colon1 = strchr(input, ':');
    if (!colon1) return 0; // Invalid format

    const char *colon2 = strchr(colon1 + 1, ':');
    if (!colon2) return 0; // Invalid format

    // Extract the 'field' part
    size_t field_len = colon1 - input;
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    // Extract the 'operator' part
    size_t op_len = colon2 - (colon1 + 1);
    strncpy(op, colon1 + 1, op_len);
    op[op_len] = '\0';

    // Extract the 'value' part (everything after the second colon)
    strcpy(value, colon2 + 1);

    return 1;
}

int match_condition(REPORT_T *report, const char *field, const char *op, const char *value) {
    // Handling integer comparisons for severity
    if (strcmp(field, "severity") == 0) {
        int sev_val = atoi(value);
        if (strcmp(op, "==") == 0) return report->severity == sev_val;
        if (strcmp(op, "!=") == 0) return report->severity != sev_val;
        if (strcmp(op, "<") == 0)  return report->severity < sev_val;
        if (strcmp(op, "<=") == 0) return report->severity <= sev_val;
        if (strcmp(op, ">") == 0)  return report->severity > sev_val;
        if (strcmp(op, ">=") == 0) return report->severity >= sev_val;
    }
    // Handling integer comparisons for timestamp
    else if (strcmp(field, "timestamp") == 0) {
        time_t ts_val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return report->timestamp == ts_val;
        if (strcmp(op, "!=") == 0) return report->timestamp != ts_val;
        if (strcmp(op, "<") == 0)  return report->timestamp < ts_val;
        if (strcmp(op, "<=") == 0) return report->timestamp <= ts_val;
        if (strcmp(op, ">") == 0)  return report->timestamp > ts_val;
        if (strcmp(op, ">=") == 0) return report->timestamp >= ts_val;
    }
    // Handling string comparisons for category
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(report->issue, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(report->issue, value) != 0;
    }
    // Handling string comparisons for inspector
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(report->name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(report->name, value) != 0;
    }

    fprintf(stderr, "Unknown field or operator: %s %s\n", field, op);
    return 0;
}

void filter(char *role, char *user, char *district_id, char **conds, int num_cond) {
    char reports_path[PATH_LEN];
    snprintf(reports_path, sizeof(reports_path), "%s/reports.dat", district_id);

    int fd = open(reports_path, O_RDONLY);
    if (fd < 0) {
        perror("open reports.dat in filter");
        return;
    }

    REPORT_T report;
    int found = 0;

    while (read(fd, &report, sizeof(REPORT_T)) == sizeof(REPORT_T)) {
        int match =1;

        for (int i = 0; i < num_cond; i++) {
            char field[32], op[4], value[64];
            if (!parse_condition(conds[i], field, op, value)) {
                fprintf(stderr, "Invalid condition format: %s\n", conds[i]);
                match = 0;
                break;
            }
            if (!match_condition(&report, field, op, value)) {
                match = 0;
                break;
            }
        }

        if (match) {
            char time_buffer[32];
            struct tm *tm_info = localtime(&report.timestamp);
            strftime(time_buffer,sizeof(time_buffer),"%Y-%m-%d %H:%M:%S",tm_info);
            printf("ID: %-5d Inspector: %-20s Issue Category: %-10s Severity: %d Time: %s\n", report.id, report.name, report.issue, report.severity, time_buffer);
            found ++;
        }
    }

    if (found == 0) {
        printf("No reports match the given conditions.\n");
    }
    else {
        printf("\n%d report(s) found.\n", found);
    }

    close(fd);
    log_action(district_id, role, user, "Filtered by condition(s)");
}

int main(int argc, char *argv[]) {
    char *role = NULL, * user = NULL, *command = NULL;
    char *firstArg = NULL;
    char *secondArg = NULL;
    char **filterConds = NULL;
    int filterCount = 0;

    //Storing each argument in their varible
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) {
            role = argv[++i];
        }
        else if (strcmp(argv[i], "--user") == 0) {
            user = argv[++i];
        }
        else if (strcmp(argv[i], "--add") == 0) {
            command = "add";
            firstArg = argv[++i];
        }
        else if (strcmp(argv[i], "--list") == 0) {
            command = "list";
            firstArg = argv[++i];
        }
        else if (strcmp(argv[i], "--view") == 0) {
            command = "view";
            firstArg = argv[++i];
            secondArg = argv[++i];
        }
        else if (strcmp(argv[i], "--remove_report") == 0) {
            command = "remove_report";
            firstArg = argv[++i];
            secondArg = argv[++i];
        }
        else if (strcmp(argv[i], "--update_threshold") == 0) {
            command = "update_threshold";
            firstArg = argv[++i];
            secondArg = argv[++i];
        }
        else if (strcmp(argv[i], "--filter") == 0) {
            command = "filter";
            firstArg = argv[++i];
            filterConds = &argv[i + 1];
            filterCount = argc - i - 1;
            break;
        }
    }

    //Making sure that everything is set okay
    if (!role || !command) {
        fprintf(stderr, "Usage: city_manager --role <role> --user <user> <command> \n");
        return 1;
    }
     if(strcmp(command, "add") == 0) {
        add(role, user, firstArg);
     }
    else if(strcmp(command, "list") == 0) {
        list(role, user, firstArg);

    }
    else if(strcmp(command, "view") == 0) {
        view(role, user, firstArg, atoi(secondArg));
    }
    else if(strcmp(command, "remove_report") == 0) {
        remove_report(role, user, firstArg, atoi(secondArg));
    }
    else if(strcmp(command, "update_threshold") == 0) {
        update_threshold(role, user, firstArg, atoi(secondArg));
    }
    else if(strcmp(command, "filter") == 0) {
        filter(role, user, firstArg, filterConds, filterCount);
    }
    else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    return 0;
}
