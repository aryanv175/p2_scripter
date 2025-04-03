#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }

    // Get file path and search string from arguments
    char *file_path = argv[1];
    char *search_string = argv[2];
    
    // Open the file using system call
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }
    
    // Read the file line by line
    char line[1024];
    char buffer[1];
    int index = 0;
    int found = 0;
    
    while (1) {
        ssize_t bytes_read = read(fd, buffer, 1);
        
        // End of file or error
        if (bytes_read <= 0) {
            if (index > 0) {
                // Process the last line if it doesn't end with newline
                line[index] = '\0';
                if (strstr(line, search_string) != NULL) {
                    write(STDOUT_FILENO, line, strlen(line));
                    write(STDOUT_FILENO, "\n", 1);
                    found = 1;
                }
            }
            break;
        }
        
        // Build line until newline character
        if (buffer[0] != '\n') {
            if (index < 1023) { // Leave space for null terminator
                line[index++] = buffer[0];
            }
        } else {
            // End of line, process it
            line[index] = '\0';
            
            // Check if the line contains the search string
            if (strstr(line, search_string) != NULL) {
                write(STDOUT_FILENO, line, strlen(line));
                write(STDOUT_FILENO, "\n", 1);
                found = 1;
            }
            
            // Reset index for next line
            index = 0;
        }
    }
    
    // Check if the string was found
    if (!found) {
        char not_found_msg[1024];
        sprintf(not_found_msg, "\"%s\" not found.\n", search_string);
        write(STDOUT_FILENO, not_found_msg, strlen(not_found_msg));
    }
    
    // Close the file
    close(fd);
    return 0;
}
