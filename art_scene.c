
#include <stdio.h>
#include <stdlib.h>

#include "primitive.h"
#include "vector_scene.h"

// [função auxiliar] cria um triângulo (polígono com 3 vértices)
static Primitive* create_triangle(Point a, Point b, Point c, Color color, double alpha, int z_index){

	Primitive* t = primitive_create_polygon(-1, color, alpha, z_index);
	polygon_add_vertex(t, a);
	polygon_add_vertex(t, b);
	polygon_add_vertex(t, c);

	return t;
}

int main(void){

	// cores da cena
	Color wall = {240, 230, 210};	// fundo bege
	Color brown = {139, 90, 43};	// mesa
	Color grey = {128, 128, 128};	// gato
	Color pink = {255, 170, 180};	// orelhas e nariz
	Color green = {60, 180, 75};	// olhos
	Color black = {0, 0, 0};		// pupilas
	Color white = {255, 255, 255};	// brilho dos olhos

	VectorScene* scene = vector_scene_create(800, 600, wall);

	// ponteiro não nulo
	if(scene == NULL){
		fprintf(stderr, "Erro: nao foi possivel criar a cena.\n");
		return EXIT_FAILURE;
	}

	// mesa (camada 1): tampo + 4 pernas
	vector_scene_add_primitive(scene, primitive_create_rectangle(-1, (Point){150, 400}, 500, 30, brown, 1.0, 1));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){180, 430}, (Point){165, 590}, brown, 1.0, 1));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){300, 430}, (Point){290, 590}, brown, 1.0, 1));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){500, 430}, (Point){510, 590}, brown, 1.0, 1));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){620, 430}, (Point){635, 590}, brown, 1.0, 1));

	// cauda (camada 2): 3 segmentos ligados
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){470, 390}, (Point){530, 375}, grey, 1.0, 2));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){530, 375}, (Point){560, 335}, grey, 1.0, 2));
	vector_scene_add_primitive(scene, primitive_create_segment(-1, (Point){560, 335}, (Point){550, 290}, grey, 1.0, 2));

	// corpo (camada 2): trapézio
	Primitive* body = primitive_create_polygon(-1, grey, 1.0, 2);
	polygon_add_vertex(body, (Point){360, 260});
	polygon_add_vertex(body, (Point){440, 260});
	polygon_add_vertex(body, (Point){485, 400});
	polygon_add_vertex(body, (Point){315, 400});
	vector_scene_add_primitive(scene, body);

	// orelhas (camada 2) e interior das orelhas (camada 3)
	vector_scene_add_primitive(scene, create_triangle((Point){345, 200}, (Point){355, 125}, (Point){400, 165}, grey, 1.0, 2));
	vector_scene_add_primitive(scene, create_triangle((Point){455, 200}, (Point){445, 125}, (Point){400, 165}, grey, 1.0, 2));
	vector_scene_add_primitive(scene, create_triangle((Point){355, 185}, (Point){361, 142}, (Point){385, 165}, pink, 1.0, 3));
	vector_scene_add_primitive(scene, create_triangle((Point){445, 185}, (Point){439, 142}, (Point){415, 165}, pink, 1.0, 3));

	// cabeça (camada 4): tapa a base das orelhas
	vector_scene_add_primitive(scene, primitive_create_circle(-1, (Point){400, 215}, 60, grey, 1.0, 4));

	// olhos: íris (camada 5), pupila (camada 6) e brilho semi-transparente (camada 7)
	vector_scene_add_primitive(scene, primitive_create_circle(-1, (Point){375, 205}, 16, green, 1.0, 5));
	vector_scene_add_primitive(scene, primitive_create_circle(-1, (Point){425, 205}, 16, green, 1.0, 5));
	vector_scene_add_primitive(scene, primitive_create_circle(-1, (Point){375, 205}, 8, black, 1.0, 6));
	vector_scene_add_primitive(scene, primitive_create_circle(-1, (Point){425, 205}, 8, black, 1.0, 6));
	vector_scene_add_primitive(scene, primitive_create_rectangle(-1, (Point){377, 196}, 6, 6, white, 0.6, 7));
	vector_scene_add_primitive(scene, primitive_create_rectangle(-1, (Point){427, 196}, 6, 6, white, 0.6, 7));

	// nariz (camada 5): triângulo invertido
	vector_scene_add_primitive(scene, create_triangle((Point){391, 230}, (Point){409, 230}, (Point){400, 242}, pink, 1.0, 5));

	printf("Cena criada com %zu primitivas.\n", vector_scene_get_count(scene));

	// guardar .vsc e gerar .ppm
	if(vector_scene_save_text(scene, "art_group_119844_120300.vsc")){
		printf("Cena guardada em 'art_group_119844_120300.vsc'\n");
	}
	if(vector_scene_render(scene, "art_group_119844_120300.ppm")){
		printf("Imagem gerada em 'art_group_119844_120300.ppm'\n");
	}

	vector_scene_destroy(scene);

	return EXIT_SUCCESS;
}
