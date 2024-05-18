# NachOS (Not Another Heuristic Operating System)

Este proyecto se lleva a cabo a lo largo del cursado de Sistemas Operativos 2 de la carrera de Licenciatura en Ciencias de la Computación de la UNR.

Consta de un sistema operativo de juguete primeramente desarrollado en la Universidad de California y reformado en la UNR.

# Funcionamiento

Al ser un sistema operativo con finalidad educativa, no se ejecuta sobre la máquina principal. En lugar de esto, simula una máquina con un procesador MIPS la cual se encuentra en la carpeta machine.

# Ejecución

Para ejecutarlo se debe hacer "make" en la carpeta de code. 

Es posible la creación de programas .c en la carpeta de userland. Para ejecutarlos se deben agregar al Makefile de dicha carpeta. Luego ir a userprog y hacer "make". Una vez hecho esto, escribir (en la carpeta userprog) ./nachos -x ../userland/nombre_archivo

