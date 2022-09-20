#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <math.h> 
#include <string.h>

// Realizado por Rubén Zamora Belinchón y Juan José Martín García

// Variables globales

// Mutex
pthread_mutex_t mutex;

// Condicion
pthread_cond_t *centroDisponible;

// Fichero de salida
FILE *fdSalida;

// Variables
int nHabitantes;
int nCentros = 5;
int nFabricas = 3;
int nVacunadosTanda;
int nVacunasIniciales;
int nVacunasTotalesFabrica;
int nMinVacunasTanda;
int nMaxVacunasTanda;
int tMinFabricacionTanda;
int tMaxFabricacionTanda;
int tMaxReparto;
int tMaxDarseCuenta;
int tMaxDesplazamiento;

// Arrays principales
int *nVacunasCentros;
int *habitantes;
int *nVacunasFabricas;
int *fabricas;

// Arrays estadisticas
int *nVacunasCentrosRepartidos;
int **nVacunasFabricasCentros;

int nTandas = 10;

// Funciones auxiliares
void *vacunacion (void* args);
void *fabricacion (void* args);
void valoresGlobales (FILE *fd);
void imprimirDatos ();
void imprimirEstadisticas ();
int cogerValores (FILE *fd);
int menosVacunas (int *nVacunasCentro);

int main(int argc, char *argv[]) {

	// Variables locales
	pthread_t *th_habitantes;
	pthread_t *th_fabricas;
	
	nVacunadosTanda = nHabitantes/nTandas;
	nVacunasTotalesFabrica = nHabitantes/nFabricas;
	
	int i,j;
	
	FILE *fdEntrada;
	
	// Ficheros de entrada y de salida

	if (argc == 1) {
		fdEntrada = fopen("entrada_vacunacion.txt", "r");
		fdSalida = fopen("salida_vacunacion.txt", "w");
	} else if (argc == 2) {
		fdEntrada = fopen(argv[1], "r");
		fdSalida = fopen("salida_vacunacion.txt", "w");
	} else if (argc == 3) {
		fdEntrada = fopen(argv[1], "r");
		fdSalida = fopen(argv[2], "w");
	} else {
		printf("Error en el número de argumentos.\n");
		exit(1);
	}

	// Guardamos los valores del fichero de entrada e imprimimos los datos por pantalla

	valoresGlobales(fdEntrada);
	imprimirDatos();

	srand(time(NULL)); // Genera un random basado en la función de la hora del procesador

	// Asignamos memoria a los arrays en tiempo de ejecución

	// Arrays unidireccionales
	th_habitantes = (pthread_t*) malloc (nHabitantes * sizeof(pthread_t));	
	th_fabricas = (pthread_t*) malloc (nFabricas * sizeof(pthread_t));
	
	nVacunasCentros = (int*) malloc (nCentros * sizeof(int));
	nVacunasFabricas = (int*) malloc (nFabricas * sizeof(int));
	
	habitantes = (int*) malloc (nHabitantes * sizeof(int));
	fabricas = (int*) malloc (nFabricas * sizeof(int));
	
	centroDisponible = (pthread_cond_t*) malloc (nCentros * sizeof(pthread_cond_t));
	
	nVacunasCentrosRepartidos = (int*) malloc (nCentros * sizeof(int));

	// Array bidimensional

	nVacunasFabricasCentros = (int **) malloc (nFabricas * sizeof(int*)); 
	
	for (i = 0; i < nCentros; i++) {
		nVacunasFabricasCentros[i] = (int*) malloc (nCentros * sizeof(int)); 
	}

	// Inicializamos el mutex y las condiciones

	pthread_mutex_init(&mutex, NULL);

	for (i = 0; i < nCentros; i++) {
		pthread_cond_init(&centroDisponible[i], NULL);
	}

	// Inicializamos 

	// Variables de los centros

	for (i = 0; i < nCentros; i++) {
		nVacunasCentros[i] = nVacunasIniciales;
		nVacunasCentrosRepartidos[i] = 0;
	}

	// Variables de las fábricas

	for (i = 0; i < nFabricas; i++) {
		nVacunasFabricas[i] = nVacunasTotalesFabrica;
	}

	for (i = 0; i < nFabricas; i++) {
		for (j = 0; j < nCentros; j++) {
			nVacunasFabricasCentros[i][j] = 0;
		}
	}

	// Hilos
	
	// Fabricas
	for (i = 0; i < nFabricas; i++) {
		fabricas[i] = i;
		pthread_create(&th_fabricas[i], NULL, fabricacion, (void *) &fabricas[i]);
	}
	
	// Habitantes

	for (i = 0; i < nTandas; i++) { // Tandas
		for (j = 0; j < nVacunadosTanda; j++) { // Habitantes por tanda
			habitantes[j] = j + (nVacunadosTanda*i);
			pthread_create(&th_habitantes[j], NULL, vacunacion, (void *) &habitantes[j]);
		}
		for (j = 0; j < nVacunadosTanda; j++) {
			pthread_join(th_habitantes[j],NULL);
		}	
	}

	for (i = 0; i < nFabricas; i++) {
		pthread_join(th_fabricas[i], NULL);
	}

	printf("Vacunacion finalizada.\n\n");
	fprintf(fdSalida, "Vacunacion finalizada.\n\n");

	// Imprimimos por pantalla las estadisticas de la vacunación

	imprimirEstadisticas();

	// Liberamos memoria
	
	free(th_habitantes);
	free(th_fabricas);
	free(nVacunasCentros);
	free(nVacunasFabricas);
	free(habitantes);
	free(fabricas);
	free(centroDisponible);
	free(nVacunasFabricasCentros);

	// Destruimos el mutex

	pthread_mutex_destroy(&mutex);
	
	// Destruimos las condiciones
	
	for (i = 0; i < nCentros; i++) {
		pthread_cond_destroy(&centroDisponible[i]);
	}

	// Cerramos el fichero de salida

	fclose(fdSalida);
	
	exit(0);
}

void *vacunacion (void* args) {

	int nHabitante = *((int *) args);
	int i = (rand() % nCentros); // Centro elegido

	sleep(rand() % tMaxDarseCuenta + 1); // Tiempo que tarda en darse cuenta el habitante

	printf("Habitante %i elige el centro %i para vacunarse.\n", nHabitante+1, i+1);
	fprintf(fdSalida,"Habitante %i elige el centro %i para vacunarse.\n", nHabitante+1, i+1);

	sleep(rand() % tMaxDesplazamiento + 1); // Tiempo que tarda en desplazarse al centro elegido

	pthread_mutex_lock(&mutex); // Entramos en la seccion critica

	while (nVacunasCentros[i] == 0) {
		pthread_cond_wait(&centroDisponible[i], &mutex);
	}

	if (nVacunasCentros[i] > 0) { // Si disponemos de más de una vacuna en el centro
		nVacunasCentros[i]--;
	}

	pthread_cond_signal(&centroDisponible[i]);

	pthread_mutex_unlock(&mutex); // Salimos de la seccion critica

	printf("Habitante %i vacunado en el centro %i.\n", nHabitante+1, i+1);
	fprintf(fdSalida,"Habitante %i vacunado en el centro %i.\n", nHabitante+1, i+1);

	pthread_exit(NULL);

}

void *fabricacion (void* args) {

	int nFabrica = *((int *) args);
	
	while (nVacunasFabricas[nFabrica] > 0) { // Mientras que a la fabrica le queden vacunas por fabricar
	
		int vacunasT = rand() % (nMaxVacunasTanda - nMinVacunasTanda + 1) + nMinVacunasTanda;
		int sobrantes = 0;
		
		int i,k;
		
		int *vacunas;
		
		vacunas = (int*) malloc (nCentros * sizeof(int));

		sleep(rand() % (tMaxFabricacionTanda - tMinFabricacionTanda + 1) + tMinFabricacionTanda); // Tiempo en preparar las vacunas

		nVacunasFabricas[nFabrica] -= vacunasT;

		if (nVacunasFabricas[nFabrica] < 0) { // Si nos pasamos de las vacunas asignadas a cada fábrica
			vacunasT += nVacunasFabricas[nFabrica];
			nVacunasFabricas[nFabrica] = 0;
		}

		sobrantes = vacunasT % nCentros; // Si el número de vacunas producido no es múltiplo del número de centros se guardan

		pthread_mutex_lock(&mutex); // Entramos en la seccion critica

		if (vacunasT > 0) {
			printf("Fábrica %i prepara %i vacunas.\n", nFabrica+1, vacunasT);
			fprintf(fdSalida, "Fábrica %i prepara %i vacunas.\n", nFabrica+1, vacunasT);

			sleep(rand() % tMaxReparto + 1); // Tiempo que tarda la fábrica en repartir las vacunas

			for (i = 0; i < nCentros; i++) {

	    			nVacunasCentros[i] += vacunasT/nCentros;
	      			nVacunasFabricasCentros[nFabrica][i] += vacunasT/nCentros;
	      			nVacunasCentrosRepartidos[i] += vacunasT/nCentros;
	      			vacunas[i] = vacunasT/nCentros;
	    		}

			while (sobrantes > 0) {
		
				k = menosVacunas(nVacunasCentros);
					
	  			nVacunasCentros[k]++;
		  		nVacunasFabricasCentros[nFabrica][k]++;
		  		nVacunasCentrosRepartidos[k]++;
		  		vacunas[k]++;
	  		
	  			sobrantes--;
	  		}
	  		
	  		for (i = 0; i < nCentros; i++) {
		  		printf("Fábrica %i entrega %i vacunas en el centro %i.\n", nFabrica+1, vacunas[i], i+1);
		  		fprintf(fdSalida,"Fábrica %i entrega %i vacunas en el centro %i.\n", nFabrica+1, vacunas[i], i+1);
			} 
		}

		if (nVacunasFabricas[nFabrica] == 0) { // Si hemos llegado al límite de vacunas preparadas por cada fábrica
			printf("Fábrica %i ha fabricado todas sus vacunas.\n", nFabrica+1);
			fprintf(fdSalida,"Fábrica %i ha fabricado todas sus vacunas.\n", nFabrica+1);
		}

		pthread_mutex_unlock(&mutex); // Salimos de la seccion critica

		for (i = 0; i < nCentros; i++) { // Tras el reparto de vacunas, nos aseguramos que todos los centros estén disponibles
			pthread_cond_signal(&centroDisponible[i]);
		}
		
		free(vacunas);
	}

	pthread_exit(NULL);

}

void valoresGlobales (FILE *fd) {

	nHabitantes = cogerValores(fd);
	nVacunasIniciales = cogerValores(fd);
	nMinVacunasTanda = cogerValores(fd);
	nMaxVacunasTanda = cogerValores(fd);
	tMinFabricacionTanda = cogerValores(fd);
	tMaxFabricacionTanda = cogerValores(fd);
	tMaxReparto = cogerValores(fd);
	tMaxDarseCuenta = cogerValores(fd);
	tMaxDesplazamiento = cogerValores(fd);
}

void imprimirDatos () {

	nVacunadosTanda = nHabitantes/nTandas;
	nVacunasTotalesFabrica = nHabitantes/nFabricas;

	printf("VACUNACION EN PANDEMIA: CONFIGURACION INICIAL \n");
    	fprintf(fdSalida,"VACUNACION EN PANDEMIA: CONFIGURACION INICIAL \n");
        printf("Habitantes: %i \n", nHabitantes);
        fprintf(fdSalida,"Habitantes: %i \n", nHabitantes);
        printf("Centros de Vacunacion: %i \n", nCentros);
        fprintf(fdSalida,"Centros de Vacunacion: %i \n", nCentros);
        printf("Fabricas: %i \n", nFabricas);
        fprintf(fdSalida,"Fabricas: %i \n", nFabricas);
        printf("Vacunados por tandas: %i \n", nVacunadosTanda);
        fprintf(fdSalida,"Vacunados por tandas: %i \n", nVacunadosTanda);
        printf("Vacunas Iniciales en cada centro: %i \n", nVacunasIniciales);
        fprintf(fdSalida,"Vacunas Iniciales en cada centro: %i \n", nVacunasIniciales);
        printf("Vacunas Totales por fabrica: %i \n", nVacunasTotalesFabrica);
        fprintf(fdSalida,"Vacunas Totales por fabrica: %i \n", nVacunasTotalesFabrica);
	printf("Mínimo número de vacunas fabricadas en cada tanda: %i \n",nMinVacunasTanda);
        fprintf(fdSalida, "Mínimo número de vacunas fabricadas en cada tanda: %i \n",nMinVacunasTanda);
        printf("Máximo número de vacunas fabricadas en cada tanda: %i \n",nMaxVacunasTanda);
        fprintf(fdSalida,"Máximo número de vacunas fabricadas en cada tanda: %i \n",nMaxVacunasTanda);
        printf("Tiempo mínimo de fabricación de una tanda de vacunas: %i \n", tMinFabricacionTanda);
        fprintf(fdSalida,"Tiempo mínimo de fabricación de una tanda de vacunas: %i \n", tMinFabricacionTanda);
        printf("Tiempo máximo de fabricación de una tanda de vacunas: %i \n", tMaxFabricacionTanda);
        fprintf(fdSalida,"Tiempo máximo de fabricación de una tanda de vacunas: %i \n", tMaxFabricacionTanda);
        printf("Tiempo máximo de reparto de vacunas a los centros: %i \n", tMaxReparto);
        fprintf(fdSalida,"Tiempo máximo de reparto de vacunas a los centros: %i \n", tMaxReparto);
        printf("Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i \n", tMaxDarseCuenta);
        fprintf(fdSalida,"Tiempo máximo que un habitante tarda en ver que está citado para vacunarse: %i \n", tMaxDarseCuenta);
        printf("Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i \n",tMaxDesplazamiento);
        fprintf(fdSalida,"Tiempo máximo de desplazamiento del habitante al centro de vacunación: %i \n",tMaxDesplazamiento);
        printf("\n");
        fprintf(fdSalida,"\n");
        printf("PROCESO DE VACUNACION\n");
        fprintf(fdSalida,"PROCESO DE VACUNACION\n");
}

void imprimirEstadisticas () {

	int i,j;
	
	nVacunasTotalesFabrica = nHabitantes/nFabricas;

	printf("Estadisticas:\n");
	fprintf(fdSalida, "Estadisticas:\n");

	for (i = 0; i < nFabricas; i++) {
		printf("\nFabrica %i:\n", i+1);
		fprintf(fdSalida, "\nFabrica %i:\n", i+1);
		printf("Ha fabricado %i vacunas.\n", nVacunasTotalesFabrica-nVacunasFabricas[i]);
		fprintf(fdSalida, "Ha fabricado %i vacunas.\n", nVacunasTotalesFabrica-nVacunasFabricas[i]);
		for (j = 0; j < nCentros; j++) {
			printf("Vacunas entregadas al centro %i: %i.\n", j+1, nVacunasFabricasCentros[i][j]);
			fprintf(fdSalida, "Vacunas entregadas al centro %i: %i.\n", j+1, nVacunasFabricasCentros[i][j]);
		}
	}

	for (i = 0; i < nCentros; i++) {

		printf("\nCentro %i:\n", i+1);
		fprintf(fdSalida, "\nCentro %i:\n", i+1);
		printf("Ha recibido %i vacunas.\n", nVacunasCentrosRepartidos[i]);
		fprintf(fdSalida, "Ha recibido %i vacunas.\n", nVacunasCentrosRepartidos[i]);
		printf("Se han vacunado %i habitantes.\n", (nVacunasCentrosRepartidos[i]+nVacunasIniciales) - nVacunasCentros[i]);
		fprintf(fdSalida, "Se han vacunado %i habitantes.\n", (nVacunasCentrosRepartidos[i]+nVacunasIniciales) - nVacunasCentros[i]);
		printf("Han sobrado %i vacunas.\n", nVacunasCentros[i]);
		fprintf(fdSalida, "Han sobrado %i vacunas.\n", nVacunasCentros[i]);
	}
}

int cogerValores (FILE *fd) {

	int valor;
	char buffer[100];

	fgets(buffer,6,fd);

	valor = atoi(buffer);

	return valor;	
}

int menosVacunas(int *nVacunasCentro) {

	int minimo = nVacunasCentro[0];     
	int ind = 0;
	
	for (int i = 0; i < nCentros; i++) {         
		if (nVacunasCentro[i] < minimo) {
			minimo = nVacunasCentro[i];
			ind = i;
		}
	}
	return ind;
}

