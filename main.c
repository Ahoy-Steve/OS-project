#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int main(int argc, char *argv[]) {
    char *role = NULL, * user = NULL, *command = NULL;
    char *firstArg = NULL;
    char *secondArg = NULL;

    //Storing each argument in their varible
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) {
            role = argv[i++];
        }
        else if (strcmp(argv[i], "--user") == 0) {
            user = argv[i++];
        }
        else if (strcmp(argv[i], "--add") == 0) {
            command = "add";
            firstArg = argv[i++];
        }
        else if (strcmp(argv[i], "--list") == 0) {
            command = "list";
            firstArg = argv[i++];
        }
        else if (strcmp(argv[i], "--view") == 0) {
            command = "view";
            firstArg = argv[i++];
        }
        else if (strcmp(argv[i], "--remove_report") == 0) {
            command = "remove_report";
            firstArg = argv[i++];
            secondArg = argv[i++];
        }
        else if (strcmp(argv[i], "--update_threshold") == 0) {
            command = "update_threshold";
            firstArg = argv[i++];
            secondArg = argv[i++];
        }
        else if (strcmp(argv[i], "--filter") == 0) {
            command = "filter";
            firstArg = argv[i++];
        }
    }

    //Making sure that everything is set okay
    if (!role || !command) {
        fprintf(stderr, "Usage: city_manager --role <role> --user <user> <command> \n");
        exit(1);
    }
     if(strcmp(command, "add") == 0) {
        //add
     }
    else if(strcmp(command, "list") == 0) {
        //list

    }
    else if(strcmp(command, "view") == 0) {
        //view
    }
    else if(strcmp(command, "remove_report") == 0) {
        //remove_report
    }
    else if(strcmp(command, "update_threshold") == 0) {
        //update_thershold
    }
    else if(strcmp(command, "filter") == 0) {
        //filter
    }
    else {
        fprintf(stderr, "Unknown command %s\n", command);
        exit(1);
    }
    return 0;
}
