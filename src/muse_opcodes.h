/**
 * @file muse_opcodes.h
 * @author Srikumar K. S. (mailto:kumar@muvee.com)
 *
 * Copyright (c) 2006 Jointly owned by Srikumar K. S. and muvee Technologies Pte. Ltd. 
 *
 * All rights reserved. See LICENSE.txt distributed with this source code
 * or http://muvee-symbolic-expressions.googlecode.com/svn/trunk/LICENSE.txt
 * for terms and conditions under which this software is provided to you.
 */


#ifndef __MUSE_OPCODES_H__
#define __MUSE_OPCODES_H__

#ifndef __MUSE_H__
#include "muse.h"
#endif

BEGIN_MUSE_C_FUNCTIONS

/**
 * A single cell is used to represent an integer
 * in muSE. Integers are all 64-bit to avoid any
 * conversion and data loss issues.
 */
typedef longlong_t muse_int_cell;

/**
 * A float in muSE is a 64-bit double.
 */
typedef double muse_float_cell;

/**
 * A cons cell consists of references to two muse
 * cells called the head and the tail. This pair
 * structure is used in the construction of lists,
 * where the tail of the first cons cell of a list
 * refers to the "rest of the list".
 */
typedef struct { muse_cell head, tail; } muse_cons_cell;

/**
 * muSE provides the facility to invoke C functions
 * during the evaluation process. Such a C-function is
 * stored in a standard muSE cell as a function pointer
 * \c fn and a \c context pointer that provides arbitrary
 * data to the function that is not managed by the muse
 * environment.
 */ 
typedef struct { muse_nativefn_t fn; void *context; }	muse_nativefn_cell;

/**
 * A string is represented in a single cell by storing
 * a pair of pointers to the start of the string and
 * the end of the string. The end pointer points to the
 * '\0' character that is stored at the end of the string.
 * The text cell is managed by the muse environment and
 * will be de4stroyed at garbage collection time if no
 * other cells refer to it.
 */
typedef struct { muse_char *start, *end; }	muse_text_cell;

/**
 * A muse cell is a union of all the possible cell types.
 * In many lisp systems the cell data itself encodes the 
 * type of the contents. In muSE, only the cell reference
 * encodes the cell's content type. A cell reference
 * of type \c muse_cell is a 32-bit integer whose least
 * 3-bits encode the cell type and the higher bits encode
 * the index to the cell within the heap.
 * 
 * In a 32-bit build, \c muse_cell_data will be 64-bits
 * in size. In a 64-bit, it'll be 128-bits in size.
 */
typedef union
{
	muse_int_cell		i;
	muse_float_cell		f;
	muse_cons_cell		cons;
	muse_nativefn_cell	fn;
	muse_text_cell		text;
} muse_cell_data;

/**
 * A stack is used to keep track of temporary 
 * references to objects so that a cons-ing operation
 * does not result in an object being inadvertently
 * garbage collected.
 */
typedef struct 
{
	int size; 			/**< The size of the stack in cells. */
	muse_cell *bottom; 	/**< Points to the first element of the stack. */
	muse_cell *top;		/**< Points to the cell of the stack that will receive
							the next cell pushed on top of the stack. */
} muse_stack;

/**
 * The muse heap is an array of cells where the cells available
 * for allocation are collected into a free list.
 */
typedef struct
{
	int					size_cells;	/**< The heap size given in number of cells. */
	muse_cell_data		*cells;		/**< Pointer to the heap of cells. */
	unsigned char		*marks;		/**< An array of marks that is used to keep track
										of cell references during garbage collection.
										Each cell is given 1-bit in the marks array,
										hence the size of the marks array is 1/8 of the
										total number of cells in the heap. */
	muse_cell			free_cells;		/**< A reference to the first cell in the free list. */
	int					free_cell_count; /**< The number of free cells. This is used nearly
											only for diagnostic purposes. May be removed in the
											future for efficiency reasons. */
} muse_heap;

typedef enum
{
	MUSE_PROCESS_DEAD			= 0x0,
	MUSE_PROCESS_VIRGIN			= 0x1,
	MUSE_PROCESS_PAUSED			= 0x2,
	MUSE_PROCESS_RUNNING		= 0x4,
	MUSE_PROCESS_WAITING		= 0x8,
	MUSE_PROCESS_HAS_TIMEOUT	= 0x10
} muse_process_state_bits_t;

/**
 * A frame is the local environment of a process.
 */
typedef struct _muse_process_frame_t
{
	struct _muse_process_frame_t *next, *prev;

	int			state_bits;
	int			attention;
	int			remaining_attention;
	int			atomicity;
	jmp_buf		jmp;
	muse_int	timeout_us;
	muse_stack	stack;
	muse_stack	bindings_stack;
	/**<
	 * The bindings stack is used to manage bindings during
	 * continuation invocation. The stack structure is similar
	 * to \c muse_stack_t in that every entry is a cell, but
	 * there is a meaning to the entries on the stack - they
	 * are all in pairs - the even indices are all symbols
	 * and the odd indices are all their bound values.
	 */

	muse_stack	locals;
	/**<
	 * The "locals" are a set of process-local storage values.
	 * A symbol's value is different for each process.
	 */

	muse_stack	cstack; ///< Holds the C stack pointer. If the pointer is NULL, its the main process.

	muse_cell	thunk;
	muse_cell	mailbox;
	muse_cell	mailbox_end;

} muse_process_frame_t;

/**
 * The muse environment contains all info relevant to
 * evaluation of expressions in muSE.
 * 
 * @see g_muse_env
 */
struct _muse_env
{
	muse_heap			heap;
//	muse_stack			stack;
	muse_stack			symbol_stack;
	int					num_symbols;
//	muse_stack			bindings_stack;

	muse_cell			specials;
	muse_cell			*builtin_symbols;
	int					*parameters;
	void				*stack_base;
	void				*timer;
	muse_process_frame_t	*current_process;
};

extern muse_env *g_muse_env;
extern const char *g_muse_typenames[];

static inline muse_env *_env() 
{ 
	return g_muse_env; 
}

/** 
 * Returns the T symbol that is used to represent "TRUE" 
 * if no other object is available to represent it.
 */
static inline muse_cell _t()
{
	return _env()->builtin_symbols[MUSE_T];
}

/**
 * The cell index is stored in the upper 29 bits
 * of the \c muse_cell. This returns the index of 
 * a cell referred by a \c muse_cell.
 */
static inline muse_cell _celli( muse_cell cell ) 
{ 
	return cell >> 3; 
}

/**
 * Returns a reference to the cell at the
 * given index \p i.
 */
static inline muse_cell _cellati( int i )
{
	return i << 3;
}

/**
 * Returns a newly allocated process local cell.
 * The return value is an index into the locals stack.
 */
static inline int _newlocal()
{
	return _env()->num_symbols++;
}
static inline muse_cell _localcell(int ix)
{
	return 7 + (ix << 3);
}

/**
 * Returns the type of the cell referred to by
 * the given cell reference. The type is encoded in 
 * the least 3 bits of the \c muse_cell.
 */
static inline muse_cell_t _cellt( muse_cell cell ) 
{ 
	return (muse_cell_t)(cell & 7); 
}
static inline const char *_typename( muse_cell cell )
{
	return g_muse_typenames[_cellt(cell)];
}
static inline muse_boolean _isnumbert( int cell_t )
{
	return (cell_t == MUSE_INT_CELL || cell_t == MUSE_FLOAT_CELL) ? MUSE_TRUE : MUSE_FALSE;
}
static inline muse_boolean _isnumber( muse_cell cell )
{
	return _isnumbert(_cellt(cell));
}
static inline muse_boolean _isfn( muse_cell cell )
{
	int t = _cellt(cell);
	return (t == MUSE_NATIVEFN_CELL || t == MUSE_LAMBDA_CELL) ? MUSE_TRUE : MUSE_FALSE;
}
static inline muse_boolean _isquote( muse_cell cell )
{
	return (cell == _env()->builtin_symbols[MUSE_QUOTE]) ? MUSE_TRUE : MUSE_FALSE;
}
static inline muse_cell _setcellt( muse_cell cell, muse_cell_t t ) 
{ 
	return (muse_cell)((cell & ~7) | t); 
}
static inline muse_cell_data *_ptr( muse_cell cell )
{
	muse_assert( cell >= 0 );
	muse_assert( _celli(cell) < _env()->heap.size_cells );
	return _env()->heap.cells + _celli(cell);
}
static inline muse_functional_object_t *_fnobjdata( muse_cell c )
{
	if ( _cellt(c) == MUSE_NATIVEFN_CELL )
	{
		muse_functional_object_t *d = (muse_functional_object_t*)_ptr(c)->fn.context;
		if ( d && d->magic_word == 'muSE' )
			return d;
	}

	return NULL;
}
static inline muse_int _intvalue( muse_cell c )
{
	switch ( _cellt(c) )
	{
		case MUSE_INT_CELL : return _ptr(c)->i;
		case MUSE_FLOAT_CELL : return (muse_int)_ptr(c)->f;
		default : return 0;
	}
}
static inline muse_float _floatvalue( muse_cell c )
{
	switch ( _cellt(c) )
	{
		case MUSE_INT_CELL : return (muse_float)_ptr(c)->i;
		case MUSE_FLOAT_CELL : return _ptr(c)->f;
		default : return 0.0;
	}
}
static inline muse_heap *_heap()
{
	return &_env()->heap;
}
static inline muse_stack *_stack()
{
	muse_stack *s = &_env()->current_process->stack;
	return s;
}
static inline muse_cell _spush( muse_cell cell )
{
	if ( cell )
	{
		muse_assert( _stack()->top - _stack()->bottom < _stack()->size );
		muse_assert( _celli(cell) >= 0 && _celli(cell) < _env()->heap.size_cells );
		return *(_stack()->top++) = cell;
	}
	else
		return MUSE_NIL;
}
static inline int _spos()
{
	return (int)(_stack()->top - _stack()->bottom);
}
static inline void _unwind( int stack_pos )
{
	_stack()->top = _stack()->bottom + stack_pos;
}
static inline muse_stack *_symstack()
{
	return &_env()->symbol_stack;
}
static inline muse_cell _head( muse_cell c )
{
	muse_assert( _cellt(c) == MUSE_CONS_CELL || _cellt(c) == MUSE_SYMBOL_CELL || _cellt(c) == MUSE_LAMBDA_CELL );
	return _ptr(c)->cons.head;
}
static inline muse_cell _tail( muse_cell c )
{
	muse_assert( _cellt(c) == MUSE_CONS_CELL || _cellt(c) == MUSE_SYMBOL_CELL || _cellt(c) == MUSE_LAMBDA_CELL );
	return _ptr(c)->cons.tail;
}
static inline muse_cell _symname( muse_cell sym )
{
	return _tail(_head(_tail(sym)));
}
static inline muse_cell _step( muse_cell *c )
{
	muse_cell _c = *c;
	(*c) = _tail(*c);
	return _c;
}
static inline muse_cell _next( muse_cell *c )
{
	return _head(_step(c));
}
static inline void _lpush( muse_cell h, muse_cell *l )
{
	_ptr(h)->cons.tail = *l;
	(*l) = h;
}
static inline void _seth( muse_cell c, muse_cell h )
{
	muse_assert( _cellt(c) == MUSE_CONS_CELL || _cellt(c) == MUSE_SYMBOL_CELL || _cellt(c) == MUSE_LAMBDA_CELL );
	muse_assert( h < 0 || _celli(h) < _env()->heap.size_cells );
	_ptr(c)->cons.head = h;
}
static inline void _sett( muse_cell c, muse_cell t )
{
	muse_assert( _cellt(c) == MUSE_CONS_CELL || _cellt(c) == MUSE_SYMBOL_CELL || _cellt(c) == MUSE_LAMBDA_CELL );
	muse_assert( t < 0 || _celli(t) < _env()->heap.size_cells );
	_ptr(c)->cons.tail = t;
}
static inline void _setht( muse_cell c, muse_cell h, muse_cell t )
{
	muse_cell_data *p = _ptr(c);
	muse_assert( _cellt(c) == MUSE_CONS_CELL || _cellt(c) == MUSE_SYMBOL_CELL || _cellt(c) == MUSE_LAMBDA_CELL );
	muse_assert( h < 0 || _celli(h) < _env()->heap.size_cells );
	muse_assert( t < 0 || _celli(t) < _env()->heap.size_cells );
	p->cons.head = h;
	p->cons.tail = t;
}
static inline muse_cell _def( muse_cell symbol, muse_cell value )
{
	_env()->current_process->locals.bottom[_head(symbol)>>3] = value;
	return value;
}
static inline muse_cell _symval( muse_cell symbol )
{
	return _env()->current_process->locals.bottom[_head(symbol)>>3];
}
static inline int _bspos()
{
	return (int)(_env()->current_process->bindings_stack.top - _env()->current_process->bindings_stack.bottom);
}
static inline void _push_binding( muse_cell symbol )
{
	muse_stack *s = &_env()->current_process->bindings_stack;
	muse_assert( s->top - s->bottom < s->size - 1 );
	*(s->top++) = symbol;
	*(s->top++) = _symval(symbol);
}
static inline void _unwind_bindings( int pos )
{
	muse_stack *s = &_env()->current_process->bindings_stack;
	muse_assert( pos >= 0 && pos <= s->top - s->bottom );

	{
		muse_cell *p = s->bottom + pos;
		while ( s->top > p )
		{
			s->top -= 2;
			_def( s->top[0], s->top[1] );
		}
	}
}
static inline void _mark( muse_cell c )
{
	int ci = _celli(c);
	unsigned char *m = _heap()->marks + (ci >> 3);
	muse_assert( ci >= 0 && ci < _heap()->size_cells );
	(*m) |= (1 << (ci & 7));
}
static inline void _unmark( muse_cell c )
{
	int ci = _celli(c);
	unsigned char *m = _heap()->marks + (ci >> 3);
	muse_assert( ci >= 0 && ci < _heap()->size_cells );
	(*m) &= ~(1 << (ci & 7));
}
static inline int _ismarked( muse_cell c )
{
	if ( (c & 7) == 7 ) 
		return 7; /**< TLS is always considered marked. */
	else
	{
		int ci = _celli(c);
		const unsigned char *m = _heap()->marks + (ci >> 3);
		muse_assert( ci >= 0 && ci < _heap()->size_cells );
		return (*m) & (1 << (ci & 7));
	}
}
static inline int _iscompound( muse_cell c )
{
	return _cellt(c) < MUSE_NATIVEFN_CELL;
}
static inline muse_cell _takefreecell()
{
	muse_cell c;
	c = _step( &(_env()->heap.free_cells) );
	_sett(c,MUSE_NIL);
	_env()->heap.free_cell_count--;
	return c;
}
static inline void _returncell( muse_cell c )
{
	muse_cell *f = &_env()->heap.free_cells;
	_setht(c, MUSE_NIL, *f);
	(*f) = c;
	_env()->heap.free_cell_count++;
}
static inline muse_cell _qq( muse_cell c )
{
	return c > 0 ? -c : c;
}
static inline muse_cell _quq( muse_cell c )
{
	return c < 0 ? -c : c;
}

/* Process functions. */
muse_process_frame_t *create_process( muse_env *env, int attention, muse_cell thunk, void *sp );
muse_process_frame_t *init_process_mailbox( muse_process_frame_t *p );
muse_boolean prime_process( muse_env *env, muse_process_frame_t *process );
muse_boolean switch_to_process( muse_env *env, muse_process_frame_t *process );
void yield_process( int spent_attention );
muse_boolean kill_process( muse_env *env, muse_process_frame_t *process );
muse_cell process_id( muse_process_frame_t *process );
void mark_process( muse_process_frame_t *p );
static muse_cell fn_pid( muse_env *env, muse_process_frame_t *process, muse_cell args );
void enter_atomic();
void leave_atomic();

END_MUSE_C_FUNCTIONS

#endif /* __MUSE_OPCODES_H__ */
