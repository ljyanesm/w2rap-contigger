#include "FastIfstream.h"
#include "FetchReads.h"
#include "Intvector.h"
#include "MainTools.h"
#include "PairsManager.h"
#include "ParallelVecUtilities.h"
#include "feudal/ObjectManager.h"
#include "feudal/PQVec.h"
#include "lookup/LookAlign.h"
#include "paths/HyperBasevector.h"
#include "paths/RemodelGapTools.h"
#include "paths/long/BuildReadQGraph.h"
#include "paths/long/PlaceReads0.h"
#include "paths/long/SupportedHyperBasevector.h"
#include "paths/long/large/AssembleGaps.h"
#include "paths/long/large/Repath.h"
#include "tclap/CmdLine.h"



int qgraph_builder(const String work_dir, const string file_prefix, uint large_k, uint NUM_THREADS, int MAX_MEM_GB, bool DUMP_ALL){
  
    // ********************** Set sys resources ******************
    // Set computational limits (XXX TODO: putin a separate source to import in different code)
    std::cout << "Set computational resources " << std::endl;
    PrintSysInfo();
    SetThreads(NUM_THREADS, False);
    int64_t max_bytes = int64_t(round(MAX_MEM_GB * 1024.0 * 1024.0 * 1024.0));
    SetMaxMemory(max_bytes);

    // ********************** Load files *************************
    std::cout << "Loading files " << std::endl;
    

    vec<String> subsam_names =  { "C" };
    vec<int64_t> subsam_starts = { 0 };

    vecbvec bases;
    bases.ReadAll( work_dir + "/frag_reads_orig.fastb" );
    VecPQVec quals;
    quals.ReadAll( work_dir + "/frag_reads_orig.qualp" );

    // variables to run buildReadQGraph XXX TODO: Document one by one
    std::cout << "Starting to build Reads qgraph" << std::endl;
    HyperBasevector hbv;
    ReadPathVec paths;
    bool FILL_JOIN=False;
    bool SHORT_KMER_READ_PATHER=False;
    bool RQGRAPHER_VERBOSE=False;
    
    buildReadQGraph(bases, quals, FILL_JOIN, FILL_JOIN, 7, 3, .75, 0, "", True, SHORT_KMER_READ_PATHER, &hbv, &paths, RQGRAPHER_VERBOSE);
    std::cout << "Finish building Reads qgraph" << std::endl;
    
    vecbvec edges(hbv.Edges().begin(), hbv.Edges().end());
    vec<int> inv;
    hbv.Involution(inv);
    
    FixPaths( hbv, paths );

    if (DUMP_ALL) {
        // Save graph kmer 60
        std::cout << "Dumping small_k graph... " << std::endl;
        BinaryWriter::writeFile(work_dir + "/" + file_prefix + ".60.hbv", hbv);
        //BinaryWriter::writeFile(work_dir + "/" + file_prefix + ".60.hbx", HyperBasevectorX(hbv));
        edges.WriteAll(work_dir + "/" + file_prefix + ".60.fastb");
        //BinaryWriter::writeFile(work_dir + "/" + file_prefix + ".60.inv", inv);
        std::cout << "   DONE!" << std::endl;
    }

    
    int64_t checksum_60 = hbv.CheckSum( );
    PRINT(checksum_60);
    
    // Variables to run Repath XXX TODO: Document one by one
    const string run_head = work_dir + "/" + file_prefix;
    Repath( hbv, edges, inv, paths, hbv.K(), large_k, run_head+".large", True, True, False );

    if (DUMP_ALL) {
        hbv.DumpFasta( run_head + ".after_repath.fasta", False );
    }
    return 0;
}

int main(int argc, const char* argv[]){
    std::string out_prefix;
    std::string out_dir;
    bool dump_all;
    unsigned int small_K,large_K;
    unsigned int threads;
    int max_mem;
    std::vector<unsigned int> allowed_k={60,64,72,80,84,88,96,100,108,116,128,136,144,152,160,168,172,180,188,192,196,200,208,216,224,232,240,260,280,300,320,368,400,440,460,500,544,640};


    //========== Command Line Option Parsing ==========

    std::cout<<"Welcome to w2rap-contigger::02_qgraph"<<std::endl;
    try {
        TCLAP::CmdLine cmd("", ' ', "0.1 alpha");

        TCLAP::ValueArg<std::string> out_dirArg     ("o","out_dir",     "Output dir path",           true,"","string",cmd);
        TCLAP::ValueArg<std::string> out_prefixArg     ("p","prefix",     "Prefix for the output files",           true,"","string",cmd);
        //TCLAP::ValueArg<unsigned int>         small_KArg        ("k","small_k",        "Small k (default: 60)", false,60,"int",cmd);
        TCLAP::ValueArg<bool>         dumpAllArg        ("","dump_all",        "Enable extra data dumps", false,false,"bool",cmd);
        TCLAP::ValuesConstraint<unsigned int> largeKconst (allowed_k);
        TCLAP::ValueArg<unsigned int>         large_KArg        ("K","large_k",        "Large k (default: 200)", false,200,&largeKconst,cmd);
        TCLAP::ValueArg<unsigned int>         threadsArg        ("t","threads",        "Number of threads on parallel sections (default: 4)", false,4,"int",cmd);
        TCLAP::ValueArg<unsigned int>         max_memArg       ("m","max_mem",       "Maximum memory in GB (soft limit, impacts performance, default 10000)", false,10000,"int",cmd);

        cmd.parse( argc, argv );

        // Get the value parsed by each arg.
        out_dir=out_dirArg.getValue();
        out_prefix=out_prefixArg.getValue();
        dump_all=dumpAllArg.getValue();
        //small_K=small_KArg.getValue();
        large_K=large_KArg.getValue();
        threads=threadsArg.getValue();
        max_mem=max_memArg.getValue();

    } catch (TCLAP::ArgException &e)  // catch any exceptions
    { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; return 1;}




    qgraph_builder(out_dir, out_prefix, /*small_K,*/ large_K, threads, max_mem, dump_all );

    return 0;
}
