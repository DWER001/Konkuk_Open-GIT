// DEFINITION
#include <string.h> // for memset()

#define ku_mmu_PAGE_SIZE 4
#define ku_mmu_PDE_SIZE 1
#define ku_mmu_PMDE_SIZE 1
#define ku_mmu_PTE_SIZE 1
#define ku_mmu_OFFSET_SIZE 1
#define ku_mmu_MEMORY_MAX_SIZE 256
#define ku_mmu_SWAP_MAX_SIZE 512

#define ku_mmu_ADDRESSING_BIT 8
#define ku_mmu_PDE_BIT 2
#define ku_mmu_PMDE_BIT 2
#define ku_mmu_PTE_BIT 2
#define ku_mmu_PAGE_BIT 2
#define ku_mmu_SWAP_BIT 7
#define ku_mmu_PDE_MASK 0xC0
#define ku_mmu_PMDE_MASK 0x30
#define ku_mmu_PTE_MASK 0xC
#define ku_mmu_PAGE_OFFSET_MASK 0x3
#define ku_mmu_PFN_MASK 0x1
#define ku_mmu_SWAP_MASK 0x1


struct ku_pte{ // Dependency for Assignment
        char pte;
};

struct PCB{
        char pid;
        char *PDBR;
};

struct node{ // for Free-List
        void *idx; // data
        struct node *next; // next ptr
};


// Global Variables
static char *ku_mmu_mem; // Physical Memory Address (using malloc)
static char *ku_mmu_swap; // Physical SWAP Address (using malloc)

static struct node *ku_mmu_mem_queue; // Memory Free-LIST (FIFO QUEUE)
static struct node *ku_mmu_swap_queue; // SWAP Free-LIST (FIFO QUEUE)

static struct node *ku_mmu_pcb_list; // PCB-LIST (LIFO STACK)
static struct node *ku_mmu_swapout_queue; // Swapout-LIST (FIFO QUEUE)


// Function Pre-Definition
void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size);
int ku_run_proc(char pid, struct ku_pte **ku_cr3);
int ku_page_fault(char pid, char va);

int SWAP_IN(char *PTE);
int SWAP_OUT();


//
// Assignment #1 Core Functions
//

// Init() <초기화>
// * PF 0번지는 사용 금지
//
// mem_size : Physical Memory Size (byte, Virtual)
// swap_size : Physical Disk Size (byte, Virtual)
// Return Value -> Physical Memory Address
void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size){
        struct node *tmp_node, *origin_mem, *origin_swap;

        // Wrong Parameter
        if(mem_size == 0){
                printf("Wrong Parameter (mem_size) : 0 is Impossible\n");
                return 0;
        }
        if(swap_size == 0){
                printf("Wrong Parameter (swap_size) : 0 is Impossible\n");
                return 0;
        }
        if(mem_size > ku_mmu_MEMORY_MAX_SIZE){
                printf("Warning: mem_size too big\n");
        }
        if(swap_size > ku_mmu_SWAP_MAX_SIZE){
                printf("Warning: swap_size too big\n");
        }
        if(mem_size % 4 != 0){
                printf("Warning: mem_size is NOT multiple of 4\n");
        }
        if(swap_size % 4 != 0){
                printf("Warning: swap_size is NOT multiple of 4\n");
        }


        // Allocate Physical Location
        ku_mmu_mem = (char *)malloc(mem_size);
        ku_mmu_swap = (char *)malloc(swap_size);
// For DEBUG
//printf("=================\nPhy.  Memory > %p\n", ku_mmu_mem);
//printf("Swap. Memory > %p\n=================\n", ku_mmu_swap);

        // mem/swap Free-LIST Setting
        ku_mmu_mem_queue = (struct node *)malloc(sizeof(struct node));
        ku_mmu_swap_queue = (struct node *)malloc(sizeof(struct node));

        ku_mmu_mem_queue->idx = (char *)ku_mmu_mem;
        ku_mmu_mem_queue->next = NULL;
        ku_mmu_swap_queue->idx = (char *)ku_mmu_swap;
        ku_mmu_swap_queue->next = NULL;

        memset(ku_mmu_mem_queue->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero
        memset(ku_mmu_swap_queue->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

        origin_mem = ku_mmu_mem_queue;
        origin_swap = ku_mmu_swap_queue;

        for(int i=1; i<mem_size/ku_mmu_PAGE_SIZE; ++i){
                tmp_node = (struct node *)malloc(sizeof(struct node));
                tmp_node->idx = (char *)(ku_mmu_mem + i * ku_mmu_PAGE_SIZE); // Physical Address Binding
                tmp_node->next = NULL;

                memset(tmp_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

                ku_mmu_mem_queue->next = tmp_node;
                ku_mmu_mem_queue = ku_mmu_mem_queue->next;
        }
        for(int i=1; i<swap_size/ku_mmu_PAGE_SIZE; ++i){
                tmp_node = (struct node *)malloc(sizeof(struct node));
                tmp_node->idx = (char *)(ku_mmu_swap + i * ku_mmu_PAGE_SIZE); // SWAP Address Binding
                tmp_node->next = NULL;

                memset(tmp_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

                ku_mmu_swap_queue->next = tmp_node;
                ku_mmu_swap_queue = ku_mmu_swap_queue->next;
        }
        ku_mmu_mem_queue->next = NULL;
        ku_mmu_swap_queue->next = NULL;

        ku_mmu_mem_queue = origin_mem;
        ku_mmu_swap_queue = origin_swap;

        // mem/swap Free-LIST 조정
        // PFN0과 SWAP0은 사용하지 않음(queue에서 제외)
        ku_mmu_mem_queue = ku_mmu_mem_queue->next;
        ku_mmu_swap_queue = ku_mmu_swap_queue->next;


        // PCB-LIST Initialize
        ku_mmu_pcb_list = (struct node *)malloc(sizeof(struct node));
        ku_mmu_pcb_list->idx = NULL;
        ku_mmu_pcb_list->next = NULL;


        // Swapout-LIST Initialize
        ku_mmu_swapout_queue = (struct node *)malloc(sizeof(struct node));
        ku_mmu_swapout_queue->idx = NULL;
        ku_mmu_swapout_queue->next = NULL;


        return ku_mmu_mem;
}



// RunProc() <Context Switch>
// * PCB 교체
// * 새 프로세스 등장시 PCB 할당 및 PDE 생성
//
// pid : CPU에 들어온 PID
// ku_cr3 : PDBR Physical Address
// Return Value -> Success 0 Fail -1
int ku_run_proc(char pid, struct ku_pte **ku_cr3){
        struct PCB *nPCB;
        struct node *tmp_node;

        // 새 PID인지 확인
        struct node *find_node;

        find_node = ku_mmu_pcb_list;
        while(find_node->next != NULL){
                // 기존에 존재하는 PID인 경우
                if( ((struct PCB *)find_node->idx)->pid == pid){
                        *ku_cr3 = (struct ku_pte*)((struct PCB*)find_node->idx)->PDBR;

                        return 0;
                }
                find_node = find_node->next;
        }


        // 새로운 PID인 경우
        // 새 PCB 생성
        nPCB = (struct PCB *)malloc(sizeof(struct PCB));
        nPCB->PDBR = NULL;
        nPCB->pid = pid;

        // PCB-List 관리
        tmp_node = (struct node *)malloc(sizeof(struct node));
        tmp_node->idx = (struct PCB *)nPCB;
        tmp_node->next = ku_mmu_pcb_list;

        ku_mmu_pcb_list = tmp_node;

        // PDBR 할당(Memory Free-LIST에서 한 Page 가져오기)
        if(ku_mmu_mem_queue == NULL){ // Allocate P.Memory
                if(SWAP_OUT() == 1){
                        printf("Swap-OUT Failed\n");
                        return -1;
                }
        }

        // DEQueue - Memory Free-LIST Queue
        tmp_node = ku_mmu_mem_queue;
        ku_mmu_mem_queue = ku_mmu_mem_queue->next;

        nPCB->PDBR = tmp_node->idx; // Physical Address
        memset(nPCB->PDBR, 0, ku_mmu_PAGE_SIZE); // Fill Zero

        free(tmp_node);


        *ku_cr3 = (struct ku_pte*)nPCB->PDBR;
        return 0;
}



// PageFault() <Page Fault Handler>
// * Demand Paging or Swapping
// * Page Replacement Policy : FIFO
// * PDE / PMDE / PTE 을 위한 Page는 Swap-out 금지
// * Swap Space 관리
//
// pid : Paging이 발생한 PID
// va : Virtual Address
// Return Value -> Success 0 Fail -1
int ku_page_fault(char pid, char va){
        struct node *tmp_node;
        char PFN;

        // PID 찾기
        struct node *find_node;
        struct PCB *pcb = NULL;

        find_node = ku_mmu_pcb_list;
        while(find_node->next != NULL){
                if(((struct PCB*)find_node->idx)->pid == pid){
                        pcb = find_node->idx;
                        break;
                }
                find_node = find_node->next;
        }

        if(pcb == NULL){
                printf("Wrong Parameter (pid)\n");
                return -1;
        }



        char *pa = (pcb->PDBR); // Physical Address
        char v_PDE = (va & ku_mmu_PDE_MASK) >> (ku_mmu_ADDRESSING_BIT - ku_mmu_PDE_BIT);
        char v_PMDE = (va & ku_mmu_PMDE_MASK) >> (ku_mmu_PTE_BIT + ku_mmu_PAGE_BIT);
        char v_PTE = (va & ku_mmu_PTE_MASK) >> (ku_mmu_PAGE_BIT);
        char v_offset = va & ku_mmu_PAGE_OFFSET_MASK;


// For DEBUG
//printf("\n[PAGE FAULT][PID %d] PDBR > %p\n", pid, pcb->PDBR);
//printf("SRC | Virtual %d (%d | %d)\n", va, v_PDE*16 + v_PMDE*4 + v_PTE, v_offset);
        // PDE -> PMDE 연결
        char *target = pa + v_PDE;
        if( ((*target) & ku_mmu_PFN_MASK) == 0 ){ // PMDE을 가르키고 있는 메모리 없음
                if(ku_mmu_mem_queue == NULL){ // Allocate P.Memory
                        if(SWAP_OUT() == 1){
                                printf("Swap-OUT Failed\n");
                                return -1;
                        }
                }

                // DEQueue - Memory Free-LIST Queue
                tmp_node = ku_mmu_mem_queue;
                ku_mmu_mem_queue = ku_mmu_mem_queue->next;

                memset(tmp_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

                PFN = ((char *)tmp_node->idx - ku_mmu_mem) / ku_mmu_PAGE_SIZE;
                *target = (PFN << ku_mmu_PAGE_BIT) | ku_mmu_PFN_MASK;

                free(tmp_node);
        }
        else{ // PMDE를 가리키는 메모리 있음
                PFN = (*target) >> ku_mmu_PAGE_BIT;
        }


        // PMDE -> PTE 연결
        target = (ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE) + v_PMDE;
        if( ((*target) & ku_mmu_PFN_MASK) == 0 ){ // PTE를 가르키고 있는 메모리 없음
                if(ku_mmu_mem_queue == NULL){ // Allocate P.Memory
                        if(SWAP_OUT() == 1){
                                printf("Swap-OUT Failed\n");
                                return -1;
                        }
                }

                // DEQueue - Memory Free-LIST Queue
                tmp_node = ku_mmu_mem_queue;
                ku_mmu_mem_queue = ku_mmu_mem_queue->next;

                memset(tmp_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

                PFN = ((char *)tmp_node->idx - ku_mmu_mem) / ku_mmu_PAGE_SIZE;
                *target = (PFN << ku_mmu_PAGE_BIT) | ku_mmu_PFN_MASK;

                free(tmp_node);
        }
        else{ // PTE를 가르키고 있는 메모리 있음
                PFN = (*target) >> ku_mmu_PAGE_BIT;
        }


        // PTE -> Physical Address 연결
        target = (ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE) + v_PTE;
        if( ((*target) & ku_mmu_PFN_MASK) == 0 ){ // Physical-PAGE를 가르키고 있는 메모리 없음

                // <PDE/PMDE/PTE 자체는 Swap-OUT을 하지 않기에 상단 PDE PMDE 파트에서 Swap-IN 구현 필요없음>
                if( ((*target) >> ku_mmu_SWAP_MASK) != 0 ){ // 현재 해당 데이터가 SWAP에 있는 경우 (Swap-In)
                        SWAP_IN(target);
                        PFN = (*target) >> ku_mmu_PAGE_BIT;
                }
                else{ // 현재 해당 데이터가 없는 경우 (새로 생성)
                        if(ku_mmu_mem_queue == NULL){ // Allocate P.Memory
                                if(SWAP_OUT() == 1){
                                        printf("Swap-OUT Failed\n");
                                        return -1;
                                }
                        }

                        // DEQueue - Memory Free-LIST Queue
                        tmp_node = ku_mmu_mem_queue;
                        ku_mmu_mem_queue = ku_mmu_mem_queue->next;

                        memset(tmp_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero

                        PFN = ((char *)tmp_node->idx - ku_mmu_mem) / ku_mmu_PAGE_SIZE;
                        *target = (PFN << ku_mmu_PAGE_BIT) | ku_mmu_PFN_MASK;

                        free(tmp_node);


                        // ENQueue - Swapout-LIST Queue <PDE/PMDE/PTE 자체는 제외>
                        struct node *ptr_target = ku_mmu_swapout_queue;
                        while(ptr_target->next != NULL){
                                ptr_target = ptr_target->next;
                        }

                        struct node *ptr_new = (struct node *)malloc(sizeof(struct node));
                        ptr_new->idx = NULL;
                        ptr_new->next = NULL;

                        ptr_target->idx = target;
                        ptr_target->next = ptr_new;
                }
        }
        else{ // Physical-PAGE를 가르키고 있는 메모리 있음
                PFN = (*target) >> ku_mmu_PAGE_BIT;
        }
// For DEBUG
//printf("DST | Physical %d (%d | %d) \n", (PFN<<2) + v_offset, PFN, v_offset);

        return 0;
}



//
// Assignment #1 Custom Functions
//

// Doing SWAP-OUT
// Success : 0 / Failed : 1
int SWAP_OUT(){
// For DEBUG
// printf("[SWAP-OUT] ");
        struct node *tmp_node, *find_node;

        // Swap-OUT 전 Swap영역 검사
        find_node = ku_mmu_swapout_queue;
        if(find_node->next == NULL){
                printf("No Memory to SWAP-out: ");
                return 1;
        }

        if(ku_mmu_swap_queue == NULL){
                printf("Not Enough SWAP Space: ");
                return 1;
        }


        // Swap-OUT
        struct node *mem_node = find_node;
        struct node *swap_node = ku_mmu_swap_queue;

        char *PTE = mem_node->idx;
        char PFN = (*PTE) >> ku_mmu_PAGE_BIT;
        char swapN = ((char *)swap_node->idx - ku_mmu_swap) / ku_mmu_PAGE_SIZE;

// For DEBUG
//printf("PFN %d -> SWAP %d\n", PFN, swapN);
        memcpy(ku_mmu_swap + swapN * ku_mmu_PAGE_SIZE, ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE, ku_mmu_PAGE_SIZE); // copy DATA (mem -> swap)
        memset(ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE, 0, ku_mmu_PAGE_SIZE); // clear mem DATA
        (*PTE) = swapN << 1; // move PTE pointer


        // ENQueue - Memory Free-LIST Queue
        tmp_node = (struct node *)malloc(sizeof(struct node));
        tmp_node->idx = NULL;
        tmp_node->next = NULL;

        find_node = ku_mmu_mem_queue;
        if(find_node == NULL){
                tmp_node->idx = (char *)(ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE); // Physical Address Binding
                ku_mmu_mem_queue = tmp_node;
        }
        else{
                while(find_node->next != NULL){
                        find_node = find_node->next;
                }

                tmp_node->idx = (char *)(ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE); // Physical Address Binding
                find_node->next = tmp_node;
        }

        // DEQueue - Swapout-LIST Queue 및 SWAP Free-LIST Queue
        tmp_node = ku_mmu_swapout_queue;
        ku_mmu_swapout_queue = ku_mmu_swapout_queue->next;
        free(tmp_node);

        tmp_node = ku_mmu_swap_queue;
        ku_mmu_swap_queue = ku_mmu_swap_queue->next;
        free(tmp_node);


        return 0;
}

// Doing SWAP-IN
// Success : 0 / Failed : 1
int SWAP_IN(char *PTE){
// For DEBUG
//printf("[SWAP-IN] ");
        struct node *tmp_node, *find_node;

        // Swap-IN 전 Memory 영역 검사
        if(ku_mmu_mem_queue == NULL){ // Allocate P.Memory
                if(SWAP_OUT() == 1){
                        printf("Swap-OUT Failed\n");
                        return 1;
                }
        }

        char swapN = (*PTE) >> ku_mmu_SWAP_MASK;
        char PFN;

        // DEQueue - Memory Free-LIST Queue
        struct node *mem_node = ku_mmu_mem_queue;
        ku_mmu_mem_queue = ku_mmu_mem_queue->next;
        memset(mem_node->idx, 0, ku_mmu_PAGE_SIZE); // Fill Zero
        PFN = ((char *)mem_node->idx - ku_mmu_mem) / ku_mmu_PAGE_SIZE;
        free(mem_node);

        // Swap-IN
// For DEBUG
// printf("SWAP %d -> PFN %d\n", swapN, PFN);
        memcpy(ku_mmu_mem + PFN * ku_mmu_PAGE_SIZE, ku_mmu_swap + swapN * ku_mmu_PAGE_SIZE, ku_mmu_PAGE_SIZE); // copy DATA (swap -> mem)
        memset(ku_mmu_swap + swapN * ku_mmu_PAGE_SIZE, 0, ku_mmu_PAGE_SIZE); // clear swap DATA
        (*PTE) = (PFN << ku_mmu_PAGE_BIT) | ku_mmu_PFN_MASK; // move PTE pointer


        // ENQueue - Swap Free-LIST Queue
        tmp_node = (struct node *)malloc(sizeof(struct node));
        tmp_node->idx = NULL;
        tmp_node->next = NULL;

        find_node = ku_mmu_swap_queue;
        if(find_node == NULL){
                tmp_node->idx = (char *)(ku_mmu_swap + swapN * ku_mmu_PAGE_SIZE); // SWAP Address Binding
                ku_mmu_swap_queue = tmp_node;
        }
        else{
                while(find_node->next != NULL){
                        find_node = find_node->next;
                }

                tmp_node->idx = (char *)(ku_mmu_swap + swapN * ku_mmu_PAGE_SIZE); // SWAP Address Binding
                find_node->next = tmp_node;
        }

        return 0;
}
