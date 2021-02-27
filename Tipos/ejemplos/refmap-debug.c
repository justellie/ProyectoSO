#include "../RefMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int char_cmp( void* ptr_a , void* ptr_b ){
    char a = *((char*) ptr_a) , b = *((char*) ptr_b);
    return ( ( a < b )?    -1 :
                ( a == b )? 0 : 1 );
}

void* char_cpy( void* int_ptr ){
    char* value = int_ptr;
    char* newv  = malloc( sizeof(char) );
    *newv = *value;
    return newv;
}

void print_empty_state( RefMap* m ){
    printf( "[RefMap] Is empty?: %s\n" , refmap_unsafe_empty(m)? "yes" : "no" );
    printf( "[RefMap] size:      %d\n" , refmap_unsafe_size(m) );
}

void unsafe_put( RefMap* map , char k , char* value ){
    refmap_put( map , &k , value );
}

void char_debug( void* char_value ){
    char* c = char_value;
    fprintf( stderr , "%c" , *c  );
}

int main(){
    char word[] = "SEARCHXMPL";
    char* w = word;
    int i = 0;

    RefMap map;
    refmap_init( &map , &char_cmp, char_cpy , NULL );
    printf( "-- Test: Verification: --\n" );
    while( *w != '\0' ){
        unsafe_put( &map , *w , w );
        //refmap_debug( &map , 1 , &char_debug , &char_debug );
        //fprintf( stderr , "Inserted %c ------\n" , *w );
        //refmap_debug( &map , 1 , NULL , NULL );
        //fprintf( stderr , "------------------\n\n" , *w );
        //
        w += 1;
        i += 1;
    }
    fprintf( stderr , "------ Delete test ------\n" , *w );
    refmap_debug( &map , 1 , &char_debug , &char_debug );

    printf( "Inserted: %d elements in the map.\n" , i );
    refmap_destroy( &map );
    printf( "Deleted: %d elements in the map.\n" ,  i );
    return 0;
}
