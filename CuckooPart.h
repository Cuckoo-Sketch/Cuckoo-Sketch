#define hash_seed1 13
#define hash_seed2 17
using namespace std;
struct Item{
	char key[key_length + 1];
	int priority;
	int count;
};
struct Bucket{
	Item items[bucket_size];
	bool empty[bucket_size];

	Bucket(){
		for(int k = 0; k < bucket_size; k++)
			empty[k] = true;
	}
};
void item_copy(Item& dst, Item& src){
	memcpy(dst.key, src.key, key_length);
	dst.priority = src.priority;
	dst.count = src.count;
}
class CuckooPart{
	private:
		int size;
		Bucket* buckets;
		BOBHash32* hash[2] = {NULL};
	public:
		Item replace_item;
		CuckooPart(int memory){
			size = (memory/(sizeof(int)+key_length))/bucket_size;
			buckets = new Bucket[size];
			hash[0] = new BOBHash32(hash_seed1);
			hash[1] = new BOBHash32(hash_seed2);
		}
		~CuckooPart(){
			delete buckets;
			delete hash[0];
			delete hash[1];
		}
		int query(const char *key){		
			int value[2];
			for(int i = 0; i < 2; i++){
				int hash_value = hash[i]->run(key, key_length) % size;
					for(int k = 0; k < bucket_size; k++){
						if(!buckets[hash_value].empty[k] && 
								!memcmp(key, buckets[hash_value].items[k].key, key_length))
						return buckets[hash_value].items[k].count;
				}
			}
			return -1;
		}
		int insert(const char *key, int priority){
			string str(key, key_length);
			int hash_value[2];
			hash_value[0] = hash[0]->run(key, key_length) % size;
			hash_value[1] = hash[1]->run(key, key_length) % size;

			Item *same_item_p = NULL, *empty_item_p = NULL, *lessPrior_item_p = NULL;
			bool *empty_p;
			int pos;
			int min_priority = priority;
			for(int i = 0; i < 2; i++){
				Bucket &tmp = buckets[hash_value[i]];
				for(int k = 0; k < bucket_size; k++){
					if(tmp.empty[k]){
						if(!empty_item_p){
							empty_item_p = &tmp.items[k];
							empty_p = &tmp.empty[k];
						}
					}
					else{
						if((!same_item_p) && (!memcmp(tmp.items[k].key, key, key_length)))
							same_item_p = &tmp.items[k];
						if(tmp.items[k].priority < min_priority){
							lessPrior_item_p = &tmp.items[k];
							pos = hash_value[i];
							min_priority = tmp.items[k].priority;
						}
						else if(tmp.items[k].priority == min_priority){
							if(!lessPrior_item_p || tmp.items[k].count < lessPrior_item_p->count){
								lessPrior_item_p = &tmp.items[k];
								pos = hash_value[i];
							}
						}
					}
				}
			}
			if(same_item_p){
				same_item_p->count++;
				return 1;
			}
			if(empty_item_p){
				memcpy(empty_item_p->key, key, key_length);
				empty_item_p->count = 1;
				empty_item_p->priority = priority;
				*empty_p = false;
				return 1;
			}
			if(!lessPrior_item_p){
				return -1;
			}

			item_copy(replace_item, *lessPrior_item_p);
			memcpy(lessPrior_item_p->key, key, key_length);
			lessPrior_item_p->count = 1;
			lessPrior_item_p->priority = priority;

			int cuckoo_count = 0;
			while(cuckoo_count < 1){
				pos = hash[0]->run(replace_item.key, key_length) % size
					+ hash[1]->run(replace_item.key, key_length) % size - pos;
				min_priority = replace_item.priority;
				empty_item_p = NULL;
				lessPrior_item_p = NULL;
				for(int k = 0; k < bucket_size; k++){
					if(buckets[pos].empty[k]){
						empty_item_p = &buckets[pos].items[k];
						empty_p = &buckets[pos].empty[k];
						break;
					}
					if(buckets[pos].items[k].priority < min_priority){
						lessPrior_item_p = &buckets[pos].items[k];
						min_priority = buckets[pos].items[k].priority;
					}
					else if(buckets[pos].items[k].priority == min_priority)
						if(!lessPrior_item_p || buckets[pos].items[k].count < lessPrior_item_p->count)
						   lessPrior_item_p = &buckets[pos].items[k];	
				}
				if(empty_item_p){
					item_copy(*empty_item_p, replace_item);	
					*empty_p = false;
					return 1;
				}
				if(!lessPrior_item_p)
					return 0;
				Item tmp;
				item_copy(tmp, *lessPrior_item_p);
				item_copy(*lessPrior_item_p, replace_item);
				item_copy(replace_item, tmp);
				cuckoo_count++;
			}
			return 0;
		}
};
