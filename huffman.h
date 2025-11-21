#ifndef HUFFMAN_H
#define HUFFMAN_H

// Funciones principales
int comprimir_archivo_huffman(const char *entrada, const char *salida);
int descomprimir_archivo_huffman(const char *entrada, const char *salida);
void mostrar_ayuda(void);

// Funciones auxiliares de uso general
void escribir_salida(const char *msg);
int longitud_cadena(const char *str);
int comparar_cadena(const char *s1, const char *s2);

#endif // HUFFMAN_H
