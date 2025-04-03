#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * This program searches for a specified string in a text file and displays
 * lines that contain the string. If the file doesn't contain the search string,
 * it displays a "not found" message. If any error occurs, it displays a message
 * on standard error and returns -1.
 */

int main(int argc, char ** argv) {
    if (argc != 3) {
        /* Check if correct number of arguments were provided */
        fprintf(stderr, "Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }

    /* Get file path and search string from arguments */
    char *file_path = argv[1];
    char *search_string = argv[2];
    
    /* Open the file using system call */
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        /* Handle file open error */
        perror("Error opening file");
        return -1;
    }
    
    /* Variables for reading and processing the file */
    char line[1024];    /* buffer to store each complete line */
    char buffer[1];     /* single character buffer for reading */
    int index = 0;      /* current position in line buffer */
    int found = 0;      /* flag to track if search string was found */
    
    /* Process the file character by character */
    while (1) {
        ssize_t bytes_read = read(fd, buffer, 1);
        
        /* End of file or error handling */
        if (bytes_read <= 0) {
            if (index > 0) {
                /* Process the last line if it doesn't end with newline */
                line[index] = '\0';
                if (strstr(line, search_string) != NULL) {
                    write(STDOUT_FILENO, line, strlen(line));
                    write(STDOUT_FILENO, "\n", 1);
                    found = 1;
                }
            }
            break;
        }
        
        /* Build line until newline character is reached */
        if (buffer[0] != '\n') {
            if (index < 1023) { /* Leave space for null terminator */
                line[index++] = buffer[0];
            }
        } else {
            /* End of line, process it */
            line[index] = '\0';
            
            /* Check if the line contains the search string */
            if (strstr(line, search_string) != NULL) {
                /* Output matching line to standard output */
                write(STDOUT_FILENO, line, strlen(line));
                write(STDOUT_FILENO, "\n", 1);
                found = 1;
            }
            
            /* Reset index for next line */
            index = 0;
        }
    }
    
    /* Check if the string was found and display appropriate message */
    if (!found) {
        char not_found_msg[1024];
        sprintf(not_found_msg, "\"%s\" not found.\n", search_string);
        write(STDOUT_FILENO, not_found_msg, strlen(not_found_msg));
    }
    
    /* Close the file descriptor */
    close(fd);
    return 0;
}
