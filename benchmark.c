/**
 * @file benchmark.c
 * @brief Testes de desempenho para o relatório (ordenação e picking).
 *
 * Ficheiro novo (não faz parte dos ficheiros fornecidos).
 * Para cada N (100 a 100 000 primitivas aleatórias) mede:
 *  - Ordenação por z_index: tempo, comparações e trocas do Insertion Sort
 *    e do Merge Sort (sobre a mesma cena desordenada).
 *  - Picking: M seleções em pontos aleatórios, com a estratégia direta e com
 *    a estratégia em duas fases (AABB), contando os testes da PickStats.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "primitive.h"
#include "vector_scene.h"

#define CANVAS_SIZE 1000 /* cena de 1000x1000 */
#define NUM_PICKS 1000   /* M: número de seleções por cena */
#define SEED 2026        /* semente fixa: resultados repetíveis */

/* Número real aleatório em [0, max[ */
static double random_double(double max) {
  return max * rand() / ((double)RAND_MAX + 1.0);
}

/* Cria uma cena com n primitivas aleatórias dos 4 tipos.
 * Como a semente é sempre a mesma, duas chamadas com o mesmo n
 * produzem exatamente a mesma cena. */
static VectorScene* create_random_scene(int n) {
  srand(SEED);

  Color bg = {255, 255, 255};
  VectorScene* scene = vector_scene_create(CANVAS_SIZE, CANVAS_SIZE, bg);

  for (int i = 0; i < n; i++) {
    Color color = {(uint8_t)(rand() % 256), (uint8_t)(rand() % 256),
                   (uint8_t)(rand() % 256)};
    double x = random_double(CANVAS_SIZE);
    double y = random_double(CANVAS_SIZE);
    double size = 5.0 + random_double(45.0); /* dimensão entre 5 e 50 */
    int z_index = rand() % 1000;
    Primitive* p = NULL;

    switch (i % 4) {
      case 0:
        p = primitive_create_circle(-1, (Point){x, y}, size, color, 1.0,
                                    z_index);
        break;
      case 1:
        p = primitive_create_rectangle(-1, (Point){x, y}, size, size, color,
                                       1.0, z_index);
        break;
      case 2:
        p = primitive_create_segment(-1, (Point){x, y},
                                     (Point){x + size, y + size}, color, 1.0,
                                     z_index);
        break;
      case 3: /* triângulo */
        p = primitive_create_polygon(-1, color, 1.0, z_index);
        polygon_add_vertex(p, (Point){x, y});
        polygon_add_vertex(p, (Point){x + size, y});
        polygon_add_vertex(p, (Point){x, y + size});
        break;
    }
    vector_scene_add_primitive(scene, p);
  }
  return scene;
}

/* Ordena uma cena aleatória com n primitivas e devolve o tempo em segundos. */
static double time_sort(int n, bool use_fast_sort, SortStats* stats) {
  VectorScene* scene = create_random_scene(n);

  clock_t start = clock();
  vector_scene_sort_by_z(scene, use_fast_sort, stats);
  clock_t end = clock();

  vector_scene_destroy(scene);
  return (double)(end - start) / CLOCKS_PER_SEC;
}

/* Faz NUM_PICKS seleções numa cena com n primitivas, somando os contadores.
 * Devolve o tempo total em segundos. */
static double time_picking(int n, bool is_opt, PickStats* total) {
  VectorScene* scene = create_random_scene(n);
  total->aabb_tests = 0;
  total->exact_tests = 0;

  srand(SEED + 1); /* mesmos pontos para as duas estratégias */

  clock_t start = clock();
  for (int i = 0; i < NUM_PICKS; i++) {
    Point pt = {random_double(CANVAS_SIZE), random_double(CANVAS_SIZE)};
    PickStats stats;
    vector_scene_pick_top_primitive(scene, pt, is_opt, &stats);
    total->aabb_tests += stats.aabb_tests;
    total->exact_tests += stats.exact_tests;
  }
  clock_t end = clock();

  vector_scene_destroy(scene);
  return (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void) {
  int sizes[] = {100, 500, 1000, 5000, 10000, 50000, 100000};
  int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

  printf("=== ORDENACAO POR Z-INDEX ===\n");
  printf("%8s | %14s %14s %10s | %12s %12s %10s\n", "N", "Ins.Comp",
         "Ins.Trocas", "Ins.t(s)", "Mrg.Comp", "Mrg.Trocas", "Mrg.t(s)");
  for (int i = 0; i < num_sizes; i++) {
    SortStats ins = {0, 0}, mrg = {0, 0};
    double t_ins = time_sort(sizes[i], false, &ins);
    double t_mrg = time_sort(sizes[i], true, &mrg);
    printf("%8d | %14lu %14lu %10.4f | %12lu %12lu %10.4f\n", sizes[i],
           ins.comparisons, ins.swaps, t_ins, mrg.comparisons, mrg.swaps,
           t_mrg);
  }

  printf("\n=== PICKING (M = %d selecoes por cena) ===\n", NUM_PICKS);
  printf("%8s | %14s %9s | %12s %12s %9s | %8s\n", "N", "Direta.Exatos",
         "t(s)", "Opt.AABB", "Opt.Exatos", "t(s)", "Racio");
  for (int i = 0; i < num_sizes; i++) {
    PickStats direct, opt;
    double t_direct = time_picking(sizes[i], false, &direct);
    double t_opt = time_picking(sizes[i], true, &opt);
    /* Rácio: testes exatos da estratégia direta / testes exatos da otimizada */
    double ratio = opt.exact_tests > 0
                       ? (double)direct.exact_tests / opt.exact_tests
                       : 0.0;
    printf("%8d | %14lu %9.4f | %12lu %12lu %9.4f | %8.1f\n", sizes[i],
           direct.exact_tests, t_direct, opt.aabb_tests, opt.exact_tests,
           t_opt, ratio);
  }

  return EXIT_SUCCESS;
}
