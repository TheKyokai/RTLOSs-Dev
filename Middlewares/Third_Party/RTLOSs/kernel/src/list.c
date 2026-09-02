#include "../inc/list.h"

#include "stddef.h"

void List_Init(List* list)
{
    list->head = NULL;
    list->tail = NULL;
}

inline int List_Empty(List* list)
{
    return list->head == NULL;
}

void List_Insert_Front(List* list, List_Node* node)
{
    if (!list) return;
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
    if (!list) return;
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
    if (!list || !list->head)
        return NULL;
    List_Node *node = list->head;
    list->head = list->head->next;
    if (list->head)
        list->head->prev = NULL;
    node->prev = NULL;
    node->next = NULL;
    if (!list->head)
        list->tail = NULL;
    return node->data;
}

void* List_Remove_Back(List* list)
{
    if (!list || !list->tail)
        return NULL;
    List_Node *node = list->tail;
    list->tail = list->tail->prev;
    if (list->tail)
        list->tail->next = NULL;
    node->prev = NULL;
    node->next = NULL;
    if (!list->tail)
        list->head = NULL;
    return node->data;
}



void* List_Peek_Front(List* list)
{
    if (!list || !list->head)
        return NULL;
    return list->head->data;
}

void* List_Peek_Back(List* list)
{
    if (!list || !list->tail)
        return NULL;
    return list->tail->data;
}

void List_Insert_Before(List* list, List_Node* node, List_Node* target_node)
{
    if (!list || !node || !target_node)
        return;
    
    List_Node *current_node = list->head;
    while (current_node)
    {
        if (current_node == target_node)
            break;
        current_node = current_node->next;
    }

    // Target node not a part of the list
    if (!current_node)
        return;

    node->prev = target_node->prev;
    node->next = target_node;
    if (target_node->prev)
        target_node->prev->next = node;
    target_node->prev = node;
    if (target_node == list->head)
        list->head = node;    
}

void List_Insert_After(List* list, List_Node* node, List_Node* target_node)
{
    if (!list || !node || !target_node)
        return;
    
    List_Node *current_node = list->head;
    while (current_node)
    {
        if (current_node == target_node)
            break;
        current_node = current_node->next;
    }

    // Target node not a part of the list
    if (!current_node)
        return;

    node->prev = target_node;
    node->next = target_node->next;
    if (target_node->next)
        target_node->next->prev = node;
    target_node->next = node;
    if (target_node == list->tail)
        list->tail = node;
}