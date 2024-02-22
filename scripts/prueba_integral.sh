#!/bin/bash
[[ ${IP_FS} ]] || export IP_FS='127.0.0.1'
[[ ${IP_MEMORIA} ]] || export IP_MEMORIA='127.0.0.1'
[[ ${IP_CPU} ]] || export IP_CPU='127.0.0.1'

echo "Prueba Integral"
echo .

echo "Escribo las configs de CPU."
cat << EOF > cpu/cfg/cpu.config
IP_MEMORIA=${IP_MEMORIA}
PUERTO_MEMORIA=8002
PUERTO_ESCUCHA_DISPATCH=8006
PUERTO_ESCUCHA_INTERRUPT=8007
EOF
echo "CPU configs done:"
cat cpu/cfg/cpu.config
echo .


echo "Escribo las configs de FILESYSTEM."
cat << EOF > filesystem/cfg/filesystem.config
IP_MEMORIA=${IP_MEMORIA}
PUERTO_MEMORIA=8002
PUERTO_ESCUCHA=8003
PATH_FAT=./fs/fat.dat
PATH_BLOQUES=./fs/bloques.dat
PATH_FCB=./fs/fcbs
CANT_BLOQUES_TOTAL=8192
CANT_BLOQUES_SWAP=1024
TAM_BLOQUE=16
RETARDO_ACCESO_BLOQUE=2500
RETARDO_ACCESO_FAT=500
EOF
echo "Filesystem configs done:"
cat filesystem/cfg/filesystem.config
echo .


echo "Escribo las configs de KERNEL."
cat << EOF > kernel/cfg/kernel.config
CPU_IP=${IP_CPU}
CPU_DISPATCH_PORT=8006
CPU_INTERRUPT_PORT=8007
MEMORY_IP=${IP_MEMORIA}
MEMORY_PORT=8002
FILE_SYSTEM_IP=${IP_FS}
FILE_SYSTEM_PORT=8003
PLANNER=PRIORIDADES
QUANTUM=2000
MULTIPROGRAMMING_DEGREE=10
RECURSOS=[RECURSO]
INSTANCIAS_RECURSOS=[1]
EOF
echo "Kernel configs done:"
cat kernel/cfg/kernel.config
echo .


echo "Escribo las configs de MEMORIA."
cat << EOF > memoria/cfg/memoria.config
PUERTO_ESCUCHA=8002
IP_FILESYSTEM=${IP_FS}
PUERTO_FILESYSTEM=8003
TAM_MEMORIA=1024
TAM_PAGINA=16
PATH_INSTRUCCIONES=./mappa-pruebas
RETARDO_RESPUESTA=1000
ALGORITMO_REEMPLAZO=FIFO
ALGORITMO_ASIGNACION=PLACEHOLDER
EOF
echo "Memoria configs done:"
cat memoria/cfg/memoria.config
echo .