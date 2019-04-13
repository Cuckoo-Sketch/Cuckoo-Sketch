#include"CMSketch.h"
#include"CuckooPart.h"
class CuckooSketch{
	private:
		CMSketch cm_sketch;
		CuckooPart prior_part;
	public:
		CuckooSketch(int memory, double alpha):
			cm_sketch(memory - (int)(memory*alpha)), prior_part((int)(memory * alpha)){}
		int query(const char *key){
			int result = prior_part.query(key);
			if(result == -1){
				return cm_sketch.query(key);
			}
			else{
				return result;
			}
		}
		void insert(const char *key, int priority){
			int sig = prior_part.insert(key, priority);
			if(sig == 1)
				return;
			else if(sig == 0){
				cm_sketch.insert(prior_part.replace_item.key, prior_part.replace_item.count);
				return;
			}
			else{
				cm_sketch.insert(key);
			}
		}
};

