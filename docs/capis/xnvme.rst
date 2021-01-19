.. _sec-c-api-xnvme:

=======
 xnvme
=======

.. _sec-c-api-xnvme-header:

Header
======

.. literalinclude:: ../../include/libxnvme.h
   :language: c

.. _sec-c-api-xnvme-enum:

Enums
=====


.. _sec-c-api-xnvme-enum-XNVME_FILE_OFLG:

XNVME_FILE_OFLG
---------------

.. doxygenenum:: XNVME_FILE_OFLG


.. _sec-c-api-xnvme-enum-xnvme_pr:

xnvme_pr
--------

.. doxygenenum:: xnvme_pr


.. _sec-c-api-xnvme-enum-xnvme_queue_opts:

xnvme_queue_opts
----------------

.. doxygenenum:: xnvme_queue_opts



.. _sec-c-api-xnvme-struct:

Structs
=======


.. _sec-c-api-xnvme-struct-xnvme_be_attr:

xnvme_be_attr
-------------

.. doxygenstruct:: xnvme_be_attr
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_be_attr_list:

xnvme_be_attr_list
------------------

.. doxygenstruct:: xnvme_be_attr_list
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_cmd_ctx:

xnvme_cmd_ctx
-------------

.. doxygenstruct:: xnvme_cmd_ctx
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_enumeration:

xnvme_enumeration
-----------------

.. doxygenstruct:: xnvme_enumeration
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_ident:

xnvme_ident
-----------

.. doxygenstruct:: xnvme_ident
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_lba_range:

xnvme_lba_range
---------------

.. doxygenstruct:: xnvme_lba_range
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_timer:

xnvme_timer
-----------

.. doxygenstruct:: xnvme_timer
   :members:
   :undoc-members:



.. _sec-c-api-xnvme-func:

Functions
=========


.. _sec-c-api-xnvme-func-XNVME_ILOG2:

XNVME_ILOG2
-----------

.. doxygenfunction:: XNVME_ILOG2


.. _sec-c-api-xnvme-func-XNVME_MAX:

XNVME_MAX
---------

.. doxygenfunction:: XNVME_MAX


.. _sec-c-api-xnvme-func-XNVME_MIN:

XNVME_MIN
---------

.. doxygenfunction:: XNVME_MIN


.. _sec-c-api-xnvme-func-XNVME_MIN_S64:

XNVME_MIN_S64
-------------

.. doxygenfunction:: XNVME_MIN_S64


.. _sec-c-api-xnvme-func-XNVME_MIN_U64:

XNVME_MIN_U64
-------------

.. doxygenfunction:: XNVME_MIN_U64


.. _sec-c-api-xnvme-func-xnvme_be_attr_fpr:

xnvme_be_attr_fpr
-----------------

.. doxygenfunction:: xnvme_be_attr_fpr


.. _sec-c-api-xnvme-func-xnvme_be_attr_list_bundled:

xnvme_be_attr_list_bundled
--------------------------

.. doxygenfunction:: xnvme_be_attr_list_bundled


.. _sec-c-api-xnvme-func-xnvme_be_attr_list_fpr:

xnvme_be_attr_list_fpr
----------------------

.. doxygenfunction:: xnvme_be_attr_list_fpr


.. _sec-c-api-xnvme-func-xnvme_be_attr_list_pr:

xnvme_be_attr_list_pr
---------------------

.. doxygenfunction:: xnvme_be_attr_list_pr


.. _sec-c-api-xnvme-func-xnvme_be_attr_pr:

xnvme_be_attr_pr
----------------

.. doxygenfunction:: xnvme_be_attr_pr


.. _sec-c-api-xnvme-func-xnvme_buf_alloc:

xnvme_buf_alloc
---------------

.. doxygenfunction:: xnvme_buf_alloc


.. _sec-c-api-xnvme-func-xnvme_buf_free:

xnvme_buf_free
--------------

.. doxygenfunction:: xnvme_buf_free


.. _sec-c-api-xnvme-func-xnvme_buf_phys_alloc:

xnvme_buf_phys_alloc
--------------------

.. doxygenfunction:: xnvme_buf_phys_alloc


.. _sec-c-api-xnvme-func-xnvme_buf_phys_free:

xnvme_buf_phys_free
-------------------

.. doxygenfunction:: xnvme_buf_phys_free


.. _sec-c-api-xnvme-func-xnvme_buf_phys_realloc:

xnvme_buf_phys_realloc
----------------------

.. doxygenfunction:: xnvme_buf_phys_realloc


.. _sec-c-api-xnvme-func-xnvme_buf_realloc:

xnvme_buf_realloc
-----------------

.. doxygenfunction:: xnvme_buf_realloc


.. _sec-c-api-xnvme-func-xnvme_buf_virt_alloc:

xnvme_buf_virt_alloc
--------------------

.. doxygenfunction:: xnvme_buf_virt_alloc


.. _sec-c-api-xnvme-func-xnvme_buf_virt_free:

xnvme_buf_virt_free
-------------------

.. doxygenfunction:: xnvme_buf_virt_free


.. _sec-c-api-xnvme-func-xnvme_buf_vtophys:

xnvme_buf_vtophys
-----------------

.. doxygenfunction:: xnvme_buf_vtophys


.. _sec-c-api-xnvme-func-xnvme_cmd_ctx_clear:

xnvme_cmd_ctx_clear
-------------------

.. doxygenfunction:: xnvme_cmd_ctx_clear


.. _sec-c-api-xnvme-func-xnvme_cmd_ctx_cpl_status:

xnvme_cmd_ctx_cpl_status
------------------------

.. doxygenfunction:: xnvme_cmd_ctx_cpl_status


.. _sec-c-api-xnvme-func-xnvme_cmd_ctx_from_dev:

xnvme_cmd_ctx_from_dev
----------------------

.. doxygenfunction:: xnvme_cmd_ctx_from_dev


.. _sec-c-api-xnvme-func-xnvme_cmd_ctx_from_queue:

xnvme_cmd_ctx_from_queue
------------------------

.. doxygenfunction:: xnvme_cmd_ctx_from_queue


.. _sec-c-api-xnvme-func-xnvme_cmd_ctx_pr:

xnvme_cmd_ctx_pr
----------------

.. doxygenfunction:: xnvme_cmd_ctx_pr


.. _sec-c-api-xnvme-func-xnvme_cmd_pass:

xnvme_cmd_pass
--------------

.. doxygenfunction:: xnvme_cmd_pass


.. _sec-c-api-xnvme-func-xnvme_cmd_pass_admin:

xnvme_cmd_pass_admin
--------------------

.. doxygenfunction:: xnvme_cmd_pass_admin


.. _sec-c-api-xnvme-func-xnvme_enumerate:

xnvme_enumerate
---------------

.. doxygenfunction:: xnvme_enumerate


.. _sec-c-api-xnvme-func-xnvme_enumeration_fpp:

xnvme_enumeration_fpp
---------------------

.. doxygenfunction:: xnvme_enumeration_fpp


.. _sec-c-api-xnvme-func-xnvme_enumeration_fpr:

xnvme_enumeration_fpr
---------------------

.. doxygenfunction:: xnvme_enumeration_fpr


.. _sec-c-api-xnvme-func-xnvme_enumeration_pp:

xnvme_enumeration_pp
--------------------

.. doxygenfunction:: xnvme_enumeration_pp


.. _sec-c-api-xnvme-func-xnvme_enumeration_pr:

xnvme_enumeration_pr
--------------------

.. doxygenfunction:: xnvme_enumeration_pr


.. _sec-c-api-xnvme-func-xnvme_file_close:

xnvme_file_close
----------------

.. doxygenfunction:: xnvme_file_close


.. _sec-c-api-xnvme-func-xnvme_file_get_cmd_ctx:

xnvme_file_get_cmd_ctx
----------------------

.. doxygenfunction:: xnvme_file_get_cmd_ctx


.. _sec-c-api-xnvme-func-xnvme_file_open:

xnvme_file_open
---------------

.. doxygenfunction:: xnvme_file_open


.. _sec-c-api-xnvme-func-xnvme_file_pread:

xnvme_file_pread
----------------

.. doxygenfunction:: xnvme_file_pread


.. _sec-c-api-xnvme-func-xnvme_file_pwrite:

xnvme_file_pwrite
-----------------

.. doxygenfunction:: xnvme_file_pwrite


.. _sec-c-api-xnvme-func-xnvme_file_stat:

xnvme_file_stat
---------------

.. doxygenfunction:: xnvme_file_stat


.. _sec-c-api-xnvme-func-xnvme_file_sync:

xnvme_file_sync
---------------

.. doxygenfunction:: xnvme_file_sync


.. _sec-c-api-xnvme-func-xnvme_ident_fpr:

xnvme_ident_fpr
---------------

.. doxygenfunction:: xnvme_ident_fpr


.. _sec-c-api-xnvme-func-xnvme_ident_from_uri:

xnvme_ident_from_uri
--------------------

.. doxygenfunction:: xnvme_ident_from_uri


.. _sec-c-api-xnvme-func-xnvme_ident_pr:

xnvme_ident_pr
--------------

.. doxygenfunction:: xnvme_ident_pr


.. _sec-c-api-xnvme-func-xnvme_is_pow2:

xnvme_is_pow2
-------------

.. doxygenfunction:: xnvme_is_pow2


.. _sec-c-api-xnvme-func-xnvme_lba_fpr:

xnvme_lba_fpr
-------------

.. doxygenfunction:: xnvme_lba_fpr


.. _sec-c-api-xnvme-func-xnvme_lba_fprn:

xnvme_lba_fprn
--------------

.. doxygenfunction:: xnvme_lba_fprn


.. _sec-c-api-xnvme-func-xnvme_lba_pr:

xnvme_lba_pr
------------

.. doxygenfunction:: xnvme_lba_pr


.. _sec-c-api-xnvme-func-xnvme_lba_prn:

xnvme_lba_prn
-------------

.. doxygenfunction:: xnvme_lba_prn


.. _sec-c-api-xnvme-func-xnvme_lba_range_fpr:

xnvme_lba_range_fpr
-------------------

.. doxygenfunction:: xnvme_lba_range_fpr


.. _sec-c-api-xnvme-func-xnvme_lba_range_from_offset_nbytes:

xnvme_lba_range_from_offset_nbytes
----------------------------------

.. doxygenfunction:: xnvme_lba_range_from_offset_nbytes


.. _sec-c-api-xnvme-func-xnvme_lba_range_from_slba_elba:

xnvme_lba_range_from_slba_elba
------------------------------

.. doxygenfunction:: xnvme_lba_range_from_slba_elba


.. _sec-c-api-xnvme-func-xnvme_lba_range_from_slba_naddrs:

xnvme_lba_range_from_slba_naddrs
--------------------------------

.. doxygenfunction:: xnvme_lba_range_from_slba_naddrs


.. _sec-c-api-xnvme-func-xnvme_lba_range_from_zdescr:

xnvme_lba_range_from_zdescr
---------------------------

.. doxygenfunction:: xnvme_lba_range_from_zdescr


.. _sec-c-api-xnvme-func-xnvme_lba_range_pr:

xnvme_lba_range_pr
------------------

.. doxygenfunction:: xnvme_lba_range_pr


.. _sec-c-api-xnvme-func-xnvme_queue_get_capacity:

xnvme_queue_get_capacity
------------------------

.. doxygenfunction:: xnvme_queue_get_capacity


.. _sec-c-api-xnvme-func-xnvme_queue_get_cmd_ctx:

xnvme_queue_get_cmd_ctx
-----------------------

.. doxygenfunction:: xnvme_queue_get_cmd_ctx


.. _sec-c-api-xnvme-func-xnvme_queue_get_outstanding:

xnvme_queue_get_outstanding
---------------------------

.. doxygenfunction:: xnvme_queue_get_outstanding


.. _sec-c-api-xnvme-func-xnvme_queue_init:

xnvme_queue_init
----------------

.. doxygenfunction:: xnvme_queue_init


.. _sec-c-api-xnvme-func-xnvme_queue_poke:

xnvme_queue_poke
----------------

.. doxygenfunction:: xnvme_queue_poke


.. _sec-c-api-xnvme-func-xnvme_queue_put_cmd_ctx:

xnvme_queue_put_cmd_ctx
-----------------------

.. doxygenfunction:: xnvme_queue_put_cmd_ctx


.. _sec-c-api-xnvme-func-xnvme_queue_set_cb:

xnvme_queue_set_cb
------------------

.. doxygenfunction:: xnvme_queue_set_cb


.. _sec-c-api-xnvme-func-xnvme_queue_term:

xnvme_queue_term
----------------

.. doxygenfunction:: xnvme_queue_term


.. _sec-c-api-xnvme-func-xnvme_queue_wait:

xnvme_queue_wait
----------------

.. doxygenfunction:: xnvme_queue_wait


.. _sec-c-api-xnvme-func-xnvme_timer_bw_pr:

xnvme_timer_bw_pr
-----------------

.. doxygenfunction:: xnvme_timer_bw_pr


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed:

xnvme_timer_elapsed
-------------------

.. doxygenfunction:: xnvme_timer_elapsed


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_msecs:

xnvme_timer_elapsed_msecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_msecs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_nsecs:

xnvme_timer_elapsed_nsecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_nsecs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_secs:

xnvme_timer_elapsed_secs
------------------------

.. doxygenfunction:: xnvme_timer_elapsed_secs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_usecs:

xnvme_timer_elapsed_usecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_usecs


.. _sec-c-api-xnvme-func-xnvme_timer_pr:

xnvme_timer_pr
--------------

.. doxygenfunction:: xnvme_timer_pr


.. _sec-c-api-xnvme-func-xnvme_timer_start:

xnvme_timer_start
-----------------

.. doxygenfunction:: xnvme_timer_start


.. _sec-c-api-xnvme-func-xnvme_timer_stop:

xnvme_timer_stop
----------------

.. doxygenfunction:: xnvme_timer_stop

