module=${1}
echo "Ejecuto modulo ${module}..."
mkdir -p filesystem/fs/fcbs
#rm filesystem/fs/fcbs/*
#rm filesystem/fs/bloques.dat
#rm filesystem/fs/fat.dat
#rm */*.log
cd ${module}
make clean
make all
./bin/${module}.out cfg/${module}.config