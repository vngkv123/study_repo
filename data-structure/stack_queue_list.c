#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct _node{
	int value;
	struct _node *next;
}node;

typedef node *node_ptr;

typedef struct Queue{
	int q_count;
	node_ptr front;
	node_ptr rear;
}queue;

typedef struct Stack{
	node_ptr top;
}stack;

void *init_stack(stack *_stack);
void *insert_stack(stack *_stack);			/* push */
void *delete_stack(stack *_stack);			/* pop */
void *show_stack(stack *_stack);

void *init_queue(queue *_queue);
void *insert_queue(queue *_queue);
void *delete_queue(queue *_queue);
void *show_queue(queue *_queue);

void menu(){
	printf("-----------Menu------------\n");
	puts("1. insert stack");
	puts("2. insert queue");
	puts("3. delete stack");
	puts("4. delete queue");
	puts("5. show stack");
	puts("6. show queue");
	puts("7. exit");
}

int main(int argc, char *argv[]){
	stack user_stack;
	queue user_queue;
	int user_choice;

	init_stack(&user_stack);
	init_queue(&user_queue);

	while(1){
		menu();
		printf("Your choice : ");
		scanf("%d", &user_choice);
		switch(user_choice){
			case 1:
				insert_stack(&user_stack);
				break;
			case 2:
				insert_queue(&user_queue);
				break;
			case 3:
				delete_stack(&user_stack);
				break;
			case 4:
				delete_queue(&user_queue);
				break;
			case 5:
				show_stack(&user_stack);
				break;
			case 6:
				show_queue(&user_queue);
				break;
			case 7:
				exit(0);
				break;
			default:
				puts("Invalid choice");
				break;
		}
	}
}

void *init_stack(stack *_stack){
	_stack->top = NULL;
}

void *init_queue(queue *_queue){
	_queue->q_count = 0;
	_queue->front = NULL;
}

void *insert_stack(stack *_stack){
	node_ptr now = malloc(sizeof(node));
	printf("data : ");
	scanf("%d", &now->value);
	now->next = _stack->top;			/* actually prev */
	_stack->top = now;
}

void *delete_stack(stack *_stack){
	node_ptr prev;
	prev = _stack->top->next;
	memset(_stack->top, 0, sizeof(node));
	free(_stack->top);
	_stack->top = prev;
}

void *show_stack(stack *_stack){
	if( _stack->top == NULL ){
		puts("Insert data first !!");
		return 0;
	}
	node_ptr now;
	now = _stack->top;
	while( now != NULL ){
		printf("data : %d\n", now->value);
		now = now->next;
	}
}

void *insert_queue(queue *_queue){
	node_ptr now;
	now = malloc(sizeof(node));
	printf("data : ");
	scanf("%d", &now->value);
	if( _queue->q_count++ == 0 ){
		_queue->front = now;
		_queue->rear = now;
	}
	else{
		_queue->rear->next = now;
		_queue->rear = now;
	}
}


void *delete_queue(queue *_queue){
	node_ptr save;
	save = _queue->front->next;
	free(_queue->front);
	_queue->front = save;
	_queue->q_count—;
}
void *show_queue(queue *_queue){
	node_ptr now;
	now = _queue->front;
	if( now == NULL ){
		printf("Insert data first !!\n");
		return 0;
	}
	while( now != NULL ){
		printf("data : %d\n", now->value);
		now = now->next;
	}
}
