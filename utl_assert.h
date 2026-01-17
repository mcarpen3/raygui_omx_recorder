#ifndef _UTIL_ASSERT_H_
#define _UTIL_ASSERT_H_

#define utl_assert_continue_local( _condition )   if( !( _condition ) ) { printf( "\033[1;31m%s #%d: Assert Continue\033[1;0m\n", __FILE__, __LINE__ ); }
#endif
