//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include <math.h>

//
// TODO:Student Information
//
const char *studentName = "Zhuowen Zou";
const char *studentID   = "A14554030";
const char *email       = "zhz402@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

// Set: tags, lastest access (+ valid bits), access counter

cacheSet* icache; 
cacheSet* dcache;
cacheSet* l2cache; 



//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //

  icache = (cacheSet*)malloc(icacheSets * sizeof(cacheSet));
  for (int i = 0; i < icacheSets; i++) {

	  icache[i].counter = 0;

	  icache[i].tags = (uint32_t*)malloc(icacheAssoc * sizeof(uint32_t));
	  icache[i].accs = (uint32_t*)malloc(icacheAssoc * sizeof(uint32_t));
	  for (int j = 0; j < icacheAssoc; j++) {
		  icache[i].tags[j] = 0;
		  icache[i].accs[j] = 0; // acc = 0 means invalid by default
	  }

  }

  dcache = (cacheSet*)malloc(dcacheSets * sizeof(cacheSet));
  for (int i = 0; i < dcacheSets; i++) {

	  dcache[i].counter = 0;

	  dcache[i].tags = (uint32_t*)malloc(dcacheAssoc * sizeof(uint32_t));
	  dcache[i].accs = (uint32_t*)malloc(dcacheAssoc * sizeof(uint32_t));
	  for (int j = 0; j < dcacheAssoc; j++) {
		  dcache[i].tags[j] = 0;
		  dcache[i].accs[j] = 0; // acc = 0 means invalid by default
	  }
  }

  l2cache = (cacheSet*)malloc(l2cacheSets * sizeof(cacheSet));
  for (int i = 0; i < l2cacheSets; i++) {
	  
	  l2cache[i].counter = 0;

	  l2cache[i].tags = (uint32_t*)malloc(l2cacheAssoc * sizeof(uint32_t));
	  l2cache[i].accs = (uint32_t*)malloc(l2cacheAssoc * sizeof(uint32_t));
	  for (int j = 0; j < l2cacheAssoc; j++) {
		  l2cache[i].tags[j] = 0;
		  l2cache[i].accs[j] = 0; // acc = 0 means invalid by default
	  }

  }
  
}


// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
	
	addrParsed ap = iParseAddr(addr);

	icacheRefs += 1;       // I$ references

	icache[ap.index].counter += 1;

	int loc = findTag(ap.tag, &icache[ap.index], icacheAssoc);

	// addr is in the cache
	if ( loc != -1) {
		// update latest accs
		icache[ap.index].accs[loc] = icache[ap.index].counter;

		//l2cache_update(addr);
		return icacheHitTime;
	}
	else {
	
		icacheMisses += 1;   // missed

		// inclusive or exclusive l2, addr always goes in l1;
		int loc = findEntry(&icache[ap.index], icacheAssoc); 

		// update cache
		icache[ap.index].tags[loc] = ap.tag;
		icache[ap.index].accs[loc] = icache[ap.index].counter;

		// ACCESS time 
		uint32_t penalty = l2cache_access(addr, 0);
		icachePenalties += penalty; // 
		return penalty;
	}

}

// function for evicting a line due to inclusion policy
void
icache_evict(uint32_t addr)
{

	addrParsed ap = iParseAddr(addr);
	int loc = findTag(ap.tag, &icache[ap.index], icacheAssoc);

	// addr is in the cache
	if (loc != -1) {
		// update latest accs
		icache[ap.index].accs[loc] = 0; // invalidate cache line
	}
}


// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{

	addrParsed ap = dParseAddr(addr);

	dcacheRefs += 1;       // I$ references

	dcache[ap.index].counter += 1;

	int loc = findTag(ap.tag, &dcache[ap.index], dcacheAssoc);

	// addr is in the cache
	if (loc != -1) {
		// update latest accs
		dcache[ap.index].accs[loc] = dcache[ap.index].counter;

		//l2cache_update(addr);
		return dcacheHitTime;
	}
	else {

		dcacheMisses += 1;       // missed

		// inclusive or exclusive l2, addr always goes in l1;
		int loc = findEntry(&dcache[ap.index], dcacheAssoc);

		// update cache
		dcache[ap.index].tags[loc] = ap.tag;
		dcache[ap.index].accs[loc] = dcache[ap.index].counter;

		// ACCESS time 
		uint32_t penalty = l2cache_access(addr, 1);

		dcachePenalties += penalty; // +l2cacheHitTime;

		return penalty;
	}

}


// function for evicting a line due to inclusion policy
void
dcache_evict(uint32_t addr)
{

	addrParsed ap = dParseAddr(addr);
	int loc = findTag(ap.tag, &dcache[ap.index], dcacheAssoc);

	// addr is in the cache
	if (loc != -1) {
		// update latest accs
		dcache[ap.index].accs[loc] = 0; // invalidate cache line
	}
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr, uint8_t from)
{

	addrParsed ap = l2ParseAddr(addr);

	l2cacheRefs += 1;       // I$ references
	l2cache[ap.index].counter += 1;

	int loc = findTag(ap.tag, &(l2cache[ap.index]), l2cacheAssoc);

	// addr is in the cache
	if (loc != -1) {
		// update latest accs
		l2cache[ap.index].accs[loc] = l2cache[ap.index].counter;
		return l2cacheHitTime;
	}
	else {

		l2cacheMisses += 1;       // missed

		// inclusive or exclusive l2, addr always goes in l1;
		addrParsed ap = l2ParseAddr(addr);
		int loc = findEntry(&l2cache[ap.index], l2cacheAssoc);

		// handle inclusion policy: 
		if (inclusive) {
			if (l2cache[ap.index].accs[loc] != 0) {
				dcache_evict(addr);
				icache_evict(addr);
			}
		}
		
		// update cache
		l2cache[ap.index].tags[loc] = ap.tag;
		l2cache[ap.index].accs[loc] = l2cache[ap.index].counter;

		// ACCESS time 
		uint32_t penalty = memspeed;
		l2cachePenalties += penalty; // 
		return penalty + l2cacheHitTime;
	}
}

// update least recently used address
void
l2cache_update(uint32_t addr) {

	addrParsed ap = l2ParseAddr(addr);
	int loc = findTag(ap.tag, &l2cache[ap.index], l2cacheAssoc);

	// addr is in the cache
	if (loc != -1) {
		// update latest accs
		l2cache[ap.index].counter += 1;
		l2cache[ap.index].accs[loc] = l2cache[ap.index].counter;
	}
}


// Find index of tag if exist; return -1 if not
int
findTag(uint32_t tag, cacheSet* cs, int len)
{
	for (int i = 0; i < len; i++) {
		if ((cs->accs[i] != 0) && (tag == cs->tags[i])) {
			return i;
		}
	}
	return -1;
}

// Find the location for new cache
int
findEntry(cacheSet* cs, int len)
{
	int loc = 0;
	for (int i = 0; i < len; i++) {
		// acc == 0 means available
		if (cs->accs[i] == 0) {
			return i;
		}
		// prefer the location with ealiest access
		else if (cs->accs[i] < cs->accs[loc]) {
			loc = i;
		}
	}
	return loc;
}

addrParsed
iParseAddr(uint32_t addr)
{
	addrParsed temp;
	temp.offset = addr & (blocksize - 1);
	temp.index = addr / blocksize;
	temp.index = temp.index & (icacheSets - 1);
	temp.tag = addr / (blocksize * icacheSets);
	return temp;
}

addrParsed
dParseAddr(uint32_t addr)
{
	addrParsed temp;
	temp.offset = addr & (blocksize - 1);
	temp.index = addr / blocksize;
	temp.index = temp.index & (dcacheSets - 1);
	temp.tag = addr / (blocksize * dcacheSets);
	return temp;
}

addrParsed
l2ParseAddr(uint32_t addr)
{
	addrParsed temp;
	temp.offset = addr & (blocksize - 1);
	temp.index = addr / blocksize;
	temp.index = temp.index & (l2cacheSets - 1);
	temp.tag = addr / (blocksize * l2cacheSets);
	return temp;
}