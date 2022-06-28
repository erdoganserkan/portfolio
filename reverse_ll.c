typedef struct llmem_s {
	int node;
	struct llmem_s *next, *prev;
} llmem_t;

llmem_t *reverse_ll(llmem_t *head) {
	llmem_t *prev=NULL, *next=NULL, *current;
	for(current = head ; ;) {
		if(!current)
			break;		// list is NULL //

		next = current->next;
		current->next = prev;

		if(!next){
			// End of loop
			return current;	//	NEW HEAD is LAST ITEM //
		}
		prev = current;
		current = next;
	}

	return current;
}

llmem_t * init_ll (int cnt) {
	llmem_t *ret = NULL;
	llmem_t *prev = NULL;
	srand(0);
	volatile int ix= 0;
	for(ix=0 ; ix<cnt ; ix++) {
		llmem_t *itemp = (llmem_t *)calloc(1, sizeof(llmem_t));
		if(!itemp)
			return ret;
		if(!ret)
			ret = itemp;
		itemp->node = rand();
		if(prev) {
			prev->next = itemp;
		}
		prev = itemp;
	}

	return ret;
}

void print_ll(llmem_t *head) {
	llmem_t *itemp = head;
	cout << "listing linked-list items : " << endl;
	while(itemp) {
		cout << "item_node: " << itemp->node << " " << endl;
		itemp = itemp->next;
	}
	cout << endl;
}

