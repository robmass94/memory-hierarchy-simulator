/*

Author: Robert Massicotte
Course: CDA5155
Assignment: Memory Hierarchy Simulator
Compilation: g++ -g -std=c++11 hierarchy.cpp -o hierarchy.exe

*/

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace std;

bool isPowerOfTwo(int);
void getConfig(string, map<string, int>&);
void printConfig(map<string, int>&);
unsigned int createMask(int, int);

class CacheEntry
{
public:
	CacheEntry()
  	{
  		tag = UINT_MAX;
  		valid_bit = 0;
  		dirty_bit = 0;
  		phys_page_num = UINT_MAX;
  	}
  	void setTag(unsigned int t)
  	{
  		tag = t;
  	}
  	void setValidBit(unsigned int v)
  	{
  		valid_bit = v;
  	}
	void setDirtyBit(unsigned int d)
  	{
  		dirty_bit = d;
  	}
  	void setPhysPageNum(unsigned int p)
  	{
  		phys_page_num = p;
  	}
  	unsigned int getTag()
  	{
  		return tag;
  	}
  	unsigned int getValidBit()
  	{
  		return valid_bit;
  	}
  	unsigned int getDirtyBit()
  	{
  		return dirty_bit;
  	}
  	unsigned int getPhysPageNum()
  	{
  		return phys_page_num;
  	}
private:
	unsigned int tag;
  	unsigned int valid_bit;
  	unsigned int dirty_bit;
	// page number of address from which index and tag bits were determined
  	unsigned int phys_page_num;
};

class CacheSet
{
public:
	CacheSet(int n, int sn)
  	{
    	num_entries = n;
    	set_num = sn;
    	entries = new deque<CacheEntry*>;
    	for (int i = 0; i < num_entries; ++i) {
    		entries->push_back(new CacheEntry());
    	}
  	}
  	~CacheSet()
  	{
  		for (int i = 0; i < num_entries; ++i) {
  			delete entries->at(i);
  		}
  		delete entries;
  	}
  	bool readEntry(unsigned int tag)
  	{
  		CacheEntry *current;
  		for (int i = 0; i < num_entries; ++i) {
  			current = entries->at(i);
  			if ((current->getValidBit() == 1) && (current->getTag() == tag)) {
  				entries->erase(entries->begin() + i);
  				entries->push_back(current);
  				return true;
  			}
  		}
  		return false;
  	}
  	void addEntry(unsigned int tag, unsigned int phys_page_num, unsigned int dirty)
  	{
  		CacheEntry *lru = entries->front();
  		entries->pop_front();
  		lru->setTag(tag);
  		lru->setValidBit(1);
  		lru->setDirtyBit(dirty);
  		lru->setPhysPageNum(phys_page_num);
  		entries->push_back(lru);
  	}
  	void updateDirtyEntry(unsigned int tag)
  	{
  		CacheEntry *current;
   		for (int i = 0; i < num_entries; ++i) {
  			current = entries->at(i);
  			if (current->getTag() == tag) {
				current->setDirtyBit(1);
  			}
  		} 		
  	}
  	int invalidateEntries(unsigned int phys_page_num)
  	{
  		int dirty_count = 0;
  		CacheEntry *current;
  		for (int i = 0; i < num_entries; ++i) {
  			current = entries->at(i);
  			if (current->getPhysPageNum() == phys_page_num) {
  				//cout << "Invalidating cache line " << i << " of set " << set_num << " with tag " << entries->at(i)->getTag() << " since phys page " << phys_page_num << " is being replaced" << endl;
  				current->setValidBit(0);
  				if (current->getDirtyBit() == 1) {
  					current->setDirtyBit(0);
  					++dirty_count;
  				}
  			}
  		}
  		return dirty_count; // return number of invalidated dirty cache entries (need to write back to memory if write-back policy)
  	}
  	bool isLRUEntryDirty()
  	{
  		return entries->front()->getDirtyBit();
  	}
private:
	int num_entries;
	int set_num;
	deque<CacheEntry*> *entries;
};

class Cache
{
public:
  	Cache(int s, int ss, string t)
  	{
    	num_sets = s;
    	set_size = ss;
    	type = t;
    	sets = new vector<CacheSet*>;
    	for (int i = 0; i < num_sets; ++i) {
      		sets->push_back(new CacheSet(set_size, i));
      	}
  	}
	~Cache() 
	{
		for (int i = 0; i < num_sets; ++i) {
			delete sets->at(i);
		}
		delete sets;
	}
  	bool readEntry(unsigned int index, unsigned int tag)
  	{
  		return sets->at(index)->readEntry(tag);
  	}
  	void addEntry(unsigned int index, unsigned int tag, unsigned int phys_page_num, unsigned int dirty)
  	{
  		//cout << "Updating cache entry " << index << " with tag " << tag << " and page " << phys_page_num << endl;
  		sets->at(index)->addEntry(tag, phys_page_num, dirty);
  	}
  	void updateDirtyEntry(unsigned int index, unsigned int tag)
  	{
  		sets->at(index)->updateDirtyEntry(tag);
  	}
  	int invalidateEntries(unsigned int phys_page_num)
  	{
  		//cout << "Searching sets for physical page " << phys_page_num << " to invalidate" << endl;
  		//cout << "Invalidating " << type << " cache entries ..." << endl;
  		int dirty_count = 0;
  		for (int i = 0; i < num_sets; ++i) {
  			dirty_count += sets->at(i)->invalidateEntries(phys_page_num);
  		}
  		return dirty_count; // return number of invalidated dirty cache entries (need to write back to memory if write-back policy)
  	}
  	bool isLRUEntryDirty(unsigned int index)
  	{
  		// returns true if LRU entry of cache set has dirty bit set
  		return sets->at(index)->isLRUEntryDirty();
  	}
private:
	vector<CacheSet*> *sets;
  	int num_sets;
  	int set_size;
  	string type;
};

class PageTableEntry
{
public:
	PageTableEntry()
	{
		phys_page_num = UINT_MAX;
		resident_bit = 0;
		dirty_bit = 0;
	}
  	void setPhysPageNum(unsigned int p)
  	{
  		phys_page_num = p;
  	}
  	void setResidentBit(unsigned int r)
  	{
  		resident_bit = r;
  	}
	void setDirtyBit(unsigned int d)
  	{
  		dirty_bit = d;
  	}
  	unsigned int getPhysPageNum()
  	{
  		return phys_page_num;
  	}
  	unsigned int getResidentBit()
  	{
  		return resident_bit;
  	}
  	unsigned int getDirtyBit()
  	{
  		return dirty_bit;
  	}
private:
	unsigned int phys_page_num;
	unsigned int resident_bit;
	unsigned int dirty_bit;
};

class PageTable
{
public:
	PageTable(int n)
	{
		num_entries = n;
		entries = new vector<PageTableEntry*>;
		for (int i = 0; i < num_entries; ++i) {
			entries->push_back(new PageTableEntry());
		}
	}
	~PageTable()
	{
		for (int i = 0; i < num_entries; ++i) {
			delete entries->at(i);
		}
		delete entries;
	}
	unsigned int readEntry(unsigned int index)
	{
		PageTableEntry *e = entries->at(index);
		if (e->getResidentBit() == 1) {
			return e->getPhysPageNum();
		}
		return UINT_MAX;
	}
	void addEntry(unsigned int index, unsigned int phys_page_num)
	{
		entries->at(index)->setPhysPageNum(phys_page_num);
		entries->at(index)->setResidentBit(1);
	}
	void invalidateEntries(unsigned int phys_page_num)
	{
		PageTableEntry *current;
		for (int i = 0; i < num_entries; ++i) {
			current = entries->at(i);
			if (current->getPhysPageNum() == phys_page_num) {
				current->setResidentBit(0);
				current->setDirtyBit(0);
			}
		}
	}
	void setPageDirtyBit(unsigned int phys_page_num)
	{
		PageTableEntry *current;
		for (int i = 0; i < num_entries; ++i) {
			current = entries->at(i);
			if (current->getPhysPageNum() == phys_page_num) {
				current->setDirtyBit(1);
			}
		}
	}
private:
	int num_entries;
	vector<PageTableEntry*> *entries;
};

class TLBEntry
{
public:
	TLBEntry()
	{
		tag = UINT_MAX;
		phys_page_num = UINT_MAX;
		valid_bit = 0;
	}
	void setTag(unsigned int t)
	{
		tag = t;
	}
	void setPhysPageNum(unsigned int p)
	{
		phys_page_num = p;
	}
	void setValidBit(unsigned int v)
	{
		valid_bit = v;
	}
	unsigned int getTag()
	{
		return tag;
	}
	unsigned int getPhysPageNum()
	{
		return phys_page_num;
	}
	unsigned int getValidBit()
	{
		return valid_bit;
	}
private:
	unsigned int phys_page_num;
	unsigned int tag;
	unsigned int valid_bit;
};

class TLBSet
{
public:
	TLBSet(int n, int sn)
	{
		num_entries = n;
		set_num = sn;
		entries = new deque<TLBEntry*>;
		for (int i = 0; i < num_entries; ++i) {
			entries->push_back(new TLBEntry());
		}
	}
	~TLBSet()
	{
		for (int i = 0; i < num_entries; ++i) {
			delete entries->at(i);
		}
		delete entries;
	}
	unsigned int readEntry(unsigned int tag)
	{
		TLBEntry *current;
		for (int i = 0; i < num_entries; ++i) {
			current = entries->at(i);
			if ((current->getTag() == tag) && (current->getValidBit() == 1)) {
				entries->erase(entries->begin() + i);
				entries->push_back(current);
				return current->getPhysPageNum();
			}
		}
		return UINT_MAX;
	}
	void addEntry(unsigned int tag, unsigned int phys_page_num)
	{
		TLBEntry *lru = entries->front();
		entries->pop_front();
		lru->setPhysPageNum(phys_page_num);
		lru->setTag(tag);
		lru->setValidBit(1);
		entries->push_back(lru);
	}
	void invalidateEntries(int phys_page_num)
	{
		TLBEntry *current;
		for (int i = 0; i < num_entries; ++i) {
			current = entries->at(i);
			if (current->getPhysPageNum() == phys_page_num) {
		  		//cout << "Invalidating TLB entry " << i << " of set " << set_num << " since phys page " << phys_page_num << " is being replaced" << endl;
				current->setValidBit(0);
			}
		}
	}
private:
	int num_entries;
	int set_num;
	deque<TLBEntry*> *entries;
};

class TLB
{
public:
	TLB(int s, int ss, string t)
	{
		num_sets = s;
		set_size = ss;
		type = t;
		sets = new vector<TLBSet*>;
		for (int i = 0; i < num_sets; ++i) {
			sets->push_back(new TLBSet(set_size, i));
		}
	}
	~TLB()
	{
		for (int i = 0; i < num_sets; ++i) {
			delete sets->at(i);
		}
		delete sets;
	}
	unsigned int readEntry(unsigned int index, unsigned int tag)
	{
		return sets->at(index)->readEntry(tag);
	}
	void addEntry(unsigned int index, unsigned int tag, unsigned int phys_page_num)
	{
		sets->at(index)->addEntry(tag, phys_page_num);
	}
	void invalidateEntries(unsigned int phys_page_num)
	{
		//cout << "Invalidating " << type << " TLB entries ..." << endl; 
		for (int i = 0; i < num_sets; ++i) {
			sets->at(i)->invalidateEntries(phys_page_num);
		}
	}
private:
	int num_sets;
	int set_size;
	vector<TLBSet*> *sets;
	string type;
};

class PhysicalPage
{
public:
	PhysicalPage(unsigned int n, bool rb)
	{
		page_number = n;
		modified = false;
		referenced_before = rb;
	}
	void setModified(bool m)
	{
		modified = m;
	}
	void setReferencedBefore(bool rb)
	{
		referenced_before = rb;
	}
	unsigned int getPageNum()
	{
		return page_number;
	}
	bool wasModified()
	{
		return modified;
	}
	bool wasReferencedBefore()
	{
		return referenced_before;
	}
private:
	unsigned int page_number;
	bool modified;
	bool referenced_before;
};

int main(int argc, char **argv)
{
	map<string, int> config;
	string file_str;
	istringstream iss;
	char stream_type;
	char access_type;
	string str_hex_address;

	Cache *instruction_cache;
	Cache *data_cache;
	PageTable *page_table;
	TLB *instruction_tlb;
	TLB *data_tlb;
	deque<PhysicalPage*> *physical_pages;

	unsigned int hex_address;
	unsigned int orig_hex_address;
	int hex_address_size;
	unsigned int virtual_page_num;
	unsigned int page_offset;
	string ref_type;
	unsigned int tlb_tag;
	unsigned int tlb_index;
	string tlb_ref;
	string pt_ref;
	unsigned int physical_page_num;
	unsigned int cache_tag;
	unsigned int cache_index;
	string cache_ref;
	bool result;

	int itlb_hits = 0;
	int itlb_misses = 0;
	int dtlb_hits = 0;
	int dtlb_misses = 0;
	int pt_hits = 0;
	int pt_faults = 0;
	int ic_hits = 0;
	int ic_misses = 0;
	int dc_hits = 0;
	int dc_misses = 0;
	int reads = 0;
	int writes = 0;
	int inst_refs = 0;
	int data_refs = 0;
	int memory_refs = 0;
	int disk_refs = 0;

	bool need_to_visit_pt;
	bool referenced_before;
	bool is_dirty;
	int invalidated_dirty_count;

	getConfig("trace.config", config);
	printConfig(config);

	if (config["tlbs_enabled"] && !config["virtual_addresses_enabled"]) {
		fprintf(stderr, "hierarchy: TLBs cannot be enabled when virtual addresses are disabled\n");
		exit(EXIT_FAILURE);
	}

	printf("\n");

	instruction_cache = new Cache(config["instruction_cache_sets"], config["instruction_cache_set_size"], "instruction");
	data_cache = new Cache(config["data_cache_sets"], config["data_cache_set_size"], "data");
	page_table = new PageTable(config["virtual_pages"]);
	instruction_tlb = new TLB(config["instruction_tlb_sets"], config["instruction_tlb_set_size"], "instruction");
	data_tlb = new TLB(config["data_tlb_sets"], config["data_tlb_set_size"], "data");
	physical_pages = new deque<PhysicalPage*>;

	for (int i = 0; i < config["physical_pages"]; ++i)
	{
		physical_pages->push_back(new PhysicalPage(i, false));
	}

	if (config["virtual_addresses_enabled"]) {
		printf("%-8s %-7s %-6s %-4s %-7s %-5s %-4s %-4s %-6s %-7s %-5s %-4s\n", "Virtual", "Virtual", "Page", "Ref", "TLB", "TLB", "TLB", "PT", "Phys", "Cache", "Cache", "Cache");
	} else {
		printf("%-8s %-7s %-6s %-4s %-7s %-5s %-4s %-4s %-6s %-7s %-5s %-4s\n", "Physical", "Virtual", "Page", "Ref", "TLB", "TLB", "TLB", "PT", "Phys", "Cache", "Cache", "Cache");
	}
	printf("%-8s %-7s %-6s %-4s %-7s %-5s %-4s %-4s %-6s %-7s %-5s %-4s\n", "Address", "Page #", "Offset", "Type", "Tag", "Index", "Ref", "Ref", "Page #", "Tag", "Index", "Ref");
	printf("%-8s %-7s %-6s %-4s %-7s %-5s %-4s %-4s %-6s %-7s %-5s %-4s\n", "--------", "-------", "------", "----", "-------", "-----", "----", "----", "------", "-------", "-----", "-----");

	while (getline(cin, file_str)) {
		iss.str(file_str);
		iss >> stream_type;
		iss.ignore();
		iss >> access_type;
		if (access_type == 'W') {
			++writes;
			if (stream_type == 'I') {
			  fprintf(stderr, "hierarchy: write to an instruction in reference\n");
			  exit(EXIT_FAILURE);
			}
		} else {
			++reads;
		}
		iss.ignore();
		iss >> str_hex_address;
		hex_address_size = str_hex_address.length() * 4;
		hex_address = stoi(str_hex_address, nullptr, 16);
		orig_hex_address = hex_address;
		iss.str(string());
		iss.clear();

		page_offset = hex_address & createMask(0, config["page_offset_bits"] - 1);
		if (config["virtual_addresses_enabled"]) {
			virtual_page_num = (hex_address & createMask(config["page_offset_bits"], config["page_offset_bits"] + config["page_index_bits"] - 1)) >> config["page_offset_bits"];
			if (virtual_page_num >= config["virtual_pages"]) { // Virtual pages are 0 ... n-1, so virtual page number cannot be >= n
				fprintf(stderr, "hierarchy: address %x is too large\n", hex_address);
				exit(EXIT_FAILURE);
			}
		} else {
			physical_page_num = (hex_address & createMask(config["page_offset_bits"], hex_address_size - 1)) >> config["page_offset_bits"];
			if (physical_page_num >= config["physical_pages"]) { // Physical pages are 0 ... n-1, so physical page number cannot be >= n
				fprintf(stderr, "hierarchy: address %x is too large\n", hex_address);
				exit(EXIT_FAILURE);
			}
		}
		
		if (stream_type == 'I') {
			ref_type = "inst";
			++inst_refs;
			if (config["virtual_addresses_enabled"]) {
				if (config["tlbs_enabled"]) {
					tlb_index = (hex_address & createMask(config["page_offset_bits"], config["page_offset_bits"] + config["instruction_tlb_index_bits"] - 1)) >> config["page_offset_bits"];
					if (tlb_index >= config["instruction_tlb_sets"]) {
						fprintf(stderr, "hierarchy: address %x is too large\n", hex_address);
						exit(EXIT_FAILURE);
					}
					tlb_tag = (hex_address & createMask(config["page_offset_bits"] + config["instruction_tlb_index_bits"], hex_address_size)) >> (config["page_offset_bits"] + config["instruction_tlb_index_bits"]);
					physical_page_num = instruction_tlb->readEntry(tlb_index, tlb_tag);
					if (physical_page_num < UINT_MAX) { // ITLB hit
						int i;
						tlb_ref = "hit";
						++itlb_hits;
						pt_ref = "none";
						need_to_visit_pt = false;
						for (i = 0; i < physical_pages->size(); ++i) {
							if (physical_pages->at(i)->getPageNum() == physical_page_num) {
								break;
							}
						}
						// move page frame to end of queue
						PhysicalPage *p = physical_pages->at(i);
						p->setReferencedBefore(true);
						physical_pages->push_back(p);
						physical_pages->erase(physical_pages->begin() + i);
					} else { // ITLB miss, need to go to page table
						tlb_ref = "miss";
						++itlb_misses;
						need_to_visit_pt = true;
					}
				} else { // TLBs disabled
					need_to_visit_pt = true;
				}

				if (need_to_visit_pt) {
					++memory_refs;
					physical_page_num = page_table->readEntry(virtual_page_num);
					if (physical_page_num < UINT_MAX) { // Page table hit
						int i;
						pt_ref = "hit";
						++pt_hits;
						for (i = 0; i < physical_pages->size(); ++i) {
							if (physical_pages->at(i)->getPageNum() == physical_page_num) {
								break;
							}
						}
						// move page frame to end of queue
						PhysicalPage *p = physical_pages->at(i);
						p->setReferencedBefore(true);
						physical_pages->push_back(p);
						physical_pages->erase(physical_pages->begin() + i);
					} else { // Page table fault (miss), go to disk, bring page into LRU frame
						pt_ref = "miss";
						++pt_faults;
						++disk_refs;
						PhysicalPage *p = physical_pages->front();
						physical_page_num = p->getPageNum();
						referenced_before = p->wasReferencedBefore();
						is_dirty = p->wasModified();
						physical_pages->pop_front();
						p->setReferencedBefore(true);
						p->setModified(false);
						physical_pages->push_back(p); // move page frame to end of queue
						if (referenced_before) { 
							// Page is being replaced, invalidate corresponding cache, TLB, and page table entries
							//cout << "Physical page " << physical_page_num << " was used previously, need to invalidate" << endl;
							if (is_dirty) { // if replaced page is dirty, need to write back to disk
								//cout << "Writing dirty page to disk" << endl;
								++disk_refs; 
							}
							page_table->invalidateEntries(physical_page_num);
							invalidated_dirty_count = data_cache->invalidateEntries(physical_page_num);
							if (!config["data_cache_write_through"]) { // need to write back invalidated data cache entries if write-back policy
								memory_refs += invalidated_dirty_count;
							}
							instruction_cache->invalidateEntries(physical_page_num);
							if (config["tlbs_enabled"]) {
								data_tlb->invalidateEntries(physical_page_num);
								instruction_tlb->invalidateEntries(physical_page_num);
							}
						}
						page_table->addEntry(virtual_page_num, physical_page_num); // update page table
					}
					if (config["tlbs_enabled"]) {
						instruction_tlb->addEntry(tlb_index, tlb_tag, physical_page_num); // update instruction TLB
					}
				}
				// replace virtual page number with acquired physical page number
				// first clear virtual page number bits by creating a mask, negating it, and anding it with the hex address
				// then substitute the physical page number by shifting the value and oring it with the result of the and operation
				hex_address = (hex_address & ~(createMask(config["page_offset_bits"], config["page_offset_bits"] + config["page_index_bits"] - 1))) | (physical_page_num << config["page_offset_bits"]);
			}

			cache_index = (hex_address & createMask(config["instruction_cache_offset_bits"], config["instruction_cache_offset_bits"] + config["instruction_cache_index_bits"] - 1)) >> config["instruction_cache_offset_bits"];
			if (cache_index >= config["instruction_cache_sets"]) {
				fprintf(stderr, "hierarchy: address %x is too large\n", orig_hex_address);						
				exit(EXIT_FAILURE);				
			}		
			cache_tag = (hex_address & createMask(config["instruction_cache_offset_bits"] + config["instruction_cache_index_bits"], hex_address_size-1)) >> (config["instruction_cache_offset_bits"] + config["instruction_cache_index_bits"]);
			result = instruction_cache->readEntry(cache_index, cache_tag);
			if (result) {
				cache_ref = "hit";
				++ic_hits;
			} else {
				cache_ref = "miss";
				++ic_misses;
				// bring in from memory, update cache
				++memory_refs;
				instruction_cache->addEntry(cache_index, cache_tag, physical_page_num, 0);
			}
		} else {
			ref_type = "data";
			++data_refs;

			if (config["virtual_addresses_enabled"]) {
				if (config["tlbs_enabled"]) {
					tlb_index = (hex_address & createMask(config["page_offset_bits"], config["page_offset_bits"] + config["data_tlb_index_bits"] - 1)) >> config["page_offset_bits"];
					if (tlb_index >= config["data_tlb_sets"]) {
						fprintf(stderr, "hierarchy: address %x is too large\n", hex_address);						
						exit(EXIT_FAILURE);
					}
					tlb_tag = (hex_address & createMask(config["page_offset_bits"] + config["data_tlb_index_bits"], hex_address_size)) >> (config["page_offset_bits"] + config["data_tlb_index_bits"]);
					physical_page_num = data_tlb->readEntry(tlb_index, tlb_tag);
					if (physical_page_num < UINT_MAX) { // DTLB hit
						int i;
						tlb_ref = "hit";
						++dtlb_hits;
						pt_ref = "none";
						need_to_visit_pt = false;
						for (i = 0; i < physical_pages->size(); ++i) {
							if (physical_pages->at(i)->getPageNum() == physical_page_num) {
								break;
							}
						}
						// move page frame to end of queue
						PhysicalPage *p = physical_pages->at(i);
						p->setReferencedBefore(true);
						physical_pages->push_back(p);
						physical_pages->erase(physical_pages->begin() + i);
					} else { // DTLB miss, need to go to page table
						tlb_ref = "miss";
						++dtlb_misses;
						need_to_visit_pt = true;
					}
				} else { // TLBs disabled
					need_to_visit_pt = true;
				}

				if (need_to_visit_pt) {
					++memory_refs;
					physical_page_num = page_table->readEntry(virtual_page_num);
					if (physical_page_num < UINT_MAX) { // Page table hit
						int i;
						pt_ref = "hit";
						++pt_hits;	
						for (i = 0; i < physical_pages->size(); ++i) {
							if (physical_pages->at(i)->getPageNum() == physical_page_num) {
								break;
							}
						}
						// move page frame to end of queue
						PhysicalPage *p = physical_pages->at(i);
						p->setReferencedBefore(true);
						physical_pages->push_back(p);
						physical_pages->erase(physical_pages->begin() + i);
					} else { // Page table fault (miss), go to disk, bring page into LRU frame
						pt_ref = "miss";
						++pt_faults;
						++disk_refs;
						PhysicalPage *p = physical_pages->front();
						physical_page_num = p->getPageNum();
						referenced_before = p->wasReferencedBefore();
						is_dirty = p->wasModified();
						physical_pages->pop_front();
						p->setReferencedBefore(true);
						p->setModified(false);
						physical_pages->push_back(p);
						if (referenced_before) { 
							// Page is being replaced, invalidate corresponding cache, TLB, and page table entries
							//cout << "Physical page " << physical_page_num << " was used previously, need to invalidate" << endl;
							if (is_dirty) {
								//cout << "Writing dirty page to disk" << endl;
								++disk_refs; // if replaced page is dirty, need to write back to disk
							}
							page_table->invalidateEntries(physical_page_num);
							invalidated_dirty_count = data_cache->invalidateEntries(physical_page_num);
							if (!config["data_cache_write_through"]) { // need to write back invalidated data cache entries if write-back policy
								memory_refs += invalidated_dirty_count;
							}
							instruction_cache->invalidateEntries(physical_page_num);
							if (config["tlbs_enabled"]) {
								data_tlb->invalidateEntries(physical_page_num);
								instruction_tlb->invalidateEntries(physical_page_num);
							}
						}
						page_table->addEntry(virtual_page_num, physical_page_num); // update page table
					}
					if (config["tlbs_enabled"]) {
						data_tlb->addEntry(tlb_index, tlb_tag, physical_page_num); // update data TLB
					}
				}
				// replace virtual page number with acquired physical page number
				// first clear virtual page number bits by creating a mask, negating it, and anding it with the hex address
				// then substitute the physical page number by shifting the value and oring it with the result of the and operation
				hex_address = (hex_address & ~(createMask(config["page_offset_bits"], config["page_offset_bits"] + config["page_index_bits"] - 1))) | (physical_page_num << config["page_offset_bits"]);
			}

			if (access_type == 'W') { // writing to page (only occurs for data references)
				int i;
				PhysicalPage *current;
				for (i = 0; i < physical_pages->size(); ++i) {
					current = physical_pages->at(i);
					if (current->getPageNum() == physical_page_num) {
						current->setModified(true);
						break;
					}
				}
				page_table->setPageDirtyBit(physical_page_num); // update the dirty bit for corresponding entries
			}

			cache_index = (hex_address & createMask(config["data_cache_offset_bits"], config["data_cache_offset_bits"] + config["data_cache_index_bits"] - 1)) >> config["data_cache_offset_bits"];
			if (cache_index >= config["data_cache_sets"]) {	
				fprintf(stderr, "hierarchy: address %x is too large\n", orig_hex_address);						
				exit(EXIT_FAILURE);
			}
			cache_tag = (hex_address & createMask(config["data_cache_offset_bits"] + config["data_cache_index_bits"], 31)) >> (config["data_cache_offset_bits"] + config["data_cache_index_bits"]);
			result = data_cache->readEntry(cache_index, cache_tag);
			if (result) {
				cache_ref = "hit";
				++dc_hits;
				if (config["data_cache_write_through"]) { // write-through, no-write allocate
					if (access_type == 'W') {
						// update cache, access and update next level of memory hierarchy
						++memory_refs;
					} else {
						// do nothing
					}
				} else { // write-back, write allocate
					if (access_type == 'W') {
						// update cache (set dirty bit)
						data_cache->updateDirtyEntry(cache_index, cache_tag);
					} else {
						// do nothing
					}
				}
			} else {
				cache_ref = "miss";
				++dc_misses;
				if (config["data_cache_write_through"]) { // write-through, no-write allocate
					if (access_type == 'W') {
						// access and update next level of memory hierarchy
						++memory_refs;
					} else {
						// bring in from memory, update cache
						++memory_refs;
						//cout << "Updating data cache with index " << cache_index << ", tag " << cache_tag << ", and page " << physical_page_num << endl;
						data_cache->addEntry(cache_index, cache_tag, physical_page_num, 0);
					}
				} else { // write-back, write allocate
					if (access_type == 'W') {
						is_dirty = data_cache->isLRUEntryDirty(cache_index);
						data_cache->addEntry(cache_index, cache_tag, physical_page_num, 1); // update cache with new entry (dirty because of write)
						++memory_refs; // access next level of memory hierarchy
						if (is_dirty) { // if replaced cache entry was dirty, update next level of memory hierarchy
							++memory_refs;
						}
					} else {
						data_cache->addEntry(cache_index, cache_tag, physical_page_num, 0); // update cache
						++memory_refs; // access next level of memory hierarchy
					}
				}
			}
		}

		printf("%08x ", orig_hex_address);
		if (config["virtual_addresses_enabled"]) {
			printf("%7x ", virtual_page_num);
		} else {
			printf("%7s ", " ");
		}
		printf("%6x ", page_offset);
		printf("%-4s ", ref_type.c_str());
		if (config["tlbs_enabled"]) {
			printf("%7x %5x %-4s ", tlb_tag, tlb_index, tlb_ref.c_str());
		} else {
			printf("%7s %5s %4s ", " ", " ",  " ");
		}
		if (config["virtual_addresses_enabled"]) {
			printf("%-4s ", pt_ref.c_str());
		} else {
			printf("%4s ", " ");
		}
		printf("%6x ", physical_page_num);
		printf("%7x %5x %-4s\n", cache_tag, cache_index, cache_ref.c_str());
	}
	
	printf("\nSimulation statistics\n\n");

	printf("%-17s: %d\n", "itlb hits", itlb_hits);
	printf("%-17s: %d\n", "itlb misses", itlb_misses);
	printf("%-17s: ", "itlb hit ratio");
	if (config["tlbs_enabled"] && (itlb_hits > 0 || itlb_misses > 0)) {
		printf("%f\n\n", static_cast<double>(itlb_hits) / (itlb_hits + itlb_misses));
	} else {
		printf("N/A\n\n");
	}

	printf("%-17s: %d\n", "dtlb hits", dtlb_hits);
	printf("%-17s: %d\n", "dtlb misses", dtlb_misses);
	printf("%-17s: ", "dtlb hit ratio");
	if (config["tlbs_enabled"] && (dtlb_hits > 0 || dtlb_misses > 0)) {
		printf("%f\n\n", static_cast<double>(dtlb_hits) / (dtlb_hits + dtlb_misses));
	} else {
		printf("N/A\n\n");
	}
	
	printf("%-17s: %d\n", "pt hits", pt_hits);
	printf("%-17s: %d\n", "pt faults", pt_faults);
	printf("%-17s: ", "pt hit ratio");
	if (config["virtual_addresses_enabled"] && (pt_hits > 0 || pt_faults > 0)) {
		printf("%f\n\n", static_cast<double>(pt_hits) / (pt_hits + pt_faults));
	} else {
		printf("N/A\n\n");
	}
	
	printf("%-17s: %d\n", "ic hits", ic_hits);
	printf("%-17s: %d\n", "ic misses", ic_misses);
	printf("%-17s: ", "ic hit ratio");
	if (ic_hits > 0 || ic_misses > 0) {
		printf("%f\n\n", static_cast<double>(ic_hits) / (ic_hits + ic_misses));
	} else {
		printf("N/A\n\n");
	}
	
	printf("%-17s: %d\n", "dc hits", dc_hits);
	printf("%-17s: %d\n", "dc misses", dc_misses);
	printf("%-17s: ", "dc hit ratio");
	if (dc_hits > 0 || dc_misses > 0) {
		printf("%f\n\n", static_cast<double>(dc_hits) / (dc_hits + dc_misses));
	} else {
		printf("N/A\n\n");
	}

	printf("%-17s: %d\n", "Total reads", reads);
	printf("%-17s: %d\n", "Total writes", writes);
	printf("%-17s: ", "Ratio of reads");
	if (reads > 0 || writes > 0) {
		printf("%f\n\n", static_cast<double>(reads) / (reads + writes));
	} else {
		printf("N/A\n\n");
	}

	printf("%-17s: %d\n", "Total inst refs", inst_refs);
	printf("%-17s: %d\n", "Total data refs", data_refs);
	printf("%-17s: ", "Ratio of insts");
	if (inst_refs > 0 || data_refs > 0) {
		printf("%f\n\n", static_cast<double>(inst_refs) / (inst_refs + data_refs));
	} else {
		printf("N/A\n\n");
	}
	
	printf("%-17s: %d\n", "main memory refs", memory_refs);
	printf("%-17s: %d\n", "disk refs", disk_refs);
	
	delete instruction_cache;
	delete data_cache;
	delete page_table;
	delete instruction_tlb;
	delete data_tlb;

	for (int i = 0; i < physical_pages->size(); ++i) {
		delete physical_pages->at(i);
	}

	delete physical_pages;

	return 0;
}

bool isPowerOfTwo(int n)
{
	while (n > 1 && n % 2 == 0) {
		n /= 2;
	}
	return (n == 1);
}

void getConfig(string config_filename, map<string, int>& config)
{
	ifstream in_file;
	string file_str;
	char file_char;
	int file_num;

	in_file.open(config_filename.c_str());
	if (!in_file.is_open()) {
		fprintf(stderr, "hierarchy: failed to open configuration file %s", config_filename.c_str());
		exit(EXIT_FAILURE);
	}

	getline(in_file, file_str);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 256) {
		fprintf(stderr, "hierarchy: the number of instruction TLB sets must be between 1 and 256, inclusive\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the number of instruction TLB sets must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["instruction_tlb_sets"] = file_num;
	config["instruction_tlb_index_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8) {
		fprintf(stderr, "hierarchy: instruction TLB associativity must be between 1 and 8, inclusive\n");
		exit(EXIT_FAILURE);
	}
	config["instruction_tlb_set_size"] = file_num;

	getline(in_file, file_str);

	getline(in_file, file_str);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 256) {
		fprintf(stderr, "hierarchy: the number of data TLB sets must be between 1 and 256, inclusive\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the number of data TLB sets must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["data_tlb_sets"] = file_num;
	config["data_tlb_index_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8) {
		fprintf(stderr, "hierarchy: data TLB associativity must be between 1 and 8, inclusive\n");
		exit(EXIT_FAILURE);
	}
	config["data_tlb_set_size"] = file_num;

	getline(in_file, file_str);

	getline(in_file, file_str);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num > 8192) {
		fprintf(stderr, "hierarchy: the number of virtual pages cannot be greater than 8192\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the number of virtual pages must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["virtual_pages"] = file_num;
	config["page_index_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num > 1024) {
		fprintf(stderr, "hierarchy: the number of physical pages cannot be greater than 1024\n");
		exit(EXIT_FAILURE);
	}
	config["physical_pages"] = file_num;
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the page size must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["page_size"] = file_num;
	config["page_offset_bits"] = log2(file_num);

	getline(in_file, file_str);

	getline(in_file, file_str);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8192) {
		fprintf(stderr, "hierarchy: the number of instruction cache must be between 1 and 8192, inclusive\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the number of instruction cache sets must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["instruction_cache_sets"] = file_num;
	config["instruction_cache_index_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8) {
		fprintf(stderr, "hierarchy: instruction cache associativity must be between 1 and 8\n");
		exit(EXIT_FAILURE);
	}
	config["instruction_cache_set_size"] = file_num;
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 4) {
		fprintf(stderr, "hierarchy: the instruction cache line size must be at least 4\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the instruction cache line size must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["instruction_cache_line_size"] = file_num;
	config["instruction_cache_offset_bits"] = log2(file_num);

	getline(in_file, file_str);

	getline(in_file, file_str);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8192) {
		fprintf(stderr, "hierarchy: the number of data cache must be between 1 and 8192, inclusive\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the number of data cache sets must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["data_cache_sets"] = file_num;
	config["data_cache_index_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 1 || file_num > 8) {
		fprintf(stderr, "hierarchy: data cache associativity must be between 1 and 8, inclusive\n");
		exit(EXIT_FAILURE);
	}
	config["data_cache_set_size"] = file_num;
	getline(in_file, file_str, ':');
	in_file.ignore();
	in_file >> file_num;
	if (file_num < 8) {
		fprintf(stderr, "hierarchy: the data cache line size must be at least 8\n");
		exit(EXIT_FAILURE);
	}
	if (!isPowerOfTwo(file_num)) {
		fprintf(stderr, "hierarchy: the data cache line size must be a power of two\n");
		exit(EXIT_FAILURE);
	}
	config["data_cache_line_size"] = file_num;
	config["data_cache_offset_bits"] = log2(file_num);
	getline(in_file, file_str, ':');
	in_file.ignore();
	file_char = in_file.get();
	if (file_char == 'y') {
		config["data_cache_write_through"] = 1;
	} else if (file_char == 'n') {
		config["data_cache_write_through"] = 0;
	} else {
		fprintf(stderr, "hierarchy: invalid value for write-through configuration\n");
		exit(EXIT_FAILURE);
	}

	getline(in_file, file_str);

	getline(in_file, file_str, ':');
	in_file.ignore();
	file_char = in_file.get();
	if (file_char == 'y') {
		config["virtual_addresses_enabled"] = 1;
	} else if (file_char == 'n') {
		config["virtual_addresses_enabled"] = 0;
	} else {
		fprintf(stderr, "hierarchy: invalid value for virtual address configuration\n");
		exit(EXIT_FAILURE);
	}

	getline(in_file, file_str, ':');
	in_file.ignore();
	file_char = in_file.get();
	if (file_char == 'y') {
		config["tlbs_enabled"] = 1;
	} else if (file_char == 'n') {
		config["tlbs_enabled"] = 0;
	} else {
		fprintf(stderr, "hierarchy: invalid value for TLB configuration\n");
		exit(EXIT_FAILURE);
	}

	in_file.close();
}

void printConfig(map<string, int>& config)
{
	printf("Instruction TLB contains %d sets.\nEach set contains %d entries.\nNumber of bits used for the index is %d.\n\n", config["instruction_tlb_sets"], config["instruction_tlb_set_size"], config["instruction_tlb_index_bits"]);
	printf("Data TLB contains %d sets.\nEach set contains %d entries.\nNumber of bits used for the index is %d.\n\n", config["data_tlb_sets"], config["data_tlb_set_size"], config["data_tlb_index_bits"]);
	printf("Number of virtual pages is %d.\nNumber of physical pages is %d.\nEach page contains %d bytes.\nNumber of bits used for the page table index is %d.\nNumber of bits used for the page offset is %d.\n\n", config["virtual_pages"], config["physical_pages"], config["page_size"], config["page_index_bits"], config["page_offset_bits"]);
	printf("I-cache contains %d sets.\nEach set contains %d entries.\nEach line is %d bytes.\nNumber of bits used for the index is %d.\nNumber of bits used for the offset is %d.\n\n", config["instruction_cache_sets"], config["instruction_cache_set_size"], config["instruction_cache_line_size"], config["instruction_cache_index_bits"], config["instruction_cache_offset_bits"]);
	printf("D-cache contains %d sets.\nEach set contains %d entries.\nEach line is %d bytes.\n", config["data_cache_sets"], config["data_cache_set_size"], config["data_cache_line_size"]);
	if (config["data_cache_write_through"]) {
		printf("The cache uses a no write-allocate and write-through policy.\n");
	} else {
		printf("The cache uses a write-allocate and write-back policy.\n");
	}
	printf("Number of bits used for the index is %d.\nNumber of bits used for the offset is %d.\n\n", config["data_cache_index_bits"], config["data_cache_offset_bits"]);

	if (config["virtual_addresses_enabled"]) {
		printf("The addresses read in are virtual addresses.\n");
	} else {
		printf("The addresses read in are physical addresses.\n");
	}

	if (!config["tlbs_enabled"]) {
		printf("TLBs are disabled in this configuration.\n");
	}
}

unsigned int createMask(int startBit, int endBit)
{
	unsigned int mask = 0;
	for (int i = startBit; i <= endBit; ++i) {
		mask |= 1 << i;
	}
	return mask;
}