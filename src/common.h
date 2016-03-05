#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdbool.h>

#if defined( _WIN32 ) || defined( _WIN64 )
#   define OS_WINDOWS 1
#else
#   define OS_WINDOWS 0
#endif

void mem_init( void );
void* mem_alloc( size_t );
void* mem_realloc( void*, size_t );
void* mem_slot_alloc( size_t );
void mem_free( void* );
void mem_free_all( void );

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( a[ 0 ] ) )
#define STATIC_ASSERT( cond ) switch ( 0 ) { case 0: case cond: ; }
#define UNREACHABLE() printf( \
   "%s:%d: internal error: unreachable code\n", \
   __FILE__, __LINE__ );

struct str {
   char* value;
   int length;
   int buffer_length;
};

void str_init( struct str* );
void str_deinit( struct str* );
void str_copy( struct str*, const char* value, int length );
void str_grow( struct str*, int length );
void str_append( struct str*, const char* cstr );
void str_append_sub( struct str*, const char* cstr, int length );
void str_append_number( struct str*, int number );
void str_clear( struct str* );

struct list_link {
   struct list_link* next;
   void* data;
};

struct list {
   struct list_link* head;
   struct list_link* tail;
   int size;
};

typedef struct list_link* list_iter_t;

#define list_iter_init( i, list ) ( ( *i ) = ( list )->head )
#define list_end( i ) ( *( i ) == NULL )
#define list_data( i ) ( ( *i )->data )
#define list_next( i ) ( ( *i ) = ( *i )->next )
#define list_size( list ) ( ( list )->size )
#define list_head( list ) ( ( list )->head->data )
#define list_tail( list ) ( ( list )->tail->data )

void list_init( struct list* );
void list_append( struct list*, void* );
void list_append_head( struct list*, void* );
void list_deinit( struct list* );

struct options {
   struct list includes;
   const char* source_file;
   const char* object_file;
   const char* cache_path;
   int tab_size;
   int cache_lifetime;
   bool acc_err;
   bool one_column;
   bool help;
   bool preprocess;
   bool clear_cache;
   bool ignore_cache;
   bool write_asserts;
};

#if OS_WINDOWS

// NOTE: Volume information is not included. Maybe add it later.
struct fileid {
   int id_high;
   int id_low;
};

#define NEWLINE_CHAR "\r\n"

#else

#include <sys/types.h>
#include <sys/stat.h>

struct fileid {
   dev_t device;
   ino_t number;
};

#define NEWLINE_CHAR "\n"

struct fs_query {
   struct stat stat;
   bool exists;
};

#endif

bool c_read_fileid( struct fileid*, const char* path );
bool c_same_fileid( struct fileid*, struct fileid* );
bool c_read_full_path( const char* path, struct str* );
void c_extract_dirname( struct str* );

int alignpad( int size, int align_size );

void fs_init_query( struct fs_query* query, const char* path );
bool fs_exists( struct fs_query* query );
bool fs_isdir( struct fs_query* query );
int fs_get_mtime( struct fs_query* query );
bool fs_create_dir( const char* path );
const char* fs_get_tempdir( void );

#endif