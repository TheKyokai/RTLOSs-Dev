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
    node->prev = NULL;
    if (list->head)
        list->head->prev = node;
    list->head = node;
    if (!list->tail)
        list->tail = node;
}

void List_Insert_Back(List* list, List_Node* node)
{
    node->prev = list->tail;
    node->next = NULL;
    if (list->tail)
        list->tail->next = node;
    list->tail = node;
    if (!list->head)
        list->head = node;
}

void List_Insert_Sorted(List* list, List_Node* node, List_Comparison_Function* comp)
{

}



void* List_Remove_Front(List* list)
{
    if (!list->head)
        return NULL;
    List_Node *node = list->head;
    list->head = list->head->next;
    node->prev = NULL;
    node->next = NULL;
    if (!list->head)
        list->tail = NULL;
    return node->data;
}

void* List_Remove_Back(List* list)
{
    if (!list->tail)
        return NULL;
    List_Node *node = list->tail;
    list->tail = list->tail->prev;
    node->prev = NULL;
    node->next = NULL;
    if (!list->tail)
        list->head = NULL;
    return node->data;
}
