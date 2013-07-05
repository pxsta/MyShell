#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"list.h"

list_node *create_list(int object_size)
{
    list_node *header = (list_node *)calloc(1, sizeof(list_node));
    header->object_size = object_size;
    header->prev = NULL;
    header->next = NULL;
    header->data = NULL;
    return header;
}


//末尾に要素を加える
void add_list(list_node *head, void *data)
{
    list_node *newNode;
    list_node *currentNode = head;


    newNode = (list_node *)calloc(1, sizeof(list_node));
    newNode->data = malloc(head->object_size);
    memcpy(newNode->data, data, head->object_size);
    while (currentNode && currentNode->next)
    {
        currentNode = currentNode->next;
    }
    currentNode->next = newNode;
    newNode->prev = currentNode;

}

//先頭に要素を加える
void unshift(list_node *head, void *data)
{
    list_node *newNode;

    if (data == NULL)
    {
        fprintf(stderr, "NULLをリストに追加しようとした\n");
        return;
    }

    newNode = (list_node *)calloc(1, sizeof(list_node));
    newNode->data = malloc(head->object_size);
    memcpy(newNode->data, data, head->object_size);

    newNode->next = head->next;
    newNode->prev = head;
    head->next = newNode;
}

//先頭から要素を1つ取り出す
void *shift(list_node *head)
{
    void *first_node_data;
    list_node *next_first_node;
    if (head->next == NULL || head->next->data == NULL)
    {
        //fprintf(stderr,"リストに要素がない\n");
        return NULL;
    }

    first_node_data = malloc(head->object_size);
    memcpy(first_node_data, head->next->data, head->object_size);

    next_first_node = head->next->next;
    free(head->next->data);
    free(head->next);

    head->next = next_first_node;
    next_first_node->prev = head;
    return first_node_data;
}

//目的の要素のデータへの参照を取得する
void *elementAt(list_node *head, int index)
{
    int i = 0;
    list_node *currentNode = head->next;

    if (head == NULL || head->next == NULL || head->next->data == NULL)
    {
        //fprintf(stderr,"リストに要素がない\n");
        return NULL;
    }

    while (i < index)
    {
        if (currentNode == NULL || currentNode->next == NULL)
        {
            //fprintf(stderr,"範囲外アクセス\n");
            return NULL;
        }

        currentNode = currentNode->next;
        i++;
    }
    return currentNode->data;
}

list_node *get_next(list_node *head)
{
    return head->next;
}

list_node *get_prev(list_node *head)
{
    return head->prev;
}

void free_list(list_node *head)
{
    list_node *currentNode, *nextNode;

    currentNode = head->next;
    while (currentNode && currentNode->next)
    {
        nextNode = currentNode->next;
        free(currentNode);
        currentNode = nextNode;
    }
    free(head);
}
