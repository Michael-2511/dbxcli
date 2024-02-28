#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <linux/limits.h>   // pentru PATH_MAX
#include <fcntl.h>
#include <unistd.h>

#define MAX_REVISIONS 100

typedef struct {
    char content[10000];
    time_t timestamp;       // Timestamp of the revision
} FileRevision;

typedef struct {
    char filename[50];
    FileRevision revisions[MAX_REVISIONS];
    int numRevisions;
} File;

File createFile(const char *filename) {
    File file;
    strncpy(file.filename, filename, sizeof(file.filename));
    file.numRevisions = 0;
    return file;
}

void addRevision(File *file, const char *newContent) {
    if (file->numRevisions < MAX_REVISIONS) {
        strncpy(file->revisions[file->numRevisions].content, newContent, sizeof(file->revisions[0].content));
        file->revisions[file->numRevisions].timestamp = time(NULL);
        file->numRevisions++;
    } else {
        // Handle maximum revisions reached
        printf("revs: Maximum number of revisions reached for file %s\n", file->filename);
        exit(1);
    }
}

void listRevisions(const File *file) {
    printf("Revisions for file %s:\n", file->filename);
    for (int i = 0; i < file->numRevisions; ++i) {
        printf("Revision %d: %s (Timestamp: %ld)\n", i + 1, file->revisions[i].content, file->revisions[i].timestamp);
    }
}

int fileModified(const char *filename, time_t *lastModified) {
    struct stat fileStat;
    if (stat(filename, &fileStat) == 0) {
        if (fileStat.st_mtime != *lastModified) {
            *lastModified = fileStat.st_mtime;
            return 1; // File has been modified
        }
    }
    return 0; // File not modified
}

void createFileFromRevision(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (file != NULL) {
        fputs(content, file);
        fclose(file);
    } else {
        printf("revs: Error creating file %s\n", filename), exit(1);
    }
}

int is_directory(const char *path) {
    struct stat path_stat;
    
    if (stat(path, &path_stat) != 0) {
        perror("revs: Error checking file or directory");
        return -1; // Indicate error
    }

    // The stat function follows symbolic links and 
    //      provides information about the file or directory
    //      to which the link points.
    // The lstat function is similar to stat but does not 
    //      follow symbolic links. It provides information 
    //      about the link itself, not the target of the link.

    return S_ISDIR(path_stat.st_mode);
}

void processFilesInDirectory() {
    DIR *dir;
    // struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        while (1) {
            rewinddir(dir);
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                const char *filename = ent->d_name;
                
                // Stat structure to get file information
                struct stat fileStat;
                char filePath[PATH_MAX];

                // Construct the full path
                snprintf(filePath, sizeof(filePath), "%s/%s", ".", filename);

                // Use stat to get file information
                if (stat(filePath, &fileStat) != -1 && S_ISREG(fileStat.st_mode)) {

                    // Skip the executable file and other non-text files
                    if (is_directory(filePath) != 0 ||
                        access(filePath, X_OK) != 0) {

                        File currentFile = createFile(filename);
                        time_t lastModified = 0;

                        if (fileModified(filePath, &lastModified)) {
                            FILE *file = fopen(filePath, "r");
                            if (file != NULL) {
                                fseek(file, 0, SEEK_END);
                                long fileSize = ftell(file);
                                fseek(file, 0, SEEK_SET);

                                char *content = (char *)malloc(fileSize + 1);
                                fread(content, 1, fileSize, file);
                                content[fileSize] = '\0';

                                addRevision(&currentFile, content);

                                free(content);
                                fclose(file);

                                // Create a file for the latest revision
                                int latestRevisionIndex = currentFile.numRevisions - 1;
                                char outputFilename[100];
                                sprintf(outputFilename, "Versions/latest_revision_%s", currentFile.filename);
                                createFileFromRevision(outputFilename, currentFile.revisions[latestRevisionIndex].content);
                            }
                        }
                    }
                }
            }
            // Sleep for a few seconds before checking again
            // sleepSeconds(5);
            sleep(5);
        }
        closedir(dir);
    } else {
        perror("revs: Error opening directory");
    }
}

int main() {
    processFilesInDirectory();

    return 0;
}
