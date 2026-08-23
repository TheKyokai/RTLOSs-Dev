#include "../inc/list.h"

#include "stddef.h"

void List_Init(List* list)
{
    list->head = NULL;
    list->tail = NULL;
}

void List_Insert_Front(List* list, List_Node* node)
{
    node->next = list->head;
    if (list->head)
        list->head->prev = node;
    node->prev = NULL;
}

void List_Insert_Back(List* list, List_Node* node)
{
    node->prev = list->tail;
    if (list->tail)
        list->tail->next = node;
    list->tail = node;
    node->next = NULL;
}

void List_Insert_Sorted(List* list, List_Node* node, List_Comparison_Function* comp)
{

}



void* List_Remove_Front(List* list)
{
    if (!list->head)
        return NULL;
    List_Node *node = list->head;
    node->next = NULL;
    list->head = list->head->next;
    if (!list->head)
        list->tail = NULL;
    return node->data;
}

void* List_Remove_Back(List* list)
{
    if (!list->tail)
        return NULL;
    List_Node *node = list->tail;
    node->prev = NULL;
    list->tail = list->tail->prev;
    if (!list->tail)
        list->head = NULL;
    return node->data;
}
