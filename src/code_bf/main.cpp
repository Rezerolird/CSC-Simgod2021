#include <string>
#include <fstream>
#include <set>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

#include "BIGSI.h"
#include "Rambo.h"
#include "CSCBF.h"
#include "CSCBFBIGSI.h"

#define NUM_DB 2456
#define HASH_NUM 3
#define REP_TIME 3
#define PART_NUM 62
#define TOTAL_CAP 60000000

using namespace std;

int main(){

    //initial
    BIGSI baseline_bigsi(NUM_DB, HASH_NUM, TOTAL_CAP); 
    RAMBO baseline_rambo(REP_TIME, PART_NUM, TOTAL_CAP, HASH_NUM, NUM_DB); 
    CSCBF cscbf(REP_TIME, PART_NUM, TOTAL_CAP, HASH_NUM, NUM_DB); // CSC-RAMBO
    CSCBFBIGSI cscbf_bigsi(TOTAL_CAP, HASH_NUM, NUM_DB);    

    // data insertion
    string file_prefix = "class_";
    string lines; lines.clear(); int j = 0;
    for(int i=0; i<NUM_DB; i++){
        cout << i << endl;
        string file_name = file_prefix + to_string(i);
        ifstream file(file_name);
        if(!file.is_open()){
            cout << "open file error!" << endl;
            return 0;
        }
        while(getline(file, lines)){
            if(file.eof()){
                break;
            }
            baseline_bigsi.insertion(i, lines);
            baseline_rambo.insertion(to_string(i), lines);
            cscbf.insertion(to_string(i), lines);
            cscbf_bigsi.insertion(to_string(i), lines);

            lines.clear();
        }
        file.close();
        file_name.clear();
    }


    /*** query insert ***/
    string query_file1 = "query_new";
    string line; line.clear();
    ifstream new_query(query_file1);
    stringstream sstr;
    set<string> query_element; query_element.clear();
    string element; element.clear(); int set_ID;
    map< string, set<size_t> > ground_truth;
    int hh = 0;
    while (getline(new_query, line))
    {
        if(new_query.eof()){
            break;
        }
        sstr << line;
        sstr >> element;
        query_element.insert(element);

        while(sstr >> set_ID){
            baseline_bigsi.insertion(set_ID, element);
            baseline_rambo.insertion(to_string(set_ID), element);
            cscbf.insertion(to_string(set_ID), element);
            cscbf_bigsi.insertion(to_string(set_ID), element);
            ground_truth[element].insert(size_t(set_ID));
        }

        sstr.clear(); 
    }
    cout << "insertion done..." << endl;

    //query
    double bigsi_error = 0.0f; double bigsi_time = 0.0f;
    double rambo_error = 0.0f; double rambo_time = 0.0f;
    double cscbf_error = 0.0f; double cscbf_time = 0.0f;
    double cscbfbigsi_error = 0.0f; double cscbfbigsi_time = 0.0f;

    int count = 0;
    set<size_t> bigsi_res; bigsi_res.clear();
    set<size_t> rambo_res; rambo_res.clear();
    set<size_t> cscbf_res; cscbf_res.clear();
    set<size_t> cscbfbigsi_res; cscbfbigsi_res.clear();
    set<size_t> fp_set; fp_set.clear();

    for(auto &ele : query_element){
        bigsi_res = baseline_bigsi.query(ele);
        set_difference(bigsi_res.begin(), bigsi_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin())); 
        bigsi_error += double(fp_set.size()); bigsi_res.clear(); fp_set.clear();

        rambo_res = baseline_rambo.query(ele);
        set_difference(rambo_res.begin(), rambo_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        rambo_error += double(fp_set.size()) ; rambo_res.clear(); fp_set.clear();

        cscbf_res = cscbf.query(ele);
        set_difference(cscbf_res.begin(), cscbf_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        cscbf_error += double(fp_set.size()); cscbf_res.clear(); fp_set.clear();

        cscbfbigsi_res = cscbf_bigsi.query(ele);
        set_difference(cscbfbigsi_res.begin(), cscbfbigsi_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        cscbfbigsi_error += double(fp_set.size()); cscbfbigsi_res.clear(); fp_set.clear();
        count++;
    }

    /* ave false positive */
    bigsi_error /= count; bigsi_time = baseline_bigsi.time / count;
    rambo_error /= count; rambo_time = baseline_rambo.time / count;
    cscbf_error /= count; cscbf_time = cscbf.time / count;
    cscbfbigsi_error /= count; cscbfbigsi_time = cscbf_bigsi.time / count;

    cout << bigsi_error << " " << bigsi_time << endl;
    cout << rambo_error << " " << rambo_time << endl;
    cout << cscbf_error << " " << cscbf_time << endl;
    cout << cscbfbigsi_error << " " << cscbfbigsi_time << endl;

    return 0;
}
