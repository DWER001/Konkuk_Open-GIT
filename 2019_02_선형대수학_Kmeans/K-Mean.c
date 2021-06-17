#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* 실행 관련 전처리기 */
#define LAB 10 // 실험 횟수 (과제 제한사항)
#define K 9	// Cluster 갯수 K
#define MAX_RUN 20	// 최대 실행 횟수 (과제 제한사항)
#define RAND_SEED 77777	// rand()에 대한 SEED값 정의

#define TITLE 500	// Title 500개
#define WORD 4423	// Word 4423개
#define STRLEN 500	// 문자열 길이 정의

#define FILE_TITLE "lTitle.txt"	// Title List Location
#define FILE_WORD "lWord.txt"	// Word List Location
#define FILE_DATA "vData.txt"	// Coefficient Vector Location


/* 구조체 정의 */
struct Word {	// 단어 구조체
	double coefficient;
	char str[STRLEN];
};

struct Cluster {	// 클러스터 구조체
	struct Word *list;
	int len;
};

struct Title {	// 제목 구조체
	struct Word *list;
	int label;
	char str[STRLEN];
};


/* 함수 정의 */
double init_seed() {	// 클러스터 초기 난수값 함수
	const int rate = 100000000;	// 10^8 으로 설정
	return ((double)(rand() % rate)) / rate;
}

// Title에 대한 Word를 비교하여 Distance를 측정하는 함수
double distance_calc(struct Word *w1, int len1, struct Word *w2, int len2) {
	double result = 0.0;

	if (len1 != len2) return -1.0;	// 비정상 호출

	for (int i = 0; i < len1; ++i) {
		// 차이값^2를 저장
		// sqrt(dist1) == sqrt(dist2) 결과는 dist1 == dist2의 결과와 같음
		result += pow((w1[i].coefficient - w2[i].coefficient), 2.0);
	}

	return result;
}

int Title_Stats(); // Title 정보 확인 함수. 최하단에 정의함


/* 전역변수 정의 */
struct Title *arr;		// 데이터를 저장하는 변수. 500x4423
struct Cluster *cluster;	// 클러스터를 저장하는 변수. K개 생성


int main() {
	// 파일입출력용 변수
	FILE* file_titleList = fopen(FILE_TITLE, "r");
	FILE* file_wordList = fopen(FILE_WORD, "r");
	FILE* file_dataList = fopen(FILE_DATA, "r");

	char input[STRLEN];


	// Title 입력
	arr = (struct Title *)malloc(TITLE * sizeof(struct Title));	// Title 개만큼 동적 할당

	for (int i = 0; i < TITLE; ++i) {
		arr[i].list = (struct Word *)malloc(WORD * sizeof(struct Word));	// Title 내 Word개만큼의 Word List 동적할당

		fscanf(file_titleList, " %s", input);
		strcpy(arr[i].str, input);		// Title List 저장
	}

	// Word 입력
	for (int i = 0; i < WORD; ++i) {
		fscanf(file_wordList, " %s", input);

		for (int j = 0;j < TITLE; ++j) {	// WORD를 각각의 Title list에 저장
			strcpy(arr[j].list[i].str, input);
		}
	}

	// Coefficient Vector 입력
	for (int i = 0; i < WORD; ++i) {
		for (int j = 0;j < TITLE; ++j) {
			fscanf(file_dataList, " %le", &arr[j].list[i].coefficient);		// Coefficient 값 저장
		}
	}


	// 파일입출력용 변수 Release
	fclose(file_titleList);
	fclose(file_wordList);
	fclose(file_dataList);


	// rand()에 대한 SEED값 설정
	srand(RAND_SEED);


	// Algorithm Start!
	// 실험 횟수만큼 순회
	for (int lab = 0; lab < LAB; ++lab) {

		// Cluster 초기값 설정
		cluster = (struct Cluster *)malloc(K * sizeof(struct Cluster));		// Cluster 동적할당

		for (int k = 0; k < K; ++k) {
			cluster[k].list = (struct Word *)malloc(WORD * sizeof(struct Word));	// Cluster 내 중심값 동적할당

			for (int j = 0; j < WORD; ++j) {
				cluster[k].list[j].coefficient = init_seed();	// Cluster 초기값을 랜덤하게 설정
			}
		}


		// MAX_RUN 회동안 Update Partition -> Update Centroid 반복
		for (int iter = 0; iter < MAX_RUN; ++iter) {

			// Init Cluster
			for (int k = 0; k < K; ++k) {
				cluster[k].len = 0;
			}


			// Update Partition
			// Node를 1~ K 사이의 클러스터에 배정(Labeling)
			for (int i = 0; i < TITLE; ++i) {
				int index = INT_MIN;
				double compare = 3.402823e+38; // DOUBLE_MAX

				for (int k = 0; k < K; ++k) {	// 한개의 Node를 K개의 Cluster와 거리 비교
					double distance = distance_calc(cluster[k].list, WORD, arr[i].list, WORD);

					if (distance < compare) {	// 현 기준 가장 짧은 거리를 가진 클러스터일 시
						index = k;
						compare = distance;
					}
				}

				cluster[index].len++;	// Cluster에 해당 Cluster에 몇개의 Node가 있는지 업데이트
				arr[i].label = index;	// Labeling
			}


			// Print Variable
			double jClust = 0.0;	// J-Clust 값
			double jClust_array[K] = { 0, };	// J1 ~ Jk

			for (int i = 0; i < TITLE; ++i) {	// J-Clust 값 산출을 위해 각 distance 확인
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
			// Cluster를 기준으로 중심값을 재설정
			double cluster_centroid[K][WORD] = { 0.0, };

			for (int i = 0; i < TITLE; ++i) { // TITLE번 순회하며 각각 올바른 Cluster의 중심값을 업데이트함
				for (int j = 0;j < WORD; ++j) {	// Cluster의 중심값을 순회하며 coefficient 값 확인
					cluster_centroid[arr[i].label][j] += arr[i].list[j].coefficient;
				}
			}

			for (int k = 0; k < K; ++k) {
				for (int j = 0;j < WORD; ++j) {	// 중심값 Update
					cluster[k].list[j].coefficient = (cluster_centroid[k][j] / cluster[k].len);
				}
			}

		}

		Title_Stats(); // Title 목록 확인


		// 동적 할당 해제
		for (int i = 0; i < K; ++i) {
			free(cluster[i].list);	// Cluster의 중심값 Release
		}
		free(cluster);	// Cluster Release

	}

	for (int i = 0; i < TITLE; ++i) {
		free(arr[i].list);	// Word List Release
	}
	free(arr);	// Title List Release


	return 0;
}


// 제목 정보 확인 함수
int Title_Stats() {	
	struct Word* title_array;

	for (int k = 0; k < K;++k) {	// 각 Cluster 확인
		int index = 0;
		title_array = (struct Word *)malloc(sizeof(struct Word) * cluster[k].len);	// Cluster 내 Node의 갯수만큼 동적할당

		for (int i = 0; i < TITLE; ++i) {	// 각 Title을 순회하며 확인
			if (arr[i].label != k) continue;	// Label이 Cluster와 일치하지 않으면 통과

			// 값 복사
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


		free(title_array);	// 동적할당 해제
	}


	return 0;
}