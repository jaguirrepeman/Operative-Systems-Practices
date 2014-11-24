
/* Leemos de path. Guardamos en buf la cantidad que indica size, a partir de offset*/

static int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	char buffer[TAM_BLOQUE_BYTES];
	//En principio leeremos los bytes que indica size
	int bytes2Read=size, totalRead=0;
	EstructuraNodoI *nodoI = miSistemaDeFicheros.nodosI[fi->fh];
	fprintf(stderr, "--->>>my_read: path %s, size %zu, offset %jd, fh %"PRIu64"\n", path, size, (intmax_t)offset, fi->fh);

	int tamArchivo = nodoI->tamArchivo;

	//Si el archivo no tiene tamaño como para leer size, leemos hasta el final del archivo
	if ((tamArchivo - offset) < size)
		bytes2Read = tamArchivo - offset;

	//Leemos los datos
	while(bytes2Read > totalRead){
		int i;
		int bloqueActual, offBloque;
		//Nos ponemos en el bloque que indica offset
		bloqueActual = nodoI->idxBloques[offset / TAM_BLOQUE_BYTES];
		//Nos colocamos en la posicion dentro del bloque
		offBloque = offset % TAM_BLOQUE_BYTES;

		

		if ( (lseek(miSistemaDeFicheros.fdDiscoVirtual, bloqueActual * TAM_BLOQUE_BYTES, SEEK_SET) == (off_t) - 1) ||
			 (read(miSistemaDeFicheros.fdDiscoVirtual, &buffer, TAM_BLOQUE_BYTES) == -1) ){
			perror("Falló lseek/read en my_raed");
			return -EIO;
		}


		//Leemos lo que podamos en el bloque en que estamos
		for(i=offBloque; (i<TAM_BLOQUE_BYTES) && (totalRead<bytes2Read); i++){
			buf[totalRead++]=buffer[i];
		}
		
		//Pasamos a leer al siguiente bloque.
		offset+=i;
	}


	//Devolvemos lo que hemos leído
	return totalRead;
}

/*
 * Borramos el fichero indicado en path
*/
static int my_unlink(const char *path) {
	int idxNodoI;

	fprintf(stderr, "--->>>my_unlink: path %s", path);
	
	//Como en my_release, obtenemos el NodoI correspondiente
	if( (idxNodoI=buscaPosDirectorio(&miSistemaDeFicheros, (char*)path+1)) == -1){
		return -ENOENT;
	}
	//Modificamos el tamaño del fichero. Como borramos lo ponemos a 0.
	//ResizeInodo liberará el espacio que sea necesario.
	if(resizeInodo(idxNodoI, 0)<0)
		return -EIO;
	
	/*	Nos actualiza el nodo
	nodoI->tiempoModificado = time(NULL);

	/// Guardamos en disco el contenido de las estructuras modificadas
	escribeSuperBloque(&miSistemaDeFicheros);
	escribeMapaDeBits(&miSistemaDeFicheros);
	escribeNodoI(&miSistemaDeFicheros, idxNodoI, nodoI);	*/

	///Ahora borramos el i-nodo
	miSistemaDeFicheros.nodosI[idxNodoI]->libre = true;
	miSistemaDeFicheros.numNodosLibres++;
	

	//Liberamos el archivo
	miSistemaDeFicheros.directorio.archivos[idxNodoI].libre = true;
	miSistemaDeFicheros.directorio.numArchivos--;
	
	//Actualizamos en el sistema el directorio y el nodoI.
	
	escribeDirectorio(&miSistemaDeFicheros);
	escribeNodoI(&miSistemaDeFicheros, idxNodoI, miSistemaDeFicheros.nodosI[idxNodoI]);
	
	//Liberamos la memoria del heap del nodoI
	free(miSistemaDeFicheros.nodosI[idxNodoI]);
	//Lo ponemos a NULL para después de liberar la memoria para proteger de futuros accesos
	miSistemaDeFicheros.nodosI[idxNodoI]=NULL;
	sync();	
	return 0;
}
