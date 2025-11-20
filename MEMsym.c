#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//Genero los defines para tener los valores ya asignados y no tener que ir definiendolos
#define NUM_FILAS 8
#define TAM_LINEA 16
#define TAM_RAM 409

typedef struct {
    unsigned char ETQ;
    unsigned char Data[TAM_LINEA];
}T_CACHE_LINE;


//Prototipo de funciones 
void LimpiarCACHE(T_CACHE_LINE tbl[NUM_FILAS]);//Parte Arturo
void VolcarCACHE(T_CACHE_LINE *tbl);//Parte Lara
void ParsearDireccion(unsigned int addr, int *ETQ, int *palabra, int *linea, int *bloque);//Parte Arturo
void TratarFallo(T_CACHE_LINE *tbl, char *MRAM, int ETQ, int linea, int bloque);//Parte Lara

int main (int argc, char** argv){

        return 0;
}

