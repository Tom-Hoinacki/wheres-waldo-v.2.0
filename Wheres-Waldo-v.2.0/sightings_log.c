//
//  sightings_log.c
//  Waldo v1.2
//
//  Created by Thomas Hoinacki on 7/16/16.
//  Copyright (c) 2016 Thomas Hoinacki. All rights reserved.
//

// C Library Headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <time.h>
#include <sys/syslimits.h>
#include <fcntl.h>

#include "sightings_log.h"


// File Variables

//http://www.macs.hw.ac.uk/~rjp/Coursewww/Cwww/linklist.html
struct list_elm {
    int childDirToCreate;
    char * path;
    struct list_elm * next;
};

typedef struct list_elm dirLvlList;


// Function Signatures
void log_waldo_sightings_dir_depth_first(char * dirPath, FILE * sightingsLogFile, int * sightingsCount, int * firstLevel);
void log_waldo_sightings_txtfile(char* dirPath, struct dirent* in_File, FILE * sightingsLogFile, int * sightingsCount);



// Function Implementations

/* Recursive method to search all text files for "Waldo" string DEPTH-FIRST */
void log_waldo_sightings_dir_depth_first(char * dirPath, FILE * sightingsLogFile, int * sightingsCount, int * firstLevel)
{
    struct dirent * in_Dir;
    DIR * d;
    struct stat st;
    char * childDirPath = (char *)malloc(PATH_MAX);
    strcpy(childDirPath, dirPath);
    
    // Open current directory
    d = opendir(dirPath);
    stat(dirPath, &st);
 
    
    
    // Child directories
    while ((in_Dir = readdir(d)) != NULL)
    {
        if (strcmp(in_Dir->d_name, ".") == 0 || strcmp(in_Dir->d_name, "..") == 0)
            continue;
        
        if (fstatat(dirfd(d), in_Dir->d_name, &st, 0) < 0)
        {
            perror(in_Dir->d_name);
            continue;
        }
        
        // If file, open file, start string search, else traverse to next child directory
        if (S_ISREG(st.st_mode))
        {
            // Skip readding log files and text filler file in Level 1
            if (!*firstLevel)
            {
                log_waldo_sightings_txtfile(dirPath, in_Dir, sightingsLogFile, sightingsCount);
            }
        }
        else if (S_ISDIR(st.st_mode))
        {
            *firstLevel = 0;
            strcat(childDirPath, "/");
            strcat(childDirPath, in_Dir->d_name);
            log_waldo_sightings_dir_depth_first(childDirPath, sightingsLogFile, sightingsCount, firstLevel);
            strcpy(childDirPath, dirPath);
        }
    }
    
    free(childDirPath);
}


/* Opens text file and searches line by line for "Waldo" and prints to log file */
void log_waldo_sightings_txtfile(char * dirPath, struct dirent * in_File, FILE * sightingsLogFile, int * sightingsCount)
{
    char * filePath = malloc(PATH_MAX);
    FILE * entry_File;
    const int BUF_RLINE_SIZE = 150; // Memory allocated for one line
    char lineBuf[BUF_RLINE_SIZE];
    
    strcpy(filePath, dirPath);
    strcat(filePath, "/");
    strcat(filePath, in_File->d_name);
    
    struct stat statbuf;
    
    // Open current text file being navigated
    entry_File = fopen(filePath, "r");
    
    free(filePath);
    
    if (entry_File == NULL)
    {
        fprintf(stderr, "Error: Failed to open entry file - %s\n", strerror(errno));
        fclose(entry_File);
    }
    
    // Take lines in 150 character chunks using a buffer to read for "Waldo"
    while (fgets(lineBuf, BUF_RLINE_SIZE, entry_File) != NULL)
    {
        const char * tmp = lineBuf;
        
        // Take a substring up to the point "Waldo" is found in a line, if found print occurence in log
        // file and start searching through the remainder of the line until next "Waldo" and so on...
        while((tmp = strstr(tmp, "Waldo")))
        {
            // Print sighting to log file includes: Sighting occurence #, file path and line number sighted at
            fprintf(sightingsLogFile, "%d:\t%s (Line %c)\n", *sightingsCount, filePath, lineBuf[0]);
            (*sightingsCount)++; // Increment count printed at start of each log file line
            
            // End of line is hit on "Waldo" sighting then break
            if (tmp == "Waldo\n")
                break;
            
            tmp += strlen("Waldo ");
        }
    }
    
    // Close the current text file
    fclose(entry_File);
}


