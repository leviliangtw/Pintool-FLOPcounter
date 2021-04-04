
/*! @file
 *  This is a PIN tool to count the FLOP. 
 *  by leviliang
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <map>
#include "control_manager.H"
// #include <sstream>
// #include <iomanip>
// #include <map>
// #include <utility> /* for pair */
// #include <unistd.h>
// #include "mix-fp-state.H"
// using namespace CONTROLLER;

using std::setw;
using std::hex;
using std::cerr;
using std::string;
using std::ios;
using std::endl;
using std::dec;
using std::stringstream;

#if defined(__GNUC__)
#  if defined(TARGET_MAC) || defined(TARGET_WINDOWS)
     // macOS* XCODE2.4.1 gcc and Cgywin gcc 3.4.x only allow for 16b
     // alignment! So we need to pad!
#    define ALIGN_LOCK __attribute__ ((aligned(16)))
#  else
#    define ALIGN_LOCK __attribute__ ((aligned(64)))
#  endif
#else
# define ALIGN_LOCK __declspec(align(64))
#endif
#define INFOS
#define DEBUG
// Force each thread's data to be in its own data cache line so that
// multiple threads do not contend for the same data cache line.
// This avoids the false sharing problem.
#define PADSIZE 32  // 64 byte line size: 64-8-8-8-8 = 32

/* ================================================================== */
// Global variables 
/* ================================================================== */

std::ostream * out = &cerr;

const char *target_image = "matrix_multiplications.exe";

const char *target_routines[] = {
    "multiplyMatrix",
    "multiplySparseMatrix",
    // "print_matrix",
    // "print_sparse_matrix",
    ""  // EOF
};

/* Use "xed_iform_enum_t" for index */
typedef struct InsTable {
    xed_decoded_inst_t *_xedd;
    xed_iclass_enum_t _iclass;
    xed_category_enum_t _cat;
    xed_extension_enum_t _ext;
    UINT64 _execount;
    UINT64 _cmpcount;
    UINT64 _maskcount;
    UINT64 _opdno;
    UINT64 _elemno;
    bool _isFLOP;
    bool _isFMA;
    bool _isScalarSimd;
    bool _isMaskOP;
} INS_TABLE;

// sizeof(RtnCount) = 152
typedef struct RtnCount {
    RTN _rtn;
    string _name;
    string _image;
    UINT64 _address;
    UINT64 _rtnCount;
    UINT64 _icount;
    UINT64 _flopcount;
    INS_TABLE * _instable;
    struct RtnCount * _next;
} RTN_COUNT;

// sizeof(thread_data_t) = 64
class thread_data_t {
  public:
    thread_data_t() : RtnList_len(0), RtnList(0) {}
    UINT64 tid;             // sizeof(UINT64) = 8
    UINT64 RtnList_len;     // sizeof(UINT64) = 8
    RtnCount *RtnList;      // sizeof(RtnCount *) = 8
    UINT8 _pad[PADSIZE];    // sizeof(UINT8*PADSIZE) = 32
    thread_data_t * _next;  // sizeof(thread_data_t *) = 8
};

// Linked list of instruction counts for each routine
RTN_COUNT * RtnList = 0;

// Linked list of instruction counts for each thread
thread_data_t * TdList = 0;

// key for accessing TLS storage in the threads. initialized once in main()
static TLS_KEY tls_key = INVALID_TLS_KEY;

PIN_LOCK pinLock;

UINT32 numThreads = 0;

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

bool RTN_isTargetRoutine(RTN rtn) {
    string funcname = PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_NAME_ONLY);
    for (int i=0; *(target_routines[i]); i++)
        if (strcmp(StripName(funcname.c_str()), target_routines[i]) == 0 ) return true;
    return false;
}

bool XEDD_isFLOP(xed_decoded_inst_t* xedd) {
    xed_category_enum_t cat = xed_decoded_inst_get_category(xedd);
    switch (cat) {
        case XED_CATEGORY_AVX:
        case XED_CATEGORY_AVX2:
        case XED_CATEGORY_AVX512_4FMAPS:
        case XED_CATEGORY_FMA4:
        case XED_CATEGORY_IFMA:
        case XED_CATEGORY_MMX:
        case XED_CATEGORY_SSE:
        case XED_CATEGORY_VFMA:
        case XED_CATEGORY_X87_ALU:
            return true;
        default:
            break;
    }
    return false;
}

bool XEDD_isFMA(xed_decoded_inst_t* xedd) {
    xed_category_enum_t cat = xed_decoded_inst_get_category(xedd);
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

bool XEDD_isScalarSimd(xed_decoded_inst_t* xedd) {
    return xed_decoded_inst_get_attribute(xedd, XED_ATTRIBUTE_SIMD_SCALAR);
}

bool XEDD_isMaskOP(xed_decoded_inst_t* xedd) {
    return xed_decoded_inst_get_attribute(xedd, XED_ATTRIBUTE_MASKOP);
}

thread_data_t* get_tls(THREADID tid) {
    thread_data_t* tdata =
          static_cast<thread_data_t*>(PIN_GetThreadData(tls_key, tid));
    return tdata;
}

void CalculateFLOP(RTN_COUNT * RtnList) {
    int FlopCount, FMA_weight, element;
    for(RTN_COUNT *rc = RtnList; rc; rc = rc->_next) {
        FlopCount = 0;
        for(int i=0; i<XED_IFORM_LAST; i++) {
            if( rc->_instable[i]._isFLOP ) {
                FMA_weight = (rc->_instable[i]._isFMA) ? 2 : 1;
                element = rc->_instable[i]._elemno;
                if( rc->_instable[i]._isMaskOP )
                    rc->_instable[i]._cmpcount = rc->_instable[i]._maskcount * FMA_weight;
                else
                    rc->_instable[i]._cmpcount = rc->_instable[i]._execount * FMA_weight * element;
                FlopCount += rc->_instable[i]._cmpcount;
            }
        }
        rc->_flopcount = FlopCount;
    }
}

int CountOnes(int val) {
    int num = 0;
    while(val) {
        num += val % 2;
        val /= 2;
    }
    return num;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

// This function is called to do simple increasement (+1)
VOID docount(UINT64 * counter, THREADID threadid) {
    PIN_GetLock(&pinLock, threadid+1); // for output
    (*counter)++;
    PIN_ReleaseLock(&pinLock);
}

// This function is called before every routine is executed
VOID PIN_FAST_ANALYSIS_CALL routine_counter_mt(string *rtn_name, THREADID threadid) {
    thread_data_t* tdata = get_tls(threadid);
    RTN_COUNT *rc = new RTN_COUNT;
    rc->_instable = new INS_TABLE[XED_IFORM_LAST];
    for (int i=0; i<XED_IFORM_LAST; i++ ) {
        rc->_instable[i]._execount = 0;
        rc->_instable[i]._cmpcount = 0;
    }
    rc->_name = *rtn_name;
    rc->_icount = 0;
    rc->_flopcount = 0;
    rc->_next = tdata->RtnList;
    tdata->RtnList = rc;
    tdata->RtnList_len += 1;
}

// This function is called before every instruction is executed
VOID PIN_FAST_ANALYSIS_CALL instruction_counter_mt(
    xed_decoded_inst_t* xedd, xed_iform_enum_t iform, 
    UINT64 elemno, 
    bool isFLOP, bool isFMA, bool isScalarSimd, bool isMaskOP, const CONTEXT *ctxt, THREADID threadid) {
    thread_data_t* tdata = get_tls(threadid);
    tdata->RtnList->_icount++;
    tdata->RtnList->_instable[iform]._xedd = xedd;
    tdata->RtnList->_instable[iform]._execount++;
    tdata->RtnList->_instable[iform]._elemno = elemno;
    tdata->RtnList->_instable[iform]._isFLOP = isFLOP;
    tdata->RtnList->_instable[iform]._isFMA = isFMA;
    tdata->RtnList->_instable[iform]._isScalarSimd = isScalarSimd;
    tdata->RtnList->_instable[iform]._isMaskOP = isMaskOP;

    /* Mask Testing -> Need AVX512 instructions for test */
    const xed_inst_t* xedi = xedd->_inst;
    for(int j=0; j<xedi->_noperands; j++) {
        const xed_operand_t* op = xed_inst_operand(xedi, j);
        xed_operand_enum_t op_enum = xed_operand_name(op);
        xed_reg_enum_t reg_enum = xed_decoded_inst_get_reg(xedd, op_enum);
        if( reg_enum >= XED_REG_MASK_FIRST && reg_enum <= XED_REG_MASK_LAST ) {
            REG reg = INS_XedExactMapToPinReg(reg_enum);
            UINT64 value = PIN_GetContextReg(ctxt, reg);
            tdata->RtnList->_instable[iform]._maskcount = CountOnes(value);
            // *out << xed_reg_enum_t2str(reg_enum) << " " << value << " " << CountOnes(value) << endl;
        }
    }
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

                /* Instrument the multiplyMatrix() and multiplySparseMatrix() functions. */
                if ( RTN_isTargetRoutine(rtn) ) {
                    INFOS printf("        [INFOS] Decorated Routine Name: %s\n", RTN_Name(rtn).c_str());
                    // DEBUG printf("        [DEBUG] Undecorated Routine Name: %s\n", functionName.c_str());

                    /* Allocate a counter for this routine */
                    RTN_COUNT * rc = new RTN_COUNT;
                    INS_TABLE *instb = new INS_TABLE[XED_IFORM_LAST];
                    for (int i=0; i<XED_IFORM_LAST; i++ ) {
                        instb[i]._execount = 0;
                        instb[i]._cmpcount = 0;
                    }

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
                    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_rtnCount), IARG_THREAD_ID, IARG_END);
                    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)routine_counter_mt, IARG_FAST_ANALYSIS_CALL,
                       IARG_PTR, &(rc->_name), IARG_THREAD_ID, IARG_END);

                    // PIN_GetFullContextRegsSet();
                    // PIN_GetContextReg (const CONTEXT *ctxt, REG reg)

                    /* For each instruction of the routine */
                    for ( INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins) ) {
                        xed_decoded_inst_t* xedd = INS_XedDec(ins);
                        xed_iform_enum_t iform = xed_decoded_inst_get_iform_enum(xedd);

                        /* Store the basic information of instuctions in the INS_TABLE */
                        if( instb[iform]._xedd == NULL ) {
                            instb[iform]._xedd = new xed_decoded_inst_t;
                            *(instb[iform]._xedd) = *xedd;
                            instb[iform]._iclass = xed_decoded_inst_get_iclass(xedd);
                            instb[iform]._cat = xed_decoded_inst_get_category(xedd);
                            instb[iform]._ext = xed_decoded_inst_get_extension(xedd);
                            instb[iform]._opdno = xed_decoded_inst_noperands(xedd);
                            instb[iform]._elemno = xed_decoded_inst_operand_elements(xedd, 0);
                            instb[iform]._isFLOP = XEDD_isFLOP(xedd);
                            instb[iform]._isFMA = XEDD_isFMA(xedd);
                            instb[iform]._isScalarSimd = XEDD_isScalarSimd(xedd);
                            instb[iform]._isMaskOP = XEDD_isMaskOP(xedd);
                        }

                        /* Insert a call to docount to increment the instruction counter for this rtn */
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_icount), IARG_THREAD_ID, IARG_END);
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(instb[iform]._execount), IARG_THREAD_ID, IARG_END);
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)instruction_counter_mt, IARG_FAST_ANALYSIS_CALL, IARG_PTR, instb[iform]._xedd, IARG_UINT64, iform, IARG_UINT64, instb[iform]._elemno, IARG_BOOL, instb[iform]._isFLOP, IARG_BOOL, instb[iform]._isFMA, IARG_BOOL, instb[iform]._isScalarSimd, IARG_BOOL, instb[iform]._isMaskOP, IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
                    }

                    RTN_Close(rtn);
                }
            }
        }
    }
}

// Note that opening a file in a callback is only supported on Linux systems.
//
// This routine is executed every time a thread is created.
VOID ThreadStart(THREADID threadid, CONTEXT *ctxt, INT32 flags, VOID *v) {
    // This function is locked no need for a Pin Lock here
    numThreads++;

    PIN_GetLock(&pinLock, threadid+1); // for output
    *out << "* Starting tid " << threadid << endl;
    PIN_ReleaseLock(&pinLock);

    thread_data_t* tdata = new thread_data_t;
    tdata->tid = threadid;

    tdata->_next = TdList;
    TdList = tdata;

    if (PIN_SetThreadData(tls_key, tdata, threadid) == FALSE) {
        cerr << "PIN_SetThreadData failed" << endl;
        PIN_ExitProcess(1);
    }
}

// This routine is executed every time a thread is destroyed.
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v) {
    PIN_GetLock(&pinLock, threadid+1);
    *out << "* Stopping tid " << threadid << ", code: " << code << endl;
    PIN_ReleaseLock(&pinLock);

    thread_data_t* tdata = get_tls(threadid);
    // *out << tdata << endl;
    CalculateFLOP(tdata->RtnList);

    // delete tdata;
}

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v) {

    CalculateFLOP(RtnList);
    
    *out <<  "===============================================" << endl;
    *out <<  "           The Total Analysis Result           " << endl;
    *out <<  "===============================================" << endl;
 
    for(RTN_COUNT * rc = RtnList; rc; rc = rc->_next) {

        /* Info of FLOP instructions */
        if(rc->_icount > 0) {

            /* Basic Info */
            *out << "Routine (Procedure): " << rc->_name  << endl
                << "Image:               " << rc->_image  << endl
                << "Address:             " << "0x" << hex << rc->_address << dec  << endl
                << "Calls:               " << setw(10) << rc->_rtnCount  << endl
                << "Instructions counts: " << setw(10) << rc->_icount  << endl
                << "FLOP counts (TODO):  " << setw(10) << rc->_flopcount << endl;

            *out << "FLOP instructions: " << endl
                 << "    " << std::setiosflags(ios::left) 
                 << setw(27) << "[XED_IFORM]"
                 << std::resetiosflags(ios::left) 
                //  << setw(12) << "XED_ICLASS"
                 << setw(12) << "[XED_CAT]"
                 << setw(11) << "[XED_EXT]"
                 << setw(10) << "*[e_cnt]"
                 << setw(10) << "*[c_cnt]"
                 << setw(10) << "*[m_cnt]" 
                 << setw(8) << "*[FMA]"
                 << setw(7) << "*[SS]"
                 << setw(10) << "[MaskOP]"
                 << setw(8) << "[#opd]"
                 << setw(15) << "*[opd1]"
                 << setw(15) << "[opd2]"
                 << setw(15) << "[opd3]"
                 << setw(15) << "[opd4]"
                 << setw(15) << "[opd5]"
                 << endl; 

            for(int i=0; i<XED_IFORM_LAST; i++) {
                if( rc->_instable[i]._isFLOP && rc->_instable[i]._execount ) {
                    *out << "    " << std::setiosflags(ios::left) 
                         << setw(27) << xed_iform_enum_t2str(static_cast<xed_iform_enum_t>(i)) 
                         << std::resetiosflags(ios::left) 
                        //  << setw(12) << xed_iclass_enum_t2str(rc->_instable[i]._iclass)
                         << setw(12) << xed_category_enum_t2str(rc->_instable[i]._cat) 
                         << setw(11) << xed_extension_enum_t2str(rc->_instable[i]._ext)
                         << setw(10) << rc->_instable[i]._execount 
                         << setw(10) << rc->_instable[i]._cmpcount 
                         << setw(10) << rc->_instable[i]._maskcount
                         << setw(8) << rc->_instable[i]._isFMA 
                         << setw(7) << rc->_instable[i]._isScalarSimd 
                         << setw(10) << rc->_instable[i]._isMaskOP
                         << setw(8) << rc->_instable[i]._opdno;
                    for(int j=0; j<(int)xed_decoded_inst_noperands(rc->_instable[i]._xedd); j++) {
                        *out << setw(15) 
                        << decstr(xed_decoded_inst_operand_element_size_bits(rc->_instable[i]._xedd, j)) 
                        + "/" 
                        + xed_operand_element_type_enum_t2str(xed_decoded_inst_operand_element_type(rc->_instable[i]._xedd, j)) 
                        + "/" 
                        + decstr(xed_decoded_inst_operand_elements(rc->_instable[i]._xedd, j));
                    }

                    /* Mask Testing*/
                    // *out << endl; 
                    // *out << "        ";
                    // const xed_inst_t* xedi = rc->_instable[i]._xedd->_inst;
                    // for(int j=0; j<xedi->_noperands; j++) {
                    //     const xed_operand_t* op = xed_inst_operand(xedi, j);
                    //     xed_operand_enum_t op_enum = xed_operand_name(op); 
                    //     xed_reg_enum_t reg_enum = xed_decoded_inst_get_reg(rc->_instable[i]._xedd, op_enum);
                    //     *out << xed_reg_enum_t2str(reg_enum) << " ";
                    // }

                    *out << endl; 

                    /* Mask Testing*/
                    // char buf[100];
                    // // const xed_inst_t* xedi = rc->_instable[i]._xedd->_inst;
                    // xed_operand_values_t* op_value = xed_decoded_inst_operands(rc->_instable[i]._xedd);
                    // // const xed_operand_t* op = xed_inst_operand(xedi, 0);
                    // // xed_operand_values_dump(op_value, buf, 100);
                    // xed_operand_values_print_short(op_value, buf, 100);
                    // // xed_operand_print(op, buf, 100);
                    // xed_bits_t mask = xed3_operand_get_mask(rc->_instable[i]._xedd);
                    // *out << buf << endl;
                    // *out << mask << endl;
                }
            }
            *out << "    * [e_cnt]: Executions Count. " << endl;
            *out << "    * [c_cnt]: Computation Count. " << endl;
            *out << "    * [m_cnt]: Masking Bits Count. " << endl;
            *out << "    * [FMA]:   Fused Multiply-Add. " << endl;
            *out << "    * [SS]:    Scalar SIMD. " << endl;
            *out << "    * [opd]:   Shows bits, type and number of elements (#element). " << endl;
            *out << endl;
        }
    }
 
    /* Deallocate the dynamic memory allocation*/
    for (RTN_COUNT *rc = RtnList; rc;) {
        RTN_COUNT *cur = rc;
        if (rc->_icount > 0) {
            for(int i=0; i<XED_IFORM_LAST; i++) {
                if( rc->_instable[i]._xedd != NULL ) {
                    delete rc->_instable[i]._xedd;
                }
            }
        }
        delete [] rc->_instable;
        rc = rc->_next;
        delete cur;
    }
    
    *out <<  "===============================================" << endl;
    *out <<  "      The Multi-Threading Analysis Result      " << endl;
    *out <<  "===============================================" << endl;

    for(thread_data_t *td = TdList; td; td = td->_next) {
        *out << "Thread ID: " << td->tid << endl;
        *out << "Routine counts: " << td->RtnList_len << endl;
        for (RTN_COUNT * rc = td->RtnList; rc; rc = rc->_next) {

            /* Basic Info */
            *out << "    Routine (Procedure): " << rc->_name  << endl
                 << "    Image:               " << rc->_image  << endl
                 << "    Instructions counts: " << setw(10) << rc->_icount  << endl
                 << "    FLOP counts (TODO):  " << setw(10) << rc->_flopcount 
                 << endl;

            *out << "    FLOP instructions: " << endl
                 << "        " << std::setiosflags(ios::left) 
                 << setw(27) << "[XED_IFORM]"
                 << std::resetiosflags(ios::left) 
                //  << setw(12) << "XED_ICLASS"
                //  << setw(12) << "XED_CAT"
                //  << setw(11) << "XED_EXT"
                 << setw(9) << "[e_cnt]"
                 << setw(9) << "[c_cnt]"
                 << setw(9) << "[m_cnt]" 
                 << setw(7) << "[FMA]"
                 << setw(6) << "[SS]"
                 << setw(10) << "[MaskOP]"
                 << setw(17) << "[#element_opd1]" 
                 << endl;
            for(int i=0; i<XED_IFORM_LAST; i++) {
                if( rc->_instable[i]._isFLOP ) {
                    *out << "        " << std::setiosflags(ios::left) 
                         << setw(27) << xed_iform_enum_t2str(static_cast<xed_iform_enum_t>(i)) 
                         << std::resetiosflags(ios::left) 
                        //  << setw(12) << xed_iclass_enum_t2str(rc->_instable[i]._iclass)
                        //  << setw(12) << xed_category_enum_t2str(rc->_instable[i]._cat) 
                        //  << setw(11) << xed_extension_enum_t2str(rc->_instable[i]._ext)
                         << setw(9) << rc->_instable[i]._execount 
                         << setw(9) << rc->_instable[i]._cmpcount 
                         << setw(9) << rc->_instable[i]._maskcount
                         << setw(7) << rc->_instable[i]._isFMA 
                         << setw(6) << rc->_instable[i]._isScalarSimd 
                         << setw(10) << rc->_instable[i]._isMaskOP 
                         << setw(17) << rc->_instable[i]._elemno;
                        //  << " Test xedd: " << xed_iform_enum_t2str(xed_decoded_inst_get_iform_enum(rc->_instable[i]._xedd))

                    /* Mask Testing */
                    // *out << " Mask Testing (Find register): ";
                    // const xed_inst_t* xedi = rc->_instable[i]._xedd->_inst;
                    // for(int j=0; j<xedi->_noperands; j++) {
                    //     const xed_operand_t* op = xed_inst_operand(xedi, j);
                    //     xed_operand_enum_t op_enum = xed_operand_name(op); 
                    //     xed_reg_enum_t reg_enum = xed_decoded_inst_get_reg(rc->_instable[i]._xedd, op_enum);
                    //     *out << xed_reg_enum_t2str(reg_enum) << " ";
                    // }

                    *out << endl;

                    // char buf[100];
                    // xed_operand_values_t* op_value = xed_decoded_inst_operands(rc->_instable[i]._xedd);
                    // xed_reg_enum_t op_enum = xed_decoded_inst_get_reg(rc->_instable[i]._xedd, XED_OPERAND_);
                    // *out << xed_reg_enum_t2str(op_enum) << endl; 
                    // xed_reg_enum_t op_enum = xed_decoded_inst_get_reg(rc->_instable[i]._xedd, XED_OPERAND_MASK);
                    // *out << xed_reg_enum_t2str(op_enum) << endl; 
                    // xed_operand_values_dump(op_value, buf, 100);
                    // xed_operand_values_print_short(op_value, buf, 100);
                    // xed_operand_print(op, buf, 100);
                    // xed_bits_t mask = xed3_operand_get_mask(rc->_instable[i]._xedd);
                    // *out << buf << endl;
                    // *out << mask << endl;
                    }
            }
        }
        *out << endl;
    }
 
    /* Deallocate the dynamic memory allocation*/
    for(thread_data_t *td = TdList; td;) {
        thread_data_t *td_cur = td;
        for (RTN_COUNT * rc = td->RtnList; rc;) {
            RTN_COUNT * rc_cur = rc;
            delete [] rc->_instable;
            rc = rc->_next;
            delete rc_cur;
        }
        td = td->_next;
        delete td_cur;
    }

    /* IFORM Testing */
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
    // *out << "sizeof(UINT32): " << sizeof(UINT32) << endl;
    // *out << "sizeof(UINT64): " << sizeof(UINT64) << endl;
    // *out << "sizeof(RtnCount): " << sizeof(RtnCount) << endl;
    // *out << "sizeof(RtnCount *): " << sizeof(RtnCount *) << endl;
    // *out << "sizeof(thread_data_t): " << sizeof(thread_data_t) << endl;
}

/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[]) {
    // Initialize the pin lock
    PIN_InitLock(&pinLock);

    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) ) 
        return Usage();
    
    string fileName = KnobOutputFile.Value();

    if( !fileName.empty() ) 
        out = new std::ofstream(fileName.c_str());

    // Obtain  a key for TLS storage.
    tls_key = PIN_CreateThreadDataKey(NULL);
    if (tls_key == INVALID_TLS_KEY)
    {
        cerr << "number of already allocated keys reached the MAX_CLIENT_TLS_KEYS limit" << endl;
        PIN_ExitProcess(1);
    }

    // Register Analysis routines to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

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
