/* The MIT License

   Copyright (c) 2008-2009, by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef _AC_KLIST_H
#define _AC_KLIST_H

#include <stdlib.h>

#ifndef CALLOC_FUNC
#define CALLOC_FUNC(count, size) calloc(count, size)
#endif

#define KMEMPOOL_INIT(name, kmptype_t, kmpfree_f)						\
	typedef struct {													\
		size_t cnt, n, max;												\
		kmptype_t **buf;												\
	} kmp_##name##_t;													\
	static inline kmp_##name##_t *kmp_init_##name() {					\
		return CALLOC_FUNC(1, sizeof(kmp_##name##_t));						\
	}																	\
	static inline void kmp_destroy_##name(kmp_##name##_t *mp) {			\
		size_t k;														\
		for (k = 0; k < mp->n; ++k) {									\
			kmpfree_f(mp->buf[k]); free(mp->buf[k]);					\
		}																\
		free(mp->buf); free(mp);										\
	}																	\
	static inline kmptype_t *kmp_alloc_##name(kmp_##name##_t *mp) {		\
		++mp->cnt;														\
		if (mp->n == 0) return CALLOC_FUNC(1, sizeof(kmptype_t));			\
		return mp->buf[--mp->n];										\
	}																	\
	static inline void kmp_free_##name(kmp_##name##_t *mp, kmptype_t *p) { \
		--mp->cnt;														\
		if (mp->n == mp->max) {											\
			mp->max = mp->max? mp->max<<1 : 16;							\
			mp->buf = realloc(mp->buf, sizeof(void*) * mp->max);		\
		}																\
		mp->buf[mp->n++] = p;											\
	}

#define kmempool_t(name) kmp_##name##_t
#define kmp_init(name) kmp_init_##name()
#define kmp_destroy(name, mp) kmp_destroy_##name(mp)
#define kmp_alloc(name, mp) kmp_alloc_##name(mp)
#define kmp_free(name, mp, p) kmp_free_##name(mp, p)

//static inline int KL_CMP_GENERIC(a, b) {return ((a) - (b));}
#define KL_CMP_GENERIC(a, b) ((a) - (b)) 
#define KLIST_INIT(name, kltype_t, kmpfree_t) KLIST_INITEX(name, kltype_t, kmpfree_t, kl_cmp_##name)
#define KLIST_INITEX(name, kltype_t, kmpfree_t, klcmp_t)				\
	struct __kl1_##name {												\
		kltype_t data;													\
		struct __kl1_##name *next;										\
	};																	\
	typedef struct __kl1_##name kl1_##name;								\
	KMEMPOOL_INIT(name, kl1_##name, kmpfree_t)							\
	typedef struct {													\
		kl1_##name *head, *tail;										\
		kmp_##name##_t *mp;												\
		size_t size;													\
	} kl_##name##_t;													\
	static inline kl_##name##_t *kl_init_##name() {						\
		kl_##name##_t *kl = CALLOC_FUNC(1, sizeof(kl_##name##_t));		\
		kl->mp = kmp_init(name);										\
		kl->head = kl->tail = kmp_alloc(name, kl->mp);					\
		kl->head->next = 0;												\
		return kl;														\
	}																	\
	static inline void kl_destroy_##name(kl_##name##_t *kl) {			\
		kl1_##name *p;													\
		for (p = kl->head; p != kl->tail; p = p->next)					\
			kmp_free(name, kl->mp, p);									\
		kmp_free(name, kl->mp, p);										\
		kmp_destroy(name, kl->mp);										\
		free(kl);														\
	}																	\
	static inline kltype_t *kl_pushp_##name(kl_##name##_t *kl) {		\
		kl1_##name *q, *p = kmp_alloc(name, kl->mp);					\
		q = kl->tail; p->next = 0; kl->tail->next = p; kl->tail = p;	\
		++kl->size;														\
		return &q->data;												\
	}																	\
    static inline int kl_cmp_##name(kltype_t a, kltype_t b){return a-b;}\
    static inline kl1_##name *kl_find_##name(kl_##name##_t *kl, kltype_t val){\
		kl1_##name *p = NULL;                                           \
		if (kl->head->next != 0){								        \
            for (p=kl->head; p!=kl->tail;p=p->next){                    \
                if (klcmp_t(p->data, val) == 0)                         \
                    return p;                                           \
            }                                                           \
            p = NULL;                                                   \
        }                                                               \
        return p;                                                       \
    }                                                                   \
    typedef int (*klCanDel_##name_t)(kltype_t a);                       \
	static inline int kl_delex_##name(kl_##name##_t *kl, klCanDel_##name_t canDel) {\
        int result = 0;                                                 \
		kl1_##name *p, *q;                                              \
		if (kl->head->next != 0) {      								\
            for (q=p=kl->head;p!=kl->tail;p=p->next){                   \
                if (canDel(p->data)){                                   \
                    if (p!=q) q->next=p->next;                          \
                    else if (p=kl->head) kl->head=p->next;              \
                    kmp_free(name, kl->mp, p);                          \
                    p = q;                                              \
                    --kl->size;                                         \
                    result++;                                           \
                }                                                       \
                q = p;                                                  \
            }                                                           \
        }                                                               \
		return result;      											\
	}																	\
	static inline int kl_del_##name(kl_##name##_t *kl, kltype_t val) {	\
		kl1_##name *p, *q;                                              \
		if (kl->head->next == 0) return -1;								\
        for (q=p=kl->head;p!=kl->tail;p=p->next){                       \
            if (klcmp_t(p->data, val) == 0){                            \
                if (p!=q) q->next=p->next;                              \
                else kl->head=p->next;                                  \
                kmp_free(name, kl->mp, p);                              \
                --kl->size;                                             \
                return 0;                                               \
            }                                                           \
            q = p;                                                      \
        }                                                               \
		return -1;														\
	}																	\
	static inline int kl_shift_##name(kl_##name##_t *kl, kltype_t *d) { \
		kl1_##name *p;													\
		if (kl->head->next == 0) return -1;								\
		--kl->size;														\
		p = kl->head; kl->head = kl->head->next;						\
		if (d) *d = p->data;											\
		kmp_free(name, kl->mp, p);										\
		return 0;														\
	}

#define kliter_t(name) kl1_##name
#define klist_t(name) kl_##name##_t
#define kl_val(iter) ((iter)->data)
#define kl_next(iter) ((iter)->next)
#define kl_begin(kl) ((kl)->head)
#define kl_end(kl) ((kl)->tail)
#define kl_size(kl) ((kl)->size)

#define kl_init(name) kl_init_##name()
#define kl_destroy(name, kl) kl_destroy_##name(kl)
#define kl_pushp(name, kl) kl_pushp_##name(kl)
#define kl_shift(name, kl, d) kl_shift_##name(kl, d)
#define kl_del(name, kl, val) kl_del_##name(kl, val)
#define kl_delex(name, kl, canDel_callback) kl_delex_##name(kl, canDel_callback)
#define kl_find(name, kl, val) kl_find_##name(kl, val)


#endif
