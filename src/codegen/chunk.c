#include <string.h>
#include <limits.h>

#include "phase.h"

#define STR_ENCRYPTION_CONSTANT 157135

static void do_sptr( struct codegen* codegen );
static void do_svct( struct codegen* codegen );
static bool svct_script( struct script* script );
static void do_sflg( struct codegen* codegen );
static void do_snam( struct codegen* codegen );
static void do_func( struct codegen* codegen );
static void do_fnam( struct codegen* codegen );
static void do_strl( struct codegen* codegen );
static void do_mini( struct codegen* codegen );
static bool mini_var( struct var* var );
static void do_aray( struct codegen* codegen );
static bool aray_var( struct var* var );
static bool aray_hidden_var( struct var* var );
static void do_aini( struct codegen* codegen );
static void do_aini_single( struct codegen* codegen, struct var* var );
static void do_load( struct codegen* codegen );
static void do_mimp( struct codegen* codegen );
static bool mimp_var( struct var* var );
static void do_aimp( struct codegen* codegen );
static bool aimp_array( struct var* var );
static void do_mexp( struct codegen* codegen );
static bool mexp_array( struct var* var );
static bool mexp_zeroinit_scalar( struct var* var );
static bool mexp_nonzeroinit_scalar( struct var* var );
static void do_mstr( struct codegen* codegen );
static bool mstr_var( struct var* var );
static void do_astr( struct codegen* codegen );
static bool astr_var( struct var* var );
static void do_atag( struct codegen* codegen );
static bool atag_var( struct var* var );
static void write_atagchunk( struct codegen* codegen, struct var* var );

void c_write_chunk_obj( struct codegen* codegen ) {
   if ( codegen->task->library_main->format == FORMAT_LITTLE_E ) {
      codegen->compress = true;
   }
   // Reserve header.
   c_add_int( codegen, 0 );
   c_add_int( codegen, 0 );
   c_write_user_code( codegen );
   int chunk_pos = c_tell( codegen );
   do_sptr( codegen );
   do_svct( codegen );
   do_sflg( codegen );
   do_snam( codegen );
   do_func( codegen );
   do_fnam( codegen );
   do_strl( codegen );
   do_mini( codegen );
   do_aray( codegen );
   do_aini( codegen );
   do_load( codegen );
   do_mimp( codegen );
   do_aimp( codegen );
   if ( codegen->task->library_main->importable ) {
      do_mexp( codegen );
      do_mstr( codegen );
      do_astr( codegen );
      do_atag( codegen );
   }
   // NOTE: When a BEHAVIOR lump is below 32 bytes in size, the engine will not
   // process it. It will consider the lump as being of unknown format, even
   // though it appears to be valid, just empty. An unknown format is an error,
   // and will cause the engine to halt execution of other scripts. Pad up to
   // the minimum limit to avoid this situation.
   while ( c_tell( codegen ) < 32 ) {
      c_add_byte( codegen, 0 );
   }
   c_seek( codegen, 0 );
   if ( codegen->task->library_main->format == FORMAT_LITTLE_E ) {
      c_add_str( codegen, "ACSe" );
   }
   else {
      c_add_str( codegen, "ACSE" );
   }
   c_add_int( codegen, chunk_pos );
   c_flush( codegen );
}

void do_sptr( struct codegen* codegen ) {
   if ( list_size( &codegen->task->library_main->scripts ) ) {
      struct {
         short number;
         short type;
         int offset;
         int num_param;
      } entry;
      c_add_str( codegen, "SPTR" );
      c_add_int( codegen, sizeof( entry ) *
         list_size( &codegen->task->library_main->scripts ) );
      list_iter_t i;
      list_iter_init( &i, &codegen->task->library_main->scripts );
      while ( ! list_end( &i ) ) {
         struct script* script = list_data( &i );
         entry.number = ( short ) script->assigned_number;
         entry.type = ( short ) script->type;
         entry.offset = script->offset;
         entry.num_param = script->num_param;
         c_add_sized( codegen, &entry, sizeof( entry ) );
         list_next( &i );
      }
   }
}

void do_svct( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->scripts );
   while ( ! list_end( &i ) ) {
      count += ( int ) svct_script( list_data( &i ) );
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   struct {
      short number;
      short size;
   } entry;
   c_add_str( codegen, "SVCT" );
   c_add_int( codegen, sizeof( entry ) * count );
   list_iter_init( &i, &codegen->task->library_main->scripts );
   while ( ! list_end( &i ) ) {
      struct script* script = list_data( &i );
      if ( svct_script( script ) ) {
         entry.number = ( short ) script->assigned_number;
         entry.size = ( short ) script->size;
         c_add_sized( codegen, &entry, sizeof( entry ) );
      }
      list_next( &i );
   }
}

inline bool svct_script( struct script* script ) {
   enum { DEFAULT_SCRIPT_SIZE = 20 };
   return ( script->size > DEFAULT_SCRIPT_SIZE );
}

void do_sflg( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->scripts );
   while ( ! list_end( &i ) ) {
      struct script* script = list_data( &i );
      if ( script->flags ) {
         ++count;
      }
      list_next( &i );
   }
   if ( count ) {
      struct {
         short number;
         short flags;
      } entry;
      c_add_str( codegen, "SFLG" );
      c_add_int( codegen, sizeof( entry ) * count );
      list_iter_init( &i, &codegen->task->library_main->scripts );
      while ( ! list_end( &i ) ) {
         struct script* script = list_data( &i );
         if ( script->flags ) {
            entry.number = ( short ) script->assigned_number;
            entry.flags = ( short ) script->flags;
            c_add_sized( codegen, &entry, sizeof( entry ) );
         }
         list_next( &i );
      }
   }
}

void do_snam( struct codegen* codegen ) {
   int count = 0;
   int total_length = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->scripts );
   while ( ! list_end( &i ) ) {
      struct script* script = list_data( &i );
      if ( script->named_script ) {
         struct indexed_string* name = t_lookup_string( codegen->task,
            script->number->value );
         total_length += name->length + 1;
         ++count;
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   int size =
      sizeof( int ) +
      sizeof( int ) * count +
      total_length;
   int padding = alignpad( size, 4 );
   size += padding;
   c_add_str( codegen, "SNAM" );
   c_add_int( codegen, size );
   c_add_int( codegen, count );
   // Offsets
   // -----------------------------------------------------------------------
   list_iter_init( &i, &codegen->task->library_main->scripts );
   int offset = sizeof( int ) + sizeof( int ) * count;
   while ( ! list_end( &i ) ) {
      struct script* script = list_data( &i );
      if ( script->named_script ) {
         c_add_int( codegen, offset );
         struct indexed_string* name = t_lookup_string( codegen->task,
            script->number->value );
         offset += name->length + 1;
      }
      list_next( &i );
   }
   // Text
   // -----------------------------------------------------------------------
   list_iter_init( &i, &codegen->task->library_main->scripts );
   while ( ! list_end( &i ) ) {
      struct script* script = list_data( &i );
      if ( script->named_script ) {
         struct indexed_string* name = t_lookup_string( codegen->task,
            script->number->value );
         c_add_sized( codegen, name->value, name->length + 1 );
      }
      list_next( &i );
   }
   while ( padding ) {
      c_add_byte( codegen, 0 );
      --padding;
   }
}

void do_func( struct codegen* codegen ) {
   int count = list_size( &codegen->task->library_main->funcs );
   // Imported functions:
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->funcs );
      while ( ! list_end( &k ) ) {
         struct func* func = list_data( &k );
         struct func_user* impl = func->impl;
         if ( impl->usage ) {
            ++count;
         }
         list_next( &k );
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   struct  {
      char params;
      char size;
      char value;
      char padding;
      int offset;
   } entry;
   entry.padding = 0;
   c_add_str( codegen, "FUNC" );
   c_add_int( codegen, sizeof( entry ) * count );
   // Imported functions:
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->funcs );
      entry.size = 0;
      entry.offset = 0;
      while ( ! list_end( &k ) ) {
         struct func* func = list_data( &k );
         struct func_user* impl = func->impl;
         if ( impl->usage ) {
            entry.params = ( char ) func->max_param;
            // In functions with optional parameters, a hidden parameter used
            // to store the number of arguments passed, is found after the
            // last function parameter.
            if ( func->min_param != func->max_param ) {
               ++entry.params;
            }
            entry.value = ( char ) ( func->return_type != NULL );
            c_add_sized( codegen, &entry, sizeof( entry ) );
         }
         list_next( &k );
      }
      list_next( &i );
   }
   // Visible functions:
   list_iter_init( &i, &codegen->task->library_main->funcs );
   while ( ! list_end( &i ) ) {
      struct func* func = list_data( &i );
      struct func_user* impl = func->impl;
      entry.params = ( char ) func->max_param;
      if ( func->min_param != func->max_param ) {
         ++entry.params;
      }
      entry.size = ( char ) ( impl->size - func->max_param );
      entry.value = ( char ) ( func->return_type != NULL );
      entry.offset = impl->obj_pos;
      c_add_sized( codegen, &entry, sizeof( entry ) );
      list_next( &i );
   }
}

void do_fnam( struct codegen* codegen ) {
   int count = 0;
   int size = 0;
   // Imported functions:
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->funcs );
      while ( ! list_end( &k ) ) {
         struct func* func = list_data( &k );
         struct func_user* impl = func->impl;
         if ( impl->usage ) {
            size += t_full_name_length( func->name ) + 1;
            ++count;
         }
         list_next( &k );
      }
      list_next( &i );
   }
   // Functions:
   list_iter_init( &i, &codegen->task->library_main->funcs );
   while ( ! list_end( &i ) ) {
      struct func* func = list_data( &i );
      size += t_full_name_length( func->name ) + 1;
      ++count;
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   int offset =
      sizeof( int ) +
      sizeof( int ) * count;
   int padding = alignpad( offset + size, 4 );
   c_add_str( codegen, "FNAM" );
   c_add_int( codegen, offset + size + padding );
   c_add_int( codegen, count );
   // Offsets:
   // -----------------------------------------------------------------------
   // Imported functions.
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->funcs );
      while ( ! list_end( &k ) ) {
         struct func* func = list_data( &k );
         struct func_user* impl = func->impl;
         if ( impl->usage ) {
            c_add_int( codegen, offset );
            offset += t_full_name_length( func->name ) + 1;
         }
         list_next( &k );
      }
      list_next( &i );
   }
   // Functions.
   list_iter_init( &i, &codegen->task->library_main->funcs );
   while ( ! list_end( &i ) ) {
      struct func* func = list_data( &i );
      c_add_int( codegen, offset );
      offset += t_full_name_length( func->name ) + 1;
      list_next( &i );
   }
   // Names:
   // -----------------------------------------------------------------------
   // Imported functions.
   struct str str;
   str_init( &str );
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->funcs );
      while ( ! list_end( &k ) ) {
         struct func* func = list_data( &k );
         struct func_user* impl = func->impl;
         if ( impl->usage ) {
            t_copy_name( func->name, true, &str );
            c_add_sized( codegen, str.value, str.length + 1 );
         }
         list_next( &k );
      }
      list_next( &i );
   }
   // Functions.
   list_iter_init( &i, &codegen->task->library_main->funcs );
   while ( ! list_end( &i ) ) {
      struct func* func = list_data( &i );
      t_copy_name( func->name, true, &str );
      c_add_sized( codegen, str.value, str.length + 1 );
      list_next( &i );
   }
   str_deinit( &str );
   while ( padding ) {
      c_add_byte( codegen, 0 );
      --padding;
   }
}

void do_strl( struct codegen* codegen ) {
   int i = 0;
   int count = 0;
   int size = 0;
   struct indexed_string* string = codegen->task->str_table.head_usable;
   while ( string ) {
      ++i;
      if ( string->used ) {
         // Plus one for the NUL character.
         size += string->length + 1;
         count = i;
      }
      string = string->next_usable;
   }
   if ( ! count ) {
      return;
   }
   int offset = 
      // String count, padded with a zero on each size.
      sizeof( int ) * 3 +
      // String offsets.
      sizeof( int ) * count;
   int padding = alignpad( offset + size, 4 );
   int offset_initial = offset;
   const char* name = "STRL";
   if ( codegen->task->library_main->encrypt_str ) {
      name = "STRE";
   }
   c_add_str( codegen, name );
   c_add_int( codegen, offset + size + padding );
   // String count.
   c_add_int( codegen, 0 );
   c_add_int( codegen, count );
   c_add_int( codegen, 0 );
   // Offsets.
   i = 0;
   string = codegen->task->str_table.head_usable;
   while ( i < count ) {
      if ( string->used ) {
         c_add_int( codegen, offset );
         // Plus one for the NUL character.
         offset += string->length + 1;
      }
      else {
         c_add_int( codegen, 0 );
      }
      string = string->next_usable;
      ++i;
   }
   // Strings.
   offset = offset_initial;
   string = codegen->task->str_table.head_usable;
   while ( string ) {
      if ( string->used ) {
         if ( codegen->task->library_main->encrypt_str ) {
            int key = offset * STR_ENCRYPTION_CONSTANT;
            // Each character of the string is encoded, including the NUL
            // character.
            for ( int i = 0; i <= string->length; ++i ) {
               char ch = ( char )
                  ( ( ( int ) string->value[ i ] ) ^ ( key + i / 2 ) );
               c_add_byte( codegen, ch );
            }
            offset += string->length + 1;
         }
         else {
            c_add_sized( codegen, string->value, string->length + 1 );
         }
      }
      string = string->next_usable;
   }
   while ( padding ) {
      c_add_byte( codegen, 0 );
      --padding;
   }
}

void do_mini( struct codegen* codegen ) {
   struct var* first_var = NULL;
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mini_var( var ) ) {
         if ( ! first_var ) {
            first_var = var;
         }
         ++count;
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   c_add_str( codegen, "MINI" );
   c_add_int( codegen, sizeof( int ) * ( count + 1 ) );
   c_add_int( codegen, first_var->index );
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mini_var( var ) ) {
         c_add_int( codegen, var->value->expr->value );
      }
      list_next( &i );
   }
}

inline bool mini_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP && ! var->dim &&
      var->type->primitive && var->value && var->value->expr->value );
}

void do_aray( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( var->storage == STORAGE_MAP &&
         ( var->dim || ! var->type->primitive ) ) {
         ++count;
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   struct {
      int number;
      int size;
   } entry;
   c_add_str( codegen, "ARAY" );
   c_add_int( codegen, sizeof( entry ) * count );
   // Arrays.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( aray_var( var ) ) { 
         entry.number = var->index;
         entry.size = var->size;
         c_add_sized( codegen, &entry, sizeof( entry ) );
      }
      list_next( &i );
   }
   // Hidden arrays.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( aray_hidden_var( var ) ) {
         entry.number = var->index;
         entry.size = var->size;
         c_add_sized( codegen, &entry, sizeof( entry ) );
      }
      list_next( &i );
   }
}

inline bool aray_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP &&
      ( var->dim || ! var->type->primitive ) && ! var->hidden );
}

inline bool aray_hidden_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP &&
      ( var->dim || ! var->type->primitive ) && var->hidden );
}

void do_aini( struct codegen* codegen ) {
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( var->storage == STORAGE_MAP &&
         ( var->dim || ! var->type->primitive ) && var->value ) {
         do_aini_single( codegen, var );
      }
      list_next( &i );
   }
}

void do_aini_single( struct codegen* codegen, struct var* var ) {
   int count = 0;
   struct value* value = var->value;
   while ( value ) {
      if ( value->string_initz ) {
         struct indexed_string_usage* usage =
            ( struct indexed_string_usage* ) value->expr->root;
         count = value->index + usage->string->length;
      }
      else {
         if ( value->expr->value ) {
            count = value->index + 1;
         }
      }
      value = value->next;
   }
   if ( ! count ) {
      return;
   }
   c_add_str( codegen, "AINI" );
   c_add_int( codegen, sizeof( int ) + sizeof( int ) * count );
   c_add_int( codegen, var->index );
   count = 0;
   value = var->value;
   while ( value ) {
      // Nullify uninitialized space.
      if ( count < value->index && ( ( value->expr->value &&
         ! value->string_initz ) || value->string_initz ) ) {
         c_add_int_zero( codegen, value->index - count );
         count = value->index;
      }
      if ( value->string_initz ) {
         struct indexed_string_usage* usage =
            ( struct indexed_string_usage* ) value->expr->root;
         for ( int i = 0; i < usage->string->length; ++i ) {
            c_add_int( codegen, usage->string->value[ i ] );
         }
         count += usage->string->length;
      }
      else {
         if ( value->expr->value ) {
            c_add_int( codegen, value->expr->value );
            ++count;
         }
      }
      value = value->next;
   }
}

void do_load( struct codegen* codegen ) {
   int size = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      size += lib->name.length + 1;
      list_next( &i );
   }
   if ( size ) {
      int padding = alignpad( size, 4 );
      c_add_str( codegen, "LOAD" );
      c_add_int( codegen, size + padding );
      list_iter_init( &i, &codegen->task->library_main->dynamic );
      while ( ! list_end( &i ) ) {
         struct library* lib = list_data( &i );
         c_add_sized( codegen, lib->name.value, lib->name.length + 1 );
         list_next( &i );
      }
      while ( padding ) {
         c_add_byte( codegen, 0 );
         --padding;
      }
   }
}

// NOTE: This chunk might cause any subsequent chunk to be misaligned.
void do_mimp( struct codegen* codegen ) {
   int size = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->vars );
      while ( ! list_end( &k ) ) {
         struct var* var = list_data( &k );
         if ( mimp_var( var ) ) {
            size += sizeof( int ) + t_full_name_length( var->name ) + 1;
         }
         list_next( &k );
      }
      list_next( &i );
   }
   if ( ! size ) {
      return;
   }
   c_add_str( codegen, "MIMP" );
   c_add_int( codegen, size );
   struct str str;
   str_init( &str );
   list_iter_init( &i, &codegen->task->library_main->dynamic );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->vars );
      while ( ! list_end( &k ) ) {
         struct var* var = list_data( &k );
         if ( mimp_var( var ) ) {
            c_add_int( codegen, var->index );
            t_copy_name( var->name, true, &str );
            c_add_sized( codegen, str.value, str.length + 1 );
         }
         list_next( &k );
      }
      list_next( &i );
   }
   str_deinit( &str );
}

inline bool mimp_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP && var->used && ! var->dim &&
      var->type->primitive );
}

// NOTE: This chunk might cause any subsequent chunk to be misaligned.
void do_aimp( struct codegen* codegen ) {
   int count = 0;
   int size = sizeof( int );
   list_iter_t i;
   list_iter_init( &i, &codegen->task->libraries );
   list_next( &i );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->vars );
      while ( ! list_end( &k ) ) {
         struct var* var = list_data( &k );
         if ( aimp_array( var ) ) {
            size +=
               // Array index.
               sizeof( int ) +
               // Array size.
               sizeof( int ) +
               // Name of array.
               t_full_name_length( var->name ) + 1;
            ++count;
         }
         list_next( &k );
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   c_add_str( codegen, "AIMP" );
   c_add_int( codegen, size );
   c_add_int( codegen, count );
   struct str str;
   str_init( &str );
   list_iter_init( &i, &codegen->task->libraries );
   list_next( &i );
   while ( ! list_end( &i ) ) {
      struct library* lib = list_data( &i );
      list_iter_t k;
      list_iter_init( &k, &lib->vars );
      while ( ! list_end( &k ) ) {
         struct var* var = list_data( &k );
         if ( aimp_array( var ) ) {
            c_add_int( codegen, var->index );
            c_add_int( codegen, var->size );
            t_copy_name( var->name, true, &str );
            c_add_sized( codegen, str.value, str.length + 1 );
         }
         list_next( &k );
      }
      list_next( &i );
   }
   str_deinit( &str );
}

inline bool aimp_array( struct var* var ) {
   return ( var->storage == STORAGE_MAP && var->used &&
      ( ! var->type->primitive || var->dim ) );
}

void do_mexp( struct codegen* codegen ) {
   int count = 0;
   int size = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( var->storage == STORAGE_MAP && ! var->hidden ) {
         size += t_full_name_length( var->name ) + 1;
         ++count;
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   int offset =
      // Number of variables. A zero is NOT padded on each side.
      sizeof( int ) +
      sizeof( int ) * count;
   int padding = alignpad( offset + size, 4 );
   c_add_str( codegen, "MEXP" );
   c_add_int( codegen, offset + size + padding );
   // Number of variables.
   c_add_int( codegen, count );
   // Offsets
   // -----------------------------------------------------------------------
   // Arrays.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_array( var ) ) {
         c_add_int( codegen, offset );
         offset += t_full_name_length( var->name ) + 1;
      }
      list_next( &i );
   }
   // Scalars, with-no-value.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_zeroinit_scalar( var ) ) {
         c_add_int( codegen, offset );
         offset += t_full_name_length( var->name ) + 1;
      }
      list_next( &i );
   }
   // Scalars, with-value.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_nonzeroinit_scalar( var ) ) {
         c_add_int( codegen, offset );
         offset += t_full_name_length( var->name ) + 1;
      }
      list_next( &i );
   }
   // Name
   // -----------------------------------------------------------------------
   struct str str;
   str_init( &str );
   // Arrays.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_array( var ) ) {
         t_copy_name( var->name, true, &str );
         c_add_sized( codegen, str.value, str.length + 1 );
      }
      list_next( &i );
   }
   // Scalars, with-no-value.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_zeroinit_scalar( var ) ) {
         t_copy_name( var->name, true, &str );
         c_add_sized( codegen, str.value, str.length + 1 );
      }
      list_next( &i );
   }
   // Scalars, with-value.
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( mexp_nonzeroinit_scalar( var ) ) {
         t_copy_name( var->name, true, &str );
         c_add_sized( codegen, str.value, str.length + 1 );
      }
      list_next( &i );
   }
   str_deinit( &str );
   while ( padding ) {
      c_add_byte( codegen, 0 );
      --padding;
   }
}

inline bool mexp_array( struct var* var ) {
   return ( var->storage == STORAGE_MAP &&
      ( var->dim || ! var->type->primitive ) && ! var->hidden );
}

inline bool mexp_zeroinit_scalar( struct var* var ) {
   return ( var->storage == STORAGE_MAP && ! var->dim &&
      var->type->primitive && ! var->hidden &&
      ( ! var->value || ! var->value->expr->value ) );
}

inline bool mexp_nonzeroinit_scalar( struct var* var ) {
   return ( var->storage == STORAGE_MAP && ! var->dim &&
      var->type->primitive && ! var->hidden && var->value &&
      var->value->expr->value );
}

void do_mstr( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      if ( mstr_var( list_data( &i ) ) ) {
         ++count;
      }
      list_next( &i );
   }
   if ( count ) {
      c_add_str( codegen, "MSTR" );
      c_add_int( codegen, sizeof( int ) * count );
      list_iter_init( &i, &codegen->task->library_main->vars );
      while ( ! list_end( &i ) ) {
         struct var* var = list_data( &i );
         if ( mstr_var( var ) ) {
            c_add_int( codegen, var->index );
         }
         list_next( &i );
      }
   }
}

inline bool mstr_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP && ! var->dim &&
      var->type->primitive && var->initial_has_str );
} 

void do_astr( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( astr_var( var ) ) {
         ++count;
      }
      list_next( &i );
   }
   if ( count ) {
      c_add_str( codegen, "ASTR" );
      c_add_int( codegen, sizeof( int ) * count );
      list_iter_init( &i, &codegen->task->library_main->vars );
      while ( ! list_end( &i ) ) {
         struct var* var = list_data( &i );
         if ( astr_var( var ) ) {
            c_add_int( codegen, var->index );
         }
         list_next( &i );
      }
   }
}

inline bool astr_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP && var->dim &&
      var->type->primitive && var->initial_has_str );
}

void do_atag( struct codegen* codegen ) {
   int count = 0;
   list_iter_t i;
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      if ( atag_var( list_data( &i ) ) ) {
         ++count;
      }
      list_next( &i );
   }
   if ( ! count ) {
      return;
   }
   list_iter_init( &i, &codegen->task->library_main->vars );
   while ( ! list_end( &i ) ) {
      struct var* var = list_data( &i );
      if ( atag_var( var ) ) {
         write_atagchunk( codegen, var );
      }
      list_next( &i );
   }
}

bool atag_var( struct var* var ) {
   return ( var->storage == STORAGE_MAP && ! var->type->primitive &&
      var->initial_has_str );
}

void write_atagchunk( struct codegen* codegen, struct var* var ) {
   enum { CHUNK_VERSION = 0 };
   enum {
      TAG_INTEGER,
      TAG_STRING,
      TAG_FUNCTION
   };
   int count = 0;
   struct value* value = var->value;
   while ( value ) {
      if ( value->expr->has_str && ! value->string_initz ) {
         count = value->index + 1;
      }
      value = value->next;
   }
   if ( ! count ) {
      return;
   }
   c_add_str( codegen, "ATAG" );
   c_add_int( codegen,
      // Version.
      sizeof( char ) +
      // Array number.
      sizeof( int ) +
      // Number of elements to tag.
      sizeof( char ) * count );
   c_add_byte( codegen, CHUNK_VERSION );
   c_add_int( codegen, var->index );
   value = var->value;
   int written = 0;
   while ( written < count ) {
      if ( value->expr->has_str && ! value->string_initz ) {
         while ( written < value->index ) {
            c_add_byte( codegen, TAG_INTEGER );
            ++written;
         }
         c_add_byte( codegen, TAG_STRING );
         ++written;
      }
      value = value->next;
   }
}