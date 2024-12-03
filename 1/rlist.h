#pragma once

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef typeof
/** 'typeof' is a GNU extension. */
#define typeof __typeof__
#endif

#ifndef __has_builtin
#  define __has_builtin(x) 0
#endif

#ifndef __has_attribute
#  define __has_attribute(x) 0
#endif

#ifndef offsetof
#  if __has_builtin(__builtin_offsetof)
#    define offsetof(type, member) __builtin_offsetof(type, member)
#  else
#    define offsetof(type, member) ((size_t)&((type *)0)->member)
#  endif
#endif /* offsetof */

/**
 * List entry and head structure.
 *
 * All functions has always_inline attribute. This way if caller
 * has no_sanitize_address attribute then rlist functions are not
 * ASAN instrumented too.
 */
struct rlist {
	struct rlist *prev;
	struct rlist *next;
};

/**
 * init list head (or list entry as ins't included in list)
 */
static inline void
rlist_create(struct rlist *list)
{
	list->next = list;
	list->prev = list;
}

/**
 * add item to list
 */
static inline void
rlist_add(struct rlist *head, struct rlist *item)
{
	item->prev = head;
	item->next = head->next;
	item->prev->next = item;
	item->next->prev = item;
}

/**
 * add item to list tail
 */
static inline void
rlist_add_tail(struct rlist *head, struct rlist *item)
{
	item->next = head;
	item->prev = head->prev;
	item->prev->next = item;
	item->next->prev = item;
}

/**
 * delete element
 */
static inline void
rlist_del(struct rlist *item)
{
	item->prev->next = item->next;
	item->next->prev = item->prev;
	rlist_create(item);
}

static inline struct rlist *
rlist_shift(struct rlist *head)
{
        struct rlist *shift = head->next;
        head->next = shift->next;
        shift->next->prev = head;
        shift->next = shift->prev = shift;
        return shift;
}

static inline struct rlist *
rlist_shift_tail(struct rlist *head)
{
        struct rlist *shift = head->prev;
        rlist_del(shift);
        return shift;
}

/**
 * return first element
 */
static inline struct rlist *
rlist_first(const struct rlist *head)
{
	return head->next;
}

/**
 * return last element
 */
static inline struct rlist *
rlist_last(const struct rlist *head)
{
	return head->prev;
}

/**
 * return next element by element
 */
static inline struct rlist *
rlist_next(const struct rlist *item)
{
	return item->next;
}

/**
 * return previous element
 */
static inline struct rlist *
rlist_prev(const struct rlist *item)
{
	return item->prev;
}

/**
 * return TRUE if list is empty
 */
static inline int
rlist_empty(const struct rlist *item)
{
	return item->next == item->prev && item->next == item;
}

/**
@brief delete from one list and add as another's head
@param to the head that will precede our entry
@param item the entry to move
*/
static inline void
rlist_move(struct rlist *to, struct rlist *item)
{
	rlist_del(item);
	rlist_add(to, item);
}

/**
@brief delete from one list and add_tail as another's head
@param to the head that will precede our entry
@param item the entry to move
*/
static inline void
rlist_move_tail(struct rlist *to, struct rlist *item)
{
	item->prev->next = item->next;
	item->next->prev = item->prev;
	item->next = to;
	item->prev = to->prev;
	item->prev->next = item;
	item->next->prev = item;
}

static inline void
rlist_swap(struct rlist *rhs, struct rlist *lhs)
{
	struct rlist tmp = *rhs;
	*rhs = *lhs;
	*lhs = tmp;
	/* Relink the nodes. */
	if (lhs->next == rhs)              /* Take care of empty list case */
		lhs->next = lhs;
	lhs->next->prev = lhs;
	lhs->prev->next = lhs;
	if (rhs->next == lhs)              /* Take care of empty list case */
		rhs->next = rhs;
	rhs->next->prev = rhs;
	rhs->prev->next = rhs;
}

/**
 * move all items of list head2 to the head of list head1
 */
static inline void
rlist_splice(struct rlist *head1, struct rlist *head2)
{
	if (!rlist_empty(head2)) {
		head1->next->prev = head2->prev;
		head2->prev->next = head1->next;
		head1->next = head2->next;
		head2->next->prev = head1;
		rlist_create(head2);
	}
}

/**
 * move all items of list head2 to the tail of list head1
 */
static inline void
rlist_splice_tail(struct rlist *head1, struct rlist *head2)
{
	if (!rlist_empty(head2)) {
		head1->prev->next = head2->next;
		head2->next->prev = head1->prev;
		head1->prev = head2->prev;
		head2->prev->next = head1;
		rlist_create(head2);
	}
}

/**
 * move the initial part of list head2, up to but excluding item,
 * to list head1; the old content of head1 is discarded
 */
static inline void
rlist_cut_before(struct rlist *head1, struct rlist *head2, struct rlist *item)
{
	if (head1->next == item) {
		rlist_create(head1);
		return;
	}
	head1->next = head2->next;
	head1->next->prev = head1;
	head1->prev = item->prev;
	head1->prev->next = head1;
	head2->next = item;
	item->prev = head2;
}

/**
 * list head initializer
 */
#define RLIST_HEAD_INITIALIZER(name) { &(name), &(name) }

/**
 * list link node
 */
#define RLIST_LINK_INITIALIZER { 0, 0 }

/**
 * allocate and init head of list
 */
#define RLIST_HEAD(name)	\
	struct rlist name = RLIST_HEAD_INITIALIZER(name)

/**
 * return entry by list item
 */
#define rlist_entry(item, type, member) ({				\
	(type *)( (char *)(item) - offsetof(type,member));		\
})

/**
 * return first entry
 */
#define rlist_first_entry(head, type, member)				\
	rlist_entry(rlist_first(head), type, member)

/**
 * Remove one element from the list and return it
 * @pre the list is not empty
 */
#define rlist_shift_entry(head, type, member)				\
        rlist_entry(rlist_shift(head), type, member)			\

/**
 * Remove one element from the list tail and return it
 * @pre the list is not empty
 */
#define rlist_shift_tail_entry(head, type, member)				\
        rlist_entry(rlist_shift_tail(head), type, member)			\


/**
 * return last entry
 * @pre the list is not empty
 */
#define rlist_last_entry(head, type, member)				\
	rlist_entry(rlist_last(head), type, member)

/**
 * return next entry
 */
#define rlist_next_entry(item, member)					\
	rlist_entry(rlist_next(&(item)->member), typeof(*item), member)

/**
 * return previous entry
 */
#define rlist_prev_entry(item, member)					\
	rlist_entry(rlist_prev(&(item)->member), typeof(*item), member)

#define rlist_prev_entry_safe(item, head, member)			\
	((rlist_prev(&(item)->member) == (head)) ? NULL :               \
	 rlist_entry(rlist_prev(&(item)->member), typeof(*item), member))

/**
 * add entry to list
 */
#define rlist_add_entry(head, item, member)				\
	rlist_add((head), &(item)->member)

/**
 * add entry to list tail
 */
#define rlist_add_tail_entry(head, item, member)			\
	rlist_add_tail((head), &(item)->member)

/**
delete from one list and add as another's head
*/
#define rlist_move_entry(to, item, member) \
	rlist_move((to), &((item)->member))

/**
delete from one list and add_tail as another's head
*/
#define rlist_move_tail_entry(to, item, member) \
	rlist_move_tail((to), &((item)->member))

/**
 * delete entry from list
 */
#define rlist_del_entry(item, member)					\
	rlist_del(&((item)->member))

/**
 * foreach through list
 */
#define rlist_foreach(item, head)					\
	for (item = rlist_first(head); item != (head); item = rlist_next(item))

/**
 * foreach backward through list
 */
#define rlist_foreach_reverse(item, head)				\
	for (item = rlist_last(head); item != (head); item = rlist_prev(item))

/**
 * return true if entry points to head of list
 *
 * NOTE: avoid using &item->member, because it may result in ASAN errors
 * in case the item type or member is supposed to be aligned, and the item
 * points to the list head.
 */
#define rlist_entry_is_head(item, head, member)				\
	((char *)(item) + offsetof(typeof(*item), member) == (char *)(head))

/**
 * foreach through all list entries
 */
#define rlist_foreach_entry(item, head, member)				\
	for (item = rlist_first_entry((head), typeof(*item), member);	\
	     !rlist_entry_is_head((item), (head), member);		\
	     item = rlist_next_entry((item), member))

/**
 * foreach backward through all list entries
 */
#define rlist_foreach_entry_reverse(item, head, member)			\
	for (item = rlist_last_entry((head), typeof(*item), member);	\
	     !rlist_entry_is_head((item), (head), member);		\
	     item = rlist_prev_entry((item), member))

/**
 * foreach through all list entries safe against removal
 */
#define	rlist_foreach_entry_safe(item, head, member, tmp)		\
	for ((item) = rlist_first_entry((head), typeof(*item), member);	\
	     !rlist_entry_is_head((item), (head), member) &&		\
	     ((tmp) = rlist_next_entry((item), member));                \
	     (item) = (tmp))

/**
 * foreach backward through all list entries safe against removal
 */
#define rlist_foreach_entry_safe_reverse(item, head, member, tmp)	\
	for ((item) = rlist_last_entry((head), typeof(*item), member);	\
	     !rlist_entry_is_head((item), (head), member) &&		\
	     ((tmp) = rlist_prev_entry((item), member));		\
	     (item) = (tmp))

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */
