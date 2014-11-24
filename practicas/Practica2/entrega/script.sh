#!/bin/bash

MPOINT="./punto-montaje"

rm -R -f copiasTemporales
mkdir copiasTemporales

cp ./src/fuseLib.c ./copiasTemporales/fichero1.txt
cp ./src/fuseLib.h ./copiasTemporales/fichero2.txt
cp ./src/myFS.c ./copiasTemporales/fichero3.txt


#a) Copie dos ficheros de texto que ocupen más de un boque (por ejemplo //fuseLib.c y myFS.h) a nuestro SF y a un directorio temporal, por ejemplo ./copiasTemporales

echo "Copiando fichero 1 y 2"
cp ./copiasTemporales/fichero1.txt $MPOINT/
cp ./copiasTemporales/fichero2.txt $MPOINT/
read -p "Press enter..."

#b) Audite el disco y haga un diff entre los ficheros originales y los copiados en el SF
#./audita disco-virtual
echo "Diferenciando fichero 1 y 2"
diff ./copiasTemporales/fichero1.txt $MPOINT/fichero1.txt && diff ./copiasTemporales/fichero2.txt $MPOINT/fichero2.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#Trunque el primer fichero (man truncate) en copiasTemporales y en nuestro SF de manera que ocupe ocupe un bloque de datos menos.
echo "Truncando fichero 1"
truncate ./copiasTemporales/fichero1.txt -s-5000
truncate $MPOINT/fichero1.txt -s-5000
read -p "Press enter..."

#c) Audite el disco y haga un diff entre el fichero original y el truncado.
#./audita disco-virtual
echo "Diferenciando fichero 1 truncado"
diff ./copiasTemporales/fichero1.txt $MPOINT/fichero1.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#d) Copie un tercer fichero de texto a nuestro SF.
echo "Copiando fichero 3"
cp ./copiasTemporales/fichero3.txt $MPOINT/
read -p "Press enter..."

#e) Audite el disco y haga un diff entre el fichero original y el copiado en el SF
#./audita disco-virtual
echo "Diferenciando fichero 3"
diff ./copiasTemporales/fichero3.txt $MPOINT/fichero3.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#f) Trunque el segundo fichero en copiasTemporales y en nuestro SF haciendo que ocupe algún bloque de datos más.
echo "Truncando fichero 2"
truncate ./copiasTemporales/fichero2.txt -s+10000
truncate $MPOINT/fichero2.txt -s+10000
read -p "Press enter..."

#g) Audite el disco y haga un diff entre el fichero original y el truncado
#./audita disco-virtual
echo "Diferenciando fichero 2 truncado"
diff ./copiasTemporales/fichero2.txt $MPOINT/fichero1.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

