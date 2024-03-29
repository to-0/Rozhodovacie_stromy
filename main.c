#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
typedef struct node{
    char *value;
    struct node *parent; //rodic, potrebjem pri redukcii
    int variable; //podla ktorej premennej sa rozhodujem vo funkcii use (ked preskakujem urovne v redukoovanom diagrame), vlastne urcuje aj uroven v ktorej je
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

TABLE_EL **table;
int option;

void insert(NODE *current,int *nodes, int level){
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
    temp->variable = level+1;
    temp->parent = current;
    current->left = temp;
    (*nodes)++;
    insert(temp,nodes,level+1);
    //vlozim doprava
    NODE *temp2 = (NODE *)malloc(sizeof(NODE));
    temp2->value = (char *)malloc(k+1);
    strncpy(temp2->value,current->value+k,length/2);
    temp2->value[k] = '\0';
    temp2->left = NULL;
    temp2->right = NULL;
    temp2->variable = level+1;
    temp2->parent = current;
    current->right = temp2;
    (*nodes)++;
    insert(temp2,nodes,level+1);
}
BDD *BDD_create(char *bfunkcia){
    int n = strlen(bfunkcia);
    int nodes=1;
    NODE *root = (NODE*)malloc(sizeof(NODE));
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    root->variable = 0;
    //printf("%s\n",bfunkcia);
    int var_count = 0;
    int pom = n;
    while(pom>1){
        var_count++;
        pom/=2;
    }
    //printf("Count var %d\n",var_count);
    root->value  = (char *)malloc(n+1);
    strcpy(root->value,bfunkcia);
    root->value[n] = '\0';
    //printf("%s\n",root->value);
    insert(root,&nodes,0);
    BDD *pointer = (BDD *)malloc(sizeof(BDD));
    pointer->root = root;
    pointer->num_nodes = nodes;
    pointer->num_variables = var_count;
    //printf("NODES %d\n",nodes);
    return pointer;
}
char BDD_use(BDD *bdd, char *vstupy){
     int i = strlen(vstupy);
     int k=0;
     NODE *n = bdd->root;
     char c;
     for(k=0;k<i;k++){
          c=vstupy[n->variable];
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
void reduce_end_nodes_recursion(NODE *current, NODE **zero, NODE **one, int *count){ //chcem sa dostat na predposlednu uroven a vzdy poprejata vsetko aby boli iba 2 koncove uzly
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
        else if(*zero == NULL){ //ak zero je null skontrolujem ci jedno z childov nema hodnotu null, a potom este  ten druhy child musim skontrolat ci nema rovnaku hodnotu, aby som ho mohol zredukovat
            if(*current->left->value == '0'){
                *zero = current->left;
                if(*current->right->value == '0' && current->right != *zero){
                    free(current->right);
                    (*count)++;
                    current->right = *zero;
                }
            }
            else if (*current->right->value == '0'){
                *zero = current->right;
                if(*current->left->value == '0' && current->left != *zero){ //ese skontrolovat laveho childa ci sa nerovna 0 a uvolnit ked tak
                    free(current->left);
                    (*count)++;
                    current->left = *zero;
                }
            }

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
            if(*current->left->value == '1'){
                *one = current->left;
                if(*current->right->value == '1' && current->right != *one){
                    free(current->right);
                    (*count)++;
                    current->right = *one;
                }
            }
            else if (!strcmp(current->right->value,"1")){
                if(*current->left->value == '1' && current->left != *one) {
                    free(current->left);
                    (*count)++;
                    current->left = *one;
                }
                *one = current->right;
            }

        }
        return;
    }
    reduce_end_nodes_recursion(current->left, zero, one, count);
    reduce_end_nodes_recursion(current->right, zero, one, count);
}

void reduce_inner_nodes(TABLE_EL **table,int n,int *count){
    int i;
    for(i=n-2;i>0;i--){ //prechadzam od preposlednej urovne po root, idem od konca lebo inac ked mazem cely podstrom tak by mi to v tabulke ukazovalo zle hodnoty kedze uz mazem strom
        TABLE_EL *el1 = table[i];
        while(el1!=NULL){ //zoberiem si prvy prvok a budem ho porovnavat s otastatnymi na tej urovni a takto postupne porovm vsetky dvojice
            TABLE_EL *el2 = el1->next;
            TABLE_EL *prev = el1;
            if(!strcmp(el1->node->left->value,el1->node->right->value)){ //ak je ten node zbytocny (LAVY POTOMOK == PRAVY POTOMOK
                NODE *parent = el1->node->parent;
                if(parent->left == el1->node) //je to lavy child
                    parent->left = el1->node->left;
                else parent->right = el1->node->left;
                el1->node->left->parent = parent; //nastavim spravneho rodica
                free(el1->node); //a ten podstrom vymazem
                el1->node = NULL; //toto kvoli uvolnovaniu celeho diagramu potom, aby to nepadalo, nastavim na null ukazovatel na node
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
                    //free_subtree(el2->node,count);
                    free(el2->node);
                    free(el2);
                    el2 = prev->next;
                    continue;
                }
                prev = el2;
                if(el2!=NULL)
                 el2 = el2->next;
            }
            el1 = el1->next;
        }
    }
}

//test, toto by sa mozno dalo pouzit ze nemusim skakat rekurziou ale aj tak bude trbea updatnut tabulku
//kebyze to chcem zapracovat do toho musel by som si pocitat, ktory prvok chcem vymazat z tabulky a este sa k nemu cyklicky vzdy dostat, ked to necham iba na updatE_tabulka funkciu
//tak to vsetko spravi v 1 cykle
void free_element(TABLE_EL **element, TABLE_EL **prev){
    if(*prev==NULL){
        *prev = (*element)->next; //iba docasne
        free(*element); //element je prvy prvok v spajanom zozname tak ho vymazem
        *element = *prev;
        *prev = NULL;
    }
    else {
        (*prev)->next = (*element)->next;
        TABLE_EL  *temp = (*element)->next;
        free(*element);
        *element = temp;
    }
}
void connect_new_outer(NODE **parent,NODE *child, NODE *new_child){
    if((*parent)->left == child){ //idem to napojit z lava na novu nulu
        (*parent)->left = new_child;
    }
    else{
        (*parent)->right = new_child;
    }
}

void reduce_outer_ends_table(TABLE_EL **table, int n,NODE **zero, NODE **one, int *count){
    int k = n-1; //chcem ist na poslednu vrstvu
    TABLE_EL *element = table[k];
    TABLE_EL *prev = NULL;
    while(element!=NULL){ //idem prechadzat pposlednu vrstvu a mazat nadbytocne posledne elementy, potrebujem len 2 1/0
        NODE *current = element->node;
        if(*zero!=NULL){
            if(*current->value == '0' && current != *zero){
                connect_new_outer(&(current->parent),current,*zero);
                free(current);
               free_element(&element,&prev); //idem zmenit ten spajany zoznam
                (*count)++;
               continue;
            }
        }
        else {
            if(*current->value == '0')
                *zero = current;
        }
        if(*one!=NULL){
            if(*current->value == '1' && current != *one){
                connect_new_outer(&(current->parent),current,*one);
                free(current);
                free_element(&element,&prev); //idem zmenit ten spajany zoznam
                (*count)++;
                continue;
            }
        }
        else{
            if(*current->value == '1')
                *one = current;
        }
        prev = element;
        element = element->next;
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
void fill_table(TABLE_EL **table,NODE *current, int level) { //rekurzivne naplnim tabulku
    if (current->left == NULL || current->right == NULL) return;
    TABLE_EL *element = table[level];

    while (element != NULL) //iba sa nastavim na koniec spajaneho zoznamu
    {
        if(element->next == NULL)
            break;
        element = element->next;
    }

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
/**
 * Redukuje na poslednej urovni duplicitne ukazovatele na rovnake nodes, pretoze pred vytvorenim tabulky somrekurizvne volal reduce outer ends
 */
void reduce_table_after_recursion(TABLE_EL **table,int n){ //volam iba ked redukujem rekurzivne vonkajsie uzly, a vycistim poslednu uroven tabulky, aby tam viac-krat nebolo to iste... (tie iste nodes)
    int k = n-1;
    TABLE_EL  *e = table[k];
    TABLE_EL *e2 = NULL;
    while(e!=NULL){
        e2 = e->next;
        TABLE_EL *prev = e;
        while(e2!=NULL){
            if( e!= NULL && e2!=NULL && e2->node == e->node){
                prev->next = e2->next;
                free(e2);
                e2 = prev->next;
                continue;
            }
            prev = e2;
            e2 = e2->next;
        }
        e = e->next;
    }
}

int BDD_reduce(BDD *bdd){
    NODE *root = bdd->root;
    NODE *zero = NULL;
    NODE *one = NULL;
    //index tabulky je uroven v strome
    int i;
    int n = bdd->num_variables+1;
    for(i=0;i<n;i++) //vynulujem tabulku
        table[i]=NULL;
    table[0] = (TABLE_EL *)malloc(sizeof(TABLE_EL));
    table[0]->node = root; //na nultej urovni je iba koren
    table[0]->next = NULL; //nema ziadnych next
   int outer_reduced = 0;
    clock_t start,end;
    double time_taken;
   if(option == 1){
       start = clock();
       fill_table(table,root,1); //podla neho vyplnim dalej tabulku
       end = clock();
       time_taken = ((double)end-start)/CLOCKS_PER_SEC*1000;
       printf("Fill table %.2f\n",time_taken);
       start= clock();
       reduce_outer_ends_table(table,n,&zero,&one,&outer_reduced);
       end = clock();
       time_taken = ((double)end-start)/CLOCKS_PER_SEC*1000;
       printf("Reduce outer table %.2f\n",time_taken);
   }
   else{
       start = clock();
       reduce_end_nodes_recursion(root, &zero, &one, &outer_reduced);
       end = clock();
       time_taken = ((double)end-start)/CLOCKS_PER_SEC*1000;
       printf("Reduce outer recursion %.2f\n",time_taken);
       start = clock();
       fill_table(table,root,1);
       reduce_table_after_recursion(table,n);
       end = clock();
       time_taken = ((double)end-start)/CLOCKS_PER_SEC*1000;
       printf("Table fill and update %.2f\n",time_taken);
   }
    int inner_reduced = 0;
    reduce_inner_nodes(table,n,&inner_reduced);
    printf("Inner %d outer:%d  \n",inner_reduced,outer_reduced);
    return (outer_reduced+inner_reduced);
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
char *generate_input(int variables){
    int size=1;
    int i;
    size = (int)pow(2,variables);
    /*for(i=0;i<variables;i++)
        size*=2;*/
    char *str = (char *)malloc((size+1)*sizeof(char));
    for(i=0;i<size;i++){
        int r = rand()%2;
        if(r==1){
            str[i] = '1';
        }
        else str[i]='0';
    }
    str[size] = '\0';
    return str;
}
char **generate_table_use(int variables) {
    int i, j;
    int remain;
    int combinations = pow(2, variables);
    char **help_arr = (char **) malloc(combinations * (sizeof(char *)));
    for (i = 0; i < combinations; i++) {
        int num = i;
        help_arr[i] = (char *) malloc((variables + 1) * sizeof(int));
        for (j = variables - 1; j >= 0; j--) {
            remain = num % 2;
            if (remain == 0) {
                help_arr[i][j] = '0';
            } else help_arr[i][j] = '1';
            num = num / 2;
        }
        help_arr[i][variables] = '\0';
    }
    return help_arr;
}
void test_use(BDD *bdd,int variables,char **help_table){
    int i;
    int combinations = pow(2,variables);
    for(i= 0; i<combinations;i++){
        BDD_use(bdd,help_table[i]);
        //printf("%s\n",help_table[i]);
        }
}
void free_bdd_and_table(TABLE_EL *table[],int n){
    int i;
    for(i=0;i<n;i++){
        TABLE_EL *e1 = table[i];
        TABLE_EL *prev = NULL;
        while(e1!=NULL){
            prev = e1;
            //printf("%p\n",e1);
            e1 = e1->next;
            //ked vonkajsie uzly, redukujem cez rekurziu, v tabulke mam  poslednu vrstvu 2^n elementov ale tie nodes su duplicitne
            if(i==n-1)
            if(prev->node!=NULL){
                //printf("prev->node %p\n",prev->node);
                free(prev->node);
            }
            free(prev);
        }
    }

}
int main() {
    int i;
    clock_t start,end;
    int vars;
    printf("Pocet premennych:\n");
    scanf("%d",&vars);
    printf("Sposob redukcie koncovych uzlov cez tabulku (1) alebo rekurzivne (2):\n");
    scanf("%d",&option);
    printf("Pocet bdd na testovanie: \n");
    int num;

    scanf("%d",&num);
    start = clock();
    int all_nodes=0;
    int reduced=0;
    char **arr_use = generate_table_use(vars);
    for(i = 0;i<num;i++){
        char *input = generate_input(vars);
        BDD *p = BDD_create(input);
        int before_reduce = p->num_nodes;
        all_nodes += before_reduce;
        int n = p->num_variables+1; //+1 lebo vratane root urovne
        table =  (TABLE_EL **)malloc((p->num_variables+1)*sizeof(TABLE_EL *));
        int nodes_reduced = BDD_reduce(p);
        reduced+=nodes_reduced;
        printf("nodes reduced: %d\n",nodes_reduced);
        int after_reduce = before_reduce - nodes_reduced;
        test_use(p,p->num_variables,arr_use);
        printf("Before reduce: %d nodes, after reduce %d nodes\n",before_reduce,after_reduce);
        double percent = (double)((double) after_reduce/before_reduce)*100;
        printf("Reduced diagram has %.2f percent of nodes of original diagram.\n",percent);
        free_bdd_and_table(table,n);
    }
    double percent = (double)((double) (all_nodes-reduced)/all_nodes)*100;
    printf("Reduced diagrams had %.2f percent of nodes of original diagrams nodes.\n",percent);
    end = clock();
    double time_taken = ((double)end-start)/CLOCKS_PER_SEC; // sekundy
    time_taken /= 60; //minuty
    printf("Celkovy cas behu algoritmu 2000x vytvorenie bdd stromu, redukcia a nasledne testovanie vsetkych moznosti %.2f minut\n",time_taken);
    return 0;
}
