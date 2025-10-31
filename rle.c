#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int comprimir_rle(char *in_buf, int in_size, char *out_buf) {
    int i, j = 0;
    int count = 1;

    if (in_size == 0) return 0;

    for (i = 0; i < in_size - 1; i++) {
        if (in_buf[i] == in_buf[i + 1]) {
            count++;
        } else {
            // Escribir conteo
            if (count >= 100) {
                out_buf[j++] = '0' + (count / 100);
                out_buf[j++] = '0' + ((count / 10) % 10);
                out_buf[j++] = '0' + (count % 10);
            } else if (count >= 10) {
                out_buf[j++] = '0' + (count / 10);
                out_buf[j++] = '0' + (count % 10);
            } else {
                out_buf[j++] = '0' + count;
            }
            out_buf[j++] = in_buf[i];
            count = 1;
        }
    }

    // Último carácter
    if (count >= 100) {
        out_buf[j++] = '0' + (count / 100);
        out_buf[j++] = '0' + ((count / 10) % 10);
        out_buf[j++] = '0' + (count % 10);
    } else if (count >= 10) {
        out_buf[j++] = '0' + (count / 10);
        out_buf[j++] = '0' + (count % 10);
    } else {
        out_buf[j++] = '0' + count;
    }
    out_buf[j++] = in_buf[i];

    return j;
}


int descomprimir_rle(char *in_buf, int in_size, char *out_buf) {
    int i = 0, j = 0;

    while (i < in_size) {
        int count = 0;
        
        // Leer dígitos
        while (i < in_size && in_buf[i] >= '0' && in_buf[i] <= '9') {
            count = count * 10 + (in_buf[i] - '0');
            i++;
        }
        
        // Leer carácter
        if (i < in_size) {
            char ch = in_buf[i++];
            for (int k = 0; k < count; k++) {
                out_buf[j++] = ch;
            }
        }
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
    
    char in_buf[4096];
    char out_buf[8192];
    
    // Abrir archivo de entrada
    int fd_in = open(input_file, O_RDONLY);
    if (fd_in < 0) {
        print_error("Error: No se pudo abrir el archivo de entrada\n");
        return 1;
    }

    // Leer archivo
    ssize_t bytes_read = read(fd_in, in_buf, sizeof(in_buf));
    close(fd_in);

    if (bytes_read < 0) {
        print_error("Error: No se pudo leer el archivo\n");
        return 1;
    }

    int result_size = 0;

    // Comprimir o descomprimir
    if (do_compress) {
        result_size = comprimir_rle(in_buf, bytes_read, out_buf);
    } else if (do_decompress) {
        result_size = descomprimir_rle(in_buf, bytes_read, out_buf);
    } else {
        print_error("Error: Debe especificar -c o -d\n");
        return 1;
    }

    // Escribir resultado
    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        print_error("Error: No se pudo crear el archivo de salida\n");
        return 1;
    }

    write(fd_out, out_buf, result_size);
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