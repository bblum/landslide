###########################################################################
#
#    #####          #######         #######         ######            ###
#   #     #         #     #            #            #     #           ###
#   #               #     #            #            #     #           ###
#    #####          #     #            #            ######             #
#         #         #     #            #            #
#   #     #         #     #            #            #                 ###
#    #####          #######            #            #                 ###
#
###########################################################################
TABSTOP = 4
UPDATE_METHOD = offline
410REQPROGS = idle
410REQBINPROGS = shell init
410TESTS = thr_join_exit thr_exit_join paraguay rwlock_downgrade_read_test rwlock_write_write_test rwlock_dont_starve_writers rwlock_dont_starve_readers broadcast_test broadcast_two_waiters mutex_test paradise_lost htm1 htm2
STUDENTTESTS =
410USER_LIBS_EARLY += libthrgrp.a
