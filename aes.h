#ifndef AES_H
#define AES_H

#include <unistd.h>

// Tamaños
#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 16

// Contexto AES
typedef struct {
    unsigned char round_keys[176]; // 11 round keys de 16 bytes cada una
} AES_Context;

// Funciones de cifrado y descifrado
int cifrar_archivo_aes(const char *entrada, const char *salida, const unsigned char *clave);
int descifrar_archivo_aes(const char *entrada, const char *salida, const unsigned char *clave);

// Funciones auxiliares
void generar_clave_aes(const char *clave_str, unsigned char *clave);
void escribir_salida(const char *msg);
int longitud_cadena(const char *str);
int comparar_cadena(const char *s1, const char *s2);

#endif // AES_H
