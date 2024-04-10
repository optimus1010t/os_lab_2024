#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <unistd.h>

void delete_path(const char *path){
    struct stat statbuf;
    if (stat(path, &statbuf) == -1){
        return;
    }   

    if(S_ISDIR(statbuf.st_mode)){
        DIR *dir = opendir(path);
        if (dir == NULL) {
            return;
        }

        struct dirent *entry;
        while((entry = readdir(dir))!=NULL){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);

            // recursively delete subdirectories and files
            delete_path(subpath);
        }

        closedir(dir);
        rmdir(path);
    }
    else if(S_ISREG(statbuf.st_mode)){
        unlink(path);
        printf("[-] %s\n", path);
    }
}

void sync_file(const char *src_path, const char *dst_path){
    struct stat src_stat, dst_stat;

    if (stat(src_path, &src_stat) == -1) {
        return;
    }
    if (stat(dst_path, &dst_stat) == -1){       // if dst_path does not exist, create it
        printf("[+] %s\n", dst_path);
    }
    else if(src_stat.st_mtime == dst_stat.st_mtime && src_stat.st_size == dst_stat.st_size){        // if src_path and dst_path are the same, return
        return;   
    }
    else{                                // if src_path and dst_path are different, overwrite dst_path
        printf("[o] %s\n", dst_path);
    }

    FILE *src_file = fopen(src_path, "r");
    if (src_file == NULL) {
        return;
    }

    FILE *dst_file = fopen(dst_path, "w");
    if (dst_file == NULL) {
        fclose(src_file);
        return;
    }

    char buffer[4096];
    size_t bytes_read;
    while((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0){
        fwrite(buffer, 1, bytes_read, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);   
}

void sync_directory(const char *src_path, const char *dst_path){
    struct stat src_stat, dst_stat;

    if (stat(src_path, &src_stat) == -1) {               // if src_path does not exist, delete dst_path
        delete_path(dst_path);
    }
    else if(stat(dst_path, &dst_stat) == -1) {           // if dst_path does not exist, create it
        mkdir(dst_path, src_stat.st_mode);
    }

    DIR *dir = opendir(src_path);
    if (dir == NULL) {
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir))!=NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_subpath[1024];
        char dst_subpath[1024];
        snprintf(src_subpath, sizeof(src_subpath), "%s/%s", src_path, entry->d_name);
        snprintf(dst_subpath, sizeof(dst_subpath), "%s/%s", dst_path, entry->d_name);
        struct stat subpath_stat;
        if (stat(src_subpath, &subpath_stat) == -1) {
            continue;
        }

        // recursively sync subdirectories and files
        if(S_ISDIR(subpath_stat.st_mode)){
            sync_directory(src_subpath, dst_subpath);
        }
        else if(S_ISREG(subpath_stat.st_mode)){
            sync_file(src_subpath, dst_subpath);
        }
    }

    // delete files and directories that are not in src_path
    DIR *dir2 = opendir(dst_path);
    rewinddir(dir);
    while((entry = readdir(dir2))!=NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_subpath[1024];
        char dst_subpath[1024];
        snprintf(src_subpath, sizeof(src_subpath), "%s/%s", src_path, entry->d_name);
        snprintf(dst_subpath, sizeof(dst_subpath), "%s/%s", dst_path, entry->d_name);
        struct stat subpath_stat;
        if (stat(src_subpath, &subpath_stat) == -1){
            delete_path(dst_subpath);
        }
    }

    closedir(dir);
    closedir(dir2);
}

void sync_permissions_and_timestamps(const char *src_path, const char *dst_path){
    struct stat src_stat,dst_stat;
    if(stat(src_path, &src_stat)==-1){
        return;
    }
    if(stat(dst_path, &dst_stat)==-1){
        return;
    }

    // check if permissions are different
    if(src_stat.st_mode != dst_stat.st_mode){
        printf("[p] %s\n", dst_path);
        chmod(dst_path, src_stat.st_mode);
    }

    // check if timestamps are different
    if(src_stat.st_mtime != dst_stat.st_mtime){
        printf("[t] %s\n", dst_path);
        struct utimbuf times;
        times.actime = src_stat.st_atime;
        times.modtime = src_stat.st_mtime;
        utime(dst_path, &times);
    }

    DIR *dir = opendir(src_path);
    if (dir == NULL) {
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir))!=NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_subpath[1024];
        char dst_subpath[1024];
        snprintf(src_subpath, sizeof(src_subpath), "%s/%s", src_path, entry->d_name);
        snprintf(dst_subpath, sizeof(dst_subpath), "%s/%s", dst_path, entry->d_name);

        struct stat subpath_stat;
        if (stat(src_subpath, &subpath_stat) == -1) {
            continue;
        }

        // recursively sync subdirectories and files
        if(S_ISDIR(subpath_stat.st_mode) || S_ISREG(subpath_stat.st_mode)){
            sync_permissions_and_timestamps(src_subpath, dst_subpath);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src dst\n", argv[0]);
        return 1;
    }

    sync_directory(argv[1], argv[2]);
    sync_permissions_and_timestamps(argv[1], argv[2]);

    return 0;
}