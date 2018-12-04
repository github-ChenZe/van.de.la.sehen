#include "debugger.h"

typedef struct linked_list_node
{
	int value;
	struct linked_list_node* next;
	char name[12];
} linked_list_node;


__attribute__((tracker))
void mystruct_tracker(linked_list_node node)
{

	printf("<tracker1/>\n");
}

int test()
{
	track_var linked_list_node node;
	node.value = 4;
}

int main()
{
	track_var linked_list_node node;
	scanf("%d %p %s", &(node.value), &(node.next), node.name);
	scanf("%d %p %s", &(node.value), &(node.next), node.name);
	node.next = &node;
	//node.value = 1;
}


