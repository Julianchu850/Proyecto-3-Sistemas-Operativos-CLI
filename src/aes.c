#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 4096
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16

// S-box de AES (tabla de sustitución)
static const unsigned char sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// S-box inversa para descifrado
static const unsigned char inv_sbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Rcon (constantes para expansión de clave)
static const unsigned char Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// Estructura para manejar el estado de AES
typedef struct {
    unsigned char round_keys[176]; // 11 round keys de 16 bytes cada una
} AES_Context;

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

// Multiplicación en GF(2^8) para AES
unsigned char gmul(unsigned char a, unsigned char b) {
    unsigned char p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        unsigned char hi_bit = a & 0x80;
        a <<= 1;
        if (hi_bit) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

// Expansión de clave (Key Schedule)
void aes_key_expansion(const unsigned char *key, AES_Context *ctx) {
    // Copiar la clave inicial
    for (int i = 0; i < 16; i++) {
        ctx->round_keys[i] = key[i];
    }
    
    // Generar las 10 round keys restantes
    for (int i = 4; i < 44; i++) {
        unsigned char temp[4];
        for (int j = 0; j < 4; j++) {
            temp[j] = ctx->round_keys[(i-1)*4 + j];
        }
        
        if (i % 4 == 0) {
            // RotWord
            unsigned char t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;
            
            // SubWord
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
            
            // XOR con Rcon
            temp[0] ^= Rcon[i/4];
        }
        
        for (int j = 0; j < 4; j++) {
            ctx->round_keys[i*4 + j] = ctx->round_keys[(i-4)*4 + j] ^ temp[j];
        }
    }
}

// AddRoundKey
void add_round_key(unsigned char *state, const unsigned char *round_key) {
    for (int i = 0; i < 16; i++) {
        state[i] ^= round_key[i];
    }
}

// SubBytes
void sub_bytes(unsigned char *state) {
    for (int i = 0; i < 16; i++) {
        state[i] = sbox[state[i]];
    }
}

void inv_sub_bytes(unsigned char *state) {
    for (int i = 0; i < 16; i++) {
        state[i] = inv_sbox[state[i]];
    }
}

// ShiftRows
void shift_rows(unsigned char *state) {
    unsigned char temp;
    
    // Fila 1: shift 1
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;
    
    // Fila 2: shift 2
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;
    
    // Fila 3: shift 3
    temp = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = temp;
}

void inv_shift_rows(unsigned char *state) {
    unsigned char temp;
    
    // Fila 1: shift 3 (inverso de 1)
    temp = state[13];
    state[13] = state[9];
    state[9] = state[5];
    state[5] = state[1];
    state[1] = temp;
    
    // Fila 2: shift 2
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;
    
    // Fila 3: shift 1 (inverso de 3)
    temp = state[3];
    state[3] = state[7];
    state[7] = state[11];
    state[11] = state[15];
    state[15] = temp;
}

// MixColumns
void mix_columns(unsigned char *state) {
    for (int i = 0; i < 4; i++) {
        unsigned char a = state[i*4];
        unsigned char b = state[i*4 + 1];
        unsigned char c = state[i*4 + 2];
        unsigned char d = state[i*4 + 3];
        
        state[i*4] = gmul(a, 2) ^ gmul(b, 3) ^ c ^ d;
        state[i*4 + 1] = a ^ gmul(b, 2) ^ gmul(c, 3) ^ d;
        state[i*4 + 2] = a ^ b ^ gmul(c, 2) ^ gmul(d, 3);
        state[i*4 + 3] = gmul(a, 3) ^ b ^ c ^ gmul(d, 2);
    }
}

void inv_mix_columns(unsigned char *state) {
    for (int i = 0; i < 4; i++) {
        unsigned char a = state[i*4];
        unsigned char b = state[i*4 + 1];
        unsigned char c = state[i*4 + 2];
        unsigned char d = state[i*4 + 3];
        
        state[i*4] = gmul(a, 14) ^ gmul(b, 11) ^ gmul(c, 13) ^ gmul(d, 9);
        state[i*4 + 1] = gmul(a, 9) ^ gmul(b, 14) ^ gmul(c, 11) ^ gmul(d, 13);
        state[i*4 + 2] = gmul(a, 13) ^ gmul(b, 9) ^ gmul(c, 14) ^ gmul(d, 11);
        state[i*4 + 3] = gmul(a, 11) ^ gmul(b, 13) ^ gmul(c, 9) ^ gmul(d, 14);
    }
}

// Cifrar un bloque de 16 bytes
void aes_encrypt_block(unsigned char *block, const AES_Context *ctx) {
    add_round_key(block, ctx->round_keys);
    
    for (int round = 1; round < 10; round++) {
        sub_bytes(block);
        shift_rows(block);
        mix_columns(block);
        add_round_key(block, ctx->round_keys + round * 16);
    }
    
    sub_bytes(block);
    shift_rows(block);
    add_round_key(block, ctx->round_keys + 160);
}

// Descifrar un bloque de 16 bytes
void aes_decrypt_block(unsigned char *block, const AES_Context *ctx) {
    add_round_key(block, ctx->round_keys + 160);
    inv_shift_rows(block);
    inv_sub_bytes(block);
    
    for (int round = 9; round > 0; round--) {
        add_round_key(block, ctx->round_keys + round * 16);
        inv_mix_columns(block);
        inv_shift_rows(block);
        inv_sub_bytes(block);
    }
    
    add_round_key(block, ctx->round_keys);
}

// Cifrar archivo
int cifrar_archivo_aes(const char *entrada, const char *salida, const unsigned char *clave) {
    AES_Context ctx;
    aes_key_expansion(clave, &ctx);
    
    int fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir archivo de entrada\n");
        return -1;
    }
    
    int fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear archivo de salida\n");
        close(fd_in);
        return -1;
    }
    
    // Obtener tamaño del archivo
    struct stat st;
    fstat(fd_in, &st);
    long file_size = st.st_size;
    
    // Escribir tamaño original (para remover padding al descifrar)
    write(fd_out, &file_size, sizeof(long));
    
    unsigned char buffer[AES_BLOCK_SIZE];
    ssize_t bytes_leidos;
    
    while ((bytes_leidos = read(fd_in, buffer, AES_BLOCK_SIZE)) > 0) {
        // Aplicar padding PKCS#7 si es necesario
        if (bytes_leidos < AES_BLOCK_SIZE) {
            unsigned char padding = AES_BLOCK_SIZE - bytes_leidos;
            for (int i = bytes_leidos; i < AES_BLOCK_SIZE; i++) {
                buffer[i] = padding;
            }
        }
        
        aes_encrypt_block(buffer, &ctx);
        
        if (write(fd_out, buffer, AES_BLOCK_SIZE) != AES_BLOCK_SIZE) {
            escribir_salida("Error al escribir\n");
            close(fd_in);
            close(fd_out);
            return -1;
        }
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
}

// Descifrar archivo
int descifrar_archivo_aes(const char *entrada, const char *salida, const unsigned char *clave) {
    AES_Context ctx;
    aes_key_expansion(clave, &ctx);
    
    int fd_in = open(entrada, O_RDONLY);
    if (fd_in == -1) {
        escribir_salida("Error: No se pudo abrir archivo de entrada\n");
        return -1;
    }
    
    int fd_out = open(salida, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out == -1) {
        escribir_salida("Error: No se pudo crear archivo de salida\n");
        close(fd_in);
        return -1;
    }
    
    // Leer tamaño original
    long file_size;
    read(fd_in, &file_size, sizeof(long));
    
    unsigned char buffer[AES_BLOCK_SIZE];
    long bytes_escritos_total = 0;
    
    while (read(fd_in, buffer, AES_BLOCK_SIZE) == AES_BLOCK_SIZE) {
        aes_decrypt_block(buffer, &ctx);
        
        // Calcular cuántos bytes escribir (remover padding en el último bloque)
        ssize_t bytes_a_escribir = AES_BLOCK_SIZE;
        if (bytes_escritos_total + AES_BLOCK_SIZE > file_size) {
            bytes_a_escribir = file_size - bytes_escritos_total;
        }
        
        if (write(fd_out, buffer, bytes_a_escribir) != bytes_a_escribir) {
            escribir_salida("Error al escribir\n");
            close(fd_in);
            close(fd_out);
            return -1;
        }
        
        bytes_escritos_total += bytes_a_escribir;
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
}

// Generar clave de 16 bytes desde una cadena
void generar_clave_aes(const char *clave_str, unsigned char *clave) {
    int len = longitud_cadena(clave_str);
    
    // Simple hash para generar clave de 16 bytes
    for (int i = 0; i < AES_KEY_SIZE; i++) {
        clave[i] = 0;
    }
    
    for (int i = 0; i < len; i++) {
        clave[i % AES_KEY_SIZE] ^= (unsigned char)clave_str[i];
        clave[(i + 7) % AES_KEY_SIZE] ^= (unsigned char)(clave_str[i] * 3);
    }
}

void mostrar_ayuda() {
    escribir_salida("Uso: ./aes [opciones]\n\n");
    escribir_salida("Opciones:\n");
    escribir_salida("  -e              Encriptar archivo\n");
    escribir_salida("  -u              Desencriptar archivo\n");
    escribir_salida("  -i <archivo>    Archivo de entrada\n");
    escribir_salida("  -o <archivo>    Archivo de salida\n");
    escribir_salida("  -k <clave>      Clave secreta\n");
    escribir_salida("  -h              Mostrar ayuda\n\n");
    escribir_salida("Ejemplos:\n");
    escribir_salida("  ./aes -e -i documento.pdf -o doc_cifrado.aes -k MiClave\n");
    escribir_salida("  ./aes -u -i doc_cifrado.aes -o documento.pdf -k MiClave\n");
}

int main(int argc, char *argv[]) {
    int operacion = 0;
    char *archivo_entrada = 0;
    char *archivo_salida = 0;
    char *clave_str = 0;
    
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
    
    if (operacion == 0 || !archivo_entrada || !archivo_salida || !clave_str) {
        escribir_salida("Error: Faltan parametros\n\n");
        mostrar_ayuda();
        return 1;
    }
    
    unsigned char clave[AES_KEY_SIZE];
    generar_clave_aes(clave_str, clave);
    
    escribir_salida(operacion == 1 ? "Cifrando con AES-128...\n" : "Descifrando con AES-128...\n");
    
    int resultado;
    if (operacion == 1) {
        resultado = cifrar_archivo_aes(archivo_entrada, archivo_salida, clave);
    } else {
        resultado = descifrar_archivo_aes(archivo_entrada, archivo_salida, clave);
    }
    
    if (resultado == 0) {
        escribir_salida(operacion == 1 ? "Cifrado exitoso\n" : "Descifrado exitoso\n");
    } else {
        escribir_salida("Operacion fallida\n");
    }
    
    return resultado;
}