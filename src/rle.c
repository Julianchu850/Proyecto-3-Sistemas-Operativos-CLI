#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>



int comprimir_rle(const unsigned char *in_buf, int in_size, unsigned char *out_buf) {
    int i = 0, j = 0;

    while (i < in_size) {
        unsigned char current = in_buf[i];
        int count = 1;

        // Contar repeticiones
        while (i + count < in_size && in_buf[i + count] == current && count < 255) {
            count++;
        }

        // Guardar (count, byte)
        out_buf[j++] = (unsigned char)count;
        out_buf[j++] = current;

        i += count;
    }

    return j; // tamaño del buffer comprimido
}

int descomprimir_rle(unsigned char *in_buf, int in_size, unsigned char *out_buf){
    
    if (in_size <= 0) return 0;

    // Asegurar que tenemos pares de bytes (count, value)
    if(in_size % 2 != 0){
        in_size -= 1;
    }

    int i = 0, j = 0;
    while(i < in_size){
        unsigned char count = in_buf[i];      // ← unsigned char en vez de unsigned int
        unsigned char value = in_buf[i+1];
        
        // Protección contra desbordamiento
        if(j + count > 8192){
            count = 8192 - j;
        }
        
        for(int k = 0; k < count; k++){
            out_buf[j++] = value;
        }
        
        i += 2;
    }
    return j;
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
int procesar_archivo(const char *input_file, const char *output_file, 
                     int actions[]) {
    
    //procesar_directorio(input_file);
    // printf(input_file);
    unsigned char in_buf[4096];   // ← Cambio de char a unsigned char
    unsigned char out_buf[8192];  // ← Cambio de char a unsigned char
    
    // Abrir archivo de entrada
    int fd_in = open(input_file, O_RDONLY);
    if (fd_in < 0) {
        perror("Error: No se pudo abrir el archivo de entrada\n");
        return 1;
    }

    // Abrir archivo de salida
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("Error: No se pudo crear el archivo de salida\n");
        close(fd_in);
        return 1;
    }
    

    // Leer archivo y escribir resultado
    ssize_t bytes_read;
    while((bytes_read = read(fd_in, in_buf, sizeof(in_buf))) > 0) {
        printf("Bytes leidos: %ld\n", bytes_read);
        if (actions[0]) {
            int result_size = comprimir_rle(in_buf, bytes_read, out_buf);
            write(fd_out, out_buf, result_size); 
        } else if (actions[1]) {
            int result_size = descomprimir_rle(in_buf, bytes_read, out_buf);
            write(fd_out, out_buf, result_size); 
        }
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
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



void procesar_directorio(const char *path, char *outputFile, int actions[]) {
    DIR *dir;
    struct dirent *entry;

    char pathIFile[500];
    char *pathDir;

    // Abrir directorio
    dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    // Crear path del directorio de salida
    pathDir = actualizarPath(path, outputFile);

    // Crear directorio de salida
    if (mkdir(pathDir, 0755) == -1) {
        perror("mkdir");
        closedir(dir);
        return;
    }
    
    // Leer entradas
    while ((entry = readdir(dir)) != NULL) {
// ignorar . y ..
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // reconstruir path input  ✔ REQUERIDO
        snprintf(pathIFile, sizeof(pathIFile), "%s/%s", path, entry->d_name);

        // generar nombre de salida  ✔ PASAR SOLO EL FILE NAME
        char *newName = procesarNombreSalida(entry->d_name, actions);

        // generar path de salida
        char fullOutputPath[500];
        snprintf(fullOutputPath, sizeof(fullOutputPath), "%s/%s", pathDir, newName);

        // procesar
        if (esDirectorio(pathIFile)) {
            procesar_directorio(pathIFile, fullOutputPath, actions);
        } else {
            procesar_archivo(pathIFile, fullOutputPath, actions);
        }

        free(newName);
    }

    closedir(dir);
}



int procesarEntrada(const char *inputFile, char *outputFile, int actions[]){
        if(esDirectorio(inputFile)){ // define si es un directorio, se cierra la ejecucion si no es ni dir, ni archivo
            printf("procesar directorio");
            procesar_directorio(inputFile, outputFile, actions);
            return 0;
        }else{
            printf("procesar archivo");

            return procesar_archivo(inputFile, outputFile, actions);
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

int str_len(const char *s) {
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

void print_error(const char *msg) {
    write(2, msg, str_len(msg));
}



int main(int argc, char *argv[]) {
    /**
     * actions[]: Lista de acciones a tomar, cada posicion tomara el valor de 0 o 1
     * para saber si hace esa accion.
     * actions[0] : comprimir
     * actions[1] : descomprimir
     * actions[2] : encriptar
     * actions[3] : desencriptar
     * 
    */
     int actions[4];

    
    int isEmpty = 1; // confirma si el array actions esta vacio
    
    const char *input_file = 0;
     char *output_file = 0;
    const char *comp_alg = "rle";

    // Parsear argumentos
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'c') actions[0] = 1;
                else if (argv[i][j] == 'd') actions[1] = 1;
                else if (argv[i][j] == 'i') {
                    if (i + 1 < argc) input_file = argv[++i];
                    break;
                }
                else if (argv[i][j] == 'o') {
                    if (i + 1 < argc) output_file = argv[++i];
                    break;
                }
            }
        }
        else if (str_cmp(argv[i], "--comp-alg") == 0 && i + 1 < argc) {
            comp_alg = argv[++i];
        }
    }

    // Validar entrada y salida
    if (input_file == 0 || output_file == 0) {
        print_error("Uso: ./rle [opciones] -i <entrada> -o <salida>\n");
        print_error("Opciones:\n");
        print_error("  -c: comprimir\n");
        print_error("  -d: descomprimir\n");
        print_error("  -i <archivo>: archivo de entrada\n");
        print_error("  -o <archivo>: archivo de salida\n");
        print_error("\nEjemplos:\n");
        print_error("  ./rle -c -i prueba1.txt -o prueba1.dat\n");
        print_error("  ./rle -d -i prueba1.dat -o prueba1_recuperado.txt\n");
        return 1;
    }

    // Validar que solo se use -c o -d, no ambos
    if (actions[0] && actions[1]) {
        print_error("Error: No puede usar -c y -d al mismo tiempo\n");
        return 1;
    }

    for(int i = 0; i< 4; i++){
        if(actions[i] == 1){
            isEmpty = 0;
            break;
        }
    }
    if (isEmpty) {
        print_error("Error: Debe especificar -c (comprimir) o -d (descomprimir)\n");
        return 1;
    }

    return procesarEntrada(input_file, output_file, actions);
}