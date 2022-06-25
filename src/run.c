// run.c : run any program on the computer using EverythingSearch
//
// Copyright © 2014 Dror Harari
//
// (MIT license)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the “Software”),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <limits.h>
#include <process.h>

#define  EVERYTHINGUSERAPI
#include "../include/Everything.h"

struct Favorite
{
    char *name;
    char *executable;
    struct Favorite *next;
};
static struct Favorite *s_Favorites;

static void help()
{
    fprintf(stderr, "Usage: run [options] <program> <...program parameters...>\n");
    fprintf(stderr, "\tprogram: (partial) name of program without .exe\n");
    fprintf(stderr, "\t-#: Run the #'th program as listed by -l\n");
    fprintf(stderr, "\t-d: Remove the given program from the favorites list\n");
    fprintf(stderr, "\t-f: List favorites\n");
    fprintf(stderr, "\t-k: Pause after run\n");
    fprintf(stderr, "\t-l: Just list matching names\n");
    fprintf(stderr, "\t-p: Print matching program path to the standard output (without running it)\n");
    fprintf(stderr, "\t-s: With -#, save the #'th program as listed by -l as the favorite for the given program\n");
    fprintf(stderr, "\t-w: Use whole-word search\n");
}

static void print_error()
{
    int err = Everything_GetLastError();
    char err_buff[256] = { 0 }, *err_str = err_buff;

    switch (err) {
        case EVERYTHING_ERROR_CREATETHREAD:     err_str = "Everything error: CREATETHREAD"; break;
        case EVERYTHING_ERROR_REGISTERCLASSEX:  err_str = "Everything error: REGISTERCLASSEX"; break;
        case EVERYTHING_ERROR_CREATEWINDOW:     err_str = "Everything error: CREATEWINDOW"; break;
        case EVERYTHING_ERROR_IPC:              err_str = "Everything error: Is EverythingSearch running? (IPC)"; break;
        case EVERYTHING_ERROR_MEMORY:           err_str = "Everything error: MEMORY	"; break;
        case EVERYTHING_ERROR_INVALIDCALL:      err_str = "Everything error: INVALIDCALL"; break;
        default:                                sprintf(err_buff, "Everything error: Unknown error code %d", err);
    }

    fprintf(stderr, "%s", err_str);
}

static void reset_search(char *pattern)
{
    Everything_Reset();
    Everything_SetMax(200);
    Everything_SetSearch(pattern);
}

// Turn "c:\location\prog.exe" into "path:c:\location prog.exe" for Everything
static void set_pattern_if_path(char *pattern, int pattern_size, char *input)
{
    char *path = strrchr(input, '\\');

    if (path) {
        sprintf_s(pattern, pattern_size, "path:%s", input);
        pattern[strlen("path:") + path - input] = ' '; // Last \ turns into space (for AND)
    }
    else {
        strcpy_s(pattern, pattern_size, input);
        return;
    }
}

static int ends_with(const char *str, const char *suffix)
{
    size_t suffix_len = strlen(suffix);
    size_t str_len = strlen(str);

    return str_len >= suffix_len && strcmp(suffix, str + str_len - suffix_len) == 0;
}

static int starts_with(const char *str, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);

    return str_len >= prefix_len && strncmp(prefix, str, prefix_len) == 0;
}

static int skipped_file(const char *file_name, const char *path)
{
    return
        ends_with(file_name, ".pf")             ||
        ends_with(file_name, ".mui")            ||
        ends_with(file_name, ".res")            ||
        ends_with(file_name, ".manifest")       ||
        ends_with(file_name, ".config")         ||
        strstr(path, "\\obj\\")                 ||
        strstr(path, "Windows\\servicing\\")    ||
        strstr(path, "Windows\\WinSxS\\")       ||
        strstr(path, "\\$Recycle.Bin\\")        ||
        ends_with(path, "\\Prefetch");
}

static char *get_favorites_path()
{
    static char favorites_filename[] = "run.fav";
    static char module_file_buff[MAX_PATH + sizeof(favorites_filename)] = { 0 };
    static char *favorites_path = NULL;
    DWORD hr;

    if (!favorites_path) {
        hr = GetModuleFileName(NULL, module_file_buff, sizeof(module_file_buff) - sizeof(favorites_filename));
        if (GetLastError() == ERROR_SUCCESS) {
            char *last_backslash = strrchr(module_file_buff, '\\');
            if (last_backslash) {
                strcpy(last_backslash + 1, favorites_filename);
                favorites_path = module_file_buff;
            }
        }
    }

    return favorites_path;
}

static void load_favorites()
{
    FILE* file;
    char *favorites_filepath = get_favorites_path();

    if (!favorites_filepath || _access(favorites_filepath, 0) != 0) {
        return;
    }

    file = fopen(favorites_filepath, "r");
    if (!file) {
        fprintf(stderr, "Could not open favorites file '%s' for read - %s", favorites_filepath, strerror(errno));
        return;
    }
    else {
        char line_buff[2048 + 1];
        char *line;
        struct Favorite *last_favorite = NULL;

        while ((line = fgets(line_buff, sizeof(line_buff) - 1, file))) {
            char *space;
            char *executable;
            struct Favorite *favorite;

            space = strchr(line, '\n');
            if (space) {
                *space = '\0';
            }

            space = strchr(line, ' ');
            executable = space ? space + 1 : "";
            if (space) {
                *space = '\0';
            }

            favorite = (struct Favorite *)malloc(sizeof(struct Favorite));
            favorite->name = (char *)malloc(strlen(line) + 1);
            strcpy(favorite->name, line);
            favorite->executable = (char *)malloc(strlen(executable) + 1);
            strcpy(favorite->executable, executable);
            favorite->next = NULL;

            if (!s_Favorites) {
                s_Favorites = last_favorite = favorite;
            }
            else {
                last_favorite->next = favorite;
                last_favorite = favorite;
            }
        }

        fclose(file);
    }
}

static char *lookup_favorite(char *name)
{
    struct Favorite *favorites = s_Favorites;

    while (favorites) {
        if (_stricmp(name, favorites->name) == 0) {
            return favorites->executable;
        }

        favorites = favorites->next;
    }

    return NULL;
}

static char *list_favorites()
{
    struct Favorite *favorites = s_Favorites;

    fprintf(stderr, "Run's favorites:\n");
    while (favorites) {
        fprintf(stderr, "%s ==> %s\n", favorites->name, favorites->executable);
        favorites = favorites->next;
    }

    return NULL;
}

static char *save_favorite(char *name, char *executable)
{
    struct Favorite *favorites = s_Favorites;
    int replaced = FALSE;
    FILE* file;
    char *favorites_filepath = get_favorites_path();

    if (!favorites_filepath) {
        fprintf(stderr, "Could not save favorite (cannot determine favorites file location)\n");
        return name;
    }

    // Named is always saved without the wildcards (%,*)
    {
        char *p_name = name;
        char *p_new_name, * new_name;

        p_new_name = new_name = (char *)malloc(strlen(name) + 1);
        while (*p_name) {
            switch (*p_name) {
            case '%':
            case '*':
                p_name++;
                continue;
            default:
                *p_new_name++ = *p_name++;
            }
        }
        *p_new_name++ = *p_name++;
        name = new_name;
    }

    while (favorites) {
        if (_stricmp(name, favorites->name) == 0) {
            favorites->executable = (char *)malloc(strlen(executable) + 1);
            strcpy(favorites->executable, executable);
            replaced = TRUE;
            break;
        }

        favorites = favorites->next;
    }

    if (!replaced) {
        struct Favorite *new_favorite = (struct Favorite *)malloc(sizeof(struct Favorite));
        new_favorite->name = (char *)malloc(strlen(name) + 1);
        strcpy(new_favorite->name, name);
        new_favorite->executable = (char *)malloc(strlen(executable) + 1);
        strcpy(new_favorite->executable, executable);
        new_favorite->next = s_Favorites;
        s_Favorites = new_favorite;
    }

    file = fopen(favorites_filepath, "w");
    if (!file) {
        fprintf(stderr, "Could not open favorites file '%s' for write - %s", favorites_filepath, strerror(errno));
        return name;
    }

    favorites = s_Favorites;
    while (favorites) {
        fprintf(file, "%s %s\n", favorites->name, favorites->executable);
        favorites = favorites->next;
    }

    fclose(file);

    return name;
}

static void delete_favorite(char *name)
{
    struct Favorite *favorites = s_Favorites;
    FILE* file;
    char *favorites_filepath = get_favorites_path();

    if (!favorites_filepath) {
        fprintf(stderr, "Could not delete favorite (cannot determine favorites file location)\n");
        return;
    }

    if (!lookup_favorite(name)) {
        fprintf(stderr, "Could find favorite program '%s' to delete\n", name);
        return;
    }

    file = fopen(favorites_filepath, "w");
    if (!file) {
        fprintf(stderr, "Could not open favorites file '%s' for write - %s", favorites_filepath, strerror(errno));
        return;
    }

    favorites = s_Favorites;
    while (favorites) {
        if (_stricmp(name, favorites->name) == 0) {
            fprintf(stderr, "Deleted favorite program '%s' (%s)\n", name, favorites->executable);
        }
        else {
            fprintf(file, "%s %s\n", favorites->name, favorites->executable);
        }

        favorites = favorites->next;
    }

    fclose(file);
}

int main(int argc, char *argv[], char *envv[])
{
    int i;
    intptr_t status;
    char exe_pattern[4096];
    char *favorite_exe;
    const char *exe_name;
    const char *exe_path;
    int is_list = FALSE;
    int is_whole_word = FALSE;
    int is_pause = FALSE;
    int is_path_only = FALSE;
    int is_save = FALSE;
    int is_delete = FALSE;
    int chosen_option = 0;
    int prm_no = 1;
    int n_results;
    int ok;

    if (argc < 2) {
        help();
        exit(1);
    }

    load_favorites();

    while (prm_no < argc && argv[prm_no][0] == '-') {
        switch (argv[prm_no][1]) {
        case 'l':
            is_list = TRUE;
            break;

        case 'w':
            is_whole_word = TRUE;
            break;

        case 'k':
            is_pause = TRUE;
            break;

        case 'p':
            is_path_only = TRUE;
            break;

        case 's':
            is_save = TRUE;
            break;

        case 'd':
            is_delete = TRUE;
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

        case 'f':
            list_favorites();
            exit(0);

        default:
            fprintf(stderr, "Unrecognized option '%s'\n\n", argv[prm_no]);
            help();
            exit(2);
        }

        prm_no++;
    }

    if (!argv[prm_no] || !*argv[prm_no]) {
        fprintf(stderr, "Missing program to %s\n", is_delete ? "delete" : "run");
        help();
        exit(2);
    }
    else if (is_delete) {
        delete_favorite(argv[prm_no]);
        exit(0);
    }
    else if (is_save && chosen_option == 0) {
        fprintf(stderr, "Cannot save a favorite program without selecting one\n");
        help();
        exit(2);
    }

    favorite_exe = lookup_favorite(argv[prm_no]);
    if (favorite_exe && !(is_list || chosen_option != 0)) {
        strcpy_s(exe_pattern, sizeof(exe_pattern), favorite_exe);
    }
    else {
        if (chosen_option == 0) {
            chosen_option = 1;
        }

        // First try: just with .exe
        set_pattern_if_path(exe_pattern, sizeof(exe_pattern) - sizeof(".exe"), argv[prm_no]);
        if (!ends_with(exe_pattern, ".exe"))
            strcat(exe_pattern, ".exe");

        reset_search(exe_pattern);
        Everything_SetMatchWholeWord(TRUE);

        ok = Everything_Query(TRUE);

        // No results? Relax
        if (ok && (Everything_GetNumResults() == 0 || !starts_with(Everything_GetResultFileName(0), argv[prm_no]))) {
            set_pattern_if_path(exe_pattern, sizeof(exe_pattern) - sizeof("*.exe"), argv[prm_no]);
            if (!ends_with(exe_pattern, ".exe"))
                strcat(exe_pattern, "*.exe");
            reset_search(exe_pattern);
            Everything_SetMatchWholeWord(TRUE);

            ok = Everything_Query(TRUE);

            if (ok && Everything_GetNumResults() == 0 && !is_whole_word) {
                set_pattern_if_path(exe_pattern, sizeof(exe_pattern) - sizeof("*.exe"), argv[prm_no]);
                if (!ends_with(exe_pattern, ".exe"))
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
                if (skipped_file(Everything_GetResultFileName(i), Everything_GetResultPath(i))) {
                    continue;
                }

                cur_option++;

                exe_name = Everything_GetResultFileName(i);
                exe_path = Everything_GetResultPath(i);
                sprintf(exe_pattern, "%s\\%s", exe_path, exe_name);

                if (is_list) {
                    int is_default = favorite_exe && _stricmp(favorite_exe, exe_pattern) == 0;

                    printf("%d) %s%s [%s]%s\n", cur_option,
                        cur_option == chosen_option ? "CHOSEN: " : "",
                        Everything_GetResultFileName(i), Everything_GetResultPath(i),
                        is_default ? " (default)" : "");
                }
                else {
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
        if (exe_name && exe_pattern) {
            sprintf(exe_pattern, "%s\\%s", exe_path, exe_name);
        }
        else {
            *exe_pattern = '\0';
        }
    }

    if (is_save) {
        if (*exe_pattern) {
            char *new_name = save_favorite(argv[prm_no], exe_pattern);
            fprintf(stderr, "Saved favorite '%s' as: %s\n", new_name, exe_pattern);
            status = 0;
        }
        else {
            fprintf(stderr, "Favorite number %d is not valid\n", chosen_option);
            status = 1;
        }
    }
    else if (is_path_only) {
        fprintf(stdout, "%s", exe_pattern);
    }
    else {
        fprintf(stderr, "Running: %s:\n", exe_pattern);

        // Make sure empty parameters do not vanish by replacing them with explicit quotes
        for (i = prm_no; argv[i]; i++) {
            if (argv[i][0] == '\0') {
                argv[i] = "\"\"";
            }
        }

        status = _spawnvpe(_P_WAIT, exe_pattern, argv + prm_no, envv);
    }

    if (is_pause) {
        fprintf(stderr, "\nHit RETURN to continue...");
        getchar();
    }

    return status;
}
