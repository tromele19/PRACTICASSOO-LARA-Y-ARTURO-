#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//Genero los defines para tener los valores ya asignados y no tener que ir definiendolos
#define NUM_FILAS 8
#define TAM_LINEA 16
#define TAM_RAM 4096

//Genero los defines para las mascaras y tenerlas ya definidas
#define MASK_PALABRA 0xF
#define MASK_LINEA 0x7
#define MASK_ETQ 0x1F
typedef struct {
    unsigned char ETQ;
    unsigned char Data[TAM_LINEA];
}T_CACHE_LINE;

//Genero las variables locales requeridas para poder ejecutar correctamente el programa
int globaltime = 0;
int numfallos = 0;
unsigned char Simul_RAM[TAM_RAM];
char texto[100];
int texto_idx = 0;
//Prototipo de funciones
//Genero la funcion para poder limpiar la cache
void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]){
        //Genero el bucle para que recorra todas las filas de la cache
        for (int i = 0;i< NUM_FILAS;i++){
                tbl[i].ETQ = 0xFF;
                for(int j = 0; j < TAM_LINEA; j++){//Genero el bucle para que recorra los bytes de la linea
                        tbl[i].Data[j] = 0x23;
                }
        }
}
//Genero la funcion para volcar la cache
void VolcarCACHE(T_CACHE_LINE *tbl){
        printf("el volcado cache\n");
        for (int i = 0; i < NUM_FILAS; i++){//Genero el bucle para recorrer todas las lineas de la cache
                printf("Linea %d | Etiqueta %02X |",i, tbl[i].ETQ);
                for (int j = TAM_LINEA -1; j >= 0;j++){//Genero el bucle para imprimir el num de linea de mayor a menor peso
                        printf("%02X ", tbl[i].Data[j]);//El %X para el hexadecimal
                }
        }

}
//Genero la funcion para poder parsear la direccion de la memoria cache
void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque){
        addr &= 0xFFF;//Limito la direcciion a 12 bits
        *palabra = addr & MASK_PALABRA;//Extraigo los 4 bits menos significativos
        *linea = (addr >> 4) & MASK_LINEA;//Desplazo 4 bits a la izquierda y aplico la mascara
        *ETQ = (addr >> 7 )& MASK_ETQ;//Desplazo 7 bits a la derecha y vuelvo a aplicar la mascara
        *bloque = addr / TAM_LINEA;//Calculo el numero de bloque
}
//Genero la funcion para poder tratar el fallo en la cache
void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ, int linea, int bloque){
         int base = bloque * TAM_LINEA;//Calculo el offset del bloque en RAM
        for (int i = 0; i < TAM_LINEA; i++) {//Genero el bucle para recorrer los 16 bytes del bloque para copiarlo
                if (base + i < TAM_RAM)//Genero la condicion para ver que no nos hemos salido de la RAM
                tbl[linea].Data[i] = MRAM[base + i];
                else
                tbl[linea].Data[i] = 0x00;//Si se sale del rango de la RAM, lo relleno con 0x00
        }
        tbl[linea].ETQ = (unsigned char)ETQ;
}


int main (int argc, char** argv){
    T_CACHE_LINE CACHE[NUM_FILAS];
    LimpiarCACHE(CACHE);

    /* Inicializar texto */
    for (int i = 0; i < 100; i++)
        texto[i] = 0;
    texto_idx = 0;

    /* Leer RAM */
    FILE *fram = fopen("CONTENTS_RAM.bin", "rb");
    if (!fram) {
        printf("Error: no se pudo abrir CONTENTS_RAM.bin\n");
        return -1;
    }

    size_t nr = fread(Simul_RAM, 1, TAM_RAM, fram);
    fclose(fram);

    if (nr < TAM_RAM) {
        printf("Aviso: CONTENTS_RAM.bin incompleto, rellenando\n");
        for (size_t i = nr; i < TAM_RAM; i++)
            Simul_RAM[i] = 0x00;
    }

    /* Abrir fichero de accesos */
    FILE *fdirs = fopen("accesos_memoria.txt", "r");
    if (!fdirs)
        fdirs = fopen("dirs_memoria.txt", "r");

    if (!fdirs) {
        printf("Error: no se pudo abrir accesos_memoria.txt ni dirs_memoria.txt\n");
        return -1;
    }

    char linea_buf[256];
    unsigned int addr;
    int total_accesos = 0;

    globaltime = 0;
    numfallos = 0;

    /* ---------------- BUCLE PRINCIPAL ---------------- */
    while (fgets(linea_buf, sizeof(linea_buf), fdirs) != NULL) {

        char *p = linea_buf;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == '\0' || *p == '\n' || *p == '#')
            continue;

        if (sscanf(p, "%x", &addr) != 1)
            continue;

        addr &= 0x0FFF;

        int ETQ, palabra, linea_c, bloque;
        ParsearDireccion(addr, &ETQ, &palabra, &linea_c, &bloque);

        unsigned char etq_cache = CACHE[linea_c].ETQ;

        if (etq_cache == (unsigned char)ETQ) {
            /* ACIERTO */
            unsigned char dato = CACHE[linea_c].Data[palabra];

            printf("T: %d, Acierto CACHE, ADDR %04X ETQ %X linea %02X palabra %02X dato %02X\n",
                   globaltime, addr, ETQ, linea_c, palabra, dato);

            if (texto_idx < 99)
                texto[texto_idx++] = (char)dato;

        } else {
            /* FALLO */
            numfallos++;

            printf("T: %d, Fallo CACHE %d, ADDR %04X ETQ %X linea %02X palabra %02X bloque %02X\n",
                   globaltime, numfallos, addr, ETQ, linea_c, palabra, bloque);

            globaltime += 20;

            printf("Cargando bloque %02X en linea %02X\n", bloque, linea_c);
            TratarFallo(CACHE, (char*)Simul_RAM, ETQ, linea_c, bloque);

            VolcarCACHE(CACHE);

            unsigned char dato = CACHE[linea_c].Data[palabra];

            printf("T: %d, (Tras carga) Acierto CACHE, ADDR %04X ETQ %X linea %02X palabra %02X dato %02X\n",
                   globaltime, addr, ETQ, linea_c, palabra, dato);

            if (texto_idx < 99)
                texto[texto_idx++] = (char)dato;
        }

        VolcarCACHE(CACHE);

        total_accesos++;
        sleep(1);
    }

    fclose(fdirs);

    /* ---------------- ESTADÍSTICAS ---------------- */
    double tmedio = (total_accesos > 0) ? (double)globaltime / total_accesos : 0;

    printf("\n=== ESTADISTICAS ===\n");
    printf("Accesos: %d\n", total_accesos);
    printf("Fallos: %d\n", numfallos);
    printf("Tiempo total: %d\n", globaltime);
    printf("Tiempo medio: %.2f\n", tmedio);
    printf("Texto leído: %s\n", texto);
    printf("====================\n");

    /* ---------------- VOLCADO BINARIO FINAL ---------------- */
    FILE *fc = fopen("CONTENTS_CACHE.bin", "wb");
    if (!fc) {
        printf("Error: no se pudo crear CONTENTS_CACHE.bin\n");
        return -1;
    }
    for (int l = 0; l < NUM_FILAS; l++)
        for (int b = 0; b < TAM_LINEA; b++)
            fwrite(&CACHE[l].Data[b], 1, 1, fc);

	    fclose(fc);
	return 0;
}
