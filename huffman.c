#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_TREE_NODES 512
#define MAX_CODE_LENGTH 256

// Estructura para nodo del árbol de Huffman
typedef struct HuffmanNode {
    unsigned char byte;
    unsigned long frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

// Estructura para tabla de códigos
typedef struct {
    unsigned char bits[MAX_CODE_LENGTH];
    int length;
} HuffmanCode;

// Pool de nodos (evita malloc/free)
static HuffmanNode node_pool[MAX_TREE_NODES];
static int node_pool_index = 0;

// Funciones auxiliares
void escribir_salida(const char *msg) {
    int len = 0;
    while (msg[len] != '\0') len++;
    write(STDOUT_FILENO, msg, len);
}

int longitud_cadena(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int comparar_cadena(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return 0;
        i++;
    }
    return s1[i] == s2[i];
}

// Crear nuevo nodo
HuffmanNode* crear_nodo(unsigned char byte, unsigned long freq) {
    if (node_pool_index >= MAX_TREE_NODES) return 0;
    
    HuffmanNode *node = &node_pool[node_pool_index++];
    node->byte = byte;
    node->frequency = freq;
    node->left = 0;
    node->right = 0;
    return node;
}

// Cola de prioridad simple para Huffman
typedef struct {
    HuffmanNode *nodes[MAX_TREE_NODES];
    int size;
} PriorityQueue;

void pq_init(PriorityQueue *pq) {
    pq->size = 0;
}

void pq_insert(PriorityQueue *pq, HuffmanNode *node) {
    int i = pq->size++;
    pq->nodes[i] = node;
    
    // Ordenar (bubble up)
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->nodes[i]->frequency >= pq->nodes[parent]->frequency) break;
        
        HuffmanNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[parent];
        pq->nodes[parent] = temp;
        i = parent;
    }
}

HuffmanNode* pq_extract_min(PriorityQueue *pq) {
    if (pq->size == 0) return 0;
    
    HuffmanNode *min = pq->nodes[0];
    pq->nodes[0] = pq->nodes[--pq->size];
    
    // Reordenar (bubble down)
    int i = 0;
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        
        if (left < pq->size && pq->nodes[left]->frequency < pq->nodes[smallest]->frequency) {
            smallest = left;
        }
        if (right < pq->size && pq->nodes[right]->frequency < pq->nodes[smallest]->frequency) {
            smallest = right;
        }
        
        if (smallest == i) break;
        
        HuffmanNode *temp = pq->nodes[i];
        pq->nodes[i] = pq->nodes[smallest];
        pq->nodes[smallest] = temp;
        i = smallest;
    }
    
    return min;
}

// Construir árbol de Huffman
HuffmanNode* construir_arbol_huffman(unsigned long *frequencies) {
    PriorityQueue pq;
    pq_init(&pq);
    
    // Crear nodos hoja para cada byte con frecuencia > 0
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode *node = crear_nodo((unsigned char)i, frequencies[i]);
            pq_insert(&pq, node);
        }
    }
    
    // Construir árbol
    while (pq.size > 1) {
        HuffmanNode *left = pq_extract_min(&pq);
        HuffmanNode *right = pq_extract_min(&pq);
        
        HuffmanNode *parent = crear_nodo(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        
        pq_insert(&pq, parent);
    }
    
    return pq_extract_min(&pq);
}

// Generar códigos Huffman
void generar_codigos(HuffmanNode *node, unsigned char *code, int depth, HuffmanCode *codes) {
    if (!node) return;
    
    if (!node->left && !node->right) {
        // Nodo hoja
        codes[node->byte].length = depth;
        for (int i = 0; i < depth; i++) {
            codes[node->byte].bits[i] = code[i];
        }
        return;
    }
    
    if (node->left) {
        code[depth] = 0;
        generar_codigos(node->left, code, depth + 1, codes);
    }
    
    if (node->right) {
        code[depth] = 1;
        generar_codigos(node->right, code, depth + 1, codes);
    }
}

// Escribir bits en un buffer
typedef struct {
    unsigned char buffer[4096];
    int byte_pos;
    int bit_pos;
} BitWriter;

void bw_init(BitWriter *bw) {
    bw->byte_pos = 0;
    bw->bit_pos = 0;
    bw->buffer[0] = 0;
}

void bw_write_bit(BitWriter *bw, int bit) {
    if (bit) {
        bw->buffer[bw->byte_pos] |= (1 << (7 - bw->bit_pos));
    }
    
    bw->bit_pos++;
    if (bw->bit_pos == 8) {
        bw->bit_pos = 0;
        bw->byte_pos++;
        if (bw->byte_pos < 4096) {
            bw->buffer[bw->byte_pos] = 0;
        }
    }
}

int bw_flush(BitWriter *bw, int fd) {
    int bytes_to_write = bw->byte_pos;
    if (bw->bit_pos > 0) bytes_to_write++;
    
    if (bytes_to_write > 0) {
        if (write(fd, bw->buffer, bytes_to_write) != bytes_to_write) {
            return -1;
        }
    }
    
    bw_init(bw);
    return 0;
}

// Comprimir archivo
int comprimir_archivo_huffman(const char *entrada, const char *salida) {
    node_pool_index = 0;
    
    // Paso 1: Contar frecuencias
    unsigned long frequencies[256] = {0};
    
    int fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir archivo de entrada\n");
        return -1;
    }
    
    unsigned char buffer[4096];
    ssize_t bytes_leidos;
    unsigned long total_bytes = 0;
    
    while ((bytes_leidos = read(fd_in, buffer, 4096)) > 0) {
        for (ssize_t i = 0; i < bytes_leidos; i++) {
            frequencies[buffer[i]]++;
            total_bytes++;
        }
    }
    
    close(fd_in);
    
    if (total_bytes == 0) {
        escribir_salida("Archivo vacio\n");
        return -1;
    }
    
    // Paso 2: Construir árbol de Huffman
    HuffmanNode *root = construir_arbol_huffman(frequencies);
    if (!root) {
        escribir_salida("Error al construir arbol\n");
        return -1;
    }
    
    // Paso 3: Generar códigos
    HuffmanCode codes[256] = {0};
    unsigned char code[MAX_CODE_LENGTH];
    generar_codigos(root, code, 0, codes);
    
    // Paso 4: Escribir archivo comprimido
    int fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear archivo de salida\n");
        return -1;
    }
    
    // Escribir encabezado: total_bytes
    write(fd_out, &total_bytes, sizeof(unsigned long));
    
    // Escribir frecuencias
    write(fd_out, frequencies, sizeof(frequencies));
    
    // Comprimir datos
    fd_in = open(entrada, O_RDONLY);
    BitWriter bw;
    bw_init(&bw);
    
    while ((bytes_leidos = read(fd_in, buffer, 4096)) > 0) {
        for (ssize_t i = 0; i < bytes_leidos; i++) {
            HuffmanCode *hc = &codes[buffer[i]];
            for (int j = 0; j < hc->length; j++) {
                bw_write_bit(&bw, hc->bits[j]);
                
                if (bw.byte_pos >= 4000) {
                    if (bw_flush(&bw, fd_out) != 0) {
                        close(fd_in);
                        close(fd_out);
                        return -1;
                    }
                }
            }
        }
    }
    
    bw_flush(&bw, fd_out);
    
    close(fd_in);
    close(fd_out);
    
    return 0;
}

// Leer bits
typedef struct {
    unsigned char buffer[4096];
    int byte_pos;
    int bit_pos;
    int bytes_available;
    int fd;
} BitReader;

void br_init(BitReader *br, int fd) {
    br->byte_pos = 0;
    br->bit_pos = 0;
    br->bytes_available = 0;
    br->fd = fd;
}

int br_read_bit(BitReader *br) {
    if (br->byte_pos >= br->bytes_available) {
        br->bytes_available = read(br->fd, br->buffer, 4096);
        if (br->bytes_available <= 0) return -1;
        br->byte_pos = 0;
        br->bit_pos = 0;
    }
    
    int bit = (br->buffer[br->byte_pos] >> (7 - br->bit_pos)) & 1;
    
    br->bit_pos++;
    if (br->bit_pos == 8) {
        br->bit_pos = 0;
        br->byte_pos++;
    }
    
    return bit;
}

// Descomprimir archivo
int descomprimir_archivo_huffman(const char *entrada, const char *salida) {
    node_pool_index = 0;
    
    int fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir archivo de entrada\n");
        return -1;
    }
    
    // Leer encabezado
    unsigned long total_bytes;
    if (read(fd_in, &total_bytes, sizeof(unsigned long)) != sizeof(unsigned long)) {
        escribir_salida("Error al leer encabezado\n");
        close(fd_in);
        return -1;
    }
    
    // Leer frecuencias
    unsigned long frequencies[256];
    if (read(fd_in, frequencies, sizeof(frequencies)) != sizeof(frequencies)) {
        escribir_salida("Error al leer frecuencias\n");
        close(fd_in);
        return -1;
    }
    
    // Reconstruir árbol
    HuffmanNode *root = construir_arbol_huffman(frequencies);
    if (!root) {
        escribir_salida("Error al reconstruir arbol\n");
        close(fd_in);
        return -1;
    }
    
    // Descomprimir
    int fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear archivo de salida\n");
        close(fd_in);
        return -1;
    }
    
    BitReader br;
    br_init(&br, fd_in);
    
    unsigned char out_buffer[4096];
    int out_pos = 0;
    unsigned long bytes_escritos = 0;
    
    HuffmanNode *current = root;
    
    while (bytes_escritos < total_bytes) {
        int bit = br_read_bit(&br);
        if (bit == -1) break;
        
        current = (bit == 0) ? current->left : current->right;
        
        if (!current->left && !current->right) {
            // Nodo hoja
            out_buffer[out_pos++] = current->byte;
            bytes_escritos++;
            current = root;
            
            if (out_pos >= 4096) {
                write(fd_out, out_buffer, out_pos);
                out_pos = 0;
            }
        }
    }
    
    if (out_pos > 0) {
        write(fd_out, out_buffer, out_pos);
    }
    
    close(fd_in);
    close(fd_out);
    
    return 0;
}

void mostrar_ayuda() {
    escribir_salida("Uso: ./huffman [opciones]\n\n");
    escribir_salida("Opciones:\n");
    escribir_salida("  -c              Comprimir archivo\n");
    escribir_salida("  -d              Descomprimir archivo\n");
    escribir_salida("  -i <archivo>    Archivo de entrada\n");
    escribir_salida("  -o <archivo>    Archivo de salida\n");
    escribir_salida("  -h              Mostrar ayuda\n\n");
    escribir_salida("Ejemplos:\n");
    escribir_salida("  ./huffman -c -i documento.pdf -o documento.huff\n");
    escribir_salida("  ./huffman -d -i documento.huff -o documento.pdf\n");
    escribir_salida("  ./huffman -c -i imagen.png -o imagen.huff\n");
}

int main(int argc, char *argv[]) {
    int operacion = 0; // 1 = comprimir, 2 = descomprimir
    char *archivo_entrada = 0;
    char *archivo_salida = 0;
    
    for (int i = 1; i < argc; i++) {
        if (comparar_cadena(argv[i], "-c")) {
            operacion = 1;
        } else if (comparar_cadena(argv[i], "-d")) {
            operacion = 2;
        } else if (comparar_cadena(argv[i], "-i") && i + 1 < argc) {
            archivo_entrada = argv[++i];
        } else if (comparar_cadena(argv[i], "-o") && i + 1 < argc) {
            archivo_salida = argv[++i];
        } else if (comparar_cadena(argv[i], "-h")) {
            mostrar_ayuda();
            return 0;
        }
    }
    
    if (operacion == 0 || !archivo_entrada || !archivo_salida) {
        escribir_salida("Error: Faltan parametros\n\n");
        mostrar_ayuda();
        return 1;
    }
    
    // Obtener tamaño del archivo original
    struct stat st;
    if (stat(archivo_entrada, &st) == 0) {
        escribir_salida("Tamaño original: ");
        char size_str[32];
        long size = st.st_size;
        int len = 0;
        if (size == 0) {
            size_str[len++] = '0';
        } else {
            char temp[32];
            int temp_len = 0;
            while (size > 0) {
                temp[temp_len++] = '0' + (size % 10);
                size /= 10;
            }
            for (int i = temp_len - 1; i >= 0; i--) {
                size_str[len++] = temp[i];
            }
        }
        size_str[len] = '\0';
        escribir_salida(size_str);
        escribir_salida(" bytes\n");
    }
    
    escribir_salida(operacion == 1 ? "Comprimiendo...\n" : "Descomprimiendo...\n");
    
    int resultado;
    if (operacion == 1) {
        resultado = comprimir_archivo_huffman(archivo_entrada, archivo_salida);
    } else {
        resultado = descomprimir_archivo_huffman(archivo_entrada, archivo_salida);
    }
    
    if (resultado == 0) {
        escribir_salida(operacion == 1 ? "Compresion exitosa\n" : "Descompresion exitosa\n");
        
        // Mostrar tamaño resultante
        if (stat(archivo_salida, &st) == 0) {
            escribir_salida("Tamaño resultado: ");
            char size_str[32];
            long size = st.st_size;
            int len = 0;
            if (size == 0) {
                size_str[len++] = '0';
            } else {
                char temp[32];
                int temp_len = 0;
                while (size > 0) {
                    temp[temp_len++] = '0' + (size % 10);
                    size /= 10;
                }
                for (int i = temp_len - 1; i >= 0; i--) {
                    size_str[len++] = temp[i];
                }
            }
            size_str[len] = '\0';
            escribir_salida(size_str);
            escribir_salida(" bytes\n");
        }
    } else {
        escribir_salida("Operacion fallida\n");
    }
    
    return resultado;
}