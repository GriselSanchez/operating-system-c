# tp-2023-2c-salvame-superman

## Correr el programa
- Comprobar archivos de configuración
- Levantar en el siguiente orden los módulos:
  1. Memoria
  2. File system 
  3. CPU
  4. Kernel

## Leer archivo de log de kernel
tail -f kernel/kernel.log

## Recursos

[Guía Template](https://raniagus.github.io/so-project-template/guia/)

## Paquetes a instalar

### CSpec

1. `git clone https://github.com/mumuki/cspec.git`
2. `cd cspec`
3. `make install`

### Commons

1. `git clone https://github.com/sisoputnfrba/so-commons-library.git`
2. `cd so-commons-library`
3. `make install`

# File System

## Operaciones Archivo
### Abrir Archivo
- Payload: ABRIR_ARCHIVO | largo_nombre (int) | nombre (char*)
- Response: 
  - Success -> tamanio archivo
  - Error -> -1

### Crear Archivo
- Payload: CREAR_ARCHIVO | largo_nombre (int) | nombre (char*)
- Response: 
  - Success -> ARCHIVO_OK
  - Error -> no hay

### Truncar Archivo
- Payload: TRUNCAR_ARCHIVO | largo_nombre (int) | nombre (char*) | tamanio (int)
- Response: 
  - Success -> ARCHIVO_OK
  - Error -> no hay

### Leer Archivo
- Payload: LEER_ARCHIVO | largo_nombre (int) | nombre (char*) | puntero (int) | dir_fisica (int) | pid (int)
- Response: 
  - Success -> ARCHIVO_OK
  - Error -> no hay

### Escribir Archivo
- Payload: ESCRIBIR_ARCHIVO | largo_nombre (int) | nombre (char*) | puntero (int) | dir_fisica (int) | pid (int)
- Response: 
  - Success -> ARCHIVO_OK
  - Error -> no hay

## Operaciones SWAP
### Nuevo Proceso
- Payload: SWAP_NEW_PROCESS | cant_bloques (int)
- Response: 
  - Success -> SWAP_OK | pos_swap_1 (int) | ... | pos_swap_n (int) 
  - Error -> SWAP_ERROR

### Finalizar Proceso
- Payload: SWAP_END_PROCESS | cant_bloques (int) | pos_swap_1 (int) | ... | pos_swap_n (int) 
- Response: 
  - Success -> SWAP_OK 
  - Error -> SWAP_ERROR

### Swap In
- Payload: SWAP_IN | pos_swap (int)
- Response: 
  - Success -> data_bloque (array de uint32) 
  - Error -> SWAP_ERROR

### Swap Out
- Payload: SWAP_OUT | pos_swap (int) | data_pagina (array de uint32)
- Response: 
  - Success -> SWAP_OK
  - Error -> SWAP_ERROR


```
Nota: El tamaño de la información a leer/escribir de la memoria coincidirá con el tamaño del bloque / página. Siempre se leerá/escribirá un bloque completo, los punteros utilizados siempre serán el 1er byte del bloque o página.
```

### Ver bits de archivos .dat
- `~/fs$ xxd -b <archivo>.dat`
### Archivos necesarios
- `/home/utnso/fs/fcb` (directorio)

---
## Scripts
Antes de ejecutar los scripts, setear las variables de ip:

`export IP_MEMORIA=123.456.etc`

`export IP_FS=123.456.etc` 

`export IP_CPU=123.456.etc`
