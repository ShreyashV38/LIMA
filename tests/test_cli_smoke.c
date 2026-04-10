/*
 * test_cli_smoke.c
 * Automates the manual verification sequence from the implementation plan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_FILE "smoke_in.txt"
#define OUTPUT_FILE "smoke_out.txt"
#define SAVE_FILE "test.lima"

int main(void) {
    FILE *fin = fopen(INPUT_FILE, "w");
    if (!fin) {
        fprintf(stderr, "Failed to create input file\n");
        return 1;
    }

    /* Write the test sequence */
    fprintf(fin, "mkdir docs\n");
    fprintf(fin, "cd docs\n");
    fprintf(fin, "touch hello.txt\n");
    /* Note: 'edit' is skipped as it requires interactive terminal / raw mode */
    fprintf(fin, "ls\n");
    fprintf(fin, "cd ..\n");
    fprintf(fin, "ls\n");
    fprintf(fin, "pwd\n");
    fprintf(fin, "back\n");
    fprintf(fin, "pwd\n");
    fprintf(fin, "undo\n");
    fprintf(fin, "save %s\n", SAVE_FILE);
    fprintf(fin, "load %s\n", SAVE_FILE);
    fprintf(fin, "exit\n");
    fclose(fin);

    /* Execute the CLI shell */
#ifdef _WIN32
    int ret = system("build\\bin\\lima.exe < " INPUT_FILE " > " OUTPUT_FILE " 2>&1");
#else
    int ret = system("./build/bin/lima.exe < " INPUT_FILE " > " OUTPUT_FILE " 2>&1");
#endif

    if (ret != 0) {
        fprintf(stderr, "lima.exe returned non-zero exit code: %d\n", ret);
        return 1;
    }

    /* Read and verify the output */
    FILE *fout = fopen(OUTPUT_FILE, "r");
    if (!fout) {
        fprintf(stderr, "Failed to open output file\n");
        return 1;
    }

    char content[8192] = {0};
    fread(content, 1, sizeof(content) - 1, fout);
    fclose(fout);

    /* Run assertions */
    int failed = 0;
    if (!strstr(content, "[FILE] hello.txt")) {
        fprintf(stderr, "FAIL: Missing '[FILE] hello.txt' in output\n");
        failed = 1;
    }
    if (!strstr(content, "[DIR]  docs")) {
        fprintf(stderr, "FAIL: Missing '[DIR]  docs' in output\n");
        failed = 1;
    }
    if (!strstr(content, "Undo successful.")) {
        fprintf(stderr, "FAIL: Missing 'Undo successful.' in output\n");
        failed = 1;
    }
    if (!strstr(content, "VFS saved to")) {
        fprintf(stderr, "FAIL: Missing 'VFS saved to' in output\n");
        failed = 1;
    }
    if (!strstr(content, "VFS loaded from")) {
        fprintf(stderr, "FAIL: Missing 'VFS loaded from' in output\n");
        failed = 1;
    }

    /* Clean up */
    remove(INPUT_FILE);
    remove(OUTPUT_FILE);
    remove(SAVE_FILE);

    if (failed) {
        fprintf(stderr, "Output was:\n%s\n", content);
        return 1;
    }

    printf("CLI Smoke Test PASSED.\n");
    return 0;
}
