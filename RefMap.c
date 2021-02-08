// Universidad de Carabobo. FaCYT
// Sistemas Operativos
// Gabriel Peraza. CI: 26.929.687

// Liberías de C y POSIX/Unix:
#include "RefMap.h"
// Liberías de C y POSIX/Unix:
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>  // errno y perror()
#include <math.h>   // REQUIRES -lm

#define LT -1
#define EQ 0
#define GT 1

// Inline dynamic memory management:
#define STOP_IF_UNSAFE( ptr , err_msg )             \
    do { if( ptr == NULL ){                         \
            fprintf( stderr , "%s\n\n" , err_msg ); \
            exit( 1 ); }                            \
    } while(0);

//Inline stack operations:
#define PUSH(stack,counter,element) \
        stack[counter]   = element; \
        counter         += 1;       \
        
#define POP(stack,counter,element)  \
    counter -= 1;                   \
    element  = stack[counter];      \

#define EMPTY (used == 0)


//      ---------------------------------------
//      [*] Declaración de funciones estáticas: (visibles sólo dentro de este archivo)
//      ---------------------------------------

static void    init         ( NodeRB* node , void* key , void* value , COLOR color , int N );
static NodeRB* rotateLeft   ( NodeRB* h );
static NodeRB* rotateRight  ( NodeRB* h );
static void    flipColors   ( NodeRB* h );
static NodeRB* balance      ( NodeRB* h );
static NodeRB* moveRedRight ( NodeRB* h );
static NodeRB* moveRedLeft  ( NodeRB* h );
static void*   deleteMin    ( NodeRB** h ); // [$] Mutable traversal. Returns the retrieved key.
static void*   delete       ( NodeRB** root , void* key , int (*compare)(void*,void*) );    // [$] Mutable traversal. Returns the extracted key.
static NodeRB* minimum      ( NodeRB* h );
static int     contains     ( NodeRB* node , void* key , int (*cmp)(void*,void*) );
static NodeRB* get          ( NodeRB* node , void* key , int (*cmp)(void*,void*) );
static int     isRed        ( NodeRB* node );
static COLOR _flipcolor ( COLOR c );

// Refmap private functions:
static void refmap_unsafe_delete   ( RefMap* t , void* key );
static void refmap_unsafe_deleteMin( RefMap* t );

static void print_opaque( void *unknow );
static void debug( NodeRB* x , int indent , int incr ,
                   char black[] , char red[] , char normal[] ,
                   void (*key)(void*) , void (*val)(void*) );

//      ------------------------------
//      [_] Private functions (NodeRB)
//      ------------------------------

static void init( NodeRB* node , void* key , void* value , COLOR color , int N ){
    // Llave y contenido:
    node->key   = key;
    node->value = value;

    // Datos del árbol
    node->color = color;
    node->N     = N;

    // Sin hijos:
    node->left  = NULL;
    node->right = NULL;
}

static int isRed( NodeRB* node ){
    if( node == NULL ) return 0;
    else               return node->color == RED;
}

static int nodeSize( NodeRB* node ){
    if( node == NULL ) return 0;
    else               return node->N;
}

// Retorna la nueva raíz del sub-árbol h luego de aplicar la rotación.
static NodeRB* rotateLeft( NodeRB* h ){
    // Rotate left (Robert Sedgewick, Kevin Wayne. Algorithms 4th ed.) 
    /*  ANTES:
                E   <-- h
              /   \
            <%     S  <-- x
                 /  \
                <@  #>
      
        DESPUÉS:
                   S  <-- x
                 /  \
          h --> E   #>
              /  \
            <%   <@
    */
    
    NodeRB* x = h->right;
    h->right  = x->left;
    x->left   = h;

    x->color  = h->color;
    h->color  = RED;

    x->N      = h->N;
    h->N      = 1 + nodeSize(h->left) + nodeSize(h->right);
    return x;
}

// Retorna la nueva raíz del sub-árbol h luego de aplicar la rotación.
static NodeRB* rotateRight( NodeRB* h ){
    // Rotate Right:
    NodeRB* x = h->left;
    h->left   = x->right;
    x->right  = h;

    x->color  = h->color;
    h->color  = RED;

    x->N      = h->N;
    h->N      = 1 + nodeSize(h->left) + nodeSize(h->right);
    return x;
}

// Retorna el color opuesto.
static COLOR _flipcolor( COLOR c ){
    return (COLOR) !( (int) c );
}

// Pre{ node != NULL }
static void flipColors( NodeRB* h ){
    h->color        = _flipcolor(h->color);
    h->left->color  = _flipcolor(h->left->color);
    h->right->color = _flipcolor(h->right->color);
}

static NodeRB* moveRedLeft( NodeRB* h ){
    flipColors(h);
    if( isRed(h->right->left) ){
        h->right = rotateRight(h->right);
        h        = rotateLeft(h);
        flipColors(h);
    }
    return h;
}

// Invierte los colores y rota cuando el nieto no es rojo.
static NodeRB* moveRedRight( NodeRB* h ){
    flipColors(h);
    if( isRed(h->left->left) ){
        h = rotateRight(h);
        flipColors(h);
    }
    return h;
}

// Contiene la clave?
static int contains( NodeRB* node , void* key , int (*cmp)(void*,void*) ){    // (11)
    return get(node,key,cmp) != NULL;
}

static NodeRB* get( NodeRB* node , void* key , int (*cmp)(void*,void*) ){
    NodeRB* search = node;
    while( search != NULL ){
        switch( (*cmp)(key,search->key) ){
            case LT: search = search->left;  continue;
            case EQ: return search;          continue;
            case GT: search = search->right; continue;
        }
    }
    return search;
}

static NodeRB* minimum( NodeRB* h ){
    NodeRB* min = h;
    while( min->left != NULL )
        min = min->left;
    return min;
}

// Balancea la carga del subárbol:
static NodeRB* balance( NodeRB* h ){
    if( isRed(h->right) && !isRed(h->left)       ) h = rotateLeft(h);
    if( isRed(h->left)  &&  isRed(h->left->left) ) h = rotateRight(h);
    if( isRed(h->left)  &&  isRed(h->right)      ) flipColors(h);

    h->N = nodeSize(h->left) + nodeSize(h->right) + 1;
    return h;
}


//      ----------------------------------------------
//      [#] Definición de funciones visibles (RefMap):
//      ----------------------------------------------
// Utilidades del árbol rojo y negro.

void refmap_init( RefMap* t                            ,
                  int   (*cmp)           (void*,void*) ,
                  void* (*copyProtocol)        (void*) ,
                  void  (*deallocationProtocol)(void*) ){
    STOP_IF_UNSAFE( copyProtocol , "refmap_init: Attempt to passing an invalid copyProtocol." );

    t->root    = NULL;
    t->cmp     = cmp;
    t->copykey = copyProtocol;
    if( deallocationProtocol == NULL ) t->freekey = free;
    else                               t->freekey = deallocationProtocol;
    // Atomicity:
    pthread_mutex_init( &(t->lock) , NULL ); // Do not pass any particular attribute.
}

int refmap_unsafe_empty( RefMap* t ){ return t->root == NULL; }
int refmap_unsafe_size ( RefMap* t ){ return nodeSize( t->root ); }

static int maxDepth( NodeRB* root ){
    if( root == NULL ) return 0;

    double depthD = ceil( log2(root->N) );
    int    depthI = (int) depthD;
    return depthI * 2;
}

// [$] Mutable traversal
// Worst-case cost 2*log2(N):
void refmap_unsafe_put( RefMap* t , void* key , void* value ){
    static int extra = 2;

    // :::::::::::::::::::::::
    // Non-Recursive Interface
    // :::::::::::::::::::::::
    // -- Nothing in particular for this case --

    // [^] Build Stack:
    NodeRB*** stack;
    int used = 0;
    int size = maxDepth( t->root ) + extra;

    stack = malloc( size * sizeof(NodeRB**) );
    STOP_IF_UNSAFE( stack , "refmap_unsafe_put: Unable to build stack" );

    // [F] Function Pointers:
    int   (*compare) (void*,void*) = t->cmp;
    void* (*allocate)(void*)       = t->copykey;

    // [S] Search:
    NodeRB** head;
    NodeRB*  node;
    int      DONE = 0;

    // Prelude:
    PUSH( stack , used , &t->root );
    // Mutable traversal:
    while( !DONE ){
        if( EMPTY ){ perror("refmap_unsafe_put: Stack smashed in bottom"); exit(1); }
        POP( stack , used , head );
        
        node = *head;
        if( node == NULL ){
            node        = malloc( sizeof(NodeRB) );
            void* clone = (*allocate)( key );
            STOP_IF_UNSAFE( clone , "refmap_unsafe_put: "
                                    " Unable to allocate memory for key." );
            STOP_IF_UNSAFE( node  , "refmap_unsafe_put: "
                                    " Unable to allocate memory for Node." );
            init( node , clone , value , RED , 1 );
            *head = node;

            DONE = 1;
            continue; // EXIT Recursive case.
        }

        // Recursive Step:
        switch( (*compare)(key , node->key) ){
            case LT:
                PUSH( stack , used , head );            // Needs to be balanced.
                PUSH( stack , used , &node->left );     // Go to down-left.
                continue;
            case EQ:
                node->value = value;
                PUSH( stack , used , head );            // Needs to be balanced.
                DONE = 1;
                continue; // EXIT Recursive case.
            case GT:
                PUSH( stack , used , head );            // Needs to be balanced.
                PUSH( stack , used , &node->right );    // Go to down-right.
                continue;
        }
    }

    while( !EMPTY ){
        POP( stack , used , head );
        // Fix-up any right-leaning links:
        // Line 1. Larger case; Line 2. Smaller case; Line 3. Between case;
        if( isRed((*head)->right) && !isRed((*head)->left)       ) (*head) = rotateLeft(*head);
        if( isRed((*head)->left ) &&  isRed((*head)->left->left) ) (*head) = rotateRight(*head);
        if( isRed((*head)->left ) &&  isRed((*head)->right)      ) flipColors(*head);

        // Size update:
        node = *head;
        node->N = nodeSize(node->left) + nodeSize(node->right) + 1;
    }

    // [$!] TOTALLY REQUIRED:
    free( stack );

    // :::::::::::::
    // Exit Protocol
    // :::::::::::::
    t->root->color = BLACK;
}

    
void refmap_put( RefMap* t , void* key , void* value ){
    // |> BEGIN CRITICAL REGION
    pthread_mutex_lock( &t->lock );
    refmap_unsafe_put( t , key , value );
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &t->lock );
}

void* refmap_unsafe_get( RefMap* t , void* key ){
    if( t->root == NULL ) return NULL;
    else                  return get( t->root , key , t->cmp )->value;
}

void* refmap_extract( RefMap* t , void* key ){
    // |> BEGIN CRITICAL REGION
    pthread_mutex_lock( &t->lock );
    void* value = refmap_unsafe_get( t , key );
    refmap_unsafe_delete( t , key );
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &t->lock );

    return value;
}

// [$] Mutable traversal
// Returns the retrieved key.
static void* delete( NodeRB** root , void* key , int (*compare)(void*,void*) ){
    static int extra = 1;   // counts the head and the new node.

    // [^] Build Stack:
    NodeRB*** stack;
    int used = 0;
    int size = maxDepth( *root ) + extra;

    stack = malloc( size * sizeof(NodeRB**) );
    STOP_IF_UNSAFE( stack , "delete (NodeRB): Unable to build stack" );

    // [S] Search:
    NodeRB** head;
    NodeRB*  node;
    int      DONE = 0;
    void*   extracted_key = NULL;

    PUSH( stack , used , root );
    while( !DONE ){
        if( EMPTY ){ perror("delete (NodeRB): Stack smashed in bottom."); exit(1); }
        POP( stack , used , head );

        node = *head;
        if( (*compare)(key , node->key) == LT ){
            if( !isRed(node->left) && !isRed(node->left->left) ){
                node  = moveRedLeft( node );
                *head = node;
            }

            PUSH( stack , used , head );        // Requires to be balanced.
            PUSH( stack , used , &node->left );  // Go to down-left
        } else {
            if( isRed(node->left) ){ // Adjust.
                node  = rotateRight(node);
                *head = node;
            }

            if( (*compare)(key , node->key) == EQ && node->right == NULL ){
                extracted_key = node->key;
                node->key   = NULL;
                node->left  = NULL;
                node->right = NULL;
                // DEALLOCATE:
                free( *head );
                *head = NULL;

                DONE = 1;
                continue; // EXIT Recursive case.
            }

            if( !isRed(node->right) && !isRed(node->right->left) ){
                node  = moveRedRight(node);
                *head = node;
            }

            if( (*compare)(key , node->key) == EQ ){
                // DEALLOCATE KEY.
                extracted_key = node->key;

                // DELEGATE NODE DEALLICATION:
                node->value = minimum( node->right )->value;
                node->key   = deleteMin( &node->right );

                PUSH( stack , used , head );            // NOTE: Requires to be balanced
                DONE = 1;
                continue; // EXIT Recursive case.
            } else {
                PUSH( stack , used , head );            // Requires to be balanced
                PUSH( stack , used , &node->right );    // Go to down-right.
                continue;
            }
        }
    }

    while( !EMPTY ){
        POP( stack , used , head );
        *head = balance(*head);
    }

    // [$!] TOTALLY REQUIRED:
    free( stack );

    return extracted_key;
}

static void refmap_unsafe_delete( RefMap* t , void* key ){
    if( !refmap_unsafe_contains(t,key) ) return;

    NodeRB* root = t->root;
    if( !isRed(root->left) && !isRed(root->right) )
        root->color = RED;

    void* old_key = delete( &t->root , key , t->cmp );
    (*t->freekey)( old_key );

    if( !refmap_unsafe_empty(t) )
        t->root->color = BLACK;
}

void refmap_delete( RefMap* t , void* key ){
    // |> BEGIN CRITICAL REGION
    pthread_mutex_lock( &t->lock );
    refmap_unsafe_delete( t , key );
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &t->lock );
}

static void refmap_unsafe_deleteMin( RefMap* t ){
    // Initial Interface:
    if( refmap_unsafe_empty(t) ) return;

    NodeRB* root = t->root;
    if( !isRed(root->left) && !isRed(root->right) )
        t->root->color = RED;

    void* old_key = deleteMin(&t->root);
    (*t->freekey)( old_key );

    if( !refmap_unsafe_empty(t) )
        t->root->color = BLACK;
}

// [$] Mutable traversal
static void* deleteMin( NodeRB** root ){
    static int extra = 1;   // counts the head and the new node.

    // [^] Build Stack:
    NodeRB*** stack;
    int used = 0;
    int size = maxDepth( *root ) + extra;

    stack = malloc( size * sizeof(NodeRB**) );
    STOP_IF_UNSAFE( stack , "deleteMin (NodeRB): Unable to build stack" );

    // [S] Search:
    NodeRB** head;
    NodeRB*  node;
    int      DONE = 0;
    void*   extracted_key = NULL;

    PUSH( stack , used , root );
    while( !DONE ){
        if( EMPTY ){ perror("deleteMin (NodeRB): Stack smashed in bottom."); exit(1); }
        POP( stack , used , head );
        node = *head;

        if( node->left == NULL ){
            // DEALOCATE.
            extracted_key = node->key;
            node->key   = NULL;
            node->left  = NULL;
            node->right = NULL;
            node->value = NULL;

            free( *head );
            *head = NULL;

            DONE = 1;
            continue;   // EXIT Recursive case.
        }
        
        if( !isRed(node->left) && !isRed(node->left->left) ){
            node  = moveRedLeft(node);
            *head = node;
        }

        PUSH( stack , used , head );
        PUSH( stack , used , &node->left );
    }

    while( !EMPTY ){
        POP( stack , used , head );
        *head = balance(*head);
    }

    // [$!] TOTALLY REQUIRED:
    free( stack );

    return extracted_key;
}

void refmap_deleteMin( RefMap* t ){
    // |> BEGIN CRITICAL REGION
    pthread_mutex_lock( &t->lock );
    refmap_unsafe_deleteMin( t );
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &t->lock );
}

int refmap_unsafe_contains( RefMap* t , void* key ){
    return contains(t->root, key ,t->cmp);
}

// Prints always in stderr. key() and val() must print in stderr too.
static void debug( NodeRB* x , int indent , int incr ,
                   char black[] , char red[] , char normal[] ,
                   void (*key)(void*) , void (*val)(void*) ){

    if( x == NULL ) return;
    fprintf( stderr , "%*skey:   " , indent , "|> " );
    (*key)( x->key );
    putc( '\n' , stderr );

    fprintf( stderr , "%*svalue: " , indent , "  +" );
    (*val)( x->value );
    putc( '\n' , stderr );
    fprintf( stderr , "%*ssize:  %d\n" , indent , "   " , x->N );

    if( x->color == RED ) fprintf( stderr , "%*ccolor: %s%s" , indent , ' ' , red   , "RED   (<<L 3-nodes)" );
    else                  fprintf( stderr , "%*ccolor: %s%s" , indent , ' ' , black , "BLACK (=RL 2-nodes)" );
    fprintf( stderr , "%s\n" , normal );
    fprintf( stderr , "%*cleft:\n"    , indent , ' ' );
    debug( x->left  , indent + incr , incr , black , red , normal , key , val );
    debug( x->right , indent + incr , incr , black , red , normal , key , val );
}

static void print_opaque( void *unknow ){
    fprintf( stderr , "(void*) %p" , unknow );
}

void refmap_debug( RefMap* t , int use_ascii_color , void (*print_key)(void*) , void (*print_value)(void*) ){
    static char black[]  = "\x1b[38;5;32m";
    static char red[]    = "\x1b[38;5;9m";
    static char normal[] = "\x1b[0m";

    if( print_key   == NULL ) print_key   = &print_opaque;
    if( print_value == NULL ) print_value = &print_opaque;

    char *ublack , *ured , *unormal;
    if( use_ascii_color ){
        ublack  = black;
        ured    = red;
        unormal = normal;
    } else
        ublack = ured = unormal;
    fprintf( stderr , "%s" , unormal );
    debug( t->root , 4 , 4 ,
           ublack , ured , unormal ,
           print_key , print_value );
    fprintf( stderr , "%s" , unormal );
}

void refmap_clear( RefMap* t ){
    RefMap* self = t;
    // <| BEGIN CRITICAL REGION
    pthread_mutex_lock( &self->lock );

    void* k , *outk;
    while( !refmap_unsafe_empty(t) ){
        k    = t->root->key;
        refmap_unsafe_delete( t , k );
    }

    // <| END CRITICAL REGION
    pthread_mutex_unlock( &self->lock );
}

void refmap_destroy( RefMap* t ){
    RefMap* self = t;
    // <| BEGIN CRITICAL REGION
    pthread_mutex_lock( &self->lock );

    void* k , *outk;
    // BUG: when deletes the 6th elements, it delete twice, idk why, I must see what happens!
    while( !refmap_unsafe_empty(t) ){
        //refmap_debug(t);
        k    = t->root->key;
        refmap_unsafe_delete( t , k );
    }
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &self->lock );

    pthread_mutex_destroy( &self->lock );
}

void* refmap_unsafe_min( RefMap* t ){
    if( t->root == NULL ) return NULL;
    else                  return minimum(t->root)->key;
}

void* refmap_extract_min( RefMap* t ){
    // |> BEGIN CRITICAL REGION
    pthread_mutex_lock( &t->lock );
    void* value = refmap_unsafe_min(t);
    refmap_unsafe_deleteMin( t );
    // <| END CRITICAL REGION
    pthread_mutex_unlock( &t->lock );
    return value;
}

#undef LT
#undef EQ
#undef GT
#undef STOP_IF_UNSAFE
#undef PUSH
#undef POP
#undef EMPTY
