
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <string.h>

using std::setw;
using std::hex;
using std::cerr;
using std::string;
using std::ios;
using std::endl;
using std::dec;

#define INFOS
#define DEBUG

/* ================================================================== */
// Global variables 
/* ================================================================== */

const char *target_image = "matrix_multiplications.exe";
const char *target_routines[] = {
    "multiplyMatrix",
    "multiplySparseMatrix",
    // "print_matrix",
    // "print_sparse_matrix",
    ""
};

std::ostream * out = &cerr;

/* Use "xed_iform_enum_t" for index */
typedef struct InsTable {
    xed_decoded_inst_t *_xedd;
    string _iclassname;
    string _catname;
    string _extname;
    UINT64 _count;
    UINT64 _opdno;
    bool _isFMA;
} INS_TABLE;

typedef struct RtnCount {
    string _name;
    string _image;
    ADDRINT _address;
    RTN _rtn;
    UINT64 _rtnCount;
    UINT64 _icount;
    UINT64 _flopcount;
    INS_TABLE * _instable;
    struct RtnCount * _next;
} RTN_COUNT;

// Linked list of instruction counts for each routine
RTN_COUNT * RtnList = 0;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for MyPinTool output");

/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage() {
    cerr << "This tool prints out the number of dynamically executed " << endl <<
            "instructions, basic blocks and threads in the application." << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

const char * StripPath(const char * path) {
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}

const char * StripName(const char * fullname) {
    const char * name = strtok((char *)fullname, "(");
    if (name)
        return name;
    else
        return fullname;
}

bool isTargetRoutine(string funcname) {
    for (int i=0; *(target_routines[i]); i++)
        if (strcmp(StripName(funcname.c_str()), target_routines[i]) == 0 ) return true;
    return false;
}

bool INS_isFMA(INS ins) {
    xed_category_enum_t cat = xed_decoded_inst_get_category(INS_XedDec(ins));
    switch (cat) {
        case XED_CATEGORY_AVX512_4FMAPS:
        case XED_CATEGORY_FMA4:
        case XED_CATEGORY_IFMA:
        case XED_CATEGORY_VFMA:
            return true;
        default:
            break;
    }
    return false;
}

bool CAT_isFMA(xed_category_enum_t cat) {
    switch (cat) {
        case XED_CATEGORY_AVX512_4FMAPS:
        case XED_CATEGORY_FMA4:
        case XED_CATEGORY_IFMA:
        case XED_CATEGORY_VFMA:
            return true;
        default:
            break;
    }
    return false;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

// This function is called before every instruction is executed
VOID docount(UINT64 * counter) {
    (*counter)++;
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

VOID Image(IMG img, VOID *v) {
    INFOS printf( "[INFOS] Image Name: %s, %d\n", StripPath(IMG_Name(img).c_str()), strcmp(StripPath(IMG_Name(img).c_str()), target_image) );
    if( strcmp(StripPath(IMG_Name(img).c_str()), target_image) == 0 ) {
        for( SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) ) {
            for( RTN rtn= SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) ) {
                // DEBUG printf("    [DEBUG] Routine decorated Name: %s\n", (RTN_Name(rtn).c_str())); 
                // DEBUG printf("        [DEBUG] Full Routine Name: %s\n", PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_COMPLETE).c_str()); 
                // DEBUG printf("        [DEBUG] Only Routine Name: %s\n", PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY).c_str()); 
                // DEBUG printf("        [DEBUG] Stripped Routine Name: %s\n", StripName(PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY).c_str())); 
                string funcName = PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY);

                /* Instrument the multiplyMatrix() and multiplySparseMatrix() functions. */
                if ( isTargetRoutine(funcName) ) {
                    INFOS printf("        [INFOS] Decorated Routine Name: %s\n", RTN_Name(rtn).c_str());
                    // DEBUG printf("        [DEBUG] Undecorated Routine Name: %s\n", functionName.c_str());

                    /* Allocate a counter for this routine */
                    RTN_COUNT * rc = new RTN_COUNT;
                    INS_TABLE *instb = new INS_TABLE[XED_IFORM_LAST];
                    // memset(&instb->_count, 0, sizeof(UINT64) * XED_IFORM_LAST);
                    for (int i=0; i<XED_IFORM_LAST; i++ ) instb[i]._count = 0;

                    /* The RTN goes away when the image is unloaded, so save it now */
                    /* because we need it in the fini */
                    rc->_name = RTN_Name(rtn);
                    rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());
                    rc->_address = RTN_Address(rtn);
                    rc->_icount = 0;
                    rc->_rtnCount = 0;
                    rc->_flopcount = 0;
                    rc->_instable = instb;

                    /* Add to list of routines */
                    rc->_next = RtnList;
                    RtnList = rc;

                    RTN_Open(rtn);
                    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_rtnCount), IARG_END);

                    /* For each instruction of the routine */
                    for ( INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins) ) {

                        xed_decoded_inst_t* xedd = INS_XedDec(ins);
                        xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);
                        xed_iclass_enum_t iclass = xed_decoded_inst_get_iclass(xedd);
                        xed_category_enum_t cat = xed_decoded_inst_get_category(xedd);
                        xed_extension_enum_t ext = xed_decoded_inst_get_extension(xedd);
                        UINT32 iform_index = static_cast<UINT32>(iform);
                        // UINT32 iclass_index = static_cast<UINT32>(iclass);
                        // UINT32 cat_index = static_cast<UINT32>(cat);

                        // DEBUG printf("        [DEBUG] INS XED-IFORM:        %s\n", xed_iform_enum_t2str(iform));
                        // DEBUG printf("            [DEBUG] INS XED-ICLASS:   %s\n", xed_iclass_enum_t2str(iclass));
                        // DEBUG printf("            [DEBUG] INS XED-CATEGORY: %s\n", xed_category_enum_t2str(cat));

                        /* Store the basic information of instuctions in the INS_TABLE */
                        instb[iform_index]._xedd = new xed_decoded_inst_t;
                        *(instb[iform_index]._xedd) = *xedd;
                        instb[iform_index]._iclassname = xed_iclass_enum_t2str(iclass);
                        instb[iform_index]._catname = xed_category_enum_t2str(cat);
                        instb[iform_index]._extname = xed_extension_enum_t2str(ext);
                        instb[iform_index]._opdno = xed_decoded_inst_noperands(xedd);
                        instb[iform_index]._isFMA = CAT_isFMA(cat);

                        /* Insert a call to docount to increment the instruction counter for this rtn */
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_icount), IARG_END);
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(instb[iform_index]._count), IARG_END);

                        /* FLOP counts */
                        /* TODO: First, understand all flop ins, then design the computation */
                        // if (cat_index == XED_CATEGORY_SSE)
                        //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_flopcount), IARG_END);
                        // else if (cat_index == XED_CATEGORY_MMX)
                        //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_flopcount), IARG_END);
                        // else if (cat_index == XED_CATEGORY_X87_ALU)
                        //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_flopcount), IARG_END);
                        // else if(cat_index == XED_ICLASS_MOVSD_XMM)
                        //     INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_flopcount), IARG_END);
                    }

                    RTN_Close(rtn);
                }
            }
        }
    }
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v) {
    
    *out <<  "===============================================" << endl;
    *out <<  "              The Analysis Result              " << endl;
    *out <<  "===============================================" << endl;
 
    for (RTN_COUNT * rc = RtnList; rc; rc = rc->_next) {

        /* Basic Info */
        *out << "Routine (Procedure): " << rc->_name  << endl
             << "Image:               " << rc->_image  << endl
             << "Address:             " << "0x" << hex << rc->_address << dec  << endl
             << "Calls:               " << setw(10) << rc->_rtnCount  << endl
             << "Instructions counts: " << setw(10) << rc->_icount  << endl
             << "FLOP counts (TODO):  " << setw(10) << rc->_flopcount << endl;

        /* Info of all instructions */
        // if (rc->_icount > 0) 
        //     *out << setw(30) << "XED_IFORM"
        //             << setw(20) << "XED_ICLASS"
        //             << setw(20) << "XED_CATEGORY"
        //             << setw(20) << "counts"
        //             << endl; 
        // for (int i=0; i<XED_IFORM_LAST; i++) 
        //     if(rc->_instable[i]._count > 0) 
        //         *out << setw(30) << xed_iform_enum_t2str(static_cast<xed_iform_enum_t>(i)) 
        //                 << setw(20) << rc->_instable[i]._iclassname
        //                 << setw(20) << rc->_instable[i]._catname 
        //                 << setw(20) << rc->_instable[i]._count 
        //                 << endl; 
        // *out << endl;

        /* Info of "SEE" instructions */
        if (rc->_icount > 0) {
            *out << "FLOP instructions:   " << endl
                 << "     " << std::setiosflags(ios::left) 
                 << setw(25) << "XED_IFORM"
                 << std::resetiosflags(ios::left) 
                 << setw(10) << "XED_ICLASS"
                 << setw(10) << "XED_CAT"
                 << setw(10) << "XED_EXT"
                 << setw(10) << "is_FMA"
                 << setw(10) << "counts"
                 << setw(10) << "#opd"
                 << setw(10) << "opd_1" << "/" 
                 << std::setiosflags(ios::left) 
                 << setw(10) << "type"
                 << std::resetiosflags(ios::left)
                 << setw(10) << "#elements"
                 << setw(10) << "opd_2" << "/" 
                 << std::setiosflags(ios::left) 
                 << setw(10) << "type"
                 << std::resetiosflags(ios::left)
                 << setw(10) << "#elements"
                 << setw(10) << "opd_3" << "/" 
                 << std::setiosflags(ios::left) 
                 << setw(10) << "type"
                 << std::resetiosflags(ios::left)
                 << setw(10) << "#elements"
                 << endl; 
            for (int i=0; i<XED_IFORM_LAST; i++) {
                if(rc->_instable[i]._catname == "SSE") {
                    *out << "     " << std::setiosflags(ios::left) 
                         << setw(25) << xed_iform_enum_t2str(static_cast<xed_iform_enum_t>(i)) 
                         << std::resetiosflags(ios::left) 
                         << setw(10) << rc->_instable[i]._iclassname
                         << setw(10) << rc->_instable[i]._catname 
                         << setw(10) << rc->_instable[i]._extname
                         << setw(10) << rc->_instable[i]._isFMA 
                         << setw(10) << rc->_instable[i]._count 
                         << setw(10) << rc->_instable[i]._opdno;
                    for(int j=0; j<(int)xed_decoded_inst_noperands(rc->_instable[i]._xedd); j++) {
                        *out << setw(10) << xed_decoded_inst_operand_element_size_bits(rc->_instable[i]._xedd, j)
                             << "/" 
                             << std::setiosflags(ios::left) 
                             << setw(10) << xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(rc->_instable[i]._xedd, j))
                             << std::resetiosflags(ios::left)
                             << setw(10) << xed_decoded_inst_operand_elements(rc->_instable[i]._xedd, j);
                    }
                    *out << endl; 
                }
            }
        }
        *out << endl;
    }

    /* My Test Data */
    // string fma_iform_str[] = {"PFMAX_MMXq_MEMq",
    //                           "PFMAX_MMXq_MMXq",
    //                           "VFMADD132PS_YMMf32_MASKmskw_YMMf32_MEMf32_AVX512",
    //                           "VFMADD132PS_YMMqq_YMMqq_YMMqq",
    //                           "V4FMADDPS_ZMMf32_MASKmskw_ZMMf32_MEMf32_AVX512", 
    //                           "V4FMADDSS_XMMf32_MASKmskw_XMMf32_MEMf32_AVX512",
    //                           "V4FNMADDPS_ZMMf32_MASKmskw_ZMMf32_MEMf32_AVX512",
    //                           "VFMSUB132PD_XMMf64_MASKmskw_XMMf64_MEMf64_AVX512",
    //                           "VFNMADD231SS_XMMf32_MASKmskw_XMMf32_MEMf32_AVX512",
    //                           "VGETMANTPD_YMMf64_MASKmskw_YMMf64_IMM8_AVX512",
    //                           "VFNMSUBPS_YMMqq_YMMqq_YMMqq_YMMqq",
    //                           ""};
    // *out << std::setiosflags(ios::left) << " | " << setw(50) << "IFORM" << " | " << setw(15) << "CATEGORY" << " | " << setw(10) << "EXTENSION" << endl;
    // for(int i=0; fma_iform_str[i]!=""; i++) {
    //     xed_iform_enum_t fma_iform = str2xed_iform_enum_t(fma_iform_str[i].c_str());
    //     xed_category_enum_t fma_cat = xed_iform_to_category(fma_iform);
    //     xed_extension_enum_t fma_ext = xed_iform_to_extension(fma_iform);
    //     *out << std::setiosflags(ios::left) << " | " << setw(50) << xed_iform_enum_t2str(fma_iform) << " | " << setw(15) << xed_category_enum_t2str(fma_cat) << " | " << setw(10) << xed_extension_enum_t2str(fma_ext) << setw(10) << CAT_isFMA(fma_cat) << endl;
    // }
    // string s="FPMA_ADD";
    // ljstr(s, 25);
    // std::cout << ljstr(s, 10) << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) ) 
        return Usage();
    
    string fileName = KnobOutputFile.Value();

    if( !fileName.empty() ) 
        out = new std::ofstream(fileName.c_str());

    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);

    // Register function to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    cerr <<  "===============================================" << endl;
    cerr <<  "This application is instrumented by MyPinTool" << endl;
    if( !KnobOutputFile.Value().empty() ) 
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    cerr <<  "===============================================" << endl;

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
