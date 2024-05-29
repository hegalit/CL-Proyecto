# Proyecto de laboratorio de CL

Esta carpeta contiene el código necesario para el proyecto de laboratorio de CL. Los detalles 
y la documentación se pueden encontrar en la página de la asignatura.
https://www.cs.upc.edu/~cl/practica


## Instrucciones:

### Compilar el proyecto:
Primero de todo, debes compilar todos los ficheros del proyecto. Para ello, ejecuta los siguientes
comandos en el directorio "asl":
```
make antlr
make
```

Esto creará, entre otros ficheros, un ejecutable llamado "asl". Este fichero te permitirá compilar
tus programas escritos en asl.



### Compilar un script en asl
Hay 2 maneras de indicar al ejecutable anterior el script a compilar:
* Indicando el fichero que contiene tu código (debe tener formato ".asl").
* Escribiéndolo en la terminal.

El comando de ejecución es el siguiente:
```
./asl [--onlySyntax | --noCodegen] [< fichero_entrada.asl] [> fichero salida.t]
```

El ejecutable generará como salida una traducción de tu programa a t-code (un código de 3 
direcciones que actúa como lenguaje intermedio), la cual podrás ejecutar como se indica más
adelante. Para ello, debes guardar la salida como un fichero en formato ".t".

El flag `--onlySyntax` detiene la ejecución del compilador antes de analizar semánticamente
el código. Esto te permite comprobar que el análisis léxico y sintáctico se realiza correctamente.

El flag `--noCodegen` detiene la ejecución del compilador antes de traducir tu script a t-code.
Esto te permite comprobar que el código es semánticamente correcto sin recibir errores 
relacionados con la traducción a t-code.


### Ejecutar un script en t-code:
Antes de ejecutar, debes haberle dado permisos de ejecución al fichero "tvm-linux" (la máquina
virtual donde podrás ejecutar tu programa traducido). El comando que debes usar es este:
```
chmod +x ./tvm/tvm-linux 
```


Dentro del directorio "tvm", el comando para ejecutar tu script en t-code es el siguiente 
(asumiendo que tu programa se encuentra en el mismo directorio):
```
./tvm-linux myprogram.t [--debug]
```

El flag `--debug` te permite ver el valor de cada variable instrucción a instrucción
durante la ejecución del código.


### Consejos y herramientas de depurado:
A veces, al recompilar el proyecto tras haber hecho cambios en clases como "TypeCheckVisitor",
"SymbolsVisitor" o "CodeGenVisitor", puede pasar que al ejecutar el compilador se produzca un
segmentation fault (desconozco la causa, solo sé que sucede a veces). En esos casos, recomiendo
ejecutar uno de estos comandos del Makefile para limpiar el proyecto:
```
make pristine
```
o bien
```
make realclean
```

Tras esto, recompila el proyecto como de costumbre y debería estar todo solucionado. Si el 
error persiste, puedes probar a usar los flags previamente enseñados para detectar si proviene 
del análisis sintáctico, del semántico, o de la generación de código.

Otra herramienta con la que cuentas es el DEBUG_BUILD. Si en el "TypeCheckVisitor", "SymbolsVisitor"
o "CodeGenVisitor" descomentas la línea `#define DEBUG_BUILD`, al ejecutar el compilador te imprimirá
información adicional para poder depurar con mayor facilidad el código.