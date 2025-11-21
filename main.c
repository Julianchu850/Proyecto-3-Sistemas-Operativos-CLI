#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "huffman.h"
#include "aes.h"
#include "rle.h"

void print_error(const char *msg) {
    write(2, msg, strlen(msg));
}


int esDirectorio(const char *path) {
    struct stat s;

    if (stat(path, &s) != 0) {  // 0 si es exitoso
        perror("stat");
        return -1; // error
    }

    if (S_ISDIR(s.st_mode)) {
        return 1;  // es directorio
    }

    if (S_ISREG(s.st_mode)) {
        return 0;  // es archivo regular
    }

    return -1; // otro tipo (enlace, socket, etc.)
}



char *procesarNombreSalida(char *inputFile, int actions[]){
    char output[100] = {0};
    char extension[100] = {0};
    
    for(int i = 0; inputFile[i] != '\0'; i++){
        if(inputFile[i] != '.'){
            output[i] = inputFile[i];
        }
        if(inputFile[i] == '.'){
            int k = 0;
            for(int j = i; inputFile[j] != '\0'; j++){
                extension[k] = inputFile[j];
                k++;
            }
            
        }
     
    }
    
    if(actions[0]){ // Compresion
        strcat(output,"Comprimido.dat");
    }else if(actions[1]){// descompresion
        strcat(output, "_Descomprimido");
        strcat(output, ".desconocido");
    }    else if(actions[2]) { // cifrar
        strcat(output, "_Cifrado.enc");
    } 
    else if(actions[3]) { // descifrar
        strcat(output, "_Descifrado.dec");
    }
    // completar con lo de encriptacion

     // Reservar memoria dinámica para devolver el nombre
    char *resultado = malloc(strlen(output) + 1);
    if (!resultado) return NULL;

    strcpy(resultado, output);
    return resultado; 
}

/**
 * Funcion encargada de procesar la accion(encriptar, comprimir, etc) para directorios
 */
int procesar_archivo(const char *input_file, const char *output_file, int actions[], const char *alg) {
    unsigned char in_buf[4096];
    unsigned char out_buf[8192];

    if (alg == NULL) {
        print_error("Error: No se especificó algoritmo\n");
        return 1;
    }

    // **Huffman**
    if (strcmp(alg, "Huffman") == 0) {
        if (actions[0]) {
            return comprimir_archivo_huffman(input_file, output_file);
        } else if (actions[1]) {
            return descomprimir_archivo_huffman(input_file, output_file);
        }
    }
    // **RLE**
    else if (strcmp(alg, "rle") == 0) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) { perror("open input"); return 1; }
        int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) { perror("open output"); close(fd_in); return 1; }

        ssize_t bytes_read;
        while ((bytes_read = read(fd_in, in_buf, sizeof(in_buf))) > 0) {
            int result_size;
            if (actions[0]) { // comprimir
                result_size = comprimir_rle(in_buf, bytes_read, out_buf);
            } else { // descomprimir
                result_size = descomprimir_rle(in_buf, bytes_read, out_buf);
            }
            write(fd_out, out_buf, result_size);
        }
        close(fd_in);
        close(fd_out);
        return 0;
    }
    // **AES**
    else if (strcmp(alg, "aes") == 0) {
        unsigned char clave[16] = {0}; // aquí podrías usar una clave fija o pedirla
        generar_clave_aes("clave123", clave);
        if (actions[2]) return cifrar_archivo_aes(input_file, output_file, clave);
        if (actions[3]) return descifrar_archivo_aes(input_file, output_file, clave);
    }

    print_error("Algoritmo no soportado\n");
    return 1;
}

char *actualizarPath(const char *path,  char *outputFile){
    int slash = -1;
    char output[200] = {0};
    int len = strlen(path);
    // logica para tomar la posicion donde empieza el nombre del archivo en el path
            for(int i = len-1; i >= 0; i--){
                if(path[i] == '/'){
                    slash = i; 
                    break;
                }
                
                }
        

    if(slash != -1){
    strncpy(output, path, slash+1);  
    strcat(output, outputFile);

         // Reservar memoria dinámica para devolver el nombre
    char *resultado = malloc(strlen(output) + 1);
    if (!resultado) return NULL;

    strcpy(resultado, output);
    return resultado; 
    
    }else{
        char *res = malloc(strlen(outputFile) + 1);
        strcpy(res, outputFile);
        return res;
    } 
}



void procesar_directorio(const char *path, char *outputFile, int actions[], const char *alg) {
    DIR *dir;
    struct dirent *entry;
    char pathIFile[500];

    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    char *pathDir = actualizarPath(path, outputFile);
    if (!pathDir) {
        closedir(dir);
        return;
    }

    if (mkdir(pathDir, 0755) == -1) {
        perror("mkdir");
        closedir(dir);
        free(pathDir);
        return;
    }
    
    printf("[PID %d] Procesando directorio: %s -> %s\n", getpid(), path, pathDir);
    
    // Array dinámico de PIDs
    pid_t *pids = NULL;
    int num_procesos = 0;
    int capacity = 10;
    
    pids = malloc(capacity * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        closedir(dir);
        free(pathDir);
        return;
    }
    
    // Leer entradas
    while ((entry = readdir(dir)) != NULL) {
        // ignorar . y ..
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Expandir array si es necesario
        if (num_procesos >= capacity) {
            capacity *= 2;
            pid_t *new_pids = realloc(pids, capacity * sizeof(pid_t));
            if (!new_pids) {
                perror("realloc");
                break;
            }
            pids = new_pids;
        }

        // reconstruir path input
        snprintf(pathIFile, sizeof(pathIFile), "%s/%s", path, entry->d_name);

        // generar nombre de salida
        char *newName = procesarNombreSalida(entry->d_name, actions);
        if (!newName) continue;

        // generar path de salida
        char fullOutputPath[500];
        snprintf(fullOutputPath, sizeof(fullOutputPath), "%s/%s", pathDir, newName);

        /**
         * Bloque para crear procesos hijo con fork
         */
        pid_t pid = fork();
        
        if (pid < 0) {
            // en caso de error al crear proceso
            perror("fork");
            free(newName);
            continue;
        }
        else if (pid == 0) {
            /**
             * Codigo pa procesar hijos
             */
            closedir(dir);    // El hijo cierra el directorio y lista pids
            free(pids);       
            
            printf("[HIJO PID %d] Procesando: %s\n", getpid(), pathIFile);
            
            // Procesar archivo o directorio
            if (esDirectorio(pathIFile) == 1) {
    procesar_directorio(pathIFile, fullOutputPath, actions, alg);
            } else {
                procesar_archivo(pathIFile, fullOutputPath, actions, alg);
            }
            
            free(newName);
            free(pathDir);
            exit(0);  // el hijo termina aqui
        }
        else {
            // 
            pids[num_procesos++] = pid;
            printf("[PADRE PID %d] Creó hijo PID %d para: %s\n", 
                   getpid(), pid, pathIFile);
        }
        
        free(newName);
    }

    closedir(dir);

    // el padre debe esperar los procesos hijos
    printf("[PADRE PID %d] Esperando a %d procesos hijos...\n", 
           getpid(), num_procesos);
    
    for (int i = 0; i < num_procesos; i++) {
        int status;
        pid_t finished_pid = waitpid(pids[i], &status, 0);
        
        if (finished_pid > 0) {
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code == 0) {
                    printf("[PADRE PID %d] ✓ Hijo PID %d terminó exitosamente\n", 
                           getpid(), finished_pid);
                } else {
                    printf("[PADRE PID %d] ✗ Hijo PID %d terminó con error (código %d)\n", 
                           getpid(), finished_pid, exit_code);
                }
            } else {
                printf("[PADRE PID %d] ✗ Hijo PID %d terminó anormalmente\n", 
                       getpid(), finished_pid);
            }
        } else {
            perror("waitpid");
        }
    }
    
    printf("[PADRE PID %d] ✓ Todos los procesos completados para: %s\n", 
           getpid(), path);

    free(pids);
    free(pathDir);
}


int procesarEntrada(const char *inputFile, char *outputFile, int actions[], const char *alg) {
    if (esDirectorio(inputFile) == 1) {
        procesar_directorio(inputFile, outputFile, actions, alg);
        return 0;
    } else if (esDirectorio(inputFile) == 0) {
        return procesar_archivo(inputFile, outputFile, actions, alg);
    } else {
        print_error("Error: Ruta no válida\n");
        return 1;
    }
}


int str_cmp(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        i++;
    }
    return s1[i] - s2[i];
}





int main(int argc, char *argv[]) {
    int actions[4] = {0, 0, 0, 0}; // 0: comprimir, 1: descomprimir, 2: cifrar, 3: descifrar
    int isEmpty = 1;

    const char *input_file = NULL;
    char *output_file = NULL;
    const char *comp_alg = NULL;
    const char *enc_alg = NULL;
    const char *alg = NULL;

    // Parsear argumentos
   for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
        if (argv[i][1] != '-') {
            // opciones cortas
            if (argv[i][1] == 'c') actions[0] = 1;
            else if (argv[i][1] == 'd') actions[1] = 1;
            else if (argv[i][1] == 'e') actions[2] = 1;
            else if (argv[i][1] == 'u') actions[3] = 1;
            else if (argv[i][1] == 'i' && i + 1 < argc) input_file = argv[++i];
            else if (argv[i][1] == 'o' && i + 1 < argc) output_file = argv[++i];
            else {
                print_error("Opción desconocida\n");
                return 1;
            }
        } else {
            // opciones largas "--comp-alg" o "--enc-alg"
            if (strcmp(argv[i], "--comp-alg") == 0 && i + 1 < argc) {
                comp_alg = argv[++i];
            } else if (strcmp(argv[i], "--enc-alg") == 0 && i + 1 < argc) {
                enc_alg = argv[++i];
            } else {
                print_error("Opción desconocida\n");
                return 1;
            }
        }
    }
}


    if (comp_alg && enc_alg) {
        print_error("Error: No puede usar dos algoritmos al mismo tiempo\n");
        return 1;
    }

    // Determinar algoritmo a usar
    if (actions[0] || actions[1]) alg = "Huffman"; // por defecto compresión
    else if (actions[2] || actions[3]) alg = "aes"; // por defecto cifrado

    if (comp_alg != NULL && (actions[0] || actions[1])) {
        if (strcmp(comp_alg, "rle") == 0) alg = "rle";
        else if (strcmp(comp_alg, "huffman") == 0) alg = "Huffman";
    }

    if (enc_alg != NULL && (actions[2] || actions[3])) {
        if (strcmp(enc_alg, "aes") == 0) alg = "aes";
    }

    // Verificar que se haya especificado alguna acción
    for (int i = 0; i < 4; i++)
        if (actions[i]) { isEmpty = 0; break; }

    if (isEmpty) {
        print_error("Error: Debe especificar -c (comprimir), -d (descomprimir), -e (cifrar) o -u (descifrar)\n");
        return 1;
    }

    // Llamada final
    return procesarEntrada(input_file, output_file, actions, alg);
}
