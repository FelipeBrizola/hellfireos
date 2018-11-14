#include <hellfire.h>
#include <noc.h>
#include "labyrinth.h"

////////////////////////////////////////
//// CONSTANTS
////////////////////////////////////////

#define MASTER_SEND_PORT 3000
#define MASTER_RECV_PORT 4000
#define SLAVE_PORT 5000
#define TOTAL_MAZES 15
#define BUFFER_SIZE 1500



int getTotalMazes(){
	return TOTAL_MAZES;
}

int getCpuCount(){
	return hf_ncores();
}

int getCpuId(){
	return hf_cpuid();
}

////////////////////////////////////////
//// MASTER
////////////////////////////////////////

int getMasterCpuId(){
	if( getCpuCount() % 2 == 0){
		return getCpuCount() / 2;
	}else{
		return (getCpuCount() / 2) + 1;
	}
}

int isMaster(){
	return getCpuId() == getMasterCpuId();
}



////////////////////////////////////////
//// LABYRINTH METHODS
////////////////////////////////////////

void show(int *m, int lin, int col)
{
	int i, j;
	
	for (i = 0; i < lin; i++) {
		for (j = 0; j < col; j++) {
			printf("%3d ", m[i * col + j]);
		}
		printf("\n");
	}
	printf("\n");
}

void mark(int *m, int l, int c, int lin, int col)
{
	int h, i, j, k = 1;
	
	for (h = 0; h < lin * col; h++, k++) {
		for (i = 0; i < lin; i++) {
			for (j = 0; j < col; j++) {
				if (m[i * col + j] == k) {
					if (i - 1 >= 0) {
						if (m[(i - 1) * col + j] == 0)
							m[(i - 1) * col + j] = k + 1;
					}
					if (i + 1 < lin) {
						if (m[(i + 1) * col + j] == 0)
							m[(i + 1) * col + j] = k + 1;
					}
					if (j - 1 >= 0) {
						if (m[i * col + (j - 1)] == 0)
							m[i * col + (j - 1)] = k + 1;
					}
					if (j + 1 < col) {
						if (m[i * col + (j + 1)] == 0)
							m[i * col + (j + 1)] = k + 1;
					}
				}
			}
		}
	}
}

int search(int *m, int i, int j, int ei, int ej, int lin, int col)
{
	int k = 2;
	
	while (k > 1) {
		k = m[i * col + j];
		printf("[%d,%d] ", i, j);
		if (i - 1 >= 0) {
			if (m[(i - 1) * col + j] < k && m[(i - 1) * col + j] > 0) {
				i--;
				continue;
			}
		}
		if (i + 1 < lin) {
			if (m[(i + 1) * col + j] < k && m[(i + 1) * col + j] > 0) {
				i++;
				continue;
			}
		}
		if (j - 1 >= 0) {
			if (m[i * col + (j - 1)] < k && m[i * col + (j - 1)] > 0) {
				j--;
				continue;
			}
		}
		if (j + 1 < col) {
			if (m[i * col + (j + 1)] < k && m[i * col + (j + 1)] > 0) {
				j++;
				continue;
			}
		}
	}
	if (i == ei && j == ej)
		return 1;
	else
		return 0;
}

int solve(int *m, int lin, int col, int si, int sj, int ei, int ej)
{
	m[ei * col + ej] = 1;
	mark(m, ei, ej, lin, col);
	/* show(m, lin, col); */
	return search(m, si, sj, ei, ej, lin, col);
}


////////////////////////////////////////
//// UTILITY
////////////////////////////////////////

union uni_maze {
	int lines;
	int columns;
	int start_line;
	int start_col;
	int end_line;
	int end_col;
	int *maze;
};


//BUFFER CONVERSION
void maze_to_buffer(struct maze_s* maze, int8_t* buffer){

}

void buffer_to_maze(struct maze_s* maze, int8_t* buffer){

}

//COMMUNICATION
void create_communication(int port){
	if (hf_comm_create(hf_selfid(), port, 0))
		panic(0xff);
}

void destroy_communication(){
	hf_comm_destroy(hf_selfid());
}





////////////////////////////////////////
//// TASKS
////////////////////////////////////////

void task_master_sender(){

	//Cria comunicação enviar trabalho
	create_communication(MASTER_SEND_PORT);

	//Variaveis auxiliares
	int8_t buffer[BUFFER_SIZE];
	int maze_pos = 0;
	int cpu_id = 0;

	//Envia todos os labirintos para os escravos
	while( maze_pos < getTotalMazes() ){

		// Caso seja CPU que queremos mandar, seja o mestre, então pula
		if( cpu_id == getMasterCpuId() ){
			cpu_id++;
			continue;
		}

		// Converte labirinto para buffer = mazes[maze_pos];
		maze_to_buffer( &mazes[maze_pos], buffer);

		// Envia buffer para CPU escrava, na porta do escravo
		hf_send(cpu_id, SLAVE_PORT, buffer, sizeof(buffer), 0);

		// Incrementa contadores
		cpu_id++;
		maze_pos++;

		// Caso o numero de labirintos seja maior que 
		// a quantidade de cpu disponivel, recomeça a contagem
		if( cpu_id == getCpuCount()){
			cpu_id = 0;
		}

	}

	destroy_communication();


}
void task_master_receiver(){

	//Cria comunicação para receber trabalho pronto
	create_communication(MASTER_RECV_PORT);

	//Variaveis auxiliares
	int8_t buffer[BUFFER_SIZE];
	int maze_solved_total = 0;
	uint16_t cpu, task, size;
	struct maze_s * solvedMaze;

	int16_t val;

	while (maze_solved_total < getTotalMazes()){

		//Recebe de qualquer slave a task resolvida
		val = hf_recv(&cpu, &task, buffer, &size, 0);


		//PRINTA ERRO
		if (val)
			printf("hf_recv(): error %d\n", val); 		//else printf("%s", buffer);


		maze_solved_total++;

		//Converte o buffer na struct
		buffer_to_maze(solvedMaze, buffer);


		//TODO: Faz alguma coisa com o buffer...

		//Question: qual maze foi resolvido? tenho que incluir um int para saber ou não importa...


	}


	destroy_communication();

}

void task_slave(){

	//Cria comunicação para receber trabalho, e enviar pronto
	create_communication(SLAVE_PORT);

	//Variaveis
	int8_t buffer[BUFFER_SIZE];
	uint16_t cpu, task, size;
	struct maze_s * m; //maze to solve
	int16_t val;


	//Para sempre? //TODO: Fazer calculo de quantos eu(escravo) irei resolver?
	while(1){

		//Recebe labirinto
		val = hf_recv(&cpu, &task, buffer, &size, 0);

		//Converte o buffer para labirinto
		buffer_to_maze(m, buffer);



		//Resolve labirinto
		int s = solve(m->maze, m->lines, m->columns, m->start_line, m->start_col, m->end_line, m->end_col);
		if (s) {
			printf("\nOK!\n");
		} else {
			printf("\nERROR!\n");
		};


		//Converte o labirinto para buffer
		maze_to_buffer(m, buffer);


		//Envia labirinto
		hf_send(getMasterCpuId(), MASTER_RECV_PORT, buffer, sizeof(buffer), 0);

	}


}


////////////////////////////////////////
//// MAIN
////////////////////////////////////////
void app_main(void)
{
	if (isMaster()){
		hf_spawn(task_master_sender, 0, 0, 0, "master_sender", 4096);
		hf_spawn(task_master_receiver, 0, 0, 0, "master_receiver", 4096);
	}
	else{
		hf_spawn(task_slave, 0, 0, 0, "slave", 4096);
	}
}
// https://stackoverflow.com/questions/19165134/correct-portable-way-to-interpret-buffer-as-a-struct