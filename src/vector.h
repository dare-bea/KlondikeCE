// Adapted from https://www.davidpriver.com/ctemplates.html.

// vector.h
// Include this header multiple times to implement a
// simplistic dynamic array.  Before inclusion define at
// least VECTOR_T to the type the dynamic array can hold.
// See VECTOR_NAME, VECTOR_PREFIX and VECTOR_LINKAGE for
// other customization points.
//
// If you define VECTOR_DECLS_ONLY, only the declarations
// of the type and its function will be declared.
//

#ifndef VECTOR_HEADER_H
#define VECTOR_HEADER_H
// Inline functions, #defines and includes that will be
// needed for all instantiations can go up here.
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // realloc, size_t
#include <string.h>

#define VECTOR_IMPL(word) VECTOR_COMB1(VECTOR_PREFIX,word)
#define VECTOR_COMB1(pre, word) VECTOR_COMB2(pre, word)
#define VECTOR_COMB2(pre, word) pre##word

#endif // VECTOR_HEADER_H

// NOTE: this section is *not* guarded as it is intended
// to be included multiple times.

// #define VECTOR_T int /* for errorchecking only */

#ifndef VECTOR_T
#error "VECTOR_T must be defined"
#endif

// The name of the data type to be generated.
// If not given, will expand to something like
// `Vector_int` for an `int`.
#ifndef VECTOR_NAME
#define VECTOR_NAME VECTOR_COMB1(VECTOR_COMB1(Vector,_), VECTOR_T)
#endif

// Prefix for generated functions.
#ifndef VECTOR_PREFIX
#define VECTOR_PREFIX VECTOR_COMB1(VECTOR_NAME, _)
#endif

// Customize the linkage of the function.
#ifndef VECTOR_LINKAGE
#define VECTOR_LINKAGE static inline
#endif

typedef struct VECTOR_NAME {
        // `items` must be `NULL` if and only if `capacity` is `0`.
        VECTOR_T *items;
        // `length` must be less than or equal to `capacity`.
        // `length` must be `0` if `items` is `NULL`.
        size_t length;
        // `capacity` must be greater than or equal to `length`.
        // `capacity` must be `0` if and only if `items` is `NULL`.
        size_t capacity;
} VECTOR_NAME;

#define VECTOR_free VECTOR_IMPL(free)
#define VECTOR_realloc VECTOR_IMPL(realloc)
#define VECTOR_reserve VECTOR_IMPL(reserve)
#define VECTOR_reserve_exact VECTOR_IMPL(reserve_exact)
#define VECTOR_shrink_to_fit VECTOR_IMPL(shrink_to_fit)
#define VECTOR_last VECTOR_IMPL(last)
#define VECTOR_last_const VECTOR_IMPL(last_const)
#define VECTOR_at VECTOR_IMPL(at)
#define VECTOR_at_const VECTOR_IMPL(at_const)
#define VECTOR_push VECTOR_IMPL(push)
#define VECTOR_insert VECTOR_IMPL(insert)
#define VECTOR_pop VECTOR_IMPL(pop)
#define VECTOR_remove VECTOR_IMPL(remove)
#define VECTOR_extend VECTOR_IMPL(extend)
#define VECTOR_extend_within VECTOR_IMPL(extend_within)
#define VECTOR_clear VECTOR_IMPL(clear)
#define VECTOR_nclear VECTOR_IMPL(nclear)
#define VECTOR_splice VECTOR_IMPL(splice)

/**
 * Frees this vector's array allocation and empties its contents.
 */
VECTOR_LINKAGE void VECTOR_free(VECTOR_NAME *this);
/**
 * Reallocates this vector to the given capacity.
 * 
 * If the new capacity is less than the current length, the vector will be
 * truncated.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - The allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_realloc(VECTOR_NAME *this, size_t new_capacity);
/**
 * Reserves a minimum capacity for this vector to hold.
 * 
 * No reallocation will be performed if the given capacity is less than the
 * current capacity.
 * 
 * Unlike `reserve_exact`, this will reserve extra memory to lower the amount
 * of allocations.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_reserve(VECTOR_NAME *this, size_t min_capacity);
/**
 * Reserves a minimum capacity for this vector to hold, but does not
 * request extra memory.
 * 
 * No reallocation will be performed if the given capacity is less than the
 * current capacity. (`true` will still be returned in this case.)
 * 
 * Unlike `reserve`, this will not reserve extra memory to lower the amount
 * of allocations.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_reserve_exact(VECTOR_NAME *this, size_t min_capacity);
/**
 * Reallocates this vector to shrink its capacity to its length.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - The reallocation failed.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_shrink_to_fit(VECTOR_NAME *this);

/**
 * Returns a pointer to the last element of this vector.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - The vector was empty.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_last(VECTOR_NAME *this);
/**
 * Returns a const pointer to the last element of this vector.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - The vector was empty.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T const *VECTOR_last_const(VECTOR_NAME const *this);
/**
 * Returns a pointer to the element at the specified index of this vector.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - The index was not in the range [0, length).
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_at(VECTOR_NAME *this, size_t index);
/**
 * Returns a const pointer to the element at the specified index of this vector.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - The index was not in the range [0, length).
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T const *VECTOR_at_const(VECTOR_NAME const *this, size_t index);

/**
 * Pushes the provided value at the end of this vector.
 * 
 * Returns a pointer to the new element.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_push(VECTOR_NAME *this, VECTOR_T in);
/**
 * Inserts the provided value at the specified index of this vector.
 * 
 * Returns a pointer to the new element.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 * - The insertion index was not in the range [0, length].
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_insert(VECTOR_NAME *this, size_t index, VECTOR_T in);

/**
 * Pops the value at the end of this vector.
 * 
 * If `out` is non-`NULL`, the popped value is written to `*out`.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - The vector was empty.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_pop(VECTOR_NAME *this, VECTOR_T *out);
/**
 * Removes the value at the specified index of this vector.
 * 
 * If `out` is non-`NULL`, the removed value is written to `*out`.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - The index was not in the range [0, length).
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_remove(VECTOR_NAME *this, size_t index, VECTOR_T *out);

/**
 * Appends the contents of the provided array to the end of this vector.
 * 
 * Returns a pointer to the first new element.
 * 
 * The provided array may not be derived from the vector's storage. To extend
 * from the vector's contents, use `extend_within`.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 * - The provided array was `NULL`.
 * - The provided array length was zero.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_extend(VECTOR_NAME *this,
        VECTOR_T const *restrict in_array,
        size_t count);
/**
 * Appends a portion of this vector to the end of itself.
 * 
 * Returns a pointer to the first new element.
 * 
 * If `NULL` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 * - The source indices were not in the range [0, length).
 * - The provided element count was zero.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_extend_within(VECTOR_NAME *this,
        size_t from_index,
        size_t count);

/**
 * Empties this vector to the provided array.
 * 
 * Returns the length of this vector before it was emptied.
 * 
 * The output array may not overlap with the vector's storage.
 * 
 * If the output array is non-`NULL`, it will be written with the contents of
 * the vector.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE size_t VECTOR_clear(VECTOR_NAME *this, VECTOR_T *restrict out_array);
/**
 * Empties this vector to the provided array of the specified maximum size.
 * 
 * Returns the length of this vector before it was emptied, or the maximum size
 * if it was less.
 * 
 * The output array may not overlap with the vector's storage.
 * 
 * If the output array is non-`NULL`, it will be written with the contents of
 * the vector with indices less than the specified maximum size.
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE size_t VECTOR_nclear(VECTOR_NAME *this, VECTOR_T *restrict out_array, size_t out_size);

/**
 * Removes elements from this vector and inserts new elements at the same location.
 * 
 * Removes the first `out_count` elements starting at `from_index`; and inserts
 * `in_count` elements at that index.
 * 
 * If `out_array` is non-`NULL`, the removed elements will be written to it.
 * 
 * If `in_array` is non-`NULL`, the inserted elements will be sourced from it.
 * If `in_array` is `NULL`, the inserted elements will be default initialized.
 * 
 * Neither the output array nor the input array may overlap with the vector's
 * storage, and they may not overlap with each other.
 * 
 * If `false` is returned, one of the following occurred and no operation was performed:
 * - An allocation or reallocation failed.
 * - The size of the allocation or reallocation in bytes cannot be stored in a `size_t`.
 * - The starting index was out of the range [0, length].
 * - At least one removed index was out of the range [0, length).
 */
__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_splice(VECTOR_NAME *this,
        size_t from_index,
        VECTOR_T *restrict out_array,
        size_t out_count,
        VECTOR_T const *restrict in_array,
        size_t in_count);

#ifndef VECTOR_DECLS_ONLY

VECTOR_LINKAGE void VECTOR_free(VECTOR_NAME *this)
{
        if (!this)
                return;
        free(this->items);
        this->items = NULL;
        this->length = 0;
        this->capacity = 0;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_realloc(VECTOR_NAME *this, size_t new_capacity)
{
        if (!new_capacity) {
                VECTOR_free(this);
                return true;
        }
        if (SIZE_MAX / sizeof(VECTOR_T) < new_capacity)
                return false;
        VECTOR_T *const new_ptr = realloc(this->items, new_capacity * sizeof(VECTOR_T));
        if (!new_ptr)
                return false;
        this->items = new_ptr;
        this->capacity = new_capacity;
        if (new_capacity < this->length)
                this->length = new_capacity;
        return true;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_reserve(VECTOR_NAME *this, size_t min_capacity)
{
        if (min_capacity <= this->capacity)
                return true;
        size_t new_capacity = this->capacity ? this->capacity : 4;
        while (new_capacity < min_capacity) {
                if (SIZE_MAX / 2 / sizeof(VECTOR_T) < new_capacity) {
                        new_capacity = min_capacity;
                        break;
                }
                new_capacity *= 2;
        }
        return VECTOR_realloc(this, new_capacity);
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_reserve_exact(VECTOR_NAME *this, size_t min_capacity)
{
        if (min_capacity <= this->capacity)
                return true;
        return VECTOR_realloc(this, min_capacity);
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_shrink_to_fit(VECTOR_NAME *this)
{
        return VECTOR_realloc(this, this->length);
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_last(VECTOR_NAME *this)
{
        return !this->length ? NULL : &this->items[this->length - 1];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T const *VECTOR_last_const(VECTOR_NAME const *this)
{
        return !this->length ? NULL : &this->items[this->length - 1];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_at(VECTOR_NAME *this, size_t index)
{
        return this->length <= index ? NULL : &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T const *VECTOR_at_const(VECTOR_NAME const *this, size_t index)
{
        return this->length <= index ? NULL : &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_push(VECTOR_NAME *this, VECTOR_T in)
{
        if (SIZE_MAX <= this->length)
                return NULL;
        if (!VECTOR_reserve(this, this->length + 1))
                return NULL;
        const size_t index = this->length;
        this->items[index] = in;
        ++this->length;
        return &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_insert(VECTOR_NAME *this, size_t index, VECTOR_T in)
{
        if (SIZE_MAX <= this->length || this->length < index)
                return NULL;
        if (!VECTOR_reserve(this, this->length + 1))
                return NULL;
        memmove(&this->items[index + 1], &this->items[index], (this->length - index) * sizeof(VECTOR_T));
        ++this->length;
        this->items[index] = in;
        return &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_pop(VECTOR_NAME *this, VECTOR_T *out)
{
        if (!this->length)
                return false;
        --this->length;
        if (out)
                *out = this->items[this->length];
        return true;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_remove(VECTOR_NAME *this, size_t index, VECTOR_T *out)
{
        if (this->length <= index)
                return false;
        if (out)
                *out = this->items[index];
        --this->length;
        memmove(&this->items[index], &this->items[index + 1], (this->length - index) * sizeof(VECTOR_T));
        return true;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_extend(VECTOR_NAME *this,
        VECTOR_T const *restrict in_array,
        size_t count)
{
        if (SIZE_MAX - count < this->length || !in_array || !count)
                return NULL;
        if (!VECTOR_reserve(this, this->length + count))
                return NULL;
        const size_t index = this->length;
        memcpy(&this->items[index], in_array, count * sizeof(VECTOR_T));
        this->length += count;
        return &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE VECTOR_T *VECTOR_extend_within(VECTOR_NAME *this,
        size_t from_index,
        size_t count)
{
        if (SIZE_MAX - count < this->length
            || this->length < from_index
            || this->length - from_index < count
            || !count)
                return NULL;
        if (!VECTOR_reserve(this, this->length + count))
                return NULL;
        const size_t index = this->length;
        memcpy(&this->items[index], &this->items[from_index], count * sizeof(VECTOR_T));
        this->length += count;
        return &this->items[index];
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE size_t VECTOR_clear(VECTOR_NAME *this, VECTOR_T *restrict out_array)
{
        const size_t count = this->length;
        this->length = 0;
        if (out_array)
                memcpy(out_array, this->items, count * sizeof(VECTOR_T));
        return count;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE size_t VECTOR_nclear(VECTOR_NAME *this, VECTOR_T *restrict out_array, size_t out_size)
{
        const size_t count = out_size < this->length ? out_size : this->length;
        this->length = 0;
        if (out_array)
                memcpy(out_array, this->items, count * sizeof(VECTOR_T));
        return count;
}

__attribute__((nonnull(1)))
VECTOR_LINKAGE bool VECTOR_splice(VECTOR_NAME *this,
        size_t from_index,
        VECTOR_T *restrict out_array,
        size_t out_count,
        VECTOR_T const *restrict in_array,
        size_t in_count)
{
        if (this->length < from_index
            || this->length - from_index < out_count
            || SIZE_MAX - in_count < this->length - out_count)
                return false;
        const size_t new_length = this->length - out_count + in_count;
        if (!VECTOR_reserve(this, new_length))
                return false;

        if (out_array)
                memcpy(out_array, &this->items[from_index], out_count * sizeof(VECTOR_T));

        memmove(&this->items[from_index + in_count],
                &this->items[from_index + out_count],
                (this->length - from_index - out_count) * sizeof(VECTOR_T));

        if (in_array) {
                memcpy(&this->items[from_index], in_array, in_count * sizeof(VECTOR_T));
        } else {
                for (size_t i = from_index; i < from_index + in_count; ++i) {
                        this->items[i] = (VECTOR_T){0};
                }
        }

        this->length = new_length;
        return true;
}

#endif

// Cleanup
// These need to be undef'ed so they can be redefined the
// next time you need to instantiate this template.


#undef VECTOR_free
#undef VECTOR_realloc
#undef VECTOR_reserve
#undef VECTOR_reserve_exact
#undef VECTOR_shrink_to_fit
#undef VECTOR_last
#undef VECTOR_last_const
#undef VECTOR_at
#undef VECTOR_at_const
#undef VECTOR_push
#undef VECTOR_insert
#undef VECTOR_pop
#undef VECTOR_remove
#undef VECTOR_extend
#undef VECTOR_extend_within
#undef VECTOR_clear
#undef VECTOR_nclear
#undef VECTOR_splice

#undef VECTOR_T
#undef VECTOR_PREFIX
#undef VECTOR_NAME
#undef VECTOR_LINKAGE
#ifdef VECTOR_DECLS_ONLY
#undef VECTOR_DECLS_ONLY
#endif
