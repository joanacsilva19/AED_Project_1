# AED_Project_1

```
# Compilar e testar
make
make test          # ou make test-fase1 ... test-fase5
make demo
make memcheck      # valgrind
```
```
# Composição artística
gcc -Wall -Wextra -Wpedantic -std=c17 -g -O2 -o art_scene art_scene.c primitive.c vector_scene.c -lm
./art_scene
valgrind --leak-check=full ./art_scene
```
```
# Testes de desempenho (N de 100 a 100 000; demora uns segundos)
gcc -Wall -Wextra -Wpedantic -std=c17 -g -O2 -o benchmark benchmark.c primitive.c vector_scene.c -lm
./benchmark
```
