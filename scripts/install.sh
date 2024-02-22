github_user=$1
github_key=$2

git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make install

cd ..
git clone https://${github_user}:${github_key}@github.com/sisoputnfrba/tp-2023-2c-salvame-superman.git
cd tp-2023-2c-salvame-superman
cd utils
make all
cd ../memoria
make all
cd ../filesystem
make all
cd ../cpu
make all
cd ../kernel
make all