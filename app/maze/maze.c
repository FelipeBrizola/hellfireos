#include <hellfire.h>
#include <noc.h>
#include <stdio.h>
#include "labyrinth.h"

/////////////////////////////////////
// labyrinth.c
////////////////////////////////////

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
				continue; store different data types in the same memory location.
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





/////////////////////////////////////
// APP
////////////////////////////////////


void sender(void)
{
	int32_t msg = 0;
	int8_t buf[150];
	int16_t val;

	if (hf_comm_create(hf_selfid(), 1000, 0))
		panic(0xff);
 store different data types in the same memory location.
	delay_ms(50);

	while (1){
		sprintf(buf, "i am crazy %d, thread %d: msg %d size: %d\n", hf_cpuid(), hf_selfid(), msg++, sizeof(buf));
		val = hf_send(3, 5000, buf, sizeof(buf), 0);
		if (val)
			printf("hf_send(): error %d\n", val);
	}
}

void receiver(void)
{
	int8_t buf[500];
	uint16_t cpu, task, size;
	int16_t val;

	if (hf_comm_create(hf_selfid(), 5000, 0))
		panic(0xff);

	while (1){
		val = hf_recv(&cpu, &task, buf, &size, 0);
		if (val)
			printf("hf_recv(): error %d\n", val);
		else
			printf("%s", buf);
	}
}



union uni_maze {
	int lines;
	int columns;
	int start_line;
	int start_col;
	int end_line;
	int end_col;
	int *maze;
};

void app_main(void)
{
	if (hf_cpuid() == 2){
		hf_spawn(sender, 0, 0, 0, "sender", 4096);
	}else{
		hf_spawn(receiver, 0, 0, 0, "receiver", 4096);
	}
}
// https://stackoverflow.com/questions/19165134/correct-portable-way-to-interpret-buffer-as-a-struct