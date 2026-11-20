/**
 * @file vector_scene.c
 * @brief Implementation skeleton of the VectorScene ADT (2D Vector Scene)
 *
 * Algoritmos e Estruturas de Dados 2026/2027
 * Departamento de Electrónica, Telecomunicações e Informática - UA
 */

/**
 * STUDENT NAME: Joana Castro e Silva
 * STUDENT NUMBER: 119844
 * STUDENT NAME: Ângelo Manuel Cauchie Figueiredo
 * STUDENT NUMBER: 120300
 */

#include "vector_scene.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primitive_internal.h"

// ============================================================================
// CONCRETE STRUCT LAYOUT (OPAQUE ADT)
// ============================================================================

/**
 * @brief Concrete structure definition for VectorScene.
 * Hidden from vector_scene.h to enforce encapsulation.
 */
struct _VectorScene {
  int width;              /**< Canvas width in pixels */
  int height;             /**< Canvas height in pixels */
  Color bg_color;         /**< Default background color */
  Primitive** primitives; /**< Dynamic array of pointers to Primitive */
  size_t count;           /**< Current number of primitives in scene */
  size_t capacity;        /**< Allocated capacity for primitives array */
  int next_auto_id;       /**< Auto-increment ID generator */
};

#define INITIAL_SCENE_CAPACITY 8

// ============================================================================
// HELPER FUNCTIONS (PROVIDED TO STUDENTS)
// ============================================================================

/**
 * @brief Helper to write an RGB pixel buffer to binary PPM file (P6 format).
 */
static bool save_ppm_image(const char* filename, int width, int height,
                           const uint8_t* pixel_buffer) {
  if (!filename || !pixel_buffer || width <= 0 || height <= 0) {
    return false;
  }

  FILE* file = fopen(filename, "wb");
  if (!file) {
    return false;
  }

  fprintf(file, "P6\n%d %d\n255\n", width, height);

  size_t total_bytes = (size_t)width * height * 3;
  size_t written = fwrite(pixel_buffer, 1, total_bytes, file);
  fclose(file);

  return written == total_bytes;
}

// ============================================================================
// ACCESSORS / GETTERS (PROVIDED TO STUDENTS)
// ============================================================================

int vector_scene_get_width(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->width;
}

int vector_scene_get_height(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->height;
}

Color vector_scene_get_background_color(const VectorScene* scene) {
  if (!scene) {
    Color black = {0, 0, 0};
    return black;
  }
  return scene->bg_color;
}

size_t vector_scene_get_count(const VectorScene* scene) {
  if (!scene) {
    return 0;
  }
  return scene->count;
}

Primitive* vector_scene_get_primitive_by_index(const VectorScene* scene,
                                               size_t index) {
  if (!scene || index >= scene->count) {
    return NULL;
  }
  return scene->primitives[index];
}

Primitive* vector_scene_get_primitive_by_id(const VectorScene* scene, int id) {
  if (!scene || id < 0) {
    return NULL;
  }

  for (size_t i = 0; i < scene->count; i++) {
    if (scene->primitives[i] && scene->primitives[i]->id == id) {
      return scene->primitives[i];
    }
  }
  return NULL;
}

// ============================================================================
// SCENE LIFECYCLE (STUDENTS MUST COMPLETE)
// ============================================================================

VectorScene* vector_scene_create(int width, int height, Color bg_color){
	// verificar tamanhos não negativos
	if(width <= 0 || height <= 0){
		return NULL;
	}

	// alocar struct
	VectorScene* scene = malloc(sizeof(VectorScene));

	// verificar ponteiro nao nulo
	if(scene == NULL){
		return NULL;
	}

	// alocar array de ponteiros para as primitivas
	scene->primitives = malloc(INITIAL_SCENE_CAPACITY * sizeof(Primitive*));
	
	if(scene->primitives == NULL){
		free(scene);	// libertar espaço se array falhar
		return NULL;
	}
	
	scene->width = width;
	scene->height = height;
	scene->bg_color = bg_color;
	scene->count = 0;
	scene->capacity = INITIAL_SCENE_CAPACITY;
	scene->next_auto_id = 1;
	
	return scene;
}

void vector_scene_destroy(VectorScene* scene){
	if(!scene){
		return;
	}

	// destruir cada primitiva do array de primitivas do struct
	for(size_t i = 0; i < scene->count; i++){
		primitive_destroy(scene->primitives[i]);
	}

	// libertar espaço alocado
	free(scene->primitives);
	free(scene);
}

// ============================================================================
// COLLECTION MANAGEMENT (STUDENTS MUST COMPLETE)
// ============================================================================

int vector_scene_add_primitive(VectorScene* scene, Primitive* p){
	// verificar que scene e ponteiro existem
	if(!scene || !p){
		return -1;
	}

	// duplicar espaço do array caso esteja cheio
	if (scene->count == scene->capacity) {
		size_t new_capacity = scene->capacity * 2;
		Primitive** new_array = realloc(scene->primitives, new_capacity * sizeof(Primitive*));
		if(new_array == NULL){
			return -1;
		}
		scene->primitives = new_array;
		scene->capacity = new_capacity;
	}

	// id negativo -> atribuir automaticamente id
	if(p->id < 0){
		p->id = scene->next_auto_id++;
	}else if(p->id >= scene->next_auto_id){
		scene->next_auto_id = p->id + 1;
	}

	// acrescentar no fim do array
	scene->primitives[scene->count] = p;
	scene->count++;
	
	primitive_update_aabb(p);
	
	return p->id;
}

bool vector_scene_remove_primitive(VectorScene* scene, int id){
	// verificar que scene existe
	if(!scene || id < 0){
		return false;
	}

	// procurar posição de primitiva pelo id
	for(size_t i = 0; i < scene->count; i++){
		if(scene->primitives[i]->id == id){
			primitive_destroy(scene->primitives[i]);	// destruir

			// deslocar primitivas seguintos no array para a esquerda			
			for(size_t j = i; j + 1 < scene->count; j++){
				scene->primitives[j] = scene->primitives[j + 1];
			}
			scene->count--;
			return true;
		}
	}
	return false;
}

// ============================================================================
// MANIPULATION AND TRANSFORMATIONS (STUDENTS MUST COMPLETE)
// ============================================================================

bool vector_scene_translate_primitive(VectorScene* scene, int id, double dx, double dy){
	if(!scene){	// verificar que scene existe
		return false;
	}

	Primitive *p = vector_scene_get_primitive_by_id(scene, id);	// procurar por id
	if(p == NULL){	// verificar que ponteiro existe
		return false;
	}
	
	primitive_translate(p, dx, dy);	// translatar
	return true;
}

bool vector_scene_set_z_index(VectorScene* scene, int id, int new_z_index){
	if(!scene){	// verificar que scene existe
		return false;
	}
	
	Primitive* p = vector_scene_get_primitive_by_id(scene, id);	// procurar por id
	if(p == NULL){	// verificar que ponteiro existe
		return false;
	}
	
	p->z_index = new_z_index;	// trocar z index
	return true;
}

// ============================================================================
// SPATIAL SELECTION AND SORTING (STUDENTS MUST COMPLETE & ANALYZE)
// ============================================================================

/**
 * @brief Sorts an array of primitives by z_index using O(N^2) Insertion Sort.
 *
 * Iterative in-place sorting algorithm. Iterates through the primitive array
 * and inserts each element into its correct sorted position based on z_index.
 * Increments comparison and swap/move metrics in the SortStats structure.
 *
 * @param arr Array of Primitive pointers to sort.
 * @param count Number of elements in the array.
 * @param stats Pointer to SortStats structure to record performance metrics.
 */
static void insertion_sort_by_z(Primitive** arr, size_t count, SortStats* stats){
	if(count < 2){	// não dá para ordenar menos de 2 elementos
		return;
	}

	// insertion sort
	for (size_t i = 1; i < count; i++){
		Primitive *key = arr[i];
		size_t j = i;
		
		while (j > 0) {
			stats->comparisons++;
			if(arr[j - 1]->z_index <= key->z_index){
				break;
			}
			arr[j] = arr[j - 1];
			stats->swaps++;
			j--;
		}
		arr[j] = key;
	}
}

/**
 * @brief Merges two sorted subarrays of primitive pointers by z_index.
 *
 * Helper function for MergeSort that combines [left..mid] and [mid+1..right]
 * using a temporary pointer array. Increments comparisons and swaps/moves
 * counters in the SortStats structure.
 *
 * @param arr Array of Primitive pointers to merge.
 * @param temp_arr Auxiliary temporary array of Primitive pointers.
 * @param left Starting index of the first subarray.
 * @param mid Middle index separating the two subarrays.
 * @param right Ending index of the second subarray.
 * @param stats Pointer to SortStats structure to track metrics (can be NULL).
 */
static void merge_by_z(Primitive** arr, Primitive** temp_arr, size_t left, size_t mid, size_t right, SortStats* stats){
	size_t i = left;	// indice esquerdo
	size_t j = mid + 1;	// indice direito
	size_t k = left;	// indice do array temp

	// copiar menor elemento enquanto houver elementos nas metades
	while(i <= mid && j <= right){
		if(stats){
			stats->comparisons++;
		}
		if(arr[i]->z_index <= arr[j]->z_index){
			temp_arr[k++] = arr[i++];
		}else{
			temp_arr[k++] = arr[j++];
		}
		if(stats){
			stats->swaps++;
		}
	}

	// copiar o que sobrar da esquerda
	while(i <= mid){
		temp_arr[k++] = arr[i++];
		if(stats){
			stats->swaps++;
		}
	}

	// copiar o que sobrar da direita
	while(j <= right){
		temp_arr[k++] = arr[j++];
		if(stats){
			stats->swaps++;
		}
	}

	// copiar ordenado para o array original
	for(k = left; k <= right; k++){
		arr[k] = temp_arr[k];
		if(stats){
			stats->swaps++;
		}
	}
}

/**
 * @brief Sorts a subarray of primitives by z_index using O(N log N) MergeSort.
 *
 * Stable recursive Divide-and-Conquer algorithm. Splits the array in half,
 * recursively sorts both halves, and invokes merge_by_z() to merge them.
 *
 * @param arr Array of Primitive pointers to sort.
 * @param temp_arr Auxiliary temporary array allocated for merging.
 * @param left Starting index of the subarray to sort.
 * @param right Ending index of the subarray to sort.
 * @param stats Pointer to SortStats structure to record performance metrics.
 */
static void merge_sort_by_z(Primitive** arr, Primitive** temp_arr, size_t left, size_t right, SortStats* stats){
	if(left >= right){	// subarray com 0 ou 1 elemento já está ordenado
		return;
	}
	
	size_t mid = left + (right - left) / 2;
	
	merge_sort_by_z(arr, temp_arr, left, mid, stats);
	merge_sort_by_z(arr, temp_arr, mid + 1, right, stats);
	merge_by_z(arr, temp_arr, left, mid, right, stats);
}

void vector_scene_sort_by_z(VectorScene* scene, bool use_fast_sort, SortStats* stats){
	if(scene == NULL || scene->count < 2){
		return;
	}
	
	SortStats local_stats;
	if(stats == NULL){
		stats = &local_stats;
	}

	// contadores para benchmark a 0
	stats->comparisons = 0;
	stats->swaps = 0;
	
	if(!use_fast_sort){
		// insertion sort O(N^2)
		insertion_sort_by_z(scene->primitives, scene->count, stats);
	}else{
		// merge sort O(NlogN)
		Primitive** temp_arr = malloc(scene->count * sizeof(Primitive*));
		if(temp_arr == NULL){
			return;
		}
		merge_sort_by_z(scene->primitives, temp_arr, 0, scene->count - 1, stats);
		free(temp_arr);
	}
}

Primitive* vector_scene_pick_top_primitive(const VectorScene* scene, Point pt, bool is_opt, PickStats* stats){
	if(!scene){	// verificar que scene existe
		return NULL;
	}
	
	PickStats local_stats = {0, 0};
	
	Primitive *top = NULL;
	
	// percorrer array, guarda a que tiver maior z index e estiver mais à frenteno array
	for(size_t i = scene->count; i > 0; i--){
		Primitive *p = scene->primitives[i - 1];
		
		if(is_opt){
			// broad-phase teste com caixa delimitadora
			local_stats.aabb_tests++;
			BoundingBox box = primitive_get_aabb(p);
			if(pt.x < box.min.x || pt.x > box.max.x || pt.y < box.min.y || pt.y > box.max.y){
				continue;
			}
		}
		// narrow-phase teste geométrico exato
		local_stats.exact_tests++;
		if(primitive_contains_point(p, pt)){
			if(top == NULL || p->z_index > top->z_index){
				top = p;
			}
		}
	}
	
	if(stats != NULL){
		*stats = local_stats;
	}
	return top;
}

// ============================================================================
// RENDERING / RASTERIZATION (STUDENTS MUST COMPLETE)
// ============================================================================

/**
 * [ALUNOS - função auxiliar]
 * Converte uma coordenada real no índice do píxel correspondente, limitado
 * ao intervalo [0..max_index] (recorte contra os limites da imagem).
 */
static int clip_to_pixel_index(double coord, int max_index){
	if(coord < 0.0){
		return 0;
	}
	if(coord > (double)max_index){
		return max_index;
	}
	return (int)floor(coord);
}

bool vector_scene_render(const VectorScene* scene, const char* output_filename){
	if(!scene || !output_filename){
		return false;
	}
	
	int width = scene->width;
	int height = scene->height;

	// alocar buffer de 3 bytes para R, G e B por pixel, e pintar fundo
	size_t num_pixels = (size_t)width * height;
	uint8_t *pixel_buffer = malloc(num_pixels * 3);
	if(pixel_buffer == NULL){
		return false;
	}
	for(size_t i = 0; i < num_pixels; i++){
		pixel_buffer[i * 3 + 0] = scene->bg_color.r;
		pixel_buffer[i * 3 + 1] = scene->bg_color.g;
		pixel_buffer[i * 3 + 2] = scene->bg_color.b;
	}
	
	if(scene->count > 0){
		// ordenar por z index crescente
		Primitive** sorted = malloc(scene->count * sizeof(Primitive*));
		Primitive** temp_arr = malloc(scene->count * sizeof(Primitive*));
		// ordenar copia do array de primitivas
		if(sorted == NULL || temp_arr == NULL){
			free(sorted);
			free(temp_arr);
			free(pixel_buffer);
			return false;
		}
		for(size_t i = 0; i < scene->count; i++){
			sorted[i] = scene->primitives[i];
		}
		merge_sort_by_z(sorted, temp_arr, 0, scene->count - 1, NULL);
		free(temp_arr);

		// desenhar primitivas do fundo para a frente
		for(size_t i = 0; i < scene->count; i++){
			Primitive *p = sorted[i];
			
			BoundingBox box = primitive_get_aabb(p);	// aabb atualizada

			// não desenhar fora da imagem
			if(box.max.x < 0.0 || box.min.x >= width || box.max.y < 0.0 || box.min.y >= height){
				continue;
			}
			// testar pixeis dentro da aabb limitada pelo tamanho da imagem
			int x_min = clip_to_pixel_index(box.min.x, width - 1);
			int x_max = clip_to_pixel_index(box.max.x, width - 1);
			int y_min = clip_to_pixel_index(box.min.y, height - 1);
			int y_max = clip_to_pixel_index(box.max.y, height - 1);
			
			double alpha = p->alpha;

			// testar de o centre de cada pixel da janela pertence à primitiva
			for(int y = y_min; y <= y_max; y++){
				for(int x = x_min; x <= x_max; x++){
					Point pixel_center = {x + 0.5, y + 0.5};
					if(!primitive_contains_point(p, pixel_center)){
						continue;
					}
				uint8_t *pixel = &pixel_buffer[((size_t)y * width + x) * 3];
				pixel[0] = (uint8_t)(alpha * p->color.r + (1.0 - alpha) * pixel[0]);
                pixel[1] = (uint8_t)(alpha * p->color.g + (1.0 - alpha) * pixel[1]);
				pixel[2] = (uint8_t)(alpha * p->color.b + (1.0 - alpha) * pixel[2]);
				}
			}
		}
	free(sorted);
	}
	
	bool ok = save_ppm_image(output_filename, width, height, pixel_buffer);
	
	free(pixel_buffer);
	
	return ok;
}

// ============================================================================
// FILE INPUT / OUTPUT (PROVIDED TO STUDENTS)
// ============================================================================

bool vector_scene_save_text(const VectorScene* scene, const char* filename) {
  if (!scene || !filename) {
    return false;
  }

  FILE* file = fopen(filename, "w");
  if (!file) {
    return false;
  }

  Color bg = scene->bg_color;
  fprintf(file, "SCENE %d %d %d %d %d\n", scene->width, scene->height, bg.r,
          bg.g, bg.b);

  for (size_t i = 0; i < scene->count; i++) {
    Primitive* p = scene->primitives[i];
    if (!p) {
      continue;
    }

    Color c = p->color;

    switch (p->type) {
      case PRIM_CIRCLE:
        fprintf(file, "CIRC %d %d %d %d %d %.3f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha, p->geom.circle.center.x,
                p->geom.circle.center.y, p->geom.circle.radius);
        break;

      case PRIM_RECTANGLE:
        fprintf(file, "RECT %d %d %d %d %d %.3f %.2f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha,
                p->geom.rectangle.top_left.x, p->geom.rectangle.top_left.y,
                p->geom.rectangle.width, p->geom.rectangle.height);
        break;

      case PRIM_SEGMENT:
        fprintf(file, "SEG %d %d %d %d %d %.3f %.2f %.2f %.2f %.2f\n", p->id,
                p->z_index, c.r, c.g, c.b, p->alpha, p->geom.segment.p1.x,
                p->geom.segment.p1.y, p->geom.segment.p2.x,
                p->geom.segment.p2.y);
        break;

      case PRIM_POLYGON:
        fprintf(file, "POLY %d %d %d %d %d %.3f %zu", p->id, p->z_index, c.r,
                c.g, c.b, p->alpha, p->geom.polygon.num_vertices);

        for (size_t v = 0; v < p->geom.polygon.num_vertices; v++) {
          fprintf(file, " %.2f %.2f", p->geom.polygon.vertices[v].x,
                  p->geom.polygon.vertices[v].y);
        }
        fprintf(file, "\n");
        break;
    }
  }

  fclose(file);
  return true;
}

VectorScene* vector_scene_load_text(const char* filename) {
  if (!filename) {
    return NULL;
  }

  FILE* file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  char line[1024];
  VectorScene* scene = NULL;

  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
      continue;
    }

    char tag[16];
    if (sscanf(line, "%15s", tag) != 1) {
      continue;
    }

    if (strcmp(tag, "SCENE") == 0) {
      int w, h, r, g, b;
      if (sscanf(line, "SCENE %d %d %d %d %d", &w, &h, &r, &g, &b) == 5) {
        Color bg = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
        scene = vector_scene_create(w, h, bg);
      }
    } else if (scene != NULL) {
      int id, z_index, r, g, b;
      double alpha;

      if (strcmp(tag, "CIRC") == 0) {
        double cx, cy, radius;
        if (sscanf(line, "CIRC %d %d %d %d %d %lf %lf %lf %lf", &id, &z_index,
                   &r, &g, &b, &alpha, &cx, &cy, &radius) == 9) {
          Point center = {cx, cy};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_circle(id, center, radius, color,
                                                 alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "RECT") == 0) {
        double x, y, w, h;
        if (sscanf(line, "RECT %d %d %d %d %d %lf %lf %lf %lf %lf", &id,
                   &z_index, &r, &g, &b, &alpha, &x, &y, &w, &h) == 10) {
          Point top_left = {x, y};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_rectangle(id, top_left, w, h, color,
                                                    alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "SEG") == 0) {
        double x1, y1, x2, y2;
        if (sscanf(line, "SEG %d %d %d %d %d %lf %lf %lf %lf %lf", &id,
                   &z_index, &r, &g, &b, &alpha, &x1, &y1, &x2, &y2) == 10) {
          Point p1 = {x1, y1};
          Point p2 = {x2, y2};
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p =
              primitive_create_segment(id, p1, p2, color, alpha, z_index);
          if (p) {
            vector_scene_add_primitive(scene, p);
          }
        }
      } else if (strcmp(tag, "POLY") == 0) {
        size_t num_verts;
        int read_header = sscanf(line, "POLY %d %d %d %d %d %lf %zu", &id,
                                 &z_index, &r, &g, &b, &alpha, &num_verts);
        if (read_header == 7) {
          Color color = {(uint8_t)r, (uint8_t)g, (uint8_t)b};
          Primitive* p = primitive_create_polygon(id, color, alpha, z_index);
          if (p) {
            char* ptr = line;
            for (int k = 0; k < 7; k++) {
              while (*ptr && *ptr != ' ' && *ptr != '\t') {
                ptr++;
              }
              while (*ptr && (*ptr == ' ' || *ptr == '\t')) {
                ptr++;
              }
            }

            for (size_t v = 0; v < num_verts; v++) {
              double vx, vy;
              if (sscanf(ptr, "%lf %lf", &vx, &vy) == 2) {
                Point pt = {vx, vy};
                polygon_add_vertex(p, pt);

                for (int k = 0; k < 2; k++) {
                  while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' &&
                         *ptr != '\r') {
                    ptr++;
                  }
                  while (*ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' ||
                                  *ptr == '\r')) {
                    ptr++;
                  }
                }
              }
            }
            vector_scene_add_primitive(scene, p);
          }
        }
      }
    }
  }

  fclose(file);
  return scene;
}
