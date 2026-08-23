#ifndef LIST_H
#define LIST_H

typedef struct List_Node
{
    void* data;
    List_Node *prev, *next;
} List_Node;

typedef struct List
{
    List_Node *head, *tail;
} List;

typedef int List_Comparison_Function(void* first, List_Node* node);


void List_Init(List* list);
void List_Insert_Front(List* list, List_Node* node);
void List_Insert_Back(List* list, List_Node* node);

void* List_Remove_Front(List* list);
void* List_Remove_Back(List* list);

void List_Insert_Sorted(List* list, List_Node* node, List_Comparison_Function* comp);

#endif