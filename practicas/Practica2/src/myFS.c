#include "myFS.h"
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



// Formatea el disco virtual. Guarda el mapa de bits del super bloque y el directorio único.
int myMkfs(MiSistemaDeFicheros* miSistemaDeFicheros, int tamDisco, char* nombreArchivo) {
	// Creamos el disco virtual:
	miSistemaDeFicheros->fdDiscoVirtual = open(nombreArchivo, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	// Algunas comprobaciones mínimas:
	assert(sizeof (EstructuraSuperBloque) <= TAM_BLOQUE_BYTES);
	assert(sizeof (EstructuraDirectorio) <= TAM_BLOQUE_BYTES);
	int numBloques = tamDisco / TAM_BLOQUE_BYTES;
	int minNumBloques = 3 + MAX_BLOQUES_CON_NODOSI + 1;
	int maxNumBloques = NUM_BITS;
	if (numBloques < minNumBloques) {
		return 1;
	}
	if (numBloques >= maxNumBloques) {
		return 2;
	}

	/// MAPA DE BITS
	// Inicializamos el mapa de bits
	int i;
	for (i = 0; i < NUM_BITS; i++) {
		miSistemaDeFicheros->mapaDeBits[i] = 0;
	}

	// Los primeros tres bloques tendrán el superbloque, mapa de bits y directorio
	miSistemaDeFicheros->mapaDeBits[MAPA_BITS_IDX] = 1;
	miSistemaDeFicheros->mapaDeBits[SUPERBLOQUE_IDX] = 1;
	miSistemaDeFicheros->mapaDeBits[DIRECTORIO_IDX] = 1;

	// Los siguientes NUM_INODE_BLOCKS contendrán nodos-i
	for (i = 3; i < 3 + MAX_BLOQUES_CON_NODOSI; i++) {
		miSistemaDeFicheros->mapaDeBits[i] = 1;
	}
	escribeMapaDeBits(miSistemaDeFicheros);

	/// DIRECTORIO
	// Inicializamos el directorio
	miSistemaDeFicheros->directorio.numArchivos = 0;
	for (i = 0; i < MAX_ARCHIVOS_POR_DIRECTORIO; i++) {
		miSistemaDeFicheros->directorio.archivos[i].libre = 1;
	}
	escribeDirectorio(miSistemaDeFicheros);

	/// NODOS-I
	EstructuraNodoI nodoActual;
	nodoActual.libre = 1;
	for (i = 0; i < MAX_NODOSI; i++) {
		escribeNodoI(miSistemaDeFicheros, i, &nodoActual);
	}

	/// SUPERBLOQUE
	initSuperBloque(miSistemaDeFicheros, tamDisco);
	escribeSuperBloque(miSistemaDeFicheros);
	sync();

	// Al finalizar tenemos al menos un bloque
	assert(myQuota(miSistemaDeFicheros) >= 1);

	initNodosI(miSistemaDeFicheros);
	
	printf("SF: %s, %d B (%d B/bloque), %d bloques\n", nombreArchivo, tamDisco, TAM_BLOQUE_BYTES, numBloques);
	printf("1 bloque para SUPERBLOQUE (%u B)\n", (unsigned int)sizeof(EstructuraSuperBloque));
	printf("1 bloque para MAPA DE BITS, que cubre %u bloques, %u B\n", (unsigned int)NUM_BITS, (unsigned int)(NUM_BITS * TAM_BLOQUE_BYTES));
	printf("1 bloque para DIRECTORIO (%u B)\n", (unsigned int)sizeof(EstructuraDirectorio));
	printf("%d bloques para nodos-i (a %u B/nodo-i, %u nodos-i)\n",MAX_BLOQUES_CON_NODOSI,(unsigned int)sizeof(EstructuraNodoI),(unsigned int)MAX_NODOSI);
	printf("%d bloques para datos (%d B)\n",miSistemaDeFicheros->superBloque.numBloquesLibres,TAM_BLOQUE_BYTES*miSistemaDeFicheros->superBloque.numBloquesLibres);
	printf("¡Formato completado!\n");
	
	return 0;
}

/* Inicializa el superbloque */
void initSuperBloque(MiSistemaDeFicheros* miSistemaDeFicheros, int tamDisco) {
    miSistemaDeFicheros->superBloque.tamDiscoEnBloques = tamDisco / TAM_BLOQUE_BYTES;
    miSistemaDeFicheros->superBloque.numBloquesLibres = myQuota(miSistemaDeFicheros);

	miSistemaDeFicheros->superBloque.fechaCreacion=time(NULL);

    miSistemaDeFicheros->superBloque.tamBloque = TAM_BLOQUE_BYTES;
    miSistemaDeFicheros->superBloque.maxTamNombreArchivo = MAX_TAM_NOMBRE_ARCHIVO;
    miSistemaDeFicheros->superBloque.maxBloquesPorArchivo = MAX_BLOQUES_POR_ARCHIVO;
}


//Liberamos la memoria del sistema de ficheros y lo cerramos.
void myFree(MiSistemaDeFicheros* miSistemaDeFicheros) {
	int i;
    close(miSistemaDeFicheros->fdDiscoVirtual);
    for(i=0; i<MAX_NODOSI; i++) {
    	free(miSistemaDeFicheros->nodosI[i]);
    	miSistemaDeFicheros->nodosI[i] = NULL;
    }
}

/*	Escribe el mapa de bits en el sistema de ficheros	*/

int escribeMapaDeBits(MiSistemaDeFicheros* miSistemaDeFicheros) {
	/*Colocamos el puntero sobre el mapa de bits
	TAM_BLOQUE_BYTES vale 4096	y 	MAPA_BITS_IDX vale 1*/

    if (lseek(miSistemaDeFicheros->fdDiscoVirtual, TAM_BLOQUE_BYTES * MAPA_BITS_IDX, SEEK_SET) == (off_t) - 1) {
        perror("Falló lseek en escribeMapaDeBits");
        return -1;
    }
	/*Escribimos el mapaDeBits actual sobre el sistema de ficheros*/

    if (write(miSistemaDeFicheros->fdDiscoVirtual,
            miSistemaDeFicheros->mapaDeBits, sizeof (BIT) * NUM_BITS) == -1) {
        perror("Falló write en escribeMapaDeBits");
        return -1;
    }
    return 0;
}

/*Escribe el nodo que pasamos(nodoI) en el número de nodo indicado(numNodoI) sobre el sistema de ficheros*/

int escribeNodoI(MiSistemaDeFicheros* miSistemaDeFicheros, int numNodoI, EstructuraNodoI* nodoI) {
    int posNodoI;
	//Comprobamos que es un nodo dentro del rango permitido
    assert(numNodoI < MAX_NODOSI);
	
	//Calculamos la posición global que le corresponde al i-nodo en el sistema
    posNodoI = calculaPosNodoI(numNodoI);

	//Nos colocamos en la posición que corresponde al nodoI
    if (lseek(miSistemaDeFicheros->fdDiscoVirtual, posNodoI, SEEK_SET) == (off_t) - 1) {
        perror("Falló lseek en escribeNodoI");
        return -1;
    }
	//Escribimos nuestro nodoI en el disco
    if (write(miSistemaDeFicheros->fdDiscoVirtual, nodoI, sizeof (EstructuraNodoI)) == -1) {
        perror("Falló write en escribeNodoI");
    }
    sync();
    return 0;
}

int escribeSuperBloque(MiSistemaDeFicheros* miSistemaDeFicheros) {
	//Colocamos el puntero sobre el super bloque(Posición 0 del sistema).
    if (lseek(miSistemaDeFicheros->fdDiscoVirtual, TAM_BLOQUE_BYTES * SUPERBLOQUE_IDX, SEEK_SET) == (off_t) - 1) {
        perror("Falló lseek en escribeSuperBloque");
        return -1;
    }
	//Escribimos nuestro superbloque en el sistema
    if (write(miSistemaDeFicheros->fdDiscoVirtual, &(miSistemaDeFicheros->superBloque), sizeof (EstructuraSuperBloque)) == -1) {
        perror("Falló write en escribeSuperBloque");
        return -1;
    }
    return 0;
}

/*	Actualizamos los directorios en el disco	*/
int escribeDirectorio(MiSistemaDeFicheros* miSistemaDeFicheros) {
    if (lseek(miSistemaDeFicheros->fdDiscoVirtual, TAM_BLOQUE_BYTES * DIRECTORIO_IDX, SEEK_SET) == (off_t) - 1) {
        perror("Falló lseek en escribeDirectorio");
        return -1;
    }
    if (write(miSistemaDeFicheros->fdDiscoVirtual, &(miSistemaDeFicheros->directorio), sizeof (EstructuraDirectorio)) == -1) {
        perror("Falló write en escribeDirectorio");
        return -1;
    }
    return 0;
}

/*Dado un número de nodo, nos devuelve la posición en nuestro sistema del nodo*/

int calculaPosNodoI(int numNodoI) {
    int whichInodeBlock;
    int whichInodeInBlock;
    int inodeLocation;

	//Primero calculamos su bloque
    whichInodeBlock = numNodoI / NODOSI_POR_BLOQUE;
	//Calculamos su posición en el bloque
    whichInodeInBlock = numNodoI % NODOSI_POR_BLOQUE;
	//Calculamos su posición global en el sistema
    inodeLocation = (NODOI_IDX + whichInodeBlock) * TAM_BLOQUE_BYTES + whichInodeInBlock * sizeof (EstructuraNodoI);
    return inodeLocation;
}

/*	Leemos los i-nodos del disco y los guardamos en la estructura de nodosI */
void initNodosI(MiSistemaDeFicheros* miSistemaDeFicheros) {
    int numNodoI;
    EstructuraNodoI* temp = malloc(sizeof (EstructuraNodoI));
	
    for (numNodoI = 0; numNodoI < MAX_NODOSI; numNodoI++) {
        leeNodoI(miSistemaDeFicheros, numNodoI, temp);
	//Tenemos el nodoI de la posición numNodoI
        if (temp->libre) {
            miSistemaDeFicheros->nodosI[numNodoI] = NULL;
        } 
	//Si está vacío ponemos un NULL. Si hay un i-nodo nos lo traemos y actualizamos el contador.
	else {
            miSistemaDeFicheros->numNodosLibres--;
            miSistemaDeFicheros->nodosI[numNodoI] = malloc(sizeof (EstructuraNodoI));
            copiaNodoI(miSistemaDeFicheros->nodosI[numNodoI], temp);
        }
    }
}

/*	Leemos el nodo del disco  de la posición numNodoI y lo guardamos en nodoI */
int leeNodoI(MiSistemaDeFicheros* miSistemaDeFicheros, int numNodoI, EstructuraNodoI* nodoI) {
    int posNodoI;
    assert(numNodoI < MAX_NODOSI);
    posNodoI = calculaPosNodoI(numNodoI);

    lseek(miSistemaDeFicheros->fdDiscoVirtual, posNodoI, SEEK_SET);
    read(miSistemaDeFicheros->fdDiscoVirtual, nodoI, sizeof (EstructuraNodoI));
    return 0;
}

/*Copia los datos del nodo src en el nodo dest */
void copiaNodoI(EstructuraNodoI* dest, EstructuraNodoI* src) {
    int i;

    dest->numBloques = src->numBloques;
    dest->tamArchivo = src->tamArchivo;
    dest->tiempoModificado = src->tiempoModificado;
    dest->libre = src->libre;

    for (i = 0; i < MAX_BLOQUES_POR_ARCHIVO; i++)
        dest->idxBloques[i] = src->idxBloques[i];
}

//Busca el primer i-nodo que está vacío 
int buscaNodoLibre(MiSistemaDeFicheros* miSistemaDeFicheros) {
    int i;
    for (i = 0; i < MAX_NODOSI; i++) {
        if (miSistemaDeFicheros->nodosI[i] == NULL)
            return i;
    }
    return -1; // NO hay nodos-i libres. Esto no debería ocurrir.
}

//Busca un directorio que se llame nombre y devuelve su número de posición relativa.
int buscaPosDirectorio(MiSistemaDeFicheros* miSistemaDeFicheros, char* nombre) {
    int i;

    for (i = 0; i < MAX_ARCHIVOS_POR_DIRECTORIO; i++) {
        if (miSistemaDeFicheros->directorio.archivos[i].libre == false) {
            if (strcmp(nombre, miSistemaDeFicheros->directorio.archivos[i].nombreArchivo) == 0)
                return i;
        }
    }
    return -1;
}


// Devuelve el numero de bloques libres en el FS.
int myQuota(MiSistemaDeFicheros* miSistemaDeFicheros) {
    int freeCount = 0;
    int i;
    // Calculamos el número de bloques libres
    for (i = 0; i < miSistemaDeFicheros->superBloque.tamDiscoEnBloques; i++) {
        // Ahora estamos usando uint para representar cada bit.
        // Podríamos usar todos los bits para mejorar el almacenamiento
        if (miSistemaDeFicheros->mapaDeBits[i] == 0) {
            freeCount++;
        }
    }
    return freeCount;
}

//ISK_LBA es un int, idxBloques un array de int.

void reservaBloquesNodosI(MiSistemaDeFicheros* miSistemaDeFicheros, DISK_LBA idxBloques[], int numBloques) {
    int i = 0;
    int bloqueActual = 0;
	//Hasta que nos quedemos sin bloques libres o reservemos los bloques que queremos(numBloques).
    while ((bloqueActual < numBloques) && (i < NUM_BITS)) {
	//Reservamos espacio para numBloques de bloques para i-nodos
	//Y los marcamos como ocupados. Deben ser bloques libres
        if (miSistemaDeFicheros->mapaDeBits[i] == 0) {
            miSistemaDeFicheros->mapaDeBits[i] = 1;
            idxBloques[bloqueActual] = i;
            bloqueActual++;
        }
        ++i;
    }
}
