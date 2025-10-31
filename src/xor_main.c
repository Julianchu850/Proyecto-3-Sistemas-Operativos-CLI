#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "codec_xor.h"

// ======================================================
//   IMPLEMENTACIÓN DEL CIFRADO XOR DE FLUJO
// ======================================================
//
// Este algoritmo toma cada byte del archivo de entrada
// y lo combina con un byte de la clave usando la
// operación lógica XOR (⊕).
//
// Propiedad importante:
//   P ⊕ K = C   (encriptar)
//   C ⊕ K = P   (desencriptar)
//
// Por eso usamos la misma función para ambos procesos.
// ======================================================

// ---------- FUNCIÓN PRINCIPAL DE CIFRADO/DESCIFRADO ----------
int xor_core_fd(int in_fd, int out_fd, const unsigned char* key, size_t klen) {
    // Verificamos que exista una clave
    if (!key || klen == 0) {
        warnx("Clave vacía");
        return 1;
    }

    // Reservamos un búfer para leer y escribir en bloques (64 KiB)
    unsigned char* buf = (unsigned char*)malloc(CHUNK_SZ);
    if (!buf) {
        warnx("sin memoria");
        return 1;
    }

    // Variable para llevar el conteo total de bytes procesados
    // (nos sirve para alinear correctamente la clave)
    size_t off = 0;
    ssize_t n;

    // === BUCLE PRINCIPAL ===
    // 1. Leer del archivo de entrada por bloques
    while ((n = xread(in_fd, buf, CHUNK_SZ)) > 0) {
        // 2. Aplicar XOR a cada byte del bloque
        //    Se recorre el bloque byte a byte y se aplica:
        //    buf[i] = buf[i] XOR key[j]
        //    donde j = (off + i) % klen  → para repetir la clave
        for (ssize_t i = 0; i < n; i++) {
            buf[i] ^= key[(off + (size_t)i) % klen];
        }

        // 3. Escribir el bloque cifrado (o descifrado) en el archivo de salida
        if (xwrite(out_fd, buf, (size_t)n) < 0) {
            free(buf);
            return 1;
        }

        // 4. Actualizar el offset global (total de bytes procesados)
        off += (size_t)n;
    }

    // 5. Liberar el búfer antes de salir
    free(buf);

    // 6. Si n < 0, hubo error en la lectura; si no, todo fue bien
    return (n < 0) ? 1 : 0;
}

// ======================================================
//               CLI (Interfaz de línea de comandos)
// ======================================================
//
// Uso: ./bin/xor (-e|-d) -i <input> -o <output> -k <clave>
//
// -e : Encripta (usa XOR)
// -d : Desencripta (la misma operación XOR)
// ======================================================

static void print_help(const char* prog) {
    printf("Uso: %s (-e|-d) -i <input> -o <output> -k <clave>\n", prog);
    printf("  -e    Encriptar (XOR por flujo)\n");
    printf("  -d    Desencriptar (misma operación XOR)\n");
    printf("  -i    Archivo de entrada\n");
    printf("  -o    Archivo de salida\n");
    printf("  -k    Clave (no vacía)\n");
}

int main(int argc, char** argv) {
    int do_enc = 0, do_dec = 0;
    const char* in_path = NULL;
    const char* out_path = NULL;
    const char* key = NULL;

    // --- PARSEAR ARGUMENTOS ---
    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            print_help(argv[0]);
            return 0;
        } else if (!strcmp(a, "-e")) {
            do_enc = 1; // Modo encriptar
        } else if (!strcmp(a, "-d")) {
            do_dec = 1; // Modo desencriptar
        } else if (!strcmp(a, "-i")) {
            if (i + 1 >= argc) { fprintf(stderr, "Falta valor para -i\n"); return 1; }
            in_path = argv[++i];
        } else if (!strcmp(a, "-o")) {
            if (i + 1 >= argc) { fprintf(stderr, "Falta valor para -o\n"); return 1; }
            out_path = argv[++i];
        } else if (!strcmp(a, "-k")) {
            if (i + 1 >= argc) { fprintf(stderr, "Falta valor para -k\n"); return 1; }
            key = argv[++i];
        } else {
            fprintf(stderr, "Argumento desconocido: %s\n", a);
            return 1;
        }
    }

    // --- VALIDACIONES ---
    if ((do_enc ^ do_dec) == 0) {
        // ^ es XOR lógico → exige solo uno: -e o -d
        fprintf(stderr, "Debes especificar exactamente uno: -e o -d\n");
        print_help(argv[0]);
        return 1;
    }
    if (!in_path || !out_path || !key || key[0] == '\0') {
        fprintf(stderr, "-i, -o y -k son obligatorios (clave no vacía)\n");
        print_help(argv[0]);
        return 1;
    }

    // --- ABRIR ARCHIVOS CON SYSCALLS ---
    int fin  = xopen_rd(in_path);               // open(input, O_RDONLY)
    if (fin < 0) die("open input");

    int fout = xopen_wr_trunc(out_path, 0644);  // open(output, O_WRONLY|O_CREAT|O_TRUNC)
    if (fout < 0) die("open output");

    // --- PROCESAR XOR (ENCRIPTAR o DESENCRIPTAR) ---
    //   Es la misma operación: XOR(P, K) = C  y  XOR(C, K) = P
    int rc = xor_core_fd(fin, fout, (const unsigned char*)key, strlen(key));

    // --- CERRAR ARCHIVOS ---
    if (xclose(fin)  < 0) die("close input");
    if (xclose(fout) < 0) die("close output");

    // --- RESULTADO FINAL ---
    if (rc != 0) {
        warnx("Fallo en XOR");
        return 1;
    }

    // Si llegamos aquí, todo fue bien
    return 0;
}
