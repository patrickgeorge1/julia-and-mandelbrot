/*
 * APD - Tema 1
 * Octombrie 2020
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;


char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;
params global_param_julia;
params global_param_mandelbrot;
int **result_julia;
int **result_mandelbrot;
int width_julia, height_julia;
int width_mandelbrot, height_mandelbrot;
int P;


// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot P\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

int get_min(int a, int b) {
	if (a < b) return a;
	return b;
}

// ruleaza algoritmul Julia
void run_julia(params *par, int **result, int width, int height, int id)
{
	int w, h;
	int start = id * (double)width / P;
	int end = get_min((id + 1) * (double)width / P, width);

	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			int step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}

			result[height - h - 1][w] = step % 256;
		}
	}
}

// ruleaza algoritmul Mandelbrot
void run_mandelbrot(params *par, int **result, int width, int height, int id)
{
	int w, h;
	int start = id * (double)width / P;
	int end = get_min((id + 1) * (double)width / P, width);

	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[height - h - 1][w] = step % 256;
		}
	}
}

void *core_function(void *arg) {
	int thread_id = *(int *)arg;

	// solve julia
	run_julia(&global_param_julia, result_julia, width_julia, height_julia, thread_id);


	// solve mandelbrot
	run_mandelbrot(&global_param_mandelbrot, result_mandelbrot, width_mandelbrot, height_mandelbrot, thread_id);


	// finish thread
	pthread_exit(NULL);
}


int main(int argc, char *argv[])
{

	// se citesc argumentele programului
	get_args(argc, argv);


	// prepare julia
	read_input_file(in_filename_julia, &global_param_julia);
	width_julia = (global_param_julia.x_max - global_param_julia.x_min) / global_param_julia.resolution;
	height_julia = (global_param_julia.y_max - global_param_julia.y_min) / global_param_julia.resolution;
	result_julia = allocate_memory(width_julia, height_julia);

	// prepare manfelbrot
	read_input_file(in_filename_mandelbrot, &global_param_mandelbrot);
	width_mandelbrot = (global_param_mandelbrot.x_max - global_param_mandelbrot.x_min) / global_param_mandelbrot.resolution;
	height_mandelbrot = (global_param_mandelbrot.y_max - global_param_mandelbrot.y_min) / global_param_mandelbrot.resolution;
	result_mandelbrot = allocate_memory(width_mandelbrot, height_mandelbrot);


	// se declara P threads
	pthread_t threads[P];
	long ids[P];
	void * status;

	for (int id = 0; id < P; id++)
	{
		ids[id] = id;
		int r = pthread_create(&threads[id], NULL, core_function, &ids[id]);

  		if (r) {
  	  		printf("Eroare la crearea thread-ului %d\n", id);
  	  		exit(-1);
  		}
	}

	for (int id = 0; id < P; id++) {
  		int r = pthread_join(threads[id], &status);

  		if (r) {
  	  		printf("Eroare la asteptarea thread-ului %d\n", id);
  	  		exit(-1);
  		}
  	}

	// write juliua
	write_output_file(out_filename_julia, result_julia, width_julia, height_julia);
	free_memory(result_julia, height_julia);

	// write mandelbrot
	write_output_file(out_filename_mandelbrot, result_mandelbrot, width_mandelbrot, height_mandelbrot);
	free_memory(result_mandelbrot, height_mandelbrot);

	return 0;
}
