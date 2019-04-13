class CMSketch{
	private:
		const int d;
		int w = 0;
		int** counters = {NULL};
		BOBHash32** hash = {NULL};
	public:
		CMSketch(int memory_bytes, int _d = 3):d(_d){
			w = (memory_bytes/sizeof(int)) / d;
			counters = new int*[d];
			hash = new BOBHash32*[d];
			for(int i = 0; i < d; i++){
				counters[i] = new int[w];
				memset(counters[i], 0, w*sizeof(int));
				hash[i] = new BOBHash32(i + 760);
			}
		}
		~CMSketch(){
			for(int i = 0; i < d; i++){
				delete counters[i];
				delete hash[i];
			}
		}
		void insert(const char *key, int f = 1){
			for(int i = 0; i < d; i++){
				int index = (hash[i]->run(key, key_length)) % w;
				counters[i][index] += f;
			}
		}
		int query(const char *key){
			int ret = 1 << 30;
			for (int i = 0; i < d; i++){
				int index = (hash[i]->run(key, key_length)) % w;
				int tmp = counters[i][index];
				if(tmp < ret)
					ret = tmp;
			}
			return ret;
		}

};
