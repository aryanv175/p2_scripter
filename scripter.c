#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* CONST VARS */
const int max_line = 1024;
const int max_commands = 100; // Ampliado para permitir secuencias largas
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15

/* VARS TO BE USED FOR THE STUDENTS */
char * argvv[max_args]; /*guarda argumentos de un único comando*/
char * filev[max_redirections]; /* filev[0] fichero de entrada, 1: salida, 2: error*/
int background = 0;  /* vale 1 si cmd termina en & */

/*
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens
 * delim is | and it stores the words in tokens[] array, which we use as argv to execute the cmds
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim); /* para recorrer cada palabra*/
                                        /*reemplaza | por un fin de str: /0 */
    while (token != NULL && i < max_tokens - 1) { /* loop to get through all words*/
        tokens[i++] = token;
        token = strtok(NULL, delim); /*le pasamos null para seguir desde donde se quedó*/
    }
    tokens[i] = NULL; /* marca el final del array porque execvp() lo pide*/
    return i; /*return the number of tokens*/
}

/*
 * This function processes the command line to evaluate if there are redirections.
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) { /* recibe array de str ya procesado con tokenizar */
    // initialization for every command
    filev[0] = NULL; // STDIN
    filev[1] = NULL; // STDOUT
    filev[2] = NULL; // STDERR

    int write_idx = 0; // índice para compactar el array y eliminar redirecciones

    for (int i = 0; args[i] != NULL; i++) { /* recorre cada argumento del comando */
        if (strcmp(args[i], "<") == 0) { /* STDIN, strcmp compara caracteres */
            if (args[i+1] != NULL) {
                filev[0] = args[i+1]; /* guardamos el nombre del fichero en file[0] */
                i++; // saltamos el nombre del fichero
            } else {
                fprintf(stderr, "Error: falta nombre de archivo para redirección.\n");
            }
        } else if (strcmp(args[i], ">") == 0) { // same for STDOUT
            if (args[i+1] != NULL) {
                filev[1] = args[i+1];
                i++;
            } else {
                fprintf(stderr, "Error: falta nombre de archivo para redirección.\n");
            }
        } else if (strcmp(args[i], "!>") == 0) { /* STDERR */
            if (args[i+1] != NULL) {
                filev[2] = args[i+1];
                i++;
            } else {
                fprintf(stderr, "Error: falta nombre de archivo para redirección.\n");
            }
        } else {
            /* si no es una redirección, lo dejamos en args */
            args[write_idx++] = args[i]; //nos aseguramos de guardar aqui solo argumentos, no redirecciones
        }
    }
    args[write_idx] = NULL; /* cerramos la lista de argumentos */
}

/*
 * Esta función procesa la línea de comandos y ejecuta cada comando individual.
 * Usa execvp para ejecutar comandos simples y gestiona redirecciones y background.
 */
void procesar_linea(char *linea) {
  // & para saber si debe ejecutarse el comando en segundo plano
    char *comandos[max_commands]; //aray para guardar cada subcomando
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);

    //Check if background is indicated
    if (strchr(comandos[num_comandos - 1], '&')) { //strchr looks for & at the end
        background = 1; //if there is &, bckg = 1, else 0
        char *pos = strchr(comandos[num_comandos - 1], '&');
        *pos = '\0'; //deletes & and replaces by \0
    } else {
        background = 0;
    }

    //PIPES
    // Creamos los pipes necesarios antes del bucle
int pipe_fds[max_commands - 1][2];

for (int i = 0; i < num_comandos - 1; i++) {
    if (pipe(pipe_fds[i]) == -1) {
        perror("Error al crear pipe");
        exit(EXIT_FAILURE);
    }
}

for (int i = 0; i < num_comandos; i++) {
    memset(argvv, 0, sizeof(argvv));
    filev[0] = filev[1] = filev[2] = NULL;

    int args_count = tokenizar_linea(comandos[i], " \t\n", argvv, max_args);
    procesar_redirecciones(argvv);

    if (argvv[0] == NULL) {
        fprintf(stderr, "Línea ignorada: comando vacío o inválido\n");
        continue;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork");
        continue;
    }

    if (pid == 0) {  // CHILD
        // Si no es el primer comando, redirigimos entrada al pipe anterior
        if (i > 0) {
            dup2(pipe_fds[i - 1][0], STDIN_FILENO);
        }

        // Si no es el último comando, redirigimos salida al pipe actual
        if (i < num_comandos - 1) {
            dup2(pipe_fds[i][1], STDOUT_FILENO);
        }

        // Cerramos todos los extremos de los pipes en el hijo
        for (int j = 0; j < num_comandos - 1; j++) {
            close(pipe_fds[j][0]);
            close(pipe_fds[j][1]);
        }

        // Redirecciones de archivos
        if (filev[0]) {
            int fd_in = open(filev[0], O_RDONLY);
            if (fd_in < 0) { perror("Error en redirección de entrada"); exit(1); }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (filev[1]) {
            int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_out < 0) { perror("Error en redirección de salida"); exit(1); }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (filev[2]) {
            int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd_err < 0) { perror("Error en redirección de error"); exit(1); }
            dup2(fd_err, STDERR_FILENO);
            close(fd_err);
        }

        execvp(argvv[0], argvv);
        perror("Error en execvp");
        exit(EXIT_FAILURE);
    }

    // PADRE
    if (i > 0) {
        close(pipe_fds[i - 1][0]); // cerramos lectura anterior
    }
    if (i < num_comandos - 1) {
        close(pipe_fds[i][1]); // cerramos escritura actual
    }

    if (!background) {
        waitpid(pid, NULL, 0);
    } else {
        printf("[Background PID: %d]\n", pid);
    }
}

}

/*
 * Lee una línea del fichero carácter a carácter hasta '\n' o EOF.
 * Guarda la línea en buffer y la termina con '\0'.
 * Devuelve:
 *  - número de caracteres leídos (sin contar '\0'),
 *  - 0 si EOF sin datos,
 *  - -1 si ocurre error o línea demasiado larga.
 */
int read_line(int fd, char *buffer, int max_length) {
    int i = 0;
    char c;
    ssize_t bytes_read;

    while (i < max_length - 1) {
        bytes_read = read(fd, &c, 1);
        if (bytes_read == 0) {
            // EOF
            if (i == 0) return 0;
            break;
        } else if (bytes_read < 0) {
            perror("Error al leer del fichero");
            return -1;
        }

        if (c == '\n') break;

        buffer[i++] = c;
    }

    buffer[i] = '\0';

    // Línea muy larga
    if (i == max_length - 1 && c != '\n') {
        fprintf(stderr, "Línea demasiado larga (más de %d caracteres)\n", max_length - 1);
        return -1;
    }

    return i;
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <fichero_script>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY); //opens file in read mode
    if (fd == -1) {
        perror("Error al abrir el fichero");
        return 1;
    }


    char buffer[max_line]; // create a buffer to store each line
    int n_leidos;
    
    // Check if first line is "## Script de SSOO"
    if ((n_leidos = read_line(fd, buffer, max_line)) <= 0 ||
        strcmp(buffer, "## Script de SSOO") != 0) {
        perror("Error: El primer renglón debe ser '## Script de SSOO'");
        close(fd);
        return -1;
    }
    

    // Process remaining lines
    int line_count = 0;
    while ((n_leidos = read_line(fd, buffer, max_line)) > 0) {
        line_count++;
        printf("\u2192 Línea leída [%d]: %s\n", line_count, buffer);
        
        // Check if line is empty (only spaces)
        int solo_espacios = 1;
        for (int i = 0; i < n_leidos; i++) {
            if (buffer[i] != ' ' && buffer[i] != '\t') {
                solo_espacios = 0;
                break;
            }
        }
        
        if (solo_espacios) {
            perror("Error: Línea vacía encontrada");
            close(fd);
            return -1;
        }
        
        procesar_linea(buffer);
    }

    printf("Total lines processed: %d\n", line_count);
    close(fd);
    return 0;
}