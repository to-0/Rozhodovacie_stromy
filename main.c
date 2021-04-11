#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct bf{
    char *val;
    int length;
}BF;
typedef struct node{
    char *value; //neviem ci nemam ako value brat ze BF??
    struct node *parent; //??
    struct node *left;
    struct node *right;

}NODE;

typedef struct bdd {
    int num_variables;
    int num_nodes;
    NODE *root;
}BDD;
typedef struct table_el{
    NODE *node;
    struct table_el *next;
}TABLE_EL;

void insert(NODE *current,int *nodes){
    int length = strlen(current->value);
    if(length ==1) return; //uz to neviem viacej rozdelit
    //vlozim dolava
    NODE *temp = (NODE *)malloc(sizeof(NODE));
    int k = length/2;
    temp->value = (char *)malloc(k+1);
    strncpy(temp->value,current->value,k);
    temp->value[k] = '\0';
    temp->left = NULL;
    temp->right = NULL;
    temp->parent = current;
    current->left = temp;
    (*nodes)++;
    insert(temp,nodes);
    //vlozim doprava
    NODE *temp2 = (NODE *)malloc(sizeof(NODE));
    temp2->value = (char *)malloc(k+1);
    strncpy(temp2->value,current->value+k,length/2);
    temp2->value[k] = '\0';
    temp2->left = NULL;
    temp2->right = NULL;
    temp2->parent = current;
    current->right = temp2;
    (*nodes)++;
    insert(temp2,nodes);
}
BDD *BDD_create(BF *bfunkcia){
    int n = bfunkcia->length;
    int nodes=1;
    NODE *root = (NODE*)malloc(sizeof(NODE));
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    printf("%s\n",bfunkcia->val);
    int var_count = 0;
    int pom = n;
    while(pom>=1){
        var_count++;
        pom/=2;
    }
    printf("Count var %d\n",var_count);
    root->value  = (char *)malloc(n+1);
    strcpy(root->value,bfunkcia->val);
    root->value[n] = '\0';
    printf("%s\n",root->value);
    insert(root,&nodes);
    BDD *pointer = (BDD *)malloc(sizeof(BDD));
    pointer->root = root;
    pointer->num_nodes = nodes;
    pointer->num_variables = var_count;
    printf("NODES %d\n",nodes);
    return pointer;
}
char BDD_use(BDD *bdd, char *vstupy){
     int i = strlen(vstupy);
     int k=0;
     NODE *n = bdd->root;
     char c;
     for(k=0;k<i;k++){
          c=vstupy[k];
         if(c=='0'){
             if(n->left == NULL) break;
             n = n->left;
         }
         else {
             if(n->right ==NULL) break;
             n = n->right;
         }
     }
     c = n->value[0];
     return c;
}
void join_end_nodes(NODE *current,NODE **zero, NODE **one,int *count){ //chcem sa dostat na predposlednu uroven a vzdy poprejata vsetko aby boli iba 2 koncove uzly
    if(strlen(current->value)==2){ //ked uz som na predposlednej urovni tak chcem poprepajat tie koncove nodes aby existovali iba 2
        if(*zero!=NULL){
            if(*current->left->value == '0' && current->left != *zero){
                free(current->left);
                (*count)++;
                current->left =  *zero;
            }
            if(*current->right->value == '0' && current->right != *zero){
                free(current->right);
                (*count)++;
                current->right = *zero;
            }
        }
        else if(*zero == NULL){
            if(*current->left->value == '0')
                *zero = current->left;
            else if (*current->right->value == '0')
                *zero = current->right;
        }
        if(*one!=NULL){
            if(*current->left->value == '1' && current->left != *one){
                free(current->left);
                (*count)++;
                current->left =  *one;
            }
            if(*current->right->value == '1' && current->right != *one){
                free(current->right);
                (*count)++;
                current->right = *one;
            }
        }
        else{
            if(*current->left->value == '1')
                *one = current->left;
            else if (*current->right->value == '1')
                *one = current->right;
        }
        return;
    }
    join_end_nodes(current->left,zero,one,count);
    join_end_nodes(current->right,zero,one,count);
}
void free_subtree(NODE *e,int *count){
    int n =  strlen(e->value);
    if(n==1){
        return;
    }
    free_subtree(e->left,count);
    free_subtree(e->right,count);
    free(e);
    (*count)+=2;
}
void reduce_inner_nodes(TABLE_EL **table,int n,int *count){
    int i;
    for(i=n-2;i>0;i--){ //prechadzam od preposlednej urovne po root, idem od konca lebo inac ked mazem cely podstrom tak by mi to v tabulke ukazovalo zle hodnoty kedze uz mazem strom
        TABLE_EL *el1 = table[i];
        while(el1!=NULL){ //zoberiem si prvy prvok a budem ho porovnavat s otastatnymi na tej urovni a takto postupne porovm vsetky dvojice
            TABLE_EL *el2 = el1->next;
            TABLE_EL *prev = el1;
            if(!strcmp(el1->node->left->value,el1->node->right->value)){ //ak je ten node zbytocny (LAVY POTOMOK == PRAVY POTOMOK
                free_subtree(el1->node->right,count); //cely pravy podstrom vymazem
                NODE *parent = el1->node->parent;
                if(parent->left == el1->node) //je to lavy child
                    parent->left = el1->node->left;
                else parent->right = el1->node->left;
                el1->node->left->parent = parent; //nastavim spravneho rodica
                free(el1->node); //a ten podstrom vymazem
                (*count)++;
                el1 = el1->next;
                continue; //pokracujem dalej v cykle nepotrebujem ist aj do vnutorneho el2
            }
            while(el2!=NULL){ //tu riesim su 2 uzly na rovnakej urovni rovnake, jeden vymazem a zbytok prepojim
                if(!strcmp(el1->node->value,el2->node->value)){ //ak maju rovnaku hodnotu
                    NODE *parent = el2->node->parent;
                    if(parent->left == el2->node){ //ak je to lavy potoomok
                        parent->left = el1->node; //odpojim ho a prepojim s el1
                    }
                    else parent->right = el1->node; //ak je pravy potomok
                    prev->next = el2->next; //odpojim el2 zo spajaneho zoznamu
                    free_subtree(el2->node,count);
                    free(el2);
                    el2 = prev->next;
                }
                prev = el2;
                el2 = el2->next;
            }
            el1 = el1->next;
        }
    }
}
void update_table(TABLE_EL **table, NODE *zero, NODE *one, int n){
    int p0=0;
    int p1=0;
    //iba poslednu vrstvu idem upravit
        TABLE_EL *e = table[n-1];
        TABLE_EL *prev = e;
        while(e!=NULL){
            if(p0!=1 && e->node == zero){ //ked ten node nie je ani jedna z mojich 2 ukazovatelov, je tam navyse, treba ho vymazat
             prev->next = e->next;
             p0=1;
             free(e);
             e = prev->next;
             continue;
            }
            if(p1!=1 && e->node == one){
                prev->next = e->next;
                p1=1;
                free(e);
                e = prev->next;
                continue;
            }
            prev = e;
            e = e->next;
        }

}
//test, toto by sa mozno dalo pouzit ze nemusim skakat rekurziou ale aj tak bude trbea updatnut tabulku
//kebyze to chcem zapracovat do toho musel by som si pocitat, ktory prvok chcem vymazat z tabulky a este sa k nemu cyklicky vzdy dostat, ked to necham iba na updatE_tabulka funkciu
//tak to vsetko spravi v 1 cykle
void reduce_outer_ends(TABLE_EL **table, int n,NODE **zero, NODE **one, int *count){
    int k = n-2; //chcem ist na predposlednu vrstvu/level
    TABLE_EL *element = table[k];
    int i=0; //bude sluzit na dpocitanie kolkaty prvok v pomocnej tabulke v poslednej vrstve treba vymazat
    while(element!=NULL){ //idem prechadzat predposlednu vrstvu a mazat nadbytocne posledne elementy, potrebujem len 2 1/0
        NODE *current = element->node;
        if(*zero!=NULL){
            if(*current->left->value == '0' && current->left != *zero){
                free(current->left);
                (*count)++;
                current->left =  *zero;

            }
            else if(*current->right->value == '0' && current->right != *zero){
                free(current->right);
                (*count)++;
                current->right = *zero;
            }
        }
        else if(*zero == NULL){
            if(*current->left->value == '0')
                *zero = current->left;
            else if (*current->right->value == '0')
                *zero = current->right;
        }
        if(*one!=NULL){
            if(*current->left->value == '1' && current->left != *one){
                free(current->left);
                (*count)++;
                current->left =  *one;
            }
            else if(*current->right->value == '1' && current->right != *one){
                free(current->right);
                (*count)++;
                current->right = *one;
            }
        }
        else{
            if(*current->left->value == '1')
                *one = current->left;
            else if (*current->right->value == '1')
                *one = current->right;
        }
        i+=2; //tu si pocitam
    }
}
void print_table(TABLE_EL **table, int n){
    int i;
   for(i=0;i<n;i++){
       TABLE_EL *e = table[i];
       printf("Uroven %d:",i);
       while(e!=NULL){
           printf("%p %s, ",e->node,e->node->value);
           if(e->node->value==NULL)
               printf("Rovna sa 0");
           e = e->next;
       }
       printf("\n");
   }
}
void fill_table(TABLE_EL **table,NODE *current, int level){ //rekurzivne naplnim tabulku
    if(current->left == NULL || current->right == NULL) return;
    TABLE_EL *element = table[level];

    while(element!=NULL && element->next!=NULL) //iba sa nastavim na koniec spajaneho zoznamu
        element = element->next;
    TABLE_EL  *new_element= (TABLE_EL*)malloc(sizeof(TABLE_EL));
    new_element->node = current->left;
    new_element->next = (TABLE_EL*)malloc(sizeof(TABLE_EL));
    new_element->next->node = current->right;
    new_element->next->next = NULL;
    if(element==NULL){
        table[level] = new_element;
    }
    else element->next = new_element;

    fill_table(table,current->left,level+1);
    fill_table(table,current->right,level+1);
}
int BDD_reduce(BDD *bdd){
    NODE *root = bdd->root;
    NODE *zero = NULL;
    NODE *one = NULL;
    TABLE_EL **table =  (TABLE_EL **)malloc((bdd->num_variables)*sizeof(TABLE_EL *)); //numvariables+1 je vlastne pocet urovni vratane root
    //index tabulky je uroven v strome
    int i;
    int n = bdd->num_variables;
    for(i=0;i<(bdd->num_variables);i++) //vynulujem tabulku
        table[i]=NULL;
    table[0] = (TABLE_EL *)malloc(sizeof(TABLE_EL));
    table[0]->node = root; //na nultej urovni je iba koren
    table[0]->next = NULL; //nema ziadnych next
    int nodes_reduced = 0;
    join_end_nodes(root,&zero,&one,&nodes_reduced);
    fill_table(table,root,1); //podla neho vyplnim dalej tabulku
    printf("TU");
    //update_table(table,zero,one,n);
    printf("nie");
    print_table(table,bdd->num_variables);
    reduce_inner_nodes(table,n,&nodes_reduced);
    //update_table(table,zero,one,n);
    print_table(table,bdd->num_variables);
    return nodes_reduced;
}
void preorder(NODE *n){
    if(n!=NULL){
        putchar('(');
    }
    if(n==NULL){
        printf("()");
        return;
    }
    printf("%s",n->value);
    putchar(' ');
    preorder(n->left);
    putchar(',');
    preorder(n->right);
    putchar(')');
}
int main() {
    BF *input = (BF*)malloc(sizeof(BF));
    input->val = "10010011";
    input->length = strlen(input->val);
    BDD *p = BDD_create(input);
    printf("%c\n",BDD_use(p,"001"));
    printf("nodes reduced: %d\n",BDD_reduce(p));
    printf("%c\n",BDD_use(p,"001"));
    printf("%c\n",BDD_use(p,"001"));
    preorder(p->root);
    printf("\n%c\n",BDD_use(p,"100"));
    return 0;
}
