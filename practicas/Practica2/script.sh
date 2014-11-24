#!/bin/bash

MPOINT="./punto-montaje"

rm -R -f copiasTemporales
mkdir copiasTemporales

cp ./src/fuseLib.c ./copiasTemporales/f1.txt
cp ./src/fuseLib.h ./copiasTemporales/f2.txt
cp ./src/myFS.c ./copiasTemporales/f3.txt


#a) Copie dos ficheros de texto que ocupen más de un boque (por ejemplo //fuseLib.c y myFS.h) a nuestro SF y a un directorio temporal, por ejemplo ./copiasTemporales

echo "Copiando fichero 1 y 2"
cp ./copiasTemporales/f1.txt $MPOINT/
cp ./copiasTemporales/f2.txt $MPOINT/
read -p "Press enter..."

#b) Audite el disco y haga un diff entre los ficheros originales y los copiados en el SF
#./audita disco-virtual
echo "Diferenciando fichero 1 y 2"
diff ./copiasTemporales/f1.txt $MPOINT/f1.txt && diff ./copiasTemporales/f2.txt $MPOINT/f2.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#Trunque el primer fichero (man truncate) en copiasTemporales y en nuestro SF de manera que ocupe ocupe un bloque de datos menos.
echo "Truncando fichero 1 -5000"
truncate ./copiasTemporales/f1.txt -s-5000
truncate $MPOINT/f1.txt -s-5000
read -p "Press enter..."

#c) Audite el disco y haga un diff entre el fichero original y el truncado.
#./audita disco-virtual
echo "Diferenciando fichero 1 truncado"
diff ./copiasTemporales/f1.txt $MPOINT/f1.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#d) Copie un tercer fichero de texto a nuestro SF.
echo "Copiando fichero 3"
cp ./copiasTemporales/f3.txt $MPOINT/
read -p "Press enter..."

#e) Audite el disco y haga un diff entre el fichero original y el copiado en el SF
#./audita disco-virtual
echo "Diferenciando fichero 3"
diff ./copiasTemporales/f3.txt $MPOINT/f3.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

#f) Trunque el segundo fichero en copiasTemporales y en nuestro SF haciendo que ocupe algún bloque de datos más.
echo "Truncando fichero 2 +1000"
truncate ./copiasTemporales/f2.txt -s+10000
truncate $MPOINT/f2.txt -s+10000
read -p "Press enter..."

#g) Audite el disco y haga un diff entre el fichero original y el truncado
#./audita disco-virtual
echo "Diferenciando fichero 2 truncado"
diff ./copiasTemporales/f2.txt $MPOINT/f1.txt
OUT=$?
if [ $OUT -eq 0 ]; then
read -p "Correcto. Press enter..."
else 
echo "Error. Los archivos no son iguales."
fi

