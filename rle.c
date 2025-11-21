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