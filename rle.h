#ifndef RLE_H
#define RLE_H

#include <stddef.h>

// Tamaños de buffer
#define RLE_BUFFER_IN 4096
#define RLE_BUFFER_OUT 8192
#define RLE_MAX_COUNT 255

/**
 * comprimir_rle - Comprime datos usando Run-Length Encoding
 * @in_buf: Buffer de entrada con datos sin comprimir
 * @in_size: Tamaño del buffer de entrada en bytes
 * @out_buf: Buffer de salida (debe tener tamaño >= in_size * 2)
 * 
 * Retorna: Tamaño del buffer comprimido en bytes
 * 
 * Descripción: Codifica secuencias repetidas de bytes como pares (count, byte)
 * Ejemplo: "AAAABBC" -> [0x04, 'A', 0x02, 'B', 0x01, 'C']
 */
int comprimir_rle(const unsigned char *in_buf, int in_size, unsigned char *out_buf);

/**
 * descomprimir_rle - Descomprime datos comprimidos con RLE
 * @in_buf: Buffer de entrada con datos comprimidos
 * @in_size: Tamaño del buffer de entrada en bytes
 * @out_buf: Buffer de salida (debe tener tamaño >= RLE_BUFFER_OUT)
 * 
 * Retorna: Tamaño del buffer descomprimido en bytes
 * 
 * Descripción: Decodifica pares (count, byte) a secuencias originales
 * Ejemplo: [0x04, 'A', 0x02, 'B', 0x01, 'C'] -> "AAAABBC"
 */
int descomprimir_rle(unsigned char *in_buf, int in_size, unsigned char *out_buf);

#endif // RLE_H