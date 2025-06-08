#ifndef BASE_CONTAINER_H
#define BASE_CONTAINER_H

/////////////////////////////////////////////////////////////////////////////////////////
// Dynamic array

#define darray_push(arena, darray, item)                                                 \
  do {                                                                                   \
    (darray) = darray_hold(arena, (darray), 1, sizeof(*(darray)));                       \
    (darray)[darray_size(darray) - 1] = (item);                                          \
  } while (0);

internal U64 darray_size(void *darray);
internal void *darray_hold(Arena *arena, void *darray, U64 count, U64 item_size);
// void darray_free(void *darray);
internal void darray_clear(void *darray);

/////////////////////////////////////////////////////////////////////////////////////////
// B-Tree

#define BTIsLeaf_LRZ(n,left,right,nil) (CheckNil(nil,n->left) && CheckNil(nil,n->right))
#define BTSwap_PLRSH(a,b,parent,left,right,size,height) \
do \
{ \
    if(!CheckNil(nil,a->parent)) \
    { \
        a->parent->left = a->parent->left == a ? b : a->parent->left; \
        a->parent->right = a->parent->right == a ? b : a->parent->right; \
    } \
    if(!CheckNil(nil,b->parent)) \
    { \
        b->parent->left = b->parent->left == b ? a : b->parent->left; \
        b->parent->right = b->parent->right == b ? a : b->parent->right; \
    } \
    Swap(typeof(a->parent),a->parent,b->parent); \
    Swap(typeof(a->left),a->left,b->left); \
    if(!CheckNil(nil, a->left)) \
    { \
        a->left->parent = a; \
    } \
    if(!CheckNil(nil, b->left)) \
    { \
        b->left->parent = b; \
    } \
    Swap(typeof(a->right),a->right,b->right); \
    if(!CheckNil(nil, a->right)) \
    { \
        a->right->parent = a; \
    } \
    if(!CheckNil(nil, b->right)) \
    { \
        b->right->parent = b; \
    } \
    Swap(typeof(a->size),a->size,b->size); \
    Swap(typeof(a->height),a->height,b->height); \
} while(0)
#define BTHeight_LRHZ(n,left,right,height,nil) (BTIsLeaf_LRZ(n,left,right,nil) ? 0 : 1 + Max((CheckNil(nil,n->left) ? 0 : (n)->left->height), (CheckNil(nil,n->right) ? 0 : n->right->height)))
#define BTSize_LRSZ(n,left,right,size,nil) (1 + (CheckNil(nil,n->left) ? 0 : n->left->size) + (CheckNil(nil,n->right) ? 0 : n->right->size))
#define BTSkew_LRHZ(n,left,right,height,nil) ((CheckNil(nil,n->right) ? 0 : n->right->height) - (CheckNil(nil,n->left) ? 0 : n->left->height))
#define BTFirst_LZ(r,ret,left,nil) \
do \
{ \
    for(ret = r; !CheckNil(nil,ret->left); ret = ret->left) {} \
} while(0)
#define BTLast_RZ(r,ret,right,nil) \
do \
{ \
    for(ret = r; !CheckNil(nil,ret->right); ret = ret->right) {} \
} while(0)
#define BTNext_PLRZ(n,ret,parent,left,right,nil) \
do \
{ \
    ret = nil; \
    typeof(ret) t; \
    if(CheckNil(nil,n->right)) \
    { \
        for(t = n; !CheckNil(nil,t->parent); t = t->parent) \
        { \
            if(t->parent->left == t) \
            { \
                ret = t->parent; \
                break; \
            } \
        } \
    } \
    else \
    { \
        BTFirst_LZ(n->right, ret, left, nil); \
    } \
} while(0)
#define BTPrev_PLRZ(n,ret,parent,left,right,nil) \
do \
{ \
    ret = nil; \
    typeof(ret) t; \
    if(CheckNil(nil,n->left)) \
    { \
        for(t = n; !CheckNil(nil,t->parent); t = t->parent) \
        { \
            if(t->parent->right == t) \
            { \
                ret = t->parent; \
                break; \
            } \
        } \
    } \
    else \
    { \
        BTLast_RZ(n->left, ret, right, nil); \
    } \
} while(0)
#define BTLeftRotate_PLRSHZ(n,parent,left,right,size,height,nil) \
do \
{ \
    n->right->parent = n->parent; \
    if(!CheckNil(nil,n->parent)) \
    { \
        n->parent->left = n->parent->left == n ? n->right : n->parent->left; \
        n->parent->right = n->parent->right == n ? n->right : n->parent->right; \
    } \
    n->parent = n->right; \
    n->right = n->parent->left; \
    if(!CheckNil(nil, n->right)) \
    { \
        n->right->parent = n; \
    } \
    n->parent->left = n; \
    n->height = BTHeight_LRHZ(n,left,right,height,nil); \
    n->size = BTSize_LRSZ(n,left,right,size,nil); \
    n->parent->height = BTHeight_LRHZ(n->parent,left,right,height,nil); \
    n->parent->size = BTSize_LRSZ(n->parent,left,right,size,nil); \
} while(0)
#define BTRightRotate_PLRSHZ(n,parent,left,right,size,height,nil) \
do \
{ \
    n->left->parent = n->parent; \
    if(!CheckNil(nil,n->parent)) \
    { \
        n->parent->left = n->parent->left == n ? n->left : n->parent->left; \
        n->parent->right = n->parent->right == n ? n->left : n->parent->right; \
    } \
    n->parent = n->left; \
    n->left = n->parent->right; \
    if(!CheckNil(nil, n->left)) \
    { \
        n->left->parent = n; \
    } \
    n->parent->right = n; \
    n->height = BTHeight_LRHZ(n,left,right,height,nil); \
    n->size = BTSize_LRSZ(n,left,right,size,nil); \
    n->parent->height = BTHeight_LRHZ(n->parent,left,right,height,nil); \
    n->parent->size = BTSize_LRSZ(n->parent,left,right,size,nil); \
} while(0)
#define BTBalance_PLRSHZ(root,n,parent,left,right,size,height,nil) \
do \
{ \
    typeof(n) t; \
    for(t = n; !CheckNil(nil,t); t = t->parent) \
    { \
        t->height = BTHeight_LRHZ(t,left,right,height,nil); \
        t->size = BTSize_LRSZ(t,left,right,size,nil); \
        S64 skew = BTSkew_LRHZ(t,left,right,height,nil); \
        if(skew > 1) \
        { \
            U64 sub_skew = BTSkew_LRHZ(t->right,left,right,height,nil); \
            if(sub_skew >= 0) \
            { \
                BTLeftRotate_PLRSHZ(t,parent,left,right,size,height,nil); \
            } \
            else \
            { \
                BTRightRotate_PLRSHZ(t->right,parent,left,right,size,height,nil); \
                BTLeftRotate_PLRSHZ(t,parent,left,right,size,height,nil); \
            } \
            t = t->parent; \
        } \
        if(skew < -1) \
        { \
            U64 sub_skew = BTSkew_LRHZ(t->left,left,right,height,nil); \
            if(sub_skew <= 0) \
            { \
                BTRightRotate_PLRSHZ(t,parent,left,right,size,height,nil); \
            } \
            else \
            { \
                BTLeftRotate_PLRSHZ(t->left,parent,left,right,size,height,nil); \
                BTRightRotate_PLRSHZ(t,parent,left,right,size,height,nil); \
            } \
            t = t->parent; \
        } \
        root = CheckNil(nil,t->parent) ? t : root; \
    } \
} while(0)
#define BTInsertAfter_PLRSHZ(root,prev,n,parent,left,right,size,height,nil) \
do \
{ \
    if(CheckNil(nil,root)) \
    { \
        root = n; \
        BTBalance_PLRSHZ(root,root,parent,left,right,size,height,nil); \
    } \
    else \
    { \
        if(CheckNil(nil,prev)) \
        { \
            BTLast_RZ(root,prev,right,nil); \
        } \
        if(CheckNil(nil,prev->right)) \
        { \
            prev->right = n; \
            n->parent = prev; \
            BTBalance_PLRSHZ(root,n,parent,left,right,size,height,nil); \
        } \
        else \
        { \
            typeof(n) b; \
            BTFirst_LZ(prev->right, b, left, nil); \
            b->left = n; \
            n->parent = b; \
            BTBalance_PLRSHZ(root,n,parent,left,right,size,height,nil); \
        } \
    } \
} while(0)
#define BTInsertAfter(root,prev,n) BTInsertAfter_PLRSHZ(root,prev,n,parent,left,right,size,height,nil)
#define BTInsertBefore_PLRSHZ(root,next,n,parent,left,right,size,height,nil) \
do \
{ \
    if(CheckNil(nil,root)) \
    { \
        root = n; \
        BTBalance_PLRSHZ(root,root,parent,left,right,size,height,nil); \
    } \
    else \
    { \
        if(CheckNil(nil,next)) \
        { \
            BTFirst_LZ(root,next,left,nil); \
        } \
        if(CheckNil(nil,next->left)) \
        { \
            next->left = n; \
            n->parent = next; \
            BTBalance_PLRSHZ(root,n,parent,left,right,size,height,nil); \
        } \
        else \
        { \
            typeof(n) b; \
            BTLast_RZ(next->left, b, right, nil); \
            b->right = n; \
            n->parent = b; \
            BTBalance_PLRSHZ(root,n,parent,left,right,size,height,nil); \
        } \
    } \
} while(0)
#define BTInsertBefore(root,next,n) BTInsertBefore_PLRSHZ(root,next,n,parent,left,right,size,height,nil)
#define BTDelete_PLRSHZ(root,n,parent,left,right,size,height,nil) \
do \
{ \
    while(!BTIsLeaf_LRZ(n,left,right,nil)) \
    { \
        if(!CheckNil(nil,n->left)) \
        { \
            typeof(n) prev; \
            BTPrev_PLRZ(n,prev,parent,left,right,nil); \
            BTSwap_PLRSH(n,prev,parent,left,right,size,height); \
        } \
        else \
        { \
            typeof(n) next; \
            BTNext_PLRZ(n,next,parent,left,right,nil); \
            BTSwap_PLRSH(n,next,parent,left,right,size,height); \
        } \
    } \
    if(!CheckNil(nil,n->parent)) \
    { \
        n->parent->left = n->parent->left == n ? nil : n->parent->left; \
        n->parent->right = n->parent->right == n ? nil : n->parent->right; \
        BTBalance_PLRSHZ(root,parent,left,right,size,height,nil); \
    } \
    else \
    { \
        root = nil; \
    } \
} while(0)
#define BTPushback_PLRSHZ(root,n,parent,left,right,size,height,nil) \
do \
{ \
    typeof(root) prev = nil; \
    BTInsertAfter_PLRSHZ(root,prev,n,parent,left,right,size,height,nil); \
} while(0)
#define BTSetFindPrev_PLRKZ(root,key_v,ret,parent,left,right,key,nil) \
do \
{ \
    typeof(root) t = root; \
    ret = nil; \
    while(1) \
    {\
        if(key_v <= t->key) \
        { \
            if(!CheckNil(nil, t->left)) \
            { \
                t = t->left; \
                continue; \
            } \
            else \
            { \
                break; \
            } \
        } \
        else \
        { \
            if(!CheckNil(nil, t->right)) \
            { \
                ret = t; \
                t = t->right; \
                continue; \
            } \
            else \
            { \
                ret = t; \
                break; \
            } \
        } \
    }\
} while (0)
#define BTSetFindNext_PLRKZ(root,key_v,ret,parent,left,right,key,nil) \
do \
{ \
    typeof(root) t = root; \
    ret = nil; \
    while(1) \
    {\
        if(key_v >= t->key) \
        { \
            if(!CheckNil(nil, t->right)) \
            { \
                t = t->right; \
                continue; \
            } \
            else \
            { \
                break; \
            } \
        } \
        else \
        { \
            if(!CheckNil(nil, t->left)) \
            { \
                ret = t; \
                t = t->left; \
                continue; \
            } \
            else \
            { \
                ret = t; \
                break; \
            } \
        } \
    }\
} while (0)

/////////////////////////////////////////////////////////////////////////////////////////
// Quad Tree

typedef struct QuadTreeValue QuadTreeValue;
struct QuadTreeValue
{
  Rng2F32 rect;
  void *v;
};

// TODO(XXX): add this quadtree
// A function pointer defe for determining if an element exists in a given range 
// typedef B32 (*qtree_cmp)(Rng2F32 range, void *ptr);

// TODO(XXX): we could add multi-threading to this
typedef struct QuadTree QuadTree;
struct QuadTree
{
  Rng2F32 rect;
  // NW, NE, SW, SE
  Rng2F32 sub_rects[4];
  QuadTree *subs[4]; 
  QuadTreeValue **values;
};

internal QuadTree* quadtree_push(Arena *arena, Rng2F32 rect);
internal void      values_from_quadtree(Arena *arena, QuadTree *qt, void ***out);
internal void      quadtree_insert(Arena *arena, QuadTree *qt, Rng2F32 src_rect, void *value);

#endif
