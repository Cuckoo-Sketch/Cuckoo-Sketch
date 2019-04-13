#include"param.h"
#include"CuckooSketch.h"
using namespace std;
struct FIVE_TUPLE{
	char key[13];
};
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];

int init_Priority(unordered_map<string,int> &Read_Freq, unordered_map<string, int> &Priority_List, 
		double alpha = 1, int random_seed = 123){
	srand(random_seed);

	double Dist[8];
	double tmp = 0;
	for(int i = 0; i < 8; i++){
		Dist[i] = 1/pow(i+1, alpha);
		tmp += Dist[i];
		if(i > 0)
			Dist[i] += Dist[i-1];
	}
	for(int i = 0; i < 8; i++)
		Dist[i] = Dist[i]/tmp;
	int count[8] = {0};
	for(unordered_map<string,int>::iterator i = Read_Freq.begin();
			i != Read_Freq.end(); ++i){
		double rd = ((double)rand()) / ((double) RAND_MAX);
		Priority_List[i->first] = 0;
		for(int k = 0; k < 8; k++){
			if(rd <= Dist[k]){
				Priority_List[i->first] = k;
				count[k]+=1;
				break;
			}
		}	
	}
	printf("-------------------------------------\n");
	int count_all = 0;
	for(int i = 0; i < 8; i++){
		printf("(Priority %d) %d streams, %.2f%% of all, %.2f%% expected\n", 
				i, count[i], ((double)count[i])/((double)Priority_List.size())*100, (Dist[i]-Dist[i-1]) * 100);
		count_all += count[i];
	}
	printf("totally %d prior streams\n", count_all);
	return count_all;
}

void ReadInTraces(const char *trace_prefix){
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++){
		char datafileName[100];
		sprintf(datafileName, "%s%d.dat", trace_prefix, datafileCnt);
		
		FILE *fin = fopen(datafileName, "rb");
		cout << datafileName << endl;

		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt].clear();

		while(fread(&tmp_five_tuple, 1, 13, fin) == 13){
			traces[datafileCnt].push_back(tmp_five_tuple);
		}
		fclose(fin);
		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt].size());
	}
	printf("\n");
}

void CM_test(const TRACE &trace, int memory, 
		unordered_map<string, int> &Read_Freq, unordered_map<string, int> &Priority_List){
	printf("-------------------------------------\n");	
	CMSketch cm_sketch(memory);
	int packet_cnt = trace.size();
	clock_t start = clock();
	for(int i = 0; i < packet_cnt; i++){
		cm_sketch.insert(trace[i].key);
	}
	clock_t end = clock();
	double AE = 0;
	double RE = 0;
	int count_precise = 0;
	for(unordered_map<string, int>::iterator it = Priority_List.begin(); it!=Priority_List.end(); ++it){
		int error = ABS(Read_Freq[it->first] - cm_sketch.query(it->first.c_str()));
		count_precise += (error == 0);
		AE += error;
		RE += ((double)error)/((double)Read_Freq[it->first]);
	}
	double AAE = AE/Priority_List.size();
	double ARE = RE/Priority_List.size();
	printf("(CM Sketch)\n");
	printf("AAE: %f, ARE: %f, precision: %.2f%%\n", AAE, ARE, ((double)count_precise)/(double)Priority_List.size() * 100);
	printf("Time: %f ms, Throughput: %f M/s\n", ((double)(end-start))/1000, ((double)trace.size())/(end-start));
	return;
}

void Prior_test(const TRACE &trace, int memory, double alpha,
		unordered_map<string, int> &Read_Freq, unordered_map<string, int> &Priority_List){
	printf("-------------------------------------\n");	
	printf("(Cuckoo Sketch) memory_usage: %d, alpha: %f\n", memory, alpha);

	CuckooSketch cuckoo_sketch(memory, alpha);
	int packet_cnt = trace.size();
	clock_t start = clock();
	for(int i = 0; i < packet_cnt; i++){
		string str(trace[i].key, key_length);
		cuckoo_sketch.insert(trace[i].key, Priority_List[str]);
	}
	clock_t end = clock();

	double AE_all = 0;
	double RE_all = 0;
	double AE_prior = 0;
	double RE_prior = 0;
	double RE_separated[8] = {0};
	int count_precision[8] = {0};
	int count_all = 0;
	int count_prior = 0;
	int count_separated[8] = {0};
	for(unordered_map<string, int>::iterator it = Priority_List.begin(); it!=Priority_List.end(); ++it){

		int error = ABS(Read_Freq[it->first] - cuckoo_sketch.query(it->first.c_str()));
		double relative_error = ((double)error)/((double)Read_Freq[it->first]);

		AE_all += error;
		RE_all += relative_error;
		count_all += 1;

		if(it->second != 0){
			AE_prior += error;
			RE_prior += relative_error;
			count_prior+=1;
		}	

		RE_separated[it->second] += relative_error;
		count_separated[it->second] += 1;

		count_precision[it->second] += (error == 0);
	}

	double AAE_all = AE_all/count_all;
	double ARE_all = RE_all/count_all;
	printf("AAE_all: %f, ARE_all: %f\n", AAE_all, ARE_all);
	double AAE_prior = AE_prior/count_prior;
	double ARE_prior = RE_prior/count_prior;
	printf("AAE_prior: %f, ARE_prior: %f\n", AAE_prior, ARE_prior);	
	int count_total_precised = 0;
	double WRE = 0;
	int Wcount = 0;
	for(int i = 0; i < 8; i++){
		Wcount += count_separated[i] * (i+1);
		WRE += RE_separated[i] * (i+1);
		if(i > 0)
			count_total_precised += count_precision[i];

		double ARE = RE_separated[i]/count_separated[i];
		printf("(Priority %d) ARE: %f, precision %.2f%% <%d precise streams, %d total>\n",
				i, ARE, ((double)count_precision[i])/((double)count_separated[i]) * 100,
				count_precision[i], count_separated[i]);
	}
	printf("WARE: %f\n", WRE/Wcount);
	printf("precision: %.2f%%\n", ((double)count_total_precised)/((double)count_prior) * 100);
	printf("Time: %f ms, Throughput: %f M/s\n", ((double)(end-start))/1000, ((double)trace.size())/(end-start));
	return;
}

void Single_trace_test(const TRACE &trace, int memory){
	int packet_cnt = (int) trace.size();
	unordered_map<string, int> Read_Freq;
	unordered_map<string, int> Priority_List;
	for(int i = 0; i < packet_cnt; ++i){
		string str((const char*)(trace[i].key), key_length);
		Read_Freq[str]++;
	}
	printf("%d packets\n%d flows\n", packet_cnt, Read_Freq.size());	

	init_Priority(Read_Freq, Priority_List,2);
	CM_test(trace, memory, Read_Freq, Priority_List);
	Prior_test(trace, memory, 0.2, Read_Freq, Priority_List);
	return;
}
int main(int argc, char** argv){
	ReadInTraces("./data/");
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; datafileCnt++){
		printf("Trace %d:\n", datafileCnt);
		Single_trace_test(traces[datafileCnt], 1024*1024);
		printf("=====================================\n");
	}
	return 0;
}
