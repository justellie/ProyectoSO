#include "../RefMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int int_cmp( void* ptr_a , void* ptr_b ){
    int a = *((int*) ptr_a) , b = *((int*) ptr_b);
    return ( ( a < b )?    -1 :
                ( a == b )? 0 : 1 );
}

// int cast_cmp( void* ptr_a , void* ptr_b ){
//     int a = (int) ptr_a , b = (int) ptr_b;
//     return ( ( a < b )?    -1 : ( a == b )? 0 : 1 );
// }

void* int_cpy( void* int_ptr ){
    int* value = (int*) int_ptr;
    int* newv  = malloc( sizeof(int) );
    *newv = *value;
    return newv;
}

void print_empty_state( RefMap* m ){
    printf( "[RefMap] Is empty?: %s\n" , refmap_unsafe_empty(m)? "yes" : "no" );
    printf( "[RefMap] size:      %d\n" , refmap_unsafe_size(m) );
}

void unsafe_lookup( RefMap* map , int k ){
    printf( "Does it have %d? %s\n" , k , refmap_unsafe_contains( map , &k )? "yes" : "no" );
}

void unsafe_put( RefMap* map , int k , int* value ){
    //printf( "Putting (%d,%d) into map.\n" , k , *value );
    refmap_put( map , &k , value );
}

// void refmap_put_int( RefMap* m , int key , int value ){
//     refmap_put( m , (void*) key , (void*) value );
// }

void int_debug( void* int_value ){
    int* n = int_value;
    fprintf( stderr , "%d" , *n  );
}

int main(){
    int keys[] = {  1,  2,  3,  4,  9};
    int vals[] = {-10,-20, 30, 44, 99};
    int n      = sizeof(keys) / sizeof(int);
    RefMap map;
    int skip  = 1;
    int skip2 = 1;
    if( !skip ){
        printf( "-- Test 1: Insert Values --\n" );
        refmap_init( &map , &int_cmp , int_cpy , NULL );
        print_empty_state(&map);
        putc( '\n' , stdout );

        for( int i = 0 ; i < n ; i += 1 ){
            refmap_put( &map , keys + i , vals + i );
        }

        int* reference;
        for( int i = 0 ; i < n ; i += 1 ){
            reference = refmap_unsafe_get(&map , keys + i);
            printf( "(Key: Value) -> (%d : %p [%d] ),\n" , keys[i] , reference , *reference );
        }
        putc( '\n' , stdout );

        print_empty_state(&map);
        refmap_destroy( &map );

        putc( '\n' , stdout );
        printf( "-- Test 2: Failures on search --\n" );

        refmap_init( &map , &int_cmp , int_cpy , NULL );
        refmap_put( &map , keys + 0 , vals + 0 ); //1
        refmap_put( &map , keys + 3 , vals + 3 ); //4

        unsafe_lookup( &map , 15  );
        unsafe_lookup( &map , 1   );
        unsafe_lookup( &map , 33  );
        unsafe_lookup( &map , -32 );
        unsafe_lookup( &map ,  99 );
        unsafe_lookup( &map , -10 );
        unsafe_lookup( &map ,  -4 );
        unsafe_lookup( &map ,   4 );

        refmap_clear( &map );
    }
    if( !skip2 ){
        refmap_init( &map , &int_cmp , int_cpy , NULL );
        printf( "-- Test 3: Simple Insertions/Deletions: --\n" );
        //int limit = 4;
        int limit = 5;
        for( int i = limit; i >= 1 ; i -= 1){
            unsafe_put( &map , i , vals );
        }
        printf( "Inserted: %d elements in the map.\n" , limit );
        refmap_debug( &map , 1 , int_debug , int_debug );
        refmap_destroy( &map );
        printf( "Deleted: %d elements in the map.\n" , limit );
    }

    refmap_init( &map , &int_cmp , int_cpy , NULL );
    printf( "\n-- Test 4: Intensive Insertions/Deletions: --\n" );
    printf( " > Size of Keys (int):    %d Bytes.\n" , sizeof(int)    );
    printf( " > Size of NodeRB:        %d Bytes.\n" , sizeof(NodeRB) );
    printf( "-- Press Enter to continue -- " );
    fgetc( stdin );

    int limit = 1000000;      // Before used 93 MiB, now uses 78MiB.
    // int limit = 10000000;  // Before used 916 MiB, now uses 764MiB.
    // int limit = 10000;        // uses 2 MiB.
    // int limit = 100000;     // Before used 11MiB, now uses 9 MiB.

    int bytes  = limit * sizeof(NodeRB);
    int kbytes = limit * sizeof(int);
    int total  = bytes + kbytes;
    for( int i = limit; i >= 1 ; i -= 1){
        unsafe_put( &map , i , vals );
    }
    printf( "Inserted: %d elements in the map.\n" , limit );
    printf( "Aproximated RAW size (only Nodes):\n"
            "    %d Bytes\n"
            "    %d KiB\n"
            "    %d MiB\n" , bytes , bytes / 1024 , bytes / (1024*1024) );
    printf( "Aproximated Keys size:\n"
            "    %d Bytes\n"
            "    %d KiB\n"
            "    %d MiB\n" , kbytes , kbytes / 1024 , kbytes / (1024*1024) );
    printf( "Aproximated RefMap Size:\n"
            "    %d Bytes\n"
            "    %d KiB\n"
            "    %d MiB\n" , total , total / 1024 , total / (1024*1024) );
    printf( "-- Press Enter to continue -- " );
    fgetc( stdin );

    refmap_destroy( &map );
    printf( "Deleted: %d elements in the map.\n" , limit );

    // Warnings:
    // refmap_init( &map , &cast_cmp );
    // refmap_put_int( &map , 100 , 70 );
    // int k = refmap_unsafe_get( &map , 100 );
    // printf( "(Key: Value) -> (%d : [%d] ),\n" , 100 , k );
    // refmap_destroy( &map );
    return 0;
}
