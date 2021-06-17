#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ���� ���� ��ó���� */
#define LAB 10 // ���� Ƚ�� (���� ���ѻ���)
#define K 9	// Cluster ���� K
#define MAX_RUN 20	// �ִ� ���� Ƚ�� (���� ���ѻ���)
#define RAND_SEED 77777	// rand()�� ���� SEED�� ����

#define TITLE 500	// Title 500��
#define WORD 4423	// Word 4423��
#define STRLEN 500	// ���ڿ� ���� ����

#define FILE_TITLE "lTitle.txt"	// Title List Location
#define FILE_WORD "lWord.txt"	// Word List Location
#define FILE_DATA "vData.txt"	// Coefficient Vector Location


/* ����ü ���� */
struct Word {	// �ܾ� ����ü
	double coefficient;
	char str[STRLEN];
};

struct Cluster {	// Ŭ������ ����ü
	struct Word *list;
	int len;
};

struct Title {	// ���� ����ü
	struct Word *list;
	int label;
	char str[STRLEN];
};


/* �Լ� ���� */
double init_seed() {	// Ŭ������ �ʱ� ������ �Լ�
	const int rate = 100000000;	// 10^8 ���� ����
	return ((double)(rand() % rate)) / rate;
}

// Title�� ���� Word�� ���Ͽ� Distance�� �����ϴ� �Լ�
double distance_calc(struct Word *w1, int len1, struct Word *w2, int len2) {
	double result = 0.0;

	if (len1 != len2) return -1.0;	// ������ ȣ��

	for (int i = 0; i < len1; ++i) {
		// ���̰�^2�� ����
		// sqrt(dist1) == sqrt(dist2) ����� dist1 == dist2�� ����� ����
		result += pow((w1[i].coefficient - w2[i].coefficient), 2.0);
	}

	return result;
}

int Title_Stats(); // Title ���� Ȯ�� �Լ�. ���ϴܿ� ������


/* �������� ���� */
struct Title *arr;		// �����͸� �����ϴ� ����. 500x4423
struct Cluster *cluster;	// Ŭ�����͸� �����ϴ� ����. K�� ����


int main() {
	// ��������¿� ����
	FILE* file_titleList = fopen(FILE_TITLE, "r");
	FILE* file_wordList = fopen(FILE_WORD, "r");
	FILE* file_dataList = fopen(FILE_DATA, "r");

	char input[STRLEN];


	// Title �Է�
	arr = (struct Title *)malloc(TITLE * sizeof(struct Title));	// Title ����ŭ ���� �Ҵ�

	for (int i = 0; i < TITLE; ++i) {
		arr[i].list = (struct Word *)malloc(WORD * sizeof(struct Word));	// Title �� Word����ŭ�� Word List �����Ҵ�

		fscanf(file_titleList, " %s", input);
		strcpy(arr[i].str, input);		// Title List ����
	}

	// Word �Է�
	for (int i = 0; i < WORD; ++i) {
		fscanf(file_wordList, " %s", input);

		for (int j = 0;j < TITLE; ++j) {	// WORD�� ������ Title list�� ����
			strcpy(arr[j].list[i].str, input);
		}
	}

	// Coefficient Vector �Է�
	for (int i = 0; i < WORD; ++i) {
		for (int j = 0;j < TITLE; ++j) {
			fscanf(file_dataList, " %le", &arr[j].list[i].coefficient);		// Coefficient �� ����
		}
	}


	// ��������¿� ���� Release
	fclose(file_titleList);
	fclose(file_wordList);
	fclose(file_dataList);


	// rand()�� ���� SEED�� ����
	srand(RAND_SEED);


	// Algorithm Start!
	// ���� Ƚ����ŭ ��ȸ
	for (int lab = 0; lab < LAB; ++lab) {

		// Cluster �ʱⰪ ����
		cluster = (struct Cluster *)malloc(K * sizeof(struct Cluster));		// Cluster �����Ҵ�

		for (int k = 0; k < K; ++k) {
			cluster[k].list = (struct Word *)malloc(WORD * sizeof(struct Word));	// Cluster �� �߽ɰ� �����Ҵ�

			for (int j = 0; j < WORD; ++j) {
				cluster[k].list[j].coefficient = init_seed();	// Cluster �ʱⰪ�� �����ϰ� ����
			}
		}


		// MAX_RUN ȸ���� Update Partition -> Update Centroid �ݺ�
		for (int iter = 0; iter < MAX_RUN; ++iter) {

			// Init Cluster
			for (int k = 0; k < K; ++k) {
				cluster[k].len = 0;
			}


			// Update Partition
			// Node�� 1~ K ������ Ŭ�����Ϳ� ����(Labeling)
			for (int i = 0; i < TITLE; ++i) {
				int index = INT_MIN;
				double compare = 3.402823e+38; // DOUBLE_MAX

				for (int k = 0; k < K; ++k) {	// �Ѱ��� Node�� K���� Cluster�� �Ÿ� ��
					double distance = distance_calc(cluster[k].list, WORD, arr[i].list, WORD);

					if (distance < compare) {	// �� ���� ���� ª�� �Ÿ��� ���� Ŭ�������� ��
						index = k;
						compare = distance;
					}
				}

				cluster[index].len++;	// Cluster�� �ش� Cluster�� ��� Node�� �ִ��� ������Ʈ
				arr[i].label = index;	// Labeling
			}


			// Print Variable
			double jClust = 0.0;	// J-Clust ��
			double jClust_array[K] = { 0, };	// J1 ~ Jk

			for (int i = 0; i < TITLE; ++i) {	// J-Clust �� ������ ���� �� distance Ȯ��
				double distance = distance_calc(cluster[arr[i].label].list, WORD, arr[i].list, WORD);

				jClust += distance;
				jClust_array[arr[i].label] += distance;
			}
			jClust /= TITLE;

			/* TODO HERE */
			// Print J-Clust Value
			//printf("%e\n", jClust);

			// Print J1 ~ Jk Value
			// for (int k = 0;k < K;++k) {
			// 	printf("%e\t", jClust_array[k] / cluster[k].len);
			// } printf("\n");

			// Print Cluster Len (1 ~ K)
			// for (int k = 0;k < K;++k) {
			// 	printf("%d\t", cluster[k].len);
			// } printf("\n");
			/* TODO HERE */


			// Update Centroid
			// Cluster�� �������� �߽ɰ��� �缳��
			double cluster_centroid[K][WORD] = { 0.0, };

			for (int i = 0; i < TITLE; ++i) { // TITLE�� ��ȸ�ϸ� ���� �ùٸ� Cluster�� �߽ɰ��� ������Ʈ��
				for (int j = 0;j < WORD; ++j) {	// Cluster�� �߽ɰ��� ��ȸ�ϸ� coefficient �� Ȯ��
					cluster_centroid[arr[i].label][j] += arr[i].list[j].coefficient;
				}
			}

			for (int k = 0; k < K; ++k) {
				for (int j = 0;j < WORD; ++j) {	// �߽ɰ� Update
					cluster[k].list[j].coefficient = (cluster_centroid[k][j] / cluster[k].len);
				}
			}

		}

		Title_Stats(); // Title ��� Ȯ��


		// ���� �Ҵ� ����
		for (int i = 0; i < K; ++i) {
			free(cluster[i].list);	// Cluster�� �߽ɰ� Release
		}
		free(cluster);	// Cluster Release

	}

	for (int i = 0; i < TITLE; ++i) {
		free(arr[i].list);	// Word List Release
	}
	free(arr);	// Title List Release


	return 0;
}


// ���� ���� Ȯ�� �Լ�
int Title_Stats() {	
	struct Word* title_array;

	for (int k = 0; k < K;++k) {	// �� Cluster Ȯ��
		int index = 0;
		title_array = (struct Word *)malloc(sizeof(struct Word) * cluster[k].len);	// Cluster �� Node�� ������ŭ �����Ҵ�

		for (int i = 0; i < TITLE; ++i) {	// �� Title�� ��ȸ�ϸ� Ȯ��
			if (arr[i].label != k) continue;	// Label�� Cluster�� ��ġ���� ������ ���

			// �� ����
			strcpy(title_array[index].str, arr[i].str);
			title_array[index++].coefficient = distance_calc(arr[i].list, WORD, cluster[k].list, WORD);
		}


		if (index != cluster[k].len) { // Unknown Error
			free(title_array);
			continue;
		}

		/* TODO HERE */
		// Print Title Information
		// printf("Cluster %d || Len %d\n", k + 1, index);
		// for (int i = 0;i < index; ++i) {
		// 	printf("%02d | %10lf %s\n", i + 1, title_array[i].coefficient, title_array[i].str);
		// } printf("\n");
		/* TODO HERE */


		free(title_array);	// �����Ҵ� ����
	}


	return 0;
}