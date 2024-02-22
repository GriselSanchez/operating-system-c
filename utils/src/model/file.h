#ifndef SRC_MODEL_FILE_H_
#define SRC_MODEL_FILE_H_

typedef enum {
    W = 0,
    R = 1,
} modo_apertura;


typedef struct {
    char* nombre_archivo;
    modo_apertura modo_apertura;
    int puntero_archivo;
    int dir_fisica;
    int tamanio_archivo;
} t_file;

t_file* file_create(char* file_name, modo_apertura mode);

void file_destroy(t_file*);

#endif /* SRC_MODEL_FILE_H_ */