#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define BUFFER_SIZE 4096
#define MAX_PATH 256

// Función para escribir en salida estándar
void escribir_salida(const char *msg) {
    int len = 0;
    while (msg[len] != '\0') len++;
    write(STDOUT_FILENO, msg, len);
}

// Función para obtener longitud de cadena
int longitud_cadena(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// Comparar cadenas
int comparar_cadena(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return 0;
        i++;
    }
    return s1[i] == s2[i];
}

// Cifrar buffer: suma cada byte con la clave (módulo 256)
void cifrar_buffer_binario(unsigned char *buffer, int size, const unsigned char *clave, 
                           int len_clave, int *pos_clave) {
    for (int i = 0; i < size; i++) {
        int indice = (*pos_clave) % len_clave;
        buffer[i] = (buffer[i] + clave[indice]) % 256;
        (*pos_clave)++;
    }
}

// Descifrar buffer: resta cada byte con la clave (módulo 256)
void descifrar_buffer_binario(unsigned char *buffer, int size, const unsigned char *clave,
                              int len_clave, int *pos_clave) {
    for (int i = 0; i < size; i++) {
        int indice = (*pos_clave) % len_clave;
        buffer[i] = (buffer[i] - clave[indice] + 256) % 256;
        (*pos_clave)++;
    }
}

// Cifrar archivo completo
int cifrar_archivo_binario(const char *entrada, const char *salida, const unsigned char *clave, int len_clave) {
    int fd_in, fd_out;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos;
    int posicion = 0;
    
    fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir el archivo de entrada\n");
        return -1;
    }
    
    fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear el archivo de salida\n");
        close(fd_in);
        return -1;
    }
    
    while ((bytes_leidos = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        cifrar_buffer_binario(buffer, bytes_leidos, clave, len_clave, &posicion);
        
        ssize_t bytes_escritos = write(fd_out, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            escribir_salida("Error: Fallo al escribir en el archivo de salida\n");
            close(fd_in);
            close(fd_out);
            return -1;
        }
    }
    
    if (bytes_leidos == -1) {
        escribir_salida("Error: Fallo al leer el archivo de entrada\n");
        close(fd_in);
        close(fd_out);
        return -1;
    }
    
    close(fd_in);
    close(fd_out);
    
    return 0;
}

// Descifrar archivo completo
int descifrar_archivo_binario(const char *entrada, const char *salida, const unsigned char *clave, int len_clave) {
    int fd_in, fd_out;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_leidos;
    int posicion = 0;
    
    fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir el archivo de entrada\n");
        return -1;
    }
    
    fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear el archivo de salida\n");
        close(fd_in);
        return -1;
    }
    
    while ((bytes_leidos = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        descifrar_buffer_binario(buffer, bytes_leidos, clave, len_clave, &posicion);
        
        ssize_t bytes_escritos = write(fd_out, buffer, bytes_leidos);
        if (bytes_escritos != bytes_leidos) {
            escribir_salida("Error: Fallo al escribir en el archivo de salida\n");
            close(fd_in);
            close(fd_out);
            return -1;
        }
    }
    
    if (bytes_leidos == -1) {
        escribir_salida("Error: Fallo al leer el archivo de entrada\n");
        close(fd_in);
        close(fd_out);
        return -1;
    }
    
    close(fd_in);
    close(fd_out);
    
    return 0;
}

// Convertir string a bytes (para la clave)
void preparar_clave(const char *clave_str, unsigned char *clave_bytes, int len) {
    for (int i = 0; i < len; i++) {
        clave_bytes[i] = (unsigned char)clave_str[i];
    }
}

// Mostrar ayuda
void mostrar_ayuda() {
    escribir_salida("Uso: ./vigenere [opciones]\n\n");
    escribir_salida("Opciones:\n");
    escribir_salida("  -e              Encriptar archivo\n");
    escribir_salida("  -u              Desencriptar archivo\n");
    escribir_salida("  -i <archivo>    Archivo de entrada\n");
    escribir_salida("  -o <archivo>    Archivo de salida\n");
    escribir_salida("  -k <clave>      Clave secreta\n");
    escribir_salida("  -h              Mostrar ayuda\n\n");
    escribir_salida("Ejemplos:\n");
    escribir_salida("  ./vigenere -e -i foto.jpg -o foto_cifrada.jpg -k MiClave123\n");
    escribir_salida("  ./vigenere -u -i foto_cifrada.jpg -o foto_original.jpg -k MiClave123\n");
    escribir_salida("  ./vigenere -e -i documento.pdf -o doc_cifrado.pdf -k Pass2024\n");
}

int main(int argc, char *argv[]) {
    int operacion = 0; // 0 = ninguna, 1 = cifrar, 2 = descifrar
    char *archivo_entrada = 0;
    char *archivo_salida = 0;
    char *clave_str = 0;
    
    // Parsear argumentos
    for (int i = 1; i < argc; i++) {
        if (comparar_cadena(argv[i], "-e")) {
            operacion = 1;
        } else if (comparar_cadena(argv[i], "-u")) {
            operacion = 2;
        } else if (comparar_cadena(argv[i], "-i") && i + 1 < argc) {
            archivo_entrada = argv[++i];
        } else if (comparar_cadena(argv[i], "-o") && i + 1 < argc) {
            archivo_salida = argv[++i];
        } else if (comparar_cadena(argv[i], "-k") && i + 1 < argc) {
            clave_str = argv[++i];
        } else if (comparar_cadena(argv[i], "-h")) {
            mostrar_ayuda();
            return 0;
        }
    }
    
    // Validar parámetros
    if (operacion == 0) {
        escribir_salida("Error: Debe especificar operacion (-e para cifrar, -u para descifrar)\n\n");
        mostrar_ayuda();
        return 1;
    }
    
    if (!archivo_entrada || !archivo_salida || !clave_str) {
        escribir_salida("Error: Faltan parametros requeridos\n\n");
        mostrar_ayuda();
        return 1;
    }
    
    int len_clave = longitud_cadena(clave_str);
    if (len_clave == 0) {
        escribir_salida("Error: La clave no puede estar vacia\n");
        return 1;
    }
    
    // Preparar clave en formato de bytes
    unsigned char clave_bytes[256];
    preparar_clave(clave_str, clave_bytes, len_clave);
    
    // Procesar archivo (cifrar o descifrar)
    escribir_salida(operacion == 1 ? "Cifrando archivo...\n" : "Descifrando archivo...\n");
    
    int resultado;
    if (operacion == 1) {
        resultado = cifrar_archivo_binario(archivo_entrada, archivo_salida, clave_bytes, len_clave);
    } else {
        resultado = descifrar_archivo_binario(archivo_entrada, archivo_salida, clave_bytes, len_clave);
    }
    
    if (resultado == 0) {
        escribir_salida(operacion == 1 ? 
            "Archivo cifrado exitosamente\n" : 
            "Archivo descifrado exitosamente\n");
    } else {
        escribir_salida("Operacion fallida\n");
    }
    
    return resultado;
}