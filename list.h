#if !defined _LIST_H
#define _LIST_H

typedef struct list_node {
	int object_size;
	void* data;	
	struct list_node *next;
	struct list_node *prev;
} list_node;



list_node *create_list(int object_size);
void add_list(list_node *head,void* data);
void unshift(list_node *head,void* data);
void* shift(list_node *head);
void* elementAt(list_node *head,int index);
list_node* get_next(list_node *head);
list_node* get_prev(list_node *head);
void free_list(list_node *head);
#endif
