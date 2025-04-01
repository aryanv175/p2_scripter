#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }

    // Get file path and search string from arguments
    char *file_path = argv[1];
    char *search_string = argv[2];
    
    // Open the file
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }
    
    // Read the file line by line
    char line[1024];
    int found = 0;
    
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Check if the line contains the search string
        if (strstr(line, search_string) != NULL) {
            printf("%s\n", line);
            found = 1;
        }
    }
    
    // Check if the string was found
    if (!found) {
        printf("\"%s\" not found.\n", search_string);
    }
    
    // Close the file
    fclose(file);
    return 0;
}
