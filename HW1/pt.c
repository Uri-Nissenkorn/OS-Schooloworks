//
// Created by Uri Nissenkorn on 09/11/2022.
// Based on the same assignment by me in the previous semester
//


#include "os.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define VALID_BIT 0x000000000001
#define VALID_RANGE 0x111111111111
#define NUM_OF_TABLES 5
#define TABLE_SIZE 9
#define OFFSET 12
#define first_nine_digits 511

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);
uint64_t page_table_query(uint64_t pt, uint64_t vpn);

// helper functions
uint64_t get_pte(uint64_t * pt, uint64_t vpn_left, int n);
void update_pte(uint64_t * pt, uint64_t vpn_left, uint64_t ppn, int n);
void delete_pte(uint64_t * pt, uint64_t vpn_left, int n);
uint64_t pteToPn(uint64_t pte);


// recurse into table and get final pte
uint64_t get_pte(uint64_t * pt, uint64_t vpn_left, int n) {
    uint64_t i =  (vpn_left >> (n-1)*TABLE_SIZE) & first_nine_digits;

    if (pt[i] ^ (((pt[i]>>OFFSET)<<OFFSET)+1)) { // if table is empty
        return 0;
    }

    if (n==1) {
        return pt[i];
    } else {
        return get_pte(
            phys_to_virt(pteToPn(pt[i] << OFFSET)), 
            vpn_left , 
            n-1);
    }

}



uint64_t page_table_query(uint64_t pt, uint64_t vpn){
    uint64_t pte = get_pte(
                        phys_to_virt(pt<<OFFSET),
                        vpn,
                        NUM_OF_TABLES);
   
    // check valid digit
    if (pte & 1) {
        return  pteToPn(pte);
    }
    return NO_MAPPING;
}


void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    if (ppn == NO_MAPPING){
        delete_pte(phys_to_virt(pt<<OFFSET),
                    vpn,
                    NUM_OF_TABLES);
    } else {
        update_pte(phys_to_virt(pt<<OFFSET),
                    vpn,
                    ppn,
                    NUM_OF_TABLES);
    }
}

// recursively assign frames and update table
void update_pte(uint64_t * pt, uint64_t vpn_left, uint64_t ppn, int n) {
    uint64_t i =  (vpn_left >> (n-1)*TABLE_SIZE) & first_nine_digits;

    if (n==1) {
        pt[i] = (ppn << OFFSET) + 1; // add valid
        return;

    } else {
        if (pt[i] ^ (((pt[i]>>OFFSET)<<OFFSET) +VALID_BIT)) {
            pt[i] = (alloc_page_frame() << OFFSET) + 1;
        }

        update_pte(
                phys_to_virt(pteToPn(pt[i] << OFFSET)),
                vpn_left ,
                ppn,
                n-1);
    }
}

// recursively delete
void delete_pte(uint64_t * pt, uint64_t vpn_left, int n) {
    uint64_t i =  (vpn_left >> (n-1)*TABLE_SIZE) & first_nine_digits;
 
    if (n==1) {
        pt[i] = 0; // add valid
        return;

    } else {
        if (pt[i] ^ (((pt[i]>>OFFSET)<<OFFSET) +VALID_BIT)) {
            return;
        }

        delete_pte(
                phys_to_virt(pteToPn(pt[i] << OFFSET)),
                vpn_left ,
                n-1);
    }
}

uint64_t pteToPn(uint64_t pte) {
    return ((pte - 1)>> OFFSET) ;
}