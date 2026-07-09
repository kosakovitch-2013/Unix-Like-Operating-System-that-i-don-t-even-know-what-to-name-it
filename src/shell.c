#include <stdint.h>
#include <string.h>
#include <vfs.h>
#include <printk.h>

extern void puts(const char* str);
extern void putchar_at_cursor(char c);
extern void move_cursor_back(void);
extern char keyboard_read_char(void);

static char cwd[256] = "/";

void shell_run(void) {
    char buffer[256];
    int pos = 0;
    char c;
    
    while (1) {
        puts(cwd);
        puts("# ");
        
        pos = 0;
        while (1) {
            c = keyboard_read_char();
            
            if (c == '\n') {
                buffer[pos] = '\0';
                puts("\n");
                
                if (strcmp(buffer, "exit") == 0) {
                    puts("Goodbye!\n");
                    return;
                } else if (strcmp(buffer, "help") == 0) {
                    puts("exit\n");
                    puts("echo\n");
                    puts("ver\n");
                    puts("ls\n");
                    puts("cat <file>\n");
                    puts("cd <dir>\n");
                    puts("cd ..\n");
                    puts("mkdir <dir>\n");
                } else if (strcmp(buffer, "ver") == 0) {
                    puts("OS version: 0.1\n");
                    puts("Shell version: 0.1\n");
                } else if (strcmp(buffer, "ls") == 0) {
                    char buf[4096];
                    if (vfs_list_dir(cwd, buf, sizeof(buf)) == 0) {
                        puts(buf);
                    } else {
                        puts("Failed to list directory\n");
                    }
                } else if (strcmp(buffer, "cd ..") == 0) {
                    int len = strlen(cwd);
                    if (len > 1) {
                        while (len > 1 && cwd[len-1] != '/') len--;
                        cwd[len > 1 ? len-1 : 1] = '\0';
                        if (cwd[0] == '\0') {
                            cwd[0] = '/';
                            cwd[1] = '\0';
                        }
                    }
                } else if (strncmp(buffer, "mkdir ", 6) == 0) {
                    char* dir = buffer + 6;
                    while (*dir == ' ' || *dir == '\t') dir++;
                    if (*dir) {
                        char path[256];
                        if (*dir == '/') {
                            int i = 0;
                            while (dir[i] && i < 255) { path[i] = dir[i]; i++; }
                            path[i] = '\0';
                        } else {
                            int len = strlen(cwd);
                            for (int i = 0; i < len; i++) path[i] = cwd[i];
                            if (cwd[len-1] != '/') path[len++] = '/';
                            int i = 0;
                            while (dir[i] && len+i < 255) { path[len+i] = dir[i]; i++; }
                            path[len+i] = '\0';
                        }
                        if (vfs_mkdir(path) == 0) {
                            puts("Created directory\n");
                        } else {
                            puts("Failed to create directory\n");
                        }
                    }
                } else if (strncmp(buffer, "cd ", 3) == 0) {
                    char* dir = buffer + 3;
                    while (*dir == ' ' || *dir == '\t') dir++;
                    if (*dir == '/') {
                        int i = 0;
                        while (dir[i] && i < 255) { cwd[i] = dir[i]; i++; }
                        cwd[i] = '\0';
                    } else if (strcmp(dir, "..") == 0) {
                        int len = strlen(cwd);
                        if (len > 1) {
                            while (len > 1 && cwd[len-1] != '/') len--;
                            cwd[len > 1 ? len-1 : 1] = '\0';
                            if (cwd[0] == '\0') { cwd[0] = '/'; cwd[1] = '\0'; }
                        }
                    } else {
                        if (strcmp(cwd, "/") != 0) {
                            int len = strlen(cwd);
                            cwd[len] = '/';
                            int i = 0;
                            while (dir[i] && len+1+i < 255) { cwd[len+1+i] = dir[i]; i++; }
                            cwd[len+1+i] = '\0';
                        } else {
                            int i = 0;
                            cwd[0] = '/';
                            while (dir[i] && i+1 < 255) { cwd[i+1] = dir[i]; i++; }
                            cwd[i+1] = '\0';
                        }
                    }
                } else if (strncmp(buffer, "echo", 4) == 0) {
                    char *p = buffer + 4;
                    while (*p == ' ' || *p == '\t') ++p;
                    puts(*p ? p : "");
                    puts("\n");
                } else if (strncmp(buffer, "cat ", 4) == 0) {
                    char *p = buffer + 4;
                    while (*p == ' ' || *p == '\t') p++;
                    if (*p) {
                        char path[256];
                        if (*p == '/') {
                            int i = 0;
                            while (p[i] && i < 255) { path[i] = p[i]; i++; }
                            path[i] = '\0';
                        } else {
                            int len = strlen(cwd);
                            for (int i = 0; i < len; i++) path[i] = cwd[i];
                            if (cwd[len-1] != '/') path[len++] = '/';
                            int i = 0;
                            while (p[i] && len+i < 255) { path[len+i] = p[i]; i++; }
                            path[len+i] = '\0';
                        }
                        int fd = vfs_open(path, 0);
                        if (fd >= 0) {
                            char fbuf[512];
                            size_t n = vfs_read(fd, fbuf, sizeof(fbuf)-1);
                            if (n > 0) { fbuf[n] = '\0'; puts(fbuf); puts("\n"); }
                            vfs_close(fd);
                        } else {
                            puts("File not found\n");
                        }
                    }
                } else if (strcmp(buffer, "cat") == 0) {
                    puts("Usage: cat <filename>\n");
                }
                break;
            }
            else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    move_cursor_back();
                    putchar_at_cursor(' ');
                    move_cursor_back();
                }
            }
            else if (c >= 32 && c <= 126) {
                if (pos < 255) {
                    buffer[pos++] = c;
                    putchar_at_cursor(c);
                }
            }
        }
    }
}
