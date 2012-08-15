/*
* File:  tr.cpp
* Copyright (C) 2004 The Institute for System Programming of the Russian Academy of Sciences (ISP RAS)
*/

#include <string>

#include "common/sedna.h"
#include "common/base.h"
#include "common/utils.h"
#include "common/SSMMsg.h"
#include "common/errdbg/d_printf.h"
#include "common/pping.h"
#include "common/ipc_ops.h"
#include "common/errdbg/exceptions.h"

#include "tr/tr_globals.h"
#include "tr/tr_functions.h"
#include "tr/tr_common_funcs.h"
#include "tr/tr_utils.h"
#include "tr/cl_client.h"
#include "tr/socket_client.h"
#include "tr/auth/auc.h"
#include "tr/rcv/rcv_test_tr.h"
#include "tr/rcv/rcv_funcs.h"
#include "common/gmm.h"

#include "tr/opt/OptimizingExecutor.h"
#include "tr/opt/types/ResultQueue.h"

using namespace std;
using namespace tr_globals;

/* variables for time measurement */
DECLARE_TIME_VARS
u_timeb t_total1, t_total2, t_qep1, t_qep2, t_qep;

class ClientRequestNotSupportedException : public SednaUserSoftException
{
public:
    ClientRequestNotSupportedException(const char* _file_, const char* _function_, int _line_, const char* _err_msg_) :
        SednaUserSoftException(_file_, _function_, _line_, _err_msg_) { };
};

class TransactionProcessor
{
    client_core * client;
    SSMMsg * smServer;
    opt::IStatement * currentStatement;
public:
    TransactionProcessor(client_core * _client, SSMMsg * sm_server)
        : client(_client), smServer(sm_server), currentStatement(NULL)
    {
    };

    ~TransactionProcessor()
    {
        delete currentStatement;
    };

    void run()
    {
        msg_struct client_msg;
        bool transactionAlive = true;
        uint64_t total_time = 0;

        on_transaction_begin(smServer, tr_globals::ppc);
        client->respond_to_client(se_BeginTransactionOk);

        try {
            do {
                client->read_msg(&client_msg);
                switch (client_msg.instruction) {
                case se_Authenticate:
                {
                    do_authentication();
                    client->authentication_result(true, "");
                } break;

                case se_ExecuteSchemeProgram:
                {
                    /* Backward compatibility, not currently supported */
                    throw ClientRequestNotSupportedException(EXCEPTION_PARAMETERS, "Scheme program execution is depricated");
                    break;
                }

                case se_ExecuteLong:
                case se_Execute:
                {
                    if (currentStatement != NULL) {
                        delete currentStatement;
                        currentStatement = NULL;
                    }

                    /* Adjust client for the new statement */
                    client->set_result_type((enum se_output_method) (client_msg.body[0]));
                    client->user_statement_begin();

                    currentStatement = new opt::OptimizedStatement(client);

                    currentStatement->prepare(
                        client->get_query_type(),
                        client->get_query_string(&client_msg));

                    currentStatement->execute();

                    /* If no exception, consider query succeded */
                    client->respond_to_client(se_QuerySucceeded);

                    /* Get the first item, no break */
                };
                case se_GetNextItem:
                {
                    if (currentStatement == NULL) {
                        U_ASSERT(false);
                        // Statement not prepared;
                    };

                    currentStatement->next();
                    /* TODO: implement time */
//                        total_time += currentStatement->time;
                } break;
                case se_ShowTime:      //show time
                {
                    client->show_time_ex(total_time);
                    break;
                }
                case se_CommitTransaction:
                case se_RollbackTransaction:
                case se_CloseConnection:
                {
                    if (currentStatement != NULL) {
                        delete currentStatement;
                        currentStatement = NULL;
                    }

                    transactionAlive = false;

                    try {
                        on_transaction_end(
                          smServer,
                          client_msg.instruction == se_CommitTransaction,
                          tr_globals::ppc);
                    } catch (std::exception & exception) {
                        throw SYSTEM_EXCEPTION(exception.what());
                    };

                    switch (client_msg.instruction) {
                      case se_CommitTransaction:
                        client->respond_to_client(se_CommitTransactionOk); break;
                      case se_RollbackTransaction:
                        client->respond_to_client(se_RollbackTransactionOk); break;
                      case se_CloseConnection:
                        client->respond_to_client(se_TransactionRollbackBeforeClose); break;
                      default :
                        U_ASSERT(false);
                    };
                } break;

                default:
                  client->process_unknown_instruction(client_msg.instruction, true);
                }
            } while (transactionAlive);
        } catch (SednaUserEnvException & userException) {
            throw;
        } catch (SednaUserException & userException) {
            /* Any exception thrown from this block is considered system */

            try {
                on_transaction_end(smServer, false, tr_globals::ppc);
            } catch (std::exception & exception) {
                throw SYSTEM_EXCEPTION(exception.what());
            };

            if (userException.getCode() == SE3053) {
                client->authentication_result(false, userException.what());
            } else {
                client->error(userException.getCode(), userException.what());
            }
        } catch (std::exception & exception) {
            throw;
        }
    };
};

int TRmain(int argc, char *argv[])
{
    /* volatile to prevent clobbing by setjmp/longjmp */
    volatile int ret_code = 0;
    volatile bool sedna_server_is_running = false;
    program_name_argv_0 = argv[0]; /* we use it to get full binary path */
    tr_globals::ppc = NULL; /* pping client */
    char buf[1024]; /* buffer enough to get environment variables */
    SSMMsg *sm_server = NULL; /* shared memory messenger to communicate with SM */
    int determine_vmm_region = -1;
    int os_primitives_id_min_bound;
    SednaUserException e = USER_EXCEPTION(SE4400);

    try
    {
        if (uGetEnvironmentVariable(SEDNA_OS_PRIMITIVES_ID_MIN_BOUND, buf, 1024, NULL) != 0)
            /* Default value for command line only */
            os_primitives_id_min_bound = 1500;
        else
            os_primitives_id_min_bound = atoi(buf);

        INIT_TOTAL_TIME_VARS u_ftime(&t_total1);

        /*
         * determine_vmm_region specifies db_id for which to search layer_size
         * db_id is needed since we must report back to the proper cdb
         */
        if (uGetEnvironmentVariable(SEDNA_DETERMINE_VMM_REGION, buf, 1024, NULL) != 0)
            determine_vmm_region = -1;
        else
            determine_vmm_region = atoi(buf);

        /* Probably it is wrong since we get region info from global shared memory and if we
         * run tr from command line (no environment variable set) and non-default id_min_bound
         * is used we won't find the shmem. Also we can erroneously access shmem from the
         * wrong Sedna installation which is not good (different vmm region settings are
         * possible though the probability is low, however the worse thing is that we wreck isolation
         * of unrelated installations; we may even fail if the shmem is created by the
         * installation running as a different user). */

        InitGlobalNames(os_primitives_id_min_bound, INT_MAX);
        SetGlobalNames();

        if (determine_vmm_region != -1)
        {
            SetGlobalNamesDB(determine_vmm_region);
            vmm_determine_region();
            ReleaseGlobalNames();
            return 0;
        }
        else
        {
            try {
                open_global_memory_mapping(SE4400);
                close_global_memory_mapping();
                sedna_server_is_running = true;
            } catch (SednaUserException &e) {
                if (e.getCode() != SE4400) throw;
            }
            OS_EXCEPTIONS_INSTALL_HANDLER
        }

        ReleaseGlobalNames();

#ifdef REQUIRE_ROOT
        if (!uIsAdmin(__sys_call_error)) throw USER_EXCEPTION(SE3064);
#endif /* REQUIRE_ROOT */

        /* Determine if we run via GOV or via command line */
        bool server_mode = false;

        if (uGetEnvironmentVariable(SEDNA_SERVER_MODE, buf, 1024, __sys_call_error) == 0)
            server_mode = (atoi(buf) == 1);

        if (server_mode) {
            client = new socket_client();
        }
        else
        {
            /* We got load metadata transaction started by CDB*/
            first_transaction = (uGetEnvironmentVariable(SEDNA_LOAD_METADATA_TRANSACTION, buf, 1024, __sys_call_error) == 0);

            /* We got recovery transaction started ny SM */
            run_recovery = (uGetEnvironmentVariable(SEDNA_RUN_RECOVERY_TRANSACTION, buf, 1024, __sys_call_error) == 0);

            /* We don't allow running se_trn directly */
            if (strcmp(ACTIVE_CONFIGURATION, "Release") == 0 && !first_transaction && !run_recovery)
                throw USER_EXCEPTION(SE4613);

            client = new command_line_client(argc, argv);
            if (!sedna_server_is_running) throw USER_EXCEPTION(SE4400);
        }

        if (uSocketInit(__sys_call_error) != 0) throw USER_EXCEPTION(SE3001);

        client->init();
        client->get_session_parameters();

        /* init global names */
        InitGlobalNames(client->get_os_primitives_id_min_bound(), INT_MAX);
        SetGlobalNames();

        open_gov_shm();

        /* get global configuration */
        socket_port     = GOV_HEADER_GLOBAL_PTR -> lstnr_port_number;
        strcpy(gov_address, GOV_HEADER_GLOBAL_PTR -> lstnr_addr);
        SEDNA_DATA      = GOV_HEADER_GLOBAL_PTR -> SEDNA_DATA;
        max_stack_depth = GOV_HEADER_GLOBAL_PTR -> pp_stack_depth;

        /* check if database exists */
        int db_id = get_db_id_by_name(GOV_CONFIG_GLOBAL_PTR, db_name);

        /* there is no such database in governor */
        if (db_id == -1) throw USER_EXCEPTION2(SE4200, db_name);

        SetGlobalNamesDB(db_id);

        if (!run_recovery) {
            /* register session on governor */
            register_session_on_gov();
        }
        else {
            /* cannot register for recovery since the
             * database has status 'not started' */
            sid = 0;
        }

        tr_globals::ppc = new pping_client(GOV_HEADER_GLOBAL_PTR -> ping_port_number,
                                           run_recovery ? EL_RCV : EL_TRN,
                                           run_recovery ? NULL : &tr_globals::is_timer_fired);
        tr_globals::ppc->startup(e);

        event_logger_init((run_recovery) ? EL_RCV : EL_TRN, db_name, SE_EVENT_LOG_SHARED_MEMORY_NAME, SE_EVENT_LOG_SEMAPHORES_NAME);
        event_logger_set_sid(sid);

        if (!run_recovery && !first_transaction)
        {
            /* doing somehing only for command line client */
            client->write_user_query_to_log();
            /* doing something only for command line client */
            client->set_keep_alive_timeout(GOV_HEADER_GLOBAL_PTR -> ka_timeout);
            /* set keyboard handlers */
            set_trn_ctrl_handler();
        }

        msg_struct client_msg;

        /* transaction initialization */
        on_session_begin(sm_server, db_id, run_recovery);
        elog(EL_LOG, ("Session is ready"));

        bool expect_another_transaction = !run_recovery;

#ifdef RCV_TEST_CRASH
        rcvReadTestCfg(); // prepare recovery tester
#endif

        /* recovery routine is run instead of transaction mix */
        if (run_recovery)
        {
            on_transaction_begin(sm_server, tr_globals::ppc, true); // true means recovery is active
            on_kernel_recovery_statement_begin();

            recover_db_by_logical_log();

            on_kernel_recovery_statement_end();
            on_transaction_end(sm_server, true, tr_globals::ppc, true);
        }

        /////////////////////////////////////////////////////////////////////////////////
        /// CYCLE BY TRANSACTIONS
        /////////////////////////////////////////////////////////////////////////////////
        while (expect_another_transaction)
        {
            client->read_msg(&client_msg);
            switch (client_msg.instruction) {
                case se_BeginTransaction:
                {
                    TransactionProcessor transaction(client, sm_server);
                    transaction.run();
                } break;
                case se_CloseConnection:
                {
                    client->respond_to_client(se_CloseConnectionOk);
                    client->release();
                    delete client;
                    expect_another_transaction = false;
                } break;
                case se_ShowTime:
                {
                    client->show_time_ex(0);
                } break;
                default:
                    client->process_unknown_instruction(client_msg.instruction, false); break;
            };
        }

        /////////////////////////////////////////////////////////////////////////////////
        /// END OF CYCLE BY TRANSACTIONS
        /////////////////////////////////////////////////////////////////////////////////

        on_session_end(sm_server);

        if (run_recovery)
            elog(EL_LOG, ("recovery process by logical log finished"));
        else
            elog(EL_LOG, ("Session is closed"));


        if (show_time == 1)
        {
            u_ftime(&t_total2);
            string total_time = to_string(t_total2 - t_total1);
            cerr << "\nStatistics: total time: " << total_time.c_str() << " secs\n";
            cerr << "<step>\t\t\t<time>\n";
            cerr << endl;
#ifdef VMM_GATHER_STATISTICS
            cerr << "vmm_different_blocks_touched : " << vmm_different_blocks_touched() << endl;
            cerr << "vmm_blocks_touched           : " << vmm_blocks_touched() << endl;
            cerr << "vmm_different_blocks_modified: " << vmm_different_blocks_modified() << endl;
            cerr << "vmm_blocks_modified          : " << vmm_blocks_modified() << endl;
            cerr << "vmm_data_blocks_allocated    : " << vmm_data_blocks_allocated() << endl;
            cerr << "vmm_tmp_blocks_allocated     : " << vmm_tmp_blocks_allocated() << endl;
            cerr << "vmm_number_of_checkp_calls   : " << vmm_number_of_checkp_calls() << endl;
            cerr << "vmm_number_of_sm_callbacks   : " << vmm_number_of_sm_callbacks() << endl;
            cerr << endl;
#endif
            PRINT_DEBUG_TIME_RESULTS}

        event_logger_release();
        tr_globals::ppc->shutdown();
        delete tr_globals::ppc;
        tr_globals::ppc = NULL;

        if (!run_recovery)
            set_session_finished();

        event_logger_set_sid(-1);

        close_gov_shm();

        uSocketCleanup(__sys_call_error);

        d_printf1("Transaction has been closed\n\n");

        /* tell SM that we are done */
        if (run_recovery)
        {
            USemaphore rcv_signal_end;

            /* signal to SM that we are finished */
            if (0 != USemaphoreOpen(&rcv_signal_end, CHARISMA_DB_RECOVERED_BY_LOGICAL_LOG, __sys_call_error))
                throw SYSTEM_EXCEPTION("Cannot open CHARISMA_DB_RECOVERED_BY_LOGICAL_LOG!");

            if (0 != USemaphoreUp(rcv_signal_end, __sys_call_error))
                throw SYSTEM_EXCEPTION("Cannot up CHARISMA_DB_RECOVERED_BY_LOGICAL_LOG!");

            if (0 != USemaphoreClose(rcv_signal_end, __sys_call_error))
                throw SYSTEM_EXCEPTION("Cannot close CHARISMA_DB_RECOVERED_BY_LOGICAL_LOG!");
        }
    }
    catch(SednaUserException & e)
    {
        fprintf(stderr, "%s\n", e.what());

        on_session_end(sm_server);
        elog(EL_LOG, ("Session is closed"));
        try
        {
            if (client != NULL)
            {
                if (e.getCode() == SE3053)
                    client->authentication_result(false, e.what());
                else
                    client->error(e.getCode(), e.what());

                client->release();
                delete client;
            }
        }
        catch(ANY_SE_EXCEPTION)
        {
            d_printf1("Connection with client has been broken\n");
        }
        event_logger_release();
        if (tr_globals::ppc)
        {
            tr_globals::ppc->shutdown();
            delete tr_globals::ppc;
            tr_globals::ppc = NULL;
        }
        set_session_finished();
        close_gov_shm();
        uSocketCleanup(__sys_call_error);
        ret_code = 1;
    }
    catch(SednaException & e)
    {
        sedna_soft_fault(e, EL_TRN);
    }
    catch(ANY_SE_EXCEPTION)
    {
        sedna_soft_fault(EL_TRN);
    }

    return ret_code;
}
