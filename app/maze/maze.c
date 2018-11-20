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
#define BUFFER_SIZE 16384



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
	return 0;
	
	/*if( getCpuCount() % 2 == 0){
		return ( getCpuCount() / 2) - 1;
	}else{
		return getCpuCount() / 2;
	}*/
	
}

int getMasterReceiverCpuId(){
	return getCpuCount() -1;
	//return getMasterCpuId();
}

int isMaster(){
	return getCpuId() == getMasterCpuId();
}

int isMasterReceiver(){
	return getCpuId() == getMasterReceiverCpuId();
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
		//printf("[%d,%d] ", i, j);
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
struct maze_copy
{ 
	int lines;
	int columns;
	int start_line;
	int start_col;
	int end_line;
	int end_col;
	int maze[BUFFER_SIZE];
} ;

union uni_maze
{	
	struct maze_copy maze;
    int8_t bytes[sizeof(struct maze_copy)];
};


//BUFFER CONVERSION
void maze_to_buffer(struct maze_s* maze, union uni_maze* maze_uni){
	maze_uni->maze.lines = maze->lines;
	maze_uni->maze.columns = maze->columns;
	maze_uni->maze.start_line = maze->start_line;
	maze_uni->maze.start_col = maze->start_col;
	maze_uni->maze.end_line = maze->end_line;
	maze_uni->maze.end_col = maze->end_col;
	memcpy(maze_uni->maze.maze, maze->maze, maze->lines*maze->columns*4);
}

void buffer_to_maze(struct maze_s* maze, int8_t* buffer){

	union uni_maze maze_uni;

	memcpy(maze_uni.bytes, buffer, sizeof(struct maze_copy));

	maze->lines = maze_uni.maze.lines;
	maze->columns = maze_uni.maze.columns;
	maze->start_line = maze_uni.maze.start_line;
	maze->start_col = maze_uni.maze.start_col;
	maze->end_line = maze_uni.maze.end_line;
	maze->end_col = maze_uni.maze.end_col;
	
	memcpy(maze->maze, maze_uni.maze.maze, maze->lines*maze->columns*4);

}

//COMMUNICATION
void create_communication(int port){
	if (hf_comm_create(hf_selfid(), port, 0)){
		
		if( port == MASTER_SEND_PORT ){
			printf("\nMASTER SENDER => FAILED CREATE COMMUNICATION");
		}


		if( port == MASTER_RECV_PORT ){
			printf("\nMASTER RECEIVER => FAILED CREATE COMMUNICATION");
		}


		if( port == SLAVE_PORT){
			printf("\nSLAVE => FAILED CREATE COMMUNICATION");
		}

		panic(0xff);

	}
}

void destroy_communication(){
	hf_comm_destroy(hf_selfid());
	hf_kill(hf_selfid());
}









////////////////////////////////////////
//// TASKS
////////////////////////////////////////

void task_master_sender(){
	printf("\n###############################################");
	printf("\n### MESTRE ENVIA INICIADO ");
	printf("\n###############################################");

	printf("\n\nMASTER SENDER => CREATING COMMUNICATION");

	//Cria comunicação enviar trabalho
	create_communication(MASTER_SEND_PORT);
	int8_t buffer[BUFFER_SIZE];
	uint16_t cpu, task, size;
	int16_t val;
	int32_t channel;


	//Variaveis auxiliares
	int maze_pos = 0;
	int cpu_id = 0;


	//Envia todos os labirintos para os escravos
	while( maze_pos < getTotalMazes() ){

		channel = hf_recvprobe();
		if( channel < 0 ) continue;
		
		printf("\n\nMASTER SENDER => READY TO RECEIVE");	

		//Recebe pedido de labirinto
		val = hf_recv(&cpu, &task, buffer, &size, channel);
		printf("\nMASTER SENDER => RECEIVED REQUEST from SLAVE at CPU %d | Channel:%d", cpu, channel);	

		if( val ){
			printf("\nMASTER SENDER ==> FAILED TO RECEIVED REQUEST | error %d\n", val);
		}

		union uni_maze uni_aux;
		maze_to_buffer( &mazes[maze_pos], &uni_aux);

		int send_size =  (10 * 4) + ( mazes[maze_pos].lines * mazes[maze_pos].columns * 4 );

		printf("\nMASTER SENDER => SENDING MAZE (%d) | cpu: %d | size: %d\n", maze_pos, cpu, send_size);


		// Envia buffer para CPU escrava, na porta do escravo
		val = hf_send(cpu, SLAVE_PORT, uni_aux.bytes, send_size, channel);

		//PRINTA ERRO
		if (val)
			printf("\nMASTER SENDER => FAILED SEND MAZE | error %d\n", val);

		maze_pos++;

	}
	printf("\n###############################################");
	printf("\n### TODOS LABIRINTOS ENVIADOS ");
	printf("\n###############################################");
	printf("\n\n\n\n\n\n\n\n\n");

	destroy_communication();


}
void task_master_receiver(){

	uint32_t tempo_inicial = hf_ticktime();

	printf("\n###############################################");
	printf("\n### MESTRE RECEBE INICIADO ");
	printf("\n###############################################");

	printf("\nMASTER RECEIVER => CREATING COMMUNICATION");

	//Cria comunicação para receber trabalho pronto
	create_communication(MASTER_RECV_PORT);
	uint16_t cpu, task, size;
	int16_t val;
	int32_t channel;


	//Variaveis auxiliares
	int8_t buffer[BUFFER_SIZE];
	int maze_solved_total = 0;
	struct maze_s * solvedMaze;

	
	//Enquanto não resolver todos labirintos
	while (maze_solved_total < getTotalMazes()){

		//Fica esperando receber
		channel = hf_recvprobe();
		if( channel < 0 ) continue;

		printf("\n\nMASTER RECEIVER => READY TO RECEIVE");	

		//Recebe de qualquer slave a task resolvida
		val = hf_recv(&cpu, &task, buffer, &size, channel);

		//PRINTA ERRO
		if (val)
			printf("\nMASTER RECEIVER => FAILED RECV | error %d\n", val); 		//else printf("%s", buffer);


		maze_solved_total++;

		//Converte o buffer na struct
		buffer_to_maze(solvedMaze, buffer);
	
		printf("\nMASTER RECEIVER => RECEIVED SOLVED | cpu: %d | TotalSolved: %d | lines:%d | columns:%d\n", cpu, maze_solved_total, solvedMaze->lines, solvedMaze->columns); 		//else printf("%s", buffer);


	}

	
	uint32_t tempo_final = hf_ticktime();
	int convert_to_sec = 1000*1000*1000;
	printf("\n###############################################");
	printf("\n### TODOS LABIRINTOS RECEBIDOS ");
	printf("\n### TEMPO TOTAL : %d", (tempo_final - tempo_inicial)/(convert_to_sec) );
	printf("\n###############################################");
	printf("\n\n\n\n\n\n\n\n\n");

	destroy_communication();

}

void task_slave(){

	//Cria comunicação para receber trabalho, e enviar pronto
	printf("\nSLAVE => CREATING COMMUNICATION");
	create_communication(SLAVE_PORT);
	uint16_t cpu, task, size;
	int16_t val;
	int32_t i;

	//Variaveis
	int8_t buffer[BUFFER_SIZE];
	struct maze_s * m; //maze to solve
	
	//Para sempre? //TODO: Fazer calculo de quantos eu(escravo) irei resolver?
	while(1){

		//SEND REQUEST OF MAZE
		printf("\n\nSLAVE => REQUESTING MAZE TO SOLVE | MASTER: %d | PORT: %d", getMasterCpuId(), MASTER_SEND_PORT );
		hf_send(getMasterCpuId(), MASTER_SEND_PORT, 0, 0, getCpuId());

		//Recebe labirinto
		val = hf_recv(&cpu, &task, buffer, &size, getCpuId());

		//PRINTA ERRO
		if (val)
			printf("\nSLAVE => FAILED RECV | error %d\n", val); 		//else printf("%s", buffer);
		
		//Converte o buffer para labirinto
		buffer_to_maze(m, buffer);


		//Resolve labirinto
		printf("\nSLAVE => SOLVING MAZE | lines: %d | columns: %d | start_line: %d | start_col: %d", m->lines, m->columns, m->start_line, m->start_col);

		int s = solve(m->maze, m->lines, m->columns, m->start_line, m->start_col, m->end_line, m->end_col);		
		
		if(s){
			printf("\nSLAVE => SOLVED");
		}else{
			printf("\nSLAVE => FAILED TO SOLVE");
		}//delay_ms(x);


		

		//Envia labirinto para mestre receptor
		printf("\nSLAVE => SENDING MAZE TO MASTER RECEIVER | CPU: %d | PORT: %d | Channel: %d", getMasterReceiverCpuId(),MASTER_RECV_PORT, getCpuId());
		
		union uni_maze uni_aux;
		maze_to_buffer(m, &uni_aux);
		
		hf_send(getMasterReceiverCpuId(), MASTER_RECV_PORT, uni_aux.bytes, size, getCpuId());
		

	}


}


////////////////////////////////////////
//// MAIN
////////////////////////////////////////
void app_main(void)
{
	if (isMaster()){
		hf_spawn(task_master_sender, 0, 0, 0, "master_sender", 256 * 1024 );
	}
	
	else if( isMasterReceiver()){
		hf_spawn(task_master_receiver, 0, 0, 0, "master_receiver",  256 * 1024);
	}
	
	else{
		hf_spawn(task_slave, 0, 0, 0, "slave",  256 * 1024);
	}
}
// https://stackoverflow.com/questions/19165134/correct-portable-way-to-interpret-buffer-as-a-struct