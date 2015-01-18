// Run.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <process.h>

#define  EVERYTHINGUSERAPI
#include "Everything.h"

static void help()
{
    fprintf(stderr, "Usage: run [options] <program> <...program parameters...>\n");
    fprintf(stderr, "\tprogram: (partial) name of program without .exe\n");
    fprintf(stderr, "\t-w: Use whole-word search\n");
    fprintf(stderr, "\t-l: Just list matching names\n");
    fprintf(stderr, "\t-p: Pause after run\n");
}

static void print_error()
{
    int err =  Everything_GetLastError();
    char *str = "Unknown error";

    switch (err) {
        case EVERYTHING_ERROR_CREATETHREAD:     str = "EVERYTHING_ERROR_CREATETHREAD"; break;
        case EVERYTHING_ERROR_REGISTERCLASSEX:  str = "EVERYTHING_ERROR_REGISTERCLASSEX"; break;
        case EVERYTHING_ERROR_CREATEWINDOW:     str = "EVERYTHING_ERROR_CREATEWINDOW"; break;
        case EVERYTHING_ERROR_IPC:              str = "EVERYTHING_ERROR_IPC"; break;
        case EVERYTHING_ERROR_MEMORY:           str = "EVERYTHING_ERROR_MEMORY	"; break;
        case EVERYTHING_ERROR_INVALIDCALL:      str = "EVERYTHING_ERROR_INVALIDCALL"; break;
    }

    fprintf(stderr, "%s", str);
}

static void reset_search(char *pattern)
{
    Everything_Reset();
    Everything_SetMax(200);
	Everything_SetSearch(pattern);
}

static void set_if_path(char *pattern, char *input)
{
    char *path = strrchr(input, '\\');

    if (path) {
        strcpy(pattern, "path:");
        pattern += strlen(pattern);
        strcpy(pattern, input);
        pattern[path-input] = ' '; // Last \ turns into space (for AND)
    }
    else {
        strcpy(pattern, input);
    }
}

static int ends_with(const char *str, const char *suffix)
{
    int suffix_len = strlen(suffix);
    int str_len = strlen(str);

    return str_len >= suffix_len && strcmp(suffix, str + str_len - suffix_len) == 0;
}

static int skipped_file(const char *file_name, const char *path)
{
    return
        ends_with(file_name, ".pf")     ||
        ends_with(file_name, ".mui")     ||
        ends_with(file_name, ".res")     ||
        ends_with(file_name, ".manifest")     ||
        ends_with(path, "\\Prefetch");        
}

int main(int argc, char *argv[], char *envv[])
{
	int i;
    int status;
    char *exe_pattern;
    const char *exe_name;
    const char *exe_path;
    int is_list = FALSE;
    int is_whole_word = FALSE;
    int is_pause = FALSE;
    int chosen_option = 1;
    int prm_no = 1;
    int n_results;
    int ok;

    if (argc < 2) {
        help();
        exit(1);
    }

    while (prm_no < argc && argv[prm_no][0] == '-') {
        switch (argv[prm_no][1]) {
            case 'l':
                is_list = TRUE;
                break;

            case 'w':
                is_whole_word = TRUE;
                break;

            case 'p':
                is_pause = TRUE;
                break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                chosen_option = atoi(&argv[prm_no][1]);
                break;

            default:
                fprintf(stderr, "Unrecognized option '%s'\n\n", argv[prm_no]);
                help();
                exit(2);
        }

        prm_no++;
    } 

    exe_pattern = (char *)malloc(strlen(argv[1])+1024);

    // First try: just with .exe
    set_if_path(exe_pattern, argv[prm_no]);
    strcat(exe_pattern, ".exe");
    reset_search(exe_pattern);
    Everything_SetMatchWholeWord(TRUE);

	ok = Everything_Query(TRUE);

    // No results? Relax
    if (ok && Everything_GetNumResults() == 0) {
        set_if_path(exe_pattern, argv[prm_no]);
        strcat(exe_pattern, "*.exe");
        reset_search(exe_pattern);
        Everything_SetMatchWholeWord(TRUE);

	    ok = Everything_Query(TRUE);

        if (ok && Everything_GetNumResults() == 0 && !is_whole_word) {
            set_if_path(exe_pattern, argv[prm_no]);
            strcat(exe_pattern, "*.exe");
            reset_search(exe_pattern);
            Everything_SetMatchWholeWord(FALSE);

	        ok = Everything_Query(TRUE);
        }
    }

    if (!ok) {
        print_error();
        exit(5);
    }

    n_results = Everything_GetNumResults();
    if (!n_results) {
        fprintf(stderr, "%s not found\n", exe_pattern);
        exit(3);
    }

    if (is_list || chosen_option) {
        int cur_option = 0;
	    for (i = 0; i < n_results; i++)
	    {
            if (skipped_file(Everything_GetResultFileName(i),Everything_GetResultPath(i))) {
                continue;
            }

            cur_option++;

            if (is_list) {
		        printf("%d) %s%s [%s]\n", cur_option, 
                    cur_option == chosen_option ? "CHOSEN: " : "", 
                    Everything_GetResultFileName(i),Everything_GetResultPath(i));
            } else {
                if (cur_option == chosen_option) {
                    chosen_option = i + 1;
                    break;
                }
            }
	    }

        if (is_list) {
            return 0;
        }
    }

    exe_name = Everything_GetResultFileName(chosen_option - 1);
    exe_path = Everything_GetResultPath(chosen_option - 1);
    sprintf(exe_pattern, "%s\\%s", exe_path, exe_name);

    fprintf(stderr, "Running: %s:\n", exe_pattern);
    status = _spawnvpe(_P_WAIT , exe_pattern, argv + prm_no, envv);

    if (is_pause) {
        fprintf(stderr, "\nHit RETURN to continue...");
        getchar();
    }

    return status;
}

