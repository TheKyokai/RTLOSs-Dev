#ifndef LIST_H
#define LIST_H

typedef struct List_Node List_Node;
typedef struct List List;

struct List_Node
{
    void* data;
    List_Node *prev, *next;
};

struct List
{
    List_Node *head, *tail;
};

typedef int List_Comparison_Function(void* first, List_Node* node);


void List_Init(List* list);
int List_Empty(List* list);

void List_Insert_Front(List* list, List_Node* node);
void List_Insert_Back(List* list, List_Node* node);

void* List_Remove_Front(List* list);
void* List_Remove_Back(List* list);

void* List_Remove(List* list, List_Node* node);

void* List_Peek_Front(List* list);
void* List_Peek_Back(List* list);

void List_Insert_Before(List* list, List_Node* node, List_Node* target_node);
void List_Insert_After(List* list, List_Node* node, List_Node* target_node);

void List_Insert_Sorted(List* list, List_Node* node, List_Comparison_Function* comp);

#endif