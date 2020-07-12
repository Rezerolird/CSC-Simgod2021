#include <string>
#include <fstream>
#include <set>
#include <iostream>
#include <cmath>
#include <sstream>
#include <map>
#include "CuckooFilter.h"
#include "NCF.h"
#include "RAMCO.h"

#define NUM_DB 500
#define R 3
#define B 50
using namespace std;

int main(){

    uint64_t total_items_csccf = pow(2, 29);
    uint64_t total_items_ncf = pow(2, 22);

    //initial
    const uint32_t f = 2;
    CuckooFilter<uint32_t, 3*f> CF(NUM_DB, 500, total_items_csccf); 
    NCF<uint32_t, f> ncf(NUM_DB, 500, total_items_ncf);
    RAMCO<uint32_t, f> ramco(NUM_DB, 500, total_items_csccf, R, B);

    //data insertion
    string file_prefix = "/class_";
    string lines; lines.clear();
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
            CF.Insert(i, lines);
            ramco.Insert(i, lines);
            ncf.Insert(i, lines);

            lines.clear();
        }
        file.close();
        file_name.clear();
    }

    //query insertion
    string query_file1 = "query_new";
    string line; line.clear();
    ifstream new_query(query_file1);
    stringstream sstr;
    set<string> query_element; query_element.clear();
    string element; element.clear(); int set_ID;
    map< string, set<size_t> > ground_truth;

    while (getline(new_query, line))
    {
        if(new_query.eof()){
            break;
        }
        /* code */
        sstr << line;
        sstr >> element;
        query_element.insert(element);
        while(sstr >> set_ID){
            CF.Insert(set_ID, element);
            ncf.Insert(set_ID, element);
            ramco.Insert(set_ID, element);
            ground_truth[element].insert(size_t(set_ID));
        }
        sstr.clear(); //element.clear(); line.clear();
    }
    cout << "insertion done..." << endl;

    //query
    double csccf_error = 0.0f; double csccf_time = 0.0f; 
    double ncf_error = 0.0f; double ncf_time = 0.0f; 
    double ramco_error = 0.0f; double ramco_time = 0.0f; 

    int count = 0;

    set<size_t> csccf_res; csccf_res.clear();
    set<size_t> ncf_res; ncf_res.clear();
    set<size_t> ramco_res; ramco_res.clear();
    set<size_t> fp_set; fp_set.clear();

    for(auto &ele : query_element){

        ramco_res = ramco.Query(ele);
        set_difference(ramco_res.begin(), ramco_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        ramco_error += double(fp_set.size()); ramco_res.clear(); fp_set.clear();     

        csccf_res = CF.Query(ele);
        set_difference(csccf_res.begin(), csccf_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        csccf_error += double(fp_set.size()); csccf_res.clear(); fp_set.clear();     

        ncf_res = ncf.Query(ele);   
        set_difference(ncf_res.begin(), ncf_res.end(), ground_truth[ele].begin(), ground_truth[ele].end(), inserter(fp_set, fp_set.begin()));
        ncf_error += double(fp_set.size()); ncf_res.clear(); fp_set.clear();     

        count++;    

    }

    /* ave false positive */
    csccf_error /= count; csccf_time = CF.time / count;
    ncf_error /= count; ncf_time = ncf.time / count;
    ramco_error /= count; ramco_time = ramco.time / count;

    cout << ramco_error << " " << ramco_time << endl;
    cout << csccf_error << " " << csccf_time << endl;
    cout << ncf_error << " " << ncf_time << endl;

    return 0;
}
