### ¿Por qué se prefiere emular una CPU en vez de utilizar directamente la CPU existente?

La preferecia viene del hecho que trabajar con la CPU de nuestra computadora
requiere el manejo de Hadware, que puede variar dependiendo de la computadora de
cada uno. Al tener una emulasión, el hardware pasa a ser un objeto en nuestro có
digo, haciendolo mas accesible a nuestro uso y permite utilizar herramientas
de nuestro sistema operativo host, como gdb o valgrind para debuguear.

Un ejemplo claro de esto, es que orginalmente Nachos corria en un procesador
MICS, que no se utiliza más actualmente, pero podemos emularlo.

Por otro lado, tenemos determinismo a la hora de ejecutar cada una de nuestras 
acciones. Esto permite tener acceso al estado completo de la máquina en cada 
momento y el SO Host no influye en el mismo. Por ejemplo, podemos acceder a las 
direcciones físicas de la máquina, el estado de los registros y podemos realizar
modificaciones sobre cada uno de estos.

### ¿Cuánta memoria tiene la máquina simulada para Nachos?

El tamaño de páginas de la máquina es de 128-bytes, y debido a que el valor por 
DEFAULT de cantidad de páginas es 32, tendremos un total de 
128 x 32 = 3968-bytes.

### ¿Qué modificaría para cambiar la cantidad de memoria?

Para cambiar la cantidad de memoria basta con modificar la cantidad de páginas
de la máquina. Esto se logra mediante la bandera -m y el número de páginas que 
queremos establecer.

### ¿De qué tamaño es el dico ?

El tamaño del disco se calcula como 
DISK\_SIZE = MAGIC\_SIZE + NUM\_SECTORS * SECTOR\_SIZE
donde
NUM\_SECTORS = SECTORS\_PER\_TRACK * NUM\_TRACKS = 32 * 32
Por lo tanto,
DISK\_SIZE = 4 + 32 * 32 + 128 = 131076 bytes

### ¿Cuántas instrucciones de MIPS simula la máquina virtual de Nachos?

En el archivo encodin.hh de la carpeta machine podemos encontras las 59
operaciones que puede hacer el procesador. 

### ¿En qué archivos está definida la función *main*? ¿En qué archivo está definida la función *main* del ejecutable *nachos* del directorio *userprog*? 

Las carpetas en las cuales se encuantra al menos una función *main* son las
siguientes: 
  * threads
  * bin
  * userland (tierra del usuario)

La función main que se encuentra en threads/main.cc es la misma que se utiliza
en las otras carpetas:
  * userprog
  * vmem
  * filesys
donde cada unos de los ejecutables tiene una versión propia del nachos, ya que
userprog hereda las funcionalidades de threads, vmem las de userprog y filesys
las de vmem.

###Nombre los archivos fuente en los que figuran las funciones y métodos llamados por el main de Nachos al ejecutarlo en el directorio threads, hasta dos niveles de profundidad.

En la función **main** se ejecutan las siguientes funciones:
  - Initialize -> system.cc
    * ASSERT -> assert.hh
    * strcmp -> string.h
    * ParseDebugOpts -> system.cc
    * RandomInit -> system\_dep.cc
    * atoi -> stdlib.h
    * SetFlag -> debug.cc
    * SetOpts -> debug.cc
    * Timer -> timer.cc
    * Thread -> thread.cc
    * SetStatus -> thread.cc
    * Enable -> interrupt.cc
    * CallOnUseAbort -> system\_dep.cc
    * Machine -> machine.cc
    * SetExceptionHandler -> exception.cc
    * SynchDisk -> synch\_disk.cc
    * FileSystem -> file\_systme.cc
  - strcmp -> string.h 
  - DEBUG -> utility.hh
    * Print -> debug.cc
  - SysInfo -> sys\_info.cc
    * printf -> stdio2.h 
  - PrintVersion -> main.cc
    * printf
  - ThreadTest -> thread\_test.cc
    * DEBUG -> utility.hh
    * Choose -> thread\_test.cc
    * Run -> thread\_test.cc
  - Halt -> interrupt.cc
    * printf -> stdio2.hh
    * Print -> statistics.cc
    * CleanUp -> System.cc
  - StartProcess -> prog\_test.cc
    * ASSERT -> assert.hh
    * Open -> file\_system.hh
    * printf -> stdio2.h
    * AddressSpace -> address\_space.cc
    * InitRegisters -> address\_space.cc
    * RestoreSpace -> address\_space.cc
    * Run -> debugger\_command\_manager.cc
  - ConsoleTest -> prog\_test.cc
    * Consol -> console.cc
    * Semaphore -> semaphore.cc
    * P -> semaphore.cc
    * GetChar -> console.cc
    * PutChar -> console.cc
  - Copy -> fs\_test.cc
    * fopen -> stdio.h
    * printf -> stdio2.h
    * fseek -> stdio.h
    * ftell -> stdio.h
    * Create -> file\_system.hh
    * fclose -> stdio.h
    * Open -> file\_system.hh
    * fread -> stdio2.h
    * Write -> open\_file.cc
  - Print -> scheduler.cc
    * printf -> stdio2.h
    * Apply -> list.hh 
    * threadPrint -> scheduler.cc
  - Remove -> directory.cc
    * FinIndex -> directory.cc
  - List -> directory.cc
    * printf -> stdio2.h
  - Check -> file\_system.cc
    * DEBUG -> utility.hh
    * Bitmap -> bitmap.cc
    * Mark -> bitmap.cc
    * GetRaw -> file\_header.cc
    * FetchFrom -> file\_header.cc
    * CheckForError -> file\_sytem.cc
    * CheckFileHeader -> file\_system.cc
    * Directory -> directory.cc
    * CheckDirectory -> file\_sytem.cc
    * CheckBitmaps -> file\_sytem.cc
  - PerformanceTest -> fs\_test.cc
    * printf -> stdio2.h
    * Printf -> statistics.cc
    * FileWrite -> fs\_test.cc
    * FileRead -> fs\_test.cc
    * Remove -> file\_sytem.cc
  - Finish -> thread.cc
    * SetLevel -> interrupt.cc
    * ASSERT -> assert.hh
    * DEBUG -> utility.hh
    * GetName -> thread.cc
    * Sleep -> thread.cc

### ¿Qué efecto hacen las macros ASSERT y DEBUG definidas en lib/utility.hh? 
* **ASSERT**: llama a la funcion Assert definida en assert.cc. Esta evalua la condición que se le pasa por parametro, si la condición es falsa se llama a la función fpreintf con el file descriptor stderr para mostrar en pantalla un mensaje con un mensaje de error, en qué archivo sucedió y en qué linea. Posteriormente se limpia la salida con fflush y se llama a la función abort de stdlib.h para finalizar la ejecución y generar un core-dump
* **DEBUG**: llama a la función Print del objeto debug para mostrar un mensaje dependiendo de las opciones pasadas como parametro. A esta función se la utiliza para chequear el correcto comportamiento de las funciones.

### Comente el efecto de las distintas banderas de depuración. 

Las rutinas de depuración nos permiten activar mesajes de depuración que se con
trolan mediante las siguientes banderas.

  * `+` -> Activa todos los mesajes de depuración.
  * `t` -> Sistema de threads.
  * `s` -> Semaforos, locks y condiciones. 
  * `i` -> Emulasión de interrupciones.
  * `m` -> Emulasión de la máquina. (Requiere **USER_PROGRAM**)
  * `d` -> Emulasión del disco. (Requiere **FILESYS**)
  * `f` -> Sistema de archivos. (Requiere **FILESYS**)
  * `a` -> Espacio de direcciones. (Requiere **USER_PROGRAM**)
  * `e` -> Manejo de excepciones. (Requiere **USER_PROGRAM**) 

### ¿Dónde están definidas las constantes **USER_PRGRAM**, **FILESYS_NEEDED** y **FILESYS_STUB**?

Estas contantes se encuentran definidas en los archivos MakeFile

### ¿Qué argumentos de línea de comandos admite Nachos? ¿Qué efecto tiene la opción **-rs**?

### Modifique el caso de prueba simple del directorio threads para que se generen 5 hilos en lugar de 2.

```C++
bool threadsDone[4] = {false};

void
SimpleThread(void *name_)
{

    for (unsigned num = 0; num < 10; num++) {
        printf("*** Thread `%s` is running: iteration %u\n", currentThread->GetName(), num);
        currentThread->Yield();
    }

    int i = currentThread->GetName()[0] - '2';

    if (i != ('m' - '2')) {
        threadsDone[i] = true; // Si
    }

    printf("!!! Thread `%s` has finished SimpleThread\n", currentThread->GetName());
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{
    const char* t[4] = {"2nd", "3rd", "4th", "5th"};
    for (int i = 0; i < 4; i++){
        Thread *newThread = new Thread(t[i]);
        newThread->Fork(SimpleThread, NULL);
    }
    
    //the "main" thread also executes the same function
    SimpleThread(NULL);

   //Wait for the 2nd thread to finish if needed
    while (!(threadsDone[0] && 
             threadsDone[1] &&
             threadsDone[2] &&
             threadsDone[3])){
        currentThread->Yield(); 
    }
    printf("Test finished\n");
}
```
