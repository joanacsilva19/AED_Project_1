
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "primitive.h"
#include "vector_scene.h"

#define CANVAS_SIZE 1000 // cena de 1000x1000
#define NUM_PICKS 1000   // número de seleções por cena
#define SEED 6767        // semente fixa: resultados repetíveis

// ordem inicial dos z_index na cena
#define ORDER_RANDOM 0   // aleatória (caso médio)
#define ORDER_ASC 1      // crescente (já ordenada, melhor caso)
#define ORDER_DESC 2     // decrescente (ordem inversa, pior caso)

// resultados de um ensaio de picking (somados nas M seleções)
typedef struct {
	PickStats direct;		// estratégia direta
	PickStats opt;			// estratégia em duas fases
	unsigned long hits;		// primitivas que contêm mesmo o ponto
	int mismatches;			// seleções em que as duas estratégias diferem
	double t_direct;		// tempo total (s)
	double t_opt;
} PickResult;

// [função auxiliar] número real aleatório em [0, max[
static double random_double(double max){

	return max * rand() / ((double)RAND_MAX + 1.0);
}

// [função auxiliar] tempo em segundos entre dois clock()
static double elapsed(clock_t start, clock_t end){

	return (double)(end - start) / CLOCKS_PER_SEC;
}

// [função auxiliar] cria uma cena com n primitivas aleatórias dos 4 tipos
// dimensão entre 5 e max_size; z_index conforme a ordem pedida
// a semente é sempre a mesma, logo o mesmo n dá sempre a mesma cena
static VectorScene* create_random_scene(int n, int order, double max_size){

	srand(SEED);

	Color bg = {255, 255, 255};
	VectorScene* scene = vector_scene_create(CANVAS_SIZE, CANVAS_SIZE, bg);

	// ponteiro não nulo
	if(scene == NULL){
		return NULL;
	}

	for(int i = 0; i < n; i++){
		Color color = {(uint8_t)(rand() % 256), (uint8_t)(rand() % 256), (uint8_t)(rand() % 256)};
		double x = random_double(CANVAS_SIZE);
		double y = random_double(CANVAS_SIZE);
		double size = 5.0 + random_double(max_size - 5.0);
		Primitive* p = NULL;

		int z_index = rand() % 1000;
		if(order == ORDER_ASC){
			z_index = i;
		}
		else if(order == ORDER_DESC){
			z_index = n - i;
		}

		switch(i % 4){
			case 0:
				p = primitive_create_circle(-1, (Point){x, y}, size, color, 1.0, z_index);
				break;

			case 1:
				p = primitive_create_rectangle(-1, (Point){x, y}, size, size, color, 1.0, z_index);
				break;

			case 2:	// segmento a 45 graus
				p = primitive_create_segment(-1, (Point){x, y}, (Point){x + size, y + size}, color, 1.0, z_index);
				break;

			case 3:	// triângulo
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

// [função auxiliar] número de repetições da ordenação (mais para N pequeno)
static int sort_repetitions(int n){

	if(n <= 1000)
		return 50;
	if(n <= 10000)
		return 5;
	return 1;
}

// ordena uma cena com n primitivas e devolve o tempo médio em segundos
// a criação da cena fica fora da medição
static double time_sort(int n, int order, bool use_fast_sort, SortStats* stats){

	int reps = sort_repetitions(n);
	double total = 0.0;

	for(int r = 0; r < reps; r++){
		VectorScene* scene = create_random_scene(n, order, 50.0);

		clock_t start = clock();
		vector_scene_sort_by_z(scene, use_fast_sort, stats);
		clock_t end = clock();

		total += elapsed(start, end);
		vector_scene_destroy(scene);
	}

	return total / reps;
}

// faz m seleções numa cena com n primitivas, com as duas estratégias
static void run_picking(int n, int m, double max_size, PickResult* res){

	VectorScene* scene = create_random_scene(n, ORDER_RANDOM, max_size);

	// mesmos pontos para as duas estratégias
	Point* points = malloc(m * sizeof(Point));
	if(scene == NULL || points == NULL){
		vector_scene_destroy(scene);
		free(points);
		return;
	}
	srand(SEED + 1);
	for(int i = 0; i < m; i++){
		points[i] = (Point){random_double(CANVAS_SIZE), random_double(CANVAS_SIZE)};
	}

	res->direct = (PickStats){0, 0};
	res->opt = (PickStats){0, 0};
	res->hits = 0;
	res->mismatches = 0;
	PickStats stats;

	// estratégia direta: teste exato a todas as primitivas
	clock_t start = clock();
	for(int i = 0; i < m; i++){
		vector_scene_pick_top_primitive(scene, points[i], false, &stats);
		res->direct.aabb_tests += stats.aabb_tests;
		res->direct.exact_tests += stats.exact_tests;
	}
	res->t_direct = elapsed(start, clock());

	// estratégia em duas fases: AABB e só depois teste exato
	start = clock();
	for(int i = 0; i < m; i++){
		vector_scene_pick_top_primitive(scene, points[i], true, &stats);
		res->opt.aabb_tests += stats.aabb_tests;
		res->opt.exact_tests += stats.exact_tests;
	}
	res->t_opt = elapsed(start, clock());

	// fora da medição: quantas primitivas contêm o ponto (precisão do filtro AABB)
	// e se as duas estratégias devolvem a mesma primitiva
	size_t count = vector_scene_get_count(scene);
	for(int i = 0; i < m; i++){
		for(size_t j = 0; j < count; j++){
			if(primitive_contains_point(vector_scene_get_primitive_by_index(scene, j), points[i]))
				res->hits++;
		}
		Primitive* a = vector_scene_pick_top_primitive(scene, points[i], false, NULL);
		Primitive* b = vector_scene_pick_top_primitive(scene, points[i], true, NULL);
		if(a != b)
			res->mismatches++;
	}

	free(points);
	vector_scene_destroy(scene);
}

// [função auxiliar] imprime uma linha da tabela de picking
static void print_pick_row(const char* label, int value, int m, const PickResult* r){

	double ratio = 0.0;
	if(r->opt.exact_tests > 0){
		ratio = (double)r->direct.exact_tests / r->opt.exact_tests;
	}
	double t_ratio = 0.0;
	if(r->t_opt > 0.0){
		t_ratio = r->t_direct / r->t_opt;
	}
	double precision = 0.0;
	if(r->opt.exact_tests > 0){
		precision = 100.0 * r->hits / r->opt.exact_tests;
	}

	printf("%-3s %7d | %12lu %10.4f | %12lu %12lu %10.4f | %8.2f %6.1f%% %8.1f %6.2f | %4d\n",
	       label, value, r->direct.exact_tests, r->t_direct, r->opt.aabb_tests, r->opt.exact_tests,
	       r->t_opt, (double)r->opt.exact_tests / m, precision, ratio, t_ratio, r->mismatches);
}

int main(void){

	int sizes[] = {100, 500, 1000, 5000, 10000, 50000, 100000};
	int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
	const char* order_names[] = {"ALEATORIA", "CRESCENTE", "DECRESCENTE"};

	// ordenação: as três ordens iniciais, Insertion Sort vs Merge Sort
	for(int order = ORDER_RANDOM; order <= ORDER_DESC; order++){
		printf("=== ORDENACAO POR Z-INDEX: ORDEM %s ===\n", order_names[order]);
		printf("%8s | %14s %14s %12s | %12s %12s %10s\n", "N", "Ins.Comp", "Ins.Trocas", "Ins.t(ms)", "Mrg.Comp", "Mrg.Trocas", "Mrg.t(ms)");

		for(int i = 0; i < num_sizes; i++){
			SortStats ins = {0, 0};
			SortStats mrg = {0, 0};
			double t_ins = time_sort(sizes[i], order, false, &ins);
			double t_mrg = time_sort(sizes[i], order, true, &mrg);
			printf("%8d | %14lu %14lu %12.4f | %12lu %12lu %10.4f\n", sizes[i], ins.comparisons, ins.swaps, t_ins * 1000.0, mrg.comparisons, mrg.swaps, t_mrg * 1000.0);
			fflush(stdout);
		}
		printf("\n");
	}

	const char* pick_header = "%-3s %7s | %12s %10s | %12s %12s %10s | %8s %7s %8s %6s | %4s\n";
	PickResult res;

	// picking com N variável (M = 1000, dimensão 5 a 50)
	printf("=== PICKING: N VARIAVEL (M = %d, D = 50) ===\n", NUM_PICKS);
	printf(pick_header, "", "N", "Dir.Exatos", "Dir.t(s)", "Opt.AABB", "Opt.Exatos", "Opt.t(s)", "Ex/sel", "Prec", "Racio", "Rac.t", "Dif");
	for(int i = 0; i < num_sizes; i++){
		run_picking(sizes[i], NUM_PICKS, 50.0, &res);
		print_pick_row("N", sizes[i], NUM_PICKS, &res);
		fflush(stdout);
	}

	// picking com M variável (N = 10000)
	int picks[] = {100, 1000, 10000};
	printf("\n=== PICKING: M VARIAVEL (N = 10000, D = 50) ===\n");
	printf(pick_header, "", "M", "Dir.Exatos", "Dir.t(s)", "Opt.AABB", "Opt.Exatos", "Opt.t(s)", "Ex/sel", "Prec", "Racio", "Rac.t", "Dif");
	for(int i = 0; i < 3; i++){
		run_picking(10000, picks[i], 50.0, &res);
		print_pick_row("M", picks[i], picks[i], &res);
		fflush(stdout);
	}

	// picking com dimensão máxima D variável (N = 10000, M = 1000)
	int dims[] = {10, 25, 50, 100, 200, 400};
	printf("\n=== PICKING: DIMENSAO D VARIAVEL (N = 10000, M = %d) ===\n", NUM_PICKS);
	printf(pick_header, "", "D", "Dir.Exatos", "Dir.t(s)", "Opt.AABB", "Opt.Exatos", "Opt.t(s)", "Ex/sel", "Prec", "Racio", "Rac.t", "Dif");
	for(int i = 0; i < 6; i++){
		run_picking(10000, NUM_PICKS, dims[i], &res);
		print_pick_row("D", dims[i], NUM_PICKS, &res);
		fflush(stdout);
	}

	return EXIT_SUCCESS;
}
