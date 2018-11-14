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
#define BUFFER_SIZE 800



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
		return ( getCpuCount() / 2) - 1;
	}else{
		return getCpuCount() / 2;
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
typedef union uni_maze
{
    struct maze_copy
    { 
		int lines;
		int columns;
		int start_line;
		int start_col;
		int end_line;
		int end_col;
		int maze[BUFFER_SIZE];
    };
    int8_t bytes[BUFFER_SIZE+6];
};


//BUFFER CONVERSION
void maze_to_buffer(struct maze_s* maze, int8_t* buffer){
	//memcpy(buffer,(const unsigned char*)&maze,sizeof(maze));
}

void buffer_to_maze(struct maze_s* maze, int8_t* buffer){
	//memcpy(maze,(const unsigned char*)&buffer,sizeof(buffer));
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
	
	printf("\nMASTER SENDER => CREATING COMMUNICATION");

	//Cria comunicação enviar trabalho
	create_communication(MASTER_SEND_PORT);
	int8_t buffer[BUFFER_SIZE];
	uint16_t cpu, task, size;
	int16_t val;
	int32_t i;


	//Variaveis auxiliares
	int maze_pos = 0;
	int cpu_id = 0;


	//Envia todos os labirintos para os escravos
	while( maze_pos < getTotalMazes() ){

		i = hf_recvprobe();
		if( i < 0 ) continue;
		
		printf("\n\nMASTER SENDER => READY TO RECEIVE");	

		//Recebe pedido de labirinto
		val = hf_recv(&cpu, &task, buffer, &size, 0);
		printf("\nMASTER SENDER => RECEIVED REQUEST from SLAVE at CPU %d", cpu);	


		maze_to_buffer( &mazes[maze_pos], buffer);

		printf("\nMASTER SENDER => SENDING MAZE | cpu: %d | size: %d\n", cpu, sizeof(buffer));

		// Envia buffer para CPU escrava, na porta do escravo
		val = hf_send(cpu, SLAVE_PORT, buffer, sizeof(buffer), 0);

		//PRINTA ERRO
		if (val)
			printf("\nMASTER SENDER => FAILED SEND | error %d\n", val);

		maze_pos++;

/*
		// Caso seja CPU que queremos mandar, seja o mestre, então pula
		if( cpu_id == getMasterCpuId() ){
			cpu_id++;
			continue;
		}
		// Converte labirinto para buffer = mazes[maze_pos];
		maze_to_buffer( &mazes[maze_pos], buffer);

		printf("\nMASTER SENDER => SENDING | cpu: %d | size: %d\n", cpu_id, sizeof(buffer));

		// Envia buffer para CPU escrava, na porta do escravo
		val = hf_send(cpu, SLAVE_PORT, buffer, sizeof(buffer), 0);


		//PRINTA ERRO
		if (val)
			printf("\nMASTER SENDER => FAILED SEND | error %d\n", val);
		
		// Incrementa contadores
		cpu_id++;
		maze_pos++;

		// Caso o numero de labirintos seja maior que 
		// a quantidade de cpu disponivel, recomeça a contagem
		if( cpu_id == getCpuCount()){
			cpu_id = 0;
		}

		*/
	}

	destroy_communication();


}
void task_master_receiver(){

	printf("\nMASTER RECEIVER => CREATING COMMUNICATION");

	//Cria comunicação para receber trabalho pronto
	create_communication(MASTER_RECV_PORT);
	uint16_t cpu, task, size;
	int16_t val;
	int32_t i;


	//Variaveis auxiliares
	int8_t buffer[BUFFER_SIZE];
	int maze_solved_total = 0;
	struct maze_s * solvedMaze;


	while (maze_solved_total < getTotalMazes()){

		i = hf_recvprobe();
		if( i < 0 ) continue;

		printf("\n\nMASTER RECEIVER => READY TO RECEIVE");	

		//Recebe de qualquer slave a task resolvida
		val = hf_recv(&cpu, &task, buffer, &size, 0);

		//PRINTA ERRO
		if (val)
			printf("\nMASTER RECEIVER => FAILED RECV | error %d\n", val); 		//else printf("%s", buffer);


		printf("\nMASTER RECEIVER => RECEIVED SOLVED | cpu: %d\n", cpu); 		//else printf("%s", buffer);

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
		hf_send(getMasterCpuId(), MASTER_SEND_PORT, buffer, sizeof(buffer), 0);

		while(hf_recvprobe()<0){
			continue;
		}

		//Recebe labirinto
		val = hf_recv(&cpu, &task, buffer, &size, 0);

		//PRINTA ERRO
		if (val)
			printf("\nSLAVE => FAILED RECV | error %d\n", val); 		//else printf("%s", buffer);
		
		//Converte o buffer para labirinto
		buffer_to_maze(m, buffer);


		//Resolve labirinto
		printf("\nSLAVE => SOLVING MAZE | lines: %d | columns: %d", m->lines, m->columns);

		delay_ms(200);

		//int s = solve(m->maze, m->lines, m->columns, m->start_line, m->start_col, m->end_line, m->end_col);		
		
		//Envia labirinto para mestre receptor
		printf("\nSLAVE => SENDING MAZE TO MASTER| CPU: %d | PORT: %d", getMasterCpuId(),MASTER_RECV_PORT);
		hf_send(getMasterCpuId(), MASTER_RECV_PORT, buffer, sizeof(buffer), 0);
		
		
		/*
		printf("\nSLAVE => READY TO RECEIVE");

		//Recebe labirinto
		val = hf_recv(&cpu, &task, buffer, &size, 0);

		//PRINTA ERRO
		if (val)
			printf("\nSLAVE => FAILED RECV | error %d\n", val); 		//else printf("%s", buffer);

		//Converte o buffer para labirinto
		buffer_to_maze(m, buffer);



		printf("\nSLAVE => SOLVING MAZE | lines: %d | columns: %d", m->lines, m->columns);
		//Resolve labirinto
		delay_ms(200);
		int s = solve(m->maze, m->lines, m->columns, m->start_line, m->start_col, m->end_line, m->end_col);
		if (s) {
			printf("\nOK!\n");
		} else {
			printf("\nERROR!\n");
		};

		//Converte o labirinto para buffer
		maze_to_buffer(m, buffer);


		printf("\nSLAVE => SENDING MAZE TO MASTER| CPU: %d | PORT: %d", getMasterCpuId(),MASTER_RECV_PORT);
		//Envia labirinto
		hf_send(getMasterCpuId(), MASTER_RECV_PORT, buffer, sizeof(buffer), 0);
		*/
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
		hf_spawn(task_slave, 0, 0, 0, "slave", 8192);
	}
}
// https://stackoverflow.com/questions/19165134/correct-portable-way-to-interpret-buffer-as-a-struct