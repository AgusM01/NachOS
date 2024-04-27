/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

// README !!!!!!!!!!
// Casos interesantes con seeds
//  1. seed 2123
//     Con esta seed, ya habiendo implementado el scheduler con 
//     multicolas de prioridad y el switch de prioridades en 
//     locks, ocurre que cuando un thread pide el lock del 
//     channel y espera un receiver en un semáfororo. Cuando otro
//     sender de mayor prioridad pide el lock del channel, el thread
//     con menor prioridad que está esperando recibir el mensaje está
//     en estado BLOCKED, por lo cual uno no debe re-schedulearlo a la
//     hora de evitar la inversión de prioridad.

#ifndef NACHOS_THREADS_THREADTESTCHANNEL__HH
#define NACHOS_THREADS_THREADTESTCHANNEL__HH


void ThreadTestChannel();


#endif
