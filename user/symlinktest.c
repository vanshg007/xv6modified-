#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define O_RDONLY    0x000
#define O_WRONLY    0x001
#define O_RDWR      0x002
#define O_CREATE    0x200
#define O_TRUNC     0x400
#define O_NOFOLLOW  0x1000

void print_symlink_resolution(const char *path, int fd, int nofollow) {
    char buf[512];
    int n;

    if(fd < 0){
        printf("Failed to open: %s\n", path);
        return;
    }

    n = read(fd, buf, sizeof(buf));
    buf[n] = 0;

    if(nofollow)
        printf("Opened symlink raw (O_NOFOLLOW): %s\n", buf);
    else
        printf("Opened symlink resolved: %s -> %s\n", path, buf);

    close(fd);
}

void test_file_symlink() {
    int fd;

    printf("=== Test: File Symlink ===\n");

    printf("Creating file: /file1\n");
    fd = open("/file1", O_CREATE | O_RDWR);
    write(fd, "Hello Symlink!\n", 16);
    close(fd);

    printf("Creating symlink: /link1 -> /file1\n");
    if(symlink("/file1", "/link1") < 0){
        printf("symlink failed\n");
        exit(1);
    }

    printf("Opening /link1 (should resolve to /file1)\n");
    fd = open("/link1", O_RDONLY);
    print_symlink_resolution("/link1", fd, 0);

    printf("Opening /link1 with O_NOFOLLOW\n");
    fd = open("/link1", O_RDONLY | O_NOFOLLOW);
    print_symlink_resolution("/link1", fd, 1);
}

void test_unlink_symlink() {
    int fd;
    char buf[32];

    printf("\n=== Test: Unlink Symlink ===\n");

    printf("Creating file: /file_unlink\n");
    fd = open("/file_unlink", O_CREATE | O_RDWR);
    write(fd, "PreserveMe\n", 11);
    close(fd);

    printf("Creating symlink: /link_unlink -> /file_unlink\n");
    if(symlink("/file_unlink", "/link_unlink") < 0){
        printf("symlink failed\n");
        exit(1);
    }

    printf("Unlinking symlink only: /link_unlink\n");
    if(unlink("/link_unlink") < 0){
        printf("unlink failed\n");
        exit(1);
    }

    printf("Verifying target file still exists (/file_unlink)\n");
    fd = open("/file_unlink", O_RDONLY);
    if(fd < 0){
        printf("Error: target file was deleted! \n");
    } else {
        int n = read(fd, buf, sizeof(buf));
        buf[n] = 0;
        printf("Target file contents: %s \n", buf);
        close(fd);
    }

    // Clean up
    unlink("/file_unlink");
}

void test_dir_symlink() {
    int fd;

    printf("\n=== Test: Directory Symlink ===\n");

    printf("Creating directory: /dir1\n");
    if(mkdir("/dir1") < 0){
        printf("mkdir failed\n");
        exit(1);
    }

    printf("Creating symlink: /linkdir -> /dir1\n");
    if(symlink("/dir1", "/linkdir") < 0){
        printf("symlink to dir failed\n");
        exit(1);
    }

    printf("Opening /linkdir (should resolve to /dir1)\n");
    fd = open("/linkdir", O_RDONLY);
    if(fd < 0){
        printf("Failed to open symlink to directory\n");
    } else {
        printf("Successfully opened symlink to directory \n");
        close(fd);
    }

    // Clean up
    unlink("/linkdir");
    unlink("/dir1");
}

void test_recursive_symlink() {
    int fd;

    printf("\n=== Test: Recursive Symlink ===\n");

    printf("Creating recursive symlinks: /loop1 -> /loop2, /loop2 -> /loop1\n");
    symlink("/loop2", "/loop1");
    symlink("/loop1", "/loop2");

    printf("Opening /loop1 to test loop detection\n");
    fd = open("/loop1", O_RDONLY);
    if(fd < 0){
        printf("Detected symlink loop correctly \n");
    } else {
        printf("Symlink loop not detected! \n");
        close(fd);
    }

    // Clean up
    unlink("/loop1");
    unlink("/loop2");
}

int main(void) {
    printf("\n========== xv6 Symlink Test ==========\n\n");

    test_file_symlink();
    test_unlink_symlink();
    test_dir_symlink();
    test_recursive_symlink();

    printf("\nAll symlink tests completed!\n");
    exit(0);
}
