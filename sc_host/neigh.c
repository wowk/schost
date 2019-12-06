#include <util.h>
#include <neigh.h>

struct neigh_elem_t {
    struct neigh_t neigh;
    LIST_ENTRY(neigh_elem_t) entry;
};

struct neigh_list_t {
	struct neigh_elem_t *lh_first;
};

struct neigh_list_t neigh_list;

void neigh_list_visit(int(*callback)(struct neigh_t*, void*), void* arg)
{
    struct neigh_elem_t* elem;

    LIST_FOREACH(elem, &neigh_list, entry) {
        if(callback(&elem->neigh, arg)){
            break;
        }
    }
}

struct visit_arg_t {
    void* arg;
    void* retval;
};

static int compare_neigh_by_addr(struct neigh_t* neigh, void* arg)
{
    struct visit_arg_t* va = (struct visit_arg_t*)arg;

    if(!memcmp(&neigh->address, va->arg, 6)){
        va->retval = &neigh; 
        return 1;
    }

    return 0;
}

struct neigh_t* neigh_list_find(bd_addr addr)
{
    struct visit_arg_t va = {
        .arg    = &addr,
        .retval = NULL,
    };

    neigh_list_visit(compare_neigh_by_addr, &va);

    return va.retval;
}

void neigh_list_add(int phy, struct gecko_msg_le_gap_scan_response_evt_t* rsp)
{
    struct neigh_t* neigh = NULL;
    struct neigh_elem_t* neigh_elem = NULL;
    
    neigh = neigh_list_find(rsp->address); 
    if(!neigh) {
        neigh_elem = (struct neigh_elem_t*)Calloc(1, sizeof(struct neigh_elem_t));
        if(!neigh_elem){
            return;
        }
        neigh = &neigh_elem->neigh;
        memcpy(&neigh->address, &rsp->address, sizeof(bd_addr));
        neigh->addrtype = rsp->address_type;
        LIST_INSERT_HEAD(&neigh_list, neigh_elem, entry);
    }
    
    if(phy == 1){
        neigh->phy_1 = 1;
    }else if(phy == 4){
        neigh->phy_4 = 1;
    }
}

void neigh_list_clear(void)
{
    struct neigh_elem_t* p;
    while((p = LIST_FIRST(&neigh_list))){
        LIST_REMOVE(p, entry);
        free(p);
    }
}


