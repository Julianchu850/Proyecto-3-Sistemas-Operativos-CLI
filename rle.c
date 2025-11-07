#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

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

int procesar_archivo(const char *input_file, const char *output_file, 
                     int do_compress, int do_decompress) {
    
    unsigned char in_buf[4096];   // ← Cambio de char a unsigned char
    unsigned char out_buf[8192];  // ← Cambio de char a unsigned char
    
    // Abrir archivo de entrada
    int fd_in = open(input_file, O_RDONLY);
    if (fd_in < 0) {
        print_error("Error: No se pudo abrir el archivo de entrada\n");
        return 1;
    }

    // Abrir archivo de salida
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        print_error("Error: No se pudo crear el archivo de salida\n");
        close(fd_in);
        return 1;
    }

    // Leer archivo y escribir resultado
    ssize_t bytes_read;
    while((bytes_read = read(fd_in, in_buf, sizeof(in_buf))) > 0) {
        printf("Bytes leidos: %ld\n", bytes_read);
        if (do_compress) {
            int result_size = comprimir_rle(in_buf, bytes_read, out_buf);
            write(fd_out, out_buf, result_size); 
        } else if (do_decompress) {
            int result_size = descomprimir_rle(in_buf, bytes_read, out_buf);
            write(fd_out, out_buf, result_size); 
        }
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
}

int main(int argc, char *argv[]) {
    int do_compress = 0;
    int do_decompress = 0;
    
    const char *input_file = 0;
    const char *output_file = 0;
    const char *comp_alg = "rle";

    // Parsear argumentos
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'c') do_compress = 1;
                else if (argv[i][j] == 'd') do_decompress = 1;
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
    if (do_compress && do_decompress) {
        print_error("Error: No puede usar -c y -d al mismo tiempo\n");
        return 1;
    }

    if (!do_compress && !do_decompress) {
        print_error("Error: Debe especificar -c (comprimir) o -d (descomprimir)\n");
        return 1;
    }

    return procesar_archivo(input_file, output_file, do_compress, do_decompress);
}